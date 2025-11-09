#ifndef GPIO_DRIVER_H
#define GPIO_DRIVER_H

#include <linux/ioctl.h>

struct gpio_config {
    int gpio_num;  
    int value;     
};

#define GPIO_IOC_MAGIC 'g'

#define GPIO_SET_DIRECTION    _IOW(GPIO_IOC_MAGIC, 1, struct gpio_config)
#define GPIO_READ_PIN         _IOWR(GPIO_IOC_MAGIC, 2, struct gpio_config)
#define GPIO_WRITE_PIN        _IOW(GPIO_IOC_MAGIC, 3, struct gpio_config)
#define GPIO_SET_INTERRUPT    _IOW(GPIO_IOC_MAGIC, 4, struct gpio_config)
#define GPIO_READ_INT_STATUS  _IOWR(GPIO_IOC_MAGIC, 5, struct gpio_config)
#define GPIO_CLEAR_INT_STATUS _IOW(GPIO_IOC_MAGIC, 6, struct gpio_config)

#define GPIO_DIR_INPUT  0
#define GPIO_DIR_OUTPUT 1

#define GPIO_INT_DISABLE 0
#define GPIO_INT_ENABLE  1

#endif 
