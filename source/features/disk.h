#ifndef _MIKEOS_DISK_H
#define _MIKEOS_DISK_H

#include "types.h"

EFI_STATUS os_disk_init(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable);
int os_get_file_list(char *buffer, int max_len);
int os_file_exists(const char *filename);
int os_load_file(const char *filename, void *dest_buffer, UINTN *file_size);
int os_write_file(const char *filename, void *src_buffer, UINTN file_size);
int os_get_file_size(const char *filename, UINTN *file_size);

#endif
