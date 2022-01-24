#ifndef KSPECEM_KSPECEM_BUGS_H
#define KSPECEM_KSPECEM_BUGS_H 1

#include "kspecem_types.h"

#define BUGS_LIST_SIZE 100
#define LABEL_INFO_STR_MAX 400

// From mm/kasan/kasan.h
// TODO: This should be included properly
struct kasan_access_info {
  const void *access_addr;
  const void *first_bad_addr;
  size_t access_size;
  bool is_write;
  unsigned long ip;
};

/********/

struct kspecem_label_info {
  dfsan_label label;
  char info[LABEL_INFO_STR_MAX];
};

typedef enum {
  UNDEF_ACCESS = 0,
  KASAN_ACCESS,
  MDS_ACCESS,
  CC_ACCESS,
  PC_ACCESS
} access_type_enum;
struct kspecem_bug {
  struct kasan_access_info kinfo;
  bool in_spec;
  unsigned int spec_length;
  struct kspecem_label_info data_label;
  struct kspecem_label_info ptr_label;
  access_type_enum access_type;
};

typedef enum {PRINT_DEFAULT = 0, PRINT_NONE, PRINT_DUPLICATES} report_print_level_enum;
extern report_print_level_enum kspecem_print_level;

/********/


extern int volatile kspecem_next_bug_index;
extern struct kspecem_bug *kspecem_bugs;

void kspecem_report_clear_globals(void);
void kspecem_report_print_and_clear(bool called_from_restart, int restart_type);

void __always_inline kspecem_kasan_report(unsigned long addr, size_t size,
    bool is_write, unsigned long ip);
void __always_inline kspecem_ridl_report(unsigned long addr, size_t size,
    bool is_write, unsigned long ip,
    dfsan_label data_label, dfsan_label ptr_label);
void __always_inline kspecem_specv1_report(unsigned long addr, size_t size,
    bool is_write, unsigned long ip,
    dfsan_label data_label, dfsan_label ptr_label);
void __always_inline kspecem_smotherspectre_report(unsigned long ip, dfsan_label label);

void kspecem_report_init(void);

#endif
