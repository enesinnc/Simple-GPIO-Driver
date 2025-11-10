# Makefile for Simple GPIO Driver

# Module name
obj-m += gpio_driver.o

# Kernel build directory (adjust as needed)
KERNEL_DIR ?= /lib/modules/$(shell uname -r)/build

# Current directory
PWD := $(shell pwd)

# Default target
all:
	$(MAKE) -C $(KERNEL_DIR) M=$(PWD) modules

# Clean target
clean:
	$(MAKE) -C $(KERNEL_DIR) M=$(PWD) clean

# Install target
install:
	$(MAKE) -C $(KERNEL_DIR) M=$(PWD) modules_install
	depmod -a

# Uninstall target
uninstall:
	rm -f /lib/modules/$(shell uname -r)/extra/gpio_driver.ko
	depmod -a

# Help target
help:
	@echo "Available targets:"
	@echo "  all       - Build the kernel module"
	@echo "  clean     - Clean build files"
	@echo "  install   - Install the module"
	@echo "  uninstall - Remove the module"
	@echo "  load      - Load the module"
	@echo "  unload    - Unload the module"

# Load module
load:
	sudo insmod gpio_driver.ko

# Unload module
unload:
	sudo rmmod gpio_driver

.PHONY: all clean install uninstall help load unload
