#ifndef KSPECEM_KSPECEM_REPORT_H
#define KSPECEM_KSPECEM_REPORT_H 1

#include "kspecem_types.h"
#include "kspecem_bugs.h"

extern bool kspecem_report_error_dump_stack;

void kspecem_report_print_access_bug(
    struct kspecem_bug *this_bug);
void kspecem_report_print_restart_begin(void);
void kspecem_report_print_restart_end(unsigned long current_syscall_nr,
    void *current_checkpoint_ip, int restart_type);

#endif
