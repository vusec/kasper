#include "kspecem_bugs.h"

#include "kspecem_common.h"
#include "kspecem_assert.h"
#include "kspecem_undolog.h"
#include "kspecem_utils.h"
#include "kspecem_registers.h"
#include "kspecem_report.h"
#include "kspecem_whitelist.h"

extern char _text[], _etext[];

/* print max one report for each ip by default */
report_print_level_enum kspecem_print_level = PRINT_DEFAULT;

int volatile kspecem_next_bug_index;
struct kspecem_bug *kspecem_bugs;
uint8_t **kspecem_previous_bugs_map;

void kspecem_report_clear_globals(void) {
  // THIS MEMSET ASSUMES EVERYTHING CAN BE SET TO 0/false/NULL
  kspecem_memset(kspecem_bugs, 0, BUGS_LIST_SIZE*sizeof(struct kspecem_bug));
  kspecem_next_bug_index = 0;
}

void kspecem_report_init(void) {
  int i;
  uint64_t kernel_size = (uint64_t)_etext-(uint64_t)_text;
  uint64_t bitmap_size = (kernel_size+PAGE_SIZE) & ~0xfff;
  // compute amount of 1024 * pages blocks
  uint64_t alloc_blocks = bitmap_size / (1024*PAGE_SIZE);

  kasan_save_enable_multi_shot();

  if (bitmap_size % (1024*PAGE_SIZE) != 0) {
    alloc_blocks += 1;
  }

  kspecem_previous_bugs_map = kzalloc(sizeof(uint8_t *)*alloc_blocks, GFP_KERNEL);
  for (i = 0; i < alloc_blocks; i++) {
    kspecem_previous_bugs_map[i] = kzalloc(1024*PAGE_SIZE, GFP_KERNEL);
  }

  kspecem_bugs = kmalloc(sizeof(struct kspecem_bug)*BUGS_LIST_SIZE, GFP_KERNEL);
  lt_assert(kspecem_bugs);
  kspecem_report_clear_globals();
}

/********************************/

static uint8_t kspecem_get_bug_mask(struct kspecem_bug *bug) {
  uint8_t mask = 0;
  switch(bug->access_type) {
    case MDS_ACCESS:
      mask |= 0x1;
      break;
    case CC_ACCESS:
      mask |= 0x2;
      break;
    case KASAN_ACCESS:
      mask |= 0x4;
      break;
    case PC_ACCESS:
      mask |= 0x8;
      break;
    default:
      return 0xff;
      break;
  }
  return mask;
}

static uint64_t kspecem_get_offset_by_ip(uint64_t ip) {
  uint64_t kernel_size = (uint64_t)_etext-(uint64_t)_text;
  uint64_t bitmap_size = (kernel_size+PAGE_SIZE) & ~0xfff;
  uint64_t offset;

  lt_assert(ip >= (uint64_t)_text && ip < (uint64_t)_etext);

  offset = ip - (uint64_t)_text;
  lt_assert(offset < bitmap_size);
  return offset;
}

static bool kspecem_bug_already_reported(struct kspecem_bug *bug) {
  uint64_t ip = bug->kinfo.ip;
  uint64_t offset = kspecem_get_offset_by_ip(ip);

  uint8_t bug_mask = kspecem_get_bug_mask(bug);

  uint64_t alloc_block = offset / (1024 * PAGE_SIZE);
  uint64_t block_offset = offset % (1024 * PAGE_SIZE);
  if ((kspecem_previous_bugs_map[alloc_block][block_offset] & bug_mask)
      == bug_mask || bug_mask == 0xff) {
    return true;
  }
  return false;
}

static void kspecem_bug_set_ip_as_reported(struct kspecem_bug *bug) {
  uint64_t ip = bug->kinfo.ip;
  uint64_t offset = kspecem_get_offset_by_ip(ip);

  uint8_t bug_mask = kspecem_get_bug_mask(bug);

  uint64_t alloc_block = offset / (1024 * PAGE_SIZE);
  uint64_t block_offset = offset % (1024 * PAGE_SIZE);
  kspecem_previous_bugs_map[alloc_block][block_offset] |= bug_mask;
}

static bool kspecem_should_report_access_bug(struct kspecem_bug *bug) {
  if (kspecem_print_level == PRINT_NONE) {
    return false; // never print reports
  } else if (kspecem_print_level == PRINT_DUPLICATES) {
    return true; // print all reports, including duplicates
  }
  // print if not a duplicate
  lt_assert(kspecem_print_level == PRINT_DEFAULT);
  if (kspecem_bug_already_reported(bug)) {
    return false;
  } else {
    kspecem_bug_set_ip_as_reported(bug);
    return true;
  }
}

void kspecem_report_print_and_clear(bool called_from_restart, int restart_type) {
  int i;
  struct kspecem_bug *this_bug;
  bool reports_printed = false;
  unsigned long current_syscall_nr = kspecem_syscall_get_nr();
  void *current_checkpoint_ip = called_from_restart ? (void *)kspecem_registers.ip : NULL;

  // Check if there are no pending reports
  if (kspecem_next_bug_index == 0)
    return;

  for(i = 0; i < kspecem_next_bug_index; i++) {
    this_bug = &kspecem_bugs[i];

    if(!kspecem_should_report_access_bug(this_bug))
      continue;
    if (!reports_printed) {
      kspecem_report_print_restart_begin();
      reports_printed = true;
    }
    kspecem_report_print_access_bug(this_bug);
  }
  if(reports_printed)
    kspecem_report_print_restart_end(current_syscall_nr,
        current_checkpoint_ip, restart_type);

  // Unset kspecem_report global variables
  kspecem_report_clear_globals();
}

/********************************/

static void __always_inline kspecem_access_report(unsigned long addr, size_t size,
    bool is_write, unsigned long ip,
    dfsan_label data_label, dfsan_label ptr_label,
    access_type_enum access_type) {
  struct kspecem_bug *this_bug;

  // Assert that we have not exceeded the max number of pending reports
  lt_assert(kspecem_next_bug_index < BUGS_LIST_SIZE);

  this_bug = &kspecem_bugs[kspecem_next_bug_index];
  kspecem_next_bug_index++;

  // Assert that we are not overwriting a previous bug
  lt_assert(kspecem_checkzero(this_bug,sizeof(*this_bug)));

  // Initialize KASAN bug fields
  this_bug->kinfo.access_addr = (void *)addr;
  this_bug->kinfo.first_bad_addr = (void *)addr;
  this_bug->kinfo.access_size = size;
  this_bug->kinfo.is_write = is_write;
  this_bug->kinfo.ip = ip;
  this_bug->in_spec = kspecem_in_speculative_emulation;
  this_bug->spec_length = kspecem_speculative_instr_count;
  this_bug->data_label.label = data_label;
  this_bug->ptr_label.label = ptr_label;
  this_bug->access_type = access_type;
  dfsan_copy_label_info(data_label, this_bug->data_label.info, LABEL_INFO_STR_MAX);
  dfsan_copy_label_info(ptr_label, this_bug->ptr_label.info, LABEL_INFO_STR_MAX);

  // If not in speculation, then print the report now,
  // otherwise wait until restart_hook to print
  if(!kspecem_in_speculative_emulation) {
    kspecem_report_print_and_clear(false, -1);
  }
}

void __always_inline kspecem_kasan_report(unsigned long addr, size_t size,
    bool is_write, unsigned long ip) {
  kspecem_access_report(addr, size, is_write, ip, 0, 0, KASAN_ACCESS);
}

void __always_inline kspecem_ridl_report(unsigned long addr, size_t size,
    bool is_write, unsigned long ip,
    dfsan_label data_label, dfsan_label ptr_label) {
  kspecem_access_report(addr, size, is_write, ip, data_label, ptr_label, MDS_ACCESS);
}

void __always_inline kspecem_specv1_report(unsigned long addr, size_t size,
    bool is_write, unsigned long ip,
    dfsan_label data_label, dfsan_label ptr_label) {
  kspecem_access_report(addr, size, is_write, ip, data_label, ptr_label, CC_ACCESS);
}

void __always_inline kspecem_smotherspectre_report(unsigned long ip, dfsan_label label) {
  kspecem_access_report(0, 0, 0, ip, 0, label, PC_ACCESS);
}
