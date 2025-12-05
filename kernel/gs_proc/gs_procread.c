#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>

#define PROC_NAME "gs_proc_demo"

static int gs_proc_show(struct seq_file *m, void *v)
{
    seq_printf(m, "hello from %s\n", PROC_NAME);
    seq_printf(m, "jiffies: %lu\n", jiffies);
    return 0;
}

static int gs_proc_open(struct inode *inode, struct file *file)
{
    return single_open(file, gs_proc_show, NULL);
}

static const struct proc_ops gs_proc_ops = {
    .proc_open    = gs_proc_open,
    .proc_read    = seq_read,
    .proc_lseek   = seq_lseek,
    .proc_release = single_release,
};

static int __init gs_proc_init(void)
{
    proc_create(PROC_NAME, 0, NULL, &gs_proc_ops);
    pr_info("%s: loaded\n", PROC_NAME);
    return 0;
}

static void __exit gs_proc_exit(void)
{
    remove_proc_entry(PROC_NAME, NULL);
    pr_info("%s: unloaded\n", PROC_NAME);
}

module_init(gs_proc_init);
module_exit(gs_proc_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("adam");
MODULE_DESCRIPTION("Proc filesystem demo");
