#ifndef _MIKEOS_DIALOG_H
#define _MIKEOS_DIALOG_H

#include "types.h"

int os_dialog_box(const char *str1, const char *str2, const char *str3, int type);
int os_input_dialog(char *buffer, int max_chars, const char *prompt);
int os_list_dialog(const char *list, const char *help1, const char *help2, char *selected_item);
int os_file_selector(char *selected_filename);

#endif
