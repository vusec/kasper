import sys
import os
import pexpect
import argparse
from time import sleep

from common import getch, exec_command, snapshot_save, snapshot_load, copy_testcases, expect

FILE_PID = ".qemupid"
SYZKALLER_SSH_PORT = 1580

snapshot_name = "tmp_with_testprogs"

parser = argparse.ArgumentParser(description='pexpect run spectrestar with qemu')
parser.add_argument('--target', dest='target', default="syzkaller", type=str)
parser.add_argument('--loadvm', dest='loadvm', default=None, type=str)
parser.add_argument('--testprogs', dest='testprogs', default=None, type=str)
parser.add_argument('--gdb', dest='gdb', action='store_const', const=sum)
parser.add_argument('--interactive', '-i', dest='interactive', action='store_const', const=sum)
parser.add_argument('--restart-stats', dest='restart_stats', action='store_const', const=sum)
parser.add_argument('--restart-inlineasm-stats', dest='restart_inlineasm_stats',
        action='store_const', const=sum)

# kspecem options
parser.add_argument('--whitelist', dest='whitelist', default=None, type=str)
tests_args = parser.add_mutually_exclusive_group()
tests_args.add_argument('--tests', dest='run_tests', action='store_const', const=sum)
tests_args.add_argument('--tests-with-kocher', dest='run_tests_with_kocher', action='store_const', const=sum)
reports_args = parser.add_mutually_exclusive_group()
reports_args.add_argument('--no-reports', dest='no_reports', action='store_const', const=sum)
reports_args.add_argument('--duplicate-reports', dest='duplicate_reports', action='store_const', const=sum)
parser.add_argument('--no-speculation', dest='no_speculation', action='store_const', const=sum)
parser.add_argument('--no-calltraces', dest='no_calltraces', action='store_const', const=sum)
policies_args = parser.add_mutually_exclusive_group()
policies_args.add_argument('--specfuzz', dest='run_specfuzz_policies', action='store_const', const=sum)
policies_args.add_argument('--spectaint', dest='run_spectaint_policies', action='store_const', const=sum)
policies_args.add_argument('--kasper-phtsyscallcc', dest='run_kasper_phtsyscallcc_policies', action='store_const', const=sum)

# currently unused, for universal taskfile implemnetation
policies_args.add_argument('--kasper-phtsyscallcc-with-lvi-massaging', dest='run_kasper_phtsyscallcc__with_lvi_massaging_policies', action='store_const', const=sum)

parser.add_argument('--policy-cmd', dest='policy_cmd', default='ls', type=str)
parser.add_argument('--policy-eval', dest='policy_eval', action='store_const', const=sum)

args = parser.parse_args()

print('target: {}'.format(args.target))
if args.target == 'syzkaller':
    PROMPT = "root@syzkaller:~#"
else:
    PROMPT = "/ #"

env = os.environ
env['PIDFILE'] = FILE_PID

if args.gdb:
    env['ATTACH_GDB'] = '1'
if args.loadvm:
    env['LOADVM'] = args.loadvm
if args.target == 'syzkaller' and args.testprogs:
    env['QEMUSOCKET'] = '{}-interact'.format(os.environ['QEMUSOCKET'])
    env['SYZKALLER_SSH_PORT'] = '{}'.format(SYZKALLER_SSH_PORT)

qemu = pexpect.spawn('scripts/run-qemu.sh {}'.format(args.target),
        env=env,
        encoding='utf-8')
qemu.logfile = sys.stdout

# login
if args.target == 'syzkaller':
    expect(qemu, 'syzkaller login:', timeout=None)
    qemu.sendline('root')

# setup
expect(qemu, PROMPT, timeout=None)
qemu.sendline("printf '#!/bin/sh\\nls -la\\n' > kasper_task ; chmod +x kasper_task")
expect(qemu, PROMPT)
qemu.sendline("printf '#!/bin/sh\\nfor i in *.bin;do echo $i;./$i;./$i;./$i;./$i;./$i;./$i;./$i;./$i;./$i;./$i;" +
    "done\\n' > testcases ; chmod +x testcases")
expect(qemu, PROMPT)
qemu.sendline("printf '#!/bin/sh\\ncat /sys/kernel/debug/kasper/enable\\n' > e ; chmod +x e")
expect(qemu, PROMPT)

test_content = 'a' * 100
qemu.sendline("printf '{}\\n' > test1.txt".format(test_content))
expect(qemu, PROMPT)
for num in range(41):
    qemu.sendline("cat test1.txt >> test.txt")
    expect(qemu, PROMPT)

if args.target == 'syzkaller' and args.testprogs:
    copy_testcases(args.testprogs, SYZKALLER_SSH_PORT)

qemu.sendline('mount -t debugfs none /sys/kernel/debug')
expect(qemu, PROMPT)
sleep(1)

if args.whitelist == 't':
    # KSPECEM_WHITELIST_TASKS = 1
    qemu.sendline('echo 1 > /sys/kernel/debug/kasper/whitelist')
    expect(qemu, PROMPT)
elif args.whitelist == 'd':
    # KSPECEM_WHITELIST_DISABLED = 0
    qemu.sendline('echo 0 > /sys/kernel/debug/kasper/whitelist')
    expect(qemu, PROMPT)
elif args.whitelist == 's':
    # KSPECEM_WHITELIST_SYSCALL = 2
    qemu.sendline('echo 2 > /sys/kernel/debug/kasper/whitelist')
    expect(qemu, PROMPT)

if args.run_tests_with_kocher:
    qemu.sendline('echo 1 > /sys/kernel/debug/kdfsan/run_kocher_tests')
    expect(qemu, PROMPT)
if args.run_tests:
    qemu.sendline('echo 1 > /sys/kernel/debug/kdfsan/run_tests')
    expect(qemu, PROMPT)

if args.no_reports:
    # PRINT_NONE = 1
    qemu.sendline('echo 1 > /sys/kernel/debug/kasper/print_reports')
    expect(qemu, PROMPT)
elif args.duplicate_reports:
    # PRINT_DUPLICATES = 2
    qemu.sendline('echo 2 > /sys/kernel/debug/kasper/print_reports')
    expect(qemu, PROMPT)

if args.no_speculation:
    qemu.sendline('echo 0 > /sys/kernel/debug/kasper/spec_enabled')
    expect(qemu, PROMPT)

if args.no_calltraces:
    qemu.sendline('echo 0 > /sys/kernel/debug/kasper/calltrace_enabled')
    expect(qemu, PROMPT)

if args.restart_inlineasm_stats:
    qemu.sendline('echo 1 > /sys/kernel/debug/kasper/inlineasm_restart_stats_enabled')
    expect(qemu, PROMPT)

if args.run_specfuzz_policies:
    qemu.sendline('echo 1 > /sys/kernel/debug/kdfsan/run_specfuzz_policies')
    expect(qemu, PROMPT)
elif args.run_spectaint_policies:
    qemu.sendline('echo 1 > /sys/kernel/debug/kdfsan/run_spectaint_policies')
    expect(qemu, PROMPT)
elif args.run_kasper_phtsyscallcc_policies:
    qemu.sendline('echo 1 > /sys/kernel/debug/kdfsan/report_only_pht_syscall_cc')
    expect(qemu, PROMPT)

if args.policy_eval:
    qemu.sendline('echo 1 > /sys/kernel/debug/kdfsan/syscall_label_type')
    expect(qemu, PROMPT)

if args.interactive:
    qemu.logfile = None
    qemu.interact()
else:
    qemu.sendline('./e')
    expect(qemu, 'init kspecem writelog')
    expect(qemu, PROMPT)

    if args.restart_stats:
        for num in range(6):
            qemu.sendline('./kasper_task')
            expect(qemu, PROMPT, timeout=None)
            sleep(1)

    if args.restart_stats:
        qemu.sendline('cat /sys/kernel/debug/kasper/restarts')
        expect(qemu, PROMPT)

    if args.policy_eval:
        qemu.sendline(args.policy_cmd)
        expect(qemu, PROMPT, timeout=10000)

print('expect successful!')
