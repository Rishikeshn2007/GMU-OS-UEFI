#include "sound.h"

static inline void outb(UINT16 port, UINT8 val) {
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline UINT8 inb(UINT16 port) {
    UINT8 val;
    __asm__ volatile ("inb %1, %0" : "=a"(val) : "Nd"(port));
    return val;
}

void os_speaker_tone(UINT16 freq) {
    if (freq == 0) return;
    UINT32 div = 1193180 / freq;

    outb(0x43, 0xB6);
    outb(0x42, (UINT8)(div & 0xFF));
    outb(0x42, (UINT8)((div >> 8) & 0xFF));

    UINT8 tmp = inb(0x61);
    if ((tmp & 3) != 3) {
        outb(0x61, tmp | 3);
    }
}

void os_speaker_off(void) {
    UINT8 tmp = inb(0x61) & 0xFC;
    outb(0x61, tmp);
}
