#ifndef KSPECEM_KSPECEM_STACK_H
#define KSPECEM_KSPECEM_STACK_H 1

#include "kspecem_types.h"

#define STACKLOG_MAXLEN      (4096*16L)

extern char *kspecem_wl_stack;
extern void *kspecem_wl_stack_base_address;
extern uint64_t kspecem_wl_stack_len;

void __always_inline kspecem_save_stack(void *frameaddress);

void kspecem_stack_init(void);

#endif
