#ifndef KSPECEM_KSPECEM_UNDOLOG_H
#define KSPECEM_KSPECEM_UNDOLOG_H 1

#include "kspecem_types.h"

extern bool volatile kspecem_swap_branch_enabled;
extern unsigned int volatile kspecem_spec_call_depth;
extern unsigned int volatile kspecem_call_depth;

unsigned long __always_inline kspecem_new_checkpoint(void);
int __used noinline kspecem_restart(void *arg,
    int restart_type, uint64_t ret_addr, void *call_arg,
    void *frameaddr, struct pt_regs *regs);

#endif
