import sys
import os
import argparse
import subprocess
import pexpect
from time import sleep

QEMUSOCKET = os.environ['QEMUSOCKET']
SYZKALLER_IMG = os.environ['SYZKALLER_IMG']
SYZKALLER_SSH_PORT = os.environ['SYZKALLER_SSH_PORT']

def getch():
    import sys, tty, termios
    fd = sys.stdin.fileno()
    old_settings = termios.tcgetattr(fd)
    try:
        tty.setraw(sys.stdin.fileno())
        ch = sys.stdin.read(1)
    finally:
        termios.tcsetattr(fd, termios.TCSADRAIN, old_settings)
    return ch

def get_vm_pid():
    pid = exec_command(['head', '-1', FILE_PID])
    if pid and len(pid) > 0:
        return pid[0]
    return None

def kill_vm():
    pid = get_vm_pid()
    if pid:
        exec_command(['kill', pid])
        exec_command(['rm', FILE_PID])
        sleep(0.5)

def expect(qemu, expect, kill_on_timeout=True, kill_on_panic=True, timeout=30):
    i = qemu.expect([
        pexpect.TIMEOUT,
        expect],
        timeout=timeout)
    if i == 0 and kill_on_timeout:
        print('TIMEOUT')
        sys.exit(-1)
    if i >= 2 and kill_on_panic:
        print('PANIC')
        sys.exit(-1)

def exec_command(command):
    process = subprocess.Popen(command,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE)
    stdout, stderr = process.communicate()
    if stderr:
        print('error: {}'.format(stderr))
        return None
    return stdout.decode("utf-8").splitlines()

def socat_command(command, qemusocket):
    echo = subprocess.Popen(['echo', command], stdout=subprocess.PIPE)
    try:
        output = subprocess.check_output(
                ['socat', '-t', '100000', '-', 'unix-connect:{}'.format(qemusocket)],
                stdin=echo.stdout)
        echo.wait()
    except:
        print('{} failed'.format(command))
        sys.exit(-1)

def snapshot_save(snapshot, qemusocket=QEMUSOCKET):
    print('Saving snapshot as {}'.format(snapshot))
    socat_command('savevm {}'.format(snapshot), qemusocket)

def snapshot_load(snapshot, qemusocket=QEMUSOCKET):
    print('Loading snapshot as {}'.format(snapshot))
    socat_command('loadvm {}'.format(snapshot), qemusocket)

def copy_testcases(testprogs_path, syzkaller_ssh_port=SYZKALLER_SSH_PORT):
    print('Copying files onto VM...')
    testprogs = '{}/*.bin'.format(testprogs_path)
    exec_command([
        'sh', '-c',
        'scp -i {}/bullseye.id_rsa -P {} {} root@localhost:/root 2>/dev/null'.format(
            SYZKALLER_IMG, syzkaller_ssh_port, testprogs)])
