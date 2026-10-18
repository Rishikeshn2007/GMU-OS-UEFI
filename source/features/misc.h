#ifndef _MIKEOS_MISC_H
#define _MIKEOS_MISC_H

#include "types.h"

void os_misc_init(EFI_SYSTEM_TABLE *SystemTable);
void os_get_time_string(char *buffer);
void os_get_date_string(char *buffer);
void os_set_time_fmt(UINT8 fmt);
void os_set_date_fmt(UINT16 fmt);
void os_pause(int tenths_of_sec);
void os_fatal_error(const char *msg);
unsigned int os_get_random(unsigned int low, unsigned int high);
void os_seed_random(void);
void os_reboot(void);
void os_shutdown(void);
void os_get_cpu_info(char *brand);

#endif
