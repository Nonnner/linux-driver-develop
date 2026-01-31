# 💬 Hệ Thống Chat Nhiều Người Dùng với Linux Kernel Crypto Driver

Hệ thống chat nhiều người dùng trên Linux, sử dụng TCP socket với mã hóa AES và băm MD5 được thực hiện trong kernel space thông qua character device driver.

## 📋 Mục Lục
- [Yêu Cầu Hệ Thống](#-yêu-cầu-hệ-thống)
- [Cài Đặt](#-cài-đặt)
- [Cách Chạy Project](#-cách-chạy-project)
- [Tài Khoản Test](#-tài-khoản-test)
- [Kiến Trúc Hệ Thống](#-kiến-trúc-hệ-thống)

## 🔧 Yêu Cầu Hệ Thống

- Linux (Ubuntu 20.04+ hoặc tương đương)
- GCC compiler
- Make
- Python 3.8+ và pip (cho Web UI)

## 📦 Cài Đặt

### Bước 1: Clone repository
```bash
git clone https://github.com/Nonnner/linux-driver-develop.git
cd linux-driver-develop
```

### Bước 2: Build project
```bash
# Build server và client
make

# (Tùy chọn) Cài đặt dependencies cho Web UI
make backend
```

## 🚀 Cách Chạy Project

### Cách 1: Sử dụng Terminal Client (CLI)

**Terminal 1 - Khởi động Server:**
```bash
./server/chat_server
```
Server sẽ chạy trên port 8888 (mặc định).

**Terminal 2 - Khởi động Client:**
```bash
./client/chat_client
```

**Các lệnh trong client:**
| Lệnh | Mô tả |
|------|-------|
| `login <username>` | Đăng nhập (sẽ hỏi password) |
| `logout` | Đăng xuất |
| `send <user> <message>` | Gửi tin nhắn riêng |
| `broadcast <message>` | Gửi tin nhắn cho tất cả |
| `list` | Xem danh sách user online |
| `help` | Xem hướng dẫn |
| `exit` | Thoát |

**Ví dụ sử dụng:**
```
> login alice
Password: password123
Login successful! Welcome, alice

> list
=== Online Users ===
  [online] alice

> broadcast Xin chào mọi người!
Message broadcast to all users

> send bob Hello Bob!
Message sent to bob

> exit
Goodbye!
```

### Cách 2: Sử dụng Web UI (Giao diện web)

**Terminal 1 - Khởi động TCP Server:**
```bash
./server/chat_server
```

**Terminal 2 - Khởi động Web Backend:**
```bash
cd backend
python3 chat_backend.py
```

**Terminal 3 - Mở trình duyệt:**
```
http://localhost:5000
```

**Hoặc chạy nhanh cả 2:**
```bash
make run-web
```
Sau đó mở browser: http://localhost:5000

## 👤 Tài Khoản Test

| Username | Password |
|----------|----------|
| alice | password123 |
| bob | secret456 |
| charlie | test789 |

## 🏗️ Kiến Trúc Hệ Thống

```
┌─────────────────────────────────────────────────────────────┐
│                        USER SPACE                            │
├─────────────────────────────────────────────────────────────┤
│                                                              │
│   ┌────────────┐     TCP Socket     ┌────────────────┐      │
│   │ Web Browser│◄──────────────────►│  Flask Backend │      │
│   │  (Web UI)  │    WebSocket       │  (Python)      │      │
│   └────────────┘                    └───────┬────────┘      │
│                                             │               │
│   ┌────────────┐     TCP Socket     ┌───────▼────────┐      │
│   │ Chat Client│◄──────────────────►│  Chat Server   │      │
│   │ (Terminal) │                    │  (C program)   │      │
│   └────────────┘                    └───────┬────────┘      │
│                                             │               │
│                                    ioctl/read/write         │
│                                             │               │
├─────────────────────────────────────────────┼───────────────┤
│                        KERNEL SPACE         │               │
├─────────────────────────────────────────────┼───────────────┤
│                                             ▼               │
│   ┌─────────────────────────────────────────────────┐      │
│   │          Crypto Character Device Driver          │      │
│   │               /dev/crypto_dev                    │      │
│   │  ┌──────────────┐    ┌──────────────┐           │      │
│   │  │  AES-128-CBC │    │     MD5      │           │      │
│   │  │  Encrypt/Dec │    │   Hash Func  │           │      │
│   │  └──────────────┘    └──────────────┘           │      │
│   │           Linux Kernel Crypto API               │      │
│   └─────────────────────────────────────────────────┘      │
└─────────────────────────────────────────────────────────────┘
```

## 📁 Cấu Trúc Thư Mục

```
linux-driver-develop/
├── Makefile              # Build file chính
├── README.md             # File này
├── ARCHITECTURE.md       # Tài liệu chi tiết kiến trúc
├── common/               # Headers dùng chung
│   ├── crypto_user.h     # Interface driver
│   └── protocol.h        # Protocol definitions
├── driver/               # Kernel module (crypto)
│   ├── crypto_driver.c
│   └── Makefile
├── server/               # TCP Chat server
│   ├── chat_server.c
│   └── Makefile
├── client/               # Terminal client
│   ├── chat_client.c
│   └── Makefile
├── backend/              # Web backend (Flask)
│   ├── chat_backend.py
│   └── requirements.txt
└── web/                  # Web UI
    ├── templates/
    └── static/
```

## 🆘 Trợ Giúp

Xem các targets có sẵn:
```bash
make help
```

## 📖 Tài Liệu Chi Tiết

Xem file [ARCHITECTURE.md](ARCHITECTURE.md) để biết thêm chi tiết về kiến trúc hệ thống, luồng hoạt động và thiết kế.
