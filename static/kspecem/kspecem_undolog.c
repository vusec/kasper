#include "kspecem_undolog.h"

#include "kspecem_assert.h"
#include "kspecem_common.h"
#include "kspecem_internal.h"
#include "kspecem_statistics.h"
#include "kspecem_stats_restarts.h"

uint64_t *kspecem_writelog_addrs;
uint64_t *kspecem_writelog_data;
uint64_t *kspecem_writelog_meta;
uint64_t kspecem_wl_position;

bool kspecem_writelog_enabled = false;

int kspecem_overlaps(const void *p1, size_t s1, const void *p2, size_t s2) {
	const char *ps1 = p1, *pe1 = ps1 + s1;
	const char *ps2 = p2, *pe2 = ps2 + s2;
	return (ps1 <= ps2 && pe1 > ps2) || (ps2 <= ps1 && pe2 > ps1);
}

int kspecem_can_restore(const void *addr) {
	return (
      !kspecem_overlaps(addr, sizeof(uint64_t), kspecem_writelog_addrs, WRITELOG_BYTES) &&
      !kspecem_overlaps(addr, sizeof(uint64_t), kspecem_writelog_data, WRITELOG_BYTES) &&
      !kspecem_overlaps(addr, sizeof(uint64_t), &kspecem_writelog_addrs, sizeof(kspecem_writelog_addrs)) &&
      !kspecem_overlaps(addr, sizeof(uint64_t), &kspecem_writelog_data, sizeof(kspecem_writelog_data)) &&
      !kspecem_overlaps(addr, sizeof(uint64_t), &kspecem_wl_position, sizeof(kspecem_wl_position)));
}

void kspecem_writelog_init(void) {
  void *writelog_addrs_bytes = kmalloc(WRITELOG_BYTES, GFP_KERNEL);
  void *writelog_data_bytes = kmalloc(WRITELOG_BYTES, GFP_KERNEL);
  void *kspecem_writelog_meta_bytes = kmalloc(WRITELOG_BYTES, GFP_KERNEL);

  kspecem_writelog_meta = NULL;
  kspecem_wl_position = 0;
  kspecem_writelog_enabled = false;

  lt_assert(writelog_addrs_bytes);
  lt_assert(writelog_data_bytes);
  lt_assert(kspecem_writelog_meta_bytes);

  kspecem_writelog_meta = (uint64_t *)kspecem_writelog_meta_bytes;
  kspecem_writelog_data = (uint64_t *)writelog_data_bytes;
  kspecem_writelog_addrs = (uint64_t *)writelog_addrs_bytes;
}

void __always_inline kspecem_store(void *addr)
{
  kspecem_write_wl(addr, __builtin_return_address(0));
}

void __always_inline kspecem_memcpy(char *addr, size_t size)
{
	char *end = addr + size;
	uint64_t *reg = (uint64_t *)((uintptr_t)addr & ~0x3);
	while ((char *)reg < end) {
		kspecem_write_wl(reg, __builtin_return_address(0));
		reg++;
	}
}

void __always_inline kspecem_write_wl(uint64_t *addr, uint64_t *ret_addr) {
	uint64_t *wl_addrs_start = kspecem_writelog_addrs;
	uint64_t *wl_addrs_end = kspecem_writelog_addrs + (WRITELOG_MAXLEN * 2);
	uint64_t *wl_data_start = kspecem_writelog_data;
	uint64_t *wl_data_end = kspecem_writelog_data + (WRITELOG_MAXLEN * 2);

  uint64_t **addr_slot;
  uint64_t *region;
  void **addr_slot_meta;

  if (addr != (uint64_t *)((uintptr_t)addr & ~(WRITELOG_GRANULARITY-1))) {
    /* if address is not aligned, save both addresses */
    kspecem_write_wl((uint64_t *)(
          (uintptr_t)addr & ~(WRITELOG_GRANULARITY-1)) + 1, ret_addr);
  }
	/* align address to region boundary */
	addr = (uint64_t *)((uintptr_t)addr & ~(WRITELOG_GRANULARITY-1));

  lt_assert(kspecem_writelog_enabled == kspecem_in_speculative_emulation);
  lt_assert(kspecem_writelog_addrs);
  lt_assert(kspecem_writelog_data);

  /* add bounds checking, when writelog is full */
  if (&kspecem_writelog_addrs[kspecem_wl_position] >= wl_addrs_end ||
      &kspecem_writelog_data[kspecem_wl_position] >= wl_data_end) {
    kspecem_restart(0x0, KSPECEM_RESTART_WRITELOG_FULL, (uint64_t)ret_addr, 0x0, 0x0, 0x0);
  }
  lt_assert(&kspecem_writelog_addrs[kspecem_wl_position] < wl_addrs_end);
  lt_assert(&kspecem_writelog_data[kspecem_wl_position] < wl_data_end);
  lt_assert(addr < wl_addrs_start || addr >= wl_addrs_end);
  lt_assert(addr < wl_data_start || addr >= wl_data_end);

  if ((void *)addr == (void *)&kspecem_wl_position) {
    return;
  }
  if ((void *)addr == (void *)&kspecem_writelog_enabled) {
    return;
  }

	addr_slot = (uint64_t **)&kspecem_writelog_addrs[kspecem_wl_position];
	region = (uint64_t *)&kspecem_writelog_data[kspecem_wl_position];
	addr_slot_meta  = (void **)&kspecem_writelog_meta[kspecem_wl_position];

  if ((uint64_t)addr_slot != 0x0) {
    *addr_slot   = addr;
    *region      = *addr;
    *addr_slot_meta = ret_addr;

    kspecem_wl_position += 1;
  }
}

int kspecem_restore_writelog(uint64_t arg, void *frameaddr, struct pt_regs *regs) {
  void *addr=NULL;
	uint64_t *region;

  uint64_t num_entries = kspecem_wl_position;
  uint64_t i = 0;

  void *rsp;
  asm( "mov %%rsp, %0" : "=rm" ( rsp ));
  lt_assert(rsp);

  /* Walk the log in reverse order and restore entries. */
  while(kspecem_wl_position >= 1) {
    i++;
    kspecem_wl_position--;

    if(NULL == __memcpy(&addr, &kspecem_writelog_addrs[kspecem_wl_position], sizeof(addr)))
      return -1;

    /* make sure that no stack values are restored,
     * that are currently used within the static lib */
    if (addr > rsp && addr <= frameaddr) {
      continue;
    }
    lt_assert(addr <= rsp || addr > frameaddr);

    /* make sure that regs (passed to interrupt handlers)
     * is not corrupted by stack restores */
    if (regs != 0x0) {
      if (addr >= (void *)regs && addr < ((void *)regs)+sizeof(struct pt_regs)) {
        continue;
      }
    }
    lt_assert(kspecem_can_restore(addr));
    lt_assert(&i != addr);
    lt_assert(&num_entries != addr);

    region = (uint64_t *)&kspecem_writelog_data[kspecem_wl_position];
    lt_assert(__memcpy(addr, region, sizeof(uint64_t)) != NULL);
  }
	lt_assert(i == num_entries);
	lt_assert(kspecem_wl_position == 0);
  return 0;
}
