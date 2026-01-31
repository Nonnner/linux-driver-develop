# Multi-User Chat System with Kernel Crypto Driver

## System Architecture

This project implements a secure multi-user TCP chat system on Linux with the following components:

### Components

1. **Crypto Character Device Driver (Kernel Space)**
   - Linux kernel module (`crypto_driver.c`)
   - Provides AES encryption/decryption using ECB mode
   - Provides MD5 password hashing
   - Uses Linux Kernel Crypto API
   - Exposes `/dev/crypto_dev` device for user space access

2. **Chat Server (User Space)**
   - Multi-threaded TCP server (`chat_server.c`)
   - Listens on port 8888
   - Manages up to 100 concurrent client connections
   - Authenticates users via MD5 password hashing (using kernel driver)
   - Broadcasts messages to all authenticated clients
   - Can encrypt/decrypt messages via kernel driver (infrastructure in place)

3. **Chat Client (User Space)**
   - TCP client (`chat_client.c`)
   - Connects to server
   - Handles user authentication
   - Sends and receives messages in real-time

## Architecture Diagram

```
┌─────────────┐         ┌─────────────┐         ┌─────────────┐
│  Client A   │         │  Client B   │         │  Client C   │
│  (User)     │         │  (User)     │         │  (User)     │
└──────┬──────┘         └──────┬──────┘         └──────┬──────┘
       │                       │                       │
       │ TCP Socket            │ TCP Socket            │ TCP Socket
       │                       │                       │
       └───────────────────────┼───────────────────────┘
                               │
                      ┌────────▼────────┐
                      │  Chat Server    │
                      │  (User Space)   │
                      │  - Auth users   │
                      │  - Route msgs   │
                      └────────┬────────┘
                               │
                         ioctl/read/write
                               │
                      ┌────────▼────────┐
                      │ Crypto Driver   │
                      │ (Kernel Space)  │
                      │  - MD5 hash     │
                      │  - AES encrypt  │
                      │  - AES decrypt  │
                      └─────────────────┘
```

## Authentication Flow

1. Client connects to server via TCP socket
2. Server prompts for username
3. Client sends username
4. Server prompts for password
5. Client sends password (plaintext over connection)
6. Server sends password to crypto driver via ioctl for MD5 hashing
7. Driver computes MD5 hash using Kernel Crypto API
8. Driver returns hash to server
9. Server compares hash with stored hash in user database
10. Server sends AUTH_SUCCESS or AUTH_FAILED to client
11. If successful, client can send/receive messages

## Message Flow

1. Authenticated client sends message to server
2. Server receives message
3. (Optional) Server can encrypt message using crypto driver
4. Server broadcasts formatted message to all other authenticated clients
5. (Optional) Receiving clients decrypt message using crypto driver
6. Clients display message to user

## Design Rationale

### Why Encryption in Kernel Space?

1. **Performance**: Kernel-level crypto operations are faster, especially for bulk encryption
2. **Security**: 
   - Crypto keys are protected in kernel memory space
   - Less vulnerable to user-space memory attacks
   - Centralized security mechanism
3. **Separation of Concerns**:
   - Mechanism (crypto operations) in kernel
   - Policy (when to encrypt, user management) in user space
4. **Reusability**: Other applications can use the same crypto driver
5. **Hardware Acceleration**: Kernel Crypto API can leverage hardware crypto accelerators if available

### Key Design Decisions

1. **Character Device**: Simple interface using ioctl for crypto operations
2. **MD5 for Passwords**: Fast hashing (note: for production, use stronger algorithms like SHA-256 or bcrypt)
3. **AES-128 ECB Mode**: Simple implementation (note: for production, use CBC or GCM mode with proper IV)
4. **Thread-per-client**: Simple concurrency model for server
5. **Broadcast Model**: All messages sent to all authenticated users

## User Database

The server has a hardcoded user database for demonstration:

| Username | Password     |
|----------|--------------|
| alice    | password123  |
| bob      | password456  |
| charlie  | password789  |

## Building the System

### Prerequisites

```bash
# Install kernel headers
sudo apt-get install linux-headers-$(uname -r)

# Install build essentials
sudo apt-get install build-essential gcc make
```

### Build Kernel Module

```bash
make -f Makefile.driver
```

### Build User Space Programs

```bash
make
```

## Installation and Usage

### 1. Load Kernel Module

```bash
# Load the module
sudo insmod crypto_driver.ko

# Verify it's loaded
lsmod | grep crypto_driver

# Check device was created
ls -l /dev/crypto_dev

# View kernel logs
dmesg | tail
```

### 2. Start Chat Server

```bash
# Make executable
chmod +x chat_server

# Run server (requires crypto driver to be loaded)
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

### 3. Connect Clients

Open multiple terminals and run:

```bash
# Make executable
chmod +x chat_client

# Terminal 1 - Alice
./chat_client
# Or connect to remote server:
./chat_client <server_ip>

# Terminal 2 - Bob
./chat_client

# Terminal 3 - Charlie
./chat_client
```

### 4. Login and Chat

Each client will be prompted:
```
USERNAME: alice
PASSWORD: password123
AUTH_SUCCESS
```

Then you can type messages and they will be broadcast to all other users.

## Cleanup

### Unload Kernel Module

```bash
# Stop server first (Ctrl+C)

# Unload module
sudo rmmod crypto_driver

# Verify
lsmod | grep crypto_driver
```

### Clean Build Files

```bash
# Clean kernel module
make -f Makefile.driver clean

# Clean user space programs
make clean
```

## Testing the System

### Test 1: Authentication

1. Start server
2. Connect client with correct credentials → Should succeed
3. Connect client with wrong credentials → Should fail

### Test 2: Multi-user Chat

1. Start server
2. Connect 3 clients (alice, bob, charlie)
3. Send message from alice → Bob and charlie should receive it
4. Send message from bob → Alice and charlie should receive it

### Test 3: Join/Leave Notifications

1. Start server with 2 connected clients
2. Connect third client → Others see join notification
3. Disconnect one client → Others see leave notification

### Test 4: Driver Operations

```bash
# Check driver is working
dmesg | grep crypto_dev

# Should see messages like:
# crypto_dev: Device opened
# crypto_dev: MD5 hash request
# crypto_dev: Device closed
```

## Security Considerations

### Current Implementation (Demo)

- MD5 for password hashing (weak for production)
- AES-128 ECB mode (no IV, not recommended for production)
- Passwords transmitted in plaintext during auth
- Shared AES key hardcoded in server

### Production Recommendations

1. Use stronger hashing: SHA-256, bcrypt, or Argon2
2. Use AES in CBC or GCM mode with proper IV
3. Implement TLS/SSL for client-server communication
4. Store passwords properly salted and hashed
5. Implement proper key exchange (Diffie-Hellman)
6. Add rate limiting and brute-force protection
7. Implement proper session management

## Troubleshooting

### Error: "Failed to open crypto device"

- Ensure kernel module is loaded: `lsmod | grep crypto_driver`
- Check device exists: `ls -l /dev/crypto_dev`
- Check permissions: `sudo chmod 666 /dev/crypto_dev`

### Error: "Connection refused"

- Ensure server is running
- Check firewall rules: `sudo ufw status`
- Verify port 8888 is not in use: `netstat -tuln | grep 8888`

### Error: "Module compilation failed"

- Install kernel headers: `sudo apt-get install linux-headers-$(uname -r)`
- Check kernel version compatibility
- Review build errors in output

## File Structure

```
linux-driver-develop/
├── crypto_driver.c      # Kernel module source
├── chat_server.c        # Server source
├── chat_client.c        # Client source
├── Makefile.driver      # Kernel module makefile
├── Makefile             # User space programs makefile
├── README.md            # Original requirements (Vietnamese)
├── DOCUMENTATION.md     # This file
└── ARCHITECTURE.md      # Detailed architecture
```

## License

GPL v2 (for kernel module compatibility)

## Future Enhancements

1. Add private messaging between users
2. Implement chat rooms/channels
3. Add message history persistence
4. Implement stronger encryption (AES-GCM)
5. Add user registration/management interface
6. Implement file transfer capability
7. Add presence indicators (online/offline)
8. Create GUI client
