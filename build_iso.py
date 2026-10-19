import os
import shutil
import subprocess
from pathlib import Path

apps = sorted(Path("build_uefi").glob("*.bin"))
if not apps:
    raise SystemExit("No .bin applications found; build the kernel first.")
os.makedirs("build_uefi/iso", exist_ok=True)
for old_app in ("CALCULATOR.EFI", "HANGMAN.EFI"):
    (Path("build_uefi/iso") / old_app).unlink(missing_ok=True)
for app_path in apps:
    shutil.copy2(app_path, Path("build_uefi/iso") / app_path.name)

# Create clean ISO
subprocess.run(["bash", "-e", "-c", """
mkdir -p build_uefi/iso/EFI/BOOT
cp build_uefi/BOOTX64.EFI build_uefi/iso/EFI/BOOT/
echo "\\EFI\\BOOT\\BOOTX64.EFI" > build_uefi/iso/startup.nsh

dd if=/dev/zero of=build_uefi/iso/efiboot.img bs=1M count=33 status=none
mformat -i build_uefi/iso/efiboot.img -F -T 67584 ::
mmd -i build_uefi/iso/efiboot.img ::/EFI
mmd -i build_uefi/iso/efiboot.img ::/EFI/BOOT
mcopy -i build_uefi/iso/efiboot.img build_uefi/BOOTX64.EFI ::/EFI/BOOT/
mcopy -i build_uefi/iso/efiboot.img build_uefi/iso/startup.nsh ::/
mcopy -i build_uefi/iso/efiboot.img build_uefi/*.bin ::/

xorriso -as mkisofs -R -f \
    -e efiboot.img \
    -no-emul-boot \
    -isohybrid-gpt-basdat \
    -V "MIKEOS_UEFI" \
    -o build_uefi/mikeos-uefi.iso build_uefi/iso 2>&1
"""], check=True)

print("ISO generated.")
