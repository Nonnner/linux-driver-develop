/**
 * Chat System - Client-side JavaScript
 * 
 * Handles WebSocket communication with the backend server
 * and manages the chat UI.
 */

// Configuration constants
const REGISTRATION_SUCCESS_DELAY_MS = 2000;  // Delay before switching to login after successful registration

// Global state
let socket = null;
let currentUser = null;
let targetUser = null;  // null means broadcast
let isConnected = false;

// DOM Elements
const loginScreen = document.getElementById('login-screen');
const chatScreen = document.getElementById('chat-screen');
const loginForm = document.getElementById('login-form');
const registerForm = document.getElementById('register-form');
const loginBtn = document.getElementById('login-btn');
const registerBtn = document.getElementById('register-btn');
const loginError = document.getElementById('login-error');
const registerError = document.getElementById('register-error');
const registerSuccess = document.getElementById('register-success');
const currentUserDisplay = document.getElementById('current-user');
const userList = document.getElementById('user-list');
const messagesContainer = document.getElementById('messages');
const messageForm = document.getElementById('message-form');
const messageInput = document.getElementById('message-input');
const chatTargetUser = document.getElementById('chat-target-user');
const clearTargetBtn = document.getElementById('clear-target-btn');
const connectionStatus = document.getElementById('connection-status');

/**
 * Initialize the application
 */
function init() {
    // Connect to WebSocket server
    socket = io();
    
    // Setup event listeners
    setupSocketEvents();
}

/**
 * Setup WebSocket event listeners
 */
function setupSocketEvents() {
    socket.on('connect', () => {
        console.log('Connected to backend server');
    });
    
    socket.on('connected', (data) => {
        console.log('Connected to chat server:', data);
        isConnected = true;
        updateConnectionStatus(true);
    });
    
    socket.on('disconnect', () => {
        console.log('Disconnected from backend server');
        isConnected = false;
        updateConnectionStatus(false);
    });
    
    socket.on('server_disconnected', () => {
        console.log('Chat server disconnected');
        isConnected = false;
        updateConnectionStatus(false);
        showNotification('Mất kết nối với server chat', 'error');
    });
    
    socket.on('login_response', (data) => {
        handleLoginResponse(data);
    });
    
    socket.on('logout_response', (data) => {
        handleLogoutResponse(data);
    });
    
    socket.on('new_message', (data) => {
        handleNewMessage(data);
    });
    
    socket.on('user_list', (data) => {
        handleUserList(data);
    });
    
    socket.on('register_response', (data) => {
        handleRegisterResponse(data);
    });
    
    socket.on('error', (data) => {
        showNotification(data.message || 'Có lỗi xảy ra', 'error');
    });
}

/**
 * Show login form, hide register form
 */
function showLoginForm() {
    document.getElementById('login-form').style.display = 'block';
    document.getElementById('register-form').style.display = 'none';
    document.getElementById('login-tab').classList.add('active');
    document.getElementById('register-tab').classList.remove('active');
    hideLoginError();
    hideRegisterMessages();
}

/**
 * Show register form, hide login form
 */
function showRegisterForm() {
    document.getElementById('login-form').style.display = 'none';
    document.getElementById('register-form').style.display = 'block';
    document.getElementById('login-tab').classList.remove('active');
    document.getElementById('register-tab').classList.add('active');
    hideLoginError();
    hideRegisterMessages();
}

/**
 * Handle login form submission
 */
function handleLogin(event) {
    event.preventDefault();
    
    const username = document.getElementById('username').value.trim();
    const password = document.getElementById('password').value;
    
    if (!username || !password) {
        showLoginError('Vui lòng nhập đầy đủ thông tin');
        return false;
    }
    
    // Disable login button
    loginBtn.disabled = true;
    loginBtn.innerHTML = '<span>Đang đăng nhập...</span>';
    
    // Send login request
    socket.emit('login', { username, password });
    
    return false;
}

/**
 * Handle registration form submission
 */
function handleRegister(event) {
    event.preventDefault();
    
    const username = document.getElementById('reg-username').value.trim();
    const password = document.getElementById('reg-password').value;
    const confirmPassword = document.getElementById('reg-confirm-password').value;
    
    hideRegisterMessages();
    
    if (!username || !password || !confirmPassword) {
        showRegisterError('Vui lòng nhập đầy đủ thông tin');
        return false;
    }
    
    if (password !== confirmPassword) {
        showRegisterError('Mật khẩu xác nhận không khớp');
        return false;
    }
    
    if (username.length < 3 || username.length > 20) {
        showRegisterError('Tên đăng nhập phải từ 3-20 ký tự');
        return false;
    }
    
    if (!/^[a-zA-Z0-9_]+$/.test(username)) {
        showRegisterError('Tên đăng nhập chỉ được chứa chữ cái, số và dấu gạch dưới');
        return false;
    }
    
    if (password.length < 6) {
        showRegisterError('Mật khẩu phải có ít nhất 6 ký tự');
        return false;
    }
    
    // Disable register button
    registerBtn.disabled = true;
    registerBtn.innerHTML = '<span>Đang đăng ký...</span>';
    
    // Send register request
    socket.emit('register', { username, password });
    
    return false;
}

/**
 * Handle registration response from server
 */
function handleRegisterResponse(data) {
    registerBtn.disabled = false;
    registerBtn.innerHTML = '<span>Đăng ký</span>';
    
    if (data.success) {
        showRegisterSuccess(data.message || 'Đăng ký thành công! Bạn có thể đăng nhập ngay.');
        // Clear form
        document.getElementById('reg-username').value = '';
        document.getElementById('reg-password').value = '';
        document.getElementById('reg-confirm-password').value = '';
        // Switch to login tab after delay
        setTimeout(() => {
            showLoginForm();
        }, REGISTRATION_SUCCESS_DELAY_MS);
    } else {
        showRegisterError(data.message || 'Đăng ký thất bại');
    }
}

/**
 * Handle login response from server
 */
function handleLoginResponse(data) {
    loginBtn.disabled = false;
    loginBtn.innerHTML = '<span>Đăng nhập</span>';
    
    if (data.success) {
        currentUser = data.username;
        showChatScreen();
        hideLoginError();
    } else {
        showLoginError(data.message || 'Đăng nhập thất bại');
    }
}

/**
 * Handle logout
 */
function handleLogout() {
    socket.emit('logout');
}

/**
 * Handle logout response
 */
function handleLogoutResponse(data) {
    if (data.success) {
        currentUser = null;
        targetUser = null;
        showLoginScreen();
        clearMessages();
    }
}

/**
 * Handle sending a message
 */
function handleSendMessage(event) {
    event.preventDefault();
    
    const message = messageInput.value.trim();
    if (!message) return false;
    
    // Send message
    socket.emit('send_message', {
        to: targetUser || '',  // Empty string for broadcast
        message: message
    });
    
    // Add message to UI immediately (optimistic update)
    addMessage({
        type: targetUser ? 'private' : 'broadcast',
        from: currentUser,
        to: targetUser || 'Tất cả',
        message: message,
        own: true
    });
    
    // Clear input
    messageInput.value = '';
    messageInput.focus();
    
    return false;
}

/**
 * Handle new incoming message
 */
function handleNewMessage(data) {
    addMessage({
        type: data.type,
        from: data.from,
        to: data.to,
        message: data.message,
        own: false
    });
    
    // Play notification sound (optional)
    // playNotificationSound();
}

/**
 * Handle user list update
 */
function handleUserList(data) {
    userList.innerHTML = '';
    
    if (data.users && data.users.length > 0) {
        data.users.forEach(user => {
            const li = document.createElement('li');
            li.innerHTML = `
                <span class="status-dot ${user.online ? '' : 'offline'}"></span>
                <span class="username">${escapeHtml(user.username)}</span>
            `;
            
            // Don't allow selecting self
            if (user.username !== currentUser) {
                li.onclick = () => selectUser(user.username);
            } else {
                li.style.opacity = '0.6';
                li.style.cursor = 'default';
            }
            
            if (user.username === targetUser) {
                li.classList.add('selected');
            }
            
            userList.appendChild(li);
        });
    } else {
        userList.innerHTML = '<li style="color: #6c757d; cursor: default;">Không có người dùng online</li>';
    }
}

/**
 * Select a user for private messaging
 */
function selectUser(username) {
    targetUser = username;
    chatTargetUser.textContent = username;
    clearTargetBtn.style.display = 'inline-block';
    
    // Update UI
    document.querySelectorAll('.user-list li').forEach(li => {
        li.classList.remove('selected');
        if (li.querySelector('.username')?.textContent === username) {
            li.classList.add('selected');
        }
    });
    
    messageInput.placeholder = `Gửi tin nhắn riêng đến ${username}...`;
    messageInput.focus();
}

/**
 * Clear target user (switch to broadcast mode)
 */
function clearTarget() {
    targetUser = null;
    chatTargetUser.textContent = 'Tất cả (Broadcast)';
    clearTargetBtn.style.display = 'none';
    
    // Update UI
    document.querySelectorAll('.user-list li').forEach(li => {
        li.classList.remove('selected');
    });
    
    messageInput.placeholder = 'Nhập tin nhắn...';
}

/**
 * Refresh user list
 */
function refreshUsers() {
    socket.emit('get_users');
}

/**
 * Add a message to the chat
 */
function addMessage(data) {
    // Remove welcome message if present
    const welcomeMsg = messagesContainer.querySelector('.welcome-message');
    if (welcomeMsg) {
        welcomeMsg.remove();
    }
    
    const messageDiv = document.createElement('div');
    messageDiv.className = `message ${data.own ? 'own' : (data.type === 'broadcast' ? 'broadcast' : 'other')}`;
    
    const time = new Date().toLocaleTimeString('vi-VN', { hour: '2-digit', minute: '2-digit' });
    const typeLabel = data.type === 'broadcast' ? '📢 Broadcast' : '🔒 Riêng tư';
    
    messageDiv.innerHTML = `
        <div class="message-header">
            <span class="message-sender">${escapeHtml(data.from)}</span>
            <span class="message-type">${typeLabel}</span>
        </div>
        <div class="message-content">${escapeHtml(data.message)}</div>
        <div class="message-time">${time}</div>
    `;
    
    messagesContainer.appendChild(messageDiv);
    
    // Scroll to bottom
    messagesContainer.scrollTop = messagesContainer.scrollHeight;
}

/**
 * Clear all messages
 */
function clearMessages() {
    messagesContainer.innerHTML = `
        <div class="welcome-message">
            <p>👋 Chào mừng đến với Chat Room!</p>
            <p>Nhấp vào tên người dùng để gửi tin nhắn riêng.</p>
            <p>Hoặc gửi tin nhắn broadcast đến tất cả người dùng.</p>
        </div>
    `;
}

/**
 * Show chat screen
 */
function showChatScreen() {
    loginScreen.style.display = 'none';
    chatScreen.style.display = 'flex';
    currentUserDisplay.textContent = `👤 ${currentUser}`;
    
    // Request user list
    refreshUsers();
    
    // Focus on message input
    messageInput.focus();
}

/**
 * Show login screen
 */
function showLoginScreen() {
    chatScreen.style.display = 'none';
    loginScreen.style.display = 'flex';
    
    // Clear form
    document.getElementById('username').value = '';
    document.getElementById('password').value = '';
    document.getElementById('username').focus();
}

/**
 * Show login error
 */
function showLoginError(message) {
    loginError.textContent = message;
    loginError.style.display = 'block';
}

/**
 * Hide login error
 */
function hideLoginError() {
    loginError.style.display = 'none';
}

/**
 * Show register error
 */
function showRegisterError(message) {
    registerError.textContent = message;
    registerError.style.display = 'block';
    registerSuccess.style.display = 'none';
}

/**
 * Show register success
 */
function showRegisterSuccess(message) {
    registerSuccess.textContent = message;
    registerSuccess.style.display = 'block';
    registerError.style.display = 'none';
}

/**
 * Hide register messages
 */
function hideRegisterMessages() {
    registerError.style.display = 'none';
    registerSuccess.style.display = 'none';
}

/**
 * Update connection status indicator
 */
function updateConnectionStatus(connected) {
    if (connected) {
        connectionStatus.textContent = '🟢 Đã kết nối';
        connectionStatus.className = 'status connected';
    } else {
        connectionStatus.textContent = '🔴 Mất kết nối';
        connectionStatus.className = 'status disconnected';
    }
}

/**
 * Show notification (toast)
 */
function showNotification(message, type = 'info') {
    // Simple alert for now, could be replaced with a toast library
    if (type === 'error') {
        console.error(message);
    }
    // Could implement toast notifications here
}

/**
 * Escape HTML to prevent XSS
 */
function escapeHtml(text) {
    const div = document.createElement('div');
    div.textContent = text;
    return div.innerHTML;
}

// Initialize app when DOM is loaded
document.addEventListener('DOMContentLoaded', init);
