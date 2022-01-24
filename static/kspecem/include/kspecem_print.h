#ifndef KSPECEM_KSPECEM_PRINT_H
#define KSPECEM_KSPECEM_PRINT_H 1

#include "kspecem_types.h"

#define kspecem_printf(...) printk(KERN_INFO __VA_ARGS__)

#endif
