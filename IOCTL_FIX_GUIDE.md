# IOCTL Fix Guide - MD5 Hash Login Issue

## Problem Summary

Users were experiencing login failures with the error:
```
Failed to hash password for user alice
MD5 hash ioctl failed: Invalid argument
Authentication failed for user alice
```

This document explains what went wrong, how it was fixed, and how to prevent similar issues in the future.

---

## Root Cause Analysis

### The Mismatch

The chat server (chat_server.c) was using **v1.0 API** IOCTL command numbers while the kernel driver (driver/crypto_driver.c) was using **v2.0 API** with different command numbers.

### Version History

**v1.0 Driver (Original):**
```c
#define CRYPTO_MD5_HASH    _IOWR('c', 1, struct crypto_data)
#define CRYPTO_AES_ENCRYPT _IOWR('c', 2, struct crypto_data)
#define CRYPTO_AES_DECRYPT _IOWR('c', 3, struct crypto_data)
```

**v2.0 Driver (Enhanced with IV support):**
```c
#define IOCTL_SET_KEY   _IOW('c', 1, unsigned char[16])
#define IOCTL_SET_IV    _IOW('c', 2, unsigned char[16])
#define IOCTL_GET_IV    _IOR('c', 3, unsigned char[16])
#define IOCTL_ENCRYPT   _IOWR('c', 4, struct crypto_data)
#define IOCTL_DECRYPT   _IOWR('c', 5, struct crypto_data)
#define IOCTL_MD5_HASH  _IOWR('c', 6, struct crypto_data)
```

### What Happened

1. Server sends IOCTL with command number **1** expecting MD5 hash
2. Driver receives command **1** and interprets it as SET_KEY
3. Driver tries to process SET_KEY with crypto_data structure (wrong type!)
4. Driver returns **EINVAL** (Invalid argument)
5. Server prints "MD5 hash ioctl failed: Invalid argument"
6. Authentication fails

### Command Number Mapping

| Operation | Server v1.0 | Driver v2.0 | Result |
|-----------|-------------|-------------|---------|
| MD5 Hash  | Command 1   | Command 6   | ❌ Mismatch! |
| Encrypt   | Command 2   | Command 4   | ❌ Mismatch! |
| Decrypt   | Command 3   | Command 5   | ❌ Mismatch! |

---

## The Fix

### Solution Overview

Instead of duplicating IOCTL definitions in each file, use a **single common header** file that both the driver and user-space programs include.

### File Structure

```
common/
└── crypto_user.h       ← Single source of truth

driver/
└── crypto_driver.c     ← Includes common header

chat_server.c           ← Includes common header
chat_client.c           ← No crypto, no changes needed
```

### Changes Made

#### chat_server.c

**Before:**
```c
/* Duplicate definitions (wrong!) */
#define CRYPTO_IOC_MAGIC 'c'
#define CRYPTO_MD5_HASH _IOWR(CRYPTO_IOC_MAGIC, 1, struct crypto_data)
#define CRYPTO_AES_ENCRYPT _IOWR(CRYPTO_IOC_MAGIC, 2, struct crypto_data)
#define CRYPTO_AES_DECRYPT _IOWR(CRYPTO_IOC_MAGIC, 3, struct crypto_data)

struct crypto_data {
    unsigned char input[MAX_DATA_SIZE];
    unsigned char output[MAX_DATA_SIZE];
    unsigned char key[AES_KEY_SIZE];
    unsigned int input_len;
    unsigned int output_len;
};

// Later in code:
ioctl(crypto_fd, CRYPTO_MD5_HASH, &data);  // Command 1 - Wrong!
```

**After:**
```c
/* Include common header */
#include "common/crypto_user.h"

/* No duplicate definitions needed */

// Later in code:
ioctl(crypto_fd, IOCTL_MD5_HASH, &data);  // Command 6 - Correct!
```

---

## Verification Steps

### 1. Build Test
```bash
make clean && make
```

**Expected:** No errors or warnings

### 2. Module Test
```bash
sudo make driver-load
./setup.sh status
```

**Expected:** Module loaded successfully

### 3. Server Test
```bash
./chat_server
```

**Expected Output:**
```
Chat server started on port 8888
Waiting for clients...
Available users: alice/password123, bob/password456, charlie/password789
```

**Look for:** No "MD5 hash ioctl failed" errors

### 4. Login Test (Terminal)
```bash
./chat_client
```

Enter:
- Username: alice
- Password: password123

**Expected:** Login successful, can send messages

### 5. Login Test (Web UI)
```bash
make run-web
```

Open http://localhost:5000

Login with: alice / password123

**Expected:** Chat interface appears

---

## Understanding IOCTL Commands

### What is an IOCTL?

IOCTL (Input/Output Control) is a system call used for device-specific operations. Each IOCTL has a unique command number that identifies the operation.

### IOCTL Command Structure

```c
#define IOCTL_NAME  _IOWR(magic, number, type)
```

Where:
- `magic`: Magic number (character) to identify the driver
- `number`: Command number (unique within the driver)
- `type`: Data type being passed
- `_IOWR`: Indicates read+write operation

### Why Command Numbers Matter

The kernel uses the command number to determine which operation to perform. If the numbers don't match between user-space and kernel-space, the wrong operation is executed or the command is rejected.

---

## Best Practices

### 1. Single Source of Truth

✅ **DO:** Define IOCTLs in a common header file
```c
common/crypto_user.h  ← All IOCTL definitions here
```

❌ **DON'T:** Duplicate definitions in multiple files
```c
driver.c   ← Definition 1
server.c   ← Definition 2 (gets out of sync!)
client.c   ← Definition 3 (even worse!)
```

### 2. Version Compatibility

When enhancing a driver:
- Keep old command numbers reserved
- Add new commands with new numbers
- Document version changes

**Example:**
```c
/* v1.0 commands (deprecated but reserved) */
// #define IOCTL_OLD_ENCRYPT _IOWR('c', 2, ...)  // Reserved

/* v2.0 commands */
#define IOCTL_SET_KEY    _IOW('c', 1, ...)
#define IOCTL_ENCRYPT    _IOWR('c', 4, ...)  // New number!
```

### 3. Structure Compatibility

Ensure structures match between user-space and kernel-space:

```c
/* User-space: chat_server.c */
struct crypto_data {
    unsigned char input[4096];
    unsigned char output[4096];
    unsigned char key[16];
    unsigned char iv[16];      // Must match driver!
    unsigned int input_len;
    unsigned int output_len;
};

/* Kernel-space: crypto_driver.c */
struct crypto_data {
    unsigned char input[4096];
    unsigned char output[4096];
    unsigned char key[16];
    unsigned char iv[16];      // Must match user-space!
    unsigned int input_len;
    unsigned int output_len;
};
```

### 4. Testing Strategy

After any IOCTL changes:
1. ✅ Build both kernel module and user-space programs
2. ✅ Test each IOCTL command individually
3. ✅ Test with valid and invalid parameters
4. ✅ Check error messages are clear
5. ✅ Test the full workflow (e.g., login)

---

## Troubleshooting

### Error: "Invalid argument"

**Symptoms:**
```
MD5 hash ioctl failed: Invalid argument
```

**Possible Causes:**
1. IOCTL command number mismatch
2. Structure mismatch
3. Module not loaded
4. Wrong device file

**Solutions:**
1. Verify common header is included: `#include "common/crypto_user.h"`
2. Check IOCTL calls use correct command names (e.g., `IOCTL_MD5_HASH`)
3. Ensure module is loaded: `lsmod | grep crypto_driver`
4. Check device exists: `ls -l /dev/crypto_dev`

### Error: "No such device"

**Symptoms:**
```
Failed to open crypto device: No such file or directory
```

**Solution:**
```bash
sudo make driver-load
```

### Error: "Permission denied"

**Symptoms:**
```
Failed to open crypto device: Permission denied
```

**Solution:**
```bash
sudo chmod 666 /dev/crypto_dev
```

---

## Technical Details

### IOCTL Number Encoding

The `_IOWR()` macro encodes the command number with additional information:

```
Command = (direction << 30) | (magic << 8) | (number) | (size << 16)
```

Even if two commands have the same `number`, they can have different encoded values if:
- Direction differs (_IOR vs _IOW vs _IOWR)
- Type size differs
- Magic number differs

### Why v2.0 Changed Command Numbers

The v2.0 driver added session management with separate key/IV operations:
- Added `IOCTL_SET_KEY` (command 1)
- Added `IOCTL_SET_IV` (command 2)
- Added `IOCTL_GET_IV` (command 3)
- Moved `IOCTL_ENCRYPT` to command 4
- Moved `IOCTL_DECRYPT` to command 5
- Moved `IOCTL_MD5_HASH` to command 6

This allows more flexible usage patterns while maintaining compatibility.

---

## Summary

### The Problem
- IOCTL command number mismatch between server and driver
- Server expected v1.0 API, driver provided v2.0 API
- Authentication failed due to MD5 hash ioctl errors

### The Solution
- Use common header file (common/crypto_user.h)
- Single source of truth for all IOCTL definitions
- Server updated to use v2.0 API

### The Result
- ✅ MD5 hashing works
- ✅ Authentication succeeds
- ✅ Login functional (terminal and web)
- ✅ System fully operational

### Lessons Learned
1. Always use common headers for driver interfaces
2. Version your APIs and document changes
3. Test after every API change
4. Clear error messages save debugging time

---

## Additional Resources

- `common/crypto_user.h` - IOCTL interface definitions
- `driver/crypto_driver.c` - Kernel driver implementation
- `chat_server.c` - Server using crypto driver
- `TROUBLESHOOTING.md` - General troubleshooting guide
- `BUILD_FIX_SUMMARY.md` - Build issues guide

---

**Document Version:** 1.0  
**Last Updated:** 2026-02-01  
**Status:** Issue Resolved ✅
