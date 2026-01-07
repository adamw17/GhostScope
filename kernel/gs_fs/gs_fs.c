#include <linux/module.h>
#include <linux/init.h>
#include <linux/time.h>
#include <linux/uio.h>      // for kiocb, iov_iter
                            //
//filesystem includes
#include <linux/fs.h>
#include <linux/fs_context.h>
#include <linux/pagemap.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Adam");
MODULE_DESCRIPTION("Minimal virtual filesystem");
MODULE_VERSION("0.1");

#define HELLO_FILENAME "hello"
#define HELLO_MSG "Hello from the kernel!\n"
#define GS_FS_MAGIC 0x20240107


//necessary filesystem functions
static int gs_fs_fill_super(struct super_block *sb, struct fs_context *fc);
static int gs_fs_fill_super_dynamic(struct super_block *sb, struct fs_context *fc);
static ssize_t gs_hello_read(struct file *file, char __user *buf, size_t count, loff_t *pos);
static void gs_fs_kill_sb(struct super_block *sb);
static void gs_fs_evict_inode(struct inode *inode);
static int gs_fs_init_fs_context(struct fs_context *fc);
static int gs_fs_get_tree(struct fs_context *fc);

//filesystem declarations
static const struct super_operations gs_fs_super_ops = {
    .statfs = simple_statfs,
    .drop_inode = inode_just_drop,
    .evict_inode = gs_fs_evict_inode,
};

static const struct inode_operations gs_hello_iops = {
    .setattr = simple_setattr,        // allows truncate, etc.
};

static const struct fs_context_operations gs_fs_context_ops = {
	.get_tree = gs_fs_get_tree,
};

static struct file_system_type gs_fs_type = {
	.owner   = THIS_MODULE,
	.name    = "gs_fs",
	.init_fs_context   = gs_fs_init_fs_context,
	.kill_sb = gs_fs_kill_sb,
};

static const struct file_operations gs_hello_fops = {
    .owner   = THIS_MODULE,
    .read    = gs_hello_read,          // We only need read for now
    .llseek  = generic_file_llseek,
};

static ssize_t gs_hello_read(struct file *file, char __user *buf, size_t count, loff_t *pos)
{
    const char *msg = HELLO_MSG;
    size_t len = strlen(msg);
    ssize_t ret;

    if (*pos >= len)
        return 0;  // EOF

    if (*pos + count > len)
        count = len - *pos;

    ret = copy_to_user(buf, msg + *pos, count);
    if (ret)
        return -EFAULT;

    *pos += count;
    return count;
}

static void gs_fs_evict_inode(struct inode *inode)
{
    truncate_inode_pages_final(&inode->i_data);
    clear_inode(inode);
}

//Retaining this function for now to return to later
//Implementing using the more simple kernel vfs systems
static int __maybe_unused gs_fs_fill_super_dynamic(struct super_block *sb, struct fs_context *fc)
{
    struct inode *root_inode;
    struct inode *hello_inode;
    struct dentry *root_dentry;
    struct dentry *hello_dentry;

    sb->s_magic = GS_FS_MAGIC;
    sb->s_blocksize = PAGE_SIZE;
    sb->s_blocksize_bits = PAGE_SHIFT;
    sb->s_time_gran = 1;

    /* Root directory inode */
    root_inode = new_inode(sb);
    if (!root_inode)
        return -ENOMEM;

    root_inode->i_ino = 1;
    root_inode->i_mode = S_IFDIR | 0755;
    //root_inode->i_atime = root_inode->i_mtime = root_inode->i_ctime = current_time(root_inode);

    inode_set_atime_to_ts(root_inode, current_time(root_inode));
    inode_set_mtime_to_ts(root_inode, current_time(root_inode));
    inode_set_ctime_to_ts(root_inode, current_time(root_inode));

    root_inode->i_op = &simple_dir_inode_operations;
    root_inode->i_fop = &simple_dir_operations;

    /* Create root dentry */
    root_dentry = d_make_root(root_inode);
    if (!root_dentry)
        return -ENOMEM;

    sb->s_root = root_dentry;

    /* Now create the "hello" file */
    hello_inode = new_inode(sb);
    if (!hello_inode)
        return -ENOMEM;

    hello_inode->i_ino = 2;  // Any unique number > 1
    hello_inode->i_mode = S_IFREG | 0444;  // Regular file, read-only for all
    hello_inode->i_fop = &gs_hello_fops;
    //hello_inode->i_atime = hello_inode->i_mtime = hello_inode->i_ctime = current_time(hello_inode);

    inode_set_atime_to_ts(hello_inode, current_time(hello_inode));
    inode_set_mtime_to_ts(hello_inode, current_time(hello_inode));
    inode_set_ctime_to_ts(hello_inode, current_time(hello_inode));

    /* Size of the file (for stat) */
    hello_inode->i_size = strlen(HELLO_MSG);
    hello_inode->i_op = &gs_hello_iops;

    /* Add the file to the root directory */
    hello_dentry = d_alloc_name(root_dentry, HELLO_FILENAME);
    if (!hello_dentry) {
        iput(hello_inode);
        return -ENOMEM;
    }

    d_add(hello_dentry, hello_inode);

    return 0;
} 

static int gs_fs_fill_super(struct super_block *sb, struct fs_context *fc)
{
    static const struct tree_descr files[] = {
        {HELLO_FILENAME, &gs_hello_fops, 0444},
        { "" }
    };

    sb->s_op = &gs_fs_super_ops;

    return simple_fill_super(sb, GS_FS_MAGIC, files);
}

static int gs_fs_init_fs_context(struct fs_context *fc)
{
	fc->ops = &gs_fs_context_ops;
	return 0;
}

static int gs_fs_get_tree(struct fs_context *fc)
{
	return get_tree_nodev(fc, gs_fs_fill_super);
}

static void gs_fs_kill_sb(struct super_block *sb){
    pr_info("gs_fs: superblock kill called");
    //kill_anon_super(sb);
    kill_litter_super(sb);
}

static int __init gs_fs_init(void)
{
	int ret;

	ret = register_filesystem(&gs_fs_type);
	if (ret) {
		pr_err("gs_fs: failed to register filesystem\n");
		return ret;
	}

	pr_info("gs_fs: filesystem registered\n");
	return 0;
}

static void __exit gs_fs_exit(void)
{
	unregister_filesystem(&gs_fs_type);
	pr_info("gs_fs: filesystem unregistered\n");
}

module_init(gs_fs_init);
module_exit(gs_fs_exit);
