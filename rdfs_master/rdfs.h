/*
 * linux/fs/rdfs/rdfs.h
 * 
 * Copyright (C) 2013 College of Computer Science,
 * Chonqing University
 *
 */

#ifndef _RDFS_RDFS_H
#define _RDFS_RDFS_H

#include <linux/buffer_head.h>
#include <linux/types.h>
#include <linux/crc32.h>
#include <linux/mutex.h>
#include <linux/spinlock_types.h>
#include <linux/rwlock.h>

#include "rdfs_const.h"
struct rdfs_inode
{
    __le32  i_sum;          /* Checksum of this inode */
    __le32  i_mode;         /* File mode */
    __le32  i_link_counts;  /* Links count */
    __le32  i_bytes;        /* The bytes in the last block */
    __le32  i_flags;        /* File flags */
    __le32  i_file_acl;     /* File access control */
    __le32  i_dir_acl;      /* Directory access control */
    __le32  i_atime;        /* Access time */
    __le32  i_ctime;        /* Creation time */
    __le32  i_mtime;        /* Modification time */
    __le32  i_dtime;        /* Deletion time */
    __le32  i_uid;          /* Ower uid */
    __le32  i_gid;          /* Group id */
    __le32  i_generation;   /* File version (for NFS) */
    __le64  i_blocks;       /* Block count */
    __le64  i_size;         /* File size in bytes */
    __le64  i_pg_addr;      /* File page table */

    __le64  i_accessed_count;
    __le64  list_pro;
    __le64  list_next;
	__u8    i_hot_page_num;
	__u8    *i_cold_dram;
	__le32  *i_hot_dram;
	__le32  i_cold_page_start;
	__le32  i_cold_page_num;
	//unsigned long  block_num;
	//struct index_struct *index;
	//struct mem_node *blocks;
};
struct swap_buffer
{
	pte_t** pte;
	u64* sn_num;
	unsigned long* accessed_count;
	unsigned long file_pages;
	spinlock_t lock;
};
struct rdfs_inode_info
{
	__u16	i_state;
	__u32	i_file_acl;
	__u32	i_flags;
	__u32	i_dir_acl;
	__u32	i_dtime;
	__le64  i_pg_addr;      /* File page table */
	atomic_t i_p_counter;	/* process num counter */
	void	*i_virt_addr;	/* inode's virtual address */

	int w_flag;
	unsigned long post;
	unsigned long access_length;
	int mapping_count;
	int per_read;
//	unsigned long  block_num;
//	struct index_struct *index;
//	struct mem_node *blocks;
	long file_size;

	struct pre_alloc_bitmap *pre_bitmap;
	spin_lock_t used_bitmap_list_lock;
	struct mem_bitmap_block * used_bitmap_list;

	struct rdfs_inode* rdfs_inode;

	struct swap_buffer swap_buffer;

	struct mutex truncate_mutex;
	struct mutex i_meta_mutex;
	rwlock_t state_rwlock;

	struct inode vfs_inode;
};
struct numa_des
{
	__le64 free_inode_count;
	__le64 free_page_count;
	__le64 free_inode_phy;
	__le64 free_page_phy;

	//spinlock should be init when HMFS is mounted
	spinlock_t inode_lock;
	spinlock_t free_list_lock;
	rwlock_t   lru_rwlock;

	__le64 padding[10];	//keep the size 128B
};



//Structure of super block in NVM
struct rdfs_super_block
{
	__u8    s_volume_name[16];  /* Volume name */
	__u8    s_fs_version[16];   /* File system version */
	__u8    s_uuid[16];         /* File system universally unique identifier */

	__u8 	s_numa_flag;		/* whether use two node*/
	__u8	s_bdev_count;

	__le16  s_magic;            /* Magic number */
	__le32  s_mtime;            /* Mount time */
	__le32  s_wtime;            /* Modification time */
	__le32  s_sum;              /* Checksum of this superblock */
	__le32  s_blocksize;        /* Data block size in bytes, in beta version, it's PAGESIZE */
	__le32  s_inode_size;       /* Inode size in bytes */
	__le64  s_physaddr;			/* File system start physical address */
	__le64  s_inode_count;		/* Total number of inode */
	__le64 	s_block_count;		/* Total number of page */
	__le64  s_size;             /* The whole file system's size */
	__le64  s_numa_des[2];		/* The pointer of rdfs-space metadata */
	__le64	p_address_to_ino;	/* The pointer of mapping from virtual address to vfs inode */
};

struct rdfs_sb_info 
{
	void*		  virt_addr;   //!< the filesystem's mount addr
	phys_addr_t    phy_addr;     //!< the filesystem's phy addr             
	unsigned long num_inodes;   //!< total inodes of the fs(%5 of the whole size of nvm)
	unsigned long blocksize;
	int s_inode_size;
	unsigned long bpi;
	unsigned long initsize;     //!< initial size of the fs
	unsigned long s_mount_opt;  //!< @TODO:can be what?
	kuid_t uid;                 //!< mount uid for root directory
	kgid_t gid;                 //!< mount gid for root directory
	umode_t mode;               //!< mount mode for root directory
	atomic_t next_generation;

	int 	  bdev_count;
	atomic_t atomic_bdev_post;
};

//ssd space management
struct block_pool_per_cpu
{
	spinlock_t spinlock;
	int post;
	int capacity;
	sector_t* address;							
};


struct bdev_des
{
	struct block_device* bdev;
	spinlock_t spinlock;

	sector_t start;
	sector_t end;
	struct block_pool_per_cpu block_pool[CPU_NR+1];

	unsigned long pgtable;// physaddr of pmd page
	unsigned long free_pmd_page;
};

struct __pre_free_struct
{
	phys_addr_t free_start;
	phys_addr_t free_end;
	unsigned long free_count;
};

struct pre_free_struct
{
	struct __pre_free_struct pre_free_struct[2];
};

extern struct super_block * RDFS_SUPER_BLOCK_ADDRESS;


int rdfs_write_inode(struct inode *inode, struct writeback_control *wbc);
ssize_t rdfs_direct_IO(struct kiocb *iocb,struct iov_iter *iter,loff_t offset);
int rdfs_fill_super(struct super_block *sb, void *data, int silent);

static inline struct rdfs_inode_info *RDFS_I(struct inode *inode)
{
	return container_of(inode, struct rdfs_inode_info, vfs_inode);
}

static inline struct rdfs_sb_info * RDFS_SB(struct super_block * sb)
{
    return sb->s_fs_info;
}

static inline void* rdfs_va(phys_addr_t phys)
{
    struct rdfs_sb_info *sbi = RDFS_SB(RDFS_SUPER_BLOCK_ADDRESS);
    return (void*)(phys-sbi->phy_addr+(unsigned long)sbi->virt_addr);
}

static inline phys_addr_t rdfs_pa(void* virt_addr)
{
    struct rdfs_sb_info *sbi = RDFS_SB(RDFS_SUPER_BLOCK_ADDRESS);
    return ((unsigned long)virt_addr-(unsigned long)sbi->virt_addr+sbi->phy_addr);    
}

static inline struct rdfs_super_block * rdfs_get_super(struct super_block * sb)
{
    struct rdfs_sb_info *sbi = RDFS_SB(sb);
    return (struct rdfs_super_block*)sbi->virt_addr;
}
static inline struct rdfs_super_block *rdfs_get_redund_super(struct super_block *sb)
{
	struct rdfs_sb_info *sbi = RDFS_SB(sb);
	return (struct rdfs_super_block *)(sbi->virt_addr + RDFS_SB_SIZE);
}

static inline int rdfs_get_cpu_numa_id(void)
{
	return 0;
}

static inline struct numa_des* rdfs_get_numa_by_id(int id)
{
	struct rdfs_super_block* nsb = rdfs_get_super(RDFS_SUPER_BLOCK_ADDRESS);
	return (struct numa_des*)nsb->s_numa_des[id];
}

static inline int rdfs_get_phys_numa_id(phys_addr_t phys)
{
	struct rdfs_super_block* nsb = rdfs_get_super((struct super_block*)RDFS_SUPER_BLOCK_ADDRESS);
	if (nsb->s_numa_flag && phys > rdfs_NUMA_LINE)
	{
		return 1;
	}	
	else
	{
		return 0;
	}
}

static inline int rdfs_get_inode_numa_id(struct rdfs_inode* ni)
{
	return rdfs_get_phys_numa_id(rdfs_pa(ni));	
}

static inline struct numa_des* rdfs_get_numa_by_inode(struct rdfs_inode* ni)
{
	return rdfs_get_numa_by_id(rdfs_get_inode_numa_id(ni));
}

static inline sector_t get_sn_from_pte(pte_t* pte)
{
	return ((pte_val(*pte))&0x0FFFFFFFFFFF000)>>12;
}

static inline sector_t get_sn_from_pte_v(unsigned long pte_v)
{
	return ((pte_v)&0x0FFFFFFFFFFF000)>>12;
}

static inline int get_bdev_id_from_pte(pte_t* pte)
{
	return ((pte_val(*pte))&0x7000000000000000)>>60;
}

static inline int get_bdev_id_from_pte_v(unsigned long pte_v)
{
	return (pte_v&0x7000000000000000)>>60;
}

static inline struct bdev_des* get_bdev_des_by_id(int bdev_id)
{
	struct rdfs_sb_info *sbi = RDFS_SB(RDFS_SUPER_BLOCK_ADDRESS);
	return (struct bdev_des*)(sbi->virt_addr + BDEV_DES_OFFSET +bdev_id*sizeof(struct bdev_des));
}

static inline bool pte_is_sn(pte_t* pte)
{
	return (pte_val(*pte)&(~PAGE_MASK))==0;
}

static inline struct inode** rdfs_address_to_ino(struct super_block* sb,unsigned long address)
{
    int index=(address-RDFS_START-DIR_AREA_SIZE)/MAX_FILE_SIZE;
    return (struct inode**)(rdfs_get_super(sb))->p_address_to_ino+index*sizeof(struct inode*);
}

/* Mask out flags that are inappropriate for the given type of inode. */
static inline __le32 rdfs_mask_flags(umode_t mode, __le32 flags)
{
	flags &= cpu_to_le32(RDFS_FL_INHERITED);
	if (S_ISDIR(mode))
		return flags;
	else if (S_ISREG(mode))
		return flags & cpu_to_le32(RDFS_REG_FLMASK);
	else
		return flags & cpu_to_le32(RDFS_OTHER_FLMASK);
}


static inline struct rdfs_inode * get_rdfs_inode(struct super_block * sb, u64 ino)
{
    struct rdfs_super_block * nsb = rdfs_get_super(sb); 
    struct rdfs_inode* ni;
    if (nsb->s_numa_flag)
    {
	if (ino&0x1)
	{
		ni = (struct rdfs_inode*)((unsigned long)nsb->s_numa_des[0] + sizeof(struct numa_des) + ((ino -1)>>1 << RDFS_INODE_BITS));
	}
	else
	{
		ni = (struct rdfs_inode*)((unsigned long)nsb->s_numa_des[1] + sizeof(struct numa_des) + ((ino-2)>>1 << RDFS_INODE_BITS));
	}
    }
    else
    {
	ni = (struct rdfs_inode*)((unsigned long)nsb->s_numa_des[0] + sizeof(struct numa_des) + ((ino - 1)<<RDFS_INODE_BITS));
    }
    return ni ? ni : NULL;
}

static inline unsigned long get_rdfs_inode_phy_addr(struct super_block * sb, u64 ino)
{
    return rdfs_pa(get_rdfs_inode(sb,ino));
}

static inline unsigned long get_rdfs_inodenr(struct super_block *sb, unsigned long inode_phy)
{
	struct rdfs_super_block * nsb = rdfs_get_super(sb); 
	if (nsb->s_numa_flag)
	{
		if (inode_phy>rdfs_NUMA_LINE)
			return ((inode_phy-rdfs_pa((struct numa_des*)nsb->s_numa_des[1])-sizeof(struct numa_des))>>RDFS_INODE_BITS<<1) + 2;
		else
			return ((inode_phy-rdfs_pa((struct numa_des*)nsb->s_numa_des[0])-sizeof(struct numa_des))>>RDFS_INODE_BITS<<1) + 1;
	}
	else
		return ((inode_phy-rdfs_pa((struct numa_des*)nsb->s_numa_des[0])-sizeof(struct numa_des))>>RDFS_INODE_BITS) + 1;
}

static inline void check_eof_blocks(struct inode *inode, loff_t size)
{
	struct rdfs_inode *ni = get_rdfs_inode(inode->i_sb, inode->i_ino);

	if(unlikely(!ni))
		return;
	if((ni->i_flags & cpu_to_le32(RDFS_EOFBLOCKS_FL)) &&
			size + inode->i_sb->s_blocksize >= 
			(inode->i_blocks << inode->i_sb->s_blocksize_bits))
		ni->i_flags &= cpu_to_le32(~RDFS_EOFBLOCKS_FL);
}

static inline phys_addr_t get_phys_from_pte_val(unsigned long pte)
{
	return (pte&0x7ffffffffffff000);
}

static inline phys_addr_t rdfs_get_phy_from_pte(pte_t* pte)
{
	return (pte_val(*pte) & 0x7ffffffffffff000);
}

static inline phys_addr_t rdfs_get_phy_from_pmd(pmd_t* pmd)
{
	return (pmd_val(*pmd) & 0x7ffffffffffff000);
}

static inline phys_addr_t rdfs_get_phy_from_pud(pud_t* pud)
{
	return (pud_val(*pud) & 0x7ffffffffffff000);
}

static inline void rdfs_set_pte(pte_t *pte, phys_addr_t phys)
{
	set_pte(pte, __pte(phys| __PAGE_KERNEL));
}

static inline void rdfs_set_pmd(pmd_t *pmd, pte_t *pte)
{
	set_pmd(pmd, __pmd(rdfs_pa(pte) | _PAGE_TABLE));
}

static inline void rdfs_set_pud(pud_t *pud, pmd_t *pmd)
{
	set_pud(pud, __pud(rdfs_pa(pmd) | _PAGE_TABLE));
}

static inline pud_t* rdfs_get_pud(struct super_block* sb, u64 ino)
{
    struct rdfs_inode *ni = get_rdfs_inode(sb, ino);
    if(ni->i_pg_addr==0)
		return NULL;
    else    
		return (pud_t*)rdfs_va(ni->i_pg_addr);
}

static inline pmd_t* rdfs_get_pmd(pud_t *pud)
{
	return (pmd_t*)rdfs_va(pud_val(*pud) & PAGE_MASK);
}

static inline pte_t* rdfs_get_pte(pmd_t *pmd)
{
    return (pte_t*)rdfs_va(pmd_val(*pmd) & PAGE_MASK);
}

static inline pmd_t* rdfs_pmd_offset(pud_t* pud,unsigned long address)
{
   return (pmd_t*) rdfs_va(rdfs_get_phy_from_pud(pud))+ pmd_index(address);
}

static inline pte_t* rdfs_pte_offset(pmd_t* pmd,unsigned long address)
{
   return (pte_t*) rdfs_va(rdfs_get_phy_from_pmd(pmd))+ pte_index(address);
}

static inline void rdfs_pud_populate(pud_t* pud,pmd_t* pmd)
{
    rdfs_set_pud(pud,pmd);
}

#define	RDFS_IOC_GETFLAGS		FS_IOC_GETFLAGS
#define	RDFS_IOC_SETFLAGS		FS_IOC_SETFLAGS
#define	RDFS_IOC_GETVERSION		FS_IOC_GETVERSION
#define	RDFS_IOC_SETVERSION		FS_IOC_SETVERSION

#define RDFS_IOC32_GETFLAGS		FS_IOC32_GETFLAGS
#define RDFS_IOC32_SETFLAGS		FS_IOC32_SETFLAGS
#define RDFS_IOC32_GETVERSION		FS_IOC32_GETVERSION
#define RDFS_IOC32_SETVERSION		FS_IOC32_SETVERSION

/*
 * Inode flags (GETFLAGS/SETFLAGS)
 */
#define RDFS_FL_USER_VISIBLE		FS_FL_USER_VISIBLE	/* User visible flags */
#define RDFS_FL_USER_MODIFIABLE		FS_FL_USER_MODIFIABLE	/* User modifiable flags */

#define MAX_BLOCK_NUM 100


#endif
