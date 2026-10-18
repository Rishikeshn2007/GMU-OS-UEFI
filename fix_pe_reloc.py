#!/usr/bin/env python3
"""
fix_pe_reloc.py - Convert ELF .rela relocations to PE .reloc section
This patches the PE file produced by objcopy to have correct base relocations.
"""
import struct
import sys
import subprocess
import os

def get_elf_rela_offsets(elf_path):
    """Parse ELF file and extract all R_X86_64_RELATIVE and R_X86_64_64 offsets."""
    offsets = []
    try:
        result = subprocess.run(
            ['readelf', '-r', elf_path],
            capture_output=True, text=True
        )
        for line in result.stdout.splitlines():
            parts = line.split()
            # Lines like: "  000000004010  000000000008 R_X86_64_RELATIVE ...  000..."
            if len(parts) >= 3 and ('R_X86_64_RELATIVE' in parts[2] or 'R_X86_64_64' in parts[2]):
                try:
                    vaddr = int(parts[0], 16)
                    offsets.append(vaddr)
                except ValueError:
                    pass
    except Exception as e:
        print(f"Warning: readelf failed: {e}")
    return offsets


def get_pe_image_base(pe_data):
    """Parse PE headers to get image base and section table."""
    # DOS header
    if pe_data[:2] != b'MZ':
        raise ValueError("Not a valid PE file")
    pe_offset = struct.unpack_from('<I', pe_data, 0x3C)[0]
    if pe_data[pe_offset:pe_offset+4] != b'PE\x00\x00':
        raise ValueError("PE signature not found")

    # COFF header
    num_sections = struct.unpack_from('<H', pe_data, pe_offset + 6)[0]
    opt_hdr_size = struct.unpack_from('<H', pe_data, pe_offset + 20)[0]

    opt_offset = pe_offset + 24
    magic = struct.unpack_from('<H', pe_data, opt_offset)[0]

    if magic == 0x20B:  # PE32+
        image_base = struct.unpack_from('<Q', pe_data, opt_offset + 24)[0]
    elif magic == 0x10B:  # PE32
        image_base = struct.unpack_from('<I', pe_data, opt_offset + 28)[0]
    else:
        raise ValueError(f"Unknown PE magic: {magic:#x}")

    # Section table
    section_table_offset = opt_offset + opt_hdr_size
    sections = []
    for i in range(num_sections):
        sec_off = section_table_offset + i * 40
        name = pe_data[sec_off:sec_off+8].rstrip(b'\x00').decode('ascii', errors='replace')
        vsize = struct.unpack_from('<I', pe_data, sec_off + 8)[0]
        vaddr = struct.unpack_from('<I', pe_data, sec_off + 12)[0]
        raw_size = struct.unpack_from('<I', pe_data, sec_off + 16)[0]
        raw_off = struct.unpack_from('<I', pe_data, sec_off + 20)[0]
        sections.append({'name': name, 'vaddr': vaddr, 'vsize': vsize,
                         'raw_off': raw_off, 'raw_size': raw_size, 'header_off': sec_off})

    return image_base, sections, pe_offset, opt_offset, num_sections, section_table_offset


def build_pe_reloc_section(rva_offsets):
    """Build a PE .reloc section from a list of RVAs that need DIR64 base relocs."""
    if not rva_offsets:
        return b''

    # Group RVAs by 4KB page
    pages = {}
    for rva in sorted(rva_offsets):
        page = rva & ~0xFFF
        offset = rva & 0xFFF
        if page not in pages:
            pages[page] = []
        pages[page].append(offset)

    reloc_data = b''
    for page_rva in sorted(pages.keys()):
        offsets = pages[page_rva]
        # Each entry: 4 bits type (0xA = DIR64) | 12 bits offset
        entries = [struct.pack('<H', (0xA << 12) | off) for off in offsets]
        # Pad to DWORD boundary
        if len(entries) % 2 != 0:
            entries.append(struct.pack('<H', 0))  # padding entry

        block_size = 8 + len(entries) * 2
        block = struct.pack('<II', page_rva, block_size)
        block += b''.join(entries)
        reloc_data += block

    return reloc_data


def get_elf_text_load_addr(elf_path):
    """Get the virtual load address of the ELF to compute PE RVAs."""
    try:
        result = subprocess.run(
            ['readelf', '-S', elf_path],
            capture_output=True, text=True
        )
        for line in result.stdout.splitlines():
            if '.text' in line:
                parts = line.split()
                for i, p in enumerate(parts):
                    if p == '.text' and i+3 < len(parts):
                        try:
                            return int(parts[i+2], 16)  # addr field
                        except ValueError:
                            pass
    except Exception as e:
        print(f"Warning: readelf -S failed: {e}")
    return 0x401000


def fix_pe_reloc(elf_so_path, pe_input_path, pe_output_path):
    """Main function: read ELF relocs and patch PE file."""
    print(f"  Reading ELF relocations from {elf_so_path}...")
    elf_rela_offsets = get_elf_rela_offsets(elf_so_path)
    print(f"  Found {len(elf_rela_offsets)} absolute ELF relocations")

    if not elf_rela_offsets:
        print("  No ELF relocations found, copying PE as-is")
        import shutil
        shutil.copy2(pe_input_path, pe_output_path)
        return

    # Read PE file
    with open(pe_input_path, 'rb') as f:
        pe_data = bytearray(f.read())

    try:
        image_base, sections, pe_offset, opt_offset, num_secs, sec_table_off = get_pe_image_base(pe_data)
    except Exception as e:
        print(f"  Error parsing PE: {e}")
        import shutil
        shutil.copy2(pe_input_path, pe_output_path)
        return

    print(f"  PE image base: {image_base:#x}")

    # Find text section RVA
    text_sec = next((s for s in sections if s['name'] == '.text'), None)
    pe_text_rva = text_sec['vaddr'] if text_sec else 0x1000

    # Get ELF text load VA to compute offset
    elf_text_va = get_elf_text_load_addr(elf_so_path)
    rva_adjustment = pe_text_rva - elf_text_va
    print(f"  ELF text VA: {elf_text_va:#x}, PE text RVA: {pe_text_rva:#x}, adj: {rva_adjustment:#x}")

    # Convert ELF VAs to PE RVAs
    pe_rvas = []
    for va in elf_rela_offsets:
        rva = va + rva_adjustment
        if rva > 0 and rva < 0x10000000:  # sanity check
            pe_rvas.append(rva)

    print(f"  Converted to {len(pe_rvas)} PE RVAs")

    # Build new .reloc section
    new_reloc_data = build_pe_reloc_section(pe_rvas)

    # Align to 512 bytes (sector size)
    aligned_size = (len(new_reloc_data) + 511) & ~511
    new_reloc_padded = new_reloc_data + b'\x00' * (aligned_size - len(new_reloc_data))

    # Find existing .reloc section
    reloc_sec = next((s for s in sections if s['name'] == '.reloc'), None)
    if reloc_sec is None:
        print("  Warning: No .reloc section found in PE, copying as-is")
        import shutil
        shutil.copy2(pe_input_path, pe_output_path)
        return

    print(f"  Replacing .reloc at raw offset {reloc_sec['raw_off']:#x}, old size {reloc_sec['raw_size']:#x}")
    print(f"  New .reloc data: {len(new_reloc_data)} bytes ({len(new_reloc_padded)} padded)")

    raw_off = reloc_sec['raw_off']
    # Extend file if needed
    needed = raw_off + aligned_size
    if needed > len(pe_data):
        pe_data.extend(b'\x00' * (needed - len(pe_data)))
    pe_data[raw_off:raw_off + aligned_size] = new_reloc_padded

    # Update section header
    sec_hdr = reloc_sec['header_off']
    struct.pack_into('<I', pe_data, sec_hdr + 8, len(new_reloc_data))   # VirtualSize
    struct.pack_into('<I', pe_data, sec_hdr + 16, aligned_size)          # SizeOfRawData

    # Update DataDirectory[5] (Base Relocation Table) size
    magic = struct.unpack_from('<H', pe_data, opt_offset)[0]
    if magic == 0x20B:  # PE32+
        dd_off = opt_offset + 24 + 88 + 5 * 8
    else:
        dd_off = opt_offset + 24 + 72 + 5 * 8

    struct.pack_into('<I', pe_data, dd_off + 4, len(new_reloc_data))

    with open(pe_output_path, 'wb') as f:
        f.write(bytes(pe_data))

    print(f"  Written fixed PE to {pe_output_path}")
    print(f"  Successfully added {len(pe_rvas)} DIR64 base relocations!")


if __name__ == '__main__':
    if len(sys.argv) != 4:
        print(f"Usage: {sys.argv[0]} <input.so> <input.efi> <output.efi>")
        sys.exit(1)
    fix_pe_reloc(sys.argv[1], sys.argv[2], sys.argv[3])
