#!/bin/bash
# Generate /etc/grub.d/09_loongvisor from 10_linux with loongvisor support.
# Usage: sudo bash gen_loongvisor_grub.sh

SRC=/etc/grub.d/10_linux
DST=/etc/grub.d/09_loongvisor

if [ ! -f "$SRC" ]; then
    echo "Error: $SRC not found"
    exit 1
fi

# Locate hvisor.bin under /boot
HVISOR_BIN=$(find /boot -name "hvisor.bin" 2>/dev/null | head -1)
if [ -z "$HVISOR_BIN" ]; then
    echo "Error: hvisor.bin not found under /boot"
    exit 1
fi

HVISOR_DIR=$(dirname "$HVISOR_BIN")
TRAP_VECTOR="${HVISOR_DIR}/hvisor-trap-vector.txt"

if [ ! -f "$TRAP_VECTOR" ]; then
    echo "Error: trap vector file not found: ${TRAP_VECTOR}"
    exit 1
fi

echo "Found hvisor.bin:   $HVISOR_BIN"
echo "Found trap-vector:  $TRAP_VECTOR"

LOONGVISOR_CMD="loongvisor ${HVISOR_BIN} 0x7f0000000 0x10000000 0x1e0000000 0x10000000 ${TRAP_VECTOR}"

python3 - "$SRC" "$DST" "$LOONGVISOR_CMD" << 'PYEOF'
import sys, re

src, dst, loongvisor_cmd = sys.argv[1], sys.argv[2], sys.argv[3]

with open(src) as f:
    lines = f.readlines()

out = []
i = 0
while i < len(lines):
    line = lines[i]

    # 1. Drop the top-level simple entry (loongvisor submenu only)
    if re.search(r'linux_entry.*simple', line):
        while line.rstrip().endswith('\\'):
            i += 1
            line = lines[i]
        i += 1
        continue

    # 2. Rename submenu title and id to Loongvisor options
    line = re.sub(
        r'gettext_printf "Advanced options for %s" "\$\{OS\}"',
        'echo -n "Loongvisor options for ${OS}"',
        line
    )
    line = line.replace(
        "'gnulinux-advanced-$boot_device_id'",
        "'loongvisor-options-$boot_device_id'"
    )

    # 3. Replace linux command with loongvisor + root_linux
    line = re.sub(r'^(linux\t)', loongvisor_cmd + '\nroot_linux\t', line)

    # 4. Replace initrd command with root_initrd
    line = re.sub(r'^(initrd\t)', r'root_initrd\t', line)

    out.append(line)
    i += 1

with open(dst, 'w') as f:
    f.writelines(out)

print(f"Done: {dst}")
PYEOF

chmod +x "$DST"
echo "Run: sudo update-grub"
