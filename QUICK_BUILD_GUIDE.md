# Quick Reference - Build and Run

## The Problem (FIXED ✅)
```
insmod: ERROR: could not load module crypto_driver.ko: No such file or directory
```

## The Solution
The build system has been fixed! Follow these steps:

## Quick Start

### 1. Build Everything
```bash
make                # Build chat server and client
make driver-build   # Build kernel module
```

Expected output:
```
gcc -Wall -Wextra -pthread -O2 -o chat_server chat_server.c -pthread
gcc -Wall -Wextra -pthread -O2 -o chat_client chat_client.c -pthread
Building kernel module from driver/ directory...
Module built successfully: crypto_driver.ko
```

### 2. Load Kernel Module
```bash
sudo make driver-load
# or
sudo ./setup.sh load
```

### 3. Run the Chat System
```bash
# Terminal 1 - Start server
./chat_server

# Terminal 2 - Start client
./chat_client
```

### 4. (Optional) Web UI
```bash
# Terminal 1
./chat_server

# Terminal 2
make backend
make run-web

# Browser: http://localhost:5000
```

## Common Commands

### Build
```bash
make                # Build user programs
make driver-build   # Build kernel module
make clean          # Clean user programs
make driver-clean   # Clean kernel module
```

### Module Management
```bash
sudo ./setup.sh load      # Load module
sudo ./setup.sh unload    # Unload module
sudo ./setup.sh reload    # Reload module
./setup.sh status         # Check status (no sudo)
```

### Status Check
```bash
make driver-status
# or
./setup.sh status
```

Expected output:
```
✓ Module file: crypto_driver.ko exists
✓ Module: Loaded
✓ Device: /dev/crypto_dev exists
```

## Troubleshooting

### Module won't load?
```bash
# Check if kernel headers are installed
ls /lib/modules/$(uname -r)/build

# If missing, install them
sudo apt install linux-headers-$(uname -r)

# Rebuild
make driver-clean && make driver-build
```

### Device not found?
```bash
# Check module status
lsmod | grep crypto_driver

# Check device
ls -l /dev/crypto_dev

# Reload module
sudo ./setup.sh reload
```

### Build fails?
```bash
# Clean everything
make clean
make driver-clean

# Rebuild
make && make driver-build
```

## What Was Fixed

1. ✅ **Build Directory** - Now builds from driver/ subdirectory
2. ✅ **Kernel API** - Compatible with kernel 6.4+ and older
3. ✅ **File Organization** - Removed duplicate source files
4. ✅ **Status Messages** - Clear build feedback

## Documentation

- **BUILD_FIX_SUMMARY.md** - Detailed fix documentation
- **TROUBLESHOOTING.md** - Comprehensive troubleshooting
- **QUICKSTART.md** - Full quick start guide
- **README.md** - Main documentation
- **WEB_UI_README.md** - Web UI documentation

## Help

```bash
make help              # Show all make targets
./setup.sh help        # Show module management help
```

---
**Status:** ✅ Build issues resolved - System ready!
