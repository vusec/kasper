#include "kspecem_statistics.h"

#include "kspecem_assert.h"
#include "kspecem_common.h"

#define KSPECEM_SPEC_STATISTICS_SIZE 10000

kspecem_spec_stats_entry_t *kspecem_spec_statistics;
int kspecem_curr_spec_statistics_index;

void kspecem_statistics_init(void) {
  kspecem_spec_statistics = kzalloc(
      sizeof(kspecem_spec_stats_entry_t) * KSPECEM_SPEC_STATISTICS_SIZE, GFP_KERNEL);
  kspecem_curr_spec_statistics_index = 0;
}

void kspecem_add_spec_statistics(uint64_t func_type, uint64_t func_arg1,
    uint64_t func_arg2, uint64_t func_arg3, uint64_t return_addr) {
  kspecem_spec_stats_entry_t *entry;
  if (kspecem_curr_spec_statistics_index >= KSPECEM_SPEC_STATISTICS_SIZE)
    return;
  lt_assert(kspecem_curr_spec_statistics_index < KSPECEM_SPEC_STATISTICS_SIZE);

  entry = &kspecem_spec_statistics[kspecem_curr_spec_statistics_index++];
  entry->kspecem_function_type = func_type;
  entry->kspecem_function_arg1 = func_arg1;
  entry->kspecem_function_arg2 = func_arg2;
  entry->kspecem_function_arg3 = func_arg3;
  entry->kspecem_function_return_addr = return_addr;
}
