# Applications

The UEFI kernel is a single statically linked EFI application; it does not
load or execute the original 16-bit `.bin` applications. The built-in
applications below are C modules linked into `build_uefi/BOOTX64.EFI` and
launched from the program menu:

- `calculator.c` — signed integer arithmetic with overflow-bounded input.
- `hangman.c` — keyboard-driven word game using the kernel's random-number,
  screen, and keyboard functions.
- `apps.h` — declarations for the built-in applications.

The `.bas` files are original MikeOS BASIC programs retained as source
references. The current UEFI build does not include a BASIC interpreter, so
these files are not executable in the UEFI edition.

To add a built-in application, implement it in C using the functions in
`source/features/`, add its source to `SOURCES` in `build_kernel.sh`, and add
an entry in the program menu in `source/kernel.c`.
