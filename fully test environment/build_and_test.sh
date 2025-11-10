#!/bin/bash
#
# GPIO Driver Build and Test Script
#

set -e

DRIVER_NAME="gpio_driver"
TEST_APP="gpio_test"
DEVICE="/dev/simple_gpio"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

print_status() {
    echo -e "${GREEN}[*]${NC} $1"
}

print_error() {
    echo -e "${RED}[!]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[!]${NC} $1"
}

# Check if running as root for module operations
check_root() {
    if [ "$EUID" -ne 0 ]; then 
        print_error "This script must be run as root for module loading/unloading"
        exit 1
    fi
}

# Build the kernel module
build_driver() {
    print_status "Building kernel module..."
    make clean
    make
    if [ $? -eq 0 ]; then
        print_status "Kernel module built successfully"
    else
        print_error "Failed to build kernel module"
        exit 1
    fi
}

# Build the test application
build_test() {
    print_status "Building test application..."
    make -f Makefile.test clean
    make -f Makefile.test
    if [ $? -eq 0 ]; then
        print_status "Test application built successfully"
    else
        print_error "Failed to build test application"
        exit 1
    fi
}

# Load the kernel module
load_driver() {
    print_status "Loading kernel module..."
    
    # Remove if already loaded
    if lsmod | grep -q "$DRIVER_NAME"; then
        print_warning "Module already loaded, removing first..."
        rmmod $DRIVER_NAME
    fi
    
    # Check if IRQ parameter was provided
    if [ ! -z "$IRQ_NUM" ]; then
        print_status "Loading with IRQ number: $IRQ_NUM"
        insmod ${DRIVER_NAME}.ko gpio_irq=$IRQ_NUM
    else
        print_warning "No IRQ specified, loading without interrupt support"
        print_warning "Use: IRQ_NUM=<num> $0 load to enable interrupts"
        insmod ${DRIVER_NAME}.ko
    fi
    
    if [ $? -eq 0 ]; then
        print_status "Module loaded successfully"
        lsmod | grep $DRIVER_NAME
        
        # Check IRQ registration status
        sleep 1
        if dmesg | tail -10 | grep -q "IRQ.*registered"; then
            print_status "Interrupt handler registered successfully"
        elif dmesg | tail -10 | grep -q "without interrupt"; then
            print_warning "Running without interrupt support"
        fi
    else
        print_error "Failed to load module"
        dmesg | tail -10
        exit 1
    fi
    
    # Wait for device node creation
    sleep 1
    
    # Check device node
    if [ -e "$DEVICE" ]; then
        print_status "Device node created: $DEVICE"
        ls -l $DEVICE
        
        # Set permissions
        chmod 666 $DEVICE
        print_status "Permissions set to 666"
    else
        print_error "Device node not created"
        exit 1
    fi
}

# Unload the kernel module
unload_driver() {
    print_status "Unloading kernel module..."
    
    if lsmod | grep -q "$DRIVER_NAME"; then
        rmmod $DRIVER_NAME
        print_status "Module unloaded successfully"
    else
        print_warning "Module not loaded"
    fi
}

# Run the test application
run_test() {
    print_status "Running test application..."
    
    if [ ! -e "$DEVICE" ]; then
        print_error "Device node does not exist. Is the driver loaded?"
        exit 1
    fi
    
    if [ ! -x "./$TEST_APP" ]; then
        print_error "Test application not found or not executable"
        exit 1
    fi
    
    ./$TEST_APP
}

# Show kernel messages
show_logs() {
    print_status "Showing kernel messages (last 30 lines)..."
    dmesg | grep -i gpio | tail -30
}

# Show help
show_help() {
    echo "GPIO Driver Build and Test Script"
    echo ""
    echo "Usage: $0 [command]"
    echo "       IRQ_NUM=<num> $0 [command]  # To specify IRQ number"
    echo ""
    echo "Commands:"
    echo "  build       - Build both driver and test application"
    echo "  load        - Load the kernel module (requires root)"
    echo "  unload      - Unload the kernel module (requires root)"
    echo "  test        - Run the test application"
    echo "  logs        - Show recent kernel messages"
    echo "  all         - Build, load, and test (requires root)"
    echo "  clean       - Remove built files and unload module"
    echo "  help        - Show this help message"
    echo ""
    echo "Examples:"
    echo "  sudo $0 all                    # Build and run without interrupts"
    echo "  IRQ_NUM=42 sudo $0 all         # Build and run with IRQ 42"
    echo "  IRQ_NUM=42 sudo $0 load        # Load driver with IRQ 42"
    echo "  $0 test                        # Run test (driver must be loaded)"
    echo "  sudo $0 clean                  # Clean up everything"
    echo ""
    echo "IRQ Configuration:"
    echo "  Set IRQ_NUM environment variable to enable interrupts"
    echo "  Example: IRQ_NUM=42 sudo $0 load"
    echo "  To find your IRQ: cat /proc/interrupts"
}

# Main script
case "$1" in
    build)
        build_driver
        build_test
        ;;
    load)
        check_root
        load_driver
        ;;
    unload)
        check_root
        unload_driver
        ;;
    test)
        run_test
        ;;
    logs)
        show_logs
        ;;
    all)
        check_root
        build_driver
        build_test
        load_driver
        sleep 1
        print_status "Running comprehensive test..."
        ./$TEST_APP
        show_logs
        ;;
    clean)
        if [ "$EUID" -eq 0 ]; then
            unload_driver
        else
            print_warning "Not running as root, skipping module unload"
        fi
        print_status "Cleaning build files..."
        make clean
        make -f Makefile.test clean
        print_status "Clean complete"
        ;;
    help|--help|-h|"")
        show_help
        ;;
    *)
        print_error "Unknown command: $1"
        show_help
        exit 1
        ;;
esac

print_status "Done!"
