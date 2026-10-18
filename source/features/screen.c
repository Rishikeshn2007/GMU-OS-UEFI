#include "screen.h"
#include "font8x16.h"

ScreenContext g_Screen;
static EFI_GRAPHICS_OUTPUT_PROTOCOL *g_Gop = NULL;

static const UINT32 vga_palette[16] = {
    RGB_BLACK,
    RGB_BLUE,
    RGB_GREEN,
    RGB_CYAN,
    RGB_RED,
    RGB_MAGENTA,
    RGB_BROWN,
    RGB_LIGHT_GRAY,
    RGB_DARK_GRAY,
    RGB_LIGHT_BLUE,
    RGB_LIGHT_GREEN,
    RGB_LIGHT_CYAN,
    RGB_LIGHT_RED,
    RGB_LIGHT_MAGENTA,
    RGB_YELLOW,
    RGB_WHITE
};

UINT32 attr_to_fg_rgb(UINT8 attr) {
    return vga_palette[attr & 0x0F];
}

UINT32 attr_to_bg_rgb(UINT8 attr) {
    return vga_palette[(attr >> 4) & 0x0F];
}

EFI_STATUS os_screen_init(EFI_SYSTEM_TABLE *SystemTable) {
    EFI_STATUS Status;
    EFI_GUID gopGuid = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;

    Status = LibLocateProtocol(&gopGuid, (VOID **)&g_Gop);
    if (EFI_ERROR(Status) || g_Gop == NULL) {
        return Status;
    }

    g_Screen.Width = g_Gop->Mode->Info->HorizontalResolution;
    g_Screen.Height = g_Gop->Mode->Info->VerticalResolution;
    g_Screen.Pitch = g_Gop->Mode->Info->PixelsPerScanLine;
    g_Screen.Buffer = (UINT32 *)g_Gop->Mode->FrameBufferBase;

    g_Screen.Cols = g_Screen.Width / 8;
    g_Screen.Rows = g_Screen.Height / 16;
    g_Screen.CursorX = 0;
    g_Screen.CursorY = 0;
    g_Screen.CurrentAttr = WHITE_ON_BLACK;
    g_Screen.CursorVisible = TRUE;

    os_clear_screen();
    return EFI_SUCCESS;
}

void os_draw_rect_rgb(UINT32 x, UINT32 y, UINT32 w, UINT32 h, UINT32 rgb) {
    if (!g_Screen.Buffer) return;
    if (x >= g_Screen.Width || y >= g_Screen.Height) return;
    if (x + w > g_Screen.Width) w = g_Screen.Width - x;
    if (y + h > g_Screen.Height) h = g_Screen.Height - y;

    for (UINT32 row = 0; row < h; row++) {
        UINT32 *dst = g_Screen.Buffer + (y + row) * g_Screen.Pitch + x;
        for (UINT32 col = 0; col < w; col++) {
            dst[col] = rgb;
        }
    }
}

void os_clear_screen(void) {
    if (!g_Screen.Buffer) return;
    UINT32 bg = attr_to_bg_rgb(g_Screen.CurrentAttr);
    os_draw_rect_rgb(0, 0, g_Screen.Width, g_Screen.Height, bg);
    g_Screen.CursorX = 0;
    g_Screen.CursorY = 0;
}

void os_set_color(UINT8 attr) {
    g_Screen.CurrentAttr = attr;
}

void os_move_cursor(UINT8 row, UINT8 col) {
    if (col < g_Screen.Cols) g_Screen.CursorX = col;
    if (row < g_Screen.Rows) g_Screen.CursorY = row;
}

void os_get_cursor_pos(UINT8 *row, UINT8 *col) {
    if (row) *row = (UINT8)g_Screen.CursorY;
    if (col) *col = (UINT8)g_Screen.CursorX;
}

void os_show_cursor(void) {
    g_Screen.CursorVisible = TRUE;
}

void os_hide_cursor(void) {
    g_Screen.CursorVisible = FALSE;
}

void os_draw_char_at(UINT32 col, UINT32 row, char c, UINT8 attr) {
    if (col >= g_Screen.Cols || row >= g_Screen.Rows) return;
    if (!g_Screen.Buffer) return;

    UINT32 px = col * 8;
    UINT32 py = row * 16;
    UINT32 fg = vga_palette[attr & 0x0F];
    UINT32 bg = vga_palette[(attr >> 4) & 0x0F];
    unsigned char uc = (unsigned char)c;
    UINT32 pitch = g_Screen.Pitch;
    UINT32 *row_ptr = g_Screen.Buffer + (py * pitch) + px;

    for (int y = 0; y < 16; y++) {
        UINT8 line = font8x16[uc][y];
        for (int x = 0; x < 8; x++) {
            row_ptr[x] = (line & (0x80 >> x)) ? fg : bg;
        }
        row_ptr += pitch;
    }
}

void os_draw_string_at(UINT32 col, UINT32 row, const char *str, UINT8 attr) {
    if (!str) return;
    while (*str && col < g_Screen.Cols) {
        os_draw_char_at(col, row, *str, attr);
        col++;
        str++;
    }
}

static void screen_scroll(void) {
    if (!g_Screen.Buffer) return;
    // Scroll up by 16 scanlines
    UINT32 lines_to_move = (g_Screen.Rows - 1) * 16;
    UINT32 *dst = g_Screen.Buffer;
    UINT32 *src = g_Screen.Buffer + 16 * g_Screen.Pitch;

    for (UINT32 i = 0; i < lines_to_move * g_Screen.Pitch; i++) {
        dst[i] = src[i];
    }

    // Clear bottom row
    UINT32 bg = attr_to_bg_rgb(g_Screen.CurrentAttr);
    os_draw_rect_rgb(0, (g_Screen.Rows - 1) * 16, g_Screen.Width, 16, bg);
}

void os_print_char(char c) {
    if (c == '\r') {
        g_Screen.CursorX = 0;
        return;
    }
    if (c == '\n') {
        g_Screen.CursorX = 0;
        g_Screen.CursorY++;
        if (g_Screen.CursorY >= g_Screen.Rows) {
            screen_scroll();
            g_Screen.CursorY = g_Screen.Rows - 1;
        }
        return;
    }
    if (c == '\b') {
        if (g_Screen.CursorX > 0) {
            g_Screen.CursorX--;
            os_draw_char_at(g_Screen.CursorX, g_Screen.CursorY, ' ', g_Screen.CurrentAttr);
        }
        return;
    }
    if (c == '\t') {
        UINT32 next = (g_Screen.CursorX + 8) & ~7;
        while (g_Screen.CursorX < next && g_Screen.CursorX < g_Screen.Cols) {
            os_print_char(' ');
        }
        return;
    }

    os_draw_char_at(g_Screen.CursorX, g_Screen.CursorY, c, g_Screen.CurrentAttr);
    g_Screen.CursorX++;
    if (g_Screen.CursorX >= g_Screen.Cols) {
        g_Screen.CursorX = 0;
        g_Screen.CursorY++;
        if (g_Screen.CursorY >= g_Screen.Rows) {
            screen_scroll();
            g_Screen.CursorY = g_Screen.Rows - 1;
        }
    }
}

void os_print_string(const char *str) {
    if (!str) return;
    while (*str) {
        os_print_char(*str++);
    }
}

void os_print_newline(void) {
    os_print_char('\r');
    os_print_char('\n');
}

void os_print_space(void) {
    os_print_char(' ');
}

void os_print_horiz_line(int double_line) {
    char ch = (char)(double_line ? 0xCD : 0xC4);
    for (UINT32 c = 0; c < g_Screen.Cols; c++) {
        os_print_char(ch);
    }
}

void os_draw_block(UINT8 attr, UINT8 start_row, UINT8 start_col, UINT8 num_rows, UINT8 num_cols) {
    for (UINT8 r = 0; r < num_rows; r++) {
        for (UINT8 c = 0; c < num_cols; c++) {
            os_draw_char_at(start_col + c, start_row + r, ' ', attr);
        }
    }
}

void os_draw_background(const char *title, const char *footer, UINT8 color_attr) {
    if (!g_Screen.Buffer) return;

    // Fill background
    UINT32 bg_color = attr_to_bg_rgb(color_attr);
    os_draw_rect_rgb(0, 0, g_Screen.Width, g_Screen.Height, bg_color);

    // Top Bar (Dark Blue, row 0)
    UINT8 top_attr = MAKE_COLOR(COLOR_WHITE, COLOR_BLUE);
    os_draw_block(top_attr, 0, 0, 1, (UINT8)g_Screen.Cols);
    if (title) {
        os_draw_string_at(2, 0, title, top_attr);
    }

    // Bottom Bar (Dark Blue, last row)
    if (g_Screen.Rows > 0) {
        UINT8 btm_row = (UINT8)(g_Screen.Rows - 1);
        os_draw_block(top_attr, btm_row, 0, 1, (UINT8)g_Screen.Cols);
        if (footer) {
            os_draw_string_at(2, btm_row, footer, top_attr);
        }
    }

    // Reset cursor to row 1, col 0
    os_move_cursor(1, 0);
    os_set_color(color_attr);
}
