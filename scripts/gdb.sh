#!/usr/bin/env bash

# Note: Use hbreak in gdb-cmds.txt to set breakpoints (for some reason software
# breakpoints are not being hit when kvm is enabled).
# If a software breakpoint is desired, set 'hbreak start_kernel', then when
# that is hit, set the software breakpoint

SCRIPTPATH="$( cd -- "$(dirname "$0")" >/dev/null 2>&1 ; pwd -P )"

KERNEL=$SCRIPTPATH/../kdfsan-linux
GDB="${GDB:=gdb}"
GDB_PORT="${GDB_PORT:=45457}"
GDB_CMDS="${GDB_CMDS:=$SCRIPTPATH/gdb-cmds.txt}"
CONTINUE="${CONTINUE:=C}"

${GDB} \
  -q \
  -ex "add-auto-load-safe-path ${KERNEL}" \
  -ex "file ${KERNEL}/vmlinux" \
  -ex "target remote localhost:${GDB_PORT}" \
  -ex "source ${GDB_CMDS}" \
  ${CONTINUE:+ -ex "continue"}
