#include "kspecem_kdf.h"

bool __weak kdf_is_init_done = false;
bool __weak kdf_is_in_rt = false;
bool __weak kdfinit_is_init_done = false;
bool __weak kdfinit_is_in_rt = false;

void __weak dfsan_copy_label_info(dfsan_label label, char * dest, size_t count) {
  dest = "unknown-label";
}

bool __weak kdf_virt_addr_valid(void *addr) {
  return true;
}

int __weak kdfsan_enable(void *data, u64 *val) {
  return -1;
}
