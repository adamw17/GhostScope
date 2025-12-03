#define pr_fmt(fmt) "gs_execmon: " fmt

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/kprobes.h>
#include <linux/sched.h>
#include <linux/uaccess.h>
#include <linux/ptrace.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("adam");
MODULE_DESCRIPTION("GhostScope exec monitor via kprobes (x86)");
MODULE_VERSION("0.1");

#define GS_FNAMESZ 256

static const char *probe_symbols[] = {
    "__x64_sys_execve",
    "__x64_sys_execveat",
    "do_execveat_common", /* fallback*/
    NULL
};

static struct kprobe kp = {
    .symbol_name = NULL,
};

static int gs_pre_handler(struct kprobe *p, struct pt_regs *regs)
{
    char fname[GS_FNAMESZ];
    const char __user *user_fname = NULL;
    long ret;

    user_fname = (const char __user *)regs->di;

    if (!user_fname) {
        pr_info_ratelimited("no filename pointer (NULL)\n");
        return 0;
    }

    ret = strncpy_from_user(fname, user_fname, sizeof(fname));
    if (ret <= 0) {
        pr_info_ratelimited("pid=%d comm=%s exec <unreadable>\n",
                            current->pid, current->comm);
        return 0;
    }

    if (ret >= sizeof(fname))
        fname[GS_FNAMESZ - 1] = '\0';
    else
        fname[ret] = '\0';

    pr_info_ratelimited("pid=%d comm=%s exec %s\n",
                        current->pid, current->comm, fname);

    return 0;
}

/*Helper to try and register probe*/
static int try_register_probe(const char *sym)
{
    int ret;
    kp.symbol_name = sym;
    kp.pre_handler = gs_pre_handler;
    kp.post_handler = NULL;

    ret = register_kprobe(&kp);
    if (ret == 0) {
        pr_info("registered kprobe on %s\n", sym);
    } else {
        pr_warn("register_kprobe failed on %s: %d\n", sym, ret);
    }
    return ret;
}

static int __init gs_init(void)
{
    const char **s;
    int ret;

    pr_info("init: attempting to register exec kprobe\n");

    for (s = probe_symbols; *s != NULL; s++) {
        ret = try_register_probe(*s);
        if (ret == 0)
            return 0; /* success */
    }

    pr_err("failed to register any exec kprobe - module not loaded\n");
    return -ENODEV;
}

static void __exit gs_exit(void)
{
    unregister_kprobe(&kp);
    pr_info("unregistered kprobe (if any)\n");
}

module_init(gs_init);
module_exit(gs_exit);

