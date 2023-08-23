#!/bin/sh

make iso

qemu-system-x86_64 -s -S -cdrom build_output/sos.iso &
./kill-qemu-after-gdb.sh &