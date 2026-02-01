# Hướng Dẫn Sử Dụng Chức Năng Chat - Vietnamese Guide

## Tổng Quan

Hệ thống chat hiện đã được nâng cấp với các tính năng:
✅ **Xem danh sách người dùng online**
✅ **Chọn người để chat riêng (private chat)**
✅ **Gửi tin nhắn riêng tư**
✅ **Broadcast tin nhắn cho tất cả mọi người**

---

## Quy Trình Sử Dụng (User Flow)

### Bước 1: Đăng Nhập

**Terminal Client:**
```bash
./chat_client
# Nhập username: alice
# Nhập password: password123
```

**Web UI:**
```
1. Mở trình duyệt: http://localhost:5000
2. Nhập username: alice
3. Nhập password: password123  
4. Click "Login"
```

### Bước 2: Xem Danh Sách Người Dùng

**Terminal Client:**
```
Sau khi đăng nhập, gõ lệnh:
/list

Kết quả:
USERLIST:alice,bob,charlie
```

**Web UI:**
```
Sau khi đăng nhập, bên trái màn hình sẽ tự động hiển thị:
┌─────────────────────┐
│ alice (you)         │ ← Bạn (màu gradient)
├─────────────────────┤
│ Online Users:       │
│ • bob              │ ← Click để chat
│ • charlie          │ ← Click để chat
└─────────────────────┘
```

### Bước 3: Chọn Người Để Chat

**Terminal Client:**
```
Sử dụng lệnh /msg:
/msg bob Xin chào Bob!

Bạn sẽ thấy:
[PRIVATE to bob] Xin chào Bob!

Bob sẽ nhận được:
[PRIVATE from alice] Xin chào Bob!
```

**Web UI:**
```
1. Click vào tên "bob" trong danh sách
2. Bob sẽ được highlight màu tím
3. Input box thay đổi thành: "Private message to bob..."
4. Gõ tin nhắn: "Xin chào Bob!"
5. Enter để gửi
```

### Bước 4: Chat Thành Công!

**Kết quả:**
- ✅ Bob nhận được tin nhắn riêng từ bạn
- ✅ Charlie KHÔNG thấy tin nhắn này (private)
- ✅ Bạn thấy confirmation: "[PRIVATE to bob] Xin chào Bob!"
- ✅ Bob thấy: "[PRIVATE from alice] Xin chào Bob!"

---

## Các Tính Năng Chi Tiết

### 1. Chat Riêng (Private Message)

**Mục đích:** Gửi tin nhắn riêng cho 1 người, người khác không thấy

**Terminal:**
```bash
/msg <username> <tin nhắn>

Ví dụ:
/msg bob Chào bạn!
/msg charlie Hẹn gặp lại!
```

**Web UI:**
```
1. Click vào tên người dùng trong sidebar
2. Người đó sẽ được highlight (màu tím)
3. Gõ tin nhắn như bình thường
4. Tin nhắn chỉ gửi cho người đó
```

### 2. Broadcast (Gửi Cho Tất Cả)

**Mục đích:** Gửi tin nhắn cho TẤT CẢ mọi người online

**Terminal:**
```bash
# Chỉ cần gõ tin nhắn không có lệnh /msg
Xin chào mọi người!

Tất cả sẽ thấy:
[alice] Xin chào mọi người!
```

**Web UI:**
```
1. Không click vào ai (hoặc click vào user đang chọn để deselect)
2. Gõ tin nhắn bình thường
3. Tin nhắn sẽ gửi cho tất cả
```

### 3. Xem Danh Sách User

**Terminal:**
```bash
/list

Kết quả:
USERLIST:alice,bob,charlie
```

**Web UI:**
```
Danh sách tự động hiển thị bên trái
Cập nhật realtime khi có người vào/ra
```

### 4. Trợ Giúp

**Terminal:**
```bash
/help

Hiển thị:
[SERVER] Available commands:
[SERVER]   /list - Show online users
[SERVER]   /msg <username> <message> - Send private message
[SERVER]   /help - Show this help
[SERVER] Just type a message to broadcast to all users
```

---

## Ví Dụ Cụ Thể

### Scenario 1: Alice Chat Với Bob

**Alice (Terminal):**
```bash
./chat_client
# Login: alice / password123
/list
# Thấy: USERLIST:alice,bob,charlie
/msg bob Chào Bob, bạn khỏe không?
# Thấy: [PRIVATE to bob] Chào Bob, bạn khỏe không?
```

**Bob (Web UI):**
```
1. Login: bob / password456
2. Thấy danh sách: alice, charlie
3. Nhận tin nhắn: [PRIVATE from alice] Chào Bob, bạn khỏe không?
4. Click vào alice
5. Reply: "Tôi khỏe, cảm ơn!"
```

**Charlie:**
```
KHÔNG thấy gì cả - đây là chat riêng giữa alice và bob
```

### Scenario 2: Alice Thông Báo Cho Tất Cả

**Alice:**
```bash
# Không dùng /msg
Xin chào mọi người!
```

**Bob và Charlie:**
```
Cả hai đều thấy:
[alice] Xin chào mọi người!
```

---

## So Sánh: Private Chat vs Broadcast

| Tính Năng | Private Chat | Broadcast |
|-----------|-------------|-----------|
| **Lệnh (Terminal)** | `/msg <user> <message>` | Gõ trực tiếp |
| **Web UI** | Click vào user | Không chọn ai |
| **Ai nhận được** | Chỉ người được chọn | Tất cả mọi người |
| **Hiển thị** | `[PRIVATE from/to ...]` | `[username] message` |
| **Màu sắc (Web)** | Gradient tím/xanh | Màu thường |

---

## Giao Diện Web UI

### Màn Hình Chat

```
┌────────────────────────────────────────────────────────┐
│ Chat Application                alice 🔓 [Logout]      │
├──────────┬────────────────────────────────────────────┤
│          │                                             │
│ alice    │  [SERVER] Welcome to chat, alice!          │
│ (you)    │  [SERVER] Now chatting privately with bob  │
│          │                                             │
│ Online:  │  [PRIVATE to bob] Hello Bob!                │
│ • bob ✓  │                                     [alice] │
│ • charlie│  [PRIVATE from bob] Hi Alice!               │
│          │  [bob]                                      │
│          │                                             │
│          │  [charlie] Hello everyone!                  │
│          │  [charlie]                                  │
│          ├────────────────────────────────────────────┤
│          │ Private message to bob... [Send]            │
└──────────┴────────────────────────────────────────────┘
```

### Giải Thích Màu Sắc

- **Gradient xanh-tím:** User hiện tại (you)
- **Màu tím:** User đang được chọn để chat
- **Màu trắng:** Users khác (click để chọn)
- **Gradient tím nhạt:** Private message nhận được
- **Gradient hồng-xanh:** Private message bạn gửi
- **Màu vàng:** System messages

---

## Câu Hỏi Thường Gặp (FAQ)

### 1. Làm sao biết ai đang online?

**Terminal:** Gõ `/list`
**Web UI:** Xem sidebar bên trái, tự động cập nhật

### 2. Chat riêng với ai đó như thế nào?

**Terminal:** `/msg <username> <tin nhắn>`
**Web UI:** Click vào tên người đó, rồi gõ tin nhắn

### 3. Chuyển sang chat với người khác?

**Terminal:** Gõ `/msg <người_khác> <tin nhắn>`
**Web UI:** Click vào tên người khác

### 4. Gửi tin nhắn cho tất cả?

**Terminal:** Gõ tin nhắn trực tiếp không có `/msg`
**Web UI:** Không chọn ai (hoặc click để deselect)

### 5. Làm sao biết tin nhắn là private hay broadcast?

- Private: `[PRIVATE from/to ...]`
- Broadcast: `[username] message`

### 6. Tin nhắn có được mã hóa không?

✅ Có! Tất cả tin nhắn đều được mã hóa bằng AES qua kernel driver

---

## Khởi Động Hệ Thống

### Bước 1: Load Kernel Module
```bash
sudo make driver-load
```

### Bước 2: Start Server
```bash
./chat_server
```

### Bước 3: Start Web Backend (Nếu Dùng Web UI)
```bash
make run-web
```

### Bước 4: Kết Nối

**Terminal Client:**
```bash
./chat_client
```

**Web UI:**
```
Mở trình duyệt: http://localhost:5000
```

---

## Tài Khoản Demo

Có 3 tài khoản để test:

| Username | Password |
|----------|----------|
| alice | password123 |
| bob | password456 |
| charlie | password789 |

---

## Lưu Ý Quan Trọng

1. **Kernel Module:** Phải load module trước khi start server
2. **Port 8888:** Server chạy trên port 8888
3. **Port 5000:** Web UI chạy trên port 5000
4. **Private Messages:** Chỉ người gửi và người nhận thấy được
5. **Broadcast:** Tất cả users online đều thấy
6. **User List:** Tự động cập nhật khi có người join/leave

---

## Xử Lý Sự Cố

### Không thấy danh sách user?

**Terminal:** Gõ `/list`
**Web UI:** Reload trang hoặc logout/login lại

### Tin nhắn không gửi được?

1. Kiểm tra server còn chạy không
2. Kiểm tra kernel module đã load chưa
3. Logout và login lại

### Web UI không hoạt động?

```bash
# Restart web backend
Ctrl+C (stop)
make run-web (start lại)
```

---

## Kết Luận

Bây giờ bạn đã có đầy đủ chức năng chat:

✅ **Đăng nhập** - Login với username/password
✅ **Xem user list** - Biết ai đang online
✅ **Chọn user** - Click để chat riêng
✅ **Chat riêng** - Tin nhắn private 1-1
✅ **Chat nhóm** - Broadcast cho tất cả

**Chúc bạn chat vui vẻ!** 🎉

---

## Hỗ Trợ

Nếu cần thêm thông tin:
- Đọc `README.md` (English)
- Xem `VIETNAMESE_TROUBLESHOOTING.md` (giải quyết sự cố)
- Xem `WEB_UI_README.md` (chi tiết Web UI)

**Happy Chatting!** 💬
