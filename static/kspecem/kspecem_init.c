#include "kspecem_init.h"

#include "kspecem_bugs.h"
#include "kspecem_common.h"
#include "kspecem_print.h"
#include "kspecem_statistics.h"
#include "kspecem_stats_inlineasm.h"
#include "kspecem_stats_restarts.h"
#include "kspecem_registers.h"
#include "kspecem_undolog.h"
#include "kspecem_whitelist.h"
#include "kspecem_report.h"
#include "kspecem_stack.h"
#include "kspecem_internal.h"
#include "kspecem_interface.h"
#include "kspecem_pf_checker.h"
#include "kspecem_kdf.h"

bool kspecem_init_done = false;

static int kspecem_restart_stats(void *data, u64 *val) {
  kspecem_hook_print_restart_statistics();
  return 0;
}
DEFINE_DEBUGFS_ATTRIBUTE(kspecem_restart_stats_fops, kspecem_restart_stats, NULL, "%lld\n");

static int kspecem_enable(void *data, u64 *val) {
  if (kdfsan_enable(NULL, NULL) == -1) {
    /* In case kdfsan is not linked just enable kspecem */
    kspecem_common_late_init();
  }
  return 0;
}
DEFINE_DEBUGFS_ATTRIBUTE(kspecem_enable_fops, kspecem_enable, NULL, "%lld\n");

int __init kspecem_init(void) {
  struct dentry *kasper_dir;
  kspecem_printf("KSpecEm: Initializing...\n");

  kasper_dir  = debugfs_create_dir("kasper", NULL);
  debugfs_create_file("restarts", 0444, kasper_dir, NULL, &kspecem_restart_stats_fops);
  debugfs_create_file("enable", 0444, kasper_dir, NULL, &kspecem_enable_fops);

  debugfs_create_u8("whitelist", 0666, kasper_dir,
      (u8 *)&kspecem_whitelist_type);
  debugfs_create_u32("call_depth", 0666, kasper_dir, &kspecem_max_call_depth);
  debugfs_create_u8("print_reports", 0666, kasper_dir,
      (u8 *)&kspecem_print_level);
  debugfs_create_bool("spec_enabled", 0666, kasper_dir,
      (bool *)&kspecem_swap_branch_enabled);
  debugfs_create_bool("calltrace_enabled", 0666, kasper_dir,
      &kspecem_report_error_dump_stack);
  debugfs_create_bool("inlineasm_restart_stats_enabled", 0666, kasper_dir,
      &kspecem_inlineasm_restart_stats_enabled);

  kspecem_whitelist_init();

  kspecem_printf("KSpecEm: Initializing done.\n");
  return 0;
}

void kspecem_common_late_init(void)
{
  extern void kspecem_init_data(void);

	if (kspecem_init_done != true) {
    ENTER_KSPECEM_RT();

    kspecem_statistics_init();
    kspecem_stats_inlineasm_init();
    kspecem_stats_restarts_init();
    kspecem_report_init();
    kspecem_init_data();

    kspecem_registers_ptr = &kspecem_registers;

    kspecem_stack_init();
    /* TODO fix that kspecem_writelog_init does not need to be last */
    kspecem_writelog_init();

    kspecem_init_pf_checker();

    LEAVE_KSPECEM_RT();
		kspecem_init_done = true;
	}
}
