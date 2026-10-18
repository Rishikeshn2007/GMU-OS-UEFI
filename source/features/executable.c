#include "executable.h"
#include "disk.h"
#include "text.h"

#define MAX_EFI_IMAGE_SIZE (16 * 1024 * 1024)

EFI_STATUS os_execute_efi_file(
    EFI_HANDLE ParentImage,
    EFI_SYSTEM_TABLE *SystemTable,
    const char *filename) {
    EFI_STATUS status;
    EFI_STATUS unload_status;
    EFI_HANDLE child_image = NULL;
    EFI_STATUS child_status = EFI_SUCCESS;
    UINTN image_size = 0;
    UINTN exit_data_size = 0;
    CHAR16 *exit_data = NULL;
    VOID *image_buffer = NULL;

    if (!filename || !SystemTable || !SystemTable->BootServices) {
        return EFI_INVALID_PARAMETER;
    }

    if (!os_get_file_size(filename, &image_size)) {
        return EFI_NOT_FOUND;
    }
    if (image_size == 0 || image_size > MAX_EFI_IMAGE_SIZE) {
        return EFI_BAD_BUFFER_SIZE;
    }

    status = uefi_call_wrapper(
        SystemTable->BootServices->AllocatePool, 3,
        EfiLoaderData, image_size, &image_buffer);
    if (EFI_ERROR(status)) return status;

    {
        UINTN bytes_read = 0;
        if (!os_load_file(filename, image_buffer, &bytes_read) ||
            bytes_read != image_size) {
            uefi_call_wrapper(SystemTable->BootServices->FreePool, 1, image_buffer);
            return EFI_DEVICE_ERROR;
        }
    }

    status = uefi_call_wrapper(
        SystemTable->BootServices->LoadImage, 6,
        FALSE, ParentImage, NULL, image_buffer, image_size, &child_image);
    uefi_call_wrapper(SystemTable->BootServices->FreePool, 1, image_buffer);
    if (EFI_ERROR(status)) return status;

    status = uefi_call_wrapper(
        SystemTable->BootServices->StartImage, 3,
        child_image, &exit_data_size, &exit_data);
    if (exit_data) {
        uefi_call_wrapper(SystemTable->BootServices->FreePool, 1, exit_data);
    }

    child_status = status;
    unload_status =
        uefi_call_wrapper(SystemTable->BootServices->UnloadImage, 1, child_image);
    if (!EFI_ERROR(child_status) && EFI_ERROR(unload_status)) {
        return unload_status;
    }
    return child_status;
}
