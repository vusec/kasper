#ifndef KSPECEM_KSPECEM_INTERRUPT_H
#define KSPECEM_KSPECEM_INTERRUPT_H 1

#include "kspecem_types.h"

void __used noinline kspecem_interrupt_exception(void *arg, void *frameaddr,
    int exception_type);
void __used noinline kspecem_interrupt_timer(void *arg, void *frameaddr);

#endif
