set(SOURCES
  kspecem_assert.c
  kspecem_bugs.c
  kspecem_common.c
  kspecem_interface.c
  kspecem_internal.c
  kspecem_interrupt.c
  kspecem_init.c
  kspecem_kdf.c
  kspecem_registers.c
  kspecem_report.c
  kspecem_stack.c
  kspecem_statistics.c
  kspecem_stats_inlineasm.c
  kspecem_stats_restarts.c
  kspecem_undolog.c
  kspecem_utils.c
  kspecem_pf_checker.c
  kspecem_whitelist.c
  kspecem_regs.S
)

include_directories(./include)

enable_language(ASM)

add_library(kspecem STATIC
  ${SOURCES}
)
