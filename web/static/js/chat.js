// WebSocket connection
let socket;
let currentUsername = null;

// Initialize socket connection
function initSocket() {
    socket = io();
    
    socket.on('connect', function() {
        console.log('Connected to server');
    });
    
    socket.on('disconnect', function() {
        console.log('Disconnected from server');
        addSystemMessage('Disconnected from server');
    });
    
    socket.on('connected', function(data) {
        console.log('Socket connected:', data);
    });
    
    socket.on('login_response', function(data) {
        handleLoginResponse(data);
    });
    
    socket.on('chat_message', function(data) {
        handleChatMessage(data);
    });
    
    socket.on('user_list', function(data) {
        updateUserList(data.users);
    });
    
    socket.on('error', function(data) {
        addSystemMessage('Error: ' + data.message);
    });
    
    socket.on('disconnect_event', function(data) {
        addSystemMessage(data.message);
    });
}

// Handle login
document.getElementById('login-form').addEventListener('submit', function(e) {
    e.preventDefault();
    
    const username = document.getElementById('username').value.trim();
    const password = document.getElementById('password').value.trim();
    
    if (!username || !password) {
        showLoginError('Please enter username and password');
        return;
    }
    
    // Clear previous error and show loading
    hideLoginError();
    const submitBtn = this.querySelector('button[type="submit"]');
    const originalText = submitBtn.textContent;
    submitBtn.disabled = true;
    submitBtn.textContent = 'Logging in...';
    
    console.log('Sending login request for:', username);
    
    // Send login request
    socket.emit('login', {
        username: username,
        password: password
    });
    
    // Re-enable button after timeout
    setTimeout(function() {
        submitBtn.disabled = false;
        submitBtn.textContent = originalText;
    }, 10000);
});

// Handle login response
function handleLoginResponse(data) {
    console.log('Login response received:', data);
    
    // Re-enable login button
    const submitBtn = document.querySelector('#login-form button[type="submit"]');
    if (submitBtn) {
        submitBtn.disabled = false;
        submitBtn.textContent = 'Login';
    }
    
    if (data.success) {
        console.log('Login successful for:', data.username);
        currentUsername = data.username;
        
        // Hide login screen, show chat screen
        document.getElementById('login-screen').style.display = 'none';
        document.getElementById('chat-screen').style.display = 'block';
        
        // Update username display
        document.getElementById('current-username').textContent = currentUsername;
        document.getElementById('current-user-sidebar').textContent = currentUsername;
        
        // Add welcome message
        addSystemMessage(data.message);
        
        // Request user list
        socket.emit('get_users');
        
        // Focus on message input
        document.getElementById('message-input').focus();
        
        console.log('Switched to chat interface');
    } else {
        console.log('Login failed:', data.message);
        showLoginError(data.message);
    }
}

// Handle incoming chat message
function handleChatMessage(data) {
    const message = data.message;
    
    // Parse different message formats
    if (message.startsWith('[PRIVATE from ')) {
        // Private message received: [PRIVATE from alice] message
        const match = message.match(/^\[PRIVATE from ([^\]]+)\]\s*(.*)$/);
        if (match) {
            const sender = match[1];
            const text = match[2];
            addPrivateMessage(sender, text, data.timestamp, false);
        }
    } else if (message.startsWith('[PRIVATE to ')) {
        // Private message sent confirmation: [PRIVATE to bob] message
        const match = message.match(/^\[PRIVATE to ([^\]]+)\]\s*(.*)$/);
        if (match) {
            const recipient = match[1];
            const text = match[2];
            addPrivateMessage(recipient, text, data.timestamp, true);
        }
    } else {
        // Regular message: "[username] message" or "[SERVER] message"
        const match = message.match(/^\[([^\]]+)\]\s*(.*)$/);
        
        if (match) {
            const sender = match[1];
            const text = match[2];
            
            if (sender === 'SERVER') {
                addSystemMessage(text);
            } else {
                addMessage(sender, text, data.timestamp, sender === currentUsername);
            }
        } else {
            // If format doesn't match, show as system message
            addSystemMessage(message);
        }
    }
}

// Send message
document.getElementById('message-form').addEventListener('submit', function(e) {
    e.preventDefault();
    
    const messageInput = document.getElementById('message-input');
    const message = messageInput.value.trim();
    
    if (!message) {
        return;
    }
    
    // Check if sending private message
    if (selectedUser) {
        // Send private message
        socket.emit('send_private_message', {
            target_user: selectedUser,
            message: message
        });
    } else {
        // Send broadcast message
        socket.emit('send_message', {
            message: message
        });
    }
    
    // Clear input
    messageInput.value = '';
    messageInput.focus();
});

// Logout
document.getElementById('logout-btn').addEventListener('click', function() {
    if (confirm('Are you sure you want to logout?')) {
        socket.disconnect();
        
        // Reset state
        currentUsername = null;
        document.getElementById('messages').innerHTML = '';
        
        // Show login screen
        document.getElementById('chat-screen').style.display = 'none';
        document.getElementById('login-screen').style.display = 'block';
        
        // Clear form
        document.getElementById('username').value = '';
        document.getElementById('password').value = '';
        
        // Reconnect socket
        setTimeout(initSocket, 500);
    }
});

// UI Helper Functions
function showLoginError(message) {
    const errorDiv = document.getElementById('login-error');
    errorDiv.textContent = message;
    errorDiv.style.display = 'block';
}

function hideLoginError() {
    const errorDiv = document.getElementById('login-error');
    errorDiv.style.display = 'none';
}

function addMessage(sender, text, timestamp, isOwnMessage) {
    const messagesDiv = document.getElementById('messages');
    
    const messageDiv = document.createElement('div');
    messageDiv.className = 'message' + (isOwnMessage ? ' own' : '');
    
    const messageContent = document.createElement('div');
    messageContent.className = 'message-content';
    
    if (!isOwnMessage) {
        const senderDiv = document.createElement('div');
        senderDiv.className = 'message-sender';
        senderDiv.textContent = sender;
        messageContent.appendChild(senderDiv);
    }
    
    const textDiv = document.createElement('div');
    textDiv.className = 'message-text';
    textDiv.textContent = text;
    messageContent.appendChild(textDiv);
    
    const timeDiv = document.createElement('div');
    timeDiv.className = 'message-time';
    timeDiv.textContent = timestamp || new Date().toLocaleTimeString();
    messageContent.appendChild(timeDiv);
    
    messageDiv.appendChild(messageContent);
    messagesDiv.appendChild(messageDiv);
    
    // Clear float
    const clearDiv = document.createElement('div');
    clearDiv.style.clear = 'both';
    messagesDiv.appendChild(clearDiv);
    
    // Scroll to bottom
    messagesDiv.scrollTop = messagesDiv.scrollHeight;
}

function addSystemMessage(text) {
    const messagesDiv = document.getElementById('messages');
    
    const messageDiv = document.createElement('div');
    messageDiv.className = 'message system';
    
    const messageContent = document.createElement('div');
    messageContent.className = 'message-content';
    
    const textDiv = document.createElement('div');
    textDiv.className = 'message-text';
    textDiv.textContent = text;
    messageContent.appendChild(textDiv);
    
    messageDiv.appendChild(messageContent);
    messagesDiv.appendChild(messageDiv);
    
    // Scroll to bottom
    messagesDiv.scrollTop = messagesDiv.scrollHeight;
}

function addPrivateMessage(otherUser, text, timestamp, isSent) {
    const messagesDiv = document.getElementById('messages');
    
    const messageDiv = document.createElement('div');
    messageDiv.className = 'message private' + (isSent ? ' own' : '');
    
    const messageContent = document.createElement('div');
    messageContent.className = 'message-content';
    
    const senderDiv = document.createElement('div');
    senderDiv.className = 'message-sender';
    senderDiv.textContent = isSent ? `Private to ${otherUser}` : `Private from ${otherUser}`;
    senderDiv.style.fontStyle = 'italic';
    senderDiv.style.fontSize = '0.85em';
    senderDiv.style.color = '#9333ea';
    messageContent.appendChild(senderDiv);
    
    const textDiv = document.createElement('div');
    textDiv.className = 'message-text';
    textDiv.textContent = text;
    messageContent.appendChild(textDiv);
    
    const timeDiv = document.createElement('div');
    timeDiv.className = 'message-time';
    timeDiv.textContent = timestamp || new Date().toLocaleTimeString();
    messageContent.appendChild(timeDiv);
    
    messageDiv.appendChild(messageContent);
    messagesDiv.appendChild(messageDiv);
    
    // Clear float
    const clearDiv = document.createElement('div');
    clearDiv.style.clear = 'both';
    messagesDiv.appendChild(clearDiv);
    
    // Scroll to bottom
    messagesDiv.scrollTop = messagesDiv.scrollHeight;
}

// Global variable to track selected user for private chat
let selectedUser = null;

function updateUserList(users) {
    const userListDiv = document.getElementById('user-list');
    
    // Keep current user at top
    const currentUserHtml = `
        <div class="user-item current-user">
            <span class="user-status online"></span>
            <span>${currentUsername} (you)</span>
        </div>
    `;
    
    // Other users
    let otherUsersHtml = '<div style="margin-top: 10px; font-size: 0.9em; color: #666; padding: 5px;">Online Users (click to chat):</div>';
    users.forEach(user => {
        if (user !== currentUsername) {
            const isSelected = user === selectedUser;
            otherUsersHtml += `
                <div class="user-item ${isSelected ? 'selected' : ''}" data-username="${user}" onclick="selectUser('${user}')">
                    <span class="user-status online"></span>
                    <span>${user}</span>
                </div>
            `;
        }
    });
    
    userListDiv.innerHTML = currentUserHtml + otherUsersHtml;
}

// Select user for private chat
function selectUser(username) {
    selectedUser = username;
    console.log('Selected user for private chat:', username);
    
    // Update UI to show selection
    document.querySelectorAll('.user-item').forEach(item => {
        item.classList.remove('selected');
    });
    const selectedItem = document.querySelector(`[data-username="${username}"]`);
    if (selectedItem) {
        selectedItem.classList.add('selected');
    }
    
    // Update message input placeholder
    const messageInput = document.getElementById('message-input');
    messageInput.placeholder = `Private message to ${username}...`;
    messageInput.focus();
    
    // Show info message
    addSystemMessage(`Now chatting privately with ${username}. Type a message or click another user to switch.`);
}

// Add button to clear selection and broadcast to all
function clearUserSelection() {
    selectedUser = null;
    document.querySelectorAll('.user-item').forEach(item => {
        item.classList.remove('selected');
    });
    const messageInput = document.getElementById('message-input');
    messageInput.placeholder = 'Type a message...';
    addSystemMessage('Broadcasting to all users now.');
}

// Make selectUser available globally
window.selectUser = selectUser;
    `;
    
    const otherUsersHtml = users
        .filter(u => u && u !== currentUsername)
        .map(user => `
            <div class="user-item">
                <span class="user-status online"></span>
                <span>${user}</span>
            </div>
        `).join('');
    
    userListDiv.innerHTML = currentUserHtml + otherUsersHtml;
}

// Initialize on page load
document.addEventListener('DOMContentLoaded', function() {
    initSocket();
});
