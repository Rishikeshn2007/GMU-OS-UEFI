#ifndef _MIKEOS_SCREEN_H
#define _MIKEOS_SCREEN_H

#include "types.h"

typedef struct {
    UINT32 Width;
    UINT32 Height;
    UINT32 Pitch; // in pixels
    UINT32 *Buffer;
    UINT32 Cols;
    UINT32 Rows;
    UINT32 CursorX;
    UINT32 CursorY;
    UINT8 CurrentAttr;
    BOOLEAN CursorVisible;
} ScreenContext;

extern ScreenContext g_Screen;

EFI_STATUS os_screen_init(EFI_SYSTEM_TABLE *SystemTable);
void os_clear_screen(void);
void os_set_color(UINT8 attr);
void os_move_cursor(UINT8 row, UINT8 col);
void os_get_cursor_pos(UINT8 *row, UINT8 *col);
void os_show_cursor(void);
void os_hide_cursor(void);
void os_print_char(char c);
void os_print_string(const char *str);
void os_print_newline(void);
void os_print_space(void);
void os_print_horiz_line(int double_line);
void os_draw_background(const char *title, const char *footer, UINT8 color_attr);
void os_draw_block(UINT8 attr, UINT8 start_row, UINT8 start_col, UINT8 num_rows, UINT8 num_cols);

void os_draw_rect_rgb(UINT32 x, UINT32 y, UINT32 w, UINT32 h, UINT32 rgb);
void os_draw_char_at(UINT32 col, UINT32 row, char c, UINT8 attr);
void os_draw_string_at(UINT32 col, UINT32 row, const char *str, UINT8 attr);

UINT32 attr_to_fg_rgb(UINT8 attr);
UINT32 attr_to_bg_rgb(UINT8 attr);

#endif
