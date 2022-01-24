#ifndef KSPECEM_KSPECEM_UTILS_H
#define KSPECEM_KSPECEM_UTILS_H 1

#include "kspecem_types.h"

// TODO: These are janky but it avoids adding more functions to kdfsan's uninstrumented ABI list
size_t kspecem_strlen(const char *s);
int kspecem_strncmp(const char *cs, const char *ct, int count);
size_t kspecem_strlcpy(char *dest, const char *src, size_t size);
size_t kspecem_strlcat(char *dest, const char *src, size_t count);
void *kspecem_memset(void *s, int c, size_t count);
bool kspecem_checkzero(const void *pv, size_t count);
char* kspecem_itoa(long long num, char* str, int base);

#endif
