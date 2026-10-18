import os
import shutil

apps = ("CALCULATOR.EFI", "HANGMAN.EFI")
os.makedirs("build_uefi/iso", exist_ok=True)
for app in apps:
    app_path = os.path.join("build_uefi", app)
    if not os.path.isfile(app_path):
        raise SystemExit(f"Missing executable {app_path}; build the kernel first.")
    shutil.copy2(app_path, os.path.join("build_uefi/iso", app))

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
mcopy -i build_uefi/iso/efiboot.img build_uefi/CALCULATOR.EFI ::/
mcopy -i build_uefi/iso/efiboot.img build_uefi/HANGMAN.EFI ::/

xorriso -as mkisofs -R -f \
    -e efiboot.img \
    -no-emul-boot \
    -isohybrid-gpt-basdat \
    -V "MIKEOS_UEFI" \
    -o build_uefi/mikeos-uefi.iso build_uefi/iso 2>&1
""")

print("ISO generated.")
