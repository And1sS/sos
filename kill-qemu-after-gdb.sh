#!/bin/sh

sleep 2

while pkill -0 x86_64-elf-gdb; do
  sleep 0.5
done

pkill qemu-system-x86_64