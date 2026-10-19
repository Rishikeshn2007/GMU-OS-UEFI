#include "cli.h"
#include "screen.h"
#include "keyboard.h"
#include "text.h"
#include "disk.h"
#include "misc.h"
#include "sound.h"
#include "executable.h"

void os_command_line(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable) {
    char cmd_buffer[256];
    char time_str[32];
    char date_str[32];
    char cpu_str[64];

    os_set_color(WHITE_ON_BLACK);
    os_clear_screen();

    os_print_string("GMU-OS ");
    os_print_string(MIKEOS_VER);
    os_print_string(" (64-bit UEFI Edition)\r\n");
    os_print_string("Type 'help' for a list of commands or 'exit' to return to menu.\r\n\r\n");

    while (1) {
        os_set_color(MAKE_COLOR(COLOR_LIGHT_CYAN, COLOR_BLACK));
        os_print_string("GMU-OS> ");
        os_set_color(WHITE_ON_BLACK);

        os_input_string(cmd_buffer, sizeof(cmd_buffer));
        os_string_chomp(cmd_buffer);

        if (os_string_length(cmd_buffer) == 0) {
            continue;
        }

        if (os_string_compare(cmd_buffer, "exit")) {
            break;
        } else if (os_string_compare(cmd_buffer, "help") || os_string_compare(cmd_buffer, "?")) {
            os_print_string("Available commands:\r\n");
            os_print_string("  help     - Show this help message\r\n");
            os_print_string("  cls      - Clear the screen\r\n");
            os_print_string("  dir      - List files on storage\r\n");
            os_print_string("  ver      - Display OS version & system details\r\n");
            os_print_string("  cpu      - Show processor information\r\n");
            os_print_string("  <name>.BIN - Run a GMU-OS application from storage\r\n");
            os_print_string("  time     - Show current time\r\n");
            os_print_string("  date     - Show current date\r\n");
            os_print_string("  cat <f>  - Display contents of text file\r\n");
            os_print_string("  echo <t> - Display text\r\n");
            os_print_string("  beep     - Test PC speaker\r\n");
            os_print_string("  reboot   - Reboot computer\r\n");
            os_print_string("  shutdown - Power off computer\r\n");
            os_print_string("  exit     - Exit CLI and return to main menu\r\n");
        } else if (os_string_compare(cmd_buffer, "cls") || os_string_compare(cmd_buffer, "clear")) {
            os_clear_screen();
        } else if (os_string_compare(cmd_buffer, "ver")) {
            os_print_string("MikeOS Version ");
            os_print_string(MIKEOS_VER);
            os_print_string("\r\nTarget: ");
            os_print_string(MIKEOS_ARCH);
            os_print_string("\r\n64-bit UEFI Native Kernel - Core i3 13th Gen Ready\r\n");
        } else if (os_string_compare(cmd_buffer, "cpu")) {
            os_get_cpu_info(cpu_str);
            os_print_string("CPU: ");
            os_print_string(cpu_str);
            os_print_newline();
        } else if (os_string_compare(cmd_buffer, "time")) {
            os_get_time_string(time_str);
            os_print_string("Current Time: ");
            os_print_string(time_str);
            os_print_newline();
        } else if (os_string_compare(cmd_buffer, "date")) {
            os_get_date_string(date_str);
            os_print_string("Current Date: ");
            os_print_string(date_str);
            os_print_newline();
        } else if (os_string_compare(cmd_buffer, "dir") || os_string_compare(cmd_buffer, "ls")) {
            char flist[2048];
            int count = os_get_file_list(flist, sizeof(flist));
            if (count == 0) {
                os_print_string("No files found.\r\n");
            } else {
                char *tok = os_string_tokenize(flist, ',');
                while (tok) {
                    UINTN fsz = 0;
                    os_get_file_size(tok, &fsz);
                    char sz_str[16];
                    os_int_to_string((unsigned int)fsz, sz_str);

                    os_print_string(tok);
                    for (int s = os_string_length(tok); s < 16; s++) os_print_space();
                    os_print_string(sz_str);
                    os_print_string(" bytes\r\n");

                    tok = os_string_tokenize(NULL, ',');
                }
            }
        } else if (os_string_strincmp(cmd_buffer, "cat ", 4) || os_string_strincmp(cmd_buffer, "type ", 5)) {
            char *fname = (cmd_buffer[0] == 'c') ? (cmd_buffer + 4) : (cmd_buffer + 5);
            os_string_chomp(fname);
            if (!os_file_exists(fname)) {
                os_print_string("File not found: ");
                os_print_string(fname);
                os_print_newline();
            } else {
                UINTN fsize = 0;
                char text_buf[4096];
                if (os_load_file(fname, text_buf, &fsize)) {
                    if (fsize < sizeof(text_buf)) {
                        text_buf[fsize] = '\0';
                        os_print_string(text_buf);
                        os_print_newline();
                    } else {
                        text_buf[sizeof(text_buf) - 1] = '\0';
                        os_print_string(text_buf);
                        os_print_string("\r\n[File truncated for preview]\r\n");
                    }
                } else {
                    os_print_string("Failed to read file.\r\n");
                }
            }
        } else if (os_string_strincmp(cmd_buffer, "echo ", 5)) {
            os_print_string(cmd_buffer + 5);
            os_print_newline();
        } else if (os_string_compare(cmd_buffer, "beep")) {
            os_speaker_tone(440);
            os_pause(3);
            os_speaker_off();
            os_print_string("Beeped.\r\n");
        } else if (os_string_compare(cmd_buffer, "reboot")) {
            os_print_string("Rebooting system...\r\n");
            os_pause(5);
            os_reboot();
        } else if (os_string_compare(cmd_buffer, "shutdown") || os_string_compare(cmd_buffer, "poweroff")) {
            os_print_string("Shutting down...\r\n");
            os_pause(5);
            os_shutdown();
        } else {
            int cmd_len = os_string_length(cmd_buffer);
            if (cmd_len >= 4 &&
                os_string_strincmp(cmd_buffer + cmd_len - 4, ".BIN", 4)) {
                EFI_STATUS status =
                    os_execute_bin_file(cmd_buffer);
                if (EFI_ERROR(status)) {
                    char status_hex[17];
                    os_long_int_to_string((uint64_t)status, 16, status_hex);
                    os_print_string("Could not run application. Loader status: 0x");
                    os_print_string(status_hex);
                    os_print_newline();
                }
            } else if (os_file_exists(cmd_buffer)) {
                os_print_string("Found file '");
                os_print_string(cmd_buffer);
                os_print_string("'. Use cat or type to view text files.\r\n");
                UINTN fsz = 0;
                char preview_buf[512];
                if (os_load_file(cmd_buffer, preview_buf, &fsz)) {
                    os_print_string("File size: ");
                    char sz_str[24];
                    os_long_int_to_string((uint64_t)fsz, 10, sz_str);
                    os_print_string(sz_str);
                    os_print_string(" bytes.\r\n");
                } else {
                    os_print_string("Could not read file contents.\r\n");
                }
            } else {
                os_print_string("Unknown command: '");
                os_print_string(cmd_buffer);
                os_print_string("'. Type 'help' for command list.\r\n");
            }
        }
    }
}
