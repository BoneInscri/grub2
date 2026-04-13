#!/bin/bash
# Run hyperamp_backend (seL4 backend proxy simulator).
# This process runs persistently, polling the RX queue for requests from seL4.
# Usage: bash run_backend.sh [shm_json]
#
# Must be run AFTER seL4 zone is started and queues are initialized.

set -e

SCRIPT_DIR=$(dirname "$(realpath "$0")")
SHM_JSON="${1:-$SCRIPT_DIR/../examples/zone2_shm.json}"

if [ ! -f "$SHM_JSON" ]; then
    echo "Error: zone2_shm.json not found: $SHM_JSON"
    echo "Usage: bash run_backend.sh [shm_json_path]"
    exit 1
fi

echo "========================================="
echo "  HyperAMP Backend Proxy"
echo "========================================="
echo "  SHM config: $SHM_JSON"
echo ""
echo "  Press Ctrl+C to stop."
echo ""

hyperamp_backend -j "$SHM_JSON"
