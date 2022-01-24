#ifndef KSPECEM_KSPECEM_COMMON_H
#define KSPECEM_KSPECEM_COMMON_H 1

#include "kspecem_types.h"
#include "kspecem_kdf.h"

#define KSPECEM_MAX_SPECULATIVE_INSTRUCTIONS 250
extern unsigned int kspecem_max_speculative_instructions;
extern u32 kspecem_max_call_depth;

extern unsigned int volatile kspecem_speculative_instr_count;
extern int volatile kspecem_in_speculative_emulation;

#endif
