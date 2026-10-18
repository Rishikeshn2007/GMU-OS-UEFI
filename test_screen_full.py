#!/usr/bin/env python3
"""Extended QEMU test - waits longer and captures multiple screenshots."""
import subprocess, time, os

cmd = [
    'qemu-system-x86_64',
    '-bios', '/usr/share/ovmf/OVMF.fd',
    '-drive', 'id=cd0,file=build_uefi/mikeos-uefi.iso,if=none,format=raw',
    '-device', 'ide-cd,drive=cd0,bootindex=0',
    '-net', 'none',
    '-m', '256M',
    '-monitor', 'stdio'
]

p = subprocess.Popen(cmd, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE)

# Wait 5s for OVMF boot menu, then press Enter
time.sleep(5)
p.stdin.write(b"sendkey ret\n")
p.stdin.flush()

# Wait 8 seconds for kernel to boot and draw dialog
time.sleep(8)

# Take a screenshot
p.stdin.write(b"screendump build_uefi/screen_full.ppm\n")
p.stdin.flush()
time.sleep(1)

# Send Enter key to proceed past dialog (simulate OK)
p.stdin.write(b"sendkey ret\n")
p.stdin.flush()
time.sleep(3)

# Take another screenshot after dialog dismissed
p.stdin.write(b"screendump build_uefi/screen_after.ppm\n")
p.stdin.flush()
time.sleep(1)

p.stdin.write(b"quit\n")
p.stdin.flush()
time.sleep(1)

out, err = p.communicate(timeout=5)

# Convert PPMs to PNG
for base in ['screen_full', 'screen_after']:
    ppm = f'build_uefi/{base}.ppm'
    png = f'build_uefi/{base}.png'
    if os.path.exists(ppm):
        os.system(f'convert {ppm} {png} 2>/dev/null || python3 -c "import sys; data=open(\'{ppm}\',\'rb\').read(); h=data.split(b\'\\n\',3); px=h[3]; print(\'PPM size:\', len(px)//3, \'pixels\')"')
        print(f"Screenshot: {ppm} -> {png}")

print("Done.")
