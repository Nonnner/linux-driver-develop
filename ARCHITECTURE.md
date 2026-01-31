# System Architecture - Multi-User Chat with Kernel Crypto

## Overview

This document provides a detailed technical architecture of the multi-user chat system with kernel-level cryptographic operations.

## Layer Architecture

### 1. Application Layer (User Space)

#### Chat Client
- **Language**: C
- **Dependencies**: POSIX sockets, pthreads
- **Key Functions**:
  - TCP connection management
  - User input handling
  - Message display
  - Multi-threaded: separate thread for receiving messages

#### Chat Server
- **Language**: C
- **Dependencies**: POSIX sockets, pthreads, ioctl
- **Key Functions**:
  - Multi-client connection handling (up to 100 concurrent)
  - User authentication
  - Message broadcasting
  - Client state management
  - Interface with crypto driver

**Thread Model**:
```
Main Thread: Accept connections
    │
    ├─> Client Thread 1 (handles user 1)
    ├─> Client Thread 2 (handles user 2)
    ├─> Client Thread 3 (handles user 3)
    └─> ... (up to 100 threads)
```

### 2. Kernel Layer (Kernel Space)

#### Crypto Character Device Driver
- **Language**: C (Linux Kernel Module)
- **APIs Used**: 
  - Linux Kernel Crypto API
  - Character device API
  - ioctl interface
- **Key Functions**:
  - MD5 hashing
  - AES encryption
  - AES decryption

## Component Details

### Crypto Driver (`crypto_driver.c`)

**Module Information**:
- License: GPL
- Device Name: `/dev/crypto_dev`
- Major Number: Dynamically allocated
- Device Class: `crypto`

**IOCTL Commands**:
```c
#define CRYPTO_IOC_MAGIC 'c'
#define CRYPTO_MD5_HASH     _IOWR('c', 1, struct crypto_data)
#define CRYPTO_AES_ENCRYPT  _IOWR('c', 2, struct crypto_data)
#define CRYPTO_AES_DECRYPT  _IOWR('c', 3, struct crypto_data)
```

**Data Structure**:
```c
struct crypto_data {
    unsigned char input[4096];      // Input data
    unsigned char output[4096];     // Output data
    unsigned char key[16];          // AES key (128-bit)
    unsigned int input_len;         // Input length
    unsigned int output_len;        // Output length
};
```

**Crypto Operations**:

1. **MD5 Hash**:
   - Uses `crypto_alloc_shash("md5", 0, 0)`
   - Output: 16 bytes (128 bits)
   - Used for: Password hashing

2. **AES Encryption**:
   - Uses `crypto_alloc_skcipher("ecb(aes)", 0, 0)`
   - Key size: 16 bytes (128 bits)
   - Block size: 16 bytes
   - Mode: ECB (Electronic Codebook)
   - Padding: Automatic to 16-byte boundary

3. **AES Decryption**:
   - Same algorithm as encryption
   - Reverses the encryption process

**Driver Lifecycle**:
```
module_init
    │
    ├─> register_chrdev() - Register character device
    ├─> class_create()    - Create device class
    └─> device_create()   - Create device node (/dev/crypto_dev)

module_exit
    │
    ├─> device_destroy()  - Remove device node
    ├─> class_destroy()   - Remove device class
    └─> unregister_chrdev() - Unregister character device
```

### Chat Server (`chat_server.c`)

**Server Configuration**:
- Port: 8888
- Max Clients: 100
- Socket Type: TCP (SOCK_STREAM)
- Address: INADDR_ANY (all interfaces)

**Data Structures**:

```c
typedef struct {
    int socket;                    // Client socket FD
    int id;                        // Unique client ID
    char username[32];             // Username
    int authenticated;             // Auth status
    pthread_t thread;              // Client handler thread
} client_t;

// User database entry
struct {
    char username[32];
    unsigned char password_hash[16]; // MD5 hash
} user_db[];
```

**Server Flow**:

```
Server Start
    │
    ├─> Open crypto driver (/dev/crypto_dev)
    ├─> Initialize user database (hash passwords)
    ├─> Create TCP socket
    ├─> Bind to port 8888
    ├─> Listen for connections
    │
    └─> Accept Loop
            │
            ├─> Accept client connection
            ├─> Create client structure
            ├─> Add to client array
            └─> Spawn client handler thread
                    │
                    ├─> Request username
                    ├─> Request password
                    ├─> Hash password (via driver)
                    ├─> Verify against database
                    │
                    ├─> If authenticated:
                    │   ├─> Send AUTH_SUCCESS
                    │   ├─> Announce join to all
                    │   └─> Enter message loop
                    │           │
                    │           ├─> Receive message
                    │           ├─> Format message
                    │           └─> Broadcast to all others
                    │
                    └─> If not authenticated:
                        └─> Send AUTH_FAILED, disconnect
```

**Synchronization**:
- `pthread_mutex_t clients_mutex`: Protects client array
- Used in: add_client, remove_client, send_message_to_all

### Chat Client (`chat_client.c`)

**Client Configuration**:
- Default Server: 127.0.0.1
- Server Port: 8888
- Socket Type: TCP

**Client Flow**:

```
Client Start
    │
    ├─> Create TCP socket
    ├─> Connect to server
    │
    ├─> Authentication Phase
    │   ├─> Receive username prompt
    │   ├─> Send username
    │   ├─> Receive password prompt
    │   ├─> Send password
    │   └─> Receive auth result
    │
    ├─> If AUTH_SUCCESS:
    │   ├─> Spawn receiver thread
    │   │   └─> Loop: Receive and display messages
    │   │
    │   └─> Main Thread: Input loop
    │       ├─> Read user input
    │       ├─> Check for quit command
    │       └─> Send to server
    │
    └─> Cleanup and disconnect
```

## Communication Protocols

### Client-Server Protocol

**Authentication Phase**:
```
Client                          Server
  │                              │
  ├─────── Connect ──────────────>
  │                              │
  <─────── "USERNAME: " ──────────┤
  │                              │
  ├─────── "alice\n" ────────────>
  │                              │
  <─────── "PASSWORD: " ──────────┤
  │                              │
  ├─────── "password123\n" ──────>
  │                              │
  <─────── "AUTH_SUCCESS\n" ─────┤
  │                              │
```

**Message Phase**:
```
Client A                     Server                      Client B
  │                           │                            │
  ├─── "Hello everyone\n" ───>│                            │
  │                           │                            │
  │                           ├─── "[alice] Hello...\n" ──>
  │                           │                            │
```

### Server-Driver Protocol

**IOCTL Communication**:

```c
// Example: MD5 Hash
struct crypto_data data;
strcpy(data.input, "password123");
data.input_len = 11;

ioctl(crypto_fd, CRYPTO_MD5_HASH, &data);

// data.output now contains 16-byte MD5 hash
// data.output_len = 16
```

**Process**:
```
User Space                  Kernel Space
   (Server)                   (Driver)
      │                          │
      ├─── open("/dev/crypto_dev")
      │                          │
      ├─── ioctl(CRYPTO_MD5_HASH)
      │                          │
      │                          ├─> Copy data from user space
      │                          ├─> Allocate crypto_shash
      │                          ├─> Compute MD5 hash
      │                          ├─> Copy result to user space
      │                          └─> Return to user space
      │<───────────────────────────┤
      │                          │
      ├─── ioctl(CRYPTO_AES_ENCRYPT)
      │                          │
      │                          ├─> Copy data from user space
      │                          ├─> Allocate crypto_skcipher
      │                          ├─> Set AES key
      │                          ├─> Encrypt data
      │                          ├─> Copy result to user space
      │                          └─> Return to user space
      │<───────────────────────────┤
      │                          │
      ├─── close(crypto_fd)
      │                          │
```

## Security Architecture

### Current Implementation

**Password Security**:
```
Password Flow:
Client: "password123" (plaintext)
   │
   v
Server: Receives plaintext
   │
   v
Driver: MD5("password123") → hash
   │
   v
Server: Compare hash with stored hash
```

**Message Security** (infrastructure ready):
```
Message Flow (if encryption enabled):
Client A: "Hello" (plaintext)
   │
   v
Server: Receives plaintext
   │
   v
Driver: AES_Encrypt("Hello") → ciphertext
   │
   v
Server: Broadcast ciphertext
   │
   v
Client B: Receives ciphertext
   │
   v
Driver: AES_Decrypt(ciphertext) → "Hello"
```

### Security Layers

```
┌─────────────────────────────────────┐
│     Application Security            │
│  - User authentication              │
│  - Session management               │
│  - Input validation                 │
└─────────────────────────────────────┘
                 │
┌─────────────────────────────────────┐
│     Transport Security              │
│  - TCP reliable delivery            │
│  - (Future: TLS/SSL)                │
└─────────────────────────────────────┘
                 │
┌─────────────────────────────────────┐
│     Cryptographic Security          │
│  - Kernel Crypto API                │
│  - Hardware acceleration ready      │
│  - Memory protection (kernel space) │
└─────────────────────────────────────┘
```

## Performance Considerations

### Server Scalability

- **Thread-per-client**: Simple but limited by system thread limit
- **Memory usage**: ~8KB per client thread (default stack)
- **Max clients**: 100 (configurable via MAX_CLIENTS)

### Crypto Performance

**Kernel Space Advantages**:
- Direct hardware access
- No context switching for crypto ops
- Kernel memory is more secure
- Can use hardware crypto accelerators

**Estimated Performance**:
- MD5: ~500 MB/s (software)
- AES-128: ~200 MB/s (software)
- With hardware: 10x-100x faster

## Error Handling

### Driver Errors

- Allocation failures: Return -ENOMEM
- Crypto operation failures: Log and return error code
- Invalid IOCTL: Return -EINVAL
- Copy from/to user space: Return -EFAULT

### Server Errors

- Socket errors: Log and continue
- Client disconnects: Clean up resources
- Authentication failures: Disconnect client
- Driver unavailable: Exit with error message

### Client Errors

- Connection failures: Display error and exit
- Authentication failures: Display message and exit
- Send/receive errors: Exit gracefully

## Future Architecture Enhancements

### 1. Scalability Improvements

- Implement epoll for better I/O multiplexing
- Use thread pool instead of thread-per-client
- Add load balancing for multiple server instances

### 2. Security Enhancements

- Add TLS/SSL for transport security
- Implement proper key exchange (Diffie-Hellman)
- Use stronger algorithms (SHA-256, AES-GCM)
- Add message authentication codes (MAC)

### 3. Feature Additions

- Private messaging
- Chat rooms/channels
- File transfer
- Message persistence (database)
- Web interface

### 4. Driver Enhancements

- Support for more algorithms
- Asynchronous crypto operations
- Better error reporting
- Performance monitoring

## Conclusion

This architecture provides a solid foundation for a secure multi-user chat system with clear separation between user space policy and kernel space mechanism. The design follows Linux kernel best practices and provides extensibility for future enhancements.
