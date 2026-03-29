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


## 🔄 UPDATE 2026-03-29 (Bo sung, khong thay the noi dung cu)

### A. Cap nhat trang thai flow theo code hien tai

1. **Auth flow**
- Server van xac thuc user bang MD5 thong qua kernel driver (ioctl).

2. **Message flow (AES)**
- Da bo sung xu ly message qua driver AES trong runtime flow o server:
  - `IOCTL_ENCRYPT`
  - `IOCTL_DECRYPT`
- Flow nay duoc ap dung cho ca broadcast va private message.

3. **Web run flow**
- Da sua theo huong dung local `.venv` trong `make backend` / `make run-web` de tranh loi PEP 668.
- `run-web` co co che don process cu chiem port 5000 truoc khi start.

### B. Danh gia yeu cau tiep theo
## "Phat trien driver tren CentOS 64bit - minh hoa voi ban phim USB"

### 1) Co kha thi khong?
**Co, hoan toan kha thi**, va phu hop de lam demo do an.

Muc do kha thi cao nhat neu bat dau theo huong:
- `input_handler` (attach vao input subsystem, nghe EV_KEY)

Huong nay:
- De trien khai hon viet USB HID driver full.
- It nguy co xung dot voi `usbhid` mac dinh.
- De demo nhanh key press/release tren CentOS 64-bit.

### 2) Ban can setup them gi?

#### 2.1 He thong / moi truong
- CentOS Stream 8/9 (khuyen nghi) hoac CentOS 7.
- Kernel va `kernel-devel` phai **cung version** (`uname -r`).

#### 2.2 Goi can cai (CentOS)
```bash
sudo dnf install -y gcc make elfutils-libelf-devel bc
sudo dnf install -y kernel-devel-$(uname -r) kernel-headers-$(uname -r)
sudo dnf install -y usbutils kmod util-linux
sudo dnf install -y epel-release && sudo dnf install -y evtest
```
Neu CentOS 7 thi doi `dnf` thanh `yum`.

#### 2.3 Tool debug/quan sat
- `dmesg -w`
- `lsusb`
- `cat /proc/bus/input/devices`
- `evtest`

#### 2.4 Secure Boot (neu co)
- Neu may bat Secure Boot, module tu build thuong khong load duoc.
- Can tat Secure Boot hoac ky module.

### 3) Lo trinh trien khai de xuat (thuc te)

#### Phase 1 - De demo nhanh (khuyen nghi)
- Viet module `usb_kbd_monitor` theo `input_handler`:
  - `connect`/`disconnect`
  - callback `event()` bat `EV_KEY`
- Log keycode/state ra `dmesg`.

#### Phase 2 - Nang cap
- Them character device `/dev/kbdmon` de user-space doc stream su kien.
- Them ring-buffer + lock.

#### Phase 3 - Nang cao
- Moi xem xet huong USB driver bind truc tiep interface keyboard neu giang vien yeu cau sau.

### 4) Tieu chi nghiem thu de bao cao
1. Build module tren CentOS 64-bit thanh cong.
2. `insmod`/`rmmod` on dinh, khong crash.
3. Cam USB keyboard, nhan phim thay log EV_KEY.
4. Khong vo hieu hoa ban phim he thong.
5. (Tuy chon) user app doc duoc event tu `/dev/kbdmon`.

### 5) Rui ro chinh va cach giam
- **Rui ro mismatch kernel-devel**: luon cai dung version theo `uname -r`.
- **Rui ro xung dot HID**: uu tien `input_handler`, khong unbind `usbhid` o phase dau.
- **Rui ro Secure Boot**: xu ly truoc khi demo.

---

## 📋 PHASE 1 IMPLEMENTATION: USB Keyboard Monitor (Updated 2026-03-29)

### Overview
This section documents the completed implementation of the USB keyboard monitor as an input event handler for CentOS 7/8/9. The module demonstrates Linux input subsystem integration without modifying the default USB HID driver.

**Key Features**:
- Non-invasive event capture using `input_handler` mechanism
- Logs keyboard events (keycode + state) to kernel ring buffer
- Supports all USB keyboards (EV_KEY capable devices)
- Safe coexistence with default `usbhid` driver
- Minimal ~300 lines of C code with comprehensive comments

### File Structure
```
driver/
├── usb_kbd_monitor.c      # Input handler module (NEW)
├── Makefile              # Updated: obj-m += usb_kbd_monitor.o (MODIFIED)
└── (existing crypto_driver.c and Makefile orchestration)

Makefile                   # New targets: driver-load-kbd, driver-unload-kbd (MODIFIED)
Makefile.driver            # Updated: copy both .ko files (MODIFIED)
README.md                  # This section (NEW)
```

### Prerequisites for CentOS 7 64-bit

#### 1. System Requirements
- CentOS 7.x with kernel ~3.10.x
- USB keyboard hardware (tested with standard USB HID keyboards)
- Secure Boot: disabled (or module self-signed)
- For VMware: USB passthrough enabled

#### 2. Required Packages
Install latest **EXACT** kernel-devel matching your kernel:

```bash
# First, check your kernel version
uname -r

# Example output: 3.10.0-1160.el7.x86_64
# Install THAT EXACT version of kernel-devel
sudo yum install kernel-devel-$(uname -r)

# Install build dependencies
sudo yum install gcc make elfutils-libelf-devel

# Install debugging/testing tools (optional but recommended)
sudo yum install usbutils kmod util-linux

# For manual testing with evtest (optional)
sudo yum install evtest
```

**Important**: If kernel-devel version doesn't match `uname -r`, module loading will fail with undefined symbol errors. Double-check with:

```bash
rpm -q kernel-devel
# Must show: kernel-devel-X.X.X-XXX.el7.x86_64
# Where X.X.X-XXX matches your uname -r output exactly
```

### Building the USB Keyboard Monitor

#### Build Command
```bash
# Build both crypto_driver and usb_kbd_monitor
make driver-build

# Or manually:
make -f Makefile.driver

# Output should show:
# ✓ crypto_driver.ko
# ✓ usb_kbd_monitor.ko
```

#### Troubleshooting Build Failures

| Issue | Solution |
|-------|----------|
| `struct input_device_id not found` | kernel-devel version mismatch; verify `uname -r` |
| `undefined reference to input_register_handler` | Ensure `kernel-devel` is installed correctly |
| `error: ...` during make | Run `make driver-clean` then retry `make driver-build` |

### Loading and Testing the Module

#### Step 1: Load the Module
```bash
# Load USB keyboard monitor
make driver-load-kbd

# Or manually:
sudo insmod usb_kbd_monitor.ko

# Verify it loaded
lsmod | grep usb_kbd_monitor
```

#### Step 2: Monitor Kernel Messages
Open a new terminal and watch live kernel logs:

```bash
# Watch in real-time
dmesg -w

# Filter for usb_kbd_monitor messages only
dmesg -w | grep usb_kbd_monitor

# Alternative: tail kernel buffer
sudo tail -f /var/log/kernel
```

#### Step 3: Test Keyboard Events
1. **Plug in USB keyboard** (if not already connected)
   - Expected in `dmesg`:
     ```
     usb_kbd_monitor: connected to device <device_name>
     usb_kbd_monitor: [EV_KEY] keycode=30 state=PRESSED device=<name>
     usb_kbd_monitor: [EV_KEY] keycode=30 state=RELEASED device=<name>
     ```

2. **Press various keys** on the USB keyboard
   - For example, pressing 'A' appears as keycode 30
   - Each press shows PRESSED, release shows RELEASED

3. **Observe key mapping** (common keycodes):
   - 30 = A, 48 = B, 46 = C, 32 = D, 18 = E, 33 = F, 34 = G, 35 = H, 23 = I, 36 = J
   - 37 = K, 38 = L, 50 = M, 49 = N, 24 = O, 25 = P, 16 = Q, 19 = R, 31 = S, 20 = T
   - 22 = U, 47 = V, 17 = W, 45 = X, 21 = Y, 44 = Z
   - 2-11 = 1-9, 0 = 0, 43 = TAB, 28 = ENTER, 57 = SPACE

#### Step 4: Advanced Testing with evtest (Optional)
CentOS 7 provides `evtest` for interactive keyboard event inspection:

```bash
# List all input devices
sudo evtest

# Select the USB keyboard device (usually /dev/input/event*)
# Press keys and see EV_KEY events with timings
```

**Expected Output**:
```
Event: time ..., type 0 (EV_SYN), code 0 (SYN_REPORT), value 0
Event: time ..., type 1 (EV_KEY), code 30 (KEY_A), value 1
Event: time ..., type 0 (EV_SYN), code 0 (SYN_REPORT), value 0
Event: time ..., type 1 (EV_KEY), code 30 (KEY_A), value 0
```

### Coexistence Testing with Chat System

Verify that USB keyboard module doesn't interfere with the chat system:

```bash
# Terminal 1: Start crypto driver + chat server
make driver-load
./chat_server

# Terminal 2: Load keyboard monitor
make driver-load-kbd

# Terminal 3: Connect client and test chat
./chat_client

# Terminal 4: Monitor keyboard events
dmesg -w | grep usb_kbd_monitor
```

**Expected Behavior**:
- Chat messages are encrypted/decrypted via crypto driver (unaffected)
- USB keyboard captures work independently via usb_kbd_monitor
- No resource conflicts or crashes
- Both drivers coexist stably (`lsmod` shows both)

### Unloading the Module

```bash
# Unload USB keyboard monitor
make driver-unload-kbd

# Or manually:
sudo rmmod usb_kbd_monitor

# Verify unloaded
lsmod | grep usb_kbd_monitor  # Should return empty

# Check for clean shutdown in dmesg
dmesg | tail -5
# Should show: "usb_kbd_monitor: cleanup complete"
```

### Troubleshooting Runtime Issues

| Issue | Symptom | Solution |
|-------|---------|----------|
| Module won't load | `insmod: error inserting usb_kbd_monitor.ko: Exec format error` | Check Secure Boot; rebuild with matching kernel-devel |
| No events in dmesg | USB keyboard plugged but no log | Load module first, then plug keyboard; check `lsusb` for device |
| "Device busy" on unload | `rmmod` fails with "Device busy" | Ensure all client applications closed; check `lsof /dev/input/eventX` |
| System becomes unresponsive | High CPU or hang | Immediate unload: `sudo rmmod usb_kbd_monitor` |

### Manual Module Inspection

```bash
# Check module parameters and info
modinfo usb_kbd_monitor.ko

# Check module dependencies
modinfo -d usb_kbd_monitor.ko

# View module in kernel
lsmod | grep usb_kbd_monitor

# Check loaded at what time
dmesg | grep "usb_kbd_monitor: initializing"
```

### Demo Procedure for CentOS 7

Use this procedure for a comprehensive demo:

1. **Setup Phase** (1-2 minutes)
   ```bash
   # Verify prerequisites
   uname -r  # Note kernel version
   rpm -q kernel-devel  # Verify match
   
   # Build modules
   make driver-build
   
   # Verify .ko files exist
   ls -lh *.ko
   ```

2. **Load Crypto Driver** (30 seconds)
   ```bash
   make driver-load
   lsmod | grep crypto_driver
   ```

3. **Start Chat Server** (10 seconds)
   ```bash
   ./chat_server &
   ```

4. **Load Keyboard Monitor** (20 seconds)
   ```bash
   make driver-load-kbd
   lsmod | grep usb_kbd_monitor
   ```

5. **Open dmesg Monitor** (in new terminal)
   ```bash
   dmesg -w | grep usb_kbd_monitor
   ```

6. **Test Keyboard** (1-2 minutes)
   - Plug in USB keyboard → watch connect log
   - Press multiple keys → watch EV_KEY events
   - Type a phrase → verify keycode sequence matches
   - Unplug keyboard → watch disconnect log

7. **Test Chat** (in new terminal, 1 minute)
   ```bash
   ./chat_client
   # Send encrypted messages while keyboard monitor is running
   # Verify both work independently
   ```

8. **Cleanup** (20 seconds)
   ```bash
   make driver-unload-kbd
   make driver-unload
   ```

**Total Demo Time**: ~8-10 minutes

### Acceptance Criteria Checklist

✅ **Build**: Module builds cleanly without warnings
```bash
make driver-build
# No errors, both .ko files created
```

✅ **Load**: Module loads without errors
```bash
sudo insmod usb_kbd_monitor.ko
# Success (exit code 0)
```

✅ **Event Capture**: Keyboard events appear in dmesg
```bash
dmesg | grep "usb_kbd_monitor.*EV_KEY"
# Shows keycode and state for each key press/release
```

✅ **Stability**: No kernel warnings or oops
```bash
dmesg | grep -i "error\|crash\|oops"
# No usb_kbd_monitor-related errors
```

✅ **Coexistence**: Chat system still works
```bash
./chat_server &
./chat_client
# Can send/receive encrypted messages normally
```

✅ **Unload**: Clean module removal
```bash
sudo rmmod usb_kbd_monitor
lsmod | grep usb_kbd_monitor  # Empty
dmesg | grep "cleanup complete"  # Confirmed
```

### Next Steps / Phase 2 (Optional)

If extended functionality is desired:

1. **Add character device** (`/dev/kbdmon`) for user-space event streaming
2. **Implement ring buffer** to queue events without dmesg
3. **Write user-space tool** to read and process keyboard events
4. **Add filtering/configuration** via module parameters

### References & Documentation

- [Linux Input Subsystem](https://www.kernel.org/doc/html/latest/input/) - Official kernel documentation
- [Input Handler API](https://www.kernel.org/doc/html/latest/input/input-programming.html) - Handler programming guide
- CentOS 7 Kernel: 3.10.x LTS stable branch
- Event codes: `#include <linux/input-event-codes.h>` in kernel source

---


