#!/usr/bin/env python3
import struct, os, subprocess

# Create a 32MB disk image with MBR partition type 0xEF (EFI System Partition)
IMG_SIZE = 32 * 1024 * 1024
PART_START_SECTOR = 2048 # 1MB offset
PART_SECTORS = (IMG_SIZE // 512) - PART_START_SECTOR

img_path = "build_test/disk.img"
part_path = "build_test/part.img"

# 1. Format the partition as FAT32
os.system(f"dd if=/dev/zero of={part_path} bs=1M count=31 status=none")
os.system(f"mformat -i {part_path} -F -T {PART_SECTORS} ::")
os.system(f"mmd -i {part_path} ::/EFI")
os.system(f"mmd -i {part_path} ::/EFI/BOOT")
os.system(f"mcopy -i {part_path} build_test/esp/EFI/BOOT/BOOTX64.EFI ::/EFI/BOOT/")

# 2. Build disk image with MBR partition table
with open(img_path, "wb") as f:
    # 512 bytes MBR
    mbr = bytearray(512)
    # Partition entry 1 at offset 446 (0x1BE)
    # Status: 0x80 (bootable)
    mbr[0x1BE] = 0x80
    # Type: 0xEF (EFI System Partition)
    mbr[0x1C2] = 0xEF
    # LBA Start: 2048
    struct.pack_into("<I", mbr, 0x1C6, PART_START_SECTOR)
    # Sector count: PART_SECTORS
    struct.pack_into("<I", mbr, 0x1CA, PART_SECTORS)
    # Boot signature 0x55AA
    mbr[510] = 0x55
    mbr[511] = 0xAA
    f.write(mbr)
    # Pad until 1MB (sector 2048)
    f.write(b"\x00" * (PART_START_SECTOR * 512 - 512))
    # Write partition
    with open(part_path, "rb") as pf:
        f.write(pf.read())

print("Created disk.img successfully!")
