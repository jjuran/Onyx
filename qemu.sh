#!/bin/sh
set -e
. ./iso.sh
QEMU_PREFIX=${QEMU_PREFIX:-/usr/bin}


$QEMU_PREFIX/qemu-system-$(./target-triplet-to-arch.sh $HOST) -s -cdrom Onyx.iso -drive file=hdd.img,format=raw,media=disk -m 512M -monitor stdio -boot d -net nic,model=rtl8139 -net dump,file=net.pcap -net user \
--enable-kvm -smp 2 -cpu IvyBridge,+avx -d int -vga vmware
