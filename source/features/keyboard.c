#include "keyboard.h"

static EFI_SYSTEM_TABLE *g_ST = NULL;

void os_keyboard_init(EFI_SYSTEM_TABLE *SystemTable) {
    g_ST = SystemTable;
}

static UINT8 map_efi_key(EFI_INPUT_KEY *key) {
    if (key->ScanCode != 0) {
        switch (key->ScanCode) {
            case SCAN_UP:    return KEY_UP;
            case SCAN_DOWN:  return KEY_DOWN;
            case SCAN_LEFT:  return KEY_LEFT;
            case SCAN_RIGHT: return KEY_RIGHT;
            case SCAN_ESC:   return KEY_ESC;
            default: break;
        }
    }

    if (key->UnicodeChar == CHAR_CARRIAGE_RETURN || key->UnicodeChar == '\n') {
        return KEY_ENTER;
    }
    if (key->UnicodeChar == CHAR_BACKSPACE) {
        return KEY_BACKSPACE;
    }
    if (key->UnicodeChar == CHAR_TAB) {
        return KEY_TAB;
    }
    if (key->UnicodeChar >= 32 && key->UnicodeChar <= 126) {
        return (UINT8)key->UnicodeChar;
    }

    return 0;
}

UINT8 os_wait_for_key(void) {
    if (!g_ST || !g_ST->ConIn) return 0;

    EFI_INPUT_KEY key;
    UINTN index;
    EFI_STATUS status;

    while (1) {
        uefi_call_wrapper(g_ST->BootServices->WaitForEvent, 3, 1, &g_ST->ConIn->WaitForKey, &index);
        status = uefi_call_wrapper(g_ST->ConIn->ReadKeyStroke, 2, g_ST->ConIn, &key);
        if (!EFI_ERROR(status)) {
            UINT8 code = map_efi_key(&key);
            if (code != 0) return code;
        }
    }
}

UINT8 os_check_for_key(void) {
    if (!g_ST || !g_ST->ConIn) return 0;

    EFI_INPUT_KEY key;
    EFI_STATUS status = uefi_call_wrapper(g_ST->ConIn->ReadKeyStroke, 2, g_ST->ConIn, &key);
    if (!EFI_ERROR(status)) {
        return map_efi_key(&key);
    }
    return 0;
}
