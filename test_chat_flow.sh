#!/bin/bash
# Test script for chat flow functionality
# Tests the logic without requiring actual kernel module

echo "==================================="
echo "Chat Flow Test Script"
echo "==================================="
echo ""

# Test 1: Check if binaries are built
echo "Test 1: Checking if binaries exist..."
if [ -f "./chat_server" ] && [ -f "./chat_client" ]; then
    echo "✅ PASS: Binaries exist"
else
    echo "❌ FAIL: Binaries missing, building..."
    make
fi
echo ""

# Test 2: Check if web files exist
echo "Test 2: Checking web interface files..."
web_files=(
    "web/templates/index.html"
    "web/static/js/chat.js"
    "web/static/css/style.css"
    "backend/chat_backend.py"
)

all_exist=true
for file in "${web_files[@]}"; do
    if [ -f "$file" ]; then
        echo "✅ $file exists"
    else
        echo "❌ $file missing"
        all_exist=false
    fi
done

if [ "$all_exist" = true ]; then
    echo "✅ PASS: All web files exist"
else
    echo "❌ FAIL: Some web files missing"
fi
echo ""

# Test 3: Check JavaScript syntax
echo "Test 3: Checking JavaScript syntax..."
if command -v node &> /dev/null; then
    node -c web/static/js/chat.js 2>&1
    if [ $? -eq 0 ]; then
        echo "✅ PASS: JavaScript syntax valid"
    else
        echo "❌ FAIL: JavaScript syntax errors"
    fi
else
    echo "⚠️  SKIP: Node.js not installed, cannot check JS syntax"
fi
echo ""

# Test 4: Check Python syntax
echo "Test 4: Checking Python syntax..."
if command -v python3 &> /dev/null; then
    python3 -m py_compile backend/chat_backend.py 2>&1
    if [ $? -eq 0 ]; then
        echo "✅ PASS: Python syntax valid"
    else
        echo "❌ FAIL: Python syntax errors"
    fi
else
    echo "❌ FAIL: Python3 not installed"
fi
echo ""

# Test 5: Check for required features in chat.js
echo "Test 5: Checking for required chat flow features..."
features=(
    "selectUser"
    "updateUserList"
    "send_private_message"
    "user_list"
    "login_response"
)

for feature in "${features[@]}"; do
    if grep -q "$feature" web/static/js/chat.js; then
        echo "✅ Feature found: $feature"
    else
        echo "❌ Feature missing: $feature"
    fi
done
echo ""

# Test 6: Check HTML structure
echo "Test 6: Checking HTML structure..."
html_elements=(
    "user-list"
    "message-input"
    "login-form"
    "chat-screen"
    "broadcast-btn"
)

for element in "${html_elements[@]}"; do
    if grep -q "$element" web/templates/index.html; then
        echo "✅ HTML element found: $element"
    else
        echo "❌ HTML element missing: $element"
    fi
done
echo ""

# Test 7: Check backend handlers
echo "Test 7: Checking backend socket handlers..."
handlers=(
    "login"
    "send_private_message"
    "request_user_list"
    "send_message"
)

for handler in "${handlers[@]}"; do
    if grep -q "$handler" backend/chat_backend.py; then
        echo "✅ Handler found: $handler"
    else
        echo "❌ Handler missing: $handler"
    fi
done
echo ""

echo "==================================="
echo "Test Summary"
echo "==================================="
echo ""
echo "Manual Testing Steps:"
echo ""
echo "1. Load kernel module:"
echo "   sudo make driver-load"
echo ""
echo "2. Start TCP server (Terminal 1):"
echo "   ./chat_server"
echo ""
echo "3. Start web backend (Terminal 2):"
echo "   make run-web"
echo ""
echo "4. Open browsers:"
echo "   - Browser 1: http://localhost:5000 (login as alice)"
echo "   - Browser 2: http://localhost:5000 (login as bob)"
echo "   - Browser 3: http://localhost:5000 (login as charlie)"
echo ""
echo "5. Test user flow:"
echo "   ✓ Login → Should see user list"
echo "   ✓ Click on user → User highlighted"
echo "   ✓ Send message → Only selected user receives"
echo "   ✓ Click broadcast → Message to all"
echo ""
echo "Expected Results:"
echo "✅ User can login"
echo "✅ User can see online users list"
echo "✅ User can select another user (click to highlight)"
echo "✅ User can send private message to selected user"
echo "✅ User can broadcast to all users"
echo ""
