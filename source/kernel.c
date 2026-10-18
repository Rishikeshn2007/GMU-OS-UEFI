/* ==================================================================
 * MikeOS 64-bit -- The Mike Operating System for 64-bit UEFI
 * Ported & modernized for x86_64 and Intel Core i3 13th Gen+ processors
 * Maintains identical scheduling, policy, and user interface flow
 * ================================================================== */

#include "features/types.h"
#include "features/screen.h"
#include "features/keyboard.h"
#include "features/mouse.h"
#include "features/text.h"
#include "features/disk.h"
#include "features/sound.h"
#include "features/misc.h"
#include "features/dialog.h"
#include "features/cli.h"
#include "../programs/apps.h"

static const char os_init_msg[] = "Welcome to MikeOS 64-bit";
static const char os_version_msg[] = "Version " MIKEOS_VER " (UEFI x86_64)";
static const char dialog_string_1[] = "Thanks for trying out MikeOS 64-bit!";
static const char dialog_string_2[] = "Please select an interface: OK for the";
static const char dialog_string_3[] = "program menu, Cancel for command line.";

static const char file_finished_msg[] = ">>> Press a key to return to the program menu...";

static int is_text_file(const char *filename) {
    int len = os_string_length(filename);

    return (len >= 4 && os_string_strincmp(filename + len - 4, ".TXT", 4)) ||
           (len >= 4 && os_string_strincmp(filename + len - 4, ".BAS", 4)) ||
           (len >= 3 && os_string_strincmp(filename + len - 2, ".C", 2)) ||
           (len >= 3 && os_string_strincmp(filename + len - 2, ".H", 2)) ||
           (len >= 4 && os_string_strincmp(filename + len - 3, ".MD", 3));
}

static void open_storage_file(const char *filename) {
    os_clear_screen();
    os_set_color(WHITE_ON_BLACK);

    os_print_string("File: ");
    os_print_string(filename);
    os_print_string("\r\n\r\n");

    if (!is_text_file(filename)) {
        os_print_string("This file is not a supported text format. External applications ");
        os_print_string("cannot be run by the UEFI kernel; use a built-in app instead.\r\n");
    } else {
        UINTN file_size = 0;
        static char app_buffer[65536];

        if (!os_get_file_size(filename, &file_size)) {
            os_print_string("Error: Could not read file information.\r\n");
        } else if (file_size >= sizeof(app_buffer)) {
            os_print_string("This text file is too large to preview.\r\n");
        } else if (!os_load_file(filename, app_buffer, &file_size)) {
            os_print_string("Error: Could not read file from storage.\r\n");
        } else {
            app_buffer[file_size] = '\0';
            os_print_string(app_buffer);
        }
    }

    os_print_newline();
    os_print_string(file_finished_msg);
    os_wait_for_key();
}

static void app_selector(void) {
    static const char app_list[] =
        "Calculator,Hangman,Storage Browser,Back";
    char selected_item[64];
    char selected_file[128];

    while (1) {
        os_draw_background(os_init_msg, os_version_msg, WHITE_ON_BLUE);

        int choice = os_list_dialog(app_list, "Built-in applications",
                                    "Select an app with arrows and Enter", selected_item);
        if (choice < 0 || choice == 3) {
            return;
        }

        if (choice == 0) {
            app_calculator();
            continue;
        }
        if (choice == 1) {
            app_hangman();
            continue;
        }

        if (os_file_selector(selected_file)) open_storage_file(selected_file);
    }
}

EFI_STATUS
efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable) {
    InitializeLib(ImageHandle, SystemTable);

    // Initialize all hardware and subsystems
    os_screen_init(SystemTable);
    os_keyboard_init(SystemTable);
    os_mouse_init(SystemTable);
    os_disk_init(ImageHandle, SystemTable);
    os_misc_init(SystemTable);

    // Initial chime
    os_speaker_tone(523); // C5
    os_pause(1);
    os_speaker_tone(659); // E5
    os_pause(1);
    os_speaker_tone(784); // G5
    os_pause(1);
    os_speaker_off();

    // Main system loop (matching original MikeOS option_screen)
    while (1) {
        os_draw_background(os_init_msg, os_version_msg, WHITE_ON_BLUE);

        int choice = os_dialog_box(dialog_string_1, dialog_string_2, dialog_string_3, 1);

        if (choice == 0) {
            // OK -> App Selector
            app_selector();
        } else {
            // Cancel -> Command Line
            os_command_line(ImageHandle, SystemTable);
        }
    }

    return EFI_SUCCESS;
}
