# MD5 IOCTL "Invalid Argument" Error - Complete Debug Guide

## Problem

Users experiencing authentication failures with error:
```
MD5 hash ioctl failed: Invalid argument
Failed to hash password for user alice
Authentication failed for user alice
```

## Root Cause

The "Invalid argument" error from an IOCTL call typically means one of:

1. **IOCTL command number mismatch** - User space sends command X, kernel expects command Y
2. **Structure size mismatch** - The `_IOWR` macro encodes the struct size in the command
3. **Old kernel module loaded** - v1.0 module loaded but v2.0 code running
4. **Module not rebuilt** - Code changed but module not recompiled

## Quick Fix

**Most Common Solution: Rebuild and Reload Module**

```bash
# 1. Clean and rebuild everything
make clean
make
make driver-build

# 2. Reload the kernel module
sudo make driver-reload

# 3. Test
./chat_server
```

If that doesn't work, continue with the diagnostic steps below.

## Diagnostic Steps

### Step 1: Check Module Version

Run the diagnostic script:
```bash
./check_module_version.sh
```

This will tell you:
- Is the module loaded?
- What version is loaded?
- Is the device created?
- Is source newer than module?
- What IOCTL command number is expected?

### Step 2: Verify Module is v2.0

Check for enhanced driver:
```bash
# Check module description
modinfo crypto_driver | grep -i description

# Should show: "Enhanced Crypto Character Device Driver with AES-CBC and MD5"
```

If it says just "Crypto Character Device Driver" (without "Enhanced"), you have v1.0 loaded!

### Step 3: Check IOCTL Command Numbers

**User Space (expected):**
```c
// From common/crypto_user.h
#define IOCTL_MD5_HASH _IOWR(CRYPTO_IOC_MAGIC, 6, struct crypto_data)
// Command number: 0xe0286306 (with struct size 8232)
```

**Kernel Space (must match):**
```c
// From driver/crypto_driver.c
#define IOCTL_MD5_HASH _IOWR(CRYPTO_IOC_MAGIC, 6, struct crypto_data)
// Same command number: 0xe0286306
```

If the numbers don't match, you have a version mismatch!

### Step 4: Verify Struct Definition

**User Space:**
```c
// From common/crypto_user.h
struct crypto_data {
    unsigned char input[4096];
    unsigned char output[4096];
    unsigned char key[16];
    unsigned char iv[16];        // ← Must have iv field!
    unsigned int input_len;
    unsigned int output_len;
};
// Size: 8232 bytes
```

**Kernel Space:**
```c
// From driver/crypto_driver.c
struct crypto_data {
    unsigned char input[4096];
    unsigned char output[4096];
    unsigned char key[16];
    unsigned char iv[16];        // ← Must match!
    unsigned int input_len;
    unsigned int output_len;
};
// Size: 8232 bytes (must be identical!)
```

If v1.0 is loaded (no `iv` field), struct size is 8216 bytes → command number mismatch!

## Step-by-Step Fix Process

### Fix 1: Complete Rebuild and Reload

```bash
# Stop server if running
pkill chat_server

# Clean everything
make clean
make -f Makefile.driver clean

# Rebuild everything
make                    # Build user space programs
make driver-build       # Build kernel module

# Unload old module (if loaded)
sudo rmmod crypto_driver 2>/dev/null || true

# Load new module
sudo insmod driver/crypto_driver.ko
# or
sudo insmod crypto_driver.ko

# Set permissions
sudo chmod 666 /dev/crypto_dev

# Verify
lsmod | grep crypto
ls -l /dev/crypto_dev

# Test
./chat_server
```

### Fix 2: If Module Won't Unload

If you get "Module is in use" error:

```bash
# Find what's using it
lsof /dev/crypto_dev

# Kill those processes
pkill chat_server
pkill chat_backend

# Try again
sudo rmmod crypto_driver
```

### Fix 3: If Device Not Created

```bash
# Check dmesg for errors
sudo dmesg | tail -20

# Look for:
# - "crypto_dev: Device /dev/crypto_dev created"
# - Any error messages

# Manually create if needed (shouldn't be necessary)
sudo mknod /dev/crypto_dev c $(cat /proc/devices | grep crypto_dev | awk '{print $1}') 0
sudo chmod 666 /dev/crypto_dev
```

## Understanding the Error

### Why "Invalid Argument"?

The kernel's IOCTL handler checks if the received command matches any expected commands:

```c
switch (cmd) {
case IOCTL_SET_KEY:     // 0x40106301
    ...
case IOCTL_MD5_HASH:    // 0xe0286306
    ...
default:
    return -EINVAL;     // ← "Invalid argument" error
}
```

If user space sends `0xe0206301` (wrong size/command), it doesn't match, returns EINVAL.

### IOCTL Command Encoding

The `_IOWR` macro encodes:
```
Bits:  Dir(2) | Size(14) | Type(8) | Nr(8)
Example: 11 | 2028 (0x820) | 'c' (0x63) | 6
Result: 0xe0286306
```

If struct size changes (adding/removing `iv` field), the command number changes!

### Version Compatibility

| Version | IOCTL Commands | struct crypto_data | MD5 Command |
|---------|----------------|-------------------|-------------|
| v1.0    | Commands 1-3   | No `iv` field (8216 bytes) | Command 1 (0xc020...) |
| v2.0    | Commands 1-6   | Has `iv` field (8232 bytes) | Command 6 (0xe028...) |

**Mixing versions causes EINVAL!**

## Prevention

### 1. Always Use Common Header

**GOOD:**
```c
#include "common/crypto_user.h"  // Single source of truth
```

**BAD:**
```c
#define CRYPTO_MD5_HASH ...      // Duplicate definition - can get out of sync
```

### 2. Rebuild and Reload After Changes

```bash
# After ANY change to driver code:
make driver-build
sudo make driver-reload

# After ANY change to server code:
make
```

### 3. Check Version Match

```bash
# Before running:
./check_module_version.sh
```

## Testing the Fix

### Test 1: Module Loads

```bash
$ sudo make driver-load
Building kernel module...
  CC [M]  driver/crypto_driver.o
  LD [M]  driver/crypto_driver.ko
Module built successfully: crypto_driver.ko
Loading module...
Module loaded successfully

$ lsmod | grep crypto
crypto_driver          16384  0
```

### Test 2: Device Exists

```bash
$ ls -l /dev/crypto_dev
crw-rw-rw- 1 root root 237, 0 Feb  1 08:00 /dev/crypto_dev
```

### Test 3: Server Starts

```bash
$ ./chat_server
Chat server started on port 8888
Waiting for clients...
Available users: alice/password123, bob/password456, charlie/password789
```

**No "Failed to hash password" errors!**

### Test 4: Authentication Works

```bash
$ ./chat_client
Enter username: alice
Enter password: password123
✓ User alice authenticated successfully
```

**Authentication succeeds!**

## If Problem Persists

### 1. Enable Debug Logging

Add debug output to chat_server.c:

```c
int md5_hash(const unsigned char *input, unsigned int len, unsigned char *output)
{
    struct crypto_data data;
    memset(&data, 0, sizeof(data));
    
    fprintf(stderr, "DEBUG: md5_hash called, len=%u, sizeof(data)=%lu\n", 
            len, sizeof(data));
    fprintf(stderr, "DEBUG: IOCTL_MD5_HASH = 0x%lx\n", (unsigned long)IOCTL_MD5_HASH);
    
    // ... rest of function
}
```

### 2. Check Kernel Logs

```bash
# Watch kernel messages in real-time
sudo dmesg -w

# Or check recent messages
sudo dmesg | tail -50
```

Look for messages from crypto_driver showing what command it received.

### 3. Verify Source Files

```bash
# Check chat_server.c includes correct header
grep "crypto_user.h" chat_server.c

# Should show:
# #include "common/crypto_user.h"
```

### 4. Check Compilation

```bash
# Verify no old object files
make clean
rm -f *.o

# Fresh compile with verbose output
make VERBOSE=1
```

## Summary

**The fix is usually:**

1. ✅ Code already updated (commit 44aecc0)
2. ✅ Just need to rebuild and reload module
3. ✅ Module wasn't reloaded after code change

**Simple solution:**
```bash
make driver-build
sudo make driver-reload
./chat_server
```

**If that doesn't work:**
- Use `./check_module_version.sh` to diagnose
- Check for version mismatch
- Ensure v2.0 module is loaded
- Verify struct definitions match

---

**After following this guide, authentication should work correctly!**
