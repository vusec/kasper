#ifndef KSPECEM_KSPECEM_PF_CHECKER_H
#define KSPECEM_KSPECEM_PF_CHECKER_H 1

#include "kspecem_types.h"

void kspecem_init_pf_checker(void);
__always_inline const void *kspecem_access(const void *ptr, size_t size);

#endif
