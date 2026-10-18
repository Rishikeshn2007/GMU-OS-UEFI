#ifndef _MIKEOS_KEYBOARD_H
#define _MIKEOS_KEYBOARD_H

#include "types.h"

void os_keyboard_init(EFI_SYSTEM_TABLE *SystemTable);
UINT8 os_wait_for_key(void);
UINT8 os_check_for_key(void);

#endif
