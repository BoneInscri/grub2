#!/bin/bash
# Transfer loongstub artifacts to the USB drive staging directory.
# Usage: bash script/transfer.sh

set -e

GRUB_MOD_SRC=/opt/grub-loongarch/lib/grub/loongarch64-efi
HVISOR_BIN=../../hvisor/target/loongarch64-unknown-none/debug/hvisor.bin
TRAP_VECTOR=../../hvisor/hvisor-trap-vector.txt
DST=/media/boneinscri/3A5000/loongstub_img
SCRIPT_DIR=$(dirname "$(realpath "$0")")

YELLOW='\033[1;33m'
NC='\033[0m'
warn() { echo -e "${YELLOW}Warning: $*${NC}"; }

# Verify sources exist (warn only, continue if missing)
for f in "$HVISOR_BIN" "$TRAP_VECTOR" "$GRUB_MOD_SRC/loongstub.mod"; do
    if [ ! -f "$f" ]; then
        warn "not found: $f"
    fi
done

mkdir -p "$DST"

# Sync loongstub grub module
[ -f "$GRUB_MOD_SRC/loongstub.mod" ] && rsync -rlv --checksum "$GRUB_MOD_SRC/loongstub.mod" "$DST/" || warn "skipping loongstub.mod"

# Copy hvisor artifacts
[ -f "$HVISOR_BIN" ]  && cp -v "$HVISOR_BIN"  "$DST/hvisor.bin"             || warn "skipping hvisor.bin"
[ -f "$TRAP_VECTOR" ] && cp -v "$TRAP_VECTOR"  "$DST/hvisor-trap-vector.txt" || warn "skipping hvisor-trap-vector.txt"

# Copy scripts
[ -f "$SCRIPT_DIR/gen_loongvisor_grub.sh" ] && cp -v "$SCRIPT_DIR/gen_loongvisor_grub.sh" "$DST/" || warn "skipping gen_loongvisor_grub.sh"
[ -f "$SCRIPT_DIR/deploy.sh" ]              && cp -v "$SCRIPT_DIR/deploy.sh"               "$DST/" || warn "skipping deploy.sh"

sync
echo "Transfer done: $DST"
