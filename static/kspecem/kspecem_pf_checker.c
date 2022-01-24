#include "kspecem_pf_checker.h"

#include "kspecem_kdf.h"

/* Page-fault checker hook
    This is in the kspecem static lib because it only works
    in code that is not instrumented by kspecem;
    otherwise the hook would recursively call back into itself
*/

#define DUMMY_REGION_SIZE PAGE_SIZE
u8 * pf_dummy_region = (void*) -1;

void kspecem_init_pf_checker(void) {
  /* Using __GFP_NO_KDFSAN_SHADOW because we want to make sure
      this region has no shadow.
      (TODO: Should other internal data be allocated with __GFP_NO_KDFSAN_SHADOW?)
  */
  pf_dummy_region = kzalloc(DUMMY_REGION_SIZE, GFP_KERNEL | __GFP_NO_KDFSAN_SHADOW);
}

__always_inline const void *kspecem_access(const void *ptr, size_t size) {
  /* If the access's pointer is not mapped and the access's size
      is not greater than the dummy region, change the pointer to
      the dummy region
  */
  const void *ret_ptr;
  if(!kdf_virt_addr_valid((void*) ptr) && size < DUMMY_REGION_SIZE) {
    ret_ptr = pf_dummy_region;
  } else {
    ret_ptr = ptr;
  }
  return ret_ptr;
}
