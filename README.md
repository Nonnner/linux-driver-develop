# Linux Driver Develop - Multi-User Chat System

## 🎯 Tổng Quan (Overview)

Hệ thống chat nhiều người dùng (User A, User B, User C, …) trên Linux, sử dụng lập trình socket TCP theo mô hình client–server với mã hóa được thực hiện trong kernel space.

A secure multi-user TCP chat system on Linux that demonstrates kernel-space cryptography using a character device driver.

**🆕 NEW: Web UI Available!** - Giờ đây có thể sử dụng qua giao diện web! See [Web UI Instructions](#-web-ui-giao-diện-web) below.

## ✨ Tính Năng (Features)

- **Multi-user TCP chat**: Support for up to 100 concurrent users
- **User authentication**: Username/password login with MD5 hashing in kernel
- **Kernel-space cryptography**: Linux character device driver for crypto operations
- **AES encryption**: Message encryption/decryption using AES-128
- **Real-time messaging**: Instant message broadcasting to all authenticated users
- **Thread-safe**: Multi-threaded server with proper synchronization
- **🆕 Web Interface**: Beautiful web UI for browser-based chat

## 🏗️ Kiến Trúc Hệ Thống (System Architecture)

```
┌─────────────┐         ┌─────────────┐         ┌─────────────┐
│  Client A   │         │  Client B   │         │  Client C   │
│   (alice)   │         │    (bob)    │         │  (charlie)  │
└──────┬──────┘         └──────┬──────┘         └──────┬──────┘
       │                       │                       │
       │      TCP Socket       │     TCP Socket        │  TCP
       │      Port 8888        │     Port 8888         │  8888
       └───────────────────────┼───────────────────────┘
                               │
            🆕 Web Browser      │
            (via WebSocket)    │
                   │            │
            ┌──────▼────────────▼──┐
            │  Chat Server          │
            │  (User Space)         │
            │  - Auth users         │
            │  - Route msgs         │
            │  - Multi-thread       │
            └──────────┬────────────┘
                       │
              ioctl/read/write
              /dev/crypto_dev
                       │
            ┌──────────▼────────────┐
            │  Crypto Driver        │
            │  (Kernel Space)       │
            │  - MD5 hash           │
            │  - AES encrypt        │
            │  - AES decrypt        │
            │  Linux Crypto API     │
            └───────────────────────┘
```

## 📋 Yêu Cầu Hệ Thống (System Requirements)

### Chức Năng (Functionality)

✅ **Xác thực người dùng** - Mỗi người dùng phải đăng nhập bằng username và password trước khi tham gia chat.

✅ **Chat server** - Chạy ở user space, quản lý nhiều kết nối client đồng thời, xác thực người dùng, gửi và nhận tin nhắn.

✅ **Kernel crypto driver** - Việc băm mật khẩu (MD5) và mã hóa/giải mã tin nhắn (AES) được thực hiện trong kernel space thông qua Linux character device driver.

✅ **IOCTL interface** - Chat server giao tiếp với driver bằng ioctl/read/write.

✅ **Linux Kernel Crypto API** - Driver sử dụng Linux Kernel Crypto API để thực hiện AES và MD5.

✅ **Separation of concerns** - Driver chỉ cung cấp mechanism (mã hóa/băm), không xử lý policy (logic ứng dụng).

✅ **Client-server model** - Client không trực tiếp gọi driver, chỉ giao tiếp với server qua socket.

## 🚀 Hướng Dẫn Nhanh (Quick Start)

### 1. Cài Đặt (Installation)

```bash
# Cài đặt kernel headers và build tools
sudo apt-get update
sudo apt-get install linux-headers-$(uname -r) build-essential gcc make
```

### 2. Biên Dịch (Build)

```bash
# Build user space programs (server và client)
make

# Build kernel module (crypto driver)
make -f Makefile.driver
```

### 3. Chạy Hệ Thống (Run System)

#### Terminal 1: Load kernel module
```bash
sudo insmod crypto_driver.ko
sudo chmod 666 /dev/crypto_dev
dmesg | tail  # Kiểm tra driver đã load
```

#### Terminal 2: Start server
```bash
./chat_server
```

#### Terminal 3: Connect as alice
```bash
./chat_client
# USERNAME: alice
# PASSWORD: password123
```

#### Terminal 4: Connect as bob
```bash
./chat_client
# USERNAME: bob
# PASSWORD: password456
```

## 🌐 Web UI (Giao Diện Web)

### 🆕 NEW: Browser-Based Chat Interface!

Giờ đây có thể sử dụng hệ thống chat qua giao diện web đẹp mắt thay vì chỉ qua terminal!

#### Quick Start Web UI

**Terminal 1: Start TCP Server**
```bash
./chat_server
```

**Terminal 2: Start Web Backend**
```bash
# Install dependencies (first time only)
make backend

# Run web server
make run-web
```

**Terminal 3: Open Browser**
```
http://localhost:5000
```

Login với username/password từ bảng demo accounts bên dưới!

#### Web UI Features
- 🎨 **Beautiful Modern UI** - Gradient design with animations
- 💬 **Real-time Chat** - WebSocket for instant messaging
- 👥 **User List** - See who's online
- 📱 **Responsive** - Works on mobile and desktop
- 🔐 **Secure** - Messages still encrypted via kernel driver

#### Web UI Documentation
Xem chi tiết tại: [WEB_UI_README.md](WEB_UI_README.md)

## 👥 Tài Khoản Demo (Demo Accounts)

| Username | Password     | Mô tả            |
|----------|--------------|------------------|
| alice    | password123  | User thứ nhất    |
| bob      | password456  | User thứ hai     |
| charlie  | password789  | User thứ ba      |

## 📖 Ví Dụ Sử Dụng (Usage Example)

**Terminal 1 (Server):**
```
$ ./chat_server
Crypto driver opened successfully
User alice initialized with hashed password
User bob initialized with hashed password
User charlie initialized with hashed password
Chat server started on port 8888
Waiting for clients...
```

**Terminal 2 (Alice):**
```
$ ./chat_client
Connecting to server at 127.0.0.1:8888...
Connected to chat server
================================================
USERNAME: alice
PASSWORD: password123
AUTH_SUCCESS
Authentication successful!
================================================
You can now send messages. Type your message and press Enter.
================================================
> Hello everyone!
[bob] Hi Alice! How are you?
[charlie] Hey guys!
```

**Terminal 3 (Bob):**
```
$ ./chat_client
USERNAME: bob
PASSWORD: password456
AUTH_SUCCESS
Authentication successful!
[SERVER] alice has joined the chat
[alice] Hello everyone!
> Hi Alice! How are you?
[SERVER] charlie has joined the chat
[charlie] Hey guys!
```

## 🔧 Các Thành Phần (Components)

### 1. Crypto Device Driver (`crypto_driver.c`)
- **Kernel module** - Linux character device driver
- **Device**: `/dev/crypto_dev`
- **Features**:
  - MD5 password hashing
  - AES-128 encryption/decryption
  - IOCTL interface
  - Linux Kernel Crypto API

### 2. Chat Server (`chat_server.c`)
- **Multi-threaded TCP server** - Handles up to 100 clients
- **Port**: 8888
- **Features**:
  - User authentication via crypto driver
  - Message broadcasting
  - Join/leave notifications
  - Thread-safe client management

### 3. Chat Client (`chat_client.c`)
- **TCP client application**
- **Features**:
  - Server connection
  - User authentication
  - Real-time messaging
  - Multi-threaded (send/receive)

## 📚 Tài Liệu (Documentation)

| File | Mô Tả |
|------|-------|
| **[QUICKSTART.md](QUICKSTART.md)** | Hướng dẫn nhanh, step-by-step |
| **[DOCUMENTATION.md](DOCUMENTATION.md)** | Tài liệu chi tiết, troubleshooting |
| **[ARCHITECTURE.md](ARCHITECTURE.md)** | Kiến trúc kỹ thuật, luồng hoạt động |
| **[README.original.md](README.original.md)** | Yêu cầu gốc (Vietnamese) |

## 🔄 Luồng Xác Thực (Authentication Flow)

```
1. Client kết nối đến Server qua TCP socket
2. Server yêu cầu username
3. Client gửi username
4. Server yêu cầu password
5. Client gửi password (plaintext)
6. Server gửi password đến crypto driver qua ioctl
7. Driver tính MD5 hash sử dụng Kernel Crypto API
8. Driver trả hash về Server
9. Server so sánh hash với database
10. Server gửi AUTH_SUCCESS hoặc AUTH_FAILED
11. Nếu thành công, client có thể gửi/nhận tin nhắn
```

## 💬 Luồng Gửi Tin Nhắn (Message Flow)

```
1. Client đã xác thực gửi tin nhắn đến Server
2. Server nhận tin nhắn
3. Server format tin nhắn: "[username] message"
4. Server broadcast tin nhắn đến tất cả client khác
5. Các client hiển thị tin nhắn cho user
```

## 🛡️ Tại Sao Mã Hóa Trong Kernel? (Why Kernel-Space Crypto?)

### Lợi Ích (Benefits)

1. **Hiệu suất (Performance)**
   - Truy cập trực tiếp phần cứng
   - Có thể sử dụng hardware crypto accelerators
   - Không có context switch overhead

2. **Bảo mật (Security)**
   - Kernel memory được bảo vệ tốt hơn
   - Cách ly khỏi user-space attacks
   - Khóa mã hóa được bảo vệ trong kernel space

3. **Tập trung hóa (Centralization)**
   - Một cơ chế crypto cho nhiều ứng dụng
   - Dễ kiểm soát và cập nhật
   - Consistency across applications

4. **Tách biệt trách nhiệm (Separation of Concerns)**
   - **Mechanism** (crypto operations) → Kernel
   - **Policy** (when to encrypt, user management) → User space

## 🔐 Bảo Mật (Security Features)

### Current Implementation
- ✅ MD5 password hashing in kernel
- ✅ AES-128 encryption/decryption support
- ✅ Protected crypto operations in kernel space
- ✅ Secure user authentication

### Production Recommendations
- 🔄 Use SHA-256 or bcrypt instead of MD5
- 🔄 Implement AES-GCM instead of ECB mode
- 🔄 Add TLS/SSL for transport security
- 🔄 Implement proper key exchange (Diffie-Hellman)
- 🔄 Add rate limiting and brute-force protection

## 🧹 Dọn Dẹp (Cleanup)

```bash
# Stop server and clients (Ctrl+C)

# Unload kernel module
sudo rmmod crypto_driver

# Clean build files
make clean
make -f Makefile.driver clean
```

## 🛠️ Troubleshooting

### Lỗi: "Failed to open crypto device"

```bash
# Kiểm tra module đã load chưa
lsmod | grep crypto_driver

# Load module nếu chưa có
sudo insmod crypto_driver.ko

# Kiểm tra device
ls -l /dev/crypto_dev

# Sửa quyền truy cập
sudo chmod 666 /dev/crypto_dev
```

### Lỗi: "Connection refused"

```bash
# Kiểm tra server có chạy không
ps aux | grep chat_server

# Kiểm tra port 8888
netstat -tuln | grep 8888

# Start server
./chat_server
```

## 📁 Cấu Trúc Thư Mục (File Structure)

```
linux-driver-develop/
├── crypto_driver.c       # Kernel module source
├── chat_server.c         # Server source
├── chat_client.c         # Client source
├── Makefile              # User space build
├── Makefile.driver       # Kernel module build
├── .gitignore            # Git ignore file
├── README.md             # This file
├── README.original.md    # Original requirements
├── QUICKSTART.md         # Quick start guide
├── DOCUMENTATION.md      # Complete documentation
└── ARCHITECTURE.md       # Technical architecture
```

## 🔮 Future Enhancements

- [ ] Private messaging between users
- [ ] Chat rooms/channels
- [ ] Message history persistence
- [ ] TLS/SSL transport security
- [ ] Stronger encryption (AES-GCM)
- [ ] Web interface
- [ ] File transfer
- [ ] User registration

## 📜 License

GPL v2 (for kernel module compatibility)

## 🎓 Educational Purpose

This project demonstrates:
- ✅ Linux kernel module development
- ✅ Character device driver implementation  
- ✅ Linux Kernel Crypto API usage
- ✅ TCP socket programming
- ✅ Multi-threaded server design
- ✅ IOCTL interface design
- ✅ Proper kernel/user space separation

## 🔗 References

- [Linux Kernel Crypto API](https://www.kernel.org/doc/html/latest/crypto/index.html)
- [Linux Device Drivers (LDD3)](https://lwn.net/Kernel/LDD3/)
- [POSIX Threads Programming](https://computing.llnl.gov/tutorials/pthreads/)
- [Linux Kernel Module Programming Guide](https://sysprog21.github.io/lkmpg/)

---

## 📝 Prompt Tham Khảo (Reference Prompts)

### 🔹 PROMPT CHO BÁO CÁO / ĐỒ ÁN

Viết phần mô tả hệ thống cho một đồ án xây dựng ứng dụng chat nhiều người dùng dựa trên socket TCP, trong đó xác thực người dùng và bảo mật tin nhắn được thực hiện thông qua Linux device driver. Driver triển khai thuật toán mã hóa AES và thuật toán băm MD5 trong kernel space bằng Linux Kernel Crypto API. Trình bày kiến trúc, chức năng từng thành phần, luồng hoạt động và ưu điểm của giải pháp.

### 🔹 PROMPT "ĂN ĐIỂM" (CODE GENERATION)

Generate clean, well-commented C code compatible with Linux for a multi-user TCP chat system. Implement a user-space chat server and client, and a kernel-space character device driver providing AES encryption/decryption and MD5 hashing using the Linux Kernel Crypto API. The server must authenticate users and encrypt messages via the driver using ioctl. Follow proper kernel programming practices and separate mechanism (kernel) from policy (user space).

---

**Made with ❤️ for Linux kernel development education**
