#ifndef KSPECEM_KSPECEM_STATS_RESTARTS_H
#define KSPECEM_KSPECEM_STATS_RESTARTS_H 1

#include "kspecem_types.h"

typedef struct kspecem_function_stats_t {
 unsigned long kspecem_on_return_restarts;
 unsigned long kspecem_on_call_restarts;
 unsigned long kspecem_on_call_undefined_restarts;
 unsigned long kspecem_on_call_inlineasm_restarts;
 unsigned long kspecem_on_blacklist_restarts;
 unsigned long kspecem_on_exception_restarts;
 unsigned long kspecem_on_timer_restarts;
 unsigned long kspecem_on_limit_spec_restarts;
 unsigned long kspecem_on_call_depth_restarts;
 unsigned long kspecem_on_call_disabled;
 unsigned long kspecem_on_writelog_full_restarts;
 unsigned long kspecem_on_page_fault;
 unsigned long kspecem_total_restarts;
} kspecem_function_stats_t;
extern kspecem_function_stats_t *kspecem_total_statistics;

enum kspecem_restart_type_e {
	KSPECEM_RESTART_RETURN = 0,
	KSPECEM_RESTART_CALL = 1,
	KSPECEM_RESTART_CALL_UNDEFINED,
	KSPECEM_RESTART_CALL_INLINE_ASM,
  KSPECEM_RESTART_BLACKLIST,
	KSPECEM_RESTART_TIMER,
	KSPECEM_RESTART_EXCEPTION,
	KSPECEM_RESTART_LIMIT_SPEC,
	KSPECEM_RESTART_CALL_DEPTH,
  KSPECEM_RESTART_CALL_DISABLED,
	KSPECEM_RESTART_WRITELOG_FULL,
	KSPECEM_RESTART_PAGE_FAULT,
	__NUM_RESTART_TYPES
};
typedef enum kspecem_restart_type_e kspecem_restart_type_t;

void kspecem_stats_restarts_init(void);
void kspecem_print_restart_statistics(void);
void kspecem_add_restart_to_statistics(int restart_type, void *call_arg);

#endif
