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
#include <linux/linkage.h>
#include <linux/wait.h>
#include <linux/kdev_t.h>
#include <linux/dcache.h>
#include <linux/path.h>
#include <linux/stat.h>
#include <linux/cache.h>
#include <linux/list.h>
#include <linux/llist.h>
#include <linux/radix-tree.h>
#include <linux/rbtree.h>
#include <linux/init.h>
#include <linux/pid.h>
#include <linux/bug.h>
#include <linux/mutex.h>
#include <linux/capability.h>
#include <linux/semaphore.h>
#include <linux/fiemap.h>
#include <linux/rculist_bl.h>
#include <linux/atomic.h>
#include <linux/shrinker.h>
#include <linux/migrate_mode.h>
#include <linux/uidgid.h>
#include <linux/lockdep.h>
#include <linux/percpu-rwsem.h>
#include <linux/blk_types.h>

#include <asm/byteorder.h>
#include <uapi/linux/fs.h>
#include <linux/delay.h>

#include "rdfs.h"
#include "server.h"
#include "acl.h"
#include "xip.h"
#include "xattr.h"
#include "inode.h"
#include "pgtable.h"
#include "rdfs_config.h"
#include "memory.h"
#define CLEAN 1
#define NO_CLEAN 0
#define LOCK 1
#define NO_LOCK 0
#define iov_iter_rw(i)((0 ? (struct iov_iter*)0: (i))->type & RW_MASK)

char * local_rw_buffer;
unsigned long local_rw_addr;
int flag = 0;

int rdfs_init_local_rw_buffer(void)
{
	if(flag)
		return 0;
	flag = 1;
	local_rw_buffer = (char*)kmalloc(RDFS_BLOCK_SIZE,GFP_KERNEL);
	local_rw_addr = rdfs_mapping_address(local_rw_buffer,RDFS_BLOCK_SIZE);
	return 0;
}
int rdfs_destroy_local_rw_buffer(void)
{
	if(!flag)
		return 0;
	flag = 0;
	kfree(local_rw_buffer);
	return 0;
}
extern void print_pgtable(struct mm_struct* mm,unsigned long address);
static int rdfs_open_file(struct inode *inode, struct file *filp)
{
	rdfs_trace();
	//BUG_ON(1);
	int errval = 0;
	errval = rdfs_establish_mapping(inode);
	struct rdfs_inode* nv_i = get_rdfs_inode(inode->i_sb,inode->i_ino);
	printk("%s ino:%lx\n",__FUNCTION__,inode->i_ino);
	struct rdfs_inode_info * ri_info = RDFS_I(inode);
	printk("%s ri_info->i_virt_addr:%lx\n",__FUNCTION__,ri_info->i_virt_addr);
	unsigned long address = (unsigned long)ri_info->i_virt_addr;
	extern struct mm_struct init_mm;
	print_pgtable(&init_mm,address);
	filp->f_flags |= O_DIRECT;
	rdfs_init_local_rw_buffer();
	if(S_ISREG(inode->i_mode))
		atomic_inc(&(RDFS_I(inode)->i_p_counter));
	if(!(filp->f_flags & O_LARGEFILE))
	{
		printk("%s flag error\n",__FUNCTION__);
	}
	if(i_size_read(inode) > MAX_NON_LFS)
	{
		printk("%s inode size :%lx eror\n",__FUNCTION__,i_size_read(inode));
	}
	printk("%s open end\n",__FUNCTION__);
	return 0;
	//return generic_file_open(inode, filp);
}


static int rdfs_release_file(struct file * file)
{
	rdfs_trace();
        struct inode *inode = file->f_mapping->host;
	struct rdfs_inode_info *ni_info;
	unsigned long vaddr;
	int err = 0;
	struct rdfs_inode* ni = get_rdfs_inode(inode->i_sb,inode->i_ino);
	rdfs_destroy_local_rw_buffer();
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
ssize_t rdfs_direct_IO(struct kiocb *iocb,struct iov_iter *iter,loff_t offset)
{
	rdfs_trace();
	struct file *file=iocb->ki_filp;
	struct inode*inode=file->f_mapping->host;
	struct super_block*sb=inode->i_sb;
	ssize_t retval=0;
	int hole=0;
	loff_t size;
	void* start_addr = RDFS_I(inode)->i_virt_addr+offset;
	size_t length=iov_iter_count(iter);
	unsigned long pages_exist=0,pages_to_alloc=0,pages_need=0;
	size=i_size_read(inode);	
	return iov_iter_count(iter);
}
EXPORT_SYMBOL(rdfs_direct_IO);
ssize_t rdfs_local_file_read(struct file *filp,char __user *buf,size_t length,loff_t *ppos)
{
	rdfs_trace();
	struct address_space *mapping = filp->f_mapping;
	struct inode * inode = mapping->host;
	struct super_block *sb = inode->i_sb;
	struct rdfs_inode * ri = get_rdfs_inode(inode->i_sb,inode->i_ino);
	struct rdfs_inode_info * ri_info = RDFS_I(inode);
	size_t retval = 0,copied = 0;
	loff_t i_size = 0;
	if(i_size <= 0)
	{
		return length;
	}
	if(*ppos+length>i_size)
		length = i_size - *ppos;
	if(length <= 0)
	{
		return length;
	}
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
	*ppos = *ppos + length;
	return length - copied;
}

ssize_t rdfs_local_file_write(struct file *filp,const char __user *buf,size_t length,loff_t *ppos)
{
	rdfs_trace();
	struct address_space *mapping = filp->f_mapping;
	struct inode * inode = mapping->host;
	struct super_block *sb = inode->i_sb;
	struct rdfs_inode * ri = get_rdfs_inode(inode->i_sb,inode->i_ino);
	struct rdfs_inode_info * ri_info = RDFS_I(inode);
	size_t retval = 0;
	loff_t i_size=0;
	i_size = i_size_read(inode);
	long pages_exist = 0,pages_to_alloc = 0,pages_needed = 0;
	pages_needed = (*ppos + length + sb->s_blocksize -1) >> sb->s_blocksize_bits;
	pages_exist = (i_size + sb->s_blocksize - 1) >> sb->s_blocksize_bits;
	pages_to_alloc = pages_needed - pages_exist;
	if(pages_to_alloc > 0)
	{
		retval = rdfs_alloc_blocks(inode,pages_to_alloc,1,ALLOC_PTE);
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
	if(*ppos + length > i_size)
	{	
		i_size_write(inode, *ppos + length);
	}
	*ppos = *ppos + length;
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

