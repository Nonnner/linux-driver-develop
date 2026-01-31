# Makefile for Crypto Device Driver
#
# This Makefile builds the crypto_driver kernel module

# Module name
obj-m := crypto_driver.o

# Kernel build directory
KDIR ?= /lib/modules/$(shell uname -r)/build

# Current directory
PWD := $(shell pwd)

# Default target - build the module
all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

# Clean target - remove build artifacts
clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean

# Install target - load the module
install: all
	sudo insmod crypto_driver.ko
	@echo "Module loaded. Creating device node..."
	@sleep 1
	@if [ ! -e /dev/crypto_dev ]; then \
		sudo mknod /dev/crypto_dev c $$(cat /sys/class/crypto/crypto_dev/dev | cut -d: -f1) 0; \
		sudo chmod 666 /dev/crypto_dev; \
	fi
	@echo "Device node created at /dev/crypto_dev"

# Uninstall target - unload the module
uninstall:
	sudo rmmod crypto_driver
	sudo rm -f /dev/crypto_dev
	@echo "Module unloaded and device node removed"

# Show module info
info:
	modinfo crypto_driver.ko

.PHONY: all clean install uninstall info
