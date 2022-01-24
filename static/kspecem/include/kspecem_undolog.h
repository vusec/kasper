#ifndef KSPECEM_KSPECEM_WRITELOG_H
#define KSPECEM_KSPECEM_WRITELOG_H 1

#include "kspecem_types.h"

#define WRITELOG_MAXLEN      (1024*8*8L)

#define WRITELOG_GRANULARITY    8 /* the sizeof writelog entries in bytes */
#define WRITELOG_BYTES          (WRITELOG_MAXLEN*(WRITELOG_GRANULARITY+sizeof(void*)))

extern uint64_t kspecem_wl_position;
extern bool kspecem_writelog_enabled;

void __always_inline kspecem_store(void *addr);
void __always_inline kspecem_memcpy(char *addr, size_t size);

void kspecem_writelog_init(void);
void __always_inline kspecem_write_wl(uint64_t *addr, uint64_t *ret_addr);
int kspecem_restore_writelog(uint64_t arg, void *frameaddr, struct pt_regs *regs);

#endif
