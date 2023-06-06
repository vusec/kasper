#!/usr/bin/env bash

KERNEL="-kernel ${KERNEL}/arch/x86/boot/bzImage"
APPEND="console=ttyS0 nokaslr nosmp maxcpus=1 rcu_nocbs=0 nmi_watchdog=0 ignore_loglevel modules=sd-mod,usb-storage,ext4 rootfstype=ext4 earlyprintk=serial"
APPEND+=" biosdevname=0 kvm-intel.emulate_invalid_guest_state=1 kvm-intel.enable_apicv=1 kvm-intel.enable_shadow_vmcs=1 kvm-intel.ept=1 kvm-intel.eptad=1 kvm-intel.fasteoi=1 kvm-intel.flexpriority=1 kvm-intel.nested=1 kvm-intel.pml=1 kvm-intel.unrestricted_guest=1 kvm-intel.vmm_exclusive=1 kvm-intel.vpid=1 net.ifnames=0"
MEMORY="8192"

TYPE=$1
if [[ $TYPE = "syzkaller" ]]; then
  RAMDISK=
  HDA="${SYZKALLER_IMG}/bullseye.img"
  APPEND="${APPEND} root=/dev/sda"
  if [ -n "${SYZKALLER_SSH_PORT}" ]; then
    NET1="nic,model=e1000"
    NET2="user,host=10.0.2.10,hostfwd=tcp::${SYZKALLER_SSH_PORT}-:22"
  fi
else
  RAMDISK="${INITRAMFS}"
  HDA=
fi

set -x
qemu-system-x86_64 \
  ${KERNEL} \
  ${APPEND:+ -append "${APPEND}"} \
  ${RAMDISK:+ -initrd "${RAMDISK}"} \
  ${HDA:+ -hda "${HDA}"} \
  ${ATTACH_GDB:+ -gdb tcp::${GDB_PORT}} \
  ${ATTACH_GDB:+ -S} \
  ${NET1:+ -net ${NET1}} \
  ${NET2:+ -net ${NET2}} \
  ${ENABLE_KVM:+ -enable-kvm} \
  -display none \
  -monitor unix:${QEMUSOCKET},server,nowait \
  -smp 1 \
  ${VMDRIVE:+ -drive if=none,format=qcow2,file="${VMDRIVE}"} \
  ${LOADVM:+ -loadvm "${LOADVM}"} \
  -cpu qemu64,+smep,-smap \
  -m ${MEMORY} \
  -echr 17 \
  -serial mon:stdio \
  -snapshot \
  ${PIDFILE:+ -pidfile ${PIDFILE}} \
  -no-reboot

