# Makefile for chat server and client
CC = gcc
CFLAGS = -Wall -Wextra -pthread -O2
LDFLAGS = -pthread

.PHONY: all clean backend run-web help

all: chat_server chat_client

chat_server: chat_server.c
	$(CC) $(CFLAGS) -o chat_server chat_server.c $(LDFLAGS)

chat_client: chat_client.c
	$(CC) $(CFLAGS) -o chat_client chat_client.c $(LDFLAGS)

# Setup Python backend dependencies
backend:
	@echo "Installing Python backend dependencies..."
	@if command -v pip3 >/dev/null 2>&1; then \
		pip3 install -r backend/requirements.txt; \
	else \
		pip install -r backend/requirements.txt; \
	fi

# Run web backend server
run-web: backend
	@echo "Starting Web Backend Server..."
	@echo "Make sure TCP chat server is running first!"
	@echo "Access web UI at: http://localhost:5000"
	cd backend && python3 chat_backend.py

clean:
	rm -f chat_server chat_client
	find . -type d -name __pycache__ -exec rm -rf {} + 2>/dev/null || true
	find . -type f -name "*.pyc" -delete 2>/dev/null || true

help:
	@echo "Available targets:"
	@echo "  all        - Build chat server and client (default)"
	@echo "  backend    - Install Python dependencies for web UI"
	@echo "  run-web    - Run web backend server (requires backend)"
	@echo "  clean      - Remove build artifacts"
	@echo "  help       - Show this help message"
	@echo ""
	@echo "Quick start for Web UI:"
	@echo "  1. Terminal 1: ./chat_server"
	@echo "  2. Terminal 2: make run-web"
	@echo "  3. Browser:    http://localhost:5000"
