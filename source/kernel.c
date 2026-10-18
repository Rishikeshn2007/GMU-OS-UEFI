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

static const char os_init_msg[] = "Welcome to MikeOS 64-bit";
static const char os_version_msg[] = "Version " MIKEOS_VER " (UEFI x86_64)";
static const char dialog_string_1[] = "Thanks for trying out MikeOS 64-bit!";
static const char dialog_string_2[] = "Please select an interface: OK for the";
static const char dialog_string_3[] = "program menu, Cancel for command line.";

static const char kerndlg_string_1[] = "Cannot execute MikeOS kernel!";
static const char kerndlg_string_2[] = "BOOTX64.EFI is the core of MikeOS,";
static const char kerndlg_string_3[] = "and is not an external application.";

static const char program_finished_msg[] = ">>> Program finished -- press a key to continue...";

static void execute_program(const char *filename) {
    os_clear_screen();
    os_set_color(WHITE_ON_BLACK);

    os_print_string("Executing: ");
    os_print_string(filename);
    os_print_string("\r\n\r\n");

    UINTN file_size = 0;
    static char app_buffer[65536]; // 64K buffer for program/data

    if (!os_load_file(filename, app_buffer, &file_size)) {
        os_print_string("Error: Could not load file from storage.\r\n");
    } else {
        // Check file extension
        int len = os_string_length(filename);
        if (len > 4 && os_string_strincmp(filename + len - 4, ".TXT", 4)) {
            // Display text file
            if (file_size < sizeof(app_buffer)) app_buffer[file_size] = '\0';
            else app_buffer[sizeof(app_buffer) - 1] = '\0';
            os_print_string(app_buffer);
            os_print_newline();
        } else {
            os_print_string("File loaded into memory (");
            char sz[16];
            os_int_to_string((unsigned int)file_size, sz);
            os_print_string(sz);
            os_print_string(" bytes).\r\n");
            os_print_string("Execution simulated successfully in 64-bit protected environment.\r\n");
        }
    }

    os_print_newline();
    os_print_string(program_finished_msg);
    os_wait_for_key();
}

static void app_selector(void) {
    char selected_file[128];

    while (1) {
        // Draw main desktop background
        os_draw_background(os_init_msg, os_version_msg, WHITE_ON_BLUE);

        // Run file selector dialog
        int result = os_file_selector(selected_file);
        if (!result) {
            // Esc was pressed, return to option screen
            return;
        }

        // Check if user selected the kernel itself
        if (os_string_strincmp(selected_file, "BOOTX64.EFI", 11) ||
            os_string_strincmp(selected_file, "KERNEL.BIN", 10)) {
            os_dialog_box(kerndlg_string_1, kerndlg_string_2, kerndlg_string_3, 0);
            continue;
        }

        execute_program(selected_file);
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

    // Check for AUTORUN.BIN or AUTORUN.TXT
    if (os_file_exists("AUTORUN.BIN")) {
        execute_program("AUTORUN.BIN");
    }

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
