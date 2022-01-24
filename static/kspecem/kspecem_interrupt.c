#include "kspecem_interrupt.h"
#include "kspecem_internal.h"
#include "kspecem_undolog.h"
#include "kspecem_common.h"
#include "kspecem_assert.h"
#include "kspecem_registers.h"
#include "kspecem_stats_restarts.h"

void __used noinline kspecem_interrupt_timer(void *arg, void *frameaddr) {
  struct pt_regs *regs = (struct pt_regs *)arg;
  struct pt_regs *kspecem_regs = (struct pt_regs *)&kspecem_registers;

  /* set registers to checkpoint values to not return into speculative emulation
   * after the interrupt (using iret) */
  /* TODO: tmp solution, old code used xmm0 register when compiled and broke
      certain functions (vmacache_find, __anon_vma_prepare(?), etc.)
      we should transition everything to use pt_regs instead of jmp_buf_regs */
  asm volatile (
      "movq %0, %%rdi\n\t"
      "movq %1, %%rcx\n\t"
      "movq 0x0(%%rcx), %%rdx\n\t" // r15
      "movq %%rdx, 0x0(%%rdi)\n\t"
      "movq 0x8(%%rcx), %%rdx\n\t" // r14
      "movq %%rdx, 0x8(%%rdi)\n\t"
      "movq 0x10(%%rcx), %%rdx\n\t" // r13
      "movq %%rdx, 0x10(%%rdi)\n\t"
      "movq 0x18(%%rcx), %%rdx\n\t" // r12
      "movq %%rdx, 0x18(%%rdi)\n\t"
      "movq 0x30(%%rcx), %%rdx\n\t" // r11
      "movq %%rdx, 0x30(%%rdi)\n\t"
      "movq 0x38(%%rcx), %%rdx\n\t" // r10
      "movq %%rdx, 0x38(%%rdi)\n\t"
      "movq 0x40(%%rcx), %%rdx\n\t" // r9
      "movq %%rdx, 0x40(%%rdi)\n\t"
      "movq 0x48(%%rcx), %%rdx\n\t" // r8
      "movq %%rdx, 0x48(%%rdi)\n\t"
      "movq 0x20(%%rcx), %%rdx\n\t" // rbp
      "movq %%rdx, 0x20(%%rdi)\n\t"

      "movq 0x50(%%rcx), %%rdx\n\t" // rax
      "movq %%rdx, 0x50(%%rdi)\n\t"
      "movq 0x28(%%rcx), %%rdx\n\t" // rbx
      "movq %%rdx, 0x28(%%rdi)\n\t"
      "movq 0x58(%%rcx), %%rdx\n\t" // rcx
      "movq %%rdx, 0x58(%%rdi)\n\t"
      "movq 0x60(%%rcx), %%rdx\n\t" // rdx
      "movq %%rdx, 0x60(%%rdi)\n\t"

      "movq 0x68(%%rcx), %%rdx\n\t" // rsi
      "movq %%rdx, 0x68(%%rdi)\n\t"
      "movq 0x70(%%rcx), %%rdx\n\t" // rdi
      "movq %%rdx, 0x70(%%rdi)\n\t"

      "movq 0x90(%%rcx), %%rdx\n\t" // eflags
      "movq %%rdx, 0x90(%%rdi)\n\t"

      "movq 0x80(%%rcx), %%rdx\n\t" // rip
      "movq %%rdx, 0x80(%%rdi)\n\t"
      "movq 0x98(%%rcx), %%rdx\n\t" // rsp
      "movq %%rdx, 0x98(%%rdi)\n\t"
      :
      : "r" (regs), "r" (kspecem_regs)
      : "rdi", "rdx", "rcx", "memory");

  /* TODO cs, ss? */

  /* kspecem_restart will not restore registers but return */
  kspecem_restart((void *)0x1, KSPECEM_RESTART_TIMER, (uint64_t)__builtin_return_address(0),
      0x0, frameaddr, regs);
  lt_assert(kspecem_regs->ip == regs->ip);
}

void __used noinline kspecem_interrupt_exception(void *arg, void *frameaddr,
    int exception_type) {
  int restart_type;
  struct pt_regs *regs = (struct pt_regs *)arg;
  struct pt_regs *kspecem_regs = (struct pt_regs *)&kspecem_registers;

  /* set registers to checkpoint values to not return into speculative emulation
    * after the interrupt (using iret) */
  /* TODO: tmp solution, old code used xmm0 register when compiled and broke
      certain functions (vmacache_find, __anon_vma_prepare(?), etc.)
      we should transition everything to use pt_regs instead of jmp_buf_regs */
  asm volatile (
      "movq %0, %%rdi\n\t"
      "movq %1, %%rcx\n\t"
      "movq 0x0(%%rcx), %%rdx\n\t" // r15
      "movq %%rdx, 0x0(%%rdi)\n\t"
      "movq 0x8(%%rcx), %%rdx\n\t" // r14
      "movq %%rdx, 0x8(%%rdi)\n\t"
      "movq 0x10(%%rcx), %%rdx\n\t" // r13
      "movq %%rdx, 0x10(%%rdi)\n\t"
      "movq 0x18(%%rcx), %%rdx\n\t" // r12
      "movq %%rdx, 0x18(%%rdi)\n\t"
      "movq 0x30(%%rcx), %%rdx\n\t" // r11
      "movq %%rdx, 0x30(%%rdi)\n\t"
      "movq 0x38(%%rcx), %%rdx\n\t" // r10
      "movq %%rdx, 0x38(%%rdi)\n\t"
      "movq 0x40(%%rcx), %%rdx\n\t" // r9
      "movq %%rdx, 0x40(%%rdi)\n\t"
      "movq 0x48(%%rcx), %%rdx\n\t" // r8
      "movq %%rdx, 0x48(%%rdi)\n\t"
      "movq 0x20(%%rcx), %%rdx\n\t" // rbp
      "movq %%rdx, 0x20(%%rdi)\n\t"

      "movq 0x50(%%rcx), %%rdx\n\t" // rax
      "movq %%rdx, 0x50(%%rdi)\n\t"
      "movq 0x28(%%rcx), %%rdx\n\t" // rbx
      "movq %%rdx, 0x28(%%rdi)\n\t"
      "movq 0x58(%%rcx), %%rdx\n\t" // rcx
      "movq %%rdx, 0x58(%%rdi)\n\t"
      "movq 0x60(%%rcx), %%rdx\n\t" // rdx
      "movq %%rdx, 0x60(%%rdi)\n\t"

      "movq 0x68(%%rcx), %%rdx\n\t" // rsi
      "movq %%rdx, 0x68(%%rdi)\n\t"
      "movq 0x70(%%rcx), %%rdx\n\t" // rdi
      "movq %%rdx, 0x70(%%rdi)\n\t"

      "movq 0x90(%%rcx), %%rdx\n\t" // eflags
      "movq %%rdx, 0x90(%%rdi)\n\t"

      "movq 0x80(%%rcx), %%rdx\n\t" // rip
      "movq %%rdx, 0x80(%%rdi)\n\t"
      "movq 0x98(%%rcx), %%rdx\n\t" // rsp
      "movq %%rdx, 0x98(%%rdi)\n\t"
      :
      : "r" (regs), "r" (kspecem_regs)
      : "rdi", "rdx", "rcx", "memory");

  /* TODO cs, ss? */

  restart_type = KSPECEM_RESTART_EXCEPTION;
  if (exception_type == 1) {
    restart_type = KSPECEM_RESTART_PAGE_FAULT;
  }
  /* kspecem_restart will not restore registers but return */
  kspecem_restart((void *)0x1, restart_type, (uint64_t)__builtin_return_address(0),
      0x0, frameaddr, regs);
  lt_assert(kspecem_regs->ip == regs->ip);
}
