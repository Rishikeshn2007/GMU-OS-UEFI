#!/usr/bin/env bash
# =============================================================================
# MikeOS 64-bit UEFI Kernel - Master Build Script
# Strategy 1: Clang/LLD -> native PE32+ EFI (cleanest approach)
# Strategy 2: GCC + GNU-EFI + Python PE reloc patcher (fallback)
# =============================================================================
set -e

echo "=== Building MikeOS 64-bit UEFI Kernel ==="

SRC_DIR="source"
FEAT_DIR="source/features"
BUILD_DIR="build_uefi"

mkdir -p "$BUILD_DIR"

SOURCES=(
    "$SRC_DIR/kernel.c"
    "$FEAT_DIR/font8x16.c"
    "$FEAT_DIR/screen.c"
    "$FEAT_DIR/keyboard.c"
    "$FEAT_DIR/mouse.c"
    "$FEAT_DIR/text.c"
    "$FEAT_DIR/disk.c"
    "$FEAT_DIR/sound.c"
    "$FEAT_DIR/misc.c"
    "$FEAT_DIR/dialog.c"
    "$FEAT_DIR/cli.c"
)

# ─────────────────────────────────────────────────────
# Strategy 1: Clang native PE32+ build
# ─────────────────────────────────────────────────────
if command -v clang >/dev/null 2>&1 && command -v lld-link >/dev/null 2>&1; then
    echo ">>> Using Clang/LLD native PE32+ build path"

    EFI_INC="/usr/include/efi"
    CLANG_FLAGS=(
        -target x86_64-unknown-windows
        -ffreestanding
        -fno-stack-protector
        -fshort-wchar
        -mno-red-zone
        -nostdlib
        -O2
        -Wall
        -I"$EFI_INC"
        -I"$EFI_INC/x86_64"
        -Isource
        -Isource/features
        -DHAVE_USE_MS_ABI
    )

    OBJS=()
    for src in "${SOURCES[@]}"; do
        obj="$BUILD_DIR/$(basename "$src" .c).o"
        echo "  Clang: $src"
        clang "${CLANG_FLAGS[@]}" -c "$src" -o "$obj"
        OBJS+=("$obj")
    done

    lld-link \
        -subsystem:efi_application \
        -entry:efi_main \
        -out:"$BUILD_DIR/BOOTX64.EFI" \
        /usr/lib/gnuefi/x86_64/crt0-efi-x86_64.o \
        "${OBJS[@]}" \
        /usr/lib/gnuefi/x86_64/libgnuefi.a \
        /usr/lib/x86_64-linux-gnu/libefi.a 2>/dev/null || \
    lld-link \
        -subsystem:efi_application \
        -entry:efi_main \
        -out:"$BUILD_DIR/BOOTX64.EFI" \
        "${OBJS[@]}"

    echo ">>> Clang build complete."

# ─────────────────────────────────────────────────────
# Strategy 2: GCC + GNU-EFI + PE reloc patcher
# ─────────────────────────────────────────────────────
else
    echo ">>> Clang not found, using GCC + gnu-efi + PE reloc patcher"

    # Find gnu-efi paths
    EFI_INC="/usr/include/efi"
    GNUEFI_LIB_DIRS=(
        "/usr/lib/x86_64-linux-gnu"
        "/usr/lib"
        "/usr/lib/gnuefi"
        "/usr/lib/gnuefi/x86_64"
    )

    # Find crt0 and efi.lds
    CRT0=""
    EFI_LDS=""
    GNUEFI_LIB=""
    EFI_LIB=""

    for d in "${GNUEFI_LIB_DIRS[@]}"; do
        [[ -z "$CRT0" && -f "$d/crt0-efi-x86_64.o" ]] && CRT0="$d/crt0-efi-x86_64.o"
        [[ -z "$EFI_LDS" && -f "$d/elf_x86_64_efi.lds" ]] && EFI_LDS="$d/elf_x86_64_efi.lds"
        [[ -z "$GNUEFI_LIB" && -f "$d/libgnuefi.a" ]] && GNUEFI_LIB="$d/libgnuefi.a"
        [[ -z "$EFI_LIB" && -f "$d/libefi.a" ]] && EFI_LIB="$d/libefi.a"
    done

    if [[ -z "$CRT0" || -z "$EFI_LDS" ]]; then
        echo "ERROR: gnu-efi support files not found. Install: sudo apt-get install gnu-efi"
        echo "  Searched for: crt0-efi-x86_64.o and elf_x86_64_efi.lds"
        exit 1
    fi

    echo "  CRT0:    $CRT0"
    echo "  LDS:     $EFI_LDS"
    echo "  GNUEFI:  $GNUEFI_LIB"
    echo "  EFI:     $EFI_LIB"

    CFLAGS=(
        -I"$EFI_INC"
        -I"$EFI_INC/x86_64"
        -Isource
        -Isource/features
        -ffreestanding
        -fno-builtin
        -fno-stack-protector
        -fpic
        -fshort-wchar
        -mno-red-zone
        -DHAVE_USE_MS_ABI
        -O2
        -Wall
        -Wextra
        -Wno-unused-parameter
    )

    OBJS=()
    for src in "${SOURCES[@]}"; do
        obj="$BUILD_DIR/$(basename "$src" .c).o"
        echo "  GCC: $src"
        gcc "${CFLAGS[@]}" -c "$src" -o "$obj"
        OBJS+=("$obj")
    done

    echo "  Linking ELF shared object..."
    ld -shared -Bsymbolic -L/usr/lib -T "$EFI_LDS" \
        "$CRT0" "${OBJS[@]}" \
        -o "$BUILD_DIR/mikeos.so" \
        ${GNUEFI_LIB:+-l:$(basename "$GNUEFI_LIB")} \
        ${EFI_LIB:+-l:$(basename "$EFI_LIB")} \
        -L"$(dirname "$GNUEFI_LIB")" \
        2>/dev/null || \
    ld -shared -Bsymbolic -T "$EFI_LDS" \
        "$CRT0" "${OBJS[@]}" -o "$BUILD_DIR/mikeos.so" \
        "$GNUEFI_LIB" "$EFI_LIB"

    echo "  Converting ELF -> PE32+ with objcopy..."
    objcopy \
        -j .text -j .sdata -j .data -j .rodata \
        -j .dynamic -j .dynsym -j .rel -j .rela \
        -j .rel.* -j .rela.* -j .reloc \
        --target efi-app-x86_64 \
        "$BUILD_DIR/mikeos.so" "$BUILD_DIR/BOOTX64.RAW" 2>/dev/null || \
    objcopy \
        -j .text -j .sdata -j .data -j .rodata \
        -j .dynamic -j .dynsym -j .rel -j .rela \
        -j .rel.* -j .rela.* -j .reloc \
        -I elf64-x86-64 -O pei-x86-64 --subsystem 10 \
        "$BUILD_DIR/mikeos.so" "$BUILD_DIR/BOOTX64.RAW"

    echo "  Patching PE relocation table from ELF relocations..."
    python3 fix_pe_reloc.py \
        "$BUILD_DIR/mikeos.so" \
        "$BUILD_DIR/BOOTX64.RAW" \
        "$BUILD_DIR/BOOTX64.EFI"
fi

# ─────────────────────────────────────────────────────
# Final check
# ─────────────────────────────────────────────────────
if [[ -f "$BUILD_DIR/BOOTX64.EFI" ]]; then
    echo ""
    file "$BUILD_DIR/BOOTX64.EFI"
    ls -lh "$BUILD_DIR/BOOTX64.EFI"
    echo ""
    echo "=== Kernel Build Complete! ==="
    echo "  Output: $BUILD_DIR/BOOTX64.EFI"
else
    echo "ERROR: BOOTX64.EFI was not produced!"
    exit 1
fi
