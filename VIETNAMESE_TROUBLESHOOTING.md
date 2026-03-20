# Hướng Dẫn Khắc Phục Sự Cố (Vietnamese Troubleshooting Guide)

## 🔴 LỖI: "MD5 hash ioctl failed: Invalid argument"

### Vấn đề
Khi đăng nhập với username/password đúng, bạn vẫn nhận được lỗi:
```
MD5 hash ioctl failed: Invalid argument
Authentication failed for user alice
```

### Nguyên nhân
Kernel module chưa được load hoặc đang sử dụng phiên bản cũ (v1.0) không tương thích với code mới (v2.0).

---

## ✅ GIẢI PHÁP NHANH (Quick Fix)

### Bước 1: Build kernel module
```bash
make driver-build
```

**Kết quả mong đợi:**
```
Building kernel module from driver/ directory...
make -C /lib/modules/.../build M=.../driver modules
  CC [M]  .../driver/crypto_driver.o
  LD [M]  .../driver/crypto_driver.ko
Module built successfully: crypto_driver.ko
```

### Bước 2: Load kernel module (cần quyền sudo)
```bash
sudo make driver-load
```

**Kết quả mong đợi:**
```
Loading kernel module...
sudo insmod crypto_driver.ko
Module loaded successfully
Device created: /dev/crypto_dev
```

### Bước 3: Kiểm tra device đã được tạo
```bash
ls -l /dev/crypto_dev
```

**Kết quả mong đợi:**
```
crw-rw-rw- 1 root root 237, 0 Feb  1 08:00 /dev/crypto_dev
```

### Bước 4: Khởi động server
```bash
./chat_server
```

**Kết quả mong đợi:**
```
Chat server started on port 8888
Waiting for clients...
Available users: alice/password123, bob/password456, charlie/password789
```

✅ Không còn lỗi "MD5 hash ioctl failed"!

### Bước 5: Test đăng nhập

**Option 1: Terminal client**
```bash
./chat_client
```
Nhập:
- Username: alice
- Password: password123

**Option 2: Web UI**
```bash
make run-web
```
Mở trình duyệt: http://localhost:5000

Đăng nhập với:
- alice / password123
- bob / password456
- charlie / password789

---

## 🔍 CÔNG CỤ CHẨN ĐOÁN

### Kiểm tra nhanh
```bash
./check_module_version.sh
```

Script này sẽ:
- ✅ Kiểm tra module đã được load chưa
- ✅ Kiểm tra phiên bản module (v1.0 hay v2.0)
- ✅ Kiểm tra device /dev/crypto_dev có tồn tại không
- ✅ Cung cấp hướng dẫn fix cụ thể

---

## 📋 CÁC VẤN ĐỀ THƯỜNG GẶP

### 1. Module chưa được load

**Lỗi:**
```
Failed to open crypto device: No such file or directory
```

**Fix:**
```bash
sudo make driver-load
```

### 2. Module phiên bản cũ (v1.0)

**Lỗi:**
```
MD5 hash ioctl failed: Invalid argument
```

**Fix:**
```bash
make driver-build       # Build lại module v2.0
sudo make driver-reload # Reload module mới
```

### 3. Không có quyền truy cập device

**Lỗi:**
```
Failed to open crypto device: Permission denied
```

**Fix:**
```bash
sudo chmod 666 /dev/crypto_dev
```

### 4. Port 8888 đã được sử dụng

**Lỗi:**
```
bind failed: Address already in use
```

**Fix:**
```bash
# Tìm process đang sử dụng port
sudo lsof -i :8888

# Kill process cũ
sudo kill <PID>

# Hoặc dùng port khác (sửa trong chat_server.c)
```

### 5. Web backend không start

**Lỗi:**
```
Address already in use: Port 5000
```

**Fix:**
```bash
# Kill process Flask cũ
pkill -f chat_backend

# Hoặc dùng port khác
python3 backend/chat_backend.py --port 5001
```

---

## 🔄 QUY TRÌNH BUILD ĐẦY ĐỦ

### Build tất cả từ đầu

```bash
# 1. Clean build artifacts cũ
make clean
make driver-clean

# 2. Build user space programs
make

# 3. Build kernel module
make driver-build

# 4. Load kernel module
sudo make driver-load

# 5. Kiểm tra
./check_module_version.sh

# 6. Start server
./chat_server
```

---

## ✅ KIỂM TRA HỆ THỐNG

### Checklist đầy đủ

```bash
# 1. Kiểm tra module đã build
ls -lh crypto_driver.ko
# Kết quả mong đợi: file ~400KB

# 2. Kiểm tra module đã load
lsmod | grep crypto_driver
# Kết quả mong đợi: crypto_driver trong list

# 3. Kiểm tra device tồn tại
ls -l /dev/crypto_dev
# Kết quả mong đợi: crw-rw-rw- 1 root root 237, 0

# 4. Kiểm tra phiên bản module
modinfo crypto_driver.ko | grep version
# Kết quả mong đợi: version: 2.0

# 5. Kiểm tra programs đã build
ls -lh chat_server chat_client
# Kết quả mong đợi: cả 2 files executable

# 6. Test MD5 hash
./check_module_version.sh
# Kết quả mong đợi: ✓ All checks pass
```

---

## 📖 TÀI LIỆU KHÁC

### Tiếng Anh (English Documentation)

**Chi tiết về lỗi MD5 IOCTL:**
- `MD5_IOCTL_DEBUG_GUIDE.md` - Giải thích chi tiết kỹ thuật
- `IOCTL_FIX_GUIDE.md` - Hướng dẫn fix IOCTL issues

**Troubleshooting tổng quát:**
- `TROUBLESHOOTING.md` - Tất cả các vấn đề thường gặp
- `BUILD_FIX_SUMMARY.md` - Vấn đề build

**Hướng dẫn sử dụng:**
- `README.md` - Tổng quan dự án
- `QUICKSTART.md` - Bắt đầu nhanh
- `WEB_UI_README.md` - Hướng dẫn Web UI

**Kỹ thuật:**
- `ARCHITECTURE.md` - Kiến trúc hệ thống
- `DOCUMENTATION.md` - Tài liệu kỹ thuật

---

## 🎯 TÓM TẮT

### Nếu gặp lỗi "MD5 hash ioctl failed":

1. ✅ **Build module:** `make driver-build`
2. ✅ **Load module:** `sudo make driver-load`
3. ✅ **Kiểm tra:** `./check_module_version.sh`
4. ✅ **Start server:** `./chat_server`
5. ✅ **Test login:** alice/password123

### Tài khoản demo:
- 👤 alice / password123
- 👤 bob / password456
- 👤 charlie / password789

### Lệnh hữu ích:

```bash
# Diagnostic
./check_module_version.sh
./setup.sh status

# Rebuild everything
make clean && make && make driver-build

# Reload module
sudo make driver-reload

# Check logs
dmesg | tail -20          # Kernel logs
journalctl -f             # System logs

# Help
make help                 # List all make targets
./setup.sh help          # Setup script help
```

---

## 📞 SUPPORT

Nếu vẫn gặp vấn đề sau khi làm theo hướng dẫn:

1. Run diagnostic: `./check_module_version.sh`
2. Check kernel logs: `dmesg | tail -20`
3. Read detailed guide: `cat MD5_IOCTL_DEBUG_GUIDE.md`
4. Check project status: `cat PROJECT_STATUS.md`

---

**Cập nhật:** 1 Tháng 2, 2026  
**Phiên bản:** 2.0  
**Trạng thái:** ✅ Hoàn chỉnh và đã test

---

## ⚡ QUICK REFERENCE

| Vấn đề | Lệnh Fix |
|--------|----------|
| Module chưa load | `sudo make driver-load` |
| Module sai version | `make driver-build && sudo make driver-reload` |
| Device không có quyền | `sudo chmod 666 /dev/crypto_dev` |
| Port đã dùng | `sudo lsof -i :8888` rồi kill process |
| Build lỗi | `make clean && make` |
| Kernel module lỗi | `make driver-clean && make driver-build` |

**Chúc may mắn! Good luck! 🚀**
