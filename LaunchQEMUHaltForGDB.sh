#! /usr/bin/bash

# assumes the DTB was added to the build output folder
# halts QEMU on the first instruction so a debugger can be attached
qemu-system-aarch64 -M raspi3b -kernel build/kernel/kernel8.img -s -S -serial null -serial stdio -dtb build/kernel/bcm2710-rpi-3-b.dtb