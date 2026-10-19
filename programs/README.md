# GMU-OS Applications

Each C source file in `programs/` (except headers) builds as a separate x86-64
`ET_DYN` application image with a `.bin` extension. `build_iso.py` copies the
apps to the root of the boot volume, where `dir` lists them. Run an app by
selecting its `.bin` file in Storage Browser or entering its filename at the
command line.

## Application ABI

Each C app exports this entry point from `gmu_api.h`:

```c
int app_main(const GMU_APP_API *api);
```

The OS passes the versioned API table to the app. It currently provides
screen output, keyboard input, dialogs, and random numbers. Applications must
be freestanding: no libc, dynamic dependencies, or BIOS system calls.
The loader accepts x86-64 ELF shared images with `R_X86_64_RELATIVE`
relocations and invokes the entry point using the System V x86-64 ABI. This is
the GMU-OS `.bin` format; it is not compatible with the old 16-bit MikeOS
`.bin` files or Windows `.exe` files.

To add an app, include `programs/gmu_api.h` and implement `app_main` in
`programs/your_app.c`. Run `bash build_and_test.sh`; it automatically compiles
every `.c` app in `programs/` to `.bin` and packages the results in the ISO.

The `.bas` files are original MikeOS programs retained as source references;
this UEFI edition does not include a BASIC interpreter.
