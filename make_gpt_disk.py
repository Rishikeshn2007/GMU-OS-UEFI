#!/usr/bin/env python3
import os, subprocess

os.system("dd if=/dev/zero of=build_test/disk2.img bs=1M count=64 status=none")
os.system("/usr/sbin/sgdisk -n 1:2048:0 -t 1:ef00 build_test/disk2.img")
# format with mformat using partition offset: 2048 sectors = 1048576 bytes
# mtools supports @@offset
os.system("mformat -i build_test/disk2.img@@1048576 -F -H 2048 ::")
os.system("mmd -i build_test/disk2.img@@1048576 ::/EFI")
os.system("mmd -i build_test/disk2.img@@1048576 ::/EFI/BOOT")
os.system("mcopy -i build_test/disk2.img@@1048576 build_test/esp/EFI/BOOT/BOOTX64.EFI ::/EFI/BOOT/")
print("Created disk2.img")
