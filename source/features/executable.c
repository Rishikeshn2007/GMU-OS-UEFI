#include "executable.h"
#include "disk.h"
#include "../../programs/gmu_api.h"

#define GMU_ELF_CLASS_64 2
#define GMU_ELF_DATA_LSB 1
#define GMU_ELF_TYPE_DYN 3
#define GMU_ELF_MACHINE_X86_64 62
#define GMU_ELF_PT_LOAD 1
#define GMU_ELF_PT_DYNAMIC 2
#define GMU_ELF_PF_X 1
#define GMU_ELF_DT_NULL 0
#define GMU_ELF_DT_RELA 7
#define GMU_ELF_DT_RELASZ 8
#define GMU_ELF_DT_RELAENT 9
#define GMU_ELF_DT_REL 17
#define GMU_ELF_DT_RELSZ 18
#define GMU_ELF_DT_JMPREL 23
#define GMU_ELF_DT_PLTRELSZ 2
#define GMU_ELF_R_X86_64_RELATIVE 8
#define GMU_MAX_BIN_SIZE (16 * 1024 * 1024)

typedef struct {
    UINT8 ident[16];
    UINT16 type;
    UINT16 machine;
    UINT32 version;
    UINT64 entry;
    UINT64 program_header_offset;
    UINT64 section_header_offset;
    UINT32 flags;
    UINT16 header_size;
    UINT16 program_header_size;
    UINT16 program_header_count;
    UINT16 section_header_size;
    UINT16 section_header_count;
    UINT16 section_name_index;
} GmuElfHeader;

typedef struct {
    UINT32 type;
    UINT32 flags;
    UINT64 offset;
    UINT64 virtual_address;
    UINT64 physical_address;
    UINT64 file_size;
    UINT64 memory_size;
    UINT64 alignment;
} GmuElfProgramHeader;

typedef struct {
    INT64 tag;
    union {
        UINT64 value;
        UINT64 pointer;
    } data;
} GmuElfDynamic;

typedef struct {
    UINT64 offset;
    UINT64 info;
    INT64 addend;
} GmuElfRela;

typedef int (GMU_APP_ABI *GmuAppEntry)(const GMU_APP_API *api);

extern void GMU_APP_ABI gmu_app_clear_screen(void);
extern void GMU_APP_ABI gmu_app_set_color(UINT8 attr);
extern void GMU_APP_ABI gmu_app_print_string(const char *text);
extern void GMU_APP_ABI gmu_app_print_char(char value);
extern void GMU_APP_ABI gmu_app_print_space(void);
extern UINT8 GMU_APP_ABI gmu_app_wait_for_key(void);
extern int GMU_APP_ABI gmu_app_input_dialog(char *buffer, int max_chars, const char *prompt);
extern int GMU_APP_ABI gmu_app_list_dialog(
    const char *items,
    const char *help1,
    const char *help2,
    char *selected_item);
extern unsigned int GMU_APP_ABI gmu_app_random(unsigned int low, unsigned int high);

static const GMU_APP_API gmu_app_api = {
    GMU_APP_ABI_VERSION,
    sizeof(GMU_APP_API),
    gmu_app_clear_screen,
    gmu_app_set_color,
    gmu_app_print_string,
    gmu_app_print_char,
    gmu_app_print_space,
    gmu_app_wait_for_key,
    gmu_app_input_dialog,
    gmu_app_list_dialog,
    gmu_app_random
};

static int range_in_file(UINTN file_size, UINT64 offset, UINT64 length) {
    return offset <= file_size && length <= (UINT64)file_size - offset;
}

static void zero_memory(VOID *memory, UINTN size) {
    UINT8 *bytes = (UINT8 *)memory;
    for (UINTN i = 0; i < size; i++) bytes[i] = 0;
}

static int address_is_loaded(
    UINT64 address,
    UINT64 length,
    UINT64 load_bias,
    const GmuElfProgramHeader *headers,
    UINTN header_count) {
    UINT64 virtual_address;
    if (address < load_bias) return 0;
    virtual_address = address - load_bias;

    for (UINTN i = 0; i < header_count; i++) {
        const GmuElfProgramHeader *header = &headers[i];
        if (header->type != GMU_ELF_PT_LOAD ||
            virtual_address < header->virtual_address) continue;
        if (virtual_address - header->virtual_address <= header->memory_size &&
            length <= header->memory_size -
                (virtual_address - header->virtual_address)) return 1;
    }
    return 0;
}

static EFI_STATUS apply_relative_relocations(
    const GmuElfProgramHeader *dynamic_header,
    UINT64 load_bias,
    const GmuElfProgramHeader *headers,
    UINTN header_count) {
    const GmuElfDynamic *dynamic;
    UINT64 rela_address = 0;
    UINT64 rela_size = 0;
    UINT64 rela_entry_size = sizeof(GmuElfRela);
    UINT64 rela_count;

    if (!dynamic_header) return EFI_SUCCESS;
    if (dynamic_header->memory_size % sizeof(GmuElfDynamic) != 0 ||
        dynamic_header->virtual_address > UINT64_MAX - load_bias ||
        !address_is_loaded(load_bias + dynamic_header->virtual_address,
            dynamic_header->memory_size, load_bias, headers, header_count)) {
        return EFI_LOAD_ERROR;
    }

    dynamic = (const GmuElfDynamic *)(UINTN)(load_bias + dynamic_header->virtual_address);
    for (UINTN i = 0;
         i < dynamic_header->memory_size / sizeof(GmuElfDynamic);
         i++) {
        switch (dynamic[i].tag) {
            case GMU_ELF_DT_NULL:
                i = dynamic_header->memory_size / sizeof(GmuElfDynamic);
                break;
            case GMU_ELF_DT_RELA:
                rela_address = dynamic[i].data.pointer;
                break;
            case GMU_ELF_DT_RELASZ:
                rela_size = dynamic[i].data.value;
                break;
            case GMU_ELF_DT_RELAENT:
                rela_entry_size = dynamic[i].data.value;
                break;
            case GMU_ELF_DT_REL:
            case GMU_ELF_DT_RELSZ:
            case GMU_ELF_DT_JMPREL:
            case GMU_ELF_DT_PLTRELSZ:
                return EFI_UNSUPPORTED;
            default:
                break;
        }
    }

    if (rela_size == 0) return EFI_SUCCESS;
    if (rela_address > UINT64_MAX - load_bias ||
        rela_entry_size != sizeof(GmuElfRela) ||
        rela_size % rela_entry_size != 0) return EFI_LOAD_ERROR;

    rela_count = rela_size / rela_entry_size;
    if (!address_is_loaded(load_bias + rela_address, rela_size,
                           load_bias, headers, header_count)) return EFI_LOAD_ERROR;

    {
        const GmuElfRela *relocations =
            (const GmuElfRela *)(UINTN)(load_bias + rela_address);
        for (UINT64 i = 0; i < rela_count; i++) {
            UINT32 symbol = (UINT32)(relocations[i].info >> 32);
            UINT32 type = (UINT32)relocations[i].info;
            UINT64 target;

            if (symbol != 0 || type != GMU_ELF_R_X86_64_RELATIVE ||
                relocations[i].offset > UINT64_MAX - load_bias) {
                return EFI_UNSUPPORTED;
            }

            target = load_bias + relocations[i].offset;
            if (!address_is_loaded(target, sizeof(UINT64), load_bias,
                                   headers, header_count)) return EFI_LOAD_ERROR;
            *(UINT64 *)(UINTN)target = load_bias + (UINT64)relocations[i].addend;
        }
    }
    return EFI_SUCCESS;
}

EFI_STATUS os_execute_bin_file(const char *filename) {
    EFI_STATUS status;
    UINTN file_size = 0;
    UINTN pages;
    UINTN page_base;
    UINT64 min_address = UINT64_MAX;
    UINT64 max_address = 0;
    UINT64 load_bias;
    UINT64 mapped_size;
    VOID *file_buffer = NULL;
    GmuElfHeader *elf;
    GmuElfProgramHeader *program_headers;
    const GmuElfProgramHeader *dynamic_header = NULL;
    int entry_is_executable = 0;
    GmuAppEntry entry;
    VOID *loaded_image = NULL;

    if (!filename || !gST || !gST->BootServices) return EFI_INVALID_PARAMETER;
    if (!os_get_file_size(filename, &file_size)) return EFI_NOT_FOUND;
    if (file_size < sizeof(GmuElfHeader) || file_size > GMU_MAX_BIN_SIZE) {
        return EFI_BAD_BUFFER_SIZE;
    }

    status = uefi_call_wrapper(
        gST->BootServices->AllocatePool, 3, EfiLoaderData, file_size, &file_buffer);
    if (EFI_ERROR(status)) return status;

    {
        UINTN bytes_read = 0;
        if (!os_load_file(filename, file_buffer, &bytes_read) ||
            bytes_read != file_size) {
            uefi_call_wrapper(gST->BootServices->FreePool, 1, file_buffer);
            return EFI_DEVICE_ERROR;
        }
    }

    elf = (GmuElfHeader *)file_buffer;
    if (elf->ident[0] != 0x7F || elf->ident[1] != 'E' ||
        elf->ident[2] != 'L' || elf->ident[3] != 'F' ||
        elf->ident[4] != GMU_ELF_CLASS_64 ||
        elf->ident[5] != GMU_ELF_DATA_LSB ||
        elf->type != GMU_ELF_TYPE_DYN ||
        elf->machine != GMU_ELF_MACHINE_X86_64 ||
        elf->program_header_size != sizeof(GmuElfProgramHeader) ||
        elf->program_header_count == 0 ||
        !range_in_file(file_size, elf->program_header_offset,
            (UINT64)elf->program_header_count * elf->program_header_size)) {
        uefi_call_wrapper(gST->BootServices->FreePool, 1, file_buffer);
        return EFI_LOAD_ERROR;
    }

    program_headers = (GmuElfProgramHeader *)
        ((UINT8 *)file_buffer + elf->program_header_offset);
    for (UINTN i = 0; i < elf->program_header_count; i++) {
        GmuElfProgramHeader *header = &program_headers[i];
        UINT64 segment_end;

        if (header->type == GMU_ELF_PT_DYNAMIC) dynamic_header = header;
        if (header->type != GMU_ELF_PT_LOAD || header->memory_size == 0) continue;
        if (header->file_size > header->memory_size ||
            !range_in_file(file_size, header->offset, header->file_size) ||
            header->virtual_address > UINT64_MAX - header->memory_size) {
            uefi_call_wrapper(gST->BootServices->FreePool, 1, file_buffer);
            return EFI_LOAD_ERROR;
        }

        segment_end = header->virtual_address + header->memory_size;
        if ((header->virtual_address & ~(EFI_PAGE_SIZE - 1)) < min_address) {
            min_address = header->virtual_address & ~(EFI_PAGE_SIZE - 1);
        }
        if (segment_end > max_address) max_address = segment_end;
        if ((header->flags & GMU_ELF_PF_X) &&
            elf->entry >= header->virtual_address &&
            elf->entry - header->virtual_address < header->memory_size) {
            entry_is_executable = 1;
        }
    }

    if (min_address == UINT64_MAX || max_address <= min_address ||
        max_address > UINT64_MAX - (EFI_PAGE_SIZE - 1) ||
        !entry_is_executable) {
        uefi_call_wrapper(gST->BootServices->FreePool, 1, file_buffer);
        return EFI_LOAD_ERROR;
    }

    max_address = (max_address + EFI_PAGE_SIZE - 1) & ~(EFI_PAGE_SIZE - 1);
    mapped_size = max_address - min_address;
    if (mapped_size > GMU_MAX_BIN_SIZE) {
        uefi_call_wrapper(gST->BootServices->FreePool, 1, file_buffer);
        return EFI_BAD_BUFFER_SIZE;
    }

    pages = EFI_SIZE_TO_PAGES(mapped_size);
    status = uefi_call_wrapper(
        gST->BootServices->AllocatePages, 4,
        AllocateAnyPages, EfiLoaderData, pages, &page_base);
    if (EFI_ERROR(status)) {
        uefi_call_wrapper(gST->BootServices->FreePool, 1, file_buffer);
        return status;
    }
    if ((UINT64)page_base < min_address) {
        uefi_call_wrapper(gST->BootServices->FreePages, 2, page_base, pages);
        uefi_call_wrapper(gST->BootServices->FreePool, 1, file_buffer);
        return EFI_LOAD_ERROR;
    }

    load_bias = (UINT64)page_base - min_address;
    loaded_image = (VOID *)(UINTN)page_base;
    zero_memory(loaded_image, (UINTN)mapped_size);

    for (UINTN i = 0; i < elf->program_header_count; i++) {
        const GmuElfProgramHeader *header = &program_headers[i];
        UINT8 *destination;
        if (header->type != GMU_ELF_PT_LOAD || header->memory_size == 0) continue;
        destination = (UINT8 *)(UINTN)(load_bias + header->virtual_address);
        for (UINT64 j = 0; j < header->file_size; j++) {
            destination[j] = ((const UINT8 *)file_buffer)[header->offset + j];
        }
    }

    status = apply_relative_relocations(
        dynamic_header, load_bias, program_headers, elf->program_header_count);
    if (!EFI_ERROR(status)) {
        if (elf->entry > UINT64_MAX - load_bias) {
            status = EFI_LOAD_ERROR;
        } else {
            entry = (GmuAppEntry)(UINTN)(load_bias + elf->entry);
            (void)entry(&gmu_app_api);
        }
    }

    uefi_call_wrapper(gST->BootServices->FreePages, 2, page_base, pages);
    uefi_call_wrapper(gST->BootServices->FreePool, 1, file_buffer);
    return status;
}
