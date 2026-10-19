#include "gmu_api.h"

#include <stdint.h>
#include <stddef.h>

static int read_integer(const GMU_APP_API *api, const char *prompt, int64_t *value) {
    char input[32];
    int i = 0;
    int negative = 0;
    uint64_t magnitude = 0;

    if (api->input_dialog(input, sizeof(input), prompt) != 0) return 0;
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

static void print_result(const GMU_APP_API *api, int64_t value) {
    char digits[32];
    uint64_t magnitude;

    if (value < 0) {
        api->print_string("-");
        magnitude = (uint64_t)(-(value + 1)) + 1;
    } else {
        magnitude = (uint64_t)value;
    }
    {
        static const char table[] = "0123456789";
        int pos = 0;
        do {
            digits[pos++] = table[magnitude % 10];
            magnitude /= 10;
        } while (magnitude != 0);
        digits[pos] = '\0';
        for (int left = 0, right = pos - 1; left < right; left++, right--) {
            char temp = digits[left];
            digits[left] = digits[right];
            digits[right] = temp;
        }
    }
    api->print_string(digits);
}

static void show_message(const GMU_APP_API *api, const char *message) {
    api->clear_screen();
    api->set_color(GMU_COLOR_WHITE_ON_BLACK);
    api->print_string("Calculator\r\n\r\n");
    api->print_string(message);
    api->print_string("\r\n\r\nPress any key to continue.");
    api->wait_for_key();
}

int GMU_APP_ABI app_main(const GMU_APP_API *api) {
    static const char operations[] =
        "Add,Subtract,Multiply,Integer divide,Back";
    char selected[64];

    if (!api || api->abi_version != GMU_APP_ABI_VERSION ||
        api->struct_size < sizeof(GMU_APP_API)) {
        return 1;
    }

    while (1) {
        int operation;
        int read_status;
        int64_t left;
        int64_t right;
        int64_t result;

        api->clear_screen();
        api->set_color(GMU_COLOR_WHITE_ON_BLACK);
        api->print_string("GMU-OS Integer Calculator\r\n");
        api->print_string("Enter whole numbers from -1000000000 to 1000000000.\r\n\r\n");

        operation = api->list_dialog(operations, "Select an operation",
                                     "Esc returns to the program menu", selected);
        if (operation < 0 || operation == 4) return 0;

        read_status = read_integer(api, "Enter the first whole number:", &left);
        if (read_status == 0) continue;
        if (read_status < 0) {
            show_message(api, "Invalid number. Use digits and an optional leading minus sign.");
            continue;
        }

        read_status = read_integer(api, "Enter the second whole number:", &right);
        if (read_status == 0) continue;
        if (read_status < 0) {
            show_message(api, "Invalid number. Use digits and an optional leading minus sign.");
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
                show_message(api, "Cannot divide by zero.");
                continue;
            }
            result = left / right;
        }

        api->clear_screen();
        api->set_color(GMU_COLOR_WHITE_ON_BLACK);
        api->print_string("Result: ");
        print_result(api, result);
        if (operation == 3) {
            api->print_string("\r\nRemainder: ");
            print_result(api, left % right);
        }
        api->print_string("\r\n\r\nPress any key to continue.");
        api->wait_for_key();
    }
}
