#!/bin/sh

sleep 2

while pkill -0 gdb; do
  sleep 0.5
done

pgrep qemu | xargs kill