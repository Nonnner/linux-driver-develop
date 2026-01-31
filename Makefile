# Main Makefile for Multi-user Chat System
#
# This Makefile builds all components of the chat system:
# - Crypto Device Driver (kernel module)
# - Chat Server (user space)
# - Chat Client (user space)
# - Web Backend (Python Flask)
# - Web UI (HTML/CSS/JS)

.PHONY: all clean driver server client backend install uninstall help run-server run-client run-backend run-web

# Default target - build user space components only
all: server client

# Build all including kernel module (requires kernel headers)
full: driver server client

# Build kernel module
driver:
	@echo "Building crypto driver..."
	$(MAKE) -C driver

# Build chat server
server:
	@echo "Building chat server..."
	$(MAKE) -C server

# Build chat client
client:
	@echo "Building chat client..."
	$(MAKE) -C client

# Setup Python backend (install dependencies)
backend:
	@echo "Setting up Python backend..."
	@if command -v pip3 >/dev/null 2>&1; then \
		pip3 install -r backend/requirements.txt; \
	else \
		pip install -r backend/requirements.txt; \
	fi

# Clean all build artifacts
clean:
	@echo "Cleaning all..."
	$(MAKE) -C driver clean || true
	$(MAKE) -C server clean
	$(MAKE) -C client clean
	rm -rf backend/__pycache__ backend/*.pyc

# Install driver (requires root)
install:
	@echo "Installing crypto driver..."
	$(MAKE) -C driver install

# Uninstall driver (requires root)
uninstall:
	@echo "Uninstalling crypto driver..."
	$(MAKE) -C driver uninstall

# Run TCP chat server
run-server: server
	./server/chat_server

# Run terminal chat client
run-client: client
	./client/chat_client

# Run web backend server (connects to TCP chat server)
run-backend: backend
	@echo "Starting web backend on http://localhost:5000"
	@echo "Make sure the TCP chat server is running first!"
	cd backend && python3 chat_backend.py

# Run both TCP server and web backend
run-web: server
	@echo "Starting TCP chat server and Web backend..."
	@echo "Web UI will be available at http://localhost:5000"
	./server/chat_server & sleep 1 && cd backend && python3 chat_backend.py

# Help
help:
	@echo "=== Multi-user Chat System Build ==="
	@echo ""
	@echo "Targets:"
	@echo "  all          - Build TCP server and client (default)"
	@echo "  full         - Build all including kernel driver"
	@echo "  driver       - Build kernel module only"
	@echo "  server       - Build TCP chat server only"
	@echo "  client       - Build terminal chat client only"
	@echo "  backend      - Setup Python backend dependencies"
	@echo "  clean        - Remove all build artifacts"
	@echo "  install      - Install kernel module (requires sudo)"
	@echo "  uninstall    - Uninstall kernel module (requires sudo)"
	@echo "  run-server   - Build and run the TCP server"
	@echo "  run-client   - Build and run the terminal client"
	@echo "  run-backend  - Run the web backend server"
	@echo "  run-web      - Run both TCP server and web backend"
	@echo "  help         - Show this help message"
	@echo ""
	@echo "Quick Start (Terminal Client):"
	@echo "  1. Build:      make"
	@echo "  2. Terminal 1: ./server/chat_server"
	@echo "  3. Terminal 2: ./client/chat_client"
	@echo ""
	@echo "Quick Start (Web UI):"
	@echo "  1. Build:      make && make backend"
	@echo "  2. Terminal 1: ./server/chat_server"
	@echo "  3. Terminal 2: cd backend && python3 chat_backend.py"
	@echo "  4. Browser:    http://localhost:5000"
	@echo ""
	@echo "Test Users: alice/password123, bob/secret456, charlie/test789"
