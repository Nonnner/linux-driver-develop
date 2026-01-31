# Quick Start Guide

## Prerequisites

Before building and running the chat system, ensure you have:

```bash
# Install kernel headers for building kernel modules
sudo apt-get update
sudo apt-get install linux-headers-$(uname -r) build-essential gcc make

# Verify kernel headers are installed
ls /lib/modules/$(uname -r)/build
```

## Build Instructions

### 1. Build User Space Programs

```bash
# Build chat server and client
make

# You should see:
# gcc -Wall -Wextra -pthread -O2 -o chat_server chat_server.c -pthread
# gcc -Wall -Wextra -pthread -O2 -o chat_client chat_client.c -pthread
```

### 2. Build Kernel Module

```bash
# Build the crypto driver
make -f Makefile.driver

# You should see kernel module compilation output
# The result will be: crypto_driver.ko
```

## Installation

### 1. Load the Kernel Module

```bash
# Insert the module
sudo insmod crypto_driver.ko

# Verify it's loaded
lsmod | grep crypto_driver

# Check kernel messages
dmesg | tail -20

# You should see:
# crypto_dev: Initializing crypto driver
# crypto_dev: Registered with major number XXX
# crypto_dev: Device class registered
# crypto_dev: Device created successfully

# Verify device was created
ls -l /dev/crypto_dev

# Set permissions (if needed)
sudo chmod 666 /dev/crypto_dev
```

## Running the System

### Terminal 1: Start the Server

```bash
./chat_server
```

Expected output:
```
Crypto driver opened successfully
User alice initialized with hashed password
User bob initialized with hashed password
User charlie initialized with hashed password
Chat server started on port 8888
Waiting for clients...
Available users: alice/password123, bob/password456, charlie/password789
```

### Terminal 2: Connect as Alice

```bash
./chat_client
```

When prompted:
```
USERNAME: alice
PASSWORD: password123
AUTH_SUCCESS
Authentication successful!
```

Now Alice can type messages!

### Terminal 3: Connect as Bob

```bash
./chat_client
```

When prompted:
```
USERNAME: bob
PASSWORD: password456
AUTH_SUCCESS
Authentication successful!
```

### Terminal 4: Connect as Charlie

```bash
./chat_client
```

When prompted:
```
USERNAME: charlie
PASSWORD: password789
AUTH_SUCCESS
Authentication successful!
```

## Usage Example

**Alice types:** `Hello everyone!`

**Bob and Charlie see:**
```
[alice] Hello everyone!
```

**Bob types:** `Hi Alice! How are you?`

**Alice and Charlie see:**
```
[bob] Hi Alice! How are you?
```

**Charlie types:** `Hey guys!`

**Alice and Bob see:**
```
[charlie] Hey guys!
```

## Testing Scenarios

### Test 1: Authentication Failure

```bash
./chat_client
USERNAME: alice
PASSWORD: wrongpassword
AUTH_FAILED
Authentication failed. Disconnecting...
```

### Test 2: Join/Leave Notifications

When a new user joins, all existing users see:
```
[SERVER] alice has joined the chat
```

When a user leaves (types `quit` or disconnects):
```
[SERVER] alice has left the chat
```

### Test 3: Multiple Messages

Users can send messages continuously. All authenticated users receive all messages in real-time.

## Stopping the System

### Stop Clients

In any client terminal, type:
```
quit
```
or press `Ctrl+C`

### Stop Server

In the server terminal, press `Ctrl+C`

### Unload Kernel Module

```bash
# Remove the module
sudo rmmod crypto_driver

# Verify it's unloaded
lsmod | grep crypto_driver

# Check kernel messages
dmesg | tail -5
# Should see: crypto_dev: Driver unloaded
```

## Troubleshooting

### Problem: "Failed to open crypto device"

**Solution:**
```bash
# Check if module is loaded
lsmod | grep crypto_driver

# If not loaded, insert it
sudo insmod crypto_driver.ko

# Check if device exists
ls -l /dev/crypto_dev

# Fix permissions if needed
sudo chmod 666 /dev/crypto_dev
```

### Problem: "Connection refused" when running client

**Solution:**
```bash
# Check if server is running
ps aux | grep chat_server

# Check if port 8888 is in use
netstat -tuln | grep 8888

# Start the server if not running
./chat_server
```

### Problem: Module compilation errors

**Solution:**
```bash
# Install kernel headers
sudo apt-get install linux-headers-$(uname -r)

# Clean and rebuild
make -f Makefile.driver clean
make -f Makefile.driver
```

### Problem: "Operation not permitted" when loading module

**Solution:**
```bash
# Use sudo to load the module
sudo insmod crypto_driver.ko

# Or run as root
sudo -i
insmod crypto_driver.ko
```

## Advanced Usage

### Connect to Remote Server

```bash
# On client machine, specify server IP
./chat_client 192.168.1.100
```

### Check Crypto Operations in Kernel

```bash
# Monitor kernel messages in real-time
sudo dmesg -w

# In another terminal, run the chat system
# You'll see crypto operations logged:
# crypto_dev: Device opened
# crypto_dev: MD5 hash request
# crypto_dev: Device closed
```

### View Active Connections

```bash
# On server machine
netstat -an | grep 8888

# Or use ss
ss -tn | grep 8888
```

## Clean Up

### Remove Build Artifacts

```bash
# Clean user space programs
make clean

# Clean kernel module
make -f Makefile.driver clean

# Remove all binaries
rm -f chat_server chat_client *.ko *.o *.mod* .*.cmd modules.order Module.symvers
rm -rf .tmp_versions
```

## Notes

- The system uses hardcoded user credentials for demonstration
- Default port is 8888 (can be changed in source code)
- Maximum 100 concurrent clients (configurable)
- Messages are broadcast to all authenticated users
- The crypto driver provides MD5 hashing and AES encryption/decryption

## Next Steps

After successfully running the basic system, you can:

1. Read DOCUMENTATION.md for detailed system information
2. Read ARCHITECTURE.md for technical details
3. Modify the code to add new features
4. Experiment with the crypto operations
5. Add more users to the user database

## Support

For issues or questions:
1. Check the troubleshooting section
2. Review kernel logs: `dmesg | grep crypto_dev`
3. Check server logs in the server terminal
4. Verify network connectivity

Enjoy your secure multi-user chat system!
