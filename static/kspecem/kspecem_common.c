#include "kspecem_common.h"

unsigned int kspecem_max_speculative_instructions = KSPECEM_MAX_SPECULATIVE_INSTRUCTIONS;
u32 kspecem_max_call_depth = 100;

unsigned int volatile kspecem_speculative_instr_count;
int volatile kspecem_in_speculative_emulation;
