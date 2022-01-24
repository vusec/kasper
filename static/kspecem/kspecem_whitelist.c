#include "kspecem_whitelist.h"

#include "kspecem_utils.h"

kspecem_whitelist_type_t kspecem_whitelist_type;

void kspecem_whitelist_init(void) {
  kspecem_whitelist_type = KSPECEM_WHITELIST_SYSCALL;
}

/* currently we want to limit checkpointing to 'user-controllable' tasks */
int __always_inline kspecem_is_whitelist_task(void) {
  switch(kspecem_whitelist_type) {
    case KSPECEM_WHITELIST_DISABLED:
      return 1;
      break;
    case KSPECEM_WHITELIST_SYSCALL: {
      unsigned long orig_ax = kspecem_syscall_get_nr();
      if (orig_ax < 600 || orig_ax > 1200) {
        return 0;
      }
      /* fall through */
    }
    case KSPECEM_WHITELIST_TASKS: {
      char *task_name = current->comm;
      // taskname should either:
      // (a) be exactly "kasper_task" (for kdfsan_tests), or
      // (b) begin with "syz-executor" (for syzkaller), or
      // (c) be exactly "ls" (for restart_stats)
      if (kspecem_strncmp(task_name, "kasper_task", TASK_COMM_LEN) == 0 ||
          kspecem_strncmp(task_name, "syz-executor", kspecem_strlen("syz-executor")) == 0 ||
          kspecem_strncmp(task_name, "mkdir", TASK_COMM_LEN) == 0 ||
          kspecem_strncmp(task_name, "pwd", TASK_COMM_LEN) == 0 ||
          kspecem_strncmp(task_name, "cat", TASK_COMM_LEN) == 0 ||
          kspecem_strncmp(task_name, "tail", TASK_COMM_LEN) == 0 ||
          kspecem_strncmp(task_name, "ls", TASK_COMM_LEN) == 0) {
        return 1;
      }
      return 0;
      break;
  }
    default:
      break;
  };
  return 1;
}
