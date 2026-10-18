#include "disk.h"
#include "text.h"

static EFI_FILE_HANDLE g_RootDir = NULL;
static EFI_SYSTEM_TABLE *g_DiskST = NULL;

static void ascii_to_unicode(const char *src, CHAR16 *dest, int max) {
    int i = 0;
    while (src[i] && i < max - 1) {
        dest[i] = (CHAR16)src[i];
        i++;
    }
    dest[i] = 0;
}

static void unicode_to_ascii(const CHAR16 *src, char *dest, int max) {
    int i = 0;
    while (src[i] && i < max - 1) {
        dest[i] = (char)(src[i] & 0x7F);
        i++;
    }
    dest[i] = 0;
}

EFI_STATUS os_disk_init(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable) {
    g_DiskST = SystemTable;
    EFI_STATUS Status;
    EFI_LOADED_IMAGE_PROTOCOL *LoadedImage = NULL;
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *FileSystem = NULL;
    EFI_GUID loadedImageGuid = EFI_LOADED_IMAGE_PROTOCOL_GUID;
    EFI_GUID fileSystemGuid = EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID;

    Status = uefi_call_wrapper(SystemTable->BootServices->HandleProtocol, 3,
                               ImageHandle, &loadedImageGuid, (VOID **)&LoadedImage);
    if (!EFI_ERROR(Status) && LoadedImage != NULL) {
        Status = uefi_call_wrapper(SystemTable->BootServices->HandleProtocol, 3,
                                   LoadedImage->DeviceHandle, &fileSystemGuid, (VOID **)&FileSystem);
    }

    if (EFI_ERROR(Status) || FileSystem == NULL) {
        Status = LibLocateProtocol(&fileSystemGuid, (VOID **)&FileSystem);
    }

    if (!EFI_ERROR(Status) && FileSystem != NULL) {
        Status = uefi_call_wrapper(FileSystem->OpenVolume, 2, FileSystem, &g_RootDir);
    }

    return Status;
}

int os_get_file_list(char *buffer, int max_len) {
    if (!g_RootDir || !buffer || max_len <= 0) return 0;

    buffer[0] = '\0';
    int count = 0;
    int cur_len = 0;

    uefi_call_wrapper(g_RootDir->SetPosition, 2, g_RootDir, 0);

    UINT8 info_buf[512];
    UINTN buf_size = sizeof(info_buf);

    while (1) {
        buf_size = sizeof(info_buf);
        EFI_STATUS st = uefi_call_wrapper(g_RootDir->Read, 3, g_RootDir, &buf_size, info_buf);
        if (EFI_ERROR(st) || buf_size == 0) break;

        EFI_FILE_INFO *fi = (EFI_FILE_INFO *)info_buf;
        if (fi->Attribute & EFI_FILE_DIRECTORY) continue;

        char name[128];
        unicode_to_ascii(fi->FileName, name, sizeof(name));

        if (os_string_compare(name, ".") || os_string_compare(name, "..")) continue;

        int nlen = os_string_length(name);
        if (cur_len + nlen + 2 >= max_len) break;

        if (count > 0) {
            buffer[cur_len++] = ',';
        }
        for (int i = 0; i < nlen; i++) {
            buffer[cur_len++] = name[i];
        }
        buffer[cur_len] = '\0';
        count++;
    }

    return count;
}

int os_file_exists(const char *filename) {
    if (!g_RootDir || !filename) return 0;

    CHAR16 wname[256];
    ascii_to_unicode(filename, wname, 256);

    EFI_FILE_HANDLE file = NULL;
    EFI_STATUS st = uefi_call_wrapper(g_RootDir->Open, 5, g_RootDir, &file, wname, EFI_FILE_MODE_READ, 0);
    if (!EFI_ERROR(st) && file != NULL) {
        uefi_call_wrapper(file->Close, 1, file);
        return 1;
    }
    return 0;
}

int os_get_file_size(const char *filename, UINTN *file_size) {
    if (!g_RootDir || !filename || !file_size) return 0;

    CHAR16 wname[256];
    ascii_to_unicode(filename, wname, 256);

    EFI_FILE_HANDLE file = NULL;
    EFI_STATUS st = uefi_call_wrapper(g_RootDir->Open, 5, g_RootDir, &file, wname, EFI_FILE_MODE_READ, 0);
    if (EFI_ERROR(st) || file == NULL) return 0;

    UINT8 info_buf[512];
    UINTN buf_size = sizeof(info_buf);
    EFI_GUID fileInfoGuid = EFI_FILE_INFO_ID;
    st = uefi_call_wrapper(file->GetInfo, 4, file, &fileInfoGuid, &buf_size, info_buf);
    if (!EFI_ERROR(st)) {
        EFI_FILE_INFO *fi = (EFI_FILE_INFO *)info_buf;
        *file_size = fi->FileSize;
        uefi_call_wrapper(file->Close, 1, file);
        return 1;
    }

    uefi_call_wrapper(file->Close, 1, file);
    return 0;
}

int os_load_file(const char *filename, void *dest_buffer, UINTN *file_size) {
    if (!g_RootDir || !filename || !dest_buffer) return 0;

    CHAR16 wname[256];
    ascii_to_unicode(filename, wname, 256);

    EFI_FILE_HANDLE file = NULL;
    EFI_STATUS st = uefi_call_wrapper(g_RootDir->Open, 5, g_RootDir, &file, wname, EFI_FILE_MODE_READ, 0);
    if (EFI_ERROR(st) || file == NULL) return 0;

    UINT8 info_buf[512];
    UINTN buf_size = sizeof(info_buf);
    EFI_GUID fileInfoGuid = EFI_FILE_INFO_ID;
    st = uefi_call_wrapper(file->GetInfo, 4, file, &fileInfoGuid, &buf_size, info_buf);
    if (EFI_ERROR(st)) {
        uefi_call_wrapper(file->Close, 1, file);
        return 0;
    }

    EFI_FILE_INFO *fi = (EFI_FILE_INFO *)info_buf;
    UINTN read_size = (UINTN)fi->FileSize;

    st = uefi_call_wrapper(file->Read, 3, file, &read_size, dest_buffer);
    uefi_call_wrapper(file->Close, 1, file);

    if (EFI_ERROR(st)) return 0;
    if (file_size) *file_size = read_size;
    return 1;
}

int os_write_file(const char *filename, void *src_buffer, UINTN file_size) {
    if (!g_RootDir || !filename) return 0;

    CHAR16 wname[256];
    ascii_to_unicode(filename, wname, 256);

    EFI_FILE_HANDLE file = NULL;
    EFI_STATUS st = uefi_call_wrapper(g_RootDir->Open, 5, g_RootDir, &file, wname,
                                      EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE | EFI_FILE_MODE_CREATE, 0);
    if (EFI_ERROR(st) || file == NULL) return 0;

    UINTN write_size = file_size;
    st = uefi_call_wrapper(file->Write, 3, file, &write_size, src_buffer);
    uefi_call_wrapper(file->Close, 1, file);

    return !EFI_ERROR(st);
}
