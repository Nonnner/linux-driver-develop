/**
 * Chat System - Client-side JavaScript
 * 
 * Handles WebSocket communication with the backend server
 * and manages the chat UI.
 */

// Global state
let socket = null;
let currentUser = null;
let targetUser = null;  // null means broadcast
let isConnected = false;

// DOM Elements
const loginScreen = document.getElementById('login-screen');
const chatScreen = document.getElementById('chat-screen');
const loginForm = document.getElementById('login-form');
const loginBtn = document.getElementById('login-btn');
const loginError = document.getElementById('login-error');
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
    
    socket.on('error', (data) => {
        showNotification(data.message || 'Có lỗi xảy ra', 'error');
    });
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
