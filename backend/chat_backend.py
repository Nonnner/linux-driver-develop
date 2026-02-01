#!/usr/bin/env python3
"""
Web Backend Server for Multi-User Chat System
Provides a web interface bridge to the TCP chat server
Using Flask and SocketIO for real-time communication
"""

from flask import Flask, render_template, request, session
from flask_socketio import SocketIO, emit, join_room, leave_room
import socket
import threading
import time
import struct
import hashlib
from datetime import datetime

app = Flask(__name__, 
            template_folder='../web/templates',
            static_folder='../web/static')
app.config['SECRET_KEY'] = 'your-secret-key-change-in-production'
socketio = SocketIO(app, cors_allowed_origins="*", async_mode='eventlet')

# Configuration
TCP_SERVER_HOST = '127.0.0.1'
TCP_SERVER_PORT = 8888

# Store active connections: {session_id: tcp_socket}
active_connections = {}
connection_lock = threading.Lock()

# Demo users (same as chat_server.c)
DEMO_USERS = {
    'alice': 'password123',
    'bob': 'password456',
    'charlie': 'password789'
}


def hash_password(password):
    """Hash password with MD5 (matching server behavior)"""
    return hashlib.md5(password.encode()).hexdigest()


def connect_to_tcp_server():
    """Create connection to TCP chat server"""
    try:
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.settimeout(10)  # 10 second timeout for operations
        sock.connect((TCP_SERVER_HOST, TCP_SERVER_PORT))
        return sock
    except Exception as e:
        print(f"Failed to connect to TCP server: {e}")
        return None


def recv_until(sock, expected_text, max_bytes=1024, timeout=5):
    """Receive data until expected text is found or timeout"""
    buffer = b''
    start_time = time.time()
    
    while time.time() - start_time < timeout:
        try:
            sock.settimeout(1)
            chunk = sock.recv(max_bytes)
            if not chunk:
                break
            buffer += chunk
            
            # Check if we received the expected text
            decoded = buffer.decode('utf-8', errors='ignore')
            if expected_text in decoded:
                return decoded
                
        except socket.timeout:
            # Check what we have so far
            decoded = buffer.decode('utf-8', errors='ignore')
            if expected_text in decoded:
                return decoded
            continue
        except Exception as e:
            print(f"recv_until error: {e}")
            break
    
    # Return whatever we got
    return buffer.decode('utf-8', errors='ignore')


def tcp_receiver(session_id, tcp_sock, websocket_sid):
    """Receive messages from TCP server and forward to WebSocket"""
    buffer = b''
    try:
        while True:
            data = tcp_sock.recv(4096)
            if not data:
                break
            
            # Decode and forward to web client
            try:
                message = data.decode('utf-8', errors='ignore').strip()
                if message:
                    # Send to specific web client
                    socketio.emit('chat_message', {
                        'message': message,
                        'timestamp': datetime.now().strftime('%H:%M:%S')
                    }, room=websocket_sid)
            except Exception as e:
                print(f"Error processing message: {e}")
                
    except Exception as e:
        print(f"TCP receiver error: {e}")
    finally:
        # Cleanup
        with connection_lock:
            if session_id in active_connections:
                del active_connections[session_id]
        socketio.emit('disconnect_event', {
            'message': 'Connection to server lost'
        }, room=websocket_sid)


@app.route('/')
def index():
    """Serve the main chat page"""
    return render_template('index.html')


@socketio.on('connect')
def handle_connect():
    """Handle WebSocket connection"""
    print(f'Client connected: {request.sid}')
    emit('connected', {'status': 'connected'})


@socketio.on('disconnect')
def handle_disconnect():
    """Handle WebSocket disconnection"""
    print(f'Client disconnected: {request.sid}')
    
    # Close TCP connection if exists
    session_id = session.get('session_id')
    if session_id:
        with connection_lock:
            if session_id in active_connections:
                try:
                    active_connections[session_id].close()
                except:
                    pass
                del active_connections[session_id]


@socketio.on('login')
def handle_login(data):
    """Handle user login"""
    username = data.get('username', '').strip()
    password = data.get('password', '').strip()
    
    print(f"[LOGIN] Attempt from user: {username}")
    
    # Validate credentials
    if username not in DEMO_USERS or DEMO_USERS[username] != password:
        print(f"[LOGIN] Invalid credentials for: {username}")
        emit('login_response', {
            'success': False,
            'message': 'Invalid username or password'
        })
        return
    
    # Connect to TCP server
    tcp_sock = connect_to_tcp_server()
    if not tcp_sock:
        print(f"[LOGIN] Failed to connect to TCP server")
        emit('login_response', {
            'success': False,
            'message': 'Failed to connect to chat server. Make sure the server is running.'
        })
        return
    
    try:
        print(f"[LOGIN] Connected to TCP server, starting authentication...")
        
        # Wait for and send username
        prompt = recv_until(tcp_sock, "USERNAME:", timeout=3)
        print(f"[LOGIN] Received prompt: {repr(prompt[:50])}")
        
        if "USERNAME:" not in prompt:
            raise Exception("Did not receive USERNAME prompt")
        
        print(f"[LOGIN] Sending username: {username}")
        tcp_sock.sendall(f"{username}\n".encode())
        
        # Wait for and send password
        prompt = recv_until(tcp_sock, "PASSWORD:", timeout=3)
        print(f"[LOGIN] Received prompt: {repr(prompt[:50])}")
        
        if "PASSWORD:" not in prompt:
            raise Exception("Did not receive PASSWORD prompt")
            
        print(f"[LOGIN] Sending password")
        tcp_sock.sendall(f"{password}\n".encode())
        
        # Receive authentication result
        print(f"[LOGIN] Waiting for authentication result...")
        auth_result = recv_until(tcp_sock, "AUTH_", timeout=5)
        print(f"[LOGIN] Auth result: {repr(auth_result[:100])}")
        
        if 'AUTH_SUCCESS' in auth_result:
            # Store session
            session_id = f"{username}_{request.sid}_{time.time()}"
            session['session_id'] = session_id
            session['username'] = username
            
            with connection_lock:
                active_connections[session_id] = tcp_sock
            
            print(f"[LOGIN] User {username} authenticated successfully")
            
            # Start receiver thread
            receiver_thread = threading.Thread(
                target=tcp_receiver,
                args=(session_id, tcp_sock, request.sid),
                daemon=True
            )
            receiver_thread.start()
            
            emit('login_response', {
                'success': True,
                'username': username,
                'message': f'Welcome, {username}!'
            })
            print(f"[LOGIN] Login response sent to client")
        else:
            print(f"[LOGIN] Authentication failed for {username}")
            tcp_sock.close()
            emit('login_response', {
                'success': False,
                'message': 'Authentication failed'
            })
            
    except Exception as e:
        print(f"[LOGIN] Error during login: {e}")
        import traceback
        traceback.print_exc()
        try:
            tcp_sock.close()
        except:
            pass
        emit('login_response', {
            'success': False,
            'message': f'Login error: {str(e)}'
        })


@socketio.on('send_message')
def handle_send_message(data):
    """Handle sending a message"""
    message = data.get('message', '').strip()
    session_id = session.get('session_id')
    username = session.get('username')
    
    if not session_id or not username:
        emit('error', {'message': 'Not logged in'})
        return
    
    if not message:
        return
    
    # Get TCP connection
    with connection_lock:
        tcp_sock = active_connections.get(session_id)
    
    if not tcp_sock:
        emit('error', {'message': 'Connection lost'})
        return
    
    try:
        # Send message to TCP server
        tcp_sock.sendall(f"{message}\n".encode())
        print(f"Message from {username}: {message}")
    except Exception as e:
        print(f"Error sending message: {e}")
        emit('error', {'message': 'Failed to send message'})


@socketio.on('get_users')
def handle_get_users():
    """Handle request for user list"""
    # For now, return demo users
    # In a full implementation, this would query the server
    with connection_lock:
        online_users = [session.get('username') for sid, sock in active_connections.items()]
    
    emit('user_list', {'users': list(set(online_users))})


if __name__ == '__main__':
    print("=" * 60)
    print("Web Backend Server for Multi-User Chat System")
    print("=" * 60)
    print(f"TCP Server: {TCP_SERVER_HOST}:{TCP_SERVER_PORT}")
    print("Web Interface: http://localhost:5000")
    print("Demo users: alice/password123, bob/password456, charlie/password789")
    print("=" * 60)
    
    socketio.run(app, host='0.0.0.0', port=5000, debug=True)
