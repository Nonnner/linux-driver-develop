# Hướng Dẫn Test và Sử Dụng Chat UI
# Chat Flow Testing and Usage Guide (Vietnamese)

## 📋 Tổng Quan (Overview)

Hệ thống chat đa người dùng đã hoàn thành với đầy đủ tính năng:
- ✅ Đăng nhập (Login)
- ✅ Xem danh sách người dùng (View user list)
- ✅ Chọn người để chat (Select user to chat)
- ✅ Chat riêng tư (Private messaging)
- ✅ Chat công khai (Broadcast messaging)

---

## 🧪 Test Tự Động (Automated Tests)

### Chạy Test Script

```bash
./test_chat_flow.sh
```

**Kiểm tra:**
- ✅ File binaries đã build
- ✅ File web interface tồn tại
- ✅ JavaScript syntax hợp lệ
- ✅ Python syntax hợp lệ
- ✅ Các tính năng có mặt
- ✅ HTML elements đầy đủ
- ✅ Backend handlers hoạt động

---

## 🚀 Chuẩn Bị Môi Trường (Environment Setup)

### Bước 1: Build Code

```bash
# Build server và client
make clean && make

# Build kernel module
make driver-build
```

### Bước 2: Load Kernel Module

```bash
# Load module (cần sudo)
sudo make driver-load

# Kiểm tra module đã load
lsmod | grep crypto_driver
ls -l /dev/crypto_dev
```

### Bước 3: Start Services

**Terminal 1 - TCP Server:**
```bash
./chat_server
```

Kết quả mong đợi:
```
Chat server started on port 8888
Waiting for clients...
Available users: alice/password123, bob/password456, charlie/password789
```

**Terminal 2 - Web Backend:**
```bash
# Install dependencies (lần đầu)
make backend

# Start web server
make run-web
```

Kết quả mong đợi:
```
* Running on http://127.0.0.1:5000
```

---

## 🌐 Test Giao Diện Web (Web UI Testing)

### Test Case 1: Đăng Nhập (Login)

**Mục tiêu:** Kiểm tra user có thể đăng nhập thành công

**Các bước:**
1. Mở trình duyệt: `http://localhost:5000`
2. Nhập username: `alice`
3. Nhập password: `password123`
4. Click nút "Login"

**Kết quả mong đợi:**
- ✅ Màn hình login biến mất
- ✅ Màn hình chat hiện ra
- ✅ Hiển thị username "alice" ở góc trên
- ✅ Hiển thị message: "Welcome alice!"
- ✅ Sidebar hiển thị "alice (you)" ở đầu

**Screenshot mô tả:**
```
┌─────────────────────────────────────┐
│ 💬 Multi-User Chat      alice [Logout] │
├─────────┬───────────────────────────┤
│ 📢 Broad│ Messages:                │
│ cast All│ [System] Welcome alice!   │
│         │                           │
│ alice   │                           │
│ (you)   │                           │
│         │                           │
│ Online: │                           │
│ • bob   │                           │
│ • charlie│                          │
│         ├───────────────────────────┤
│         │ Type a message... [Send]  │
└─────────┴───────────────────────────┘
```

---

### Test Case 2: Xem Danh Sách Users (View User List)

**Mục tiêu:** Kiểm tra user có thể xem ai đang online

**Các bước:**
1. Alice đã login (Test Case 1)
2. Mở tab mới (incognito): `http://localhost:5000`
3. Login as bob (bob/password456)
4. Mở tab thứ 3 (private): `http://localhost:5000`
5. Login as charlie (charlie/password789)

**Kết quả mong đợi:**

**Tab Alice:**
```
Online Users (click to chat):
• bob       ← Xuất hiện sau khi bob login
• charlie   ← Xuất hiện sau khi charlie login
```

**Tab Bob:**
```
Online Users (click to chat):
• alice
• charlie
```

**Tab Charlie:**
```
Online Users (click to chat):
• alice
• bob
```

**Đặc điểm:**
- ✅ Danh sách tự động cập nhật (mỗi 5 giây)
- ✅ Hiển thị user hiện tại ở đầu
- ✅ Các user khác có thể click
- ✅ Hover có hiệu ứng (màu tím nhạt)

---

### Test Case 3: Chọn User Để Chat Riêng (Select User for Private Chat)

**Mục tiêu:** Kiểm tra user có thể chọn người để chat riêng

**Các bước:**
1. Alice đã login và thấy bob trong danh sách
2. Click vào tên "bob" trong danh sách

**Kết quả mong đợi:**
- ✅ Tên "bob" được highlight màu tím
- ✅ Input placeholder đổi thành: "Private message to bob..."
- ✅ Hiện message: "Now chatting privately with bob..."

**Screenshot:**
```
┌─────────────────────────────────────┐
│ 💬 Multi-User Chat      alice [Logout] │
├─────────┬───────────────────────────┤
│ 📢 Broad│ Messages:                │
│ cast All│ [System] Now chatting     │
│         │ privately with bob...     │
│ alice   │                           │
│ (you)   │                           │
│         │                           │
│ Online: │                           │
│ ┌─────┐ │                           │
│ │ bob │ ← Màu tím (selected)       │
│ └─────┘ │                           │
│ charlie │                           │
│         ├───────────────────────────┤
│         │ Private message to bob... │
└─────────┴───────────────────────────┘
```

---

### Test Case 4: Gửi Tin Nhắn Riêng (Send Private Message)

**Mục tiêu:** Kiểm tra tin nhắn riêng chỉ đến đúng người

**Các bước:**
1. Alice đã chọn bob (Test Case 3)
2. Alice gõ: "Hi Bob, this is private!"
3. Alice nhấn Enter hoặc click Send

**Kết quả mong đợi:**

**Tab Alice:**
```
[Private to bob] Hi Bob, this is private!
└─ Màu gradient hồng-tím, bên phải màn hình
```

**Tab Bob:**
```
[Private from alice] Hi Bob, this is private!
└─ Màu gradient tím, bên trái màn hình
```

**Tab Charlie:**
```
(Không thấy gì cả - message không đến charlie)
```

**Đặc điểm:**
- ✅ Chỉ alice và bob thấy tin nhắn
- ✅ Charlie không thấy (vì là private)
- ✅ Alice có confirmation message
- ✅ Màu sắc khác biệt (gradient purple/pink)

---

### Test Case 5: Chuyển Đổi Giữa Users (Switch Between Users)

**Mục tiêu:** Kiểm tra có thể chuyển đổi giữa các user

**Các bước:**
1. Alice đang chat với bob
2. Alice click vào "charlie" trong danh sách
3. Alice gõ: "Hi Charlie!"

**Kết quả mong đợi:**
- ✅ Bob không còn highlight
- ✅ Charlie được highlight màu tím
- ✅ Placeholder: "Private message to charlie..."
- ✅ Message đến charlie, không đến bob

---

### Test Case 6: Broadcast Đến Tất Cả (Broadcast to All)

**Mục tiêu:** Kiểm tra gửi message đến tất cả users

**Các bước:**
1. Alice click nút "📢 Broadcast to All"
2. Alice gõ: "Hello everyone!"
3. Alice nhấn Send

**Kết quả mong đợi:**

**Tab Alice:**
```
[alice] Hello everyone!
└─ Màu nền trắng, style bình thường
```

**Tab Bob:**
```
[alice] Hello everyone!
└─ Nhận được message
```

**Tab Charlie:**
```
[alice] Hello everyone!
└─ Nhận được message
```

**Đặc điểm:**
- ✅ Tất cả users nhận được
- ✅ Style khác với private message
- ✅ Placeholder về: "Type a message..."
- ✅ Không có user nào được highlight

---

### Test Case 7: Auto-Update User List

**Mục tiêu:** Kiểm tra danh sách tự động cập nhật

**Các bước:**
1. Alice, Bob, Charlie đang online
2. Charlie logout (đóng browser)
3. Đợi tối đa 5 giây

**Kết quả mong đợi:**
- ✅ Danh sách alice và bob tự động xóa charlie
- ✅ Message: "[SERVER] charlie has left the chat"
- ✅ Không cần refresh trang

**Ngược lại - User Join:**
1. David login (với account mới nếu có)
2. Danh sách tự động thêm david
3. Message: "[SERVER] david has joined the chat"

---

## 🎨 Kiểm Tra UI/UX (UI/UX Testing)

### Visual Elements Checklist

**Login Screen:**
- ✅ Gradient background (tím-xanh)
- ✅ Logo và title rõ ràng
- ✅ Form input đẹp
- ✅ Demo users được liệt kê
- ✅ Error messages hiển thị đúng

**Chat Screen:**
- ✅ Header với username và logout button
- ✅ Sidebar với user list
- ✅ Message area với scroll
- ✅ Input area fixed ở bottom
- ✅ Footer với encryption info

**User List:**
- ✅ Current user có gradient background
- ✅ Other users có hover effect
- ✅ Selected user có purple highlight
- ✅ Online status indicator (green dot)
- ✅ Broadcast button prominent

**Messages:**
- ✅ Own messages bên phải (float right)
- ✅ Other messages bên trái (float left)
- ✅ Private messages có gradient background
- ✅ System messages màu vàng, centered
- ✅ Timestamps trên mọi message

---

## 🐛 Troubleshooting

### Issue 1: User List Không Hiển Thị

**Triệu chứng:** Sidebar trống, không thấy users

**Nguyên nhân có thể:**
- Backend không kết nối được TCP server
- User list request không được gửi

**Giải pháp:**
```bash
# Check TCP server đang chạy
ps aux | grep chat_server

# Check web backend logs
# Xem terminal 2 có errors không

# Refresh browser với F5
# Hoặc logout và login lại
```

### Issue 2: Không Gửi Được Tin Nhắn

**Triệu chứng:** Gõ message và Send nhưng không thấy gì

**Giải pháp:**
1. Check console (F12 → Console tab)
2. Xem có errors không
3. Check WebSocket connection:
   ```javascript
   // In browser console
   console.log(socket.connected); // should be true
   ```

### Issue 3: Private Message Đến Sai Người

**Triệu chứng:** Gửi cho bob nhưng charlie nhận

**Giải pháp:**
- Kiểm tra user có được highlight không
- Re-select user (click lại)
- Check server logs để xem routing

---

## 📊 Test Results Template

Sử dụng template này để ghi lại kết quả test:

```
Test Date: _______________
Tester: _______________

Test Case 1: Login
□ Login screen hiển thị đúng
□ Nhập credentials thành công
□ Chat screen xuất hiện
□ Username hiển thị đúng
Result: PASS / FAIL

Test Case 2: User List
□ Thấy chính mình trong danh sách
□ Thấy users khác khi họ login
□ Danh sách auto-update
Result: PASS / FAIL

Test Case 3: Select User
□ Click user được highlight
□ Placeholder thay đổi
□ System message xuất hiện
Result: PASS / FAIL

Test Case 4: Private Message
□ Message gửi thành công
□ Chỉ recipient nhận
□ Style đúng (gradient)
Result: PASS / FAIL

Test Case 5: Switch Users
□ Chuyển đổi giữa users
□ Highlight cập nhật đúng
□ Messages đến đúng người
Result: PASS / FAIL

Test Case 6: Broadcast
□ Button "Broadcast to All" hoạt động
□ Tất cả users nhận message
□ Style khác private message
Result: PASS / FAIL

Test Case 7: Auto-Update
□ User join được thông báo
□ User leave được thông báo
□ Danh sách cập nhật tự động
Result: PASS / FAIL

Overall Result: _____ / 7 PASS
```

---

## ✅ Acceptance Criteria

Hệ thống coi như hoàn thành khi:

1. **✅ Đăng nhập thành công**
   - User nhập username/password
   - Hệ thống xác thực
   - Chuyển đến chat screen

2. **✅ Xem danh sách users**
   - Sidebar hiển thị tất cả users online
   - Danh sách cập nhật real-time
   - Current user được đánh dấu rõ

3. **✅ Chọn user để chat**
   - Click vào user trong danh sách
   - User được highlight
   - Input placeholder thay đổi

4. **✅ Chat riêng thành công**
   - Gửi message đến user đã chọn
   - Chỉ người nhận thấy message
   - Style riêng biệt cho private message

5. **✅ Chat công khai thành công**
   - Broadcast button hoạt động
   - Tất cả users nhận message
   - Style khác private message

6. **✅ UX tốt**
   - UI đẹp, professional
   - Responsive và smooth
   - Visual feedback rõ ràng
   - Không có bugs

---

## 📝 Notes

**Demo Users:**
- alice / password123
- bob / password456
- charlie / password789

**Ports:**
- TCP Server: 8888
- Web Backend: 5000

**Technologies:**
- Frontend: HTML, CSS, JavaScript, Socket.IO
- Backend: Python Flask, Flask-SocketIO
- Server: C with TCP sockets
- Crypto: Linux Kernel Driver (AES-128-CBC)

---

## 🎯 Conclusion

Nếu tất cả 7 test cases đều PASS:
- ✅ Hệ thống hoàn thành đầy đủ tính năng
- ✅ Đáp ứng yêu cầu người dùng
- ✅ Sẵn sàng để demo/production

Nếu có test FAIL:
- 🔍 Xem phần Troubleshooting
- 🐛 Fix bugs được phát hiện
- 🔄 Chạy lại tests

**Chúc may mắn! Good luck! 🚀**
