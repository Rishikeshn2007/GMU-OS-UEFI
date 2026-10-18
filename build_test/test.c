#include <efi.h>
#include <efilib.h>

EFI_STATUS
EFIAPI
efi_main (EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable) {
    InitializeLib(ImageHandle, SystemTable);
    Print(L"\r\n==================================\r\n");
    Print(L"  MikeOS UEFI 64-bit Test Running!\r\n");
    Print(L"==================================\r\n");
    for (volatile long i = 0; i < 80000000; i++);
    return EFI_SUCCESS;
}
