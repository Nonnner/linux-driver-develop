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

## Building the System

### 🔹 PROMPT "ĂN ĐIỂM" (CODE GENERATION)

Generate clean, well-commented C code compatible with Linux for a multi-user TCP chat system. Implement a user-space chat server and client, and a kernel-space character device driver providing AES encryption/decryption and MD5 hashing using the Linux Kernel Crypto API. The server must authenticate users and encrypt messages via the driver using ioctl. Follow proper kernel programming practices and separate mechanism (kernel) from policy (user space).

---

**Made with ❤️ for Linux kernel development education**
