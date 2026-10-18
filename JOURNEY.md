# The Journey: Converting MikeOS from 16-bit BIOS to 64-bit UEFI
### From 640 KB RAM Limits and INT 13h Interrupts to a Native UEFI Kernel in C

---

## 1. Where We Started — The Original MikeOS

MikeOS 4.7 is a real operating system written entirely in **x86 Assembly language**.
It was designed to run on the oldest possible PC hardware — a computer from the 1980s
would boot it just fine. Here is what the original OS looked like under the hood:

| Property              | Original MikeOS 4.7             |
|-----------------------|---------------------------------|
| Language              | 100% x86 Assembly (NASM)        |
| CPU Mode              | 16-bit Real Mode                |
| Max RAM Usable        | 640 KB (hard limit of real mode)|
| Max Addressable RAM   | 1 MB total                      |
| Firmware              | BIOS (Basic Input/Output System)|
| Boot Method           | MBR (Master Boot Record, 512 B) |
| Screen                | VGA Text Mode (80×25 characters)|
| Keyboard              | BIOS Interrupt INT 16h          |
| Disk Access           | BIOS Interrupt INT 13h          |
| Sound                 | Direct I/O port writes (0x61)   |
| File System           | FAT12 (floppy-style)            |
| Multitasking          | None (single-threaded)          |
| CPU Cores Used        | 1 (always core 0)               |

The entire original kernel (`kernel.asm`) is about 11,000 lines of raw assembly,
doing everything through BIOS software interrupts — a system designed in the 1970s.

### What is a BIOS Interrupt?

When the original MikeOS wanted to read a key from the keyboard, it executed:
```asm
mov ah, 0x00        ; function: "wait for key"
int 16h             ; call BIOS keyboard interrupt
; AX now contains the keycode
```

The CPU literally stopped, jumped to a BIOS routine burned into a ROM chip on the
motherboard, waited for a keypress, then returned. Every single hardware operation
worked this way — disk reads, screen writes, speaker beeps, everything.

**The problem**: BIOS interrupts only work in 16-bit real mode. Modern 64-bit Intel
processors boot in 64-bit mode. BIOS interrupts are completely unavailable.

---

## 2. The Problem — Why BIOS Cannot Run on Modern Hardware

Modern Intel processors (from Sandy Bridge onward, and certainly 13th Gen Raptor Lake)
power on and hand control to **UEFI firmware** instead of the old BIOS ROM.

UEFI (Unified Extensible Firmware Interface) is a full mini-operating system in firmware.
It starts the CPU in **64-bit Long Mode** from the very first instruction. This means:

- The CPU is already in 64-bit mode when your bootloader runs
- BIOS interrupt vectors (INT 13h, INT 16h, etc.) **do not exist**
- The 640 KB RAM limit is gone — you can use all installed RAM
- The hardware expects a **PE32+ executable** (Windows-style .EFI file), not a 512-byte MBR

Simply put: **you cannot run a BIOS-based OS on a UEFI-only machine**.

---

## 3. The Plan — What Needed to Change

To bring MikeOS to modern hardware, every single subsystem had to be rewritten:

```
BIOS Interrupt  →  UEFI Protocol
──────────────────────────────────────────────────────────────
INT 10h (VGA)   →  EFI_GRAPHICS_OUTPUT_PROTOCOL (GOP)
INT 16h (KBD)   →  EFI_SIMPLE_INPUT_PROTOCOL
INT 13h (Disk)  →  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL
INT 1Ah (Time)  →  EFI_RUNTIME_SERVICES->GetTime()
INT 15h (Memory)→  EFI_BOOT_SERVICES->AllocatePages()
PC Speaker      →  Direct I/O (unchanged, port 0x61/0x42)
Assembly        →  C language (with gcc + gnu-efi library)
MBR Boot        →  EFI System Partition / BOOTX64.EFI
```

---

## 4. How UEFI Finds the Kernel

This is one of the most important changes. Here is the exact boot sequence:

```
Power On
   │
   ▼
UEFI Firmware initialises CPU, RAM, PCIe devices
   │
   ▼
UEFI looks for an EFI System Partition (ESP) on each drive
   │  (ESP is a FAT32 partition with a special GUID)
   ▼
Inside the ESP, UEFI looks for the default boot path:
   /EFI/BOOT/BOOTX64.EFI
   │
   ▼
UEFI loads BOOTX64.EFI into RAM as a PE32+ executable
   │  (same format as a Windows .exe — 64-bit PE/COFF)
   │
   ▼
UEFI applies base relocations from the .reloc section
   │  (adjusts all absolute pointer values for where it loaded)
   │
   ▼
UEFI calls efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable)
   │
   ▼
Our kernel takes control!
```

The `EFI_SYSTEM_TABLE` pointer is the key — it is a giant table of function pointers
that gives us access to every piece of hardware through UEFI's clean API.

### The BOOTX64.EFI File

Unlike the original MikeOS which had a 512-byte MBR bootloader that loaded the
kernel separately, our entire OS — bootloader + kernel + all drivers — is compiled
into a single **BOOTX64.EFI** file (about 144 KB). UEFI loads it directly.

---

## 5. The Language Change — Assembly to C

### Why We Switched to C

| Reason                | Details                                               |
|-----------------------|-------------------------------------------------------|
| Portability           | C works across architectures; ASM is CPU-specific     |
| Maintainability       | C is 10× easier to read and modify than ASM           |
| UEFI Libraries        | gnu-efi provides C headers for all UEFI protocols     |
| 64-bit Complexity     | 64-bit ASM calling conventions are extremely complex  |
| Debugging             | C compilers produce debug symbols; ASM is opaque      |

The original MikeOS kernel.asm was 11,000 lines of Assembly for a simple OS.
Our equivalent C kernel + all features is about 2,000 lines of readable C code.

### The gnu-efi Library

We use the **gnu-efi** library which provides:
- C type definitions (`EFI_HANDLE`, `UINT64`, `CHAR16`, etc.)
- UEFI protocol structures (`EFI_GRAPHICS_OUTPUT_PROTOCOL`, etc.)
- Helper macros (`uefi_call_wrapper`, `LibLocateProtocol`)
- A startup routine (`crt0-efi-x86_64.o`) that sets up the C runtime before `efi_main`

### How the Build Works

```
source/kernel.c + source/features/*.c
           │
           ▼ gcc (with -fpic -ffreestanding -fno-stack-protector -mno-red-zone)
           │
    build_uefi/*.o  (ELF64 object files)
           │
           ▼ ld (with elf_x86_64_efi.lds linker script)
           │
    build_uefi/mikeos.so  (ELF64 shared object, position-independent)
           │
           ▼ objcopy (ELF → PE32+ conversion)
           │
           ▼ fix_pe_reloc.py  ← THE CRITICAL STEP
           │   (reads 160 R_X86_64_RELATIVE ELF relocations,
           │    writes proper IMAGE_REL_BASED_DIR64 PE .reloc entries)
           │
    build_uefi/BOOTX64.EFI  (valid PE32+ UEFI application)
           │
           ▼ xorriso (creates bootable ISO with El Torito UEFI boot)
           │
    build_uefi/mikeos-uefi.iso  ← Flash to USB, boot on hardware!
```

### The Critical Bug We Had to Fix

When `objcopy` converts an ELF file to PE, it generates base relocation entries
in the `.reloc` section. These tell UEFI "after you load me, go fix these 
absolute pointers to reflect where you actually loaded me in memory."

**The bug**: `objcopy` was only generating 2 dummy `ABSOLUTE` entries, but our
code had **160 `R_X86_64_RELATIVE` relocations** that needed to become
`IMAGE_REL_BASED_DIR64` entries. Without these, the EFI loader could not fix up
any pointers, so the very first function call in our kernel crashed with:

```
Exception: #UD - Invalid Opcode
```

**The fix**: We wrote `fix_pe_reloc.py` — a Python script that:
1. Reads all `R_X86_64_RELATIVE` entries from the ELF using `readelf`
2. Groups them into 4 KB page blocks (PE .reloc format)
3. Encodes each as `IMAGE_REL_BASED_DIR64` (type 0xA)
4. Rewrites the `.reloc` section in the PE file
5. Updates the PE DataDirectory[5] size field

After this fix, all 160 relocations were applied correctly by UEFI, and the OS booted.

---

## 6. Resource Usage — Old vs New

### CPU Usage

| Property            | Old MikeOS         | New MikeOS UEFI          |
|---------------------|--------------------|--------------------------|
| CPU Mode            | 16-bit Real Mode   | 64-bit Long Mode         |
| Cores Used          | 1 (only BSP)       | 1 (only BSP, by design)  |
| Cores Available     | N/A                | All 6 cores (idle)       |
| Clock Speed Used    | Whatever CPU runs  | Minimal (no busy loops)  |
| CPU Extensions Used | None               | SSE, CPUID               |
| Multitasking        | None               | None (cooperative)       |

> The OS uses **1 out of 6 cores** on your laptop. The other 5 cores sit idle
> in their low-power C-state. This is intentional — adding SMP/multicore support
> would require an APIC driver, inter-processor interrupts, and a scheduler,
> which are full operating system features beyond the scope of this port.

### Memory Usage

| Property              | Old MikeOS         | New MikeOS UEFI           |
|-----------------------|--------------------|---------------------------|
| Total RAM Usable      | 640 KB (max!)      | All installed RAM (16 GB) |
| Kernel Code Size      | ~48 KB             | ~144 KB (BOOTX64.EFI)     |
| Stack Size            | ~8 KB (real mode)  | 128 KB (UEFI default)     |
| Font Data             | BIOS ROM (free)    | 4 KB (font8x16 array)     |
| Framebuffer           | VGA text (4 KB)    | ~3 MB (1280×800×32bpp)    |
| Total Kernel Footprint| ~56 KB             | ~3.5 MB (mostly FB)       |

> On your 16 GB laptop, MikeOS uses about **0.02% of your total RAM**.
> The framebuffer (the pixel buffer for the screen) is the biggest consumer.

### Comparison Table

```
                    ┌─────────────────────────────────────────┐
                    │     RESOURCE USAGE COMPARISON           │
                    ├──────────────────┬──────────────────────┤
                    │ Original MikeOS  │  New MikeOS UEFI     │
  ┌─────────────────┼──────────────────┼──────────────────────┤
  │ CPU Cores       │ 1 / 1            │ 1 / 6                │
  │ RAM Used        │ ~48 KB           │ ~3.5 MB              │
  │ RAM Available   │ 640 KB           │ 16 GB                │
  │ Screen Pixels   │ 80×25 chars      │ 1280×800 pixels      │
  │ Colors          │ 16 (text attr)   │ 16M (32-bit RGB)     │
  │ Storage Access  │ FAT12 floppy     │ Any FAT32 partition  │
  │ Mouse Support   │ None             │ Full (EFI pointer)   │
  │ Code Size       │ 11K lines ASM    │ ~2K lines C          │
  └─────────────────┴──────────────────┴──────────────────────┘
```

---

## 7. What Each File Does

### Source Files (`source/`)

| File                     | What It Does                                                  |
|--------------------------|---------------------------------------------------------------|
| `kernel.c`               | Main entry point (`efi_main`). Initialises all subsystems,   |
|                          | plays startup chime, runs the main menu loop.                 |

### Feature Modules (`source/features/`)

| File                     | What It Does                                                  |
|--------------------------|---------------------------------------------------------------|
| `types.h`                | All type definitions, color constants, MAKE_COLOR macro.      |
|                          | Includes `<efi.h>` and `<efilib.h>` from gnu-efi.            |
| `screen.c / screen.h`    | GOP framebuffer driver. Draws pixels, characters, rectangles, |
|                          | scrolls, clears screen, manages cursor position.              |
| `font8x16.c / font8x16.h`| 256-character bitmap font (8×16 pixels per glyph). Used by   |
|                          | screen.c to render every character on screen.                 |
| `keyboard.c / keyboard.h`| EFI keyboard driver. Reads keys via SimpleInput protocol,     |
|                          | maps EFI scan codes to classic MikeOS key codes.             |
| `mouse.c / mouse.h`      | EFI mouse driver. Reads pointer via SimplePointer protocol,   |
|                          | draws/erases hardware cursor, saves background under cursor.  |
| `disk.c / disk.h`        | EFI file system driver. Opens ESP volume, lists files,        |
|                          | reads/writes files using SimpleFileSystem protocol.           |
| `text.c / text.h`        | String utilities: copy, compare, tokenize, length, chomp,     |
|                          | int-to-string, input string from keyboard.                    |
| `sound.c / sound.h`      | PC speaker driver. Programs 8254 PIT timer via I/O ports      |
|                          | 0x42/0x43 to generate tones. Uses port 0x61 to enable.       |
| `misc.c / misc.h`        | System utilities: time/date via EFI GetTime, random number    |
|                          | generator (Xorshift32), CPUID brand string, reboot/shutdown.  |
| `dialog.c / dialog.h`    | GUI dialogs: OK/Cancel box, input dialog, scrollable list     |
|                          | selector, file selector. All mouse + keyboard interactive.    |
| `cli.c / cli.h`          | Command-line interface. Commands: help, dir, ver, cpu, time,  |
|                          | date, cat, echo, beep, reboot, shutdown, exit.                |

### Build Scripts

| File                   | What It Does                                                    |
|------------------------|-----------------------------------------------------------------|
| `build_kernel.sh`      | Main build script. Compiles all C files with gcc, links with   |
|                        | gnu-efi, converts ELF to PE32+ with objcopy, patches .reloc.   |
| `build_iso.py`         | Creates bootable ISO using xorriso with El Torito UEFI boot.   |
| `build_and_test.sh`    | Runs build_kernel.sh + build_iso.py + QEMU test in sequence.   |
| `fix_pe_reloc.py`      | Critical PE patcher: fixes .reloc section with 160 DIR64       |
|                        | base relocation entries extracted from the ELF RELA section.   |
| `ppm2png.py`           | Converts QEMU PPM screenshots to PNG for viewing.              |
| `test_screen.py`       | Boots in headless QEMU, sends Enter key, takes screenshot.     |
| `test_screen_full.py`  | Extended QEMU test: waits 8s, presses keys, takes 2 shots.     |

---

## 8. How the Kernel Works (Simplified)

```
efi_main() called by UEFI firmware
    │
    ├─ InitializeLib()        ← Set up gnu-efi runtime helpers
    │
    ├─ os_screen_init()       ← Locate GOP, get framebuffer address/size
    │                           Set resolution to 1280×800
    │
    ├─ os_keyboard_init()     ← Save pointer to EFI ConIn (keyboard)
    │
    ├─ os_mouse_init()        ← Locate SimplePointer protocol (optional)
    │
    ├─ os_disk_init()         ← Open ESP volume root directory handle
    │
    ├─ os_misc_init()         ← Save SystemTable, seed random number gen
    │
    ├─ Startup chime          ← Three tones via PC speaker (C5, E5, G5)
    │
    ├─ Check for AUTORUN.BIN  ← Run it automatically if present
    │
    └─ Main loop (forever):
           │
           ├─ os_draw_background()  ← Blue desktop + title/footer bars
           │
           ├─ os_dialog_box()       ← "OK = Program Menu, Cancel = CLI"
           │     │
           │     ├─ [OK]     → app_selector()
           │     │               → os_file_selector() → pick file
           │     │               → execute_program() → load + display
           │     │
           │     └─ [Cancel] → os_command_line()
           │                     → accept typed commands
           │                     → dir / ver / cpu / time / cat / beep...
           │
           └─ (repeat forever)
```

---

## 9. The Key Technical Challenges

### Challenge 1: PE Relocation Table (Hardest Bug)
**Problem**: UEFI loads EFI files at random addresses for security (ASLR).
All absolute 64-bit pointers in our code must be "fixed up" after loading.
The PE `.reloc` section tells UEFI which addresses to patch.

**What went wrong**: `objcopy` produced only 2 dummy entries instead of 160.
**Solution**: `fix_pe_reloc.py` — manually rebuilt the section from ELF data.

### Challenge 2: Calling Convention
**Problem**: UEFI uses the **Microsoft x64 calling convention** (RCX, RDX, R8, R9
for the first 4 args, 32-byte shadow space on stack). Linux gcc uses SystemV
(RDI, RSI, RDX, RCX). Calling UEFI functions with the wrong convention = crash.

**Solution**: `uefi_call_wrapper()` macro from gnu-efi, plus `-mno-red-zone`
and `__attribute__((ms_abi))` on the entry point.

### Challenge 3: No Standard Library
**Problem**: UEFI runs before any OS, so there is no `libc`, no `printf`,
no `malloc`, no `memset`. Everything must be implemented from scratch.

**Solution**: All string operations live in `text.c`. Memory is stack-allocated
(static arrays). Screen output goes directly through `os_print_char()`.

### Challenge 4: Font Rendering
**Problem**: In BIOS text mode, the CPU automatically rendered characters using
ROM fonts. In framebuffer mode, you must draw each pixel manually.

**Solution**: `font8x16.c` — a 256×16-byte array where each byte is a bitmask
of 8 pixels for one row of one character. `os_draw_char_at()` reads these bits
and writes pixel colours into the framebuffer.

---

## 10. Conclusion

Starting from an OS that needed a 1980s BIOS chip and could only see 640 KB of RAM,
we produced a native 64-bit UEFI operating system that:

- Boots on any modern Intel machine (including 13th Gen Core i3)
- Uses the full 64-bit instruction set
- Accesses the complete screen as a high-resolution pixel framebuffer
- Reads and writes files on modern FAT32 storage
- Supports mouse and keyboard through UEFI firmware protocols
- Fits in a 34 MB bootable ISO that can be flashed to any USB drive

The conversion required rewriting every hardware driver from scratch in C,
solving a deep PE binary format bug, and understanding both the BIOS world
the original OS lived in and the UEFI world modern processors expect.

Total new code written: approximately **2,000 lines of C** across 11 modules.
