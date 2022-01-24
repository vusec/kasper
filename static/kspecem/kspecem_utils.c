#include "kspecem_utils.h"
#include "kspecem_assert.h"

size_t kspecem_strlen(const char *s) {
  const char *sc;
  for (sc = s; *sc != '\0'; ++sc) { ; }
  return sc - s;
}

int kspecem_strncmp(const char *cs, const char *ct, int count) {
  unsigned char c1, c2;
  while (count) {
    c1 = *cs++;
    c2 = *ct++;
    if (c1 != c2) {
      return c1 < c2 ? -1 : 1;
    }
    if (!c1) {
      break;
    }
    count--;
  }
  return 0;
}

size_t kspecem_strlcpy(char *dest, const char *src, size_t size) {
  size_t ret = kspecem_strlen(src);
  if (size) {
    size_t len = (ret >= size) ? size - 1 : ret;
    memcpy(dest, src, len);
    dest[len] = '\0';
  }
  return ret;
}

size_t kspecem_strlcat(char *dest, const char *src, size_t count) {
  size_t dsize = kspecem_strlen(dest);
  size_t len = kspecem_strlen(src);
  size_t res = dsize + len;
  lt_assert(dsize < count); // This would be a bug
  dest += dsize;
  count -= dsize;
  if (len >= count) { len = count-1; }
  __memcpy(dest, src, len);
  dest[len] = 0;
  return res;
}

// It would probably be fine if this just called __memset (the kernel's asm implementation)
void *kspecem_memset(void *s, int c, size_t count) {
  char *xs = s;
  while (count--) {
    *xs++ = c;
  }
  return s;
}

// Returns true if buffer is all zeros
bool kspecem_checkzero(const void *pv, size_t count) {
  int i;
  const u8 *p = pv;
  for (i = 0; i < count; i++) {
    if (p[i] != 0) {
      return false;
    }
  }
  return true;
}

// Using https://www.geeksforgeeks.org/implement-itoa/
static void kspecem_reverse_str(char str[], int length) {
  int start = 0;
  int end = length -1;
  while (start < end) {
    char tmp = *(str+end);
    *(str+end) = *(str+start);
    *(str+start) = tmp;
    start++;
    end--;
  }
}
char* kspecem_itoa(long long num, char* str, int base) {
  int i = 0;
  bool is_negative = false;
  if (num == 0) {
    str[i++] = '0';
    str[i] = '\0';
    return str;
  }
  if (num < 0 && base == 10) {
    is_negative = true;
    num = -num;
  }
  while (num != 0) {
    int rem = num % base;
    str[i++] = (rem > 9)? (rem-10) + 'a' : rem + '0';
    num = num/base;
  }
  if (is_negative) { str[i++] = '-'; }
  str[i] = '\0';
  kspecem_reverse_str(str, i);
  return str;
}

