#!/bin/sh

make iso

qemu-system-x86_64 -D ./log.txt -d int,cpu_reset -s -S -cdrom source/build_output/sos.iso &
./kill-qemu-after-gdb.sh &