#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Adam Weber");
MODULE_DESCRIPTION("Hello world kernel module");
MODULE_VERSION("0.1");

static int __init hello_init(void)
{
    pr_info("Hello: module loaded!\n");
    return 0;
}

static void __exit hello_exit(void)
{
    pr_info("Hello: module unloaded!\n");
}

module_init(hello_init);
module_exit(hello_exit);
