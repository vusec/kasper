#ifndef KSPECEM_KSPECEM_STATS_INLINEASM_H
#define KSPECEM_KSPECEM_STATS_INLINEASM_H 1

#include "kspecem_types.h"

extern bool kspecem_inlineasm_restart_stats_enabled;

typedef struct kspecem_inlineasm_restart_stat_t {
  void * restart_ip;
  unsigned long restart_count;
} kspecem_inlineasm_restart_stat_t;

void kspecem_stats_inlineasm_init(void);
void kspecem_add_inlineasm_restart(void * ip);
void kspecem_print_inlineasm_stats(void);

#endif
