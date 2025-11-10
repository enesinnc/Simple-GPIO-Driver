#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/io.h>
#include <linux/interrupt.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include "gpio_driver.h"

#define DRIVER_NAME "simple_gpio"
#define GPIO_BASE_ADDR 0x28000000
#define GPIO_MEM_SIZE 0x24
#define NUM_GPIOS 8

/* Module parameters */
static int gpio_irq = -1;
module_param(gpio_irq, int, 0444);
MODULE_PARM_DESC(gpio_irq, "GPIO interrupt number (IRQ line)");

#define GPIO_DATA_BIT       (1 << 0)
#define GPIO_DIR_BIT        (1 << 1)
#define GPIO_INT_STATUS_BIT (1 << 8)
#define GPIO_INT_ENABLE_BIT (1 << 9)

static const u32 gpio_offsets[NUM_GPIOS] = {
    0x00, 0x04, 0x08, 0x0c, 0x10, 0x14, 0x1c, 0x20
};

struct gpio_device {
    struct cdev cdev;
    struct class *class;
    dev_t devt;
    void __iomem *base_addr;
    int irq;
    spinlock_t lock;
};

static struct gpio_device *gpio_dev;

static inline u32 gpio_read_reg(int gpio_num)
{
    if (gpio_num < 0 || gpio_num >= NUM_GPIOS)
        return 0;
    return ioread32(gpio_dev->base_addr + gpio_offsets[gpio_num]);
}

static inline void gpio_write_reg(int gpio_num, u32 value)
{
    if (gpio_num < 0 || gpio_num >= NUM_GPIOS)
        return;
    iowrite32(value, gpio_dev->base_addr + gpio_offsets[gpio_num]);
}

static int gpio_set_direction(int gpio_num, int direction)
{
    u32 reg_val;
    unsigned long flags;

    if (gpio_num < 0 || gpio_num >= NUM_GPIOS)
        return -EINVAL;

    spin_lock_irqsave(&gpio_dev->lock, flags);
    reg_val = gpio_read_reg(gpio_num);
    
    if (direction)
        reg_val |= GPIO_DIR_BIT;  
    else
        reg_val &= ~GPIO_DIR_BIT; 
    
    gpio_write_reg(gpio_num, reg_val);
    spin_unlock_irqrestore(&gpio_dev->lock, flags);

    return 0;
}

static int gpio_read_pin(int gpio_num, int *value)
{
    u32 reg_val;
    unsigned long flags;

    if (gpio_num < 0 || gpio_num >= NUM_GPIOS)
        return -EINVAL;

    spin_lock_irqsave(&gpio_dev->lock, flags);
    reg_val = gpio_read_reg(gpio_num);
    *value = (reg_val & GPIO_DATA_BIT) ? 1 : 0;
    spin_unlock_irqrestore(&gpio_dev->lock, flags);

    return 0;
}

static int gpio_write_pin(int gpio_num, int value)
{
    u32 reg_val;
    unsigned long flags;

    if (gpio_num < 0 || gpio_num >= NUM_GPIOS)
        return -EINVAL;

    spin_lock_irqsave(&gpio_dev->lock, flags);
    reg_val = gpio_read_reg(gpio_num);
    
    if (!(reg_val & GPIO_DIR_BIT)) {
        spin_unlock_irqrestore(&gpio_dev->lock, flags);
        return -EPERM; /* bu if blogu pinin input oldugu casedir */
    }
    
    if (value)
        reg_val |= GPIO_DATA_BIT;
    else
        reg_val &= ~GPIO_DATA_BIT;
    
    gpio_write_reg(gpio_num, reg_val);
    spin_unlock_irqrestore(&gpio_dev->lock, flags);

    return 0;
}

static int gpio_set_interrupt(int gpio_num, int enable)
{
    u32 reg_val;
    unsigned long flags;

    if (gpio_num < 0 || gpio_num >= NUM_GPIOS)
        return -EINVAL;

    spin_lock_irqsave(&gpio_dev->lock, flags);
    reg_val = gpio_read_reg(gpio_num);
    
    if (enable)
        reg_val |= GPIO_INT_ENABLE_BIT;
    else
        reg_val &= ~GPIO_INT_ENABLE_BIT;
    
    gpio_write_reg(gpio_num, reg_val);
    spin_unlock_irqrestore(&gpio_dev->lock, flags);

    return 0;
}

static int gpio_read_int_status(int gpio_num, int *status)
{
    u32 reg_val;
    unsigned long flags;

    if (gpio_num < 0 || gpio_num >= NUM_GPIOS)
        return -EINVAL;

    spin_lock_irqsave(&gpio_dev->lock, flags);
    reg_val = gpio_read_reg(gpio_num);
    *status = (reg_val & GPIO_INT_STATUS_BIT) ? 1 : 0;
    spin_unlock_irqrestore(&gpio_dev->lock, flags);

    return 0;
}

static int gpio_clear_int_status(int gpio_num)
{
    u32 reg_val;
    unsigned long flags;

    if (gpio_num < 0 || gpio_num >= NUM_GPIOS)
        return -EINVAL;

    spin_lock_irqsave(&gpio_dev->lock, flags);
    reg_val = gpio_read_reg(gpio_num);
    
    /* W1TC */
    if (reg_val & GPIO_INT_STATUS_BIT) {
        gpio_write_reg(gpio_num, GPIO_INT_STATUS_BIT);
    }
    
    spin_unlock_irqrestore(&gpio_dev->lock, flags);

    return 0;
}

static irqreturn_t gpio_irq_handler(int irq, void *dev_id)
{
    int i;
    u32 reg_val;
    int handled = 0;

    for (i = 0; i < NUM_GPIOS; i++) {
        reg_val = gpio_read_reg(i);
        
        if (reg_val & GPIO_INT_STATUS_BIT) {
            pr_info("GPIO%d: Interrupt detected (value=%d)\n", 
                    i + 1, (reg_val & GPIO_DATA_BIT) ? 1 : 0);
            
            gpio_clear_int_status(i);
            handled = 1;
        }
    }

    return handled ? IRQ_HANDLED : IRQ_NONE;
}

static int gpio_open(struct inode *inode, struct file *filp)
{
    pr_debug("GPIO device opened\n");
    return 0;
}

static int gpio_release(struct inode *inode, struct file *filp)
{
    pr_debug("GPIO device closed\n");
    return 0;
}

static long gpio_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    struct gpio_config config;
    int ret = 0;

    switch (cmd) {
    case GPIO_SET_DIRECTION:
        if (copy_from_user(&config, (struct gpio_config __user *)arg, 
                          sizeof(config)))
            return -EFAULT;
        ret = gpio_set_direction(config.gpio_num, config.value);
        break;

    case GPIO_READ_PIN:
        if (copy_from_user(&config, (struct gpio_config __user *)arg, 
                          sizeof(config)))
            return -EFAULT;
        ret = gpio_read_pin(config.gpio_num, &config.value);
        if (ret == 0) {
            if (copy_to_user((struct gpio_config __user *)arg, &config, 
                            sizeof(config)))
                return -EFAULT;
        }
        break;

    case GPIO_WRITE_PIN:
        if (copy_from_user(&config, (struct gpio_config __user *)arg, 
                          sizeof(config)))
            return -EFAULT;
        ret = gpio_write_pin(config.gpio_num, config.value);
        break;

    case GPIO_SET_INTERRUPT:
        if (copy_from_user(&config, (struct gpio_config __user *)arg, 
                          sizeof(config)))
            return -EFAULT;
        ret = gpio_set_interrupt(config.gpio_num, config.value);
        break;

    case GPIO_READ_INT_STATUS:
        if (copy_from_user(&config, (struct gpio_config __user *)arg, 
                          sizeof(config)))
            return -EFAULT;
        ret = gpio_read_int_status(config.gpio_num, &config.value);
        if (ret == 0) {
            if (copy_to_user((struct gpio_config __user *)arg, &config, 
                            sizeof(config)))
                return -EFAULT;
        }
        break;

    case GPIO_CLEAR_INT_STATUS:
        if (copy_from_user(&config, (struct gpio_config __user *)arg, 
                          sizeof(config)))
            return -EFAULT;
        ret = gpio_clear_int_status(config.gpio_num);
        break;

    default:
        return -ENOTTY;
    }

    return ret;
}

static const struct file_operations gpio_fops = {
    .owner = THIS_MODULE,
    .open = gpio_open,
    .release = gpio_release,
    .unlocked_ioctl = gpio_ioctl,
};

/* Module initialization */
static int __init gpio_driver_init(void)
{
    int ret;
    struct device *device;

    pr_info("GPIO Driver: Initializing\n");

    /* Allocate device structure */
    gpio_dev = kzalloc(sizeof(struct gpio_device), GFP_KERNEL);
    if (!gpio_dev)
        return -ENOMEM;

    /* Initialize spinlock */
    spin_lock_init(&gpio_dev->lock);

    /* Request memory region */
    /* Map memory */
    gpio_dev->base_addr = ioremap(GPIO_BASE_ADDR, GPIO_MEM_SIZE);
    if (!gpio_dev->base_addr) {
        pr_err("GPIO Driver: Failed to map memory\n");
        ret = -ENOMEM;
        goto err_ioremap;
    }

    /* Allocate character device number */
    ret = alloc_chrdev_region(&gpio_dev->devt, 0, 1, DRIVER_NAME);
    if (ret < 0) {
        pr_err("GPIO Driver: Failed to allocate device number\n");
        goto err_alloc_chrdev;
    }

    /* Initialize character device */
    cdev_init(&gpio_dev->cdev, &gpio_fops);
    gpio_dev->cdev.owner = THIS_MODULE;

    /* Add character device */
    ret = cdev_add(&gpio_dev->cdev, gpio_dev->devt, 1);
    if (ret < 0) {
        pr_err("GPIO Driver: Failed to add character device\n");
        goto err_cdev_add;
    }

    /* Create device class */
    gpio_dev->class = class_create(DRIVER_NAME);
    if (IS_ERR(gpio_dev->class)) {
        pr_err("GPIO Driver: Failed to create device class\n");
        ret = PTR_ERR(gpio_dev->class);
        goto err_class_create;
    }

    /* Create device node */
    device = device_create(gpio_dev->class, NULL, gpio_dev->devt, 
                          NULL, DRIVER_NAME);
    if (IS_ERR(device)) {
        pr_err("GPIO Driver: Failed to create device\n");
        ret = PTR_ERR(device);
        goto err_device_create;
    }

    /* Register interrupt handler if IRQ number is provided */
    if (gpio_irq >= 0) {
        gpio_dev->irq = gpio_irq;
        
        ret = request_irq(gpio_dev->irq, gpio_irq_handler, 
                         IRQF_SHARED | IRQF_TRIGGER_RISING, 
                         DRIVER_NAME, gpio_dev);
        if (ret) {
            pr_err("GPIO Driver: Failed to request IRQ %d (error %d)\n", 
                   gpio_dev->irq, ret);
            pr_err("GPIO Driver: Continuing without interrupt support\n");
            gpio_dev->irq = -1;  /* Mark IRQ as not registered */
        } else {
            pr_info("GPIO Driver: IRQ %d registered successfully\n", 
                    gpio_dev->irq);
        }
    } else {
        gpio_dev->irq = -1;
        pr_info("GPIO Driver: No IRQ specified, running without interrupt support\n");
        pr_info("GPIO Driver: Load with 'gpio_irq=<num>' parameter to enable interrupts\n");
    }

    pr_info("GPIO Driver: Successfully initialized\n");
    pr_info("GPIO Driver: Device created at /dev/%s\n", DRIVER_NAME);
    pr_info("GPIO Driver: Major=%d, Minor=%d\n", 
            MAJOR(gpio_dev->devt), MINOR(gpio_dev->devt));

    return 0;

err_device_create:
    class_destroy(gpio_dev->class);
err_class_create:
    cdev_del(&gpio_dev->cdev);
err_cdev_add:
    unregister_chrdev_region(gpio_dev->devt, 1);
err_alloc_chrdev:
    iounmap(gpio_dev->base_addr);
err_ioremap:
    release_mem_region(GPIO_BASE_ADDR, GPIO_MEM_SIZE);
err_request_mem:
    kfree(gpio_dev);
    return ret;
}

/* Module cleanup */
static void __exit gpio_driver_exit(void)
{
    pr_info("GPIO Driver: Cleaning up\n");

    /* Free IRQ if registered */
    if (gpio_dev->irq >= 0) {
        free_irq(gpio_dev->irq, gpio_dev);
        pr_info("GPIO Driver: IRQ %d freed\n", gpio_dev->irq);
    }

    /* Destroy device */
    device_destroy(gpio_dev->class, gpio_dev->devt);

    /* Destroy class */
    class_destroy(gpio_dev->class);

    /* Remove character device */
    cdev_del(&gpio_dev->cdev);

    /* Unregister device number */
    unregister_chrdev_region(gpio_dev->devt, 1);

    /* Unmap memory */
    iounmap(gpio_dev->base_addr);

    /* Release memory region */
    release_mem_region(GPIO_BASE_ADDR, GPIO_MEM_SIZE);

    /* Free device structure */
    kfree(gpio_dev);

    pr_info("GPIO Driver: Successfully removed\n");
}

module_init(gpio_driver_init);
module_exit(gpio_driver_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Enes");
MODULE_DESCRIPTION("GPIO Driver");
MODULE_VERSION("1.0");
