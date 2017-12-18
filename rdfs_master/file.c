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
	errval = rdfs_establish_mapping(inode);
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
	vaddr = (unsigned long)ni_info->i_virt_addr;
	if(vaddr)
	{
		if(atomic_dec_and_test(&ni_info->i_p_counter))
		{
			err = rdfs_destroy_mapping(inode);
		}
	}
	return err;
}

char * local_rw_buffer;
unsigned long local_rw_addr;

static int rdfs_init_local_rw_buffer()
{
	local_rw_buffer = (char*)kmalloc(RDFS_BLOCK_SIZE,GFP_KERNEL);
	local_rw_addr = rdfs_mapping_address(local_rw_buffer,RDFS_BLOCK_SIZE,0);
	return 0;
}
static int rdfs_destroy_local_rw_buffer()
{
	kfree(local_rw_buffer);
	return 0;
}
ssize_t rdfs_local_file_read(struct file *filp,char __user *buf,size_t length,loff_t *ppos)
{
	rdfs_trace();
	struct address_space *mapping = filp->f_mapping;
	struct inode * inode = mapping->host;
	struct rdfs_inode * ri = get_rdfs_inode(inode->i_sb,inode->i_ino);
	struct rdfs_inode_info * ri_info = RDFS_I(inode);
	size_t retval = 0,copied = 0;

	unsigned long offset = *ppos;
	unsigned long start = offset,end = offset + length;
	unsigned long i = 0,pte = 0,block_offset = 0;
	unsigned long s_id = 0,block_id = 0;
	unsigned long size = 0;
	for(i=start;i<end;)
	{
		pte = i >> RDFS_BLOCK_SHIFT;
		block_offset = i & RDFS_BLOCK_OFFSET_MASK;
		i += RDFS_BLOCK_SIZE - block_offset;
		size = RDFS_BLOCK_SIZE - block_offset;
		retval = rdfs_search_metadata(ri_info,pte,&s_id,&block_id);
		retval = rdfs_rdma_block_rw(local_rw_addr,s_id,block_id,block_offset,RDFS_READ);
		copied += __copy_to_user(buf+copied,local_rw_buffer,size);
	}
	return length - copied;
}

ssize_t rdfs_local_file_write(struct file *filp,const char __user *buf,size_t length,loff_t *ppos)
{
	rdfs_trace();
	struct address_space *mapping = filp->f_mapping;
	struct inode * inode = mapping->host;
	struct rdfs_inode * ri = get_rdfs_inode(inode->i_sb,inode->i_ino);
	struct rdfs_inode_info * ri_info = RDFS_I(inode);
	size_t retval = 0;

	long pages_exist = 0,pages_to_alloc = 0,pages_needed = 0;
	pages_needed = (*ppos + length + sb->s_blocksize -1) >> sb->s_blocksize_bits;
	pages_exist = (size + sb->s_blocksize - 1) >> sb->s_blocksize_bits;
	pages_to_alloc = pages_needed - pages_exist;
	if(pages_to_alloc > 0)
	{
		retval = rdfs_alloc_blocks(inode,pages_to_alloc,1,ALLOC_PAGE);
		if(retval < 0)
		{
			printk("%s --> rdfs_alloc_blocks failed\n",__FUNCTION__);
			return retval;
		}
	}
	unsigned long offset = *ppos;
	unsigned long start = offset,end = offset + length;
	unsigned long i = 0,pte = 0,block_offset = 0;
	unsigned long s_id = 0,block_id = 0;
	unsigned long size = 0,copied = 0;
	for(i=start;i<end;)
	{
		pte = i >> RDFS_BLOCK_SHIFT;
		block_offset = i & RDFS_BLOCK_OFFSET_MASK;
		i += RDFS_BLOCK_SIZE - block_offset;
		size = RDFS_BLOCK_SIZE - block_offset;
		retval = __copy_from_user(local_rw_buffer,buf,size);
		retval = rdfs_search_metadata(ri_info,pte,&s_id,&block_id);
		copied += rdfs_rdma_block_rw(local_rw_addr,s_id,block_id,block_offset,RDFS_WRITE);
	}
	return length - copied;
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

