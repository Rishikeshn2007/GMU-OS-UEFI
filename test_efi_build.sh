#!/usr/bin/env bash
set -e
mkdir -p build_test

cat << 'EOF' > build_test/test.c
#include <efi.h>
#include <efilib.h>

EFI_STATUS
EFIAPI
efi_main (EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable) {
    InitializeLib(ImageHandle, SystemTable);
    Print(L"MikeOS UEFI 64-bit Test\n");
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
    --target efi-app-x86_64 build_test/test.so build_test/BOOTX64.EFI

file build_test/BOOTX64.EFI
echo "Test build succeeded!"
