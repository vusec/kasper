#ifndef KSPECEM_KSPECEM_INTERFACE_H
#define KSPECEM_KSPECEM_INTERFACE_H 1

#include "kspecem_types.h"

#include "kspecem_init.h"
#include "kspecem_assert.h"

extern bool kspecem_is_in_rt;
extern bool kspecem_is_in_restart;

void kspecem_set_rt(void);
void kspecem_unset_rt(void);

#define CHECK_KSPECEM_INIT(default_ret) do { \
  if(kspecem_init_done != true) { \
    return default_ret; \
  } \
} while(0)

#define CHECK_KSPECEM_RT(default_ret) do { \
  if(kspecem_init_done != true || kspecem_is_in_rt == true) { \
    return default_ret; \
  } \
} while(0)

#define ENTER_KSPECEM_RT() \
    unsigned long __irq_flags; \
    do { \
        kspecem_set_rt(); \
        preempt_disable(); \
        local_irq_save(__irq_flags); \
        stop_nmi(); \
    } while(0)

#define ENTER_KSPECEM_NONMI_RT() \
    unsigned long __irq_flags; \
    do { \
        kspecem_set_rt(); \
        preempt_disable(); \
        local_irq_save(__irq_flags); \
    } while(0)

#define LEAVE_KSPECEM_RT() \
    do { \
        lt_assert(irqs_disabled()); \
        restart_nmi(); \
        local_irq_restore(__irq_flags); \
        preempt_enable(); \
        kspecem_unset_rt(); \
    } while(0)

#define LEAVE_KSPECEM_NONMI_RT() \
    do { \
        lt_assert(irqs_disabled()); \
        local_irq_restore(__irq_flags); \
        preempt_enable(); \
        kspecem_unset_rt(); \
    } while(0)

void __used noinline kspecem_hook_check_spec_length(unsigned int bb_inst_count);
void __used noinline kspecem_hook_panic_info(void);

void __used noinline kspecem_hook_print_restart_statistics(void);
void __used noinline kspecem_hook_print_function_statistics(void);

/* function tracking interface */
void __used noinline kspecem_hook_track_called_function(void *func, void *ret_addr);
void __used noinline kspecem_hook_track_print_function(void);

void __used noinline kspecem_hook_check_call_depth(void *func, void *ret_addr);

unsigned long __used noinline kspecem_hook_new_checkpoint(void);

int __used noinline kspecem_hook_restart(void *arg, int restart_type, void *call_addr);

void __always_inline kspecem_hook_leave_rt(unsigned long __irq_flags);

void __used noinline kspecem_hook_save_stack(unsigned long irq_flags, void *frameaddress);

void __used noinline kspecem_hook_store(void *addr);
void __used noinline kspecem_hook_memcpy(char *addr, size_t size);

int __used noinline kspecem_hook_interrupt_exception(void *arg, void *frameaddr,
    int exception_type);
void __used noinline kspecem_hook_interrupt_timer(void *arg, void *frameaddr);

bool __used noinline kspecem_hook_kasan_report(unsigned long addr, size_t size, bool is_write, unsigned long ip);

void __used noinline kspecem_hook_ridl_report(unsigned long addr, size_t size, bool is_write, unsigned long ip, dfsan_label data_label, dfsan_label ptr_label);

void __used noinline kspecem_hook_specv1_report(unsigned long addr, size_t size, bool is_write, unsigned long ip, dfsan_label data_label, dfsan_label ptr_label);

void __used noinline kspecem_hook_smotherspectre_report(unsigned long ip, dfsan_label label);

int __used noinline kspecem_hook_is_whitelist_task(void);

void __used noinline kspecem_hook_enable(int write, void __user *buffer, size_t *lenp);

#endif
