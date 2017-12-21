/* @author page
 * @date 2017/12/3
 * rdfs/dir.c
 */

#include <linux/sched.h>
#include <linux/fs.h>
#include <linux/types.h>
#include <linux/pagemap.h>
#include <linux/init.h>
#include <linux/mm.h>
#include <linux/mm_types.h>
#include <linux/kernel.h>
#include <linux/kallsyms.h>
#include <linux/list.h>
#include <asm/page.h>
#include "rdfs.h"
#include "debug.h"
#include "pgtable.h"
#include "inode.h"
#include "memory.h"
static unsigned char rdfs_filetype_table[RDFS_FT_MAX] = 
{
	[RDFS_FT_UNKNOWN]		= DT_UNKNOWN,
	[RDFS_FT_REG_FILE]	= DT_REG,
	[RDFS_FT_DIR]		= DT_DIR,
	[RDFS_FT_CHRDEV]		= DT_CHR,
	[RDFS_FT_BLKDEV]		= DT_BLK,
	[RDFS_FT_FIFO]		= DT_FIFO,
	[RDFS_FT_SOCK]		= DT_SOCK,
	[RDFS_FT_SYMLINK]		= DT_LNK,
};

#define S_SHIFT 12
static unsigned char rdfs_type_by_mode[S_IFMT >> S_SHIFT] = {
	[S_IFREG >> S_SHIFT]	= RDFS_FT_REG_FILE,
	[S_IFDIR >> S_SHIFT]	= RDFS_FT_DIR,
	[S_IFCHR >> S_SHIFT]	= RDFS_FT_CHRDEV,
	[S_IFBLK >> S_SHIFT]	= RDFS_FT_BLKDEV,
	[S_IFIFO >> S_SHIFT]	= RDFS_FT_FIFO,
	[S_IFSOCK >> S_SHIFT]	= RDFS_FT_SOCK,
	[S_IFLNK >> S_SHIFT]	= RDFS_FT_SYMLINK,
};


static inline int rdfs_match(int len,const char* const name,struct rdfs_dir_entry *de)
{
	rdfs_trace();
    if(len != de->name_len)
        return 0;
    if(!de->inode)
        return 0;
    return !memcmp(name,de->name,len);    
}

static inline struct rdfs_dir_entry * rdfs_next_entry(struct rdfs_dir_entry *p)
{
	rdfs_trace();
	return (struct rdfs_dir_entry *)((char *)p + cpu_to_le16(p->rec_len));

}



static int rdfs_readdir(struct file *file, struct dir_context *ctx)
{
	rdfs_trace();
	loff_t pos = ctx->pos;
	int err = 0;
	struct inode *inode = file_inode(file);
	struct super_block *sb = inode->i_sb;
	unsigned long offset = pos && ~PAGE_CACHE_MASK;
	unsigned char *types = NULL;
	char *start_addr, *end_addr;
	struct rdfs_dir_entry *nde;

	types = rdfs_filetype_table;

	err = rdfs_establish_mapping(inode);


	if(pos >= inode->i_size)
		goto final;

	if(file->f_version != inode->i_version){
		if(offset){
			offset = 0;
			ctx->pos = (loff_t)(RDFS_I(inode)->i_virt_addr);
		}
		file->f_version = inode->i_version;
	}
	start_addr = RDFS_I(inode)->i_virt_addr;
	end_addr = start_addr + inode->i_size;
	if(start_addr >= end_addr)
		goto final;
	nde = (struct rdfs_dir_entry *)start_addr;
	for(;(char *)nde < end_addr;nde =  rdfs_next_entry(nde)){
		if(nde->rec_len == 0){
			//rdfs_error(sb, __FUNCTION__, "zero-length directory entry\n");
			printk("error :%s \n",__FUNCTION__);
			err = -EIO;
			goto final;
		}
		if(nde->inode)
		{
			unsigned char d_type = DT_UNKNOWN;
			if(types && nde->file_type < RDFS_FT_MAX)
				d_type = types[nde->file_type];
			if(!dir_emit(ctx, nde->name, nde->name_len, le64_to_cpu(nde->inode),d_type))
				goto final;
		}
		ctx->pos += le16_to_cpu(nde->rec_len);
	}

final:
	err = rdfs_destroy_mapping(inode);

	return err;
}

struct rdfs_dir_entry *rdfs_find_entry2(struct inode *dir, struct qstr *child, struct rdfs_dir_entry **up_nde)
{
	rdfs_trace();
	const char *name = child->name;
	int namelen = child->len;
	unsigned long  n = i_size_read(dir);
	void *start, *end;
	struct rdfs_dir_entry *nde;
	struct rdfs_dir_entry *prev;

	start = RDFS_I(dir)->i_virt_addr;
	
	if(unlikely(n <= 0))
		goto out;

	nde = (struct rdfs_dir_entry *)start;
	prev = (struct rdfs_dir_entry *)start;
	end = start + n;

	while((void *)nde < end)
	{
		if(unlikely(nde->rec_len == 0)){
			printk("error :%s \n",__FUNCTION__);
			//rdfs_error(dir->i_sb,__FUNCTION__,"zero-length directory entry\n");
			goto out;
		}
		if(rdfs_match(namelen, name, nde))
			goto found;
		prev = (struct rdfs_dir_entry *)nde;
		nde = rdfs_next_entry(nde);
	}
out:
	
	return NULL;

found:

    *up_nde = prev;
	return nde;
}

ino_t rdfs_inode_by_name(struct inode *dir,struct qstr *child)
{
	rdfs_trace();
    ino_t res = 0;
    struct rdfs_dir_entry *de;
    struct rdfs_dir_entry *nouse = NULL;

	rdfs_establish_mapping(dir);
    de = rdfs_find_entry2(dir,child,&nouse);
    if(de)
        res = le32_to_cpu(de->inode);
	rdfs_destroy_mapping(dir);
	
    return res;
}

static inline void rdfs_set_de_type(struct rdfs_dir_entry *de,struct inode *inode)
{
	rdfs_trace();
	umode_t mode = inode->i_mode;
	de->file_type = rdfs_type_by_mode[(mode & S_IFMT) >> S_SHIFT];
}

void rdfs_set_link(struct inode *dir, struct rdfs_dir_entry *de, struct inode *inode,int update_times)
{
	rdfs_trace();
    de->inode = cpu_to_le64(inode->i_ino);
    rdfs_set_de_type(de, inode);
    if(likely(update_times))
        dir->i_mtime = dir->i_ctime = CURRENT_TIME_SEC;
    RDFS_I(dir)->i_flags &= ~RDFS_BTREE_FL;
    mark_inode_dirty(dir);
}

int rdfs_add_link(struct dentry *dentry,struct inode *inode)
{
	rdfs_trace();
	struct inode *dir = dentry->d_parent->d_inode;
	const char *name = dentry->d_name.name;
    int namelen = dentry->d_name.len;
    unsigned reclen = RDFS_DIR_REC_LEN(namelen);
    unsigned short rec_len = 0,name_len = 0;
    unsigned long size;
    unsigned long nrpages;
    struct rdfs_inode_info *ni;
    struct rdfs_dir_entry *de;
    char *vaddr,*eaddr;
    char *dir_end;
	int err = 0;
    int i;

	ni = RDFS_I(dir);
	vaddr = ni->i_virt_addr;

	if(unlikely(!vaddr))
    {
		printk("error :%s \n",__FUNCTION__);
    	err = -ENOMEM;
    	goto out;
	}
    de = (struct rdfs_dir_entry *)vaddr;        
	size = le64_to_cpu(dir->i_size);
    eaddr = vaddr + size;
    nrpages = size >> PAGE_SHIFT;

    for(i = 0;i <= nrpages;i++)
    {
        dir_end = (void *)(de) + PAGE_SIZE - reclen;
        while((char *)de <= dir_end)
        {
            if((char *)de == eaddr)
            {
                name_len = 0;
        		rec_len = PAGE_SIZE;
        		goto need_new_block;
            }
            if(unlikely(de->rec_len == 0))
            {
                //rdfs_error(dir->i_sb,__func__,"zero_length directory entry");
				err = -EIO;
				printk("error :%s \n",__FUNCTION__);
                goto out;
            }
            if(rdfs_match(namelen,name,de))
            {
				printk("error :%s \n",__FUNCTION__);
                err = -EEXIST;
        		goto out;
            }
            name_len = RDFS_DIR_REC_LEN(de->name_len);
            rec_len = le16_to_cpu(de->rec_len);
            if(!de->inode && rec_len >= reclen)   
        		goto got_it;
            if(rec_len >= name_len + reclen)   
        		goto got_it;
            de = (struct rdfs_dir_entry *)((char *)de + rec_len);
        }
    }
    return -EINVAL;
	
need_new_block:
	if(dir->i_size + PAGE_SIZE <= MAX_DIR_SIZE)
    {
		err = rdfs_alloc_blocks(dir, 1, 0,ALLOC_PAGE);
		i_size_write(dir, dir->i_size + PAGE_SIZE);
		if(unlikely(err))
        {
			printk("error :%s \n",__FUNCTION__);
			//rdfs_error(dir->i_sb, __FUNCTION__, "alloc new block failed!\n");
			return err;
		}
	}
    else
    {
		err = -EIO;
		goto out;
	}

	de->rec_len = cpu_to_le16(PAGE_SIZE);
        de->inode = 0;

got_it:
	
	if(de->inode)
    {
	struct rdfs_dir_entry *de1 = (struct rdfs_dir_entry *)((char *)de + name_len);
	int temp_rec_len = rec_len - name_len;
        de1->rec_len = cpu_to_le16(temp_rec_len);
        de->rec_len = cpu_to_le16(name_len);
        de = de1;
    }
	de->name_len = namelen;
	memcpy(de->name,name,namelen);
	de->inode = cpu_to_le64(inode->i_ino);
	rdfs_set_de_type(de,inode);
	dir->i_mtime = dir->i_ctime = CURRENT_TIME_SEC;
	RDFS_I(dir)->i_flags &= ~RDFS_BTREE_FL;
	mark_inode_dirty(dir);
   	err = 0;
out:
	return err;
}


int rdfs_delete_entry(struct rdfs_dir_entry *dir,struct rdfs_dir_entry **pdir,struct inode *parent)
{
	rdfs_trace();
    unsigned short rec_len = 0;
    unsigned short temp;
    struct rdfs_dir_entry *prev;
    int err = 0;
    prev = *pdir;
    if(unlikely(dir->rec_len == 0))
    {
		printk("error :%s \n",__FUNCTION__);
        //rdfs_error(parent->i_sb,__func__,"zero_length directory entry");
        err = -EIO;
        goto out;
    }
    if(likely(dir))
        rec_len = le16_to_cpu(dir->rec_len);
    dir->inode = 0;
    temp = le16_to_cpu(prev->rec_len);
    temp += rec_len;
    prev->rec_len = cpu_to_le16(temp);
    parent->i_ctime = parent->i_mtime = CURRENT_TIME_SEC;
    RDFS_I(parent)->i_flags &= ~RDFS_BTREE_FL;
    mark_inode_dirty(parent);

 out:
    return err;    
}

int rdfs_make_empty(struct inode *inode,struct inode *parent)
{
	rdfs_trace();
	struct rdfs_inode_info *ni;
	struct rdfs_dir_entry *de;
	void *vaddr;
	int err = 0;

	err = rdfs_establish_mapping(inode);
	printk("%s -->1\n",__FUNCTION__);
	if(unlikely(err))
    {
		printk("error :%s \n",__FUNCTION__);
		//rdfs_error(inode->i_sb, __FUNCTION__, "establish mapping!\n");
		return err;
	}
	printk("%s -->2\n",__FUNCTION__);
	if(inode->i_size == 0)
    {
		err = rdfs_alloc_blocks(inode, 1, 0,ALLOC_PAGE);
		i_size_write(inode, PAGE_SIZE);
		if(unlikely(err))
        {
			printk("error :%s \n",__FUNCTION__);
			//rdfs_error(inode->i_sb, __FUNCTION__, "alloc new block failed!\n");
			return err;
	    }
	}
    else
    {
		printk("error :%s \n",__FUNCTION__);
		err = -EIO;
		goto fail;
	}
    printk("%s -->3\n",__FUNCTION__);
	ni = RDFS_I(inode);
	vaddr = ni->i_virt_addr;
	if(unlikely(!vaddr))
    {
		printk("error :%s \n",__FUNCTION__);
    	err = -ENOMEM;
   		goto fail;
    }
   
	de = (struct rdfs_dir_entry *)vaddr;

	de->name_len = 1;

	de->rec_len = cpu_to_le16(RDFS_DIR_REC_LEN(1));

	memcpy(de->name,".\0\0",4);

	de->inode = cpu_to_le32(inode->i_ino);
	
	rdfs_set_de_type(de,inode);

	de = (struct rdfs_dir_entry *)(vaddr + RDFS_DIR_REC_LEN(1));

	de->name_len = 2;
	de->rec_len = cpu_to_le16(PAGE_SIZE - RDFS_DIR_REC_LEN(1));
	de->inode = cpu_to_le32(parent->i_ino);

	memcpy(de->name,"..\0",4);

	rdfs_set_de_type(de,inode);

fail:
	
	err = rdfs_destroy_mapping(inode); //maybe wrong ,don't assign the err
	return err;
}

struct rdfs_dir_entry *rdfs_dotdot(struct inode *dir)
{
	rdfs_trace();
    struct rdfs_dir_entry *de = NULL;
    struct rdfs_inode_info *ni = RDFS_I(dir);
    void *vaddr = NULL;
    
    vaddr = ni->i_virt_addr;
    de = rdfs_next_entry((struct rdfs_dir_entry *)vaddr);
    return de;
}


int rdfs_empty_dir(struct inode *inode)
{
	rdfs_trace();
	struct rdfs_inode_info *ni;
    struct rdfs_dir_entry *de;
    unsigned long size = le64_to_cpu(inode->i_size);
    void *vaddr;

    ni = RDFS_I(inode);
    vaddr = ni->i_virt_addr;
    de = (struct rdfs_dir_entry *)vaddr;
    vaddr += size - RDFS_DIR_REC_LEN(1);

    while((void *)de <= vaddr){
        if(unlikely(de->rec_len == 0)){
            printk("error :%s \n",__FUNCTION__);
            printk("vaddr=%p,de=%p\n",vaddr,de);
            goto not_empty;
        }
        if(de->inode != 0){
            if(de->name[0] != '.')
                goto not_empty;
            if(de->name_len > 2)
                goto not_empty;
            if(de->name_len < 2){
                if(de->inode != cpu_to_le32(inode->i_ino))
                    goto not_empty;
            }else if(de->name[1] != '.')
                goto not_empty;
        }
        de = rdfs_next_entry(de);
    }
    return 1;
 not_empty:
    return 0;
}

const struct file_operations rdfs_dir_operations = 
{
	.llseek		= generic_file_llseek,
	.read		= generic_read_dir,
	.iterate	= rdfs_readdir,
	//.unlocked_ioctl	= rdfs_ioctl,
#ifdef CONFIG_COMPAT
	//.compat_ioctl	= rdfs_compat_ioctl,
#endif
	.fsync		= noop_fsync,
};


