#!/usr/bin/env python3

import subprocess, os, argparse, sys, time, datetime, json
from os import listdir
from os.path import isfile, join

SYZKALLER_BIN = os.environ.get('SYZKALLER_BIN')
SYZ_PROG2C = '{}/syz-prog2c'.format(SYZKALLER_BIN)
GCC = 'gcc'

def show(value, max_value, prefix="", size=50, file=sys.stdout):
    x = int(size*value / max_value)
    file.write("%s[%s%s] %i/%i\r" % (prefix, "#"*x, "."*(size-x), value, max_value))
    file.flush()

def progressbar(it, prefix="", size=50, file=sys.stdout):
    count = len(it)
    print('\x1b[2K\r')
    show(0, count, prefix, size)
    for i, item in enumerate(it):
        yield item
        show(i+1, count, prefix, size)
    file.write("\n")
    file.flush()

def prog2c(testcase_path):
    process = subprocess.Popen([SYZ_PROG2C, '--prog', testcase_path],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE)
    stdout, stderr = process.communicate()
    if process.returncode != 0:
        print(stderr)
        return None
    c_code = stdout.decode("utf-8").splitlines()
    return c_code

def c2bin(c_path, bin_path):
    process = subprocess.Popen([GCC, c_path, '-o', bin_path],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE)
    stdout, stderr = process.communicate()
    if process.returncode != 0:
        print(stderr)

def compile_testcase(testcase):
    testcase_c_path = testcase.replace('.txt', '.c')
    testcase_bin_path = testcase.replace('.txt', '.bin')
    c_prog = prog2c(testcase)
    if c_prog:
        with open(testcase_c_path, "w") as f:
            f.write('\n'.join(c_prog))
        c2bin(testcase_c_path, testcase_bin_path)


def main():
    parser = argparse.ArgumentParser(description='compile-testcases.py')
    parser.add_argument('-d', '--testcase-dir')
    parser.add_argument('-t', '--testcase')
    parser.add_argument('--c2bin-only', dest='c2bin_only', action='store_const', const=sum, default=False)
    args = parser.parse_args()

    if args.testcase_dir:
        path = args.testcase_dir

        if not args.c2bin_only:
            testcases = [join(path, f) for f in listdir(path) if isfile(join(path, f)) and '.txt' in f]
            for testcase in progressbar(testcases, "Compile testcases: "):
                compile_testcase(testcase)
        else:
            testcases = [join(path, f) for f in listdir(path) if isfile(join(path, f)) and '.c' in f]
            for testcase in progressbar(testcases, "Compile testcases: "):
                testcase_bin_path = testcase.replace('.c', '.bin')
                c2bin(testcase, testcase_bin_path)

    elif args.testcase:
        compile_testcase(args.testcase)

if __name__ == "__main__":
    main()
