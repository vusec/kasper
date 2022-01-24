#include "kspecem_registers.h"

struct pt_regs kspecem_registers;
struct pt_regs *kspecem_registers_ptr;

void kspecem_asm_save_registers(struct pt_regs *registers, uint64_t are_irqs_disabled);
void kspecem_asm_restore_registers(struct pt_regs *registers, uint64_t are_irqs_disabled);

__always_inline void kspecem_save_registers(void)
{
	kspecem_asm_save_registers(&kspecem_registers, 1);
}

__always_inline void kspecem_restore_registers(void)
{
	kspecem_asm_restore_registers(&kspecem_registers, 1);
}
