import subprocess, time, os

cmd = [
    'qemu-system-x86_64',
    '-bios', '/usr/share/ovmf/OVMF.fd',
    '-drive', 'id=cd0,file=build_uefi/mikeos-uefi.iso,if=none,format=raw',
    '-device', 'ide-cd,drive=cd0,bootindex=0',
    '-net', 'none',
    '-monitor', 'stdio'
]

p = subprocess.Popen(cmd, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE)

# Wait for OVMF menu to appear
time.sleep(3)
# The first option is UEFI QEMU DVD-ROM, just press Enter!
p.stdin.write(b"sendkey ret\n")
p.stdin.flush()

# Give it 4 seconds to boot and draw the desktop
time.sleep(4)

# Take screenshot
p.stdin.write(b"screendump build_uefi/screen.ppm\n")
p.stdin.flush()
time.sleep(1)

p.stdin.write(b"quit\n")
p.stdin.flush()
time.sleep(1)

out, err = p.communicate()
if os.path.exists("build_uefi/screen.ppm"):
    with open('build_uefi/screen.ppm', 'rb') as f:
        f.readline(); f.readline(); f.readline()
        data = f.read()
        non_black = sum(1 for i in range(0, len(data), 3) if data[i] != 0 or data[i+1] != 0 or data[i+2] != 0)
        print('Non black pixels:', non_black, 'out of', len(data)//3)
else:
    print("Screenshot does not exist.")
