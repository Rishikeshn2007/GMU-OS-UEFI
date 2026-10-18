#include "dialog.h"
#include "screen.h"
#include "keyboard.h"
#include "mouse.h"
#include "text.h"
#include "disk.h"

// Helper to draw a shadowed dialog window
static void draw_window(int start_col, int start_row, int width, int height, UINT8 body_attr) {
    UINT8 shadow_attr = MAKE_COLOR(COLOR_DARK_GRAY, COLOR_BLACK);

    // Drop shadow (bottom and right)
    for (int r = 1; r <= height; r++) {
        os_draw_char_at(start_col + width, start_row + r, ' ', shadow_attr);
        os_draw_char_at(start_col + width + 1, start_row + r, ' ', shadow_attr);
    }
    for (int c = 1; c <= width + 1; c++) {
        os_draw_char_at(start_col + c, start_row + height, ' ', shadow_attr);
    }

    // Main box fill
    for (int r = 0; r < height; r++) {
        for (int c = 0; c < width; c++) {
            os_draw_char_at(start_col + c, start_row + r, ' ', body_attr);
        }
    }

    // Border
    for (int c = 0; c < width; c++) {
        os_draw_char_at(start_col + c, start_row, (char)0xC4, body_attr);
        os_draw_char_at(start_col + c, start_row + height - 1, (char)0xC4, body_attr);
    }
    for (int r = 0; r < height; r++) {
        os_draw_char_at(start_col, start_row + r, (char)0xB3, body_attr);
        os_draw_char_at(start_col + width - 1, start_row + r, (char)0xB3, body_attr);
    }
    os_draw_char_at(start_col, start_row, (char)0xDA, body_attr);
    os_draw_char_at(start_col + width - 1, start_row, (char)0xBF, body_attr);
    os_draw_char_at(start_col, start_row + height - 1, (char)0xC0, body_attr);
    os_draw_char_at(start_col + width - 1, start_row + height - 1, (char)0xD9, body_attr);
}

int os_dialog_box(const char *str1, const char *str2, const char *str3, int type) {
    // type: 0 = OK, 1 = OK / Cancel
    int width = 54;
    int height = 11;
    int start_col = (g_Screen.Cols - width) / 2;
    int start_row = (g_Screen.Rows - height) / 2;

    UINT8 body_attr = MAKE_COLOR(COLOR_BLACK, COLOR_LIGHT_GRAY);
    UINT8 btn_normal = MAKE_COLOR(COLOR_WHITE, COLOR_BLUE);
    UINT8 btn_sel = MAKE_COLOR(COLOR_BLACK, COLOR_WHITE);

    draw_window(start_col, start_row, width, height, body_attr);

    if (str1) os_draw_string_at(start_col + (width - os_string_length(str1)) / 2, start_row + 2, str1, body_attr);
    if (str2) os_draw_string_at(start_col + (width - os_string_length(str2)) / 2, start_row + 3, str2, body_attr);
    if (str3) os_draw_string_at(start_col + (width - os_string_length(str3)) / 2, start_row + 4, str3, body_attr);

    int selected = 0; // 0 = OK, 1 = Cancel

    while (1) {
        if (type == 0) {
            // Only OK button
            int btn_col = start_col + (width - 10) / 2;
            os_draw_string_at(btn_col, start_row + 7, "[   OK   ]", btn_sel);
        } else {
            // OK and Cancel
            int ok_col = start_col + 14;
            int cancel_col = start_col + 30;
            os_draw_string_at(ok_col, start_row + 7, "[   OK   ]", (selected == 0) ? btn_sel : btn_normal);
            os_draw_string_at(cancel_col, start_row + 7, "[ Cancel ]", (selected == 1) ? btn_sel : btn_normal);
        }

        os_mouse_update();
        os_mouse_draw();

        // Check mouse click on buttons
        if (g_Mouse.Present && g_Mouse.LeftButton) {
            int mouse_col = g_Mouse.X / 8;
            int mouse_row = g_Mouse.Y / 16;
            if (mouse_row == start_row + 7) {
                if (type == 0) {
                    int btn_col = start_col + (width - 10) / 2;
                    if (mouse_col >= btn_col && mouse_col < btn_col + 10) {
                        os_mouse_hide();
                        return 0;
                    }
                } else {
                    int ok_col = start_col + 14;
                    int cancel_col = start_col + 30;
                    if (mouse_col >= ok_col && mouse_col < ok_col + 10) {
                        os_mouse_hide();
                        return 0;
                    }
                    if (mouse_col >= cancel_col && mouse_col < cancel_col + 10) {
                        os_mouse_hide();
                        return 1;
                    }
                }
            }
        }

        UINT8 key = os_check_for_key();
        if (key != 0) {
            if (key == KEY_LEFT || key == KEY_RIGHT || key == KEY_TAB) {
                if (type == 1) selected = 1 - selected;
            } else if (key == KEY_ENTER) {
                os_mouse_hide();
                return selected;
            } else if (key == KEY_ESC) {
                os_mouse_hide();
                return (type == 1) ? 1 : 0;
            }
        }
    }
}

int os_input_dialog(char *buffer, int max_chars, const char *prompt) {
    int width = 50;
    int height = 8;
    int start_col = (g_Screen.Cols - width) / 2;
    int start_row = (g_Screen.Rows - height) / 2;

    UINT8 body_attr = MAKE_COLOR(COLOR_BLACK, COLOR_LIGHT_GRAY);
    UINT8 input_attr = MAKE_COLOR(COLOR_WHITE, COLOR_BLACK);

    draw_window(start_col, start_row, width, height, body_attr);

    if (prompt) {
        os_draw_string_at(start_col + 3, start_row + 2, prompt, body_attr);
    }

    // Input field area
    int input_col = start_col + 3;
    int input_row = start_row + 4;
    int input_w = width - 6;

    for (int c = 0; c < input_w; c++) {
        os_draw_char_at(input_col + c, input_row, ' ', input_attr);
    }

    int pos = 0;
    buffer[0] = '\0';

    while (1) {
        os_draw_char_at(input_col + pos, input_row, '_', input_attr);
        UINT8 key = os_wait_for_key();
        os_draw_char_at(input_col + pos, input_row, ' ', input_attr);

        if (key == KEY_ENTER) {
            buffer[pos] = '\0';
            return 0; // Success
        } else if (key == KEY_ESC) {
            buffer[0] = '\0';
            return 1; // Canceled
        } else if (key == KEY_BACKSPACE) {
            if (pos > 0) {
                pos--;
                os_draw_char_at(input_col + pos, input_row, ' ', input_attr);
            }
        } else if (key >= 32 && key <= 126) {
            if (pos < max_chars - 1 && pos < input_w - 1) {
                buffer[pos] = (char)key;
                os_draw_char_at(input_col + pos, input_row, (char)key, input_attr);
                pos++;
            }
        }
    }
}

int os_list_dialog(const char *list, const char *help1, const char *help2, char *selected_item) {
    if (!list) return -1;

    // Parse comma-separated list
    char items[64][64];
    int count = 0;
    int char_idx = 0;

    for (int i = 0; list[i] != '\0' && count < 64; i++) {
        if (list[i] == ',') {
            items[count][char_idx] = '\0';
            count++;
            char_idx = 0;
        } else {
            if (char_idx < 63) {
                items[count][char_idx++] = list[i];
            }
        }
    }
    if (char_idx > 0 && count < 64) {
        items[count][char_idx] = '\0';
        count++;
    }

    if (count == 0) return -1;

    int width = 50;
    int height = 18;
    int start_col = (g_Screen.Cols - width) / 2;
    int start_row = (g_Screen.Rows - height) / 2;

    UINT8 body_attr = MAKE_COLOR(COLOR_BLACK, COLOR_LIGHT_GRAY);
    UINT8 sel_attr = MAKE_COLOR(COLOR_WHITE, COLOR_BLUE);

    draw_window(start_col, start_row, width, height, body_attr);

    if (help1) os_draw_string_at(start_col + 3, start_row + 1, help1, body_attr);
    if (help2) os_draw_string_at(start_col + 3, start_row + 2, help2, body_attr);

    int visible_rows = 10;
    int current_sel = 0;
    int scroll_offset = 0;

    while (1) {
        for (int i = 0; i < visible_rows; i++) {
            int idx = scroll_offset + i;
            int row = start_row + 4 + i;
            UINT8 attr = (idx == current_sel) ? sel_attr : body_attr;

            // Clear row
            for (int c = 0; c < width - 6; c++) {
                os_draw_char_at(start_col + 3 + c, row, ' ', attr);
            }

            if (idx < count) {
                os_draw_string_at(start_col + 4, row, items[idx], attr);
            }
        }

        os_mouse_update();
        os_mouse_draw();

        // Check mouse selection
        if (g_Mouse.Present && g_Mouse.LeftButton) {
            int mouse_col = g_Mouse.X / 8;
            int mouse_row = g_Mouse.Y / 16;
            if (mouse_col >= start_col + 3 && mouse_col < start_col + width - 3) {
                int clicked_i = mouse_row - (start_row + 4);
                if (clicked_i >= 0 && clicked_i < visible_rows && (scroll_offset + clicked_i) < count) {
                    current_sel = scroll_offset + clicked_i;
                    if (selected_item) {
                        os_string_copy(items[current_sel], selected_item);
                    }
                    os_mouse_hide();
                    return current_sel;
                }
            }
        }

        UINT8 key = os_check_for_key();
        if (key != 0) {
            if (key == KEY_UP) {
                if (current_sel > 0) {
                    current_sel--;
                    if (current_sel < scroll_offset) scroll_offset--;
                }
            } else if (key == KEY_DOWN) {
                if (current_sel < count - 1) {
                    current_sel++;
                    if (current_sel >= scroll_offset + visible_rows) scroll_offset++;
                }
            } else if (key == KEY_ENTER) {
                if (selected_item) {
                    os_string_copy(items[current_sel], selected_item);
                }
                os_mouse_hide();
                return current_sel;
            } else if (key == KEY_ESC) {
                os_mouse_hide();
                return -1;
            }
        }
    }
}

int os_file_selector(char *selected_filename) {
    char file_list[2048];
    int count = os_get_file_list(file_list, sizeof(file_list));
    if (count == 0) {
        os_dialog_box("No files found on storage drive!", "", "", 0);
        return 0;
    }

    int sel = os_list_dialog(file_list, "Select a file to run or open:", "Use arrow keys and Enter, or mouse", selected_filename);
    return (sel >= 0) ? 1 : 0;
}
