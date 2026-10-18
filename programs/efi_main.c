#include "../source/features/types.h"
#include "../source/features/screen.h"
#include "../source/features/keyboard.h"
#include "../source/features/mouse.h"
#include "../source/features/misc.h"
#include "apps.h"

EFI_STATUS
efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable) {
    InitializeLib(ImageHandle, SystemTable);
    os_screen_init(SystemTable);
    os_keyboard_init(SystemTable);
    os_mouse_init(SystemTable);
    os_misc_init(SystemTable);

#if defined(MIKEOS_APP_CALCULATOR)
    app_calculator();
#elif defined(MIKEOS_APP_HANGMAN)
    app_hangman();
#else
#error "Select a built-in application for this EFI image."
#endif

    return EFI_SUCCESS;
}
