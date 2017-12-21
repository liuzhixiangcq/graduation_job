/* @author page
 * @date 2017/12/3
 * rdfs/inode.c
 */

 #include <linux/time.h>
 #include <linux/fs.h>
 #include <linux/hardirq.h>
 #include <linux/sched.h>
 #include <linux/highuid.h>
 #include <linux/quotaops.h>
 #include <linux/sched.h>
 #include <linux/mpage.h>
 #include <linux/backing-dev.h>
 
 #include "rdfs.h"
 #include "xattr.h"
 #include "xip.h"
 #include "acl.h"
 #include "inode.h"
 #include "rdfs.h"
 #include "nvmalloc.h"
 #include "pgtable.h"
 #include "balloc.h"
 #include "tool.h"
 #include "wprotect.h"

 #include "memory.h"
static DEFINE_SPINLOCK(read_inode_lock);
static DEFINE_SPINLOCK(update_inode_lock);

extern struct inode_operations rdfs_file_inode_operations;
extern struct file_operations rdfs_xip_file_operations;
extern struct file_operations rdfs_file_operations;
extern struct inode_operations rdfs_symlink_inode_operations ;
extern struct file_operations rdfs_dir_operations;
extern struct inode_operations rdfs_dir_inode_operations;

static int rdfs_readpage(struct file *file, struct page *page);
int rdfs_get_xip_mem(struct address_space *a_ops, pgoff_t offset, int t,void **arg, unsigned long *m)
{
	return 0;
}
const struct address_space_operations rdfs_aops_xip = 
{
	.get_xip_mem	= rdfs_get_xip_mem,
};
const struct address_space_operations rdfs_aops = 
{
	.readpage	= rdfs_readpage,
	.direct_IO	= rdfs_direct_IO,
};

u64 rdfs_find_data_block(struct inode *inode, unsigned long file_blocknr)
{
	struct super_block *sb = inode->i_sb;
	struct rdfs_inode *ni = NULL;
	rdfs_trace();
	u64 *first_lev = NULL;		/* ptr to first level */
	u64 *second_lev = NULL;	/* ptr to second level */
	u64 *third_lev = NULL;		/* ptr to the third level*/
	u64 first_phys = 0;
	u64 second_phys = 0;
	u64 third_phys = 0;
	u64 bp = 0;		/* phys of this number*/
	int entry_offset = 511;
	int entry_num = 9;	
	int first_num = 0, second_num = 0, third_num = 0;

	first_num = ((file_blocknr >> entry_num ) >> entry_num) & entry_offset;
	second_num = (file_blocknr >> entry_num) & entry_offset;
	third_num = file_blocknr & entry_offset;

	ni = get_rdfs_inode(sb, inode->i_ino);

	first_phys = le64_to_cpu(ni->i_pg_addr);
	first_lev =rdfs_va(first_phys);
	if (first_lev) {
		second_phys = le64_to_cpu(first_lev[first_num]) & PAGE_MASK;
		second_lev =rdfs_va(second_phys);
		if (second_lev){
			third_phys = le64_to_cpu(second_lev[second_num]) & PAGE_MASK;
			third_lev =rdfs_va(third_phys);
			if(third_lev)
			{
				bp = third_lev[third_num];
			}
		}
	}

	return bp;
}
static void __rdfs_truncate_blocks(struct inode *inode, loff_t start, loff_t end)
{
	rdfs_trace();
	struct super_block *sb = inode->i_sb;
	struct rdfs_inode *ni;
	unsigned long first_blocknr,last_blocknr;
	struct rdfs_inode_info *ni_info;
	pud_t *pud;
	unsigned long ino;
	unsigned long vaddr;
	struct mm_struct *mm;
	mm = current->mm;
	ni_info = RDFS_I(inode);
	vaddr = (unsigned long)ni_info->i_virt_addr;
	ino = inode->i_ino;
	pud = rdfs_get_pud(sb, ino);
	ni = get_rdfs_inode(sb, ino);

	if(!ni->i_pg_addr)
		{
			printk("error :%s \n",__FUNCTION__);
			return ;
		}
	
	first_blocknr = (start + sb->s_blocksize-1) >> sb->s_blocksize_bits;
	
	if(ni->i_flags & cpu_to_le32(RDFS_EOFBLOCKS_FL))
		last_blocknr = (1UL << (2*sb->s_blocksize_bits - 6))-1;
	else
		last_blocknr = end >> sb->s_blocksize_bits;

	if(first_blocknr > last_blocknr)
		return;

	unnvmap(vaddr, pud, mm);
	rdfs_rm_pg_table(sb, inode->i_ino);
	inode->i_blocks = 0;
	
}

void rdfs_truncate_blocks(struct inode *inode, loff_t start, loff_t end)
{
	rdfs_trace();
	if (!(S_ISREG(inode->i_mode) || S_ISDIR(inode->i_mode) ||
	      S_ISLNK(inode->i_mode)))
		return;

	__rdfs_truncate_blocks(inode, start, end);
	inode->i_mtime = inode->i_ctime = CURRENT_TIME_SEC;
	rdfs_update_inode(inode);
}

int rdfs_alloc_blocks(struct inode *inode, int num, int zero,int type)
{
	rdfs_trace();
	struct super_block *sb = inode->i_sb;
	int errval = 0,i = 0;
	//unsigned long blocknr;
	int ino = inode->i_ino;
	pud_t *pud;
	unsigned long vaddr;
	struct mm_struct *mm;
	struct rdfs_inode_info *ni_info;
	struct rdfs_inode *ni = get_rdfs_inode(sb, ino);
	struct rdfs_sb_info *nsi = RDFS_SB(sb);
	unsigned long *temp;
	unsigned long pte_addr;
	phys_addr_t phys, base;
	mm = current->mm;
	base = nsi->phy_addr;
	ni_info = RDFS_I(inode);
	vaddr = (unsigned long)ni_info->i_virt_addr;
	if(!ni->i_pg_addr){
		rdfs_init_pg_table(sb, ino);
		pud = rdfs_get_pud(sb, ino);
		nvmap(sb, vaddr, pud, mm);
	}
	if(type == ALLOC_PAGE)
		errval = rdfs_new_block(&phys, num, ni);
	errval = 0;
    if(!errval)
    {
        for(i = 0;i < num; i++)
        {
			if(type == ALLOC_PAGE)
			{
				temp = (unsigned long *)(rdfs_va(phys));
				phys = *temp;
				if (zero==0)memset(temp, 0, PAGE_SIZE);
			}
			else
			{
				rdfs_new_pte(ni_info,&pte_addr);
			}
            
	           
            inode->i_blocks++;
            ni->i_blocks =  cpu_to_le32(inode->i_blocks);
			if(type == ALLOC_PAGE)
				errval = rdfs_insert_page(sb, inode, rdfs_pa(temp));
			else
				errval = rdfs_insert_page(sb,inode,pte_addr);
            if(unlikely(errval != 0))
                return errval;
        }
        return errval;
    }else{
		printk("error :%s \n",__FUNCTION__);
        //rdfs_error(sb, __FUNCTION__, "no block space left!\n");
        return -ENOSPC;
    }
}

struct backing_dev_info rdfs_backing_dev_info __read_mostly = {
	.ra_pages       = 0,   
	.capabilities	= BDI_CAP_NO_ACCT_AND_WRITEBACK,
};

static int rdfs_read_inode(struct inode *inode, struct rdfs_inode *ni)
{
	rdfs_trace();
	int ret = -EIO;
	struct rdfs_inode_info *ni_info;
	ni_info = RDFS_I(inode);
	spin_lock(&read_inode_lock);
	 if (rdfs_calc_checksum((u8 *)ni, RDFS_INODE_SIZE)) {
		printk("error :%s \n",__FUNCTION__); 
	 	//rdfs_error(inode->i_sb, (char *)rdfs_read_inode, (char *)"checksum error in inode %08x\n",  (u32)inode->i_ino); 
	 	goto bad_inode;
	 }

	inode->i_mode = le32_to_cpu(ni->i_mode);
	i_uid_write(inode, le32_to_cpu(ni->i_uid));
	i_gid_write(inode, le32_to_cpu(ni->i_gid));
	set_nlink(inode, le32_to_cpu(ni->i_link_counts));
	inode->i_size = le64_to_cpu(ni->i_size);
	inode->i_atime.tv_sec = le32_to_cpu(ni->i_atime);
	inode->i_ctime.tv_sec = le32_to_cpu(ni->i_ctime);
	inode->i_mtime.tv_sec = le32_to_cpu(ni->i_mtime);
	inode->i_atime.tv_nsec = inode->i_mtime.tv_nsec =
	inode->i_ctime.tv_nsec = 0;
	
	inode->i_generation = le32_to_cpu(ni->i_generation);
	rdfs_set_inode_flags(inode);
	/* check if the inode is active. */
	if (inode->i_nlink == 0 && (inode->i_mode == 0 || le32_to_cpu(ni->i_dtime))) {
		/* this inode is deleted */
		rdfs_dbg("read inode: inode %lu not active", inode->i_ino);
		ret = -EINVAL;
		goto bad_inode;
	}

	inode->i_blocks = le64_to_cpu(ni->i_blocks);
	ni_info->i_flags = le32_to_cpu(ni->i_flags);
	ni_info->i_file_acl = le32_to_cpu(ni->i_file_acl);
	ni_info->i_dir_acl = 0;
	ni_info->i_dtime = 0;
	ni_info->i_state = 0;
	ni_info->i_flags = le32_to_cpu(ni->i_flags);
	inode->i_ino = get_rdfs_inodenr(inode->i_sb,rdfs_pa(ni));
	inode->i_mapping->a_ops = &rdfs_aops;
	inode->i_mapping->backing_dev_info = &rdfs_backing_dev_info;

	insert_inode_hash(inode);
	switch (inode->i_mode & S_IFMT) {
	case S_IFREG:
		if (rdfs_use_xip(inode->i_sb)) {
			inode->i_mapping->a_ops = &rdfs_aops_xip;
			inode->i_fop = &rdfs_xip_file_operations;
		} else {
			inode->i_op = &rdfs_file_inode_operations;
			inode->i_fop = &rdfs_file_operations;
		}
		break;
	case S_IFDIR:
		inode->i_op = &rdfs_dir_inode_operations;
		inode->i_fop = &rdfs_dir_operations;
		break;
	case S_IFLNK:
		inode->i_op = &rdfs_symlink_inode_operations;
		break;
	default:
		inode->i_size = 0;
		break;
	}


	spin_unlock(&read_inode_lock);
	return 0;

 bad_inode:
 	printk("error :%s \n",__FUNCTION__);
	make_bad_inode(inode);

	return ret;
}

int rdfs_update_inode(struct inode *inode)
{
	rdfs_trace();
	struct rdfs_inode *ni;
	int retval = 0;

	ni = get_rdfs_inode(inode->i_sb, inode->i_ino);
	if (!ni)
		{
			printk("error :%s \n",__FUNCTION__);
			return -EACCES;
		}
	spin_lock(&update_inode_lock);

	rdfs_memunlock_inode(inode->i_sb, ni);
	ni->i_mode = cpu_to_le32(inode->i_mode);
	ni->i_uid = (inode->i_uid).val;
	ni->i_gid = (inode->i_gid).val;
	ni->i_link_counts = cpu_to_le32(inode->i_nlink);
	ni->i_size = cpu_to_le64(inode->i_size);
	ni->i_blocks = cpu_to_le64(inode->i_blocks);
	ni->i_atime = cpu_to_le32(inode->i_atime.tv_sec);
	ni->i_ctime = cpu_to_le32(inode->i_ctime.tv_sec);
	ni->i_mtime = cpu_to_le32(inode->i_mtime.tv_sec);
	ni->i_generation = cpu_to_le32(inode->i_generation);
	rdfs_memlock_inode(inode->i_sb, ni);
	spin_unlock(&update_inode_lock);
	return retval;
}

void rdfs_free_inode(struct inode *inode)
{
	rdfs_trace();
	struct super_block *sb = inode->i_sb;
	struct rdfs_super_block *ns;
	struct rdfs_inode *ni;
	unsigned long inode_phy;
	
	struct numa_des* numa_des;

	ni = get_rdfs_inode(sb, inode->i_ino);
	numa_des = get_rdfs_inode_numa(ni);

	rdfs_xattr_delete_inode(inode);
	
	spin_lock(&numa_des->inode_lock);
	inode_phy = get_rdfs_inode_phy_addr(sb, inode->i_ino);
	if(inode_phy == -BADINO)
		printk("error :%s \n",__FUNCTION__);
		//rdfs_error(sb, __FUNCTION__, (char *)-BADINO);
	
	rdfs_memunlock_inode(sb, ni);

	ni->i_dtime = cpu_to_le32(get_seconds());
	ni->i_pg_addr = 0;

	rdfs_memlock_inode(sb, ni);
	ns = rdfs_get_super(sb);
	rdfs_memunlock_super(sb, ns);

	ni->i_pg_addr = numa_des->free_inode_phy;
	numa_des->free_inode_phy = inode_phy;

	++numa_des->free_inode_count;
	
	rdfs_memlock_super(sb, ns);
	spin_unlock(&numa_des->inode_lock);
}

void rdfs_set_inode_flags(struct inode *inode)
{
	rdfs_trace();
	unsigned int flags = RDFS_I(inode)->i_flags;

	inode->i_flags &= ~(S_SYNC|S_APPEND|S_IMMUTABLE|S_NOATIME|S_DIRSYNC);
	if (flags & RDFS_SYNC_FL)
		inode->i_flags |= S_SYNC;
	if (flags & RDFS_APPEND_FL)
		inode->i_flags |= S_APPEND;
	if (flags & RDFS_IMMUTABLE_FL)
		inode->i_flags |= S_IMMUTABLE;
	if (flags & RDFS_NOATIME_FL)
		inode->i_flags |= S_NOATIME;
	if (flags & RDFS_DIRSYNC_FL)
		inode->i_flags |= S_DIRSYNC;
}

void get_rdfs_inode_flags(struct rdfs_inode_info *ei)
{
	rdfs_trace();
	unsigned int flags = ei->vfs_inode.i_flags;
	ei->i_flags &= ~(RDFS_SYNC_FL|RDFS_APPEND_FL|
			RDFS_IMMUTABLE_FL|RDFS_NOATIME_FL|RDFS_DIRSYNC_FL);
	if (flags & S_SYNC)
		ei->i_flags |= RDFS_SYNC_FL;
	if (flags & S_APPEND)
		ei->i_flags |= RDFS_APPEND_FL;
	if (flags & S_IMMUTABLE)
		ei->i_flags |= RDFS_IMMUTABLE_FL;
	if (flags & S_NOATIME)
		ei->i_flags |= RDFS_NOATIME_FL;
	if (flags & S_DIRSYNC)
		ei->i_flags |= RDFS_DIRSYNC_FL;
}

struct inode *rdfs_iget(struct super_block *sb, unsigned long ino)
{
	rdfs_trace();
	struct inode *inode;
	struct rdfs_inode *ni;
	int err;
	inode = iget_locked(sb, ino);
	inode->i_ino = ino;
	if (unlikely(!inode))
		{
			printk("error :%s \n",__FUNCTION__);
			return ERR_PTR(-ENOMEM);
		}
	if (!(inode->i_state & I_NEW))
		return inode;

	ni = get_rdfs_inode(sb, ino);
	if (!ni) {
		printk("error :%s \n",__FUNCTION__);
		err = -EACCES;
		goto fail;
	}
	err = rdfs_read_inode(inode, ni);
	if (unlikely(err))
		goto fail;

	unlock_new_inode(inode);
	return inode;
fail:
	printk("error :%s \n",__FUNCTION__);
	iget_failed(inode);
	return ERR_PTR(err);
}

static void rdfs_inode_info_init(struct rdfs_inode_info* ni_info)
{
	rdfs_trace();
	if(ni_info!=NULL)
	{
		ni_info->i_state = RDFS_STATE_NEW;
		ni_info->i_file_acl = 0;

		ni_info->i_dir_acl = 0;
		ni_info->i_pg_addr = 0;
		atomic_set(&ni_info->i_p_counter, 0);
		ni_info->i_virt_addr = 0;
		ni_info->w_flag=0;
		
		ni_info->post=0;
		ni_info->mapping_count=0;
		ni_info->per_read=PER_READ;
		//rdfs_swap_buffer_init(&ni_info->swap_buffer);		
		mutex_init(&ni_info->truncate_mutex);
		mutex_init(&ni_info->i_meta_mutex);
		rwlock_init(&ni_info->state_rwlock);
		
		spin_lock_init(&ni_info->used_bitmap_list_lock);
		ni_info->pre_bitmap = NULL;
		ni_info->used_bitmap_list = NULL;
	}
}

struct inode *rdfs_new_inode(struct inode *dir, umode_t mode, const struct qstr *qstr)
{
	rdfs_trace();
	struct super_block *sb;
	struct rdfs_sb_info *sbi;
	struct rdfs_super_block *ns;
	struct inode *inode;
	struct rdfs_inode *ni = NULL;
	struct rdfs_inode_info *ni_info;
	struct rdfs_inode *diri = NULL;
	int errval;
	ino_t ino = 0;

	unsigned long inode_phy;

	struct numa_des* numa_des;

	sb = dir->i_sb;
	inode = new_inode(sb);

	if(!inode)
	{
		printk("error :%s \n",__FUNCTION__);
		return ERR_PTR(-ENOMEM);
	}

	ni_info = RDFS_I(inode);
	sbi = RDFS_SB(sb) ;
	ns = rdfs_get_super(sb);
	numa_des = get_a_numa();
	spin_lock(&numa_des->inode_lock);

	if (numa_des->free_inode_count) 
	{
		inode_phy  = numa_des->free_inode_phy;
		ino = get_rdfs_inodenr(sb, inode_phy);
		ni = get_rdfs_inode(sb, ino);
		numa_des->free_inode_phy = ni->i_pg_addr;
		ni->i_pg_addr = cpu_to_le64(0);
		ni->list_pro=cpu_to_le64(0);
		ni->list_next=cpu_to_le64(0);
		ni->i_accessed_count = 0;
		//lcl
		ni->i_cold_page_start=0;
		ni->i_cold_page_num=0;
		//dmfs_index_init(ni_info);
		
		rdfs_memunlock_super(sb, ns);
		--numa_des->free_inode_count;
		rdfs_memlock_super(sb, ns);
	} 
	else 
	{
		rdfs_dbg("no space left to create new inode!\n");
		errval = -ENOSPC;
		goto fail1;
	}
	diri = get_rdfs_inode(sb, dir->i_ino);
	if(!diri){
		errval = -EACCES;
		goto fail1;
	}

	inode->i_ino = ino;
	printk("%s ino:%lx\n",__FUNCTION__,ino);
	inode_init_owner(inode, dir, mode);

	inode->i_blocks = inode->i_size = 0;
	inode->i_mtime = inode->i_atime = inode->i_ctime = CURRENT_TIME;
	
	rdfs_inode_info_init(ni_info);
	ni_info->i_flags = rdfs_mask_flags(mode, RDFS_I(dir)->i_flags&RDFS_FL_INHERITED);
	ni_info->rdfs_inode=ni;

	inode->i_generation = atomic_add_return(1, &sbi->next_generation);
	rdfs_set_inode_flags(inode);
	if (insert_inode_locked(inode) < 0) {
		printk("error :%s \n",__FUNCTION__);
		errval = -EINVAL;
		goto fail1;
	}

	errval = rdfs_write_inode(inode, 0);
	if(errval)
		goto fail2;

	errval = rdfs_init_acl(inode, dir);

	if (errval)
		goto fail2;

	errval = rdfs_init_security(inode, dir, qstr);
	if (errval)
		goto fail2;

	spin_unlock(&numa_des->inode_lock);

	errval = rdfs_init_pg_table(inode->i_sb, inode->i_ino);
	return inode;
fail2:
	printk("error :%s \n",__FUNCTION__);
	spin_unlock(&numa_des->inode_lock);

	clear_nlink(inode);
	unlock_new_inode(inode);
	iput(inode);
	return ERR_PTR(errval);
fail1:
	printk("error :%s \n",__FUNCTION__);
	spin_unlock(&numa_des->inode_lock);

	make_bad_inode(inode);
	iput(inode);
	return ERR_PTR(errval);
}

static int rdfs_readpage(struct file *file, struct page *page)
{
	rdfs_trace();
	struct inode *inode = page->mapping->host;
	struct super_block *sb = inode->i_sb;
	loff_t offset = 0, size = 0;
	unsigned long fillsize = 0, blocknr = 0, bytes_filled = 0;
	u64 block = 0;
	void *buf = NULL, *bp = NULL;
	int ret = 0;

	buf = kmap(page);
	if (!buf)
		{
			printk("error :%s \n",__FUNCTION__);
			return -ENOMEM;
		}
	
	offset = page_offset(page);
	size = i_size_read(inode);
	blocknr = page->index << (PAGE_CACHE_SHIFT - inode->i_blkbits);
	if (offset < size) {
		size -= offset;
		fillsize = size > PAGE_SIZE ? PAGE_SIZE : size;
		while (fillsize) {
			int count = fillsize > sb->s_blocksize ?
						sb->s_blocksize : fillsize;
			block = rdfs_find_data_block(inode, blocknr);
			if (likely(block)) 
			{
				bp = rdfs_va( block&0x7ffffffffffff000);
				if (!bp) {
					SetPageError(page);
					bytes_filled = 0;
					ret = -EIO;
					goto out;
				}

				memcpy(buf + bytes_filled, bp, count);
			} else {
				memset(buf + bytes_filled, 0, count);
			}
			bytes_filled += count;
			fillsize -= count;
			blocknr++;
		}
	}
 out:
	if (bytes_filled < PAGE_SIZE)
		memset(buf + bytes_filled, 0, PAGE_SIZE - bytes_filled);
	if (ret == 0)
		SetPageUptodate(page);

	flush_dcache_page(page);
	kunmap(page);
	unlock_page(page);

	return ret;
}

static int rdfs_block_truncate_page(struct inode *inode, loff_t newsize)
{
	rdfs_trace();
	struct super_block *sb = inode->i_sb;
	unsigned long offset = newsize & (sb->s_blocksize - 1);
	unsigned long length;
	char *bp;
	int ret = 0;
	if(!offset || newsize > inode->i_size)
		goto out;

	length = sb->s_blocksize - offset;

	bp = RDFS_I(inode)->i_virt_addr;
	if(!bp){
		ret = -EACCES;
		goto out;
	}

	memset(bp + offset, 0, length);

out:
	return ret;
}

static int rdfs_setsize(struct inode *inode, loff_t newsize)
{
	rdfs_trace();
	int ret = 0;
	loff_t oldsize = inode->i_size;
	if (!(S_ISREG(inode->i_mode) || S_ISDIR(inode->i_mode) ||
	    S_ISLNK(inode->i_mode)))
		{
			printk("error :%s \n",__FUNCTION__);
			return -EINVAL;
		}
	if (IS_APPEND(inode) || IS_IMMUTABLE(inode))
		{
			printk("error :%s \n",__FUNCTION__);
			return -EPERM;
		}

	if(newsize != oldsize){
		if (mapping_is_xip(inode->i_mapping))
			ret = xip_truncate_page(inode->i_mapping, newsize);
		else
			ret = rdfs_block_truncate_page(inode, newsize);
		if (ret)
			return ret;

		i_size_write(inode, newsize);
	}

	synchronize_rcu();
	truncate_pagecache(inode, newsize);
	__rdfs_truncate_blocks(inode, newsize, oldsize);
	check_eof_blocks(inode, newsize);
	inode->i_mtime = inode->i_ctime = CURRENT_TIME_SEC;
	rdfs_update_inode(inode);

	return ret;
}

int __rdfs_write_inode(struct inode *inode, int do_sync)
{
	rdfs_trace();
	struct rdfs_inode_info *ni_info = RDFS_I(inode);
	struct super_block *sb = inode->i_sb;
	ino_t ino = inode->i_ino;
	struct rdfs_inode * ni = get_rdfs_inode(sb, ino);
	int err = 0;
	if (IS_ERR(ni))
 		{
			printk("error :%s \n",__FUNCTION__);
			 return -EIO;
		 }

	if (ni_info->i_state & RDFS_STATE_NEW)
	memset(ni, 0, RDFS_SB(sb)->s_inode_size);
	get_rdfs_inode_flags(ni_info);
	ni->i_mode = inode->i_mode;
	
	ni->i_uid = (inode->i_uid).val;
	ni->i_gid = (inode->i_gid).val;
	ni->i_link_counts = cpu_to_le32(inode->i_nlink);
	ni->i_size = cpu_to_le64(inode->i_size);
	ni->i_atime = cpu_to_le32(inode->i_atime.tv_sec);
	ni->i_ctime = cpu_to_le32(inode->i_ctime.tv_sec);
	ni->i_mtime = cpu_to_le32(inode->i_mtime.tv_sec);

	ni->i_blocks = cpu_to_le32(inode->i_blocks);
	ni->i_dtime = cpu_to_le32(ni_info->i_dtime);
	ni->i_flags = cpu_to_le32(ni_info->i_flags);
	ni->i_file_acl = cpu_to_le32(ni_info->i_file_acl);
	if (!S_ISREG(inode->i_mode))
		ni->i_dir_acl = cpu_to_le32(ni_info->i_dir_acl);
		
	ni->i_generation = cpu_to_le32(inode->i_generation);
	
	ni_info->i_state &= ~RDFS_STATE_NEW;
	
	return err;
}

int rdfs_notify_change(struct dentry *dentry, struct iattr *attr)
{
	rdfs_trace();
	struct inode *inode = dentry->d_inode;
	struct rdfs_inode *ni = get_rdfs_inode(inode->i_sb,inode->i_ino);
	int error = 0;
	if(!ni){
		printk("error :%s \n",__FUNCTION__);
		//rdfs_error(inode->i_sb, __FUNCTION__, "inode don't exist\n");
		return -EACCES;
	}

	error = inode_change_ok(inode, attr);
	if (error){
		printk("error :%s \n",__FUNCTION__);
		//rdfs_error(inode->i_sb, __FUNCTION__, "inode change to wrong state\n");
		return error;
	}

	if (attr->ia_valid & ATTR_SIZE && (attr->ia_size != inode->i_size || ni->i_flags & cpu_to_le32(RDFS_EOFBLOCKS_FL))) {
		error = rdfs_setsize(inode, attr->ia_size);
		if (error){
			printk("error :%s \n",__FUNCTION__);
			//rdfs_error(inode->i_sb, __FUNCTION__, "inode set_size wrong\n");
			return error;
		}
	}

	setattr_copy(inode, attr);
	if (attr->ia_valid & ATTR_MODE){
		error = rdfs_acl_chmod(inode);
		if(error){
			printk("error :%s \n",__FUNCTION__);
			//rdfs_error(inode->i_sb, __FUNCTION__, "inode wrong\n");
		}
	}

	error = rdfs_update_inode(inode);
	if(error){
		printk("error :%s \n",__FUNCTION__);
		//rdfs_error(inode->i_sb, __FUNCTION__, "update inode wrong\n");
	}

	return error;
}



