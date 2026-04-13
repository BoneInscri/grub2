#!/bin/bash
# Run hyperamp_linux as a one-shot client.
# Usage: bash run_linux_client.sh [options]
#
# Examples:
#   bash run_linux_client.sh -s "hello"          # send a data message
#   bash run_linux_client.sh -p "ping" -w        # echo service, wait for reply
#   bash run_linux_client.sh -e @file.txt -o out.bin -w -B  # bulk encrypt
#   bash run_linux_client.sh -r                  # receive pending messages
#   bash run_linux_client.sh -t                  # interactive mode

SCRIPT_DIR=$(dirname "$(realpath "$0")")
SHM_JSON="$SCRIPT_DIR/../examples/zone2_shm.json"

if [ ! -f "$SHM_JSON" ]; then
    echo "Error: zone2_shm.json not found: $SHM_JSON"
    exit 1
fi

hyperamp_linux -j "$SHM_JSON" "$@"
