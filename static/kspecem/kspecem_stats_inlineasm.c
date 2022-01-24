#include "kspecem_stats_inlineasm.h"

#include "kspecem_print.h"

bool kspecem_inlineasm_restart_stats_enabled = false;

#define NUM_INLINEASM_RESTART_IPS 40
kspecem_inlineasm_restart_stat_t *kspecem_inlineasm_restart_stats; // Sorted list
unsigned long kspecem_inlineasm_restart_stat_size;

void kspecem_stats_inlineasm_init(void) {
  if (kspecem_inlineasm_restart_stats_enabled) {
    kspecem_inlineasm_restart_stats = kzalloc(
        sizeof(kspecem_inlineasm_restart_stat_t) * NUM_INLINEASM_RESTART_IPS, GFP_KERNEL);
    kspecem_inlineasm_restart_stat_size = 0;
  }
}

void kspecem_add_inlineasm_restart(void * ip) {
  kspecem_inlineasm_restart_stat_t tmp_stat;
  int i;
  // If in list, increment its count, then sort the new list by decreasing restart_count, then return
  for (i = 0; i < kspecem_inlineasm_restart_stat_size; i++) {
    if (kspecem_inlineasm_restart_stats[i].restart_ip == ip) {
      // Increment count then copy into temporary struct
      kspecem_inlineasm_restart_stats[i].restart_count++;
      tmp_stat = (kspecem_inlineasm_restart_stat_t){
        .restart_ip = ip,
        .restart_count = kspecem_inlineasm_restart_stats[i].restart_count
      };
      for (i--; i >= 0 && kspecem_inlineasm_restart_stats[i].restart_count < tmp_stat.restart_count; i--) {
        // Shift elements forward until our element is in the correct place
        kspecem_inlineasm_restart_stats[i+1] = kspecem_inlineasm_restart_stats[i];
        kspecem_inlineasm_restart_stats[i] = tmp_stat;
      }
      return;
    }
  }

  // If not in list, check whether we have enough room to add another element
  if (kspecem_inlineasm_restart_stat_size >= NUM_INLINEASM_RESTART_IPS) {
    return;
  }

  // Add it to the end of list (this maintains a sorted list, since its count is only 1)
  kspecem_inlineasm_restart_stats[kspecem_inlineasm_restart_stat_size].restart_ip = ip;
  kspecem_inlineasm_restart_stats[kspecem_inlineasm_restart_stat_size].restart_count = 1;
  kspecem_inlineasm_restart_stat_size++;
}

void kspecem_print_inlineasm_stats(void) {
  int i;
  kspecem_printf("==== Inline asm restarts: ====\n");
  for (i = 0; i < kspecem_inlineasm_restart_stat_size; i++) {
    kspecem_printf("%pS (%lu)\n", kspecem_inlineasm_restart_stats[i].restart_ip, kspecem_inlineasm_restart_stats[i].restart_count);
  }
  if (kspecem_inlineasm_restart_stat_size >= NUM_INLINEASM_RESTART_IPS) {
    kspecem_printf("WARNING: Max inlineasm restart count reached; some inlineasm IPs were not recorded.\n");
  }
  kspecem_printf("==============================\n");
}
