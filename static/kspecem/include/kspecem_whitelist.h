#ifndef KSPECEM_KSPECEM_WHITELIST_H
#define KSPECEM_KSPECEM_WHITELIST_H 1

#include "kspecem_types.h"

unsigned long kspecem_syscall_get_nr(void);

enum kspecem_whitelist_type_e {
	KSPECEM_WHITELIST_DISABLED = 0,
  KSPECEM_WHITELIST_TASKS,
  KSPECEM_WHITELIST_SYSCALL,
  __NUM_WHITELIST_TYPES,
};
typedef enum kspecem_whitelist_type_e kspecem_whitelist_type_t;

extern kspecem_whitelist_type_t kspecem_whitelist_type;

void kspecem_whitelist_init(void);

int __always_inline kspecem_is_whitelist_task(void);

#endif
