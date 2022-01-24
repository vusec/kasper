#include "kspecem_report.h"

#include "kspecem_assert.h"
#include "kspecem_bugs.h"
#include "kspecem_common.h"
#include "kspecem_print.h"

bool kspecem_report_error_dump_stack = 1;

static void kspecem_report_print_bug_begin(const char *bug_type_str,
    struct kspecem_label_info data_label, struct kspecem_label_info ptr_label,
    bool in_spec, unsigned int spec_length) {
  kspecem_printf("vvvv BEGIN REPORT vvvv\n");
  kspecem_printf("*** %s report with data_label = %d, ptr_label = %d, "
      "in_spec = %s, report_spec_length = %u\n",
      bug_type_str,
      data_label.label,
      ptr_label.label,
      in_spec ? "true" : "false",
      spec_length);
  kspecem_printf("*** data_%s\n", data_label.info);
  kspecem_printf("*** ptr_%s\n", ptr_label.info);
}

static void kspecem_report_print_bug_end(void) {
  kspecem_printf("^^^^  END REPORT  ^^^^\n");
}

bool kasan_report_original(unsigned long addr, size_t size, bool is_write,
    unsigned long ip);

void kspecem_report_print_access_bug(
    struct kspecem_bug *this_bug) {
  char *access_type_str;
  switch(this_bug->access_type) {
    case KASAN_ACCESS:
      access_type_str = "KASAN";
      break;
    case MDS_ACCESS:
      access_type_str = "MDS";
      break;
    case CC_ACCESS:
      access_type_str = "CC";
      break;
    case PC_ACCESS:
      access_type_str = "PC";
      break;
    default:
      lt_panic("Trying to print an access report with an unknown access type!\n");
      break;
  }

  kspecem_report_print_bug_begin(access_type_str,
      this_bug->data_label, this_bug->ptr_label,
      this_bug->in_spec, this_bug->spec_length);

  kasan_report_original((unsigned long)this_bug->kinfo.access_addr,
      this_bug->kinfo.access_size, this_bug->kinfo.is_write,
      this_bug->kinfo.ip);

  kspecem_report_print_bug_end();
}

/********************************/

void kspecem_report_print_restart_begin(void) {
  kspecem_printf("vvvvvvvvvvvvvvvv BEGIN RESTART vvvvvvvvvvvvvvvv\n");

}

void show_stack(struct task_struct *task, unsigned long *sp, const char *loglvl);
void kspecem_report_print_restart_end(unsigned long current_syscall_nr,
    void *current_checkpoint_ip, int restart_type) {
  kspecem_printf("* Restart with current_syscall_nr = %lu, "
      "current_checkpoint_ip = %pS(0x%px), restart_type: %u, "
      "restart_spec_length: %u\n",
      current_syscall_nr, current_checkpoint_ip,
      (void *)current_checkpoint_ip, restart_type,
      kspecem_speculative_instr_count);

  if(kspecem_report_error_dump_stack) show_stack(NULL, NULL, KERN_ERR);
  kspecem_printf("^^^^^^^^^^^^^^^^  END RESTART  ^^^^^^^^^^^^^^^^\n");
}
