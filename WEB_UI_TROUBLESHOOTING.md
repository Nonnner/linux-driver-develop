# Web UI Troubleshooting Guide

## Common Issues and Solutions

### 1. Cannot Access Chat After Login

**Symptom:**
- Login page loads ✓
- Enter credentials ✓
- Click login button ✓
- Button shows "Logging in..." ✓
- BUT chat interface doesn't appear ✗

**Possible Causes & Solutions:**

#### A. TCP Chat Server Not Running
**Check:**
```bash
# Check if server is running
ps aux | grep chat_server

# Check if port 8888 is listening
netstat -tuln | grep 8888
# or
ss -tuln | grep 8888
```

**Solution:**
```bash
# Start the TCP server
./chat_server
```

**Expected output:**
```
Crypto driver opened successfully
User alice initialized with hashed password
User bob initialized with hashed password
User charlie initialized with hashed password
Server listening on port 8888
```

#### B. Kernel Module Not Loaded
**Check:**
```bash
# Check if module is loaded
lsmod | grep crypto_driver

# Check if device exists
ls -l /dev/crypto_dev
```

**Solution:**
```bash
# Build and load module
make driver-build
sudo make driver-load

# Or use helper script
sudo ./setup.sh load
```

#### C. Web Backend Not Running
**Check:**
```bash
# Check if backend is running
ps aux | grep chat_backend

# Check if port 5000 is listening
netstat -tuln | grep 5000
```

**Solution:**
```bash
# Start web backend
cd backend
python3 chat_backend.py

# Or use Makefile
make run-web
```

#### D. Browser Console Errors
**Check:**
1. Open browser Developer Tools (F12)
2. Go to Console tab
3. Look for errors

**Common errors:**

**Error: "Failed to load Socket.IO"**
```
Solution: Check internet connection (Socket.IO loads from CDN)
Or: Download Socket.IO locally and update HTML
```

**Error: "WebSocket connection failed"**
```
Solution: Restart web backend
Check firewall settings
```

**Error: "Login response timeout"**
```
Solution: Check TCP server is responding
Check backend logs for connection errors
```

### 2. Login Takes Too Long

**Symptom:**
- Login button stuck at "Logging in..."
- No response after 5-10 seconds

**Debugging Steps:**

1. **Check Backend Logs:**
```bash
# Look for [LOGIN] messages in web backend console
[LOGIN] Attempt from user: alice
[LOGIN] Connected to TCP server, starting authentication...
[LOGIN] Received prompt: 'USERNAME: '
[LOGIN] Sending username: alice
```

If you see these messages, backend is working. If stuck, continue:

2. **Check TCP Server:**
```bash
# Server should show connection:
New connection from 127.0.0.1:xxxxx
```

3. **Test TCP Connection Manually:**
```bash
telnet localhost 8888
# Should see: USERNAME:
# Type username and press Enter
# Should see: PASSWORD:
# Type password and press Enter
# Should see: AUTH_SUCCESS or AUTH_FAILED
```

**Solutions:**
- Restart TCP server
- Restart web backend
- Check firewall not blocking localhost connections

### 3. Wrong Credentials Error

**Symptom:**
- Error message: "Invalid username or password"

**Solution:**
Use correct demo credentials:
- alice / password123
- bob / password456
- charlie / password789

**Note:** Passwords are case-sensitive!

### 4. Web Backend Won't Start

**Symptom:**
```
ImportError: No module named 'flask'
```

**Solution:**
```bash
# Install dependencies
cd backend
pip3 install -r requirements.txt

# Or use virtual environment
python3 -m venv venv
source venv/bin/activate
pip install -r requirements.txt
```

**Symptom:**
```
Address already in use: ('0.0.0.0', 5000)
```

**Solution:**
```bash
# Kill existing process
pkill -f chat_backend.py

# Or find and kill by PID
lsof -ti:5000 | xargs kill -9

# Then restart
python3 chat_backend.py
```

### 5. Chat Messages Not Appearing

**Symptom:**
- Login successful ✓
- Chat interface visible ✓
- Type message and send ✓
- BUT message doesn't appear ✗

**Debugging:**

1. **Check Browser Console (F12):**
```javascript
// Should see when sending:
Message sent: "your message"

// Should see when receiving:
Chat message: "[username] message text"
```

2. **Check Backend Logs:**
```
Message from alice: Hello world
```

3. **Check TCP Server:**
```
Received from alice: Hello world
Sending to all clients
```

**Solution:**
- Refresh page and login again
- Check WebSocket connection status
- Restart backend and server

### 6. Connection Lost / Disconnected

**Symptom:**
- System message: "Connection to server lost"
- Can't send messages

**Causes:**
- TCP server crashed
- Network interruption
- Backend restarted

**Solution:**
```bash
# 1. Restart TCP server
./chat_server

# 2. Restart web backend
make run-web

# 3. In browser: Logout and login again
```

### 7. Multiple Users Testing

**Issue:** Testing with multiple users

**Solution:**

**Method 1: Multiple Browsers**
- Chrome: alice
- Firefox: bob
- Edge: charlie

**Method 2: Incognito Windows**
- Regular window: alice
- Incognito window 1: bob
- Incognito window 2: charlie

**Method 3: Multiple Devices**
- Computer: alice
- Phone: bob
- Tablet: charlie

**Note:** Must use same web backend URL for all:
```
http://localhost:5000        (same machine)
http://192.168.x.x:5000     (different machines, replace x.x with your IP)
```

## Complete System Check

Run this complete diagnostic:

```bash
#!/bin/bash
echo "=== System Check ==="

# 1. Check kernel module
echo "1. Checking kernel module..."
if lsmod | grep -q crypto_driver; then
    echo "   ✓ Kernel module loaded"
else
    echo "   ✗ Kernel module NOT loaded"
    echo "   Run: sudo make driver-load"
fi

# 2. Check device node
echo "2. Checking device node..."
if [ -e /dev/crypto_dev ]; then
    echo "   ✓ Device node exists: $(ls -l /dev/crypto_dev)"
else
    echo "   ✗ Device node NOT found"
fi

# 3. Check TCP server
echo "3. Checking TCP server..."
if pgrep -f chat_server > /dev/null; then
    echo "   ✓ TCP server running (PID: $(pgrep -f chat_server))"
else
    echo "   ✗ TCP server NOT running"
    echo "   Run: ./chat_server"
fi

# 4. Check web backend
echo "4. Checking web backend..."
if pgrep -f chat_backend > /dev/null; then
    echo "   ✓ Web backend running (PID: $(pgrep -f chat_backend))"
else
    echo "   ✗ Web backend NOT running"
    echo "   Run: make run-web"
fi

# 5. Check ports
echo "5. Checking ports..."
if netstat -tuln 2>/dev/null | grep -q ":8888 "; then
    echo "   ✓ Port 8888 (TCP server) listening"
else
    echo "   ✗ Port 8888 NOT listening"
fi

if netstat -tuln 2>/dev/null | grep -q ":5000 "; then
    echo "   ✓ Port 5000 (Web backend) listening"
else
    echo "   ✗ Port 5000 NOT listening"
fi

echo ""
echo "=== Quick Start ==="
echo "If any checks failed, run:"
echo "  sudo make driver-load    # Load kernel module"
echo "  ./chat_server            # Start TCP server (Terminal 1)"
echo "  make run-web             # Start web backend (Terminal 2)"
echo "  # Open browser: http://localhost:5000"
```

## Getting Help

If issues persist:

1. **Enable Debug Mode:**
   ```bash
   # Backend runs with debug=True by default
   # Check terminal for detailed logs
   ```

2. **Collect Information:**
   ```bash
   # System info
   uname -r                    # Kernel version
   python3 --version           # Python version
   
   # Process status
   ps aux | grep -E 'chat_server|chat_backend'
   
   # Port status
   netstat -tuln | grep -E '8888|5000'
   
   # Module status
   lsmod | grep crypto
   dmesg | tail -20           # Recent kernel messages
   ```

3. **Check Logs:**
   - Backend: Check terminal where `chat_backend.py` is running
   - Server: Check terminal where `chat_server` is running
   - Browser: Check Console (F12 → Console tab)
   - Kernel: `dmesg | tail` or `journalctl -k -f`

4. **Create Issue:**
   Include:
   - System information (kernel version, OS)
   - Error messages from all logs
   - Steps to reproduce
   - What you've already tried

## Quick Reference

| Issue | Quick Fix |
|-------|-----------|
| Login hangs | Restart server & backend |
| Wrong password | Use demo users (alice/password123, etc.) |
| Module error | `sudo make driver-load` |
| Port in use | `pkill -f chat_backend` then restart |
| Connection lost | Logout and login again |
| No messages | Refresh page |
| Backend won't start | `pip install -r requirements.txt` |

## Advanced Debugging

### Enable Verbose Logging

**Backend:**
```python
# In chat_backend.py, add at top:
import logging
logging.basicConfig(level=logging.DEBUG)
```

**Frontend:**
```javascript
// In chat.js, add after initSocket():
socket.on('*', function(event, data) {
    console.log('Socket event:', event, data);
});
```

### Test with curl

```bash
# Test if backend is responding
curl http://localhost:5000

# Should return HTML of login page
```

### Monitor Network Traffic

```bash
# Watch connections to TCP server
watch -n 1 'netstat -tn | grep 8888'

# Watch connections to web backend  
watch -n 1 'netstat -tn | grep 5000'
```

### Check Resource Usage

```bash
# If system is slow
top -p $(pgrep -f chat_server)
top -p $(pgrep -f chat_backend)
```

---

**Last Updated:** 2026-02-01  
**Status:** Production Ready  
**For more help:** See README.md, TROUBLESHOOTING.md, WEB_UI_README.md
