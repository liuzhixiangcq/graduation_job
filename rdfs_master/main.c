/* @author page
 * @date 2017/12/3
 * rdfs/main.c
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/blkdev.h>
#include <linux/random.h>
#include <linux/buffer_head.h>
#include <linux/exportfs.h>
#include <linux/vfs.h>
#include <linux/seq_file.h>
#include <linux/mount.h>
#include <linux/mm.h>
#include <linux/kthread.h>
#include <linux/delay.h>

#include "rdfs.h"
#include "inode.h"
#include "const.h"
#include "balloc.h"
#include "debug.h"
#include "nvmalloc.h"
#include "tool.h"

static struct dentry *rdfs_mount(struct file_system_type *fs_type,int flags, 
                                const char *dev_name, void *data);

/* rdfs rdma distributed filesystem type*/
static struct file_system_type rdfs_fs_type = 
{
    .owner      = THIS_MODULE,
    .name       = "rdfs",
    .mount      = rdfs_mount,
    .kill_sb    = kill_anon_super,
};

/* mount rdfs to system*/
static struct dentry *rdfs_mount(struct file_system_type *fs_type,int flags, 
                                const char *dev_name, void *data)
{
    rdfs_trace();
	return mount_nodev(fs_type, flags, data, rdfs_fill_super);
}

/* module init*/
static int __init init_rdfs_fs(void)
{
	rdfs_trace();
	int retval = 0;
    retval = nvmalloc_init();
    retval = init_inodecache();
	if(retval)return retval;
    retval = register_filesystem(&rdfs_fs_type);
    if (retval)return retval;
}
/* module remove*/
static void __exit exit_rdfs_fs(void)
{
    rdfs_trace();
	int retval;
    unregister_filesystem(&rdfs_fs_type);
	destory_inodecache();
	//retval = nvmalloc_exit();
}

module_init(init_rdfs_fs)
module_exit(exit_rdfs_fs)

/* module information*/
MODULE_AUTHOR("page");
MODULE_DESCRIPTION("rdfs master");
MODULE_LICENSE("GPL");
MODULE_ALIAS_FS("rdfs");