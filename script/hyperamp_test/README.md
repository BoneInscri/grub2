# HyperAMP Test Scripts

Test scripts for the HyperAMP shared-memory communication between Linux (zone0) and seL4 (zone2).

## Files

| File | Description |
|------|-------------|
| `run_backend.sh` | Start the backend proxy (persistent, polls RX queue) |
| `run_linux_client.sh` | One-shot Linux client wrapper (passes args to hyperamp_linux) |
| `test_echo.sh` | Quick echo smoke test |

SHM address config is read from `../examples/zone2_shm.json`.

## Typical Workflow

**Terminal 1** — start seL4 zone first, then run the backend:
```bash
bash run_backend.sh
```

**Terminal 2** — send a test message:
```bash
bash test_echo.sh
# or manually:
bash run_linux_client.sh -s "hello" -w
bash run_linux_client.sh -p "ping"  -w        # echo service
bash run_linux_client.sh -e @plain.txt -o enc.bin -w -B  # bulk encrypt
```

## Updating Physical Addresses

Edit `../examples/zone2_shm.json` to match the actual physical addresses allocated
by hvisor for the seL4 zone. The `zone0_ram_ipa` field is the address Linux uses for mmap.
