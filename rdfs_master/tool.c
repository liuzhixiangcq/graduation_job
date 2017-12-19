/* @author page
 * @date 2017/12/3
 * rdfs/tool.c
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
 //#include <linux/smp_lock.h>
 #include <linux/vfs.h>
 #include <linux/seq_file.h>
 #include <linux/mount.h>
 #include <linux/mm.h>
 #include <linux/parser.h>

 #include <linux/kthread.h>
 #include <linux/delay.h>
 #include "const.h"
 #include "debug.h"
 #include "server.h"
 #include "pgtable.h"
 #include "rdfs.h"
 #include "device.h"
 enum 
{
	Opt_addr, Opt_bpi, Opt_size,
	Opt_num_inodes, Opt_mode, Opt_uid,
	Opt_gid, Opt_blocksize, Opt_user_xattr,
	Opt_nouser_xattr, Opt_noprotect,
	Opt_acl, Opt_noacl, Opt_xip,
	Opt_err_cont, Opt_err_panic, Opt_err_ro,
	Opt_err, Opt_bdev
};

static const match_table_t tokens = 
{
	{Opt_addr,		"physaddr=%x"},
	{Opt_bpi,		"bpi=%u"},
	{Opt_size,		"init=%s"},
	{Opt_num_inodes,	"N=%u"},
	{Opt_mode,		"mode=%o"},
	{Opt_uid,		"uid=%u"},
	{Opt_gid,		"gid=%u"},
	{Opt_blocksize,		"bs=%s"},
	{Opt_user_xattr,	"user_xattr"},
	{Opt_user_xattr,	"nouser_xattr"},
	{Opt_noprotect,		"noprotect"},
	{Opt_acl,		"acl"},
	{Opt_acl,		"noacl"},
	{Opt_xip,		"xip"},
	{Opt_err_cont,		"errors=continue"},
	{Opt_err_panic,		"errors=panic"},
	{Opt_err_ro,		"errors=remount-ro"},
	{Opt_err,		NULL},
	{Opt_bdev,		"bdev=%s"}
};
int isdigit(int ch)
{
	rdfs_trace();
	return (ch >= '0') && (ch <= '9');
}
 int rdfs_parse_options(char *options, struct rdfs_sb_info *sbi,bool remount)
 {
     rdfs_trace();
     char *p, *rest;
     substring_t args[MAX_OPT_ARGS];
     int option;
 
     if (!options)
         return 0;
 
     while ((p = strsep(&options, ",")) != NULL) {
         int token;
         if (!*p)
             continue;
 
         token = match_token(p, tokens, args);
         switch (token) {
         case Opt_addr:
             if (remount)
                 goto bad_opt;
             /* physaddr managed in get_phys_addr() */
             break;
         case Opt_bpi:
             if (remount)
                 goto bad_opt;
             if (match_int(&args[0], &option))
                 goto bad_val;
             //sbi->bpi = option;
             break;
         case Opt_uid:
             if (remount)
                 goto bad_opt;
             if (match_int(&args[0], &option))
                 goto bad_val;
             sbi->uid = make_kuid(current_user_ns(), option);
             if (!uid_valid(sbi->uid))
                 goto bad_val;
             break;
         case Opt_gid:
             if (match_int(&args[0], &option))
                 goto bad_val;
             sbi->gid = make_kgid(current_user_ns(), option);
             if (!gid_valid(sbi->gid))
                 goto bad_val;
             break;
         case Opt_mode:
             if (match_octal(&args[0], &option))
                 goto bad_val;
             sbi->mode = option & 01777U;
             break;
         case Opt_size:
             if (remount)
                 goto bad_opt;
             /* memparse() will accept a K/M/G without a digit */
             if (!isdigit(*args[0].from))
                 goto bad_val;
             sbi->initsize = memparse(args[0].from, &rest);
             break;
         case Opt_num_inodes:
             if (remount)
                 goto bad_opt;
             if (match_int(&args[0], &option))
                 goto bad_val;
                 sbi->num_inodes = option;
                 break;
         case Opt_blocksize:
             if (remount)
                 goto bad_opt;
             /* memparse() will accept a K/M/G without a digit */
             if (!isdigit(*args[0].from))
                 goto bad_val;
             sbi->blocksize = memparse(args[0].from, &rest);
             if (sbi->blocksize < NVMM_MIN_BLOCK_SIZE ||
                 sbi->blocksize > NVMM_MAX_BLOCK_SIZE ||
                 !is_power_of_2(sbi->blocksize))
                 goto bad_val;
             break;
 default: {
             goto bad_opt;
         }
         }
     }
 
     return 0;
 
 bad_val:
     printk(KERN_ERR "Bad value '%s' for mount option '%s'\n", args[0].from,
            p);
     return -EINVAL;
 bad_opt:
     printk(KERN_ERR "Bad mount option: \"%s\"\n", p);
     return -EINVAL;
 }
void rdfs_get_init_size(struct rdfs_sb_info *sbi, void **data)
{
	rdfs_trace();
	char *options = (char *) *data;
	char *end;
	char org_end;
	char *rest;
	if (!options || strncmp(options, "init=", 5) != 0 )
		return ;
	options=options+5;
	end = strchr(options, ',') ?: options + strlen(options);
	org_end = *end;
	*end = '\0';
	sbi->initsize = memparse(options, &rest);
	*end = org_end;
	options = end;
	if (*options == ',')
		options++;
  	*data = (void *) options;
}
void init_address_to_ino(unsigned long address)
{
	rdfs_trace();
	memset((char*)address,'0',ADDRESS_TO_INO_SPACE);
}
void rdfs_set_blocksize(struct super_block *sb, unsigned long size)
{
	rdfs_trace();
	int bits;
	bits = fls(size) - 1;
	sb->s_blocksize_bits = bits;
	sb->s_blocksize = (1<<bits);
}
void set_default_opts(struct rdfs_sb_info *sbi)
{
	rdfs_trace();
	set_opt(sbi->s_mount_opt, ERRORS_CONT);
}
loff_t rdfs_max_size(int bits)
{
	rdfs_trace();
	loff_t res;
	res = (1UL << 35) - 1;

	if(res > MAX_LFS_FILESIZE)
		res = MAX_LFS_FILESIZE;

	rdfs_info("max file size %llu bytes\n",res);
	return res;
}



phys_addr_t get_phys_addr(void **data)
{
	rdfs_trace();
	phys_addr_t phys_addr;
	char *options = (char *) *data;
	unsigned long long ulltmp;
	char *end;
	char org_end;
	int err;

	if (!options || strncmp(options, "physaddr=", 9) != 0)
		return (phys_addr_t)ULLONG_MAX;
	options += 9;
	end = strchr(options, ',') ?: options + strlen(options);
	org_end = *end;
	*end = '\0';
	err = kstrtoull(options, 0, &ulltmp);
	*end = org_end;
	options = end;
	phys_addr = (phys_addr_t)ulltmp;
	if (err) {
		printk(KERN_ERR "Invalid phys addr specification: %s\n",
		       (char *) *data);
		return (phys_addr_t)ULLONG_MAX;
	}
	if (phys_addr & (PAGE_SIZE - 1)) {
		printk(KERN_ERR "physical address 0x%16llx for pramfs isn't "
			  "aligned to a page boundary\n",
			  (u64)phys_addr);
		return (phys_addr_t)ULLONG_MAX;
	}
	if (*options == ',')
		options++;
	*data = (void *) options;
	return phys_addr;
}

int calc_super_checksum(void)
{
	rdfs_trace();
    return 0;
}
int rdfs_iounmap(struct super_block *sb,unsigned long size)
{
	rdfs_trace();
	struct rdfs_sb_info *sbi;
	sbi = RDFS_SB(sb);
	rdfs_exit();
	iounmap(sbi->virt_addr);
	release_mem_region(sbi->phy_addr,size);
	return 0;
}
int rdfs_ioremap(struct super_block *sb, unsigned long size)
{
	rdfs_trace();
	struct rdfs_sb_info* sbi;
	void __iomem *retval;
	sbi = RDFS_SB(sb);
	printk("%s,phy:%lx,size:%lx\n",__FUNCTION__,(unsigned long)sbi->phy_addr,size);
	retval = (void __iomem *)
			request_mem_region(sbi->phy_addr, size, "rdfsfs");
	if (!retval)
	{	
		printk("request_mem_region_exclusive error\n");
		return 0;
	}

	sbi->virt_addr = ioremap_cache(sbi->phy_addr,size);
		
	if (sbi->virt_addr==NULL)
	{
		printk("%s -->ioremap error\n",__FUNCTION__);
		return 0;
	}
	rdfs_init();

	u64 dma_addr = rdfs_mapping_address(__va(sbi->phy_addr),size);
	printk("%s dma_addr:%lx phy_addr:%lx\n",__FUNCTION__,dma_addr,sbi->phy_addr);
	return 1;
}

extern struct kmem_cache * rdfs_inode_cachep;

void rdfs_i_callback(struct rcu_head *head)
{
	rdfs_trace();
	struct inode *inode = container_of(head,struct inode,i_rcu);
	struct rdfs_inode_info* ni_info = RDFS_I(inode);
	//rdfs_swap_buffer_free(ni_info);
	kmem_cache_free(rdfs_inode_cachep,ni_info);
}
void init_once(void *foo)
{
	rdfs_trace();
	struct rdfs_inode_info *vi = (struct rdfs_inode_info *) foo;
	mutex_init(&vi->i_meta_mutex);
	mutex_init(&vi->truncate_mutex);
	inode_init_once(&vi->vfs_inode);
}
int init_inodecache(void)
{
	rdfs_trace();
    rdfs_inode_cachep = kmem_cache_create("rdfs_inode_cache",sizeof(struct rdfs_inode_info),
                        0, (SLAB_RECLAIM_ACCOUNT |SLAB_MEM_SPREAD),init_once);
    if (rdfs_inode_cachep == NULL)
        return -ENOMEM;
    return 0;
}
void destory_inodecache(void)
{
	rdfs_trace();
    kmem_cache_destroy(rdfs_inode_cachep);
}
int rdfs_calc_checksum(u8 *data, int n)
{
	rdfs_trace();
	u32 crc = 0;
	crc = crc32(~0, (__u8 *)data + sizeof(__le32), n - sizeof(__le32));
	if (*((__le32 *)data) == cpu_to_le32(crc))
		return 0;
	else
		return 1;
}
