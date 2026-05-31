#!/bin/bash
# Transfer loongstub artifacts to the USB drive staging directory.
# Usage: bash script/transfer.sh

set -e

SCRIPT_DIR=$(dirname "$(realpath "$0")")
BASE=$SCRIPT_DIR/../..

GRUB_MOD_SRC=/opt/grub-loongarch/lib/grub/loongarch64-efi
HVISOR_BIN=$BASE/hvisor/target/loongarch64-unknown-none/debug/hvisor.bin
TRAP_VECTOR=$BASE/hvisor/hvisor-trap-vector.txt
HVISOR_TOOL_OUTPUT=$BASE/hvisor-tool/output
HVISOR_TOOL_EXAMPLES=$BASE/hvisor-tool/examples/3a6000-loongarch64
GUEST_VMLINUX_BIN=$BASE/Guest/linux-6.13/vmlinux.bin
GUEST_VMLINUX_DTB_BIN=$BASE/Guest/linux-6.13-dtb/vmlinux
GUEST_SEL4_BIN=$(find "$BASE/Guest/sel4-la/build_3A5000/images/" -maxdepth 1 -name "*.bin" 2>/dev/null | head -n 1)
[ -z "$GUEST_SEL4_BIN" ] && warn "no .bin found in $BASE/Guest/sel4-la/build_3A5000/images/"
GUEST_RTTHREAD_BIN=$BASE/Guest/rt-thread-loongarch/bsp/qemu-virt64-loongarch/rtthread.bin
GUEST_NPUCORE_BIN=$BASE/Guest/NPUCore/os/target/loongarch64-unknown-linux-gnu/release/os.bin
DST=/media/$(whoami)/3A5000/loongstub_img

YELLOW='\033[1;33m'
NC='\033[0m'
warn() { echo -e "${YELLOW}Warning: $*${NC}"; }

cp_file() {
    local src="$1" dst="$2"
    if [ -f "$src" ]; then
        rsync -av --no-perms --no-owner --no-group "$src" "$dst"
    else
        warn "not found, skipping: $src"
    fi
}

# Verify sources exist (warn only, continue if missing)
for f in "$HVISOR_BIN" "$TRAP_VECTOR" "$GRUB_MOD_SRC/loongstub.mod"; do
    [ ! -f "$f" ] && warn "not found: $f"
done

MOUNT_POINT=/media/$(whoami)/3A5000
if ! mountpoint -q "$MOUNT_POINT"; then
    echo "Error: USB drive not mounted at $MOUNT_POINT"
    exit 1
fi

mkdir -p "$DST"

# Copy loongstub grub module
cp_file "$GRUB_MOD_SRC/loongstub.mod" "$DST/"

# Copy hvisor artifacts
cp_file "$HVISOR_BIN"   "$DST/hvisor.bin"
cp_file "$TRAP_VECTOR"  "$DST/hvisor-trap-vector.txt"

# Copy scripts
cp_file "$SCRIPT_DIR/gen_loongvisor_grub.sh"      "$DST/"
cp_file "$SCRIPT_DIR/deploy.sh"                   "$DST/"
cp_file "$SCRIPT_DIR/install_hvisor.sh"           "$DST/"
cp_file "$SCRIPT_DIR/re-install_hvisor.sh"        "$DST/"
cp_file "$SCRIPT_DIR/kill_virtio.sh"              "$DST/"
cp_file "$SCRIPT_DIR/test1.sh"                    "$DST/"
cp_file "$SCRIPT_DIR/test2.sh"                    "$DST/"
cp_file "$SCRIPT_DIR/test3.sh"                    "$DST/"
cp_file "$SCRIPT_DIR/test4.sh"                    "$DST/"
cp_file "$SCRIPT_DIR/test5.sh"                    "$DST/"
cp_file "$SCRIPT_DIR/../../Debug/dump_acpi.sh"    "$DST/"

# Copy hvisor-tool artifacts
cp_file "$HVISOR_TOOL_OUTPUT/hvisor"            "$DST/"
cp_file "$HVISOR_TOOL_OUTPUT/hvisor.ko"         "$DST/"
cp_file "$HVISOR_TOOL_OUTPUT/hyperamp_linux"    "$DST/"
cp_file "$HVISOR_TOOL_OUTPUT/hyperamp_backend"  "$DST/"

# Copy hyperamp test scripts
if [ -d "$SCRIPT_DIR/hyperamp_test" ]; then
    rsync -av --no-perms --no-owner --no-group "$SCRIPT_DIR/hyperamp_test/" "$DST/hyperamp_test/"
else
    warn "not found, skipping: $SCRIPT_DIR/hyperamp_test"
fi

# Copy hvisor-tool examples/3a6000-loongarch64
if [ -d "$HVISOR_TOOL_EXAMPLES" ]; then
    rsync -av --no-perms --no-owner --no-group "$HVISOR_TOOL_EXAMPLES/" "$DST/examples/"
else
    warn "not found, skipping: $HVISOR_TOOL_EXAMPLES"
fi

# Copy guest kernel
mkdir -p "$DST/Guest"
cp_file "$GUEST_VMLINUX_BIN" "$DST/Guest/vmlinux.bin"
cp_file "$GUEST_VMLINUX_DTB_BIN" "$DST/Guest/vmlinux-dtb.bin"
cp_file "$GUEST_SEL4_BIN" "$DST/Guest/sel4.bin"
cp_file "$GUEST_RTTHREAD_BIN" "$DST/Guest/rtthread.bin"
cp_file "$GUEST_NPUCORE_BIN" "$DST/Guest/NPUCore.bin"

# Copy HighSpeedCProxy-la
HIGHSPEEDCPROXY_SRC=$BASE/HighSpeedCProxy-la
if [ -d "$HIGHSPEEDCPROXY_SRC" ]; then
    echo "Copying HighSpeedCProxy-la..."
    rsync -av --no-perms --no-owner --no-group "$HIGHSPEEDCPROXY_SRC/" "$DST/HighSpeedCProxy-la/"
else
    warn "not found, skipping: $HIGHSPEEDCPROXY_SRC"
fi

sync
echo "Transfer done: $DST"