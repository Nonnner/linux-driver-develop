# Makefile for chat server and client
CC = gcc
CFLAGS = -Wall -Wextra -pthread -O2
LDFLAGS = -pthread

.PHONY: all clean backend run-web help driver-build driver-load driver-unload driver-reload driver-status driver-clean

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

# Kernel driver management targets
driver-build:
	@echo "Building kernel module..."
	$(MAKE) -f Makefile.driver

driver-load:
	@echo "Loading kernel module..."
	@if [ ! -f crypto_driver.ko ]; then \
		echo "Module not found, building first..."; \
		$(MAKE) -f Makefile.driver; \
	fi
	@echo "Note: This requires root privileges"
	@sudo ./setup.sh load

driver-unload:
	@echo "Unloading kernel module..."
	@sudo ./setup.sh unload

driver-reload:
	@echo "Reloading kernel module..."
	@sudo ./setup.sh reload

driver-status:
	@echo "Checking kernel module status..."
	@./setup.sh status

driver-clean:
	@echo "Cleaning kernel module build artifacts..."
	@$(MAKE) -f Makefile.driver clean

clean:
	rm -f chat_server chat_client
	find . -type d -name __pycache__ -exec rm -rf {} + 2>/dev/null || true
	find . -type f -name "*.pyc" -delete 2>/dev/null || true

help:
	@echo "Available targets:"
	@echo ""
	@echo "Build targets:"
	@echo "  all            - Build chat server and client (default)"
	@echo "  driver-build   - Build kernel module"
	@echo ""
	@echo "Driver management:"
	@echo "  driver-load    - Load kernel module (requires sudo)"
	@echo "  driver-unload  - Unload kernel module (requires sudo)"
	@echo "  driver-reload  - Reload kernel module (requires sudo)"
	@echo "  driver-status  - Check kernel module status"
	@echo "  driver-clean   - Clean kernel module build files"
	@echo ""
	@echo "Web UI:"
	@echo "  backend        - Install Python dependencies for web UI"
	@echo "  run-web        - Run web backend server (requires backend)"
	@echo ""
	@echo "Cleanup:"
	@echo "  clean          - Remove build artifacts"
	@echo ""
	@echo "Quick start:"
	@echo "  1. Build everything:    make && make driver-build"
	@echo "  2. Load kernel module:  make driver-load"
	@echo "  3. Start TCP server:    ./chat_server"
	@echo "  4. Connect client:      ./chat_client"
	@echo ""
	@echo "For Web UI:"
	@echo "  1. Terminal 1: ./chat_server"
	@echo "  2. Terminal 2: make run-web"
	@echo "  3. Browser:    http://localhost:5000"
