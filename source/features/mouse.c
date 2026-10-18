#include "mouse.h"
#include "screen.h"

MouseState g_Mouse = { 100, 100, FALSE, FALSE, FALSE };
static EFI_SIMPLE_POINTER_PROTOCOL *g_Pointer = NULL;

#define MOUSE_WIDTH 8
#define MOUSE_HEIGHT 12

static const UINT8 mouse_cursor[MOUSE_HEIGHT] = {
    0b10000000,
    0b11000000,
    0b11100000,
    0b11110000,
    0b11111000,
    0b11111100,
    0b11111110,
    0b11110000,
    0b10011000,
    0b00011000,
    0b00001100,
    0b00001100
};

static UINT32 mouse_bg_backup[MOUSE_WIDTH * MOUSE_HEIGHT];
static INT32 last_drawn_x = -1;
static INT32 last_drawn_y = -1;
static BOOLEAN mouse_is_drawn = FALSE;

EFI_STATUS os_mouse_init(EFI_SYSTEM_TABLE *SystemTable) {
    (void)SystemTable;
    EFI_STATUS Status;
    EFI_GUID pointerGuid = EFI_SIMPLE_POINTER_PROTOCOL_GUID;

    Status = LibLocateProtocol(&pointerGuid, (VOID **)&g_Pointer);
    if (!EFI_ERROR(Status) && g_Pointer != NULL) {
        g_Mouse.Present = TRUE;
        g_Mouse.X = g_Screen.Width / 2;
        g_Mouse.Y = g_Screen.Height / 2;
        return EFI_SUCCESS;
    }
    g_Mouse.Present = FALSE;
    return Status;
}

void os_mouse_hide(void) {
    if (!mouse_is_drawn || !g_Screen.Buffer || last_drawn_x < 0 || last_drawn_y < 0) return;

    for (int r = 0; r < MOUSE_HEIGHT; r++) {
        int py = last_drawn_y + r;
        if (py < 0 || (UINT32)py >= g_Screen.Height) continue;
        for (int c = 0; c < MOUSE_WIDTH; c++) {
            int px = last_drawn_x + c;
            if (px < 0 || (UINT32)px >= g_Screen.Width) continue;
            g_Screen.Buffer[py * g_Screen.Pitch + px] = mouse_bg_backup[r * MOUSE_WIDTH + c];
        }
    }
    mouse_is_drawn = FALSE;
}

void os_mouse_draw(void) {
    if (!g_Mouse.Present || !g_Screen.Buffer) return;

    os_mouse_hide();

    last_drawn_x = g_Mouse.X;
    last_drawn_y = g_Mouse.Y;

    // Backup background and draw cursor
    for (int r = 0; r < MOUSE_HEIGHT; r++) {
        int py = last_drawn_y + r;
        if (py < 0 || (UINT32)py >= g_Screen.Height) continue;
        for (int c = 0; c < MOUSE_WIDTH; c++) {
            int px = last_drawn_x + c;
            if (px < 0 || (UINT32)px >= g_Screen.Width) continue;

            mouse_bg_backup[r * MOUSE_WIDTH + c] = g_Screen.Buffer[py * g_Screen.Pitch + px];

            if (mouse_cursor[r] & (0x80 >> c)) {
                g_Screen.Buffer[py * g_Screen.Pitch + px] = RGB_WHITE;
            }
        }
    }
    mouse_is_drawn = TRUE;
}

void os_mouse_update(void) {
    if (!g_Mouse.Present || !g_Pointer) return;

    EFI_SIMPLE_POINTER_STATE state;
    EFI_STATUS Status = uefi_call_wrapper(g_Pointer->GetState, 2, g_Pointer, &state);
    if (!EFI_ERROR(Status)) {
        if (state.RelativeMovementX != 0 || state.RelativeMovementY != 0) {
            os_mouse_hide();
            g_Mouse.X += (INT32)(state.RelativeMovementX / 20);
            g_Mouse.Y += (INT32)(state.RelativeMovementY / 20);

            if (g_Mouse.X < 0) g_Mouse.X = 0;
            if ((UINT32)g_Mouse.X >= g_Screen.Width) g_Mouse.X = g_Screen.Width - 1;
            if (g_Mouse.Y < 0) g_Mouse.Y = 0;
            if ((UINT32)g_Mouse.Y >= g_Screen.Height) g_Mouse.Y = g_Screen.Height - 1;

            os_mouse_draw();
        }
        g_Mouse.LeftButton = state.LeftButton;
        g_Mouse.RightButton = state.RightButton;
    }
}
