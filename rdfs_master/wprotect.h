/* @author page
 * @date 2017/12/3
 * rdfs/super.c
 */

#ifndef __WPROTECT_H
#define __WPROTECT_H

static inline void rdfs_sync_super(struct rdfs_super_block *ns)
{
	rdfs_trace();
	u32 crc = 0;
	ns->s_wtime = cpu_to_be32(get_seconds());
	ns->s_sum = 0;
	crc = crc32(~0, (__u8 *)ns + sizeof(__le32), RDFS_SB_SIZE - sizeof(__le32));
	ns->s_sum = cpu_to_le32(crc);
	memcpy((void *)ns + RDFS_SB_SIZE, (void *)ns, RDFS_SB_SIZE);
}

static inline void rdfs_sync_inode(struct rdfs_inode *pi)
{
	rdfs_trace();
	u32 crc = 0;
	pi->i_sum = 0;
	crc = crc32(~0, (__u8 *)pi + sizeof(__le32), RDFS_INODE_SIZE - sizeof(__le32));
	pi->i_sum = cpu_to_le32(crc);
}

#ifdef CONFIG_RDFS_WRITE_PROTECT

#else

#define rdfs_is_protected(sb)	0
#define rdfs_writeable(vaddr, size, rw) do {} while (0)

static inline void rdfs_memunlock_range(struct super_block *sb, void *p,unsigned long len) {}

static inline void rdfs_memlock_range(struct super_block *sb, void *p,unsigned long len) {}

static inline void rdfs_memunlock_super(struct super_block *sb,struct rdfs_super_block *ns) {}

static inline void rdfs_memlock_super(struct super_block *sb,struct rdfs_super_block *ns){}

static inline void rdfs_memunlock_inode(struct super_block *sb, struct rdfs_inode *pi) {}

static inline void rdfs_memlock_inode(struct super_block *sb,struct rdfs_inode *pi){}

static inline void rdfs_memunlock_block(struct super_block *sb,void *bp) {}

static inline void rdfs_memlock_block(struct super_block *sb,void *bp) {}

#endif 

#endif
