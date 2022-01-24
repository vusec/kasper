#ifndef KSPECEM_KSPECEM_STATISTICS_H
#define KSPECEM_KSPECEM_STATISTICS_H 1

#include "kspecem_types.h"

enum kspecem_func_type_e {
	KSPECEM_FUNC_NEW_CHECKPOINT = 0,
	KSPECEM_FUNC_NEW_CHECKPOINT_INSPEC,
	KSPECEM_FUNC_RESTART,
	KSPECEM_FUNC_RESTART_RETURN,
	KSPECEM_FUNC_CHECK_CALL_DEPTH,
	__NUM_FUNC_TYPES
};
typedef struct kspecem_spec_stats_entry_t {
 enum kspecem_func_type_e kspecem_function_type;
 uint64_t kspecem_function_arg1;
 uint64_t kspecem_function_arg2;
 uint64_t kspecem_function_arg3;
 uint64_t kspecem_function_return_addr;
} kspecem_spec_stats_entry_t;
extern kspecem_spec_stats_entry_t *kspecem_spec_statistics;
extern int kspecem_curr_spec_statistics_index;

void kspecem_statistics_init(void);

void kspecem_add_spec_statistics(uint64_t func_type, uint64_t func_arg1,
    uint64_t func_arg2, uint64_t func_arg3, uint64_t return_addr);

#endif
