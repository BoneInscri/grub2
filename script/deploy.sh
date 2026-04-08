#!/bin/bash
# Deploy loongstub artifacts to the target LoongArch machine.
# Run this script on the target machine from the loongstub_img directory.
# Usage: sudo bash deploy.sh

set -e

SCRIPT_DIR=$(dirname "$(realpath "$0")")

BOOT_DST=/boot
BIN_DST=/bin
KMOD_DST="/lib/modules/$(uname -r)/extra"

# Auto-detect grub module directory for loongarch64-efi
GRUB_MOD_DST=$(find /boot -type d -name "loongarch64-efi" 2>/dev/null | head -1)
if [ -z "$GRUB_MOD_DST" ]; then
    echo "Error: could not find loongarch64-efi grub module directory under /boot"
    echo "Hint: run 'find /boot -name \"*.mod\" | head -5' to locate it manually"
    exit 1
fi
echo "Detected GRUB module dir: $GRUB_MOD_DST"

# Verify required files exist next to this script
for f in loongstub.mod hvisor.bin hvisor-trap-vector.txt gen_loongvisor_grub.sh hvisor hvisor.ko; do
    if [ ! -f "$SCRIPT_DIR/$f" ]; then
        echo "Error: missing $f in $SCRIPT_DIR"
        exit 1
    fi
done

# Install loongstub grub module
echo "Installing loongstub.mod -> $GRUB_MOD_DST/"
cp -v "$SCRIPT_DIR/loongstub.mod" "$GRUB_MOD_DST/"

# Install hvisor artifacts
echo "Installing hvisor.bin -> $BOOT_DST/"
cp -v "$SCRIPT_DIR/hvisor.bin"              "$BOOT_DST/"
cp -v "$SCRIPT_DIR/hvisor-trap-vector.txt"  "$BOOT_DST/"

# Install hvisor userspace tool
echo "Installing hvisor -> $BIN_DST/"
cp -v "$SCRIPT_DIR/hvisor" "$BIN_DST/"
chmod +x "$BIN_DST/hvisor"

# Install hvisor kernel module
echo "Installing hvisor.ko -> $KMOD_DST/"
mkdir -p "$KMOD_DST"
cp -v "$SCRIPT_DIR/hvisor.ko" "$KMOD_DST/"

# Generate /etc/grub.d/09_loongvisor and update grub config
echo "Generating /etc/grub.d/09_loongvisor ..."
bash "$SCRIPT_DIR/gen_loongvisor_grub.sh"

echo "Updating grub config ..."
update-grub

echo "Deploy done."
