#include "kspecem_stack.h"

#include "kspecem_assert.h"

char *kspecem_wl_stack;
void *kspecem_wl_stack_base_address;
uint64_t kspecem_wl_stack_len;

void kspecem_stack_init(void) {
  void *stacklog_bytes = kmalloc(STACKLOG_MAXLEN, GFP_KERNEL);

  kspecem_wl_stack = NULL;
  kspecem_wl_stack_base_address = 0x0;
  kspecem_wl_stack_len = 0x0;

  lt_assert(stacklog_bytes);
	kspecem_wl_stack = stacklog_bytes;
}

/* not all writes to the stack frame of the function are visible within LLVM IR
 * we therefore gracefully save the entire stack frame of the function when
 * creating a checkpoint
*/
void __always_inline kspecem_save_stack(void *frameaddress) {
  char *wl_stack_start = kspecem_wl_stack;
  void *rsp = __builtin_frame_address(0)+0x10;

  kspecem_wl_stack_base_address = frameaddress;
  kspecem_wl_stack_len = kspecem_wl_stack_base_address - rsp;
  lt_assert(kspecem_wl_stack_base_address);
  lt_assert(rsp);
  lt_assert(kspecem_wl_stack_len <= STACKLOG_MAXLEN);
  lt_assert(wl_stack_start);

  /* do the memcpy */
  if(NULL == __memcpy(wl_stack_start,
        kspecem_wl_stack_base_address-kspecem_wl_stack_len,
        kspecem_wl_stack_len)) {
    lt_panic("memcpy for stack saving did not work\n");
  }
}
