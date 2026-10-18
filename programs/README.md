# Applications

The UEFI applications below are separate x86-64 EFI executables. The build
places them at the root of the boot volume, where `dir` lists them. Select an
`.EFI` file in Storage Browser or enter its filename at the command line to
run it:

- `CALCULATOR.EFI` — built from `calculator.c`; signed integer arithmetic.
- `HANGMAN.EFI` — built from `hangman.c`; keyboard-driven word game.
- `efi_main.c` — initializes screen, keyboard, mouse, and random-number
  services for each standalone application.
- `apps.h` — application entry-point declarations.

The `.bas` files are original MikeOS BASIC programs retained as source
references. The current UEFI build does not include a BASIC interpreter, so
these files are not executable in the UEFI edition. The old 16-bit `.bin`
programs are not compatible with this 64-bit UEFI executable format.

To add an application, implement it in C using the functions in
`source/features/`, then add its source and EFI entry point to the application
build section in `build_kernel.sh` and copy its output into the boot volume
in `build_iso.py`.
