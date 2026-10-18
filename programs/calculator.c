#include "apps.h"
#include "../source/features/dialog.h"
#include "../source/features/keyboard.h"
#include "../source/features/screen.h"
#include "../source/features/text.h"

#include <stdint.h>

static int read_integer(const char *prompt, int64_t *value) {
    char input[32];
    int i = 0;
    int negative = 0;
    uint64_t magnitude = 0;

    if (os_input_dialog(input, sizeof(input), prompt) != 0) return 0;
    if (input[0] == '-') {
        negative = 1;
        i++;
    }
    if (input[i] == '\0') return -1;

    for (; input[i] != '\0'; i++) {
        uint64_t digit;
        if (input[i] < '0' || input[i] > '9') return -1;
        digit = (uint64_t)(input[i] - '0');
        if (magnitude > (1000000000ULL - digit) / 10) return -1;
        magnitude = magnitude * 10 + digit;
    }

    *value = negative ? -(int64_t)magnitude : (int64_t)magnitude;
    return 1;
}

static void print_result(int64_t value) {
    char digits[32];
    uint64_t magnitude;

    if (value < 0) {
        os_print_string("-");
        magnitude = (uint64_t)(-(value + 1)) + 1;
    } else {
        magnitude = (uint64_t)value;
    }
    os_long_int_to_string(magnitude, 10, digits);
    os_print_string(digits);
}

static void show_message(const char *message) {
    os_clear_screen();
    os_set_color(WHITE_ON_BLACK);
    os_print_string("Calculator\r\n\r\n");
    os_print_string(message);
    os_print_string("\r\n\r\nPress any key to continue.");
    os_wait_for_key();
}

void app_calculator(void) {
    static const char operations[] =
        "Add,Subtract,Multiply,Integer divide,Back";
    char selected[64];

    while (1) {
        int operation;
        int read_status;
        int64_t left;
        int64_t right;
        int64_t result;

        os_clear_screen();
        os_set_color(WHITE_ON_BLACK);
        os_print_string("MikeOS Integer Calculator\r\n");
        os_print_string("Enter whole numbers from -1000000000 to 1000000000.\r\n\r\n");

        operation = os_list_dialog(operations, "Select an operation",
                                   "Esc returns to the program menu", selected);
        if (operation < 0 || operation == 4) return;

        read_status = read_integer("Enter the first whole number:", &left);
        if (read_status == 0) continue;
        if (read_status < 0) {
            show_message("Invalid number. Use digits and an optional leading minus sign.");
            continue;
        }

        read_status = read_integer("Enter the second whole number:", &right);
        if (read_status == 0) continue;
        if (read_status < 0) {
            show_message("Invalid number. Use digits and an optional leading minus sign.");
            continue;
        }

        if (operation == 0) {
            result = left + right;
        } else if (operation == 1) {
            result = left - right;
        } else if (operation == 2) {
            result = left * right;
        } else {
            if (right == 0) {
                show_message("Cannot divide by zero.");
                continue;
            }
            result = left / right;
        }

        os_clear_screen();
        os_set_color(WHITE_ON_BLACK);
        os_print_string("Result: ");
        print_result(result);
        if (operation == 3) {
            os_print_string("\r\nRemainder: ");
            print_result(left % right);
        }
        os_print_string("\r\n\r\nPress any key to continue.");
        os_wait_for_key();
    }
}
