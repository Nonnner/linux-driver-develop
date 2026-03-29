# Makefile for chat server and client
CC = gcc
CFLAGS = -std=gnu99 -Wall -Wextra -pthread -O2
LDFLAGS = -pthread
VENV_DIR = .venv
VENV_PYTHON = $(VENV_DIR)/bin/python
VENV_PIP = $(VENV_DIR)/bin/pip

.PHONY: all clean backend run-web help driver-build driver-load driver-unload driver-reload driver-status driver-clean driver-load-kbd driver-unload-kbd driver-reload-kbd

all: chat_server chat_client

chat_server: chat_server.c
	$(CC) $(CFLAGS) -o chat_server chat_server.c $(LDFLAGS)

chat_client: chat_client.c
	$(CC) $(CFLAGS) -o chat_client chat_client.c $(LDFLAGS)

# Setup Python backend dependencies
backend:
	@echo "Setting up Python virtual environment for web backend..."
	@if [ ! -d "$(VENV_DIR)" ]; then \
		echo "Creating $(VENV_DIR)..."; \
		python3 -m venv $(VENV_DIR) || { \
			echo "Failed to create venv. Install python3-venv and retry:"; \
			echo "  sudo apt-get install python3-venv"; \
			exit 1; \
		}; \
	fi
	@$(VENV_PYTHON) -m pip install --upgrade pip >/dev/null
	@$(VENV_PIP) install -r backend/requirements.txt

# Run web backend server
run-web: backend
	@echo "Starting Web Backend Server..."
	@echo "Make sure TCP chat server is running first!"
	@echo "Access web UI at: http://localhost:5000"
	@if command -v lsof >/dev/null 2>&1; then \
		PIDS=`lsof -ti :5000`; \
		if [ -n "$$PIDS" ]; then \
			echo "Port 5000 is busy. Stopping stale process(es): $$PIDS"; \
			kill $$PIDS >/dev/null 2>&1 || true; \
			sleep 1; \
		fi; \
	fi
	cd backend && ../$(VENV_PYTHON) chat_backend.py

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

driver-load-kbd:
	@echo "Loading USB keyboard monitor module..."
	@if [ ! -f usb_kbd_monitor.ko ]; then \
		echo "Module not found, building first..."; \
		$(MAKE) -f Makefile.driver; \
	fi
	@echo "Loading usb_kbd_monitor.ko..."
	@sudo insmod usb_kbd_monitor.ko
	@echo "✓ Module loaded. Check 'dmesg -w' to see keyboard events."

driver-unload-kbd:
	@echo "Unloading USB keyboard monitor module..."
	@if lsmod | grep -q usb_kbd_monitor; then \
		sudo rmmod usb_kbd_monitor; \
		echo "✓ Module unloaded"; \
	else \
		echo "Module not currently loaded"; \
	fi

driver-reload-kbd:
	@echo "Reloading USB keyboard monitor module..."
	@$(MAKE) driver-unload-kbd
	@$(MAKE) driver-load-kbd
	rm -f chat_server chat_client
	find . -type d -name __pycache__ -exec rm -rf {} + 2>/dev/null || true
	find . -type f -name "*.pyc" -delete 2>/dev/null || true

help:
	@echo "Available targets:"
	@echo ""
	@echo "Build targets:"
	@echo "  all            - Build chat server and client (default)"
	@echo "  driver-build   - Build kernel modules (crypto + keyboard)"
	@echo ""
	@echo "Driver (Crypto) management:"
	@echo "  driver-load    - Load crypto module (requires sudo)"
	@echo "  driver-unload  - Unload crypto module (requires sudo)"
	@echo "  driver-reload  - Reload crypto module (requires sudo)"
	@echo "  driver-status  - Check crypto module status"
	@echo ""
	@echo "USB Keyboard Monitor management:"
	@echo "  driver-load-kbd     - Load USB keyboard monitor module (requires sudo)"
	@echo "  driver-unload-kbd   - Unload USB keyboard monitor module (requires sudo)"
	@echo "  driver-reload-kbd   - Reload USB keyboard monitor module (requires sudo)"
	@echo ""
	@echo "  driver-clean   - Clean kernel module build files"
	@echo ""
	@echo "Web UI:"
	@echo "  backend        - Install Python dependencies for web UI"
	@echo "  run-web        - Run web backend server (requires backend)"
	@echo ""
	@echo "Cleanup:"
	@echo "  clean          - Remove build artifacts"
	@echo ""
	@echo "Quick start (Chat):"
	@echo "  1. Build everything:    make && make driver-build"
	@echo "  2. Load crypto module:  make driver-load"
	@echo "  3. Start TCP server:    ./chat_server"
	@echo "  4. Connect client:      ./chat_client"
	@echo ""
	@echo "Quick start (USB Keyboard Test on CentOS 7):"
	@echo "  1. make driver-build"
	@echo "  2. make driver-load-kbd"
	@echo "  3. dmesg -w (in another terminal)"
	@echo "  4. Plug USB keyboard and press keys"
	@echo "  5. make driver-unload-kbd (when done)"
	@echo ""
	@echo "For Web UI:"
	@echo "  1. Terminal 1: ./chat_server"
	@echo "  2. Terminal 2: make run-web"
	@echo "  3. Browser:    http://localhost:5000"
