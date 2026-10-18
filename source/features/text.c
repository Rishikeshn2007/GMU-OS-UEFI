#include "text.h"
#include "keyboard.h"
#include "screen.h"

int os_string_length(const char *str) {
    int len = 0;
    if (!str) return 0;
    while (str[len] != '\0') {
        len++;
    }
    return len;
}

void os_string_copy(const char *src, char *dest) {
    if (!src || !dest) return;
    while (*src) {
        *dest++ = *src++;
    }
    *dest = '\0';
}

int os_string_compare(const char *s1, const char *s2) {
    if (!s1 || !s2) return 0;
    while (*s1 && *s2) {
        if (*s1 != *s2) return 0;
        s1++;
        s2++;
    }
    return (*s1 == *s2);
}

int os_string_strincmp(const char *s1, const char *s2, int n) {
    if (!s1 || !s2 || n <= 0) return 0;
    for (int i = 0; i < n; i++) {
        char c1 = s1[i];
        char c2 = s2[i];
        if (c1 >= 'a' && c1 <= 'z') c1 -= 32;
        if (c2 >= 'a' && c2 <= 'z') c2 -= 32;
        if (c1 != c2) return 0;
        if (s1[i] == '\0') break;
    }
    return 1;
}

void os_string_uppercase(char *str) {
    if (!str) return;
    while (*str) {
        if (*str >= 'a' && *str <= 'z') *str -= 32;
        str++;
    }
}

void os_string_lowercase(char *str) {
    if (!str) return;
    while (*str) {
        if (*str >= 'A' && *str <= 'Z') *str += 32;
        str++;
    }
}

void os_string_chomp(char *str) {
    if (!str) return;
    // Strip leading spaces
    char *start = str;
    while (*start == ' ') start++;

    int len = os_string_length(start);
    for (int i = 0; i < len; i++) {
        str[i] = start[i];
    }
    str[len] = '\0';

    // Strip trailing spaces
    for (int i = len - 1; i >= 0 && str[i] == ' '; i--) {
        str[i] = '\0';
    }
}

void os_string_strip(char *str, char c) {
    if (!str) return;
    char *read = str;
    char *write = str;
    while (*read) {
        if (*read != c) {
            *write++ = *read;
        }
        read++;
    }
    *write = '\0';
}

void os_string_truncate(char *str, int n) {
    if (!str || n < 0) return;
    if (os_string_length(str) > n) {
        str[n] = '\0';
    }
}

void os_string_join(const char *s1, const char *s2, char *dest) {
    if (!dest) return;
    int pos = 0;
    if (s1) {
        for (int i = 0; s1[i] != '\0'; i++) dest[pos++] = s1[i];
    }
    if (s2) {
        for (int i = 0; s2[i] != '\0'; i++) dest[pos++] = s2[i];
    }
    dest[pos] = '\0';
}

void os_string_reverse(char *str) {
    if (!str) return;
    int len = os_string_length(str);
    for (int i = 0, j = len - 1; i < j; i++, j--) {
        char temp = str[i];
        str[i] = str[j];
        str[j] = temp;
    }
}

void os_int_to_string(unsigned int val, char *dest) {
    if (!dest) return;
    if (val == 0) {
        dest[0] = '0';
        dest[1] = '\0';
        return;
    }
    char buf[16];
    int idx = 0;
    while (val > 0) {
        buf[idx++] = (char)('0' + (val % 10));
        val /= 10;
    }
    for (int i = 0; i < idx; i++) {
        dest[i] = buf[idx - 1 - i];
    }
    dest[idx] = '\0';
}

void os_sint_to_string(int val, char *dest) {
    if (!dest) return;
    if (val < 0) {
        *dest++ = '-';
        val = -val;
    }
    os_int_to_string((unsigned int)val, dest);
}

int os_string_to_int(const char *str) {
    if (!str) return 0;
    int res = 0;
    int sign = 1;
    if (*str == '-') {
        sign = -1;
        str++;
    }
    while (*str >= '0' && *str <= '9') {
        res = res * 10 + (*str - '0');
        str++;
    }
    return res * sign;
}

void os_long_int_to_string(uint64_t val, int base, char *dest) {
    if (!dest || base < 2 || base > 16) return;
    if (val == 0) {
        dest[0] = '0';
        dest[1] = '\0';
        return;
    }
    static const char digits[] = "0123456789ABCDEF";
    char buf[65];
    int idx = 0;
    while (val > 0) {
        buf[idx++] = digits[val % base];
        val /= base;
    }
    for (int i = 0; i < idx; i++) {
        dest[i] = buf[idx - 1 - i];
    }
    dest[idx] = '\0';
}

int os_find_char_in_string(const char *str, char c) {
    if (!str) return 0;
    for (int i = 0; str[i] != '\0'; i++) {
        if (str[i] == c) return i + 1;
    }
    return 0;
}

static char *token_saved_ptr = NULL;

char *os_string_tokenize(char *str, char delim) {
    if (str != NULL) {
        token_saved_ptr = str;
    }
    if (token_saved_ptr == NULL || *token_saved_ptr == '\0') {
        return NULL;
    }

    // Skip leading delimiters
    while (*token_saved_ptr == delim) {
        token_saved_ptr++;
    }
    if (*token_saved_ptr == '\0') return NULL;

    char *start = token_saved_ptr;
    while (*token_saved_ptr != '\0' && *token_saved_ptr != delim) {
        token_saved_ptr++;
    }

    if (*token_saved_ptr != '\0') {
        *token_saved_ptr = '\0';
        token_saved_ptr++;
    }
    return start;
}

void os_input_string(char *buffer, int max_chars) {
    if (!buffer || max_chars <= 0) return;

    int pos = 0;
    while (1) {
        UINT8 key = os_wait_for_key();
        if (key == KEY_ENTER) {
            os_print_newline();
            buffer[pos] = '\0';
            return;
        } else if (key == KEY_BACKSPACE) {
            if (pos > 0) {
                pos--;
                os_print_char('\b');
            }
        } else if (key >= 32 && key <= 126) {
            if (pos < max_chars - 1) {
                buffer[pos++] = (char)key;
                os_print_char((char)key);
            }
        }
    }
}
