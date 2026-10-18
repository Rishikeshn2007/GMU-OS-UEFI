#ifndef MIKEOS_EXECUTABLE_H
#define MIKEOS_EXECUTABLE_H

#include "types.h"

EFI_STATUS os_execute_efi_file(
    EFI_HANDLE ParentImage,
    EFI_SYSTEM_TABLE *SystemTable,
    const char *filename);

#endif
