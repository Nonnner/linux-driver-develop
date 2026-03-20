# Web UI for Multi-User Chat System

## 📱 Giao Diện Web (Web Interface)

Hệ thống chat giờ đây có giao diện web để người dùng có thể chat qua trình duyệt thay vì terminal.

## ✨ Tính Năng Web UI

- 🔐 **Login Form** - Giao diện đăng nhập đẹp và dễ sử dụng
- 💬 **Real-time Chat** - Chat thời gian thực qua WebSocket
- 👥 **User List** - Xem danh sách người dùng online
- 📱 **Responsive Design** - Hoạt động tốt trên mobile và desktop
- 🎨 **Modern UI** - Giao diện đẹp mắt với gradient và animations

## 🏗️ Kiến Trúc

```
┌─────────────┐     WebSocket      ┌─────────────┐     TCP Socket     ┌─────────────┐
│  Web Browser│◄──────────────────►│Flask Backend│◄──────────────────►│ Chat Server │
│  (HTML/JS)  │                    │  (Python)   │                    │    (C)      │
└─────────────┘                    └─────────────┘                    └─────────────┘
```

## 🚀 Cách Sử Dụng

### 1. Cài Đặt Dependencies

```bash
# Cài đặt Python dependencies cho web backend
make backend
```

Hoặc thủ công:
```bash
pip3 install -r backend/requirements.txt
```

### 2. Chạy Hệ Thống

#### Terminal 1: Start TCP Chat Server
```bash
./chat_server
```

#### Terminal 2: Start Web Backend
```bash
make run-web
```

Hoặc thủ công:
```bash
cd backend
python3 chat_backend.py
```

### 3. Truy Cập Web UI

Mở trình duyệt và truy cập:
```
http://localhost:5000
```

### 4. Đăng Nhập

Sử dụng một trong các tài khoản demo:
- **alice** / password123
- **bob** / password456
- **charlie** / password789

## 📁 Cấu Trúc Thư Mục

```
backend/
├── chat_backend.py      # Flask server với WebSocket
└── requirements.txt     # Python dependencies

web/
├── templates/
│   └── index.html      # HTML template chính
└── static/
    ├── css/
    │   └── style.css   # Stylesheet
    └── js/
        └── chat.js     # Client-side JavaScript
```

## 🔧 Cấu Hình

### Backend Configuration (backend/chat_backend.py)

```python
TCP_SERVER_HOST = '127.0.0.1'  # Chat server host
TCP_SERVER_PORT = 8888         # Chat server port
```

### Frontend Configuration

Web UI tự động kết nối đến backend qua WebSocket. Không cần cấu hình.

## 📸 Screenshots

### Login Screen
Giao diện đăng nhập với form đẹp và danh sách demo users.

### Chat Interface
- Sidebar bên trái: Danh sách users online
- Khu vực chính: Messages với scroll
- Bottom: Input box để gửi tin nhắn
- Header: Username và nút Logout

## 🎯 Tính Năng Kỹ Thuật

### Backend (Flask)
- **Flask**: Web framework
- **Flask-SocketIO**: WebSocket support cho real-time
- **Threading**: Xử lý nhiều connections đồng thời
- **Socket Bridge**: Bridge giữa WebSocket và TCP socket

### Frontend
- **Pure JavaScript**: Không dùng framework, lightweight
- **Socket.IO Client**: WebSocket client
- **Responsive CSS**: Mobile-friendly design
- **Modern UI**: Gradient, animations, shadows

## 🔐 Bảo Mật

- Messages vẫn được mã hóa bằng AES-128-CBC qua kernel driver
- WebSocket connection giữa browser và backend
- TCP socket connection giữa backend và chat server
- Session management với Flask sessions

## 🐛 Troubleshooting

### Lỗi: "Failed to connect to chat server"
```bash
# Đảm bảo chat server đang chạy
./chat_server
```

### Lỗi: "Module not found"
```bash
# Cài lại dependencies
pip3 install -r backend/requirements.txt
```

### Lỗi: "Port 5000 already in use"
```bash
# Kill process đang dùng port 5000
sudo lsof -ti:5000 | xargs kill -9
```

### Messages không hiển thị
- Kiểm tra console log trong browser (F12)
- Kiểm tra terminal của backend server
- Đảm bảo WebSocket connection thành công

## 📊 Performance

- Hỗ trợ nhiều users đồng thời
- Real-time updates với latency thấp
- Lightweight: không dùng heavy frameworks
- Efficient message passing

## 🔮 Future Enhancements

- [ ] File upload/sharing
- [ ] Emoji support
- [ ] Message history
- [ ] Private messaging UI
- [ ] Typing indicators
- [ ] Read receipts
- [ ] Dark mode
- [ ] Notification sounds

## 📝 API Documentation

### WebSocket Events

**Client → Server:**
- `login` - Đăng nhập với username/password
- `send_message` - Gửi tin nhắn
- `get_users` - Lấy danh sách users

**Server → Client:**
- `login_response` - Kết quả đăng nhập
- `chat_message` - Tin nhắn mới
- `user_list` - Danh sách users
- `error` - Thông báo lỗi
- `disconnect_event` - Ngắt kết nối

## 💡 Tips

1. **Multiple Users**: Mở nhiều tab/browsers để test multi-user
2. **Terminal Client**: Vẫn có thể dùng terminal client song song với web UI
3. **Development**: Set `debug=True` trong chat_backend.py để auto-reload
4. **Production**: Đổi SECRET_KEY và tắt debug mode

## 📚 Tech Stack

- **Backend**: Python 3.8+, Flask, Flask-SocketIO, eventlet
- **Frontend**: HTML5, CSS3, JavaScript (ES6), Socket.IO
- **Protocol**: WebSocket + TCP Socket
- **Crypto**: Vẫn sử dụng kernel driver (transparent)

---

**Developed with ❤️ for Linux Kernel Chat System**
