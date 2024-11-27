#! /usr/bin/bash

# assumes the DTB was added to the build output folder
qemu-system-aarch64 -M raspi3b -kernel build/kernel/kernel8.img -serial null -serial stdio -dtb build/kernel/bcm2710-rpi-3-b.dtb