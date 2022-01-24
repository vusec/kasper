#include "kspecem_stats_restarts.h"

#include "kspecem_stats_inlineasm.h"
#include "kspecem_print.h"

kspecem_function_stats_t *kspecem_total_statistics;

void kspecem_stats_restarts_init(void) {
  kspecem_total_statistics = kzalloc(sizeof(kspecem_function_stats_t), GFP_KERNEL);
}

void kspecem_add_restart_to_statistics(int restart_type, void *call_arg) {
  kspecem_total_statistics->kspecem_total_restarts += 1;

  switch(restart_type) {
    case KSPECEM_RESTART_RETURN:
      kspecem_total_statistics->kspecem_on_return_restarts += 1;
      break;
    case KSPECEM_RESTART_CALL:
      kspecem_total_statistics->kspecem_on_call_restarts += 1;
      break;
    case KSPECEM_RESTART_CALL_UNDEFINED:
      kspecem_total_statistics->kspecem_on_call_undefined_restarts += 1;
      break;
    case KSPECEM_RESTART_CALL_INLINE_ASM:
      kspecem_total_statistics->kspecem_on_call_inlineasm_restarts += 1;
      if (kspecem_inlineasm_restart_stats_enabled)
        kspecem_add_inlineasm_restart(call_arg);
      break;
    case KSPECEM_RESTART_BLACKLIST:
      kspecem_total_statistics->kspecem_on_blacklist_restarts += 1;
      break;
    case KSPECEM_RESTART_TIMER:
      kspecem_total_statistics->kspecem_on_timer_restarts += 1;
      break;
    case KSPECEM_RESTART_EXCEPTION:
      kspecem_total_statistics->kspecem_on_exception_restarts += 1;
      break;
    case KSPECEM_RESTART_LIMIT_SPEC:
      kspecem_total_statistics->kspecem_on_limit_spec_restarts += 1;
      break;
    case KSPECEM_RESTART_CALL_DEPTH:
      kspecem_total_statistics->kspecem_on_call_depth_restarts += 1;
      break;
    case KSPECEM_RESTART_CALL_DISABLED:
      kspecem_total_statistics->kspecem_on_call_disabled += 1;
      break;
    case KSPECEM_RESTART_WRITELOG_FULL:
      kspecem_total_statistics->kspecem_on_writelog_full_restarts += 1;
      break;
    case KSPECEM_RESTART_PAGE_FAULT:
      kspecem_total_statistics->kspecem_on_page_fault += 1;
      break;
    default:
      break;
  }
}

void kspecem_print_restart_statistics(void) {
  kspecem_printf("=== total ===\n", -1, -1);
  kspecem_printf("[SUM] { \"spec_length\": %u, \"total\": %lu, \"return\": %lu, "
      "\"call\": %lu, \"undefined\": %lu, \"inlineasm\": %lu, \"blacklist\": %lu, "
      "\"timer\": %lu, \"exception\": %lu, \"limit\": %lu, "
      "\"calldepth\": %lu, \"calldisabled\": %lu, \"writelog_full\": %lu, "
      "\"pagefault\": %lu }\n",
      0,
      kspecem_total_statistics->kspecem_total_restarts,
      kspecem_total_statistics->kspecem_on_return_restarts,
      kspecem_total_statistics->kspecem_on_call_restarts,
      kspecem_total_statistics->kspecem_on_call_undefined_restarts,
      kspecem_total_statistics->kspecem_on_call_inlineasm_restarts,
      kspecem_total_statistics->kspecem_on_blacklist_restarts,
      kspecem_total_statistics->kspecem_on_timer_restarts,
      kspecem_total_statistics->kspecem_on_exception_restarts,
      kspecem_total_statistics->kspecem_on_limit_spec_restarts,
      kspecem_total_statistics->kspecem_on_call_depth_restarts,
      kspecem_total_statistics->kspecem_on_call_disabled,
      kspecem_total_statistics->kspecem_on_writelog_full_restarts,
      kspecem_total_statistics->kspecem_on_page_fault);
  if (kspecem_inlineasm_restart_stats_enabled) kspecem_print_inlineasm_stats();
}
