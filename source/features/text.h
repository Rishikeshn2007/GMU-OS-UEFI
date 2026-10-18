#ifndef _MIKEOS_TEXT_H
#define _MIKEOS_TEXT_H

#include "types.h"

int os_string_length(const char *str);
void os_string_copy(const char *src, char *dest);
int os_string_compare(const char *s1, const char *s2);
int os_string_strincmp(const char *s1, const char *s2, int n);
void os_string_uppercase(char *str);
void os_string_lowercase(char *str);
void os_string_chomp(char *str);
void os_string_strip(char *str, char c);
void os_string_truncate(char *str, int n);
void os_string_join(const char *s1, const char *s2, char *dest);
void os_string_reverse(char *str);
void os_int_to_string(unsigned int val, char *dest);
void os_sint_to_string(int val, char *dest);
int os_string_to_int(const char *str);
void os_long_int_to_string(uint64_t val, int base, char *dest);
int os_find_char_in_string(const char *str, char c);
char *os_string_tokenize(char *str, char delim);
void os_input_string(char *buffer, int max_chars);

#endif
