import subprocess, time

p = subprocess.Popen([
    'qemu-system-x86_64',
    '-bios', '/usr/share/ovmf/OVMF.fd',
    '-cdrom', 'build_test/mikeos_uefi.iso',
    '-net', 'none',
    '-nographic'
], stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE)

# Give OVMF 1 second to start, then press Down Arrow, Enter
time.sleep(2)
p.stdin.write(b"\x1b[B\r")
p.stdin.flush()
time.sleep(3)
p.terminate()
out, err = p.communicate()
print("FULL STDOUT:")
print(out.decode('utf-8', errors='replace'))
