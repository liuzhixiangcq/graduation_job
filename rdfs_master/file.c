/* @author page
 * @date 2017/12/3
 * rdfs/file.c
 */


#include <linux/fs.h>
#include <linux/sched.h>
#include <linux/aio.h>
#include <linux/slab.h>
#include <linux/uio.h>
#include <linux/mm.h>
#include <linux/uaccess.h>

#include <linux/delay.h>

#include "rdfs.h"
#include "server.h"
#include "acl.h"
#include "xip.h"
#include "xattr.h"
#include "inode.h"
#include "pgtable.h"
#define CLEAN 1
#define NO_CLEAN 0
#define LOCK 1
#define NO_LOCK 0
#define iov_iter_rw(i)((0 ? (struct iov_iter*)0: (i))->type & RW_MASK)


static int rdfs_open_file(struct inode *inode, struct file *filp)
{
	rdfs_trace();
	int errval = 0;
	struct rdfs_inode* nv_i = get_rdfs_inode(inode->i_sb,inode->i_ino);
	filp->f_flags |= O_DIRECT;
	if(S_ISREG(inode->i_mode))
		atomic_inc(&(RDFS_I(inode)->i_p_counter));
	return generic_file_open(inode, filp);
}


static int rdfs_release_file(struct file * file)
{
	rdfs_trace();
        struct inode *inode = file->f_mapping->host;
	struct rdfs_inode_info *ni_info;
	unsigned long vaddr;
	int err = 0;
	struct rdfs_inode* ni = get_rdfs_inode(inode->i_sb,inode->i_ino);
	return err;
}


unsigned long dma_start_addr = 0x210000000;
char __user *current_buf;
unsigned long current_length;
extern int posted_req_cnt;

ssize_t rdfs_local_file_read(struct file *filp,char __user *buf,size_t length,loff_t *ppos)
{
	rdfs_trace();

}

ssize_t rdfs_local_file_write(struct file *filp,const char __user *buf,size_t length,loff_t *ppos)
{
	rdfs_trace();
}
static int rdfs_check_flags(int flags)
{
	rdfs_trace();
	if(!(flags&O_DIRECT))
		return -EINVAL;

	return 0;
}
const struct file_operations rdfs_file_operations =
{
	.llseek = generic_file_llseek,
	.read_iter = generic_file_read_iter,
	.write_iter = generic_file_write_iter,
	.read = rdfs_local_file_read,
	.write = rdfs_local_file_write,
	.mmap = generic_file_mmap,
	.open = rdfs_open_file,
	.nvrelease = rdfs_release_file,
	.fsync = noop_fsync,
	.check_flags = rdfs_check_flags,
};

#ifdef CONFIG_RDFS_XIP
const struct file_operations rdfs_xip_file_operations = 
{
/*	.llseek		= generic_file_llseek,
	.read		= xip_file_read,
	.write		= xip_file_write,
	.mmap		= xip_file_mmap,
	.open		= generic_file_open,
	.fsync		= noop_fsync,*/
};
#endif


const struct inode_operations rdfs_file_inode_operations = 
{
#ifdef CONFIG_RDFS_XATTR
	.setxattr	= generic_setxattr,
	.getxattr	= generic_getxattr,
	.listxattr	= rdfs_listxattr,
	.removexattr	= generic_removexattr,
#endif
	.setattr	= rdfs_notify_change,
	.get_acl	= rdfs_get_acl,
};

