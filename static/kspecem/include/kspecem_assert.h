#ifndef KSPECEM_KSPECEM_ASSERT_H
#define KSPECEM_KSPECEM_ASSERT_H 1

#include "kspecem_types.h"

typedef struct kspecem_spec_assert_t {
 char *fmt;
 char *file;
 char *line;
 char *condition;
} kspecem_spec_assert_t;

extern kspecem_spec_assert_t kspecem_spec_assert;

#define lt_panic(m) do { \
  panic("%s:%d: panic: %s\n", __FILE__, __LINE__, (m)); \
} while (0)

#define STRINGIT2(l) #l
#define STRINGIT(l) STRINGIT2(l)

void __used noinline kspecem_panic(char *fmt, char *file, char *line, char *condition);
void __always_inline kspecem_panic_info(void);

#define lt_assert_print(file, line, condition) { \
  kspecem_panic("assert failed in %s: %s: %s", file, line, condition); \
} while (0)

void __used noinline kspecem_assert_print(char *file, char *line, char *condition);

#define lt_assert(condition) { \
  if(!(condition)) lt_assert_print(__FILE__, STRINGIT(__LINE__), #condition); \
}

#endif
