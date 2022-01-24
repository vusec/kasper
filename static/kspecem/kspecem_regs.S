#include "sysdep.h"
#include "calling.h"

ENTRY (kspecem_asm_save_registers)
        /* only save registers if (are_irqs_disabled >= 0) == TRUE */
        cmp $0xffffffffffffffff,%rsi
        jle kspecem_asm_save_registers_end
        /* Save registers.  */
        movq %rax, (RAX)(%rdi)
        movq %rbx, (RBX)(%rdi)
        movq %rcx, (RCX)(%rdi)
        movq %rdx, (RDX)(%rdi)
        movq %rbp, (RBP)(%rdi)
        movq %r8,  ( R8)(%rdi)
        movq %r9,  ( R9)(%rdi)
        movq %r10, (R10)(%rdi)
        movq %r11, (R11)(%rdi)
        movq %r12, (R12)(%rdi)
        movq %r13, (R13)(%rdi)
        movq %r14, (R14)(%rdi)
        movq %r15, (R15)(%rdi)
        lea 8(%rsp), %rdx    /* Save SP as it will be after we return.  */
        movq %rdx, (RSP)(%rdi)
        mov (%rsp), %rax     /* Save PC we are returning to now.  */
        movq %rax, (RIP)(%rdi)
        xorq %rax, %rax
        pushfq                  /* Save EFLAGS on stack */
        pop %rax
        movq %rax, (EFLAGS)(%rdi)
kspecem_asm_save_registers_end:
        xorl %eax, %eax
        retq
END (kspecem_asm_save_registers)

        .text
ENTRY(kspecem_asm_restore_registers)
        /* Restore registers.  */
        movq (RSP)(%rdi),%r8
        movq (RBP)(%rdi),%r9
        movq (RIP)(%rdi),%rdx
        movq (RBX)(%rdi),%rbx
        movq (R10)(%rdi),%r10
        movq (R11)(%rdi),%r11
        movq (R12)(%rdi),%r12
        movq (R13)(%rdi),%r13
        movq (R14)(%rdi),%r14
        movq (R15)(%rdi),%r15
        movq (RCX)(%rdi),%rcx
        /* Set return value for setjmp.  */
        mov %esi, %eax
        mov %r8,%rsp
        movq %r9,%rbp
        movq (R8)(%rdi),%r8
        movq (R9)(%rdi),%r9

        movq (EFLAGS)(%rdi), %rax
        push %rax
        popfq /* pop EFLAGS from stack */

        movq (RAX)(%rdi),%rax
        jmpq *%rdx
END (kspecem_asm_restore_registers)
