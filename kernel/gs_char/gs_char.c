#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/uaccess.h>

#define DEVICE_NAME "gs_char"
#define BUF_SIZE 256

static dev_t dev_number;
static struct cdev gs_cdev;
static struct class *gs_class;

static char gs_buffer[BUF_SIZE];
static size_t gs_buf_len;

static int gs_open(struct inode *inode, struct file *file)
{
    pr_info("gs_char: device opened\n");
    return 0;
}

static int gs_release(struct inode *inode, struct file *file)
{
    pr_info("gs_char: device closed\n");
    return 0;
}

static ssize_t gs_read(struct file *file, char __user *buf, size_t len, loff_t *offset)
{
    ssize_t to_copy;

    if (*offset >= gs_buf_len)
        return 0;

    to_copy = min(len, gs_buf_len - *offset);

    if (copy_to_user(buf, gs_buffer + *offset, to_copy))
        return -EFAULT;

    *offset += to_copy;

    pr_info("gs_char: read %zd bytes\n", to_copy);
    return to_copy;
}

static ssize_t gs_write(struct file *file, const char __user *buf, size_t len, loff_t *offset)
{
    size_t to_copy;

    if (len > BUF_SIZE)
        to_copy = BUF_SIZE;
    else
        to_copy = len;

    if (copy_from_user(gs_buffer, buf, to_copy))
        return -EFAULT;

    gs_buf_len = to_copy;

    pr_info("gs_char: wrote %zu bytes\n", to_copy);
    return to_copy;
}

static const struct file_operations gs_fops = {
    .owner = THIS_MODULE,
    .open = gs_open,
    .release = gs_release,
    .read = gs_read,
    .write = gs_write,
};

static int __init gs_init(void)
{
    int ret;

    ret = alloc_chrdev_region(&dev_number, 0, 1, DEVICE_NAME);
    if (ret < 0) {
        pr_err("gs_char: could not allocate chrdev region\n");
        return ret;
    }

    cdev_init(&gs_cdev, &gs_fops);
    ret = cdev_add(&gs_cdev, dev_number, 1);
    if (ret < 0) {
        pr_err("gs_char: could not add cdev\n");
        unregister_chrdev_region(dev_number, 1);
        return ret;
    }

    gs_class = class_create("gs_char_class");
    if (IS_ERR(gs_class)) {
        pr_err("gs_char: failed to create class\n");
        cdev_del(&gs_cdev);
        unregister_chrdev_region(dev_number, 1);
        return PTR_ERR(gs_class);
    }

    if (IS_ERR(device_create(gs_class, NULL, dev_number, NULL, DEVICE_NAME))) {
        pr_err("gs_char: failed to create device\n");
        class_destroy(gs_class);
        cdev_del(&gs_cdev);
        unregister_chrdev_region(dev_number, 1);
        return -1;
    }

    pr_info("gs_char: module loaded. Major=%d Minor=%d\n",
             MAJOR(dev_number), MINOR(dev_number));

    return 0;
}

static void __exit gs_exit(void)
{
    device_destroy(gs_class, dev_number);
    class_destroy(gs_class);
    cdev_del(&gs_cdev);
    unregister_chrdev_region(dev_number, 1);

    pr_info("gs_char: module unloaded\n");
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("adam");
MODULE_DESCRIPTION("Minimal character driver");

module_init(gs_init);
module_exit(gs_exit);
