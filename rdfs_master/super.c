/* @author page
 * @date 2017/12/3
 * rdfs/super.c
 */

 #include <linux/module.h>
 #include <linux/init.h>
 #include <linux/kernel.h>
 #include <linux/string.h>
 #include <linux/fs.h>
 #include <linux/slab.h>
 #include <linux/init.h>
 #include <linux/blkdev.h>
 #include <linux/pagemap.h>
 #include <linux/quotaops.h>
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
 #include "debug.h"
 #include "tool.h"
 #include "const.h"
 #include "wprotect.h"
 #include "balloc.h"
 #include "pgtable.h"
 #include "inode.h"
 #include "dir.h"
 #include "rdfs_config.h"
 #include "memory.h"
 struct super_block* RDFS_SUPER_BLOCK_ADDRESS;
 EXPORT_SYMBOL(RDFS_SUPER_BLOCK_ADDRESS);

//extern int match_token(char *s, const match_table_t table, substring_t args[]);
 static struct rdfs_inode * rdfs_nvm_init(struct super_block *sb, unsigned long size, unsigned long end_bdev)
 {
     unsigned long bpi, blocksize, physaddr;
 
     struct rdfs_inode *root_i ,*root_1;
     struct rdfs_super_block *super;
     struct rdfs_sb_info *sbi = RDFS_SB(sb);
 
     struct numa_des* numa_des_p0,* numa_des_p1;
 
     unsigned long phys_bdev_end;
 
     rdfs_info("creating an empty rdfsfs of size %lu\n", size);
     rdfs_info("The physaddr:0x%lx,virtual address: 0x%lx\n",
         (unsigned long)sbi->phy_addr,(unsigned long)sbi->virt_addr);
 
     if (!sbi->virt_addr) {
         printk(KERN_ERR "get the virtual address failed\n");
         return ERR_PTR(-EINVAL);
     }
 
     if (!sbi->blocksize)
         //blocksize = PAGESIZE;
         blocksize = RDFS_BLOCK_SIZE;
 
     else
         blocksize = sbi->blocksize;
 
     rdfs_set_blocksize(sb, blocksize);
     blocksize = sb->s_blocksize;
 
     if (sbi->blocksize && sbi->blocksize != blocksize)
         sbi->blocksize = blocksize;
 
     if (size < blocksize) {
         printk(KERN_ERR "size smaller then block size\n");
         return ERR_PTR(-EINVAL);
     }
 
     if (!sbi->bpi)
         bpi = 1;
     else
         bpi = sbi->bpi;
     
     super = rdfs_get_super(sb);
     super->s_size = cpu_to_le64(size);
     super->s_blocksize = cpu_to_le32(blocksize);
     super->s_magic = cpu_to_le16(RDFS_SUPER_MAGIC);
 
     physaddr = rdfs_pa(sbi->virt_addr);
     phys_bdev_end = rdfs_pa((void*)end_bdev);
     
     if (physaddr + size < rdfs_NUMA_LINE || physaddr > rdfs_NUMA_LINE)
     {
         super->s_numa_flag=0;
         super->s_numa_des[0]=(unsigned long)end_bdev;
         super->s_numa_des[1]=(unsigned long)NULL;
         rdfs_init_numa(phys_bdev_end , physaddr + size , bpi);
         numa_des_p0=(struct numa_des*)(super->s_numa_des[0]);
         super->s_inode_count = cpu_to_le64(numa_des_p0->free_inode_count);
         super->s_block_count = cpu_to_le64(numa_des_p0->free_page_count);
     }
     else
     {
         super->s_numa_flag=1;
         super->s_numa_des[0]=(unsigned long)end_bdev;
         super->s_numa_des[1]=(unsigned long)rdfs_va(rdfs_NUMA_LINE);
         rdfs_init_numa(phys_bdev_end , rdfs_NUMA_LINE , bpi);
         rdfs_init_numa(rdfs_NUMA_LINE, physaddr + size, bpi);
         numa_des_p0=(struct numa_des*)(super->s_numa_des[0]);
         numa_des_p1=(struct numa_des*)(super->s_numa_des[1]);
         super->s_inode_count = cpu_to_le64(numa_des_p0->free_inode_count + numa_des_p1->free_inode_count);
         super->s_block_count = cpu_to_le64(numa_des_p0->free_page_count  + numa_des_p1->free_page_count );
     }
     
 
     printk("%s des[0]:%lx,des[1]:%lx\n",__FUNCTION__,super->s_numa_des[0],super->s_numa_des[1]);
     printk("super->s_block_count:%lx\n",le64_to_cpu(super->s_block_count));
 
     rdfs_init_bdev_pool(sb);
 
     rdfs_sync_super(super);
     
     root_i = get_rdfs_inode(sb, RDFS_ROOT_INO);
     printk("%s root_i:%lx\n",__FUNCTION__,(unsigned long)root_i);
     root_i->i_mode = cpu_to_le16(sbi->mode | S_IFDIR);
     root_i->i_uid = (sbi->uid).val;
     root_i->i_gid = (sbi->gid).val;
     root_i->i_link_counts = cpu_to_le16(2);
 
     root_i->list_pro=(unsigned long)root_i;
     root_i->list_next=(unsigned long)root_i;
     
     /*establish pagetable for root inode*/
 
     rdfs_init_pg_table(sb, RDFS_ROOT_INO);
    // rdfs_init_pte_free_list(RDFS_PTE_PAGE_NUMS);
     rdfs_sync_inode(root_i);
 
     if (super->s_numa_flag)
     {
         root_1 = get_rdfs_inode(sb , 2);
         root_1->list_pro=(unsigned long)root_1;
         root_1->list_next=(unsigned long)root_1;
     }
     return root_i;
 }
 
struct kmem_cache * rdfs_inode_cachep;

static struct inode *rdfs_alloc_inode(struct super_block *sb)
{
    rdfs_trace();
    struct rdfs_inode_info *vi = (struct rdfs_inode_info *)
        kmem_cache_alloc(rdfs_inode_cachep,GFP_NOFS);
    if (!vi) {
        return NULL;
    }
    memset(vi,0,sizeof(struct rdfs_inode_info)-sizeof(struct inode));
    vi->vfs_inode.i_version = 1;
    return &vi->vfs_inode;
}

static void rdfs_destroy_inode(struct inode *inode)
{
    rdfs_trace();
	call_rcu(&inode->i_rcu,rdfs_i_callback);
}
int rdfs_write_inode(struct inode *inode, struct writeback_control *wbc)
{
    rdfs_trace();
	return rdfs_update_inode(inode);
}
void rdfs_dirty_inode(struct inode *inode)
{
	rdfs_trace();
	rdfs_update_inode(inode);
}
void rdfs_evict_inode(struct inode * inode)
{
    rdfs_trace();
	int want_delete = 0;
	rdfs_establish_mapping(inode);
	if (!inode->i_nlink && !is_bad_inode(inode)){
		want_delete = 1;
		dquot_initialize(inode);
	}else{
	//	dquot_drop(inode);
	}
	truncate_inode_pages(&inode->i_data, 0);
	if (want_delete) {
		sb_start_intwrite(inode->i_sb);
		/* set dtime */
		RDFS_I(inode)->i_dtime	= get_seconds();
		mark_inode_dirty(inode);
		__rdfs_write_inode(inode, inode_needs_sync(inode));
		rdfs_truncate_blocks(inode, 0,inode->i_size);
		inode->i_size = 0;
	}

	invalidate_inode_buffers(inode);
	clear_inode(inode);

	rdfs_destroy_mapping(inode);

	if (want_delete) {
		rdfs_free_inode(inode);
		sb_end_intwrite(inode->i_sb);
	}
}
static void rdfs_put_super(struct super_block *sb)
{
    rdfs_trace();
	struct rdfs_sb_info *sbi = RDFS_SB(sb);
	struct rdfs_super_block *nsb=rdfs_get_super(sb);
	u64 size=le64_to_cpu(nsb->s_size);
	sb->s_fs_info = NULL;
	if (sbi->virt_addr!=NULL)
	{
		rdfs_iounmap(sb,size);
	}
	kfree(sbi);
}
int rdfs_remount(struct super_block *sb,int *mntflags,char *data)
{
    rdfs_trace();
	return 0;
}
int rdfs_statfs(struct dentry *d, struct kstatfs *buf)
{
	rdfs_trace();
	struct super_block *sb = d->d_sb;
	struct rdfs_super_block *ns = rdfs_get_super(sb);
	buf->f_type = RDFS_SUPER_MAGIC;			
	buf->f_bsize = sb->s_blocksize;
	buf->f_blocks = le64_to_cpu(ns->s_block_count);
	buf->f_bfree = buf->f_bavail = rdfs_count_free_blocks(sb);
	buf->f_files = le64_to_cpu(ns->s_inode_count);

	buf->f_namelen = 128;
	return 0;
}
static int rdfs_show_options(struct seq_file *seq, struct dentry *root)
{
	struct rdfs_sb_info *sbi = RDFS_SB(root->d_sb);

	seq_printf(seq, ".physaddr=0x%016llx", (u64)sbi->phy_addr);
	if (sbi->initsize)
		seq_printf(seq, ",init=%luk", sbi->initsize >> 10);
	if (sbi->blocksize)
		seq_printf(seq, ",bs=%lu", sbi->blocksize);
	if (sbi->bpi)
		seq_printf(seq, ",bpi=%lu", sbi->bpi);
	if (sbi->num_inodes)
		seq_printf(seq, ",N=%lu", sbi->num_inodes);
	if (sbi->mode != (S_IRWXUGO | S_ISVTX))
		seq_printf(seq, ",mode=%03o", sbi->mode);
	if (!uid_eq(sbi->uid, GLOBAL_ROOT_UID))
		seq_printf(seq, ",uid=%u",
				from_kuid_munged(&init_user_ns, sbi->uid));
	if (!gid_eq(sbi->gid, GLOBAL_ROOT_GID))
		seq_printf(seq, ",gid=%u",
				from_kgid_munged(&init_user_ns, sbi->gid));
	if (test_opt(root->d_sb, ERRORS_RO))
		seq_puts(seq, ",errors=remount-ro");
	if (test_opt(root->d_sb, ERRORS_PANIC))
		seq_puts(seq, ",errors=panic");

#ifdef CONFIG_rdfsFS_XATTR
	/* user xattr not enabled by default */
	if (test_opt(root->d_sb, XATTR_USER))
		seq_puts(seq, ",user_xattr");
#endif

#ifdef CONFIG_rdfsFS_POSIX_ACL
	/* acl not enabled by default */
	if (test_opt(root->d_sb, POSIX_ACL))
		seq_puts(seq, ",acl");
#endif

#ifdef CONFIG_rdfsFS_XIP
	/* xip not enabled by default */
	if (test_opt(root->d_sb, XIP))
		seq_puts(seq, ",xip");
#endif

	return 0;
}
static struct super_operations rdfs_sops = 
{
	.alloc_inode	= rdfs_alloc_inode,
	.destroy_inode	= rdfs_destroy_inode,
	.write_inode	= rdfs_write_inode,
	.dirty_inode	= rdfs_dirty_inode,
	.evict_inode	= rdfs_evict_inode,
	.put_super	    = rdfs_put_super,	
	.remount_fs	    = rdfs_remount,
	.statfs		    = rdfs_statfs,
	.show_options	= rdfs_show_options,
};

int rdfs_fill_super(struct super_block *sb, void *data, int silent)
{
    struct inode *root_i;
    struct rdfs_inode *root_rdfsi;
    struct rdfs_super_block *super, *super_redund;
    struct rdfs_sb_info *sbi = NULL;
    unsigned long initsize, blocksize,end_bdev;
    u32 random = 0;
    int retval = 0;
    rdfs_trace();
    sbi = kzalloc(sizeof(struct rdfs_sb_info),GFP_KERNEL);
    if (!sbi) return -ENOMEM;

    RDFS_SUPER_BLOCK_ADDRESS=sb;
    
    sb->s_fs_info = sbi;        
    set_default_opts(sbi);
	sbi->phy_addr = get_phys_addr(&data);
	printk("%s sbi->phy_addr:%lx\n",__FUNCTION__,sbi->phy_addr);
    rdfs_get_init_size(sbi,&data);
    if(sbi->phy_addr == (phys_addr_t)ULLONG_MAX)
	    goto out;

    get_random_bytes(&random, sizeof(u32));
    atomic_set(&sbi->next_generation, random);

    /* Init with default values */
	sbi->mode = (S_IRWXUGO | S_ISVTX);
	sbi->uid = current_fsuid();
	sbi->gid = current_fsgid();

	sbi->bdev_count=0;
	atomic_set(&(sbi->atomic_bdev_post),0);

	initsize = sbi->initsize;

	retval=rdfs_ioremap(sb,initsize);
    printk("%s rdfs_ioremap retval:%d\n",__FUNCTION__,retval);
	
    if (!retval)
	goto out;

    end_bdev = rdfs_get_bdev((unsigned long)sbi,&data);
    printk("%s end_bdev=%lx\n",__FUNCTION__,end_bdev);
    if (rdfs_parse_options(data, sbi, 0))
		goto out;
    if (initsize) {
        root_rdfsi = rdfs_nvm_init(sb,initsize,end_bdev);
        if (IS_ERR(root_rdfsi)) {
            goto out;
        }
        super = rdfs_get_super(sb);
        goto setup_sb;
    }
    
    rdfs_info("Check physaddr 0x%lx for rdfsfs image!\n",(unsigned long)sbi->phy_addr);

    if(!sbi->virt_addr){
    	printk(KERN_ERR "get rdfsfs start virtual address failed\n");
	goto out;
    }

    super = rdfs_get_super(sb);
    super_redund = rdfs_get_redund_super(sb);
    printk("%s --> rdfs_super:%lx\n",__FUNCTION__,super);
    blocksize = le32_to_cpu(super->s_blocksize);
    rdfs_set_blocksize(sb,blocksize);

    initsize = le64_to_cpu(super->s_size);
    rdfs_info("rdfsfs image appears to be %lu KB in size\n", initsize>>10);
	rdfs_info("blocksize %lu\n", blocksize);
 setup_sb:                      

    super->p_address_to_ino=(unsigned long)sbi->virt_addr+PAGE_SIZE;
    printk("p_address_to_ino:%lx\n",(unsigned long)super->p_address_to_ino);
    sb->s_magic = super->s_magic;
    sb->s_op = &rdfs_sops; 
    sb->s_maxbytes = rdfs_max_size(sb->s_blocksize_bits);
    sb->s_max_links =  RDFS_LINK_MAX;
    sb->s_flags |= MS_NOSEC;
    
    printk("%s --> rdfs_iget\n",__FUNCTION__);
    root_i = rdfs_iget(sb,RDFS_ROOT_INO);
	root_i->i_size=0;
	printk("%s --> rdfs_iget root_i:%lx\n",__FUNCTION__,root_i);
    rdfs_make_empty(root_i,root_i);
    if (IS_ERR(root_i)) {
        retval = PTR_ERR(root_i);
        goto out;
    }
    sb->s_root = d_make_root(root_i);
    if (!sb->s_root) {
        printk(KERN_ERR"get 2rdfsfs root inode failed!\n");
        retval = -ENOMEM;
        goto out;
    }

    retval = 0;
    return retval;
 out:
    if (sbi->virt_addr) {       
        rdfs_info("The zone virtual address not empty!\n");
    }
    kfree(sbi);                 
	return retval;
}
