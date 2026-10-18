import subprocess, time

cmd = [
    'qemu-system-x86_64',
    '-bios', '/usr/share/ovmf/OVMF.fd',
    '-drive', 'id=cd0,file=build_uefi/mikeos-uefi.iso,if=none,format=raw',
    '-device', 'ide-cd,drive=cd0,bootindex=0',
    '-net', 'none',
    '-nographic',
    '-serial', 'mon:stdio'
]

p = subprocess.Popen(cmd, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE)

time.sleep(2)
p.stdin.write(b"\r\n")
p.stdin.flush()
time.sleep(3)
p.terminate()

out, err = p.communicate()
for line in out.decode('latin1', errors='replace').splitlines():
    print(line)
