#!/usr/bin/env bash
set -e
mkdir -p build_test/esp/EFI/BOOT

cat << 'EOF' > build_test/test.c
#include <efi.h>
#include <efilib.h>

EFI_STATUS
EFIAPI
efi_main (EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable) {
    InitializeLib(ImageHandle, SystemTable);
    Print(L"\r\n==================================\r\n");
    Print(L"  MikeOS UEFI 64-bit Test Running!\r\n");
    Print(L"==================================\r\n");
    for (volatile long i = 0; i < 80000000; i++);
    return EFI_SUCCESS;
}
EOF

gcc -I/usr/include/efi -I/usr/include/efi/x86_64 \
    -fno-stack-protector -fpic -fshort-wchar -mno-red-zone \
    -DHAVE_USE_MS_ABI -c build_test/test.c -o build_test/test.o

ld -shared -Bsymbolic -L/usr/lib -T /usr/lib/elf_x86_64_efi.lds \
    /usr/lib/crt0-efi-x86_64.o build_test/test.o -o build_test/test.so -lgnuefi -lefi

objcopy -j .text -j .sdata -j .data -j .dynamic -j .dynsym -j .rel \
    -j .rela -j .rel.* -j .rela.* -j .reloc \
    -I elf64-x86-64 -O pei-x86-64 --subsystem 10 \
    build_test/test.so build_test/esp/EFI/BOOT/BOOTX64.EFI

# Create 4MB FAT image for EFI system partition
dd if=/dev/zero of=build_test/efiboot.img bs=1k count=4096 status=none
mformat -i build_test/efiboot.img -C -f 2880 :: || mformat -i build_test/efiboot.img -C -T 8192 ::
mmd -i build_test/efiboot.img ::/EFI
mmd -i build_test/efiboot.img ::/EFI/BOOT
mcopy -i build_test/efiboot.img build_test/esp/EFI/BOOT/BOOTX64.EFI ::/EFI/BOOT/

# Build UEFI bootable ISO using xorriso
mkdir -p build_test/iso_root
cp build_test/efiboot.img build_test/iso_root/efiboot.img
xorriso -as mkisofs -R -f -e efiboot.img -no-emul-boot -o build_test/mikeos_uefi.iso build_test/iso_root 2>/dev/null

echo "Testing ISO boot in QEMU..."
timeout 10 qemu-system-x86_64 \
    -bios /usr/share/OVMF/OVMF_CODE_4M.fd \
    -cdrom build_test/mikeos_uefi.iso \
    -net none \
    -nographic \
    -serial mon:stdio 2>&1 || true

echo "QEMU test completed!"
