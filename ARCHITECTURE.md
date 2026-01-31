# Multi-user Chat System with Linux Kernel Crypto Driver

## Mô tả hệ thống / System Description

Đây là hệ thống chat nhiều người dùng (Multi-user Chat System) trên Linux, sử dụng lập trình socket TCP theo mô hình client-server. Hệ thống bao gồm 3 thành phần chính:

1. **Chat Client** - Ứng dụng client chạy trong user space
2. **Chat Server** - Server xử lý kết nối và định tuyến tin nhắn 
3. **Crypto Device Driver** - Linux kernel module cung cấp chức năng mã hóa AES và băm MD5

## Kiến trúc hệ thống / System Architecture

```
┌─────────────────────────────────────────────────────────────────────────┐
│                              USER SPACE                                  │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                          │
│   ┌──────────────┐        TCP Socket        ┌──────────────────┐        │
│   │ Chat Client  │◄────────────────────────►│   Chat Server    │        │
│   │   (User A)   │                          │                  │        │
│   └──────────────┘                          │  - Quản lý kết   │        │
│                                             │    nối clients   │        │
│   ┌──────────────┐        TCP Socket        │  - Xác thực      │        │
│   │ Chat Client  │◄────────────────────────►│    người dùng    │        │
│   │   (User B)   │                          │  - Định tuyến    │        │
│   └──────────────┘                          │    tin nhắn      │        │
│                                             │                  │        │
│   ┌──────────────┐        TCP Socket        └────────┬─────────┘        │
│   │ Chat Client  │◄────────────────────────►         │                  │
│   │   (User C)   │                          ioctl/read/write            │
│   └──────────────┘                                   │                  │
│                                                      ▼                  │
├──────────────────────────────────────────────────────────────────────────┤
│                            KERNEL SPACE                                  │
├──────────────────────────────────────────────────────────────────────────┤
│                                                                          │
│   ┌──────────────────────────────────────────────────────────────┐      │
│   │                  Crypto Character Device Driver               │      │
│   │                       /dev/crypto_dev                         │      │
│   ├──────────────────────────────────────────────────────────────┤      │
│   │                                                               │      │
│   │   ┌─────────────────────┐    ┌─────────────────────┐         │      │
│   │   │    AES-128-CBC      │    │       MD5           │         │      │
│   │   │  Encryption/Decrypt │    │    Hash Function    │         │      │
│   │   └─────────────────────┘    └─────────────────────┘         │      │
│   │                                                               │      │
│   │              Linux Kernel Crypto API                          │      │
│   │                                                               │      │
│   └──────────────────────────────────────────────────────────────┘      │
│                                                                          │
└──────────────────────────────────────────────────────────────────────────┘
```

## Chức năng các thành phần / Component Functions

### 1. Chat Client (`client/`)
- Kết nối đến server qua TCP socket
- Đăng nhập bằng username và password (password được hash MD5)
- Gửi tin nhắn riêng hoặc broadcast
- Nhận và hiển thị tin nhắn
- Xem danh sách người dùng online

### 2. Chat Server (`server/`)
- Quản lý nhiều kết nối client đồng thời (multi-threaded)
- Xác thực người dùng (so sánh password hash)
- Định tuyến tin nhắn giữa các client
- Giao tiếp với driver để mã hóa/giải mã tin nhắn

### 3. Crypto Driver (`driver/`)
- Character device driver (`/dev/crypto_dev`)
- Cung cấp các IOCTL commands:
  - `IOCTL_SET_KEY`: Đặt key AES
  - `IOCTL_SET_IV` / `IOCTL_GET_IV`: Đặt/lấy IV
  - `IOCTL_ENCRYPT`: Mã hóa dữ liệu với AES-128-CBC
  - `IOCTL_DECRYPT`: Giải mã dữ liệu với AES-128-CBC
  - `IOCTL_MD5_HASH`: Băm dữ liệu với MD5
- Sử dụng Linux Kernel Crypto API

## Luồng hoạt động / Operation Flows

### Luồng xác thực / Authentication Flow:
```
1. Client nhập username/password
2. Client hash password bằng MD5 (qua driver hoặc fallback)
3. Client gửi LOGIN_REQ(username, password_hash) đến Server
4. Server so sánh password_hash với database
5. Nếu đúng:
   - Server tạo session_key và IV ngẫu nhiên
   - Server gửi LOGIN_RESP(SUCCESS, session_key, iv) về Client
   - Client lưu session_key và IV để mã hóa tin nhắn
6. Nếu sai:
   - Server gửi LOGIN_RESP(INVALID_CREDENTIALS) về Client
```

### Luồng gửi tin nhắn / Message Flow:
```
1. Sender nhập tin nhắn
2. Sender mã hóa tin nhắn bằng AES (qua driver)
3. Sender gửi CHAT_MSG(encrypted_message, iv) đến Server
4. Server nhận và giải mã tin nhắn
5. Server mã hóa lại tin nhắn với session_key của Receiver
6. Server gửi tin nhắn đã mã hóa đến Receiver
7. Receiver giải mã và hiển thị tin nhắn
```

## Cấu trúc thư mục / Directory Structure

```
linux-driver-develop/
├── Makefile              # Main build file
├── README.md             # Project description
├── ARCHITECTURE.md       # This documentation
├── common/               # Shared headers
│   ├── crypto_user.h     # Crypto driver interface (user space)
│   └── protocol.h        # Chat protocol definitions
├── driver/               # Kernel module
│   ├── crypto_driver.c   # Driver implementation
│   ├── crypto_driver.h   # Driver header (kernel space)
│   └── Makefile
├── server/               # TCP Chat server
│   ├── chat_server.c     # Server implementation
│   └── Makefile
├── client/               # Terminal chat client
│   ├── chat_client.c     # Client implementation
│   └── Makefile
├── backend/              # Web backend (Python Flask)
│   ├── chat_backend.py   # Flask + SocketIO server
│   └── requirements.txt  # Python dependencies
└── web/                  # Web UI
    ├── templates/
    │   └── index.html    # Main HTML template
    └── static/
        ├── css/
        │   └── style.css # Stylesheet
        └── js/
            └── chat.js   # Client-side JavaScript
```

## Kiến trúc Web / Web Architecture

```
┌─────────────────────────────────────────────────────────────────────────┐
│                              WEB LAYER                                   │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                          │
│   ┌──────────────────┐     WebSocket      ┌──────────────────┐          │
│   │  Web Browser     │◄──────────────────►│  Flask Backend   │          │
│   │  (HTML/CSS/JS)   │                    │  (chat_backend)  │          │
│   │  - Login form    │                    │  - WebSocket     │          │
│   │  - Chat UI       │                    │  - REST API      │          │
│   │  - User list     │                    │  - Bridge to TCP │          │
│   └──────────────────┘                    └────────┬─────────┘          │
│                                                    │                     │
│                                           TCP Socket                     │
│                                                    │                     │
├────────────────────────────────────────────────────┼─────────────────────┤
│                              CORE LAYER            │                     │
├────────────────────────────────────────────────────┼─────────────────────┤
│                                                    ▼                     │
│   ┌──────────────┐        TCP Socket        ┌──────────────────┐        │
│   │ Chat Client  │◄────────────────────────►│   Chat Server    │        │
│   │  (Terminal)  │                          │  (chat_server)   │        │
│   └──────────────┘                          └────────┬─────────┘        │
│                                                      │                   │
│                                             ioctl/read/write             │
│                                                      ▼                   │
├──────────────────────────────────────────────────────────────────────────┤
│                            KERNEL SPACE                                  │
├──────────────────────────────────────────────────────────────────────────┤
│   ┌──────────────────────────────────────────────────────────────┐      │
│   │                  Crypto Character Device Driver               │      │
│   │                       /dev/crypto_dev                         │      │
│   └──────────────────────────────────────────────────────────────┘      │
└──────────────────────────────────────────────────────────────────────────┘
```

## Yêu cầu hệ thống / Requirements

- Linux kernel 5.x hoặc cao hơn
- GCC compiler
- Linux kernel headers (để build driver)
- Make
- Python 3.8+ và pip (cho web backend)

## Hướng dẫn build / Build Instructions

### Build user space components only (không cần kernel headers):
```bash
make
```

### Build tất cả bao gồm kernel driver:
```bash
make full
```

### Cài đặt driver (cần sudo):
```bash
sudo make install
```

### Gỡ driver:
```bash
sudo make uninstall
```

### Setup Web Backend:
```bash
make backend
# Hoặc: pip3 install -r backend/requirements.txt
```

## Hướng dẫn sử dụng / Usage Instructions

### Cách 1: Terminal Client (CLI)

#### 1. Khởi động server:
```bash
./server/chat_server [port]
# Default port: 8888
```

#### 2. Khởi động client (terminal khác):
```bash
./client/chat_client [server_ip] [port]
# Default: 127.0.0.1:8888
```

#### 3. Các lệnh client:
```
login [username]     - Đăng nhập (password sẽ được hỏi)
logout               - Đăng xuất
send <user> <msg>    - Gửi tin nhắn riêng
broadcast <msg>      - Gửi tin nhắn cho tất cả
list                 - Xem danh sách user online
exit                 - Thoát
```

### Cách 2: Web UI (Browser)

#### 1. Khởi động TCP server:
```bash
./server/chat_server
```

#### 2. Khởi động web backend (terminal khác):
```bash
cd backend
python3 chat_backend.py
# Server chạy tại http://localhost:5000
```

#### 3. Mở browser:
```
http://localhost:5000
```

#### 4. Sử dụng giao diện web:
- Đăng nhập với username/password
- Xem danh sách người dùng online bên trái
- Nhấp vào tên người dùng để gửi tin nhắn riêng
- Gửi tin nhắn broadcast đến tất cả

### Test users mặc định:
| Username | Password    |
|----------|-------------|
| alice    | password123 |
| bob      | secret456   |
| charlie  | test789     |

## Ưu điểm của giải pháp / Solution Benefits

### Tại sao mã hóa trong kernel?

1. **Bảo mật cao hơn**: Key và dữ liệu nhạy cảm được xử lý trong kernel space, khó bị truy cập trái phép từ user space.

2. **Hiệu suất tối ưu**: Linux Kernel Crypto API được tối ưu hóa, có thể sử dụng hardware acceleration (AES-NI) nếu có.

3. **Tách biệt Mechanism vs Policy**: 
   - **Mechanism (kernel)**: Cung cấp thuật toán mã hóa/băm
   - **Policy (user space)**: Quyết định khi nào và cái gì cần mã hóa

4. **Tái sử dụng**: Driver có thể được sử dụng bởi nhiều ứng dụng khác nhau.

5. **Tin cậy**: Code trong kernel được kiểm tra kỹ lưỡng và có quyền truy cập tài nguyên hệ thống an toàn hơn.

## Lưu ý / Notes

- Hệ thống có thể chạy mà không cần driver (fallback mode cho testing)
- Trong môi trường production, nên luôn sử dụng driver để đảm bảo bảo mật
- MD5 được sử dụng cho demo, trong thực tế nên dùng SHA-256 hoặc bcrypt/scrypt cho password hashing
