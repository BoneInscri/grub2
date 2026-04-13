#!/bin/bash
# Quick smoke test: send an echo request and wait for the response.
# Requires hyperamp_backend to be running in another terminal.
# Usage: bash test_echo.sh

SCRIPT_DIR=$(dirname "$(realpath "$0")")
SHM_JSON="$SCRIPT_DIR/../examples/zone2_shm.json"

echo "=== HyperAMP Echo Test ==="
hyperamp_linux -j "$SHM_JSON" -p "hello-from-linux" -w
