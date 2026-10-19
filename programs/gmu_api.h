#ifndef GMU_OS_APP_API_H
#define GMU_OS_APP_API_H

#include <stdint.h>

#define GMU_APP_ABI_VERSION 1
#define GMU_KEY_ESC 27
#define GMU_KEY_ENTER 13
#define GMU_COLOR_WHITE_ON_BLACK 0x0F

#if defined(__x86_64__) && (defined(__GNUC__) || defined(__clang__))
#define GMU_APP_ABI __attribute__((sysv_abi))
#else
#define GMU_APP_ABI
#endif

typedef struct {
    uint32_t abi_version;
    uint32_t struct_size;
    void (GMU_APP_ABI *clear_screen)(void);
    void (GMU_APP_ABI *set_color)(uint8_t attr);
    void (GMU_APP_ABI *print_string)(const char *text);
    void (GMU_APP_ABI *print_char)(char value);
    void (GMU_APP_ABI *print_space)(void);
    uint8_t (GMU_APP_ABI *wait_for_key)(void);
    int (GMU_APP_ABI *input_dialog)(char *buffer, int max_chars, const char *prompt);
    int (GMU_APP_ABI *list_dialog)(
        const char *items,
        const char *help1,
        const char *help2,
        char *selected_item);
    unsigned int (GMU_APP_ABI *random)(unsigned int low, unsigned int high);
} GMU_APP_API;

int GMU_APP_ABI app_main(const GMU_APP_API *api);

#endif
