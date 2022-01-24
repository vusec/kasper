#include "kspecem_assert.h"
#include "kspecem_common.h"
#include "kspecem_interface.h"
#include "kspecem_statistics.h"
#include "kspecem_print.h"

kspecem_spec_assert_t kspecem_spec_assert = { .fmt = (char *)-1 };

void __used noinline kspecem_panic(char *fmt, char *file, char *line, char *condition) {
  if (kspecem_in_speculative_emulation) {
    kspecem_spec_assert.fmt  = fmt;
    kspecem_spec_assert.file = file;
    kspecem_spec_assert.line = line;
    kspecem_spec_assert.condition = condition;
    if (!kspecem_is_in_restart) {
      /* this does not require a proper restart_type (only used for statistics)
       * since it will cause a panic anyways */
      kspecem_hook_restart(0x0, 0xffff, 0x0);
    }
  } else {
    panic(fmt, file, line, condition);
  }
}

void __always_inline kspecem_panic_info(void) {
  int i;

  kspecem_printf("panic: in_spec: %d, spec_stats: %d, ip: %pS(0x%llx), %pS(0x%llx), %pS(0x%llx), %pS(0x%llx), %pS(0x%llx), %pS(0x%llx), %pS(0x%llx)\n",
      kspecem_in_speculative_emulation,
      kspecem_curr_spec_statistics_index,
      (void*)__builtin_return_address(1), (uint64_t)__builtin_return_address(1),
      (void*)__builtin_return_address(2), (uint64_t)__builtin_return_address(2),
      (void*)__builtin_return_address(3), (uint64_t)__builtin_return_address(3),
      (void*)__builtin_return_address(4), (uint64_t)__builtin_return_address(4),
      (void*)__builtin_return_address(5), (uint64_t)__builtin_return_address(5),
      (void*)__builtin_return_address(6), (uint64_t)__builtin_return_address(6),
      (void*)__builtin_return_address(7), (uint64_t)__builtin_return_address(7));

  for (i = 0; i < kspecem_curr_spec_statistics_index; i++) {
    kspecem_spec_stats_entry_t *entry =
      &kspecem_spec_statistics[i];

    switch(entry->kspecem_function_type) {
      case KSPECEM_FUNC_CHECK_CALL_DEPTH:
        kspecem_printf("[%d], [CHECK_CALL_DEPTH]      ip: %pS(0x%llx)\n",
            i, (void*)entry->kspecem_function_return_addr,
            (uint64_t)entry->kspecem_function_return_addr);
        break;
      case KSPECEM_FUNC_NEW_CHECKPOINT:
        kspecem_printf("[%d], [NEW_CHECKPOINT]        ip: %pS(0x%llx)\n",
            i, (void*)entry->kspecem_function_return_addr,
            (uint64_t)entry->kspecem_function_return_addr);
        break;
      case KSPECEM_FUNC_NEW_CHECKPOINT_INSPEC:
        kspecem_printf("[%d], [NEW_CHECKPOINT_INSPEC] ip: %pS(0x%llx)\n",
            i, (void*)entry->kspecem_function_return_addr,
            (uint64_t)entry->kspecem_function_return_addr);
        break;
      case KSPECEM_FUNC_RESTART_RETURN:
        kspecem_printf("[%d], [RESTART_RETURN]         ip: %pS(0x%llx)\n",
            i, (void*)entry->kspecem_function_return_addr,
            (uint64_t)entry->kspecem_function_return_addr);
        break;
      case KSPECEM_FUNC_RESTART:
        kspecem_printf("[%d], [RESTART]     type: %lld ip: %pS(0x%llx)\n",
            i, entry->kspecem_function_arg2, (void*)entry->kspecem_function_return_addr,
            (uint64_t)entry->kspecem_function_return_addr);
        break;
      default:
        break;
    }
  }
}

void __used noinline kspecem_assert_print(char *file, char *line, char *condition) {
  lt_assert_print(file, line, condition);
}
