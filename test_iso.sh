#!/usr/bin/env bash
set -e
mkdir -p build_test/iso/EFI/BOOT
cp build_test/esp/EFI/BOOT/BOOTX64.EFI build_test/iso/EFI/BOOT/
echo '\EFI\BOOT\BOOTX64.EFI' > build_test/iso/startup.nsh

dd if=/dev/zero of=build_test/iso/efiboot.img bs=1M count=4 status=none
mformat -i build_test/iso/efiboot.img -C -T 8192 ::
mmd -i build_test/iso/efiboot.img ::/EFI
mmd -i build_test/iso/efiboot.img ::/EFI/BOOT
mcopy -i build_test/iso/efiboot.img build_test/iso/EFI/BOOT/BOOTX64.EFI ::/EFI/BOOT/
mcopy -i build_test/iso/efiboot.img build_test/iso/startup.nsh ::/

xorriso -as mkisofs -R -f \
    -e efiboot.img \
    -no-emul-boot \
    -isohybrid-gpt-basdat \
    -V "MIKEOS_UEFI" \
    -o build_test/mikeos_uefi.iso build_test/iso

cp /usr/share/OVMF/OVMF_VARS_4M.fd build_test/vars.fd

echo "Testing hybrid ISO boot with QEMU..."
timeout 10 qemu-system-x86_64 \
    -drive if=pflash,format=raw,readonly=on,file=/usr/share/OVMF/OVMF_CODE_4M.fd \
    -drive if=pflash,format=raw,file=build_test/vars.fd \
    -cdrom build_test/mikeos_uefi.iso \
    -net none \
    -nographic \
    -serial mon:stdio 2>&1 || true

echo "Done test_iso."
