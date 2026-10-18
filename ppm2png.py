#!/usr/bin/env python3
import struct, zlib

def ppm_to_png(ppm_file, png_file):
    with open(ppm_file, 'rb') as f:
        assert f.readline() == b'P6\n'
        dims = f.readline().decode().strip().split()
        w, h = int(dims[0]), int(dims[1])
        f.readline()  # maxval
        raw = f.read()

    def make_png_chunk(ctype, data):
        c = ctype.encode() + data
        return struct.pack('>I', len(data)) + c + struct.pack('>I', zlib.crc32(c) & 0xffffffff)

    ihdr = struct.pack('>IIBBBBB', w, h, 8, 2, 0, 0, 0)
    scanlines = b''
    for y in range(h):
        row = raw[y*w*3:(y+1)*w*3]
        scanlines += b'\x00' + row

    with open(png_file, 'wb') as f:
        f.write(b'\x89PNG\r\n\x1a\n')
        f.write(make_png_chunk('IHDR', ihdr))
        f.write(make_png_chunk('IDAT', zlib.compress(scanlines, 6)))
        f.write(make_png_chunk('IEND', b''))
    print(f'Converted {ppm_file} -> {png_file}')

import os
pairs = [
    ('build_uefi/screen.ppm', 'build_uefi/screen.png'),
    ('build_uefi/screen_full.ppm', 'build_uefi/screen_full.png'),
    ('build_uefi/screen_after.ppm', 'build_uefi/screen_after.png'),
]
for ppm, png in pairs:
    if os.path.exists(ppm):
        ppm_to_png(ppm, png)
