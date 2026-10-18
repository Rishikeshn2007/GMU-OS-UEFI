import subprocess, time, os

# Create clean ISO
os.system("""
mkdir -p build_uefi/iso/EFI/BOOT
cp build_uefi/BOOTX64.EFI build_uefi/iso/EFI/BOOT/
echo "\\EFI\\BOOT\\BOOTX64.EFI" > build_uefi/iso/startup.nsh

dd if=/dev/zero of=build_uefi/iso/efiboot.img bs=1M count=33 status=none
mformat -i build_uefi/iso/efiboot.img -F -T 67584 ::
mmd -i build_uefi/iso/efiboot.img ::/EFI
mmd -i build_uefi/iso/efiboot.img ::/EFI/BOOT
mcopy -i build_uefi/iso/efiboot.img build_uefi/BOOTX64.EFI ::/EFI/BOOT/
mcopy -i build_uefi/iso/efiboot.img build_uefi/iso/startup.nsh ::/

xorriso -as mkisofs -R -f \
    -e efiboot.img \
    -no-emul-boot \
    -isohybrid-gpt-basdat \
    -V "MIKEOS_UEFI" \
    -o build_uefi/mikeos-uefi.iso build_uefi/iso 2>&1
""")

print("ISO generated.")
