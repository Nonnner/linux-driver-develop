#!/bin/bash
# Check if kernel module version matches code

echo "=== Checking Crypto Driver Version ==="
echo ""

# Check if module is loaded
if lsmod | grep -q crypto_driver; then
    echo "✓ Module is loaded"
    
    # Check module version
    MODULE_VERSION=$(modinfo crypto_driver.ko 2>/dev/null | grep "^version:" | awk '{print $2}')
    if [ -n "$MODULE_VERSION" ]; then
        echo "  Module version: $MODULE_VERSION"
    fi
    
    # Check module info from loaded module
    LOADED_INFO=$(modinfo crypto_driver 2>/dev/null | grep -E "^(version|description):")
    if [ -n "$LOADED_INFO" ]; then
        echo "  Loaded module info:"
        echo "$LOADED_INFO" | sed 's/^/    /'
    fi
else
    echo "✗ Module is NOT loaded"
    echo ""
    echo "To load the module:"
    echo "  sudo make driver-load"
    echo "  or"
    echo "  sudo ./setup.sh load"
    exit 1
fi

echo ""

# Check if device exists
if [ -e /dev/crypto_dev ]; then
    echo "✓ Device /dev/crypto_dev exists"
    ls -l /dev/crypto_dev
else
    echo "✗ Device /dev/crypto_dev does NOT exist"
    echo "  Module may have failed to create device"
    exit 1
fi

echo ""

# Check if source files are newer than module
if [ -f driver/crypto_driver.c ] && [ -f crypto_driver.ko ]; then
    SOURCE_TIME=$(stat -c %Y driver/crypto_driver.c 2>/dev/null || stat -f %m driver/crypto_driver.c 2>/dev/null)
    MODULE_TIME=$(stat -c %Y crypto_driver.ko 2>/dev/null || stat -f %m crypto_driver.ko 2>/dev/null)
    
    if [ "$SOURCE_TIME" -gt "$MODULE_TIME" ]; then
        echo "⚠ WARNING: Source file is NEWER than module!"
        echo "  You need to rebuild the module:"
        echo "    make driver-build"
        echo "    sudo make driver-reload"
    else
        echo "✓ Module is up to date with source"
    fi
fi

echo ""

# Test IOCTL command number
echo "=== Testing IOCTL Command Numbers ==="
cat > /tmp/test_ioctl.c << 'EOF'
#include <stdio.h>
#include <sys/ioctl.h>

#define CRYPTO_IOC_MAGIC 'c'
#define MAX_DATA_SIZE 4096
#define AES_KEY_SIZE 16
#define AES_IV_SIZE 16

struct crypto_data {
    unsigned char input[MAX_DATA_SIZE];
    unsigned char output[MAX_DATA_SIZE];
    unsigned char key[AES_KEY_SIZE];
    unsigned char iv[AES_IV_SIZE];
    unsigned int input_len;
    unsigned int output_len;
};

#define IOCTL_MD5_HASH _IOWR(CRYPTO_IOC_MAGIC, 6, struct crypto_data)

int main() {
    printf("struct crypto_data size: %lu bytes\n", sizeof(struct crypto_data));
    printf("IOCTL_MD5_HASH command: 0x%lx\n", (unsigned long)IOCTL_MD5_HASH);
    return 0;
}
EOF

gcc -o /tmp/test_ioctl /tmp/test_ioctl.c 2>/dev/null
if [ $? -eq 0 ]; then
    /tmp/test_ioctl
    rm -f /tmp/test_ioctl /tmp/test_ioctl.c
fi

echo ""
echo "=== Summary ==="
echo "If you're experiencing 'Invalid argument' errors:"
echo "1. Rebuild the module: make driver-build"
echo "2. Reload the module: sudo make driver-reload"
echo "3. Rebuild the server: make"
echo "4. Test again: ./chat_server"
