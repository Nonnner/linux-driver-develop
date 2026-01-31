#!/usr/bin/env python3
"""
Chat Backend Server - Web API for Chat System

This backend provides HTTP REST API and WebSocket support for web clients
to communicate with the existing C chat server via TCP socket.

Architecture:
- Flask web server with SocketIO for real-time communication
- Connects to the C chat server as a client
- Bridges web clients to the TCP chat system
"""

import os
import sys
import json
import socket
import struct
import threading
import hashlib
from flask import Flask, render_template, request, jsonify, session
from flask_socketio import SocketIO, emit, join_room, leave_room
from flask_cors import CORS

# Configuration
CHAT_SERVER_HOST = os.environ.get('CHAT_SERVER_HOST', '127.0.0.1')
CHAT_SERVER_PORT = int(os.environ.get('CHAT_SERVER_PORT', 8888))
WEB_PORT = int(os.environ.get('WEB_PORT', 5000))
SECRET_KEY = os.environ.get('SECRET_KEY', 'chat-secret-key-change-in-production')
DEBUG_MODE = os.environ.get('FLASK_DEBUG', 'false').lower() == 'true'

# Warn about default secret key in production
if SECRET_KEY == 'chat-secret-key-change-in-production' and not DEBUG_MODE:
    print("WARNING: Using default SECRET_KEY. Set SECRET_KEY environment variable in production!")

# Protocol constants (must match protocol.h)
# Note: MD5 is used for password hashing to match the C server protocol.
# In production, consider migrating to bcrypt or Argon2.
MAX_USERNAME_LEN = 32
MAX_PASSWORD_LEN = 32
MAX_MESSAGE_LEN = 1024
MD5_HASH_SIZE = 16
AES_KEY_SIZE = 16
AES_BLOCK_SIZE = 16

# Message types
MSG_TYPE_LOGIN_REQ = 1
MSG_TYPE_LOGIN_RESP = 2
MSG_TYPE_LOGOUT_REQ = 3
MSG_TYPE_LOGOUT_RESP = 4
MSG_TYPE_CHAT_MSG = 5
MSG_TYPE_CHAT_ACK = 6
MSG_TYPE_USER_LIST_REQ = 7
MSG_TYPE_USER_LIST_RESP = 8
MSG_TYPE_BROADCAST = 9
MSG_TYPE_PRIVATE_MSG = 10
MSG_TYPE_ERROR = 11
MSG_TYPE_PING = 12
MSG_TYPE_PONG = 13
MSG_TYPE_REGISTER_REQ = 14
MSG_TYPE_REGISTER_RESP = 15

# Response codes
RESP_SUCCESS = 0
RESP_ERR_INVALID_CREDENTIALS = 1
RESP_ERR_USER_ALREADY_LOGGED = 2
RESP_ERR_USER_NOT_FOUND = 3
RESP_ERR_SERVER_FULL = 4
RESP_ERR_ENCRYPTION_FAILED = 5
RESP_ERR_INTERNAL_ERROR = 6
RESP_ERR_USER_ALREADY_EXISTS = 7
RESP_ERR_INVALID_USERNAME = 8
RESP_ERR_INVALID_PASSWORD = 9

# Flask app setup
app = Flask(__name__, 
            template_folder='../web/templates',
            static_folder='../web/static')
app.config['SECRET_KEY'] = SECRET_KEY
CORS(app)
socketio = SocketIO(app, cors_allowed_origins="*", async_mode='eventlet')

# Client connections (web_sid -> ChatConnection)
connections = {}
connections_lock = threading.Lock()


class ChatConnection:
    """Manages a connection to the C chat server for a web client"""
    
    def __init__(self, web_sid):
        self.web_sid = web_sid
        self.socket = None
        self.username = None
        self.is_logged_in = False
        self.session_key = None
        self.current_iv = None
        self.receiver_thread = None
        self.running = False
        
    def connect(self):
        """Connect to the C chat server"""
        try:
            self.socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.socket.connect((CHAT_SERVER_HOST, CHAT_SERVER_PORT))
            self.running = True
            
            # Start receiver thread
            self.receiver_thread = threading.Thread(target=self._receive_loop, daemon=True)
            self.receiver_thread.start()
            
            return True
        except Exception as e:
            print(f"Connection error: {e}")
            return False
    
    def disconnect(self):
        """Disconnect from the C chat server"""
        self.running = False
        if self.socket:
            try:
                self.socket.close()
            except:
                pass
            self.socket = None
    
    def login(self, username, password):
        """Send login request to the C chat server"""
        if not self.socket:
            return False, "Not connected to server"
        
        # Hash password using MD5
        password_hash = hashlib.md5(password.encode()).digest()
        
        # Build login request message
        # Header: type(1) + flags(1) + payload_len(2) + sequence(4)
        # Payload: username(32) + password_hash(16)
        
        payload = username.encode().ljust(MAX_USERNAME_LEN, b'\x00') + password_hash
        header = struct.pack('<BBHI', MSG_TYPE_LOGIN_REQ, 0, len(payload), 0)
        
        try:
            self.socket.sendall(header + payload)
            
            # Receive response
            resp_header = self.socket.recv(8)
            if len(resp_header) < 8:
                return False, "Invalid response from server"
            
            msg_type, flags, payload_len, seq = struct.unpack('<BBHI', resp_header)
            
            if msg_type != MSG_TYPE_LOGIN_RESP:
                return False, "Unexpected response type"
            
            resp_payload = self.socket.recv(payload_len)
            if len(resp_payload) < 1:
                return False, "Invalid response payload"
            
            code = resp_payload[0]
            
            if code == RESP_SUCCESS:
                self.username = username
                self.is_logged_in = True
                if len(resp_payload) >= 33:
                    self.session_key = resp_payload[1:17]
                    self.current_iv = resp_payload[17:33]
                return True, "Login successful"
            elif code == RESP_ERR_INVALID_CREDENTIALS:
                return False, "Invalid username or password"
            elif code == RESP_ERR_USER_ALREADY_LOGGED:
                return False, "User already logged in"
            else:
                return False, f"Login failed with code {code}"
                
        except Exception as e:
            return False, f"Login error: {e}"
    
    def register(self, username, password):
        """Send registration request to the C chat server"""
        if not self.socket:
            return False, "Not connected to server"
        
        # Hash password using MD5
        password_hash = hashlib.md5(password.encode()).digest()
        
        # Build registration request message
        # Header: type(1) + flags(1) + payload_len(2) + sequence(4)
        # Payload: username(32) + password_hash(16)
        
        payload = username.encode().ljust(MAX_USERNAME_LEN, b'\x00') + password_hash
        header = struct.pack('<BBHI', MSG_TYPE_REGISTER_REQ, 0, len(payload), 0)
        
        try:
            self.socket.sendall(header + payload)
            
            # Receive response
            resp_header = self.socket.recv(8)
            if len(resp_header) < 8:
                return False, "Invalid response from server"
            
            msg_type, flags, payload_len, seq = struct.unpack('<BBHI', resp_header)
            
            if msg_type != MSG_TYPE_REGISTER_RESP:
                return False, "Unexpected response type"
            
            resp_payload = self.socket.recv(payload_len)
            if len(resp_payload) < 1:
                return False, "Invalid response payload"
            
            code = resp_payload[0]
            # Get message (next 64 bytes after code)
            message = resp_payload[1:65].rstrip(b'\x00').decode('utf-8', errors='ignore') if len(resp_payload) > 1 else ''
            
            if code == RESP_SUCCESS:
                return True, message or "Registration successful"
            elif code == RESP_ERR_USER_ALREADY_EXISTS:
                return False, message or "Username already exists"
            elif code == RESP_ERR_INVALID_USERNAME:
                return False, message or "Invalid username format"
            elif code == RESP_ERR_SERVER_FULL:
                return False, message or "Server is full"
            else:
                return False, message or f"Registration failed with code {code}"
                
        except Exception as e:
            return False, f"Registration error: {e}"
    
    def logout(self):
        """Send logout request to the C chat server"""
        if not self.socket or not self.is_logged_in:
            return False
        
        try:
            header = struct.pack('<BBHI', MSG_TYPE_LOGOUT_REQ, 0, 0, 0)
            self.socket.sendall(header)
            self.is_logged_in = False
            self.username = None
            return True
        except:
            return False
    
    def send_message(self, to_user, message):
        """Send a chat message"""
        if not self.socket or not self.is_logged_in:
            return False, "Not logged in"
        
        try:
            # Build chat message payload
            # from_user(32) + to_user(32) + msg_len(2) + msg_data(1024) + iv(16)
            from_user = self.username.encode().ljust(MAX_USERNAME_LEN, b'\x00')
            to_user_bytes = (to_user or '').encode().ljust(MAX_USERNAME_LEN, b'\x00')
            msg_bytes = message.encode()[:MAX_MESSAGE_LEN]
            msg_len = len(msg_bytes)
            msg_data = msg_bytes.ljust(MAX_MESSAGE_LEN, b'\x00')
            iv = b'\x00' * 16
            
            payload = from_user + to_user_bytes + struct.pack('<H', msg_len) + msg_data + iv
            header = struct.pack('<BBHI', MSG_TYPE_CHAT_MSG, 0, len(payload), 0)
            
            self.socket.sendall(header + payload)
            return True, "Message sent"
        except Exception as e:
            return False, f"Send error: {e}"
    
    def request_user_list(self):
        """Request the list of online users"""
        if not self.socket or not self.is_logged_in:
            return False
        
        try:
            header = struct.pack('<BBHI', MSG_TYPE_USER_LIST_REQ, 0, 0, 0)
            self.socket.sendall(header)
            return True
        except:
            return False
    
    def _receive_loop(self):
        """Background thread to receive messages from the C chat server"""
        while self.running and self.socket:
            try:
                # Receive header
                header = self.socket.recv(8)
                if len(header) < 8:
                    break
                
                msg_type, flags, payload_len, seq = struct.unpack('<BBHI', header)
                
                # Receive payload
                payload = b''
                if payload_len > 0:
                    payload = self.socket.recv(payload_len)
                
                # Process message
                self._process_message(msg_type, payload)
                
            except Exception as e:
                if self.running:
                    print(f"Receive error: {e}")
                break
        
        # Notify web client of disconnection
        if self.running:
            socketio.emit('server_disconnected', room=self.web_sid)
    
    def _process_message(self, msg_type, payload):
        """Process a received message from the C chat server"""
        if msg_type in [MSG_TYPE_BROADCAST, MSG_TYPE_PRIVATE_MSG, MSG_TYPE_CHAT_MSG]:
            # Parse chat message
            if len(payload) >= 66:
                from_user = payload[0:32].rstrip(b'\x00').decode('utf-8', errors='ignore')
                to_user = payload[32:64].rstrip(b'\x00').decode('utf-8', errors='ignore')
                msg_len = struct.unpack('<H', payload[64:66])[0]
                msg_data = payload[66:66+msg_len].decode('utf-8', errors='ignore')
                
                # Emit to web client
                socketio.emit('new_message', {
                    'type': 'broadcast' if msg_type == MSG_TYPE_BROADCAST else 'private',
                    'from': from_user,
                    'to': to_user,
                    'message': msg_data,
                    'timestamp': None
                }, room=self.web_sid)
        
        elif msg_type == MSG_TYPE_USER_LIST_RESP:
            # Parse user list
            if len(payload) >= 1:
                user_count = payload[0]
                users = []
                offset = 1
                for i in range(user_count):
                    if offset + 33 <= len(payload):
                        username = payload[offset:offset+32].rstrip(b'\x00').decode('utf-8', errors='ignore')
                        status = payload[offset+32]
                        users.append({'username': username, 'online': status == 1})
                        offset += 33
                
                socketio.emit('user_list', {'users': users}, room=self.web_sid)
        
        elif msg_type == MSG_TYPE_ERROR:
            if len(payload) >= 1:
                code = payload[0]
                message = payload[1:257].rstrip(b'\x00').decode('utf-8', errors='ignore') if len(payload) > 1 else ''
                socketio.emit('error', {'code': code, 'message': message}, room=self.web_sid)


# Flask routes
@app.route('/')
def index():
    """Serve the main chat page"""
    return render_template('index.html')


@app.route('/api/health')
def health():
    """Health check endpoint"""
    return jsonify({'status': 'ok', 'chat_server': f'{CHAT_SERVER_HOST}:{CHAT_SERVER_PORT}'})


# SocketIO event handlers
@socketio.on('connect')
def handle_connect():
    """Handle new WebSocket connection"""
    print(f"Web client connected: {request.sid}")
    
    # Create connection to C chat server
    conn = ChatConnection(request.sid)
    if conn.connect():
        with connections_lock:
            connections[request.sid] = conn
        emit('connected', {'status': 'connected'})
    else:
        emit('error', {'message': 'Could not connect to chat server'})


@socketio.on('disconnect')
def handle_disconnect():
    """Handle WebSocket disconnection"""
    print(f"Web client disconnected: {request.sid}")
    
    with connections_lock:
        conn = connections.pop(request.sid, None)
    
    if conn:
        conn.logout()
        conn.disconnect()


@socketio.on('login')
def handle_login(data):
    """Handle login request from web client"""
    username = data.get('username', '').strip()
    password = data.get('password', '')
    
    if not username or not password:
        emit('login_response', {'success': False, 'message': 'Username and password required'})
        return
    
    with connections_lock:
        conn = connections.get(request.sid)
    
    if not conn:
        emit('login_response', {'success': False, 'message': 'Not connected to server'})
        return
    
    success, message = conn.login(username, password)
    emit('login_response', {'success': success, 'message': message, 'username': username if success else None})
    
    # Request user list after successful login
    if success:
        conn.request_user_list()


@socketio.on('register')
def handle_register(data):
    """Handle registration request from web client"""
    username = data.get('username', '').strip()
    password = data.get('password', '')
    
    if not username or not password:
        emit('register_response', {'success': False, 'message': 'Username and password required'})
        return
    
    # Validate username format (alphanumeric and underscore only)
    import re
    if not re.match(r'^[a-zA-Z0-9_]+$', username):
        emit('register_response', {'success': False, 'message': 'Username can only contain letters, numbers, and underscores'})
        return
    
    if len(username) < 3 or len(username) > 20:
        emit('register_response', {'success': False, 'message': 'Username must be 3-20 characters'})
        return
    
    if len(password) < 6:
        emit('register_response', {'success': False, 'message': 'Password must be at least 6 characters'})
        return
    
    with connections_lock:
        conn = connections.get(request.sid)
    
    if not conn:
        emit('register_response', {'success': False, 'message': 'Not connected to server'})
        return
    
    success, message = conn.register(username, password)
    emit('register_response', {'success': success, 'message': message})


@socketio.on('logout')
def handle_logout():
    """Handle logout request from web client"""
    with connections_lock:
        conn = connections.get(request.sid)
    
    if conn:
        conn.logout()
        emit('logout_response', {'success': True})


@socketio.on('send_message')
def handle_send_message(data):
    """Handle message send request from web client"""
    to_user = data.get('to', '')  # Empty for broadcast
    message = data.get('message', '').strip()
    
    if not message:
        emit('error', {'message': 'Message cannot be empty'})
        return
    
    with connections_lock:
        conn = connections.get(request.sid)
    
    if not conn or not conn.is_logged_in:
        emit('error', {'message': 'Not logged in'})
        return
    
    success, msg = conn.send_message(to_user, message)
    if not success:
        emit('error', {'message': msg})


@socketio.on('get_users')
def handle_get_users():
    """Handle user list request from web client"""
    with connections_lock:
        conn = connections.get(request.sid)
    
    if conn and conn.is_logged_in:
        conn.request_user_list()


if __name__ == '__main__':
    print(f"Starting Chat Backend Server...")
    print(f"Chat Server: {CHAT_SERVER_HOST}:{CHAT_SERVER_PORT}")
    print(f"Web Server: http://0.0.0.0:{WEB_PORT}")
    print(f"Debug Mode: {DEBUG_MODE}")
    
    socketio.run(app, host='0.0.0.0', port=WEB_PORT, debug=DEBUG_MODE)
