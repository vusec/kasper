#include "kspecem_interface.h"

#include "kspecem_assert.h"
#include "kspecem_common.h"
#include "kspecem_interrupt.h"
#include "kspecem_report.h"
#include "kspecem_stack.h"
#include "kspecem_statistics.h"
#include "kspecem_stats_restarts.h"
#include "kspecem_internal.h"
#include "kspecem_whitelist.h"
#include "kspecem_undolog.h"
#include "kspecem_pf_checker.h"
#include "kspecem_kdf.h"

bool kspecem_is_in_rt = false;
bool kspecem_is_in_restart = false;

void kspecem_set_rt(void) {
  kspecem_is_in_rt = true;
}
void kspecem_unset_rt(void) {
  kspecem_is_in_rt = false;
}

void __used noinline kspecem_hook_check_spec_length(unsigned int bb_inst_count) {
  if (kspecem_in_speculative_emulation == true) {
    CHECK_KSPECEM_RT();
    CHECK_KDFINIT_RT();
    CHECK_KDFSAN_RT();
    kspecem_speculative_instr_count += bb_inst_count;
    if (kspecem_speculative_instr_count >= kspecem_max_speculative_instructions) {
      ENTER_KSPECEM_RT();
      /* restart_hook is responsible for reenabling irqs */
      kspecem_restart(0x0, KSPECEM_RESTART_LIMIT_SPEC, (uint64_t)__builtin_return_address(0), 0, 0, 0);
      lt_panic("should not return!\n");
      LEAVE_KSPECEM_RT();
    }
  }
}

void __used noinline kspecem_hook_panic_info(void) {
  kspecem_panic_info();
}

void __used noinline kspecem_hook_print_restart_statistics(void) {
  CHECK_KSPECEM_RT();
  ENTER_KSPECEM_RT();
  kspecem_print_restart_statistics();
  LEAVE_KSPECEM_RT();
}

void __used noinline kspecem_hook_check_call_depth(void *func, void *ret_addr) {
  CHECK_KSPECEM_RT();
  if (kspecem_writelog_enabled) {
    kspecem_add_spec_statistics(KSPECEM_FUNC_CHECK_CALL_DEPTH, 0, 0, 0,
        (uint64_t)__builtin_return_address(0));
    kspecem_spec_call_depth += 1;
    if (kspecem_spec_call_depth >= kspecem_max_call_depth) {
      ENTER_KSPECEM_RT();
      kspecem_restart((void *)0x0, KSPECEM_RESTART_CALL_DEPTH, (uint64_t)__builtin_return_address(0), 0, 0, 0);
      lt_panic("should not return!\n");
      LEAVE_KSPECEM_RT();
    }
  }
}

unsigned long __used noinline kspecem_hook_new_checkpoint(void) {
  unsigned long ret;

  CHECK_KSPECEM_RT(-1);
  if (in_interrupt()) return -1;
  /* if irqs are already disabled, don't take a checkpoint */
  if (irqs_disabled()) return -1;
  if (kspecem_spec_assert.fmt != (char *)-1) return -1;
  /* TODO: how to whitelist tests? */
  // if (module_index != (uint64_t)-1) {
    /* do not run task whitelist for new_checkpoint hooks in kdfsan */
    if (!kspecem_is_whitelist_task()) return -1;
  // }

  /* if kspecem is already in speculation, don't create a new checkpoint */
  if (kspecem_in_speculative_emulation == true) {
    kspecem_add_spec_statistics(KSPECEM_FUNC_NEW_CHECKPOINT_INSPEC, 0, 0, 0,
        (uint64_t)__builtin_return_address(0));
    return -1;
  }

  ENTER_KSPECEM_RT();
  ret = kspecem_new_checkpoint();

  /* DO NOT CALL LEAVE_KSPECEM_RT, interrupts should only be reenabled later */
  if (ret == -1) return ret;
  return __irq_flags;
}

int __used noinline kspecem_hook_restart(void *arg, int restart_type, void *call_arg) {
  int ret;
  CHECK_KSPECEM_INIT(0);
  ENTER_KSPECEM_RT();
  ret = kspecem_restart(arg, restart_type, (uint64_t)__builtin_return_address(0), call_arg, 0, 0);
  LEAVE_KSPECEM_RT();
  return ret;
}

void __always_inline kspecem_hook_leave_rt(unsigned long __irq_flags) {
  if (__irq_flags != -1) {
    LEAVE_KSPECEM_RT();
  }
}

void __used noinline kspecem_hook_save_stack(unsigned long irq_flags, void *frameaddress) {
  if (irq_flags != -1) {
    kspecem_save_stack(frameaddress);
  }
}

void __used noinline kspecem_hook_store(void *addr) {
  if (kspecem_writelog_enabled != 1) return;

  CHECK_KSPECEM_INIT();
  /* stop_nmi and restart_nmi include store hooks => infinite recursion */
  ENTER_KSPECEM_NONMI_RT();
  kspecem_store(addr);
  LEAVE_KSPECEM_NONMI_RT();
}

void __used noinline kspecem_hook_memcpy(char *addr, size_t size) {
  if (kspecem_writelog_enabled != 1) return;

  CHECK_KSPECEM_INIT();
  ENTER_KSPECEM_RT();
  kspecem_memcpy(addr, size);
  LEAVE_KSPECEM_RT();
}

/* Instead of handling the rest of the exception handler, do a return in this case
  * return boolean on this function, then modify calling function.
  * split bb after the inserted hook. add conditional branch to the end of the top bb
  * if false go to bottom bb, if true go to new bb which returns
  */
int __used noinline kspecem_hook_interrupt_exception(void *arg, void *frameaddr,
    int exception_type) {
  CHECK_KSPECEM_INIT(0);
  lt_assert(frameaddr);
  lt_assert(kspecem_writelog_enabled == kspecem_in_speculative_emulation);
  if (kspecem_in_speculative_emulation == true) {
    kspecem_interrupt_exception(arg, frameaddr, exception_type);
    return 1;
  }
  return 0;
}
void __used noinline kspecem_hook_interrupt_timer(void *arg, void *frameaddr) {
  CHECK_KSPECEM_INIT();
  lt_assert(frameaddr);
  lt_assert(kspecem_writelog_enabled == kspecem_in_speculative_emulation);
  if (kspecem_in_speculative_emulation == true) {
    kspecem_interrupt_timer(arg, frameaddr);
  }
}

bool __used noinline kspecem_hook_kasan_report(unsigned long addr, size_t size, bool is_write, unsigned long ip) {
  if (addr < 0x100000) {
    return false;
  }
  CHECK_KSPECEM_RT(false);
  ENTER_KSPECEM_NONMI_RT(); /* stop_nmi could call this hook (TODO: double-check this) */
  kspecem_kasan_report(addr, size, is_write, ip);
  LEAVE_KSPECEM_NONMI_RT();
  return true;
}

void __used noinline kspecem_hook_ridl_report(unsigned long addr, size_t size, bool is_write, unsigned long ip, dfsan_label data_label, dfsan_label ptr_label) {
  CHECK_KSPECEM_RT();
  ENTER_KSPECEM_NONMI_RT(); /* stop_nmi could call this hook (TODO: double-check this) */
  kspecem_ridl_report(addr, size, is_write, ip, data_label, ptr_label);
  LEAVE_KSPECEM_NONMI_RT();
}

void __used noinline kspecem_hook_specv1_report(unsigned long addr, size_t size, bool is_write, unsigned long ip, dfsan_label data_label, dfsan_label ptr_label) {
  CHECK_KSPECEM_RT();
  ENTER_KSPECEM_NONMI_RT(); /* stop_nmi could call this hook (TODO: double-check this) */
  kspecem_specv1_report(addr, size, is_write, ip, data_label, ptr_label);
  LEAVE_KSPECEM_NONMI_RT();
}

void __used noinline kspecem_hook_smotherspectre_report(unsigned long ip, dfsan_label label) {
  CHECK_KSPECEM_RT();
  ENTER_KSPECEM_NONMI_RT(); /* stop_nmi could call this hook (TODO: double-check this) */
  kspecem_smotherspectre_report(ip, label);
  LEAVE_KSPECEM_NONMI_RT();
}

int __used noinline kspecem_hook_is_whitelist_task(void) {
  int ret;
  CHECK_KSPECEM_RT(0);
  ENTER_KSPECEM_RT();
  ret = kspecem_is_whitelist_task();
  LEAVE_KSPECEM_RT();
  return ret;
}

const void *__used noinline kspecem_hook_access(const void *ptr, size_t size) {
  const void *ret_ptr;

  // If we're not in speculative emulation, don't change the pointer
  if(kspecem_in_speculative_emulation != true) return ptr;

  /* If kdfinit hasn't be initialized or we're already in the rt,
      don't change the pointer. Some notes about this:
        - No whitelisting because the 'kspecem_in_speculative_emulation'
          check already does this implicitly
        - No nmi because the accesses in stop/restart_nmi would recursively
          call back into the hook
  */
  CHECK_KSPECEM_INIT(ptr);
  ENTER_KSPECEM_NONMI_RT();

  ret_ptr = kspecem_access(ptr, size);

  LEAVE_KSPECEM_NONMI_RT();
  return ret_ptr;
}
