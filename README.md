# Kasper: Scanning for Generalized Transient Execution Gadgets in the Linux Kernel


### Setting up ###

Install dependencies, including [go-task](https://taskfile.dev/#/installation) as a task-runner:
```
sudo apt install build-essential clang-11 lld-11 libelf-dev qemu-system-x86 bison flex golang libssl-dev cmake debootstrap python3-pexpect socat ninja-build ccache
sudo sh -c "$(curl -ssL https://taskfile.dev/install.sh)" -- -d -b /usr/local/bin
```

Initialize/update git submodules (this will take awhile the first time it's run):
```
task update
```

### Building ###

Create an initramfs and [a disk image to be used with syzkaller](https://github.com/google/syzkaller/blob/master/docs/linux/setup_ubuntu-host_qemu-vm_x86-64-kernel.md#image):
```
task initramfs:create
task syzkaller:create-image
```

Configure and build [LLVM with Kasper support](https://github.com/vusec/kdfsan-llvm-project/tree/kasper-llvm-v11):
```
task llvm:config llvm:build
```

Build [syzkaller with Kasper support](https://github.com/vusec/kdfsan-syzkaller/tree/kasper-syzkaller):

**WARNING**: the version of syzkaller we use only works with golang 1.15 (we are using golang 1.15.15)!
```
task syzkaller:build
```

Configure and build a [Kasper-instrumented Linux kernel](https://github.com/vusec/kdfsan-linux/tree/kasper-linux-v5.12):
```
task kernel:config build kernel:bzImage
```


### Running ###

Test that the instrumented kernel runs correctly:
```
task qemu:test
```

Fuzz the instrumented kernel:
```
task syzkaller:run-nobench
```

### Evaluation ###

To aggregate gadgets and run the evaluation please check out [kasper-results](https://github.com/vusec/kasper-results).
