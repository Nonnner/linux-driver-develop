# Troubleshooting Guide

## Common Issues and Solutions

This guide covers common problems you might encounter when setting up and running the Multi-User Chat System.

---

## 1. "Failed to open crypto device: No such file or directory"

### Problem
When running `./chat_server`, you get:
```
Failed to open crypto device: No such file or directory
ERROR: Crypto device /dev/crypto_dev not found!
```

### Cause
The kernel module (`crypto_driver.ko`) has not been loaded.

### Solution

**Option 1: Use the setup script (Recommended)**
```bash
sudo ./setup.sh load
```

**Option 2: Manual steps**
```bash
# 1. Build the kernel module (if not already built)
make -f Makefile.driver

# 2. Load the kernel module
sudo insmod crypto_driver.ko

# 3. Set device permissions
sudo chmod 666 /dev/crypto_dev

# 4. Verify the module is loaded
lsmod | grep crypto_driver
```

**Option 3: Use Makefile targets**
```bash
make driver-build    # Build the module
make driver-load     # Load the module (uses sudo)
```

### Verification
```bash
# Check if device exists
ls -l /dev/crypto_dev

# Check if module is loaded
lsmod | grep crypto_driver

# View kernel messages
dmesg | tail -20
```

---

## 2. "MD5 hash ioctl failed: Invalid argument" - Authentication Fails

### Problem
Server starts but authentication always fails with:
```
MD5 hash ioctl failed: Invalid argument
Failed to hash password for user alice
Authentication failed for user alice
```

### Cause
**Most Common:** Old kernel module (v1.0) is loaded but you're running v2.0 code.

The IOCTL command numbers changed between versions:
- v1.0: MD5 is command 1, struct size 8216 bytes
- v2.0: MD5 is command 6, struct size 8232 bytes

When versions don't match, kernel returns "Invalid argument".

### Quick Fix

**Solution: Rebuild and reload the module**

```bash
# 1. Clean and rebuild
make clean
make
make driver-build

# 2. Reload the module
sudo make driver-reload

# 3. Test
./chat_server
```

### Diagnostic Tool

Run the diagnostic script for detailed analysis:

```bash
./check_module_version.sh
```

This will tell you:
- Module version loaded
- If source is newer than module
- What IOCTL commands are expected
- Exact fix steps

### Complete Debug Guide

For comprehensive troubleshooting, see:
- **MD5_IOCTL_DEBUG_GUIDE.md** - Complete debugging guide with:
  - Root cause explanation
  - IOCTL encoding details
  - Version compatibility table
  - Step-by-step fixes
  - Prevention strategies

### Verification

After fix, you should see:
```bash
$ ./chat_server
Chat server started on port 8888
Waiting for clients...
Available users: alice/password123, bob/password456, charlie/password789
# No "Failed to hash password" errors!
```

And authentication should work:
```bash
$ ./chat_client
Enter username: alice
Enter password: password123
✓ User alice authenticated successfully
```

---

## 3. Module Compilation Errors

### Problem
When running `make -f Makefile.driver`, you get compilation errors like:
```
fatal error: linux/module.h: No such file or directory
```

### Cause
Kernel headers are not installed.

### Solution
```bash
# Install kernel headers
sudo apt-get update
sudo apt-get install linux-headers-$(uname -r) build-essential

# Verify installation
ls /lib/modules/$(uname -r)/build

# Try building again
make -f Makefile.driver
```

### Common compilation issues:

**Issue: "No rule to make target"**
```bash
# Make sure you're in the correct directory
cd /path/to/linux-driver-develop

# Clean and rebuild
make -f Makefile.driver clean
make -f Makefile.driver
```

**Issue: Version mismatch**
```bash
# Your kernel might be newer than headers
uname -r                    # Check running kernel
dpkg -l | grep linux-headers # Check installed headers

# Install matching headers
sudo apt-get install linux-headers-$(uname -r)
```

---

## 14. "Operation not permitted" when loading module

### Problem
```
sudo insmod crypto_driver.ko
insmod: ERROR: could not insert module crypto_driver.ko: Operation not permitted
```

### Possible Causes and Solutions

#### Cause 1: Secure Boot is enabled
```bash
# Check if Secure Boot is enabled
mokutil --sb-state

# If enabled, you have two options:

# Option A: Disable Secure Boot in BIOS/UEFI
# (Reboot and enter BIOS settings)

# Option B: Sign the kernel module
# (More complex, see kernel documentation)
```

#### Cause 2: Module already loaded
```bash
# Check if module is already loaded
lsmod | grep crypto_driver

# If loaded, unload it first
sudo rmmod crypto_driver

# Then load again
sudo insmod crypto_driver.ko
```

#### Cause 3: Insufficient privileges
```bash
# Make sure you're using sudo
sudo insmod crypto_driver.ko

# Or run as root
sudo -i
insmod crypto_driver.ko
```

---

## 14. "Connection refused" when running client

### Problem
```
./chat_client
Connecting to server at 127.0.0.1:8888...
Connection failed: Connection refused
```

### Cause
The chat server is not running.

### Solution
```bash
# Check if server is running
ps aux | grep chat_server

# Check if port 8888 is listening
netstat -tuln | grep 8888
# or
ss -tuln | grep 8888

# Start the server if not running
./chat_server
```

### If port is already in use:
```bash
# Find what's using port 8888
sudo lsof -i :8888

# Kill the process (replace PID with actual process ID)
kill -9 <PID>

# Or use a different port (requires modifying source code)
```

---

## 14. "Device or resource busy" when unloading module

### Problem
```
sudo rmmod crypto_driver
rmmod: ERROR: Module crypto_driver is in use
```

### Cause
The chat server or other processes are still using the device.

### Solution
```bash
# 1. Stop the chat server
# Press Ctrl+C in the server terminal
# or
killall chat_server

# 2. Wait a moment for cleanup
sleep 2

# 3. Try unloading again
sudo rmmod crypto_driver

# 4. If still failing, force unload (use with caution)
sudo rmmod -f crypto_driver
```

---

## 14. Authentication Failures

### Problem
```
USERNAME: alice
PASSWORD: password123
AUTH_FAILED
Authentication failed. Disconnecting...
```

### Cause
Incorrect username or password.

### Solution
Use the correct demo credentials:

| Username | Password     |
|----------|--------------|
| alice    | password123  |
| bob      | password456  |
| charlie  | password789  |

**Note:** Usernames and passwords are case-sensitive!

---

## 14. Messages not appearing in clients

### Problem
Client is connected and authenticated, but messages from other users don't appear.

### Possible Causes and Solutions

#### Cause 1: Client not properly authenticated
```bash
# Make sure you see "AUTH_SUCCESS" after login
# If you see "AUTH_FAILED", check your credentials
```

#### Cause 2: Network issues
```bash
# Check if clients are connected
# On server machine:
netstat -an | grep 8888

# You should see multiple ESTABLISHED connections
```

#### Cause 3: Server crashed
```bash
# Check if server is still running
ps aux | grep chat_server

# Check server terminal for error messages

# Restart server if needed
./chat_server
```

---

## 14. Permission denied on /dev/crypto_dev

### Problem
```
Failed to open crypto device: Permission denied
```

### Cause
Device permissions are too restrictive.

### Solution
```bash
# Set device permissions
sudo chmod 666 /dev/crypto_dev

# Verify permissions
ls -l /dev/crypto_dev
# Should show: crw-rw-rw- ...

# Alternative: Add user to device group
sudo chgrp <your-group> /dev/crypto_dev
sudo chmod 660 /dev/crypto_dev
```

---

## 14. Web UI Issues

### Problem: "Failed to connect to chat server" in web UI

### Solution
```bash
# 1. Make sure TCP chat server is running
./chat_server

# 2. Check if server is listening
netstat -tuln | grep 8888

# 3. Restart web backend
make run-web
```

### Problem: Python module not found

### Solution
```bash
# Install Python dependencies
make backend

# Or manually
pip3 install -r backend/requirements.txt
```

### Problem: Port 5000 already in use

### Solution
```bash
# Find what's using port 5000
sudo lsof -i :5000

# Kill the process
kill -9 <PID>

# Or modify backend/chat_backend.py to use different port
```

---

## 14. Kernel Messages and Debugging

### Viewing kernel messages
```bash
# View recent kernel messages
dmesg | tail -20

# Filter for crypto_dev messages
dmesg | grep crypto_dev

# Follow kernel messages in real-time
sudo dmesg -w
```

### Common kernel messages

**Success:**
```
crypto_dev: Initializing crypto driver
crypto_dev: Registered with major number 245
crypto_dev: Device class registered
crypto_dev: Device created successfully
```

**Module loaded:**
```
crypto_dev: Device opened
crypto_dev: MD5 hash request
crypto_dev: AES encrypt request
```

**Module unloaded:**
```
crypto_dev: Driver unloaded
```

---

## 14. Clean Start (Reset Everything)

If nothing else works, try a complete reset:

```bash
# 1. Stop all running processes
killall chat_server chat_client 2>/dev/null
pkill -f chat_backend.py 2>/dev/null

# 2. Unload kernel module
sudo rmmod crypto_driver 2>/dev/null

# 3. Clean all build artifacts
make clean
make -f Makefile.driver clean

# 4. Rebuild everything
make
make -f Makefile.driver

# 5. Load kernel module
sudo ./setup.sh load

# 6. Start server
./chat_server
```

---

## 14. Checking System Status

Use the setup script to check overall status:

```bash
./setup.sh status
```

Expected output when everything is working:
```
=== Crypto Driver Status ===

✓ Module file: crypto_driver.ko exists
✓ Module: Loaded
crypto_driver          16384  1
✓ Device: /dev/crypto_dev exists
crw-rw-rw- 1 root root 245, 0 Jan 31 10:00 /dev/crypto_dev
```

---

## 14. Getting Help

If you're still having issues:

1. **Check existing documentation:**
   - README.md - Main documentation
   - QUICKSTART.md - Quick start guide
   - ARCHITECTURE.md - Technical details
   - DOCUMENTATION.md - Comprehensive docs

2. **Verify your system:**
   ```bash
   # Kernel version
   uname -r
   
   # GCC version
   gcc --version
   
   # Make version
   make --version
   
   # Check if headers are installed
   ls /lib/modules/$(uname -r)/build
   ```

3. **Collect debugging information:**
   ```bash
   # System info
   uname -a
   
   # Module status
   ./setup.sh status
   
   # Kernel messages
   dmesg | grep crypto_dev > kernel_messages.txt
   
   # Network status
   netstat -tuln | grep 8888
   ```

4. **Review logs:**
   - Server terminal output
   - Client terminal output
   - Kernel messages (`dmesg`)
   - Build output

---

## Quick Reference Commands

```bash
# Build everything
make && make driver-build

# Load kernel module
make driver-load          # or: sudo ./setup.sh load

# Check status
make driver-status        # or: ./setup.sh status

# Start system
./chat_server            # Terminal 1
./chat_client            # Terminal 2

# Unload module
make driver-unload       # or: sudo ./setup.sh unload

# Clean everything
make clean
make driver-clean
```

---

## Reporting Issues

When reporting issues, please include:

1. Operating system and version (`uname -a`)
2. Kernel version (`uname -r`)
3. Output of `./setup.sh status`
4. Error messages from terminal
5. Relevant kernel messages (`dmesg | grep crypto_dev`)
6. Steps to reproduce the problem

---

**Remember:** Most issues are caused by forgetting to load the kernel module. Always check that first!

For more information, see:
- README.md - Project overview
- QUICKSTART.md - Getting started guide
- DOCUMENTATION.md - Detailed documentation
