import subprocess, time

p = subprocess.Popen([
    'qemu-system-x86_64',
    '-bios', '/usr/share/ovmf/OVMF.fd',
    '-drive', 'id=cd0,file=build_test/mikeos_uefi.iso,if=none,format=raw',
    '-device', 'ide-cd,drive=cd0,bootindex=0',
    '-net', 'none',
    '-nographic',
    '-serial', 'mon:stdio'
], stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE)

time.sleep(2.5)
p.stdin.write(b"\r\n")
p.stdin.flush()
time.sleep(3)
p.terminate()
out, err = p.communicate()
print("OUTPUT:")
for line in out.decode('latin1', errors='replace').splitlines():
    if any(k in line.upper() for k in ['MIKEOS', 'TEST', 'BDSDXE', 'STARTING', 'LOAD', 'FAIL']):
        print(line)
