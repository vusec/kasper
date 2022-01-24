#include "kspecem_internal.h"

#include "kspecem_common.h"
#include "kspecem_registers.h"
#include "kspecem_assert.h"
#include "kspecem_print.h"
#include "kspecem_stats_restarts.h"
#include "kspecem_statistics.h"
#include "kspecem_report.h"
#include "kspecem_undolog.h"
#include "kspecem_stack.h"
#include "kspecem_whitelist.h"
#include "kspecem_interface.h"

bool volatile kspecem_swap_branch_enabled = true; /* allows enabling/disabling speculative emulation */
unsigned int volatile kspecem_spec_call_depth;
unsigned int volatile kspecem_call_depth;
bool volatile kspecem_swap_branch_condition = false;

void kspecem_init_data(void)
{
  kspecem_in_speculative_emulation = false;
  kspecem_swap_branch_condition = false;
  kspecem_speculative_instr_count = 0;
  kspecem_spec_call_depth = 0;
  kspecem_call_depth = 0;

  kspecem_printf("init kspecem writelog\n");
}

unsigned long __always_inline kspecem_new_checkpoint(void) {
  lt_assert(kspecem_wl_stack != 0x0);
  lt_assert(!kspecem_in_speculative_emulation);

  kspecem_speculative_instr_count = 0;
  kspecem_curr_spec_statistics_index = 0;
  kspecem_add_spec_statistics(KSPECEM_FUNC_NEW_CHECKPOINT, 0, 0, 0,
      (uint64_t)__builtin_return_address(0));

  kspecem_wl_position = 0;

  kspecem_in_speculative_emulation = true;
  if (kspecem_swap_branch_enabled) {
    kspecem_swap_branch_condition = true;
  }
  kspecem_spec_call_depth = 0;
  kspecem_call_depth = 0;
  kspecem_writelog_enabled = true;

  /* save_registers is responsible to reenabled irqs */
  return 0;
}

int __used noinline kspecem_restart(void *arg, int restart_type, uint64_t ret_addr, void *call_arg,
    void *frameaddr, struct pt_regs *regs) {
  char *wl_stack_start = kspecem_wl_stack;
  if (!kspecem_in_speculative_emulation) {
    return 0;
  }

  kspecem_is_in_restart = true;

  if (kspecem_spec_assert.fmt == (char *)-1) {
    if (restart_type == KSPECEM_RESTART_RETURN && kspecem_spec_call_depth > 0) {
      kspecem_add_spec_statistics(KSPECEM_FUNC_RESTART_RETURN, (uint64_t)arg, restart_type, 0,
          ret_addr);
      kspecem_spec_call_depth--;
      return 0;
    }

    kspecem_add_spec_statistics(KSPECEM_FUNC_RESTART, (uint64_t)arg, restart_type, 0,
        ret_addr);

    if (restart_type == KSPECEM_RESTART_CALL_INLINE_ASM) {
      kspecem_add_restart_to_statistics(restart_type, (void *)ret_addr);
    } else {
      kspecem_add_restart_to_statistics(restart_type, call_arg);
    }
  }

  lt_assert(kspecem_writelog_enabled);

  if (arg == 0) {
    frameaddr  = __builtin_frame_address(0);
  } else {
    /* include return address in safe guarding */
    frameaddr += 0x8;
  }
  if (kspecem_restore_writelog((uint64_t)arg, frameaddr, regs) == -1) {
    return -1;
  }

  kspecem_writelog_enabled = false;
  kspecem_in_speculative_emulation = false;
  kspecem_swap_branch_condition = false;
  kspecem_spec_call_depth = 0;
  kspecem_call_depth = 0;

  /* do the memcpy */
  if(NULL == __memcpy(kspecem_wl_stack_base_address-kspecem_wl_stack_len, wl_stack_start, kspecem_wl_stack_len)) {
    lt_panic("memcpy for stack restoring did not work\n");
  }

  kspecem_is_in_restart = false;

  // TODO: Not certain whether we need to check the restart type
  if (restart_type != KSPECEM_RESTART_TIMER && restart_type != KSPECEM_RESTART_EXCEPTION) {
    kspecem_report_print_and_clear(true, restart_type);
  }

  if (kspecem_spec_assert.fmt != (char *)-1) {
    panic(kspecem_spec_assert.fmt, kspecem_spec_assert.file, kspecem_spec_assert.line,
        kspecem_spec_assert.condition);
  }

  if (arg == 0x0) {
    kspecem_unset_rt();
    kspecem_restore_registers();
    lt_panic("should not return\n");
  }
  return 0;
}
