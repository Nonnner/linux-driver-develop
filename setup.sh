#!/bin/bash
#
# Setup script for Linux Chat System - Kernel Module Management
# This script helps manage the crypto_driver kernel module
#

set -e

DRIVER_NAME="crypto_driver"
DRIVER_KO="${DRIVER_NAME}.ko"
DEVICE="/dev/crypto_dev"
MAKEFILE_DRIVER="Makefile.driver"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Print colored message
print_msg() {
    local color=$1
    shift
    echo -e "${color}$@${NC}"
}

# Print error and exit
error_exit() {
    print_msg "$RED" "ERROR: $1"
    exit 1
}

# Check if running as root
check_root() {
    if [ "$EUID" -ne 0 ]; then
        error_exit "This operation requires root privileges. Please run with sudo."
    fi
}

# Check if module is loaded
is_module_loaded() {
    lsmod | grep -q "^${DRIVER_NAME} "
}

# Check if device exists
is_device_exists() {
    [ -e "$DEVICE" ]
}

# Build the kernel module
build_module() {
    print_msg "$BLUE" "Building kernel module..."
    
    if [ ! -f "$MAKEFILE_DRIVER" ]; then
        error_exit "Makefile.driver not found. Are you in the correct directory?"
    fi
    
    make -f "$MAKEFILE_DRIVER"
    
    if [ ! -f "$DRIVER_KO" ]; then
        error_exit "Failed to build kernel module. Check the build output above."
    fi
    
    print_msg "$GREEN" "✓ Kernel module built successfully"
}

# Load the kernel module
load_module() {
    check_root
    
    print_msg "$BLUE" "Loading kernel module..."
    
    # Check if already loaded
    if is_module_loaded; then
        print_msg "$YELLOW" "⚠ Module is already loaded"
        return 0
    fi
    
    # Build if module doesn't exist
    if [ ! -f "$DRIVER_KO" ]; then
        print_msg "$YELLOW" "Module not found. Building..."
        # Drop root privileges for build
        su -c "make -f $MAKEFILE_DRIVER" $SUDO_USER || error_exit "Failed to build module"
    fi
    
    # Load the module
    insmod "$DRIVER_KO" || error_exit "Failed to load kernel module"
    
    # Wait a moment for device creation
    sleep 1
    
    # Check if device was created
    if ! is_device_exists; then
        error_exit "Device $DEVICE was not created after loading module"
    fi
    
    # Set device permissions
    chmod 666 "$DEVICE" || print_msg "$YELLOW" "⚠ Warning: Failed to set device permissions"
    
    print_msg "$GREEN" "✓ Kernel module loaded successfully"
    print_msg "$GREEN" "✓ Device $DEVICE created"
    
    # Show kernel messages
    print_msg "$BLUE" "\nKernel messages:"
    dmesg | tail -5 | grep crypto_dev || true
}

# Unload the kernel module
unload_module() {
    check_root
    
    print_msg "$BLUE" "Unloading kernel module..."
    
    # Check if loaded
    if ! is_module_loaded; then
        print_msg "$YELLOW" "⚠ Module is not loaded"
        return 0
    fi
    
    # Unload the module
    rmmod "$DRIVER_NAME" || error_exit "Failed to unload kernel module. Make sure no processes are using it."
    
    print_msg "$GREEN" "✓ Kernel module unloaded successfully"
    
    # Show kernel messages
    print_msg "$BLUE" "\nKernel messages:"
    dmesg | tail -3 | grep crypto_dev || true
}

# Reload the kernel module
reload_module() {
    print_msg "$BLUE" "Reloading kernel module..."
    
    if is_module_loaded; then
        unload_module
    fi
    
    load_module
}

# Show status
show_status() {
    print_msg "$BLUE" "=== Crypto Driver Status ==="
    echo ""
    
    # Check if module file exists
    if [ -f "$DRIVER_KO" ]; then
        print_msg "$GREEN" "✓ Module file: $DRIVER_KO exists"
    else
        print_msg "$RED" "✗ Module file: $DRIVER_KO not found (need to build)"
    fi
    
    # Check if module is loaded
    if is_module_loaded; then
        print_msg "$GREEN" "✓ Module: Loaded"
        lsmod | grep "^${DRIVER_NAME} " || true
    else
        print_msg "$RED" "✗ Module: Not loaded"
    fi
    
    # Check if device exists
    if is_device_exists; then
        print_msg "$GREEN" "✓ Device: $DEVICE exists"
        ls -l "$DEVICE"
    else
        print_msg "$RED" "✗ Device: $DEVICE not found"
    fi
    
    echo ""
    print_msg "$BLUE" "Recent kernel messages:"
    dmesg | grep crypto_dev | tail -5 || print_msg "$YELLOW" "No recent kernel messages"
}

# Clean build artifacts
clean_module() {
    print_msg "$BLUE" "Cleaning build artifacts..."
    
    # Unload if loaded
    if is_module_loaded; then
        print_msg "$YELLOW" "Module is loaded. Unloading first..."
        check_root
        unload_module
    fi
    
    make -f "$MAKEFILE_DRIVER" clean
    print_msg "$GREEN" "✓ Build artifacts cleaned"
}

# Show usage
show_usage() {
    cat << EOF
Usage: $0 [COMMAND]

Kernel Module Management Script for Linux Chat System

Commands:
  load      Build (if needed) and load the kernel module
  unload    Unload the kernel module
  reload    Reload the kernel module (unload + load)
  status    Show current status of the module
  build     Build the kernel module only
  clean     Clean build artifacts
  help      Show this help message

Examples:
  sudo $0 load      # Load the module
  sudo $0 status    # Check status (no root needed)
  sudo $0 reload    # Reload the module

Note: Most commands require root privileges (use sudo)

After loading the module, you can start the chat server:
  ./chat_server
EOF
}

# Main script logic
main() {
    local command="${1:-help}"
    
    case "$command" in
        load)
            load_module
            echo ""
            print_msg "$GREEN" "You can now start the chat server:"
            print_msg "$BLUE" "  ./chat_server"
            ;;
        unload)
            unload_module
            ;;
        reload)
            reload_module
            ;;
        status)
            show_status
            ;;
        build)
            build_module
            ;;
        clean)
            clean_module
            ;;
        help|--help|-h)
            show_usage
            ;;
        *)
            print_msg "$RED" "Unknown command: $command"
            echo ""
            show_usage
            exit 1
            ;;
    esac
}

# Run main function
main "$@"
