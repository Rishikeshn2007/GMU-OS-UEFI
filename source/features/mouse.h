#ifndef _MIKEOS_MOUSE_H
#define _MIKEOS_MOUSE_H

#include "types.h"

typedef struct {
    INT32 X;
    INT32 Y;
    BOOLEAN LeftButton;
    BOOLEAN RightButton;
    BOOLEAN Present;
} MouseState;

extern MouseState g_Mouse;

EFI_STATUS os_mouse_init(EFI_SYSTEM_TABLE *SystemTable);
void os_mouse_update(void);
void os_mouse_draw(void);
void os_mouse_hide(void);

#endif
