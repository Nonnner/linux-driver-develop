# Main Makefile for Multi-user Chat System
#
# This Makefile builds all components of the chat system:
# - Crypto Device Driver (kernel module)
# - Chat Server (user space)
# - Chat Client (user space)

.PHONY: all clean driver server client install uninstall help

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

# Clean all build artifacts
clean:
	@echo "Cleaning all..."
	$(MAKE) -C driver clean || true
	$(MAKE) -C server clean
	$(MAKE) -C client clean

# Install driver (requires root)
install:
	@echo "Installing crypto driver..."
	$(MAKE) -C driver install

# Uninstall driver (requires root)
uninstall:
	@echo "Uninstalling crypto driver..."
	$(MAKE) -C driver uninstall

# Run server
run-server: server
	./server/chat_server

# Run client
run-client: client
	./client/chat_client

# Help
help:
	@echo "=== Multi-user Chat System Build ==="
	@echo ""
	@echo "Targets:"
	@echo "  all          - Build server and client (default)"
	@echo "  full         - Build all including kernel driver"
	@echo "  driver       - Build kernel module only"
	@echo "  server       - Build chat server only"
	@echo "  client       - Build chat client only"
	@echo "  clean        - Remove all build artifacts"
	@echo "  install      - Install kernel module (requires sudo)"
	@echo "  uninstall    - Uninstall kernel module (requires sudo)"
	@echo "  run-server   - Build and run the server"
	@echo "  run-client   - Build and run the client"
	@echo "  help         - Show this help message"
	@echo ""
	@echo "Quick Start:"
	@echo "  1. Build:    make"
	@echo "  2. Terminal 1: ./server/chat_server"
	@echo "  3. Terminal 2: ./client/chat_client"
	@echo ""
	@echo "With Kernel Driver (requires kernel headers and sudo):"
	@echo "  1. make full"
	@echo "  2. sudo make install"
	@echo "  3. Run server and client as above"
