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

# Strip /boot prefix for grub paths (grub resolves from boot partition root)
HVISOR_GRUB="${HVISOR_BIN#/boot}"
TRAP_VECTOR_GRUB="${TRAP_VECTOR#/boot}"

LOONGVISOR_CMD="loongvisor ${HVISOR_GRUB} 0x7f0000000 0x10000000 0x1e0000000 0x10000000 ${TRAP_VECTOR_GRUB}"

python3 - "$SRC" "$DST" "$LOONGVISOR_CMD" << 'PYEOF'
import sys, re

src, dst, loongvisor_cmd = sys.argv[1], sys.argv[2], sys.argv[3]

with open(src) as f:
    lines = f.readlines()

out = []
i = 0
loongstub_inserted = False
while i < len(lines):
    line = lines[i]

    # 1. Drop the top-level simple entry (keep only advanced/submenu entries)
    if re.search(r'\blinux_entry\b.*simple', line) or re.search(r'simple.*\blinux_entry\b', line):
        # skip continuation lines
        while line.rstrip('\n').rstrip().endswith('\\'):
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
    # Insert "insmod loongstub" once, just before the first loongvisor call
    m = re.match(r'^(\s+)linux\s+', line)
    if m:
        indent = m.group(1)
        rest = line[m.end():]  # everything after "linux "
        if not loongstub_inserted:
            out.append(f"{indent}insmod loongstub\n")
            loongstub_inserted = True
        line = f"{indent}{loongvisor_cmd}\n{indent}root_linux {rest}"

    # 4. Replace initrd command with root_initrd
    m = re.match(r'^(\s+)initrd\s+', line)
    if m:
        indent = m.group(1)
        rest = line[m.end():]
        line = f"{indent}root_initrd {rest}"

    out.append(line)
    i += 1

with open(dst, 'w') as f:
    f.writelines(out)

print(f"Done: {dst}")
PYEOF

chmod +x "$DST"
echo "Run: sudo update-grub"
