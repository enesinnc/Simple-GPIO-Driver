/*
 * GPIO Driver Test Application
 * Demonstrates how to use the GPIO driver from user space
 */

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <string.h>
#include <errno.h>
#include "gpio_driver.h"

#define DEVICE_PATH "/dev/simple_gpio"

/* Function prototypes */
void print_usage(const char *prog_name);
int set_gpio_direction(int fd, int gpio_num, int direction);
int read_gpio_pin(int fd, int gpio_num);
int write_gpio_pin(int fd, int gpio_num, int value);
int set_gpio_interrupt(int fd, int gpio_num, int enable);
int read_gpio_interrupt_status(int fd, int gpio_num);
int clear_gpio_interrupt_status(int fd, int gpio_num);
void demo_all_functions(int fd);

int main(int argc, char *argv[])
{
    int fd;
    int gpio_num, value;

    printf("=== GPIO Driver Test Application ===\n\n");

    /* Open the device */
    fd = open(DEVICE_PATH, O_RDWR);
    if (fd < 0) {
        perror("Failed to open device");
        printf("Make sure the driver is loaded and device exists at %s\n", 
               DEVICE_PATH);
        return EXIT_FAILURE;
    }

    printf("Device opened successfully\n\n");

    /* Parse command line arguments */
    if (argc == 1) {
        /* No arguments, run demo */
        demo_all_functions(fd);
    } else if (argc >= 3) {
        /* Command line operation */
        if (strcmp(argv[1], "set_dir") == 0 && argc == 4) {
            gpio_num = atoi(argv[2]);
            value = atoi(argv[3]);
            set_gpio_direction(fd, gpio_num, value);
        } else if (strcmp(argv[1], "read") == 0 && argc == 3) {
            gpio_num = atoi(argv[2]);
            read_gpio_pin(fd, gpio_num);
        } else if (strcmp(argv[1], "write") == 0 && argc == 4) {
            gpio_num = atoi(argv[2]);
            value = atoi(argv[3]);
            write_gpio_pin(fd, gpio_num, value);
        } else if (strcmp(argv[1], "set_int") == 0 && argc == 4) {
            gpio_num = atoi(argv[2]);
            value = atoi(argv[3]);
            set_gpio_interrupt(fd, gpio_num, value);
        } else if (strcmp(argv[1], "read_int") == 0 && argc == 3) {
            gpio_num = atoi(argv[2]);
            read_gpio_interrupt_status(fd, gpio_num);
        } else if (strcmp(argv[1], "clear_int") == 0 && argc == 3) {
            gpio_num = atoi(argv[2]);
            clear_gpio_interrupt_status(fd, gpio_num);
        } else {
            print_usage(argv[0]);
        }
    } else {
        print_usage(argv[0]);
    }

    close(fd);
    return EXIT_SUCCESS;
}

void print_usage(const char *prog_name)
{
    printf("Usage:\n");
    printf("  %s                              - Run demo of all functions\n", 
           prog_name);
    printf("  %s set_dir <gpio> <dir>         - Set direction (0=in, 1=out)\n", 
           prog_name);
    printf("  %s read <gpio>                  - Read pin value\n", 
           prog_name);
    printf("  %s write <gpio> <value>         - Write pin value (0 or 1)\n", 
           prog_name);
    printf("  %s set_int <gpio> <enable>      - Set interrupt (0=off, 1=on)\n", 
           prog_name);
    printf("  %s read_int <gpio>              - Read interrupt status\n", 
           prog_name);
    printf("  %s clear_int <gpio>             - Clear interrupt status\n", 
           prog_name);
    printf("\nGPIO numbers: 0-7 (corresponding to GPIO pins 1-8)\n");
}

int set_gpio_direction(int fd, int gpio_num, int direction)
{
    struct gpio_config config;
    int ret;

    config.gpio_num = gpio_num;
    config.value = direction;

    ret = ioctl(fd, GPIO_SET_DIRECTION, &config);
    if (ret < 0) {
        perror("GPIO_SET_DIRECTION failed");
        return -1;
    }

    printf("GPIO %d: Direction set to %s\n", 
           gpio_num + 1, direction ? "OUTPUT" : "INPUT");
    return 0;
}

int read_gpio_pin(int fd, int gpio_num)
{
    struct gpio_config config;
    int ret;

    config.gpio_num = gpio_num;

    ret = ioctl(fd, GPIO_READ_PIN, &config);
    if (ret < 0) {
        perror("GPIO_READ_PIN failed");
        return -1;
    }

    printf("GPIO %d: Read value = %d\n", gpio_num + 1, config.value);
    return config.value;
}

int write_gpio_pin(int fd, int gpio_num, int value)
{
    struct gpio_config config;
    int ret;

    config.gpio_num = gpio_num;
    config.value = value;

    ret = ioctl(fd, GPIO_WRITE_PIN, &config);
    if (ret < 0) {
        if (errno == EPERM) {
            printf("GPIO %d: Cannot write - pin is configured as INPUT\n", 
                   gpio_num + 1);
        } else {
            perror("GPIO_WRITE_PIN failed");
        }
        return -1;
    }

    printf("GPIO %d: Value set to %d\n", gpio_num + 1, value);
    return 0;
}

int set_gpio_interrupt(int fd, int gpio_num, int enable)
{
    struct gpio_config config;
    int ret;

    config.gpio_num = gpio_num;
    config.value = enable;

    ret = ioctl(fd, GPIO_SET_INTERRUPT, &config);
    if (ret < 0) {
        perror("GPIO_SET_INTERRUPT failed");
        return -1;
    }

    printf("GPIO %d: Interrupt %s\n", 
           gpio_num + 1, enable ? "ENABLED" : "DISABLED");
    return 0;
}

int read_gpio_interrupt_status(int fd, int gpio_num)
{
    struct gpio_config config;
    int ret;

    config.gpio_num = gpio_num;

    ret = ioctl(fd, GPIO_READ_INT_STATUS, &config);
    if (ret < 0) {
        perror("GPIO_READ_INT_STATUS failed");
        return -1;
    }

    printf("GPIO %d: Interrupt status = %s\n", 
           gpio_num + 1, config.value ? "PENDING" : "CLEAR");
    return config.value;
}

int clear_gpio_interrupt_status(int fd, int gpio_num)
{
    struct gpio_config config;
    int ret;

    config.gpio_num = gpio_num;

    ret = ioctl(fd, GPIO_CLEAR_INT_STATUS, &config);
    if (ret < 0) {
        perror("GPIO_CLEAR_INT_STATUS failed");
        return -1;
    }

    printf("GPIO %d: Interrupt status cleared\n", gpio_num + 1);
    return 0;
}

void demo_all_functions(int fd)
{
    printf("=== Running GPIO Driver Demo ===\n\n");

    /* Test GPIO 1 (index 0) as OUTPUT */
    printf("--- Testing GPIO 1 as OUTPUT ---\n");
    set_gpio_direction(fd, 0, GPIO_DIR_OUTPUT);
    write_gpio_pin(fd, 0, 1);
    sleep(1);
    write_gpio_pin(fd, 0, 0);
    printf("\n");

    /* Test GPIO 2 (index 1) as INPUT */
    printf("--- Testing GPIO 2 as INPUT ---\n");
    set_gpio_direction(fd, 1, GPIO_DIR_INPUT);
    read_gpio_pin(fd, 1);
    printf("\n");

    /* Test interrupt on GPIO 3 (index 2) */
    printf("--- Testing Interrupt on GPIO 3 ---\n");
    set_gpio_direction(fd, 2, GPIO_DIR_INPUT);
    set_gpio_interrupt(fd, 2, GPIO_INT_ENABLE);
    read_gpio_interrupt_status(fd, 2);
    printf("Note: To test actual interrupts, external hardware signal changes are needed\n");
    printf("\n");

    /* Test GPIO 4 (index 3) - demonstrate input/output switching */
    printf("--- Testing GPIO 4 Direction Switching ---\n");
    set_gpio_direction(fd, 3, GPIO_DIR_OUTPUT);
    write_gpio_pin(fd, 3, 1);
    set_gpio_direction(fd, 3, GPIO_DIR_INPUT);
    read_gpio_pin(fd, 3);
    printf("\n");

    /* Test error handling - try to write to input */
    printf("--- Testing Error Handling ---\n");
    set_gpio_direction(fd, 4, GPIO_DIR_INPUT);
    printf("Attempting to write to input pin (should fail):\n");
    write_gpio_pin(fd, 4, 1);
    printf("\n");

    /* Test multiple GPIOs */
    printf("--- Testing Multiple GPIOs ---\n");
    for (int i = 0; i < 4; i++) {
        set_gpio_direction(fd, i, GPIO_DIR_OUTPUT);
        write_gpio_pin(fd, i, i % 2);
    }
    printf("\n");

    /* Read all input pins */
    printf("--- Reading All Input Pins ---\n");
    for (int i = 0; i < 8; i++) {
        set_gpio_direction(fd, i, GPIO_DIR_INPUT);
        read_gpio_pin(fd, i);
    }
    printf("\n");

    /* Test interrupt status on all pins */
    printf("--- Checking Interrupt Status on All Pins ---\n");
    for (int i = 0; i < 8; i++) {
        read_gpio_interrupt_status(fd, i);
    }
    printf("\n");

    printf("=== Demo Complete ===\n");
}
