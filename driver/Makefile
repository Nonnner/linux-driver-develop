# Makefile for enhanced crypto kernel module and USB keyboard monitor
obj-m += crypto_driver.o
obj-m += usb_kbd_monitor.o

KDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean

install:
	sudo insmod crypto_driver.ko
	sudo chmod 666 /dev/crypto_dev

uninstall:
	sudo rmmod crypto_driver

reload: uninstall install

.PHONY: all clean install uninstall reload
