#ifndef KSPECEM_KSPECEM_KDF_H
#define KSPECEM_KSPECEM_KDF_H 1

#include "kspecem_types.h"

void __weak dfsan_copy_label_info(dfsan_label label, char * dest, size_t count);
bool __weak kdf_virt_addr_valid(void *addr);
int __weak kdfsan_enable(void *data, u64 *val);

bool __weak kdf_is_init_done;
bool __weak kdf_is_in_rt;
bool __weak kdfinit_is_init_done;
bool __weak kdfinit_is_in_rt;

#define CHECK_KDFINIT_RT(default_ret) do { \
  if(kdfinit_is_init_done && kdfinit_is_in_rt) { return default_ret; } \
} while(0)
#define CHECK_KDFSAN_RT(default_ret) do { \
  if(kdf_is_init_done && kdf_is_in_rt) { return default_ret; } \
} while(0)

#endif
