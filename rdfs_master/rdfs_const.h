/* @author page
 * @date 2017/12/3
 * rdfs/super.c
 */

#ifndef _LINUX_RDFS_FS_H
#define _LINUX_RDFS_FS_H

#include <linux/types.h>
#include <linux/magic.h>

#include "macro_cfg.h"

#define RDFS_STATE_NEW			0x00000001 /* inode is newly created */

#ifndef RDFS_START
#define RDFS_START       (0xffffcb0000000000)
#endif

#ifndef RDFS_END
#define RDFS_END         (0xffffe8ffffffffff)
#endif

#define DIR_AREA_SIZE    (1UL << 35) // 32G

/*
 * Mount flags
 */
#define RDFS_MOUNT_PROTECT		0x000001  /* Use memory protection */
#define RDFS_MOUNT_XATTR_USER		0x000002  /* Extended user attributes */
#define RDFS_MOUNT_POSIX_ACL		0x000004  /* POSIX ACL */
#define RDFS_MOUNT_XIP			0x000008  /* Execute in place */
#define RDFS_MOUNT_ERRORS_CONT		0x000010  /* Continue on errors */
#define RDFS_MOUNT_ERRORS_RO		0x000020  /* Remount fs ro on errors */
#define RDFS_MOUNT_ERRORS_PANIC		0x000040  /* Panic on errors */

#define MAX_DIR_SIZE        (1UL << 21) // 2M
#define MAX_FILE_NUM	    (1024)
#define MAX_FILE_SIZE       (1UL << 35) // 32G




/* Can be used to packed structure */
#define __ATTRIBUTE_PACKED_     __attribute__ ((packed))

#define RDFS_DATE     "2017/12/3"
#define RDFS_VERSION  "0.1"

#define RDFS_FILE_PATH_LENGTH   1024

#define RDFS_SB_SIZE    		(PAGE_SIZE>>1)
//cj
#define RDFS_SB_SPACE 			(PAGE_SIZE)
#define ADDRESS_TO_INO_SPACE 	(MAX_FILE_NUM*sizeof(unsigned long))
#define BDEV_DES_OFFSET				(RDFS_SB_SPACE+ADDRESS_TO_INO_SPACE)

/* Special inode number */
#define RDFS_ROOT_INO   (1)

#define RDFS_MIN_BLOCK_SIZE       (PAGE_SIZE)
#define RDFS_MAX_BLOCK_SIZE       (2 * (PAGE_SIZE))

/*
 * Inode flags (GETFLAGS/SETFLAGS)
 */
#define	RDFS_SECRM_FL			FS_SECRM_FL	/* Secure deletion */
#define	RDFS_UNRM_FL			FS_UNRM_FL	/* Undelete */
#define	RDFS_COMPR_FL			FS_COMPR_FL	/* Compress file */
#define RDFS_SYNC_FL			FS_SYNC_FL	/* Synchronous updates */
#define RDFS_IMMUTABLE_FL		FS_IMMUTABLE_FL	/* Immutable file */
#define RDFS_APPEND_FL			FS_APPEND_FL	/* writes to file may only append */
#define RDFS_NODUMP_FL			FS_NODUMP_FL	/* do not dump file */
#define RDFS_NOATIME_FL			FS_NOATIME_FL	/* do not update atime */
#define RDFS_DIRSYNC_FL			FS_DIRSYNC_FL	/* dirsync behaviour (directories only) */
#define RDFS_BTREE_FL			FS_BTREE_FL		/* btree format dir */

/*
 * rdfs inode flags
 *
 * RDFS_EOFBLOCKS_FL	There are blocks allocated beyond eof
 */
#define RDFS_EOFBLOCKS_FL	0x20000000

/* Flags that should be inherited by new inodes from their parent. */
#define RDFS_FL_INHERITED (FS_SECRM_FL | FS_UNRM_FL | FS_COMPR_FL |\
			   FS_SYNC_FL | FS_NODUMP_FL | FS_NOATIME_FL | \
			   FS_COMPRBLK_FL | FS_NOCOMP_FL | FS_JOURNAL_DATA_FL |\
			   FS_NOTAIL_FL | FS_DIRSYNC_FL)

/* Flags that are appropriate for regular files (all but dir-specific ones). */
#define RDFS_REG_FLMASK (~(FS_DIRSYNC_FL | FS_TOPDIR_FL))
/* Flags that are appropriate for non-directories/regular files. */
#define RDFS_OTHER_FLMASK (FS_NODUMP_FL | FS_NOATIME_FL)

/* Inode size in bytes */
#define RDFS_INODE_BITS     (7)
#define RDFS_INODE_SIZE     (1<<RDFS_INODE_BITS)

#define RDFS_BLOCK_SIZE (PAGE_SIZE)

#define INODE_NUM_PER_BLOCK (32) /* 4096/128 */

/* DIR_REC_LEN */
#define	RDFS_DIR_PAD	4
#define	RDFS_DIR_ROUND	(RDFS_DIR_PAD - 1)
#define	svn RDFS_DIR_REC_LEN(name_len)	(((name_len) + 8 + RDFS_DIR_ROUND) & \
					~RDFS_DIR_ROUND)
#define	RDFS_MAX_REC_LEN	((1 << 16) - 1)

/* error code */
#define   NOALIGN       (0x10001)  /* Start data block physical addr not page align */
#define   BADINO        (0x10002)  /* Invalid inode number */
#define   PGFAILD       (0x10003)  /* Init file page table faild*/


/*
 * Maximal count of links to a file
 */
#define RDFS_LINK_MAX 32000

/*
 * Structure of a directory entry
 */
#define RDFS_NAME_LEN   255

struct rdfs_dir_entry {
    __le64  inode;              /* Inode number */
    __le16  rec_len;            /* Directory entry length */
    __u8    name_len;           /* Name length */
    __u8    file_type;
    char    name[RDFS_NAME_LEN];/* File name */
};


/*
 * rdfs directory file types.
 */
enum {
    RDFS_FT_UNKNOWN     = 0,
    RDFS_FT_REG_FILE    = 1,
    RDFS_FT_DIR         = 2,
    RDFS_FT_CHRDEV      = 3,
    RDFS_FT_BLKDEV      = 4,
    RDFS_FT_FIFO        = 5,
    RDFS_FT_SOCK        = 6,
    RDFS_FT_SYMLINK     = 7,
    RDFS_FT_MAX
};


/*
 * rdfs_DIR_PAD defines the directory entries boundaries
 *
 * NOTE: It must be a multiple of 4
 */
#define RDFS_DIR_PAD		 	4
#define RDFS_DIR_ROUND 			(RDFS_DIR_PAD - 1)
#define RDFS_DIR_REC_LEN(name_len)	(((name_len) + 12 + RDFS_DIR_ROUND) & \
					 ~RDFS_DIR_ROUND)

/*
 * XATTR related
 */

/* Magic value in attribute blocks */
#define PRAM_XATTR_MAGIC		0xc910629e

/* Maximum number of references to one attribute block */
#define RDFS_XATTR_REFCOUNT_MAX		1024

/* Name indexes */
#define RDFS_XATTR_INDEX_USER			1
#define RDFS_XATTR_INDEX_POSIX_ACL_ACCESS	2
#define RDFS_XATTR_INDEX_POSIX_ACL_DEFAULT	3
#define RDFS_XATTR_INDEX_TRUSTED		4
#define RDFS_XATTR_INDEX_SECURITY	        5

struct rdfs_xattr_header {
	__be32	h_magic;	/* magic number for identification */
	__be32	h_refcount;	/* reference count */
	__be32	h_hash;		/* hash value of all attributes */
	__u32	h_reserved[4];	/* zero right now */
};

struct rdfs_xattr_entry {
	__u8	e_name_len;	/* length of name */
	__u8	e_name_index;	/* attribute name index */
	__be16	e_value_offs;	/* offset in disk block of value */
	__be32	e_value_block;	/* disk block attribute is stored on (n/i) */
	__be32	e_value_size;	/* size of attribute value */
	__be32	e_hash;		/* hash value of name and value */
	char	e_name[0];	/* attribute name */
};

#define RDFS_XATTR_PAD_BITS		2
#define RDFS_XATTR_PAD		(1<<rdfs_XATTR_PAD_BITS)
#define RDFS_XATTR_ROUND		(rdfs_XATTR_PAD-1)
#define RDFS_XATTR_LEN(name_len) \
	(((name_len) + rdfs_XATTR_ROUND + \
	sizeof(struct rdfs_xattr_entry)) & ~rdfs_XATTR_ROUND)
#define RDFS_XATTR_NEXT(entry) \
	((struct rdfs_xattr_entry *)( \
	  (char *)(entry) + rdfs_XATTR_LEN((entry)->e_name_len)))
#define RDFS_XATTR_SIZE(size) \
	(((size) + rdfs_XATTR_ROUND) & ~rdfs_XATTR_ROUND)

#endif /* _LINUX_rdfs_FS_H */
