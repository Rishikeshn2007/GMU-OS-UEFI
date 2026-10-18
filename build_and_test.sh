#!/usr/bin/env bash
# =============================================================================
# MikeOS 64-bit UEFI - One-command build & test script
# Usage: bash build_and_test.sh [--no-test]
# =============================================================================
set -e

echo "╔══════════════════════════════════════════════════════════╗"
echo "║         MikeOS 64-bit UEFI - Build & Test                ║"
echo "╚══════════════════════════════════════════════════════════╝"

NO_TEST=${1:-""}

# ── 1. Build Kernel ──────────────────────────────────────────
echo ""
echo "[1/3] Building UEFI kernel..."
bash build_kernel.sh

# ── 2. Build ISO ─────────────────────────────────────────────
echo ""
echo "[2/3] Building bootable ISO..."
python3 build_iso.py

echo ""
echo "  ✓ ISO: build_uefi/mikeos-uefi.iso ($(du -sh build_uefi/mikeos-uefi.iso | cut -f1))"
echo "  ✓ EFI: build_uefi/BOOTX64.EFI ($(du -sh build_uefi/BOOTX64.EFI | cut -f1))"

if [[ "$NO_TEST" == "--no-test" ]]; then
    echo ""
    echo "=== Build Complete (test skipped) ==="
    exit 0
fi

# ── 3. Test with QEMU ────────────────────────────────────────
echo ""
echo "[3/3] Testing with QEMU OVMF..."

if ! command -v qemu-system-x86_64 >/dev/null 2>&1; then
    echo "  QEMU not found, skipping test."
    echo "  Install: sudo apt-get install -y qemu-system-x86 ovmf"
    exit 0
fi

if [[ ! -f /usr/share/ovmf/OVMF.fd ]]; then
    echo "  OVMF firmware not found, skipping test."
    echo "  Install: sudo apt-get install -y ovmf"
    exit 0
fi

python3 test_screen.py
echo ""

# Check screenshot
if [[ -f build_uefi/screen.ppm ]]; then
    python3 -c "
with open('build_uefi/screen.ppm', 'rb') as f:
    f.readline(); f.readline(); f.readline()
    data = f.read()
    non_black = sum(1 for i in range(0, len(data), 3) if data[i] != 0 or data[i+1] != 0 or data[i+2] != 0)
    total = len(data) // 3
    pct = non_black * 100 // total
    if pct > 10:
        print(f'  ✓ Screen rendering OK ({pct}% non-black pixels)')
    else:
        print(f'  ✗ Screen mostly black ({pct}% non-black pixels) - check QEMU boot')
"
    # Convert to PNG
    python3 ppm2png.py 2>/dev/null || true
fi

echo ""
echo "╔══════════════════════════════════════════════════════════╗"
echo "║  Build & Test Complete!                                  ║"
echo "║                                                          ║"
echo "║  To boot on real hardware:                               ║"
echo "║    Flash build_uefi/mikeos-uefi.iso to USB with:         ║"
echo "║    sudo dd if=build_uefi/mikeos-uefi.iso of=/dev/sdX     ║"
echo "║    (replace sdX with your USB drive!)                    ║"
echo "╚══════════════════════════════════════════════════════════╝"
