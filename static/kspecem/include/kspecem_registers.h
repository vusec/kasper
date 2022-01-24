#ifndef KSPECEM_KSPECEM_REGISTERS_H
#define KSPECEM_KSPECEM_REGISTERS_H 1

#include "kspecem_types.h"

extern struct pt_regs kspecem_registers;
extern struct pt_regs *kspecem_registers_ptr;

void kspecem_save_registers(void);
void kspecem_restore_registers(void);

#endif
