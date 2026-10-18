#ifndef _MIKEOS_TYPES_H
#define _MIKEOS_TYPES_H

#include <efi.h>
#include <efilib.h>
#include <stdint.h>
#include <stdbool.h>

#define MIKEOS_VER "4.7.0"
#define MIKEOS_API_VER 18
#define MIKEOS_ARCH "x86_64 UEFI (Intel 64-bit)"

// Standard MikeOS Keys
#define KEY_UP        72
#define KEY_DOWN      80
#define KEY_LEFT      75
#define KEY_RIGHT     77
#define KEY_ESC       27
#define KEY_ENTER     13
#define KEY_BACKSPACE 8
#define KEY_TAB       9

// Classic MikeOS Color Attributes (VGA 4-bit foreground, 4-bit background)
#define COLOR_BLACK         0x0
#define COLOR_BLUE          0x1
#define COLOR_GREEN         0x2
#define COLOR_CYAN          0x3
#define COLOR_RED           0x4
#define COLOR_MAGENTA       0x5
#define COLOR_BROWN         0x6
#define COLOR_LIGHT_GRAY    0x7
#define COLOR_DARK_GRAY     0x8
#define COLOR_LIGHT_BLUE    0x9
#define COLOR_LIGHT_GREEN   0xA
#define COLOR_LIGHT_CYAN    0xB
#define COLOR_LIGHT_RED     0xC
#define COLOR_LIGHT_MAGENTA 0xD
#define COLOR_YELLOW        0xE
#define COLOR_WHITE         0xF

#define MAKE_COLOR(fg, bg) (((bg) << 4) | ((fg) & 0x0F))

#define BLACK_ON_WHITE        MAKE_COLOR(COLOR_BLACK, COLOR_WHITE)
#define WHITE_ON_BLACK        MAKE_COLOR(COLOR_WHITE, COLOR_BLACK)
#define WHITE_ON_LIGHT_RED    MAKE_COLOR(COLOR_WHITE, COLOR_LIGHT_RED)
#define WHITE_ON_GREEN        MAKE_COLOR(COLOR_WHITE, COLOR_GREEN)
#define RED_ON_LIGHT_GREEN    MAKE_COLOR(COLOR_RED, COLOR_LIGHT_GREEN)
#define WHITE_ON_BLUE         MAKE_COLOR(COLOR_WHITE, COLOR_BLUE)
#define WHITE_ON_CYAN         MAKE_COLOR(COLOR_WHITE, COLOR_CYAN)

// 32-bit RGB Colors (0x00RRGGBB)
#define RGB_BLACK         0x00000000
#define RGB_BLUE          0x000000AA
#define RGB_GREEN         0x0000AA00
#define RGB_CYAN          0x0000AAAA
#define RGB_RED           0x00AA0000
#define RGB_MAGENTA       0x00AA00AA
#define RGB_BROWN         0x00AA5500
#define RGB_LIGHT_GRAY    0x00AAAAAA
#define RGB_DARK_GRAY     0x00555555
#define RGB_LIGHT_BLUE    0x005555FF
#define RGB_LIGHT_GREEN   0x0055FF55
#define RGB_LIGHT_CYAN    0x0055FFFF
#define RGB_LIGHT_RED     0x00FF5555
#define RGB_LIGHT_MAGENTA 0x00FF55FF
#define RGB_YELLOW        0x00FFFF55
#define RGB_WHITE         0x00FFFFFF

// Classic MikeOS Theme Colors
#define MIKEOS_DESKTOP_BG 0x00336699
#define MIKEOS_BAR_BG     0x001A334D
#define MIKEOS_BAR_FG     0x00FFFFFF
#define MIKEOS_DIALOG_BG  0x00D6D6D6
#define MIKEOS_DIALOG_FG  0x00000000
#define MIKEOS_DIALOG_SEL 0x00224488
#define MIKEOS_DIALOG_SFG 0x00FFFFFF

#endif
