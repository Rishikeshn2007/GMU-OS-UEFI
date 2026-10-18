#include "misc.h"
#include "screen.h"
#include "keyboard.h"
#include "text.h"

static EFI_SYSTEM_TABLE *g_MiscST = NULL;
static uint32_t rng_state = 123456789;
static UINT8 g_time_fmt = 0; // 0 = 24-hr, 1 = 12-hr
static UINT8 g_date_fmt = 0; // 0 = M/D/Y, 1 = D/M/Y, 2 = Y/M/D

void os_misc_init(EFI_SYSTEM_TABLE *SystemTable) {
    g_MiscST = SystemTable;
    os_seed_random();
}

void os_set_time_fmt(UINT8 fmt) {
    g_time_fmt = fmt;
}

void os_set_date_fmt(UINT16 fmt) {
    g_date_fmt = (UINT8)(fmt & 0xFF);
}

void os_get_time_string(char *buffer) {
    if (!buffer || !g_MiscST) return;

    EFI_TIME time;
    EFI_STATUS st = uefi_call_wrapper(g_MiscST->RuntimeServices->GetTime, 2, &time, NULL);
    if (EFI_ERROR(st)) {
        os_string_copy("00:00:00", buffer);
        return;
    }

    UINT8 hour = time.Hour;
    UINT8 min = time.Minute;
    UINT8 sec = time.Second;

    buffer[0] = (char)('0' + (hour / 10));
    buffer[1] = (char)('0' + (hour % 10));
    buffer[2] = ':';
    buffer[3] = (char)('0' + (min / 10));
    buffer[4] = (char)('0' + (min % 10));
    buffer[5] = ':';
    buffer[6] = (char)('0' + (sec / 10));
    buffer[7] = (char)('0' + (sec % 10));
    buffer[8] = '\0';
}

void os_get_date_string(char *buffer) {
    if (!buffer || !g_MiscST) return;

    EFI_TIME time;
    EFI_STATUS st = uefi_call_wrapper(g_MiscST->RuntimeServices->GetTime, 2, &time, NULL);
    if (EFI_ERROR(st)) {
        os_string_copy("01/01/2026", buffer);
        return;
    }

    UINT16 year = time.Year;
    UINT8 month = time.Month;
    UINT8 day = time.Day;

    if (g_date_fmt == 1) { // D/M/Y
        buffer[0] = (char)('0' + (day / 10));
        buffer[1] = (char)('0' + (day % 10));
        buffer[2] = '/';
        buffer[3] = (char)('0' + (month / 10));
        buffer[4] = (char)('0' + (month % 10));
        buffer[5] = '/';
    } else { // M/D/Y default
        buffer[0] = (char)('0' + (month / 10));
        buffer[1] = (char)('0' + (month % 10));
        buffer[2] = '/';
        buffer[3] = (char)('0' + (day / 10));
        buffer[4] = (char)('0' + (day % 10));
        buffer[5] = '/';
    }

    buffer[6] = (char)('0' + ((year / 1000) % 10));
    buffer[7] = (char)('0' + ((year / 100) % 10));
    buffer[8] = (char)('0' + ((year / 10) % 10));
    buffer[9] = (char)('0' + (year % 10));
    buffer[10] = '\0';
}

void os_pause(int tenths_of_sec) {
    if (g_MiscST && g_MiscST->BootServices) {
        // Stall in microseconds: tenths * 100000 us
        uefi_call_wrapper(g_MiscST->BootServices->Stall, 1, (UINTN)tenths_of_sec * 100000);
    } else {
        for (volatile long i = 0; i < (long)tenths_of_sec * 5000000; i++);
    }
}

void os_seed_random(void) {
    if (g_MiscST && g_MiscST->RuntimeServices) {
        EFI_TIME time;
        if (!EFI_ERROR(uefi_call_wrapper(g_MiscST->RuntimeServices->GetTime, 2, &time, NULL))) {
            rng_state = (time.Second * 60 + time.Minute) * 3600 + time.Hour + time.Nanosecond;
            return;
        }
    }
    rng_state = 987654321;
}

unsigned int os_get_random(unsigned int low, unsigned int high) {
    if (low >= high) return low;
    // Xorshift32
    rng_state ^= rng_state << 13;
    rng_state ^= rng_state >> 17;
    rng_state ^= rng_state << 5;
    return low + (rng_state % (high - low + 1));
}

void os_fatal_error(const char *msg) {
    os_set_color(MAKE_COLOR(COLOR_WHITE, COLOR_RED));
    os_clear_screen();
    os_move_cursor(10, 25);
    os_print_string("*** MIKEOS FATAL ERROR ***");
    os_move_cursor(12, 15);
    if (msg) os_print_string(msg);
    os_move_cursor(14, 20);
    os_print_string("System halted. Press any key to reboot.");
    os_wait_for_key();
    os_reboot();
}

void os_reboot(void) {
    if (g_MiscST && g_MiscST->RuntimeServices) {
        uefi_call_wrapper(g_MiscST->RuntimeServices->ResetSystem, 4,
                          EfiResetCold, EFI_SUCCESS, 0, NULL);
    }
    while(1) { __asm__ volatile ("hlt"); }
}

void os_shutdown(void) {
    if (g_MiscST && g_MiscST->RuntimeServices) {
        uefi_call_wrapper(g_MiscST->RuntimeServices->ResetSystem, 4,
                          EfiResetShutdown, EFI_SUCCESS, 0, NULL);
    }
    while(1) { __asm__ volatile ("hlt"); }
}

void os_get_cpu_info(char *brand) {
    if (!brand) return;
    UINT32 regs[4];

    // CPUID 0x80000000: max extended function
    __asm__ volatile ("cpuid" : "=a"(regs[0]), "=b"(regs[1]), "=c"(regs[2]), "=d"(regs[3]) : "a"(0x80000000));
    if (regs[0] >= 0x80000004) {
        for (UINT32 i = 0; i < 3; i++) {
            __asm__ volatile ("cpuid" : "=a"(regs[0]), "=b"(regs[1]), "=c"(regs[2]), "=d"(regs[3]) : "a"(0x80000002 + i));
            for (int r = 0; r < 4; r++) {
                brand[i * 16 + r * 4 + 0] = (char)(regs[r] & 0xFF);
                brand[i * 16 + r * 4 + 1] = (char)((regs[r] >> 8) & 0xFF);
                brand[i * 16 + r * 4 + 2] = (char)((regs[r] >> 16) & 0xFF);
                brand[i * 16 + r * 4 + 3] = (char)((regs[r] >> 24) & 0xFF);
            }
        }
        brand[48] = '\0';
    } else {
        os_string_copy("Intel x86_64 Processor (13th Gen Compatible)", brand);
    }
}
