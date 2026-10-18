# MikeOS 64-bit UEFI

This repository contains MikeOS 4.7 and a modern UEFI-based port of its
desktop, file tools, and command-line interface. The UEFI version is written
in C and boots as a 64-bit EFI application on x86-64 systems. It uses UEFI
firmware protocols for graphics, keyboard, mouse, storage, and time rather
than relying on the legacy BIOS interrupts used by the original 16-bit MikeOS.

For the project background, design decisions, boot flow, and technical
challenges behind the port, see [JOURNEY.md](JOURNEY.md). The original MikeOS
documentation and license are in [doc/](doc/), including the user and
developer handbooks.

## Repository contents

| Path | Description |
|---|---|
| `source/kernel.c` | UEFI application entry point and main OS startup/menu loop. |
| `source/features/` | Kernel feature modules: framebuffer graphics and font rendering, keyboard and mouse input, file access, dialogs, CLI, sound, text utilities, and system information. |
| `source/kernel.asm` | Original MikeOS assembly kernel, retained alongside the UEFI port. |
| `source/bootload/` | Bootloader sources and related files for the original MikeOS project. |
| `programs/` | Sample MikeOS applications and programs, primarily for the original system. |
| `disk_images/` | Original MikeOS floppy, ISO, and disk-image releases. These are distinct from the UEFI ISO built by this project. |
| `doc/` | Original MikeOS user/developer handbooks, change history, credits, and license. |
| `build_kernel.sh` | Compiles and links the 64-bit UEFI application as `build_uefi/BOOTX64.EFI`. |
| `build_iso.py` | Packages the EFI application into a bootable UEFI ISO at `build_uefi/mikeos-uefi.iso`. |
| `build_and_test.sh` | Builds the kernel and ISO, then runs the available QEMU screen test. |
| `fix_pe_reloc.py` | Repairs the PE base-relocation data in the GCC/gnu-efi build path. |
| `JOURNEY.md` | Detailed account of the conversion from BIOS-based 16-bit MikeOS to the 64-bit UEFI port. |

## Build the UEFI ISO in WSL

These instructions use Ubuntu on WSL. On Windows 11, WSLg can display QEMU
windows directly. Install WSL/Ubuntu and start an Ubuntu terminal, then install
the build and test tools:

```bash
sudo apt update
sudo apt install -y build-essential clang lld gnu-efi mtools xorriso \
  python3 qemu-system-x86 ovmf
```

Change to the repository directory. For this checkout, the Windows path can be
accessed from WSL as follows:

```bash
cd /mnt/c/Users/"Rishikesh N"/Desktop/GMU-OS-UEFI
```

Build the UEFI application and ISO:

```bash
bash build_kernel.sh
python3 build_iso.py
```

On success, the EFI application is `build_uefi/BOOTX64.EFI` and the bootable
ISO is `build_uefi/mikeos-uefi.iso`. The combined build-and-test script is also
available:

```bash
bash build_and_test.sh
```

It builds both files and attempts a QEMU test when QEMU and OVMF are installed.
To build without that test, run `bash build_and_test.sh --no-test`.

## Boot in QEMU from WSL

With the ISO built, start QEMU using OVMF UEFI firmware:

```bash
qemu-system-x86_64 \
  -bios /usr/share/ovmf/OVMF.fd \
  -drive id=cd0,file=build_uefi/mikeos-uefi.iso,if=none,format=raw \
  -device ide-cd,drive=cd0,bootindex=0 \
  -m 256M \
  -vga std \
  -net none
```

QEMU should open a graphical window when WSLg is available. The desktop menu
and dialogs can be controlled with the keyboard; use the arrow keys to move,
`Enter` to select, and `Esc` to cancel or go back. The command-line interface
includes commands such as `help`, `dir`, `ver`, `cpu`, `time`, `date`, `beep`,
and `shutdown`.

If the firmware file is not found at the path above, check the firmware files
installed by Ubuntu's `ovmf` package (their location and filename can vary):

```bash
find /usr/share -iname 'OVMF*.fd'
```

Use the appropriate OVMF firmware file in QEMU's `-bios` option. If no QEMU
window appears, confirm that WSLg is available and that the `DISPLAY`
environment variable is set; WSLg configures display support automatically on
supported Windows installations.

## Write the ISO to a USB drive with Rufus

The ISO is intended to boot on x86-64 UEFI systems. To write it from Windows:

1. Build or copy `build_uefi/mikeos-uefi.iso` to a location accessible in
   Windows. From WSL, the repository is also available at
   `C:\Users\Rishikesh N\Desktop\mikeos-4.7.0`.
2. Insert the USB drive and open **Rufus**. Verify the selected device is the
   USB drive you intend to use.
3. Choose the ISO using **Boot selection** and click **START**.
4. If Rufus asks whether to write in ISO Image mode or DD Image mode, choose
   **DD Image mode** to write the hybrid UEFI image as built.
5. Confirm the write operation and wait for Rufus to finish.

**Writing the image erases the contents of the selected USB drive.** Back up
any files you need and double-check the selected device before confirming.

## Boot from the USB drive

1. Leave the USB drive connected and restart or power on the computer.
2. Open the one-time boot menu during startup. The key varies by manufacturer;
   common keys include `F12`, `F11`, `F9`, and `Esc`. The startup screen or
   computer manual identifies the correct key.
3. Select the USB device's **UEFI** boot entry. If a firmware boot menu offers
   both UEFI and legacy entries, choose UEFI.
4. If the firmware blocks the unsigned EFI application, temporarily disable
   Secure Boot in firmware settings, then try the UEFI USB entry again.

Changing firmware settings is system-specific; restore any security settings
you changed after testing. If the USB device is missing from the boot menu,
check that Rufus completed successfully and that the computer supports x86-64
UEFI boot.

## License

MikeOS is distributed under the license included in
[`doc/LICENSE.TXT`](doc/LICENSE.TXT). See the original MikeOS documentation
and [JOURNEY.md](JOURNEY.md) for additional project context.
