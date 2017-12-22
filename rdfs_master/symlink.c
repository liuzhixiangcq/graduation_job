/* @author page
 * @date 2017/12/3
 * rdfs/super.c
 */
#include <linux/fs.h>
#include <linux/namei.h>
#include "rdfs.h"
#include "xattr.h"
#include "inode.h"
#include "pgtable.h"
#include "memory.h"
int rdfs_page_symlink(struct inode *inode, const char *symname, int len)
{
	//rdfs_trace();
    struct rdfs_inode_info *ni_info;
    char *vaddr;
    int err;
    err = rdfs_alloc_blocks(inode, 1, 0,ALLOC_PAGE);
    if(err)
        return err;
    ni_info = RDFS_I(inode);
    rdfs_establish_mapping(inode);

    vaddr = (char *)(ni_info->i_virt_addr);
    memcpy(vaddr, symname, len);
    vaddr[len] = '\0';

    rdfs_destroy_mapping(inode);
    return 0;
}

static int rdfs_readlink(struct dentry *dentry, char __user *buffer , int buflen)
{
	//rdfs_trace();
    struct inode *inode = dentry->d_inode;
    struct rdfs_inode_info *ni;
    char *vaddr;
    int err = 0;

    rdfs_establish_mapping(inode);
        
    ni = RDFS_I(inode);
    vaddr = (char *)(ni->i_virt_addr);
     err = readlink_copy(buffer,buflen,vaddr);

    rdfs_destroy_mapping(inode);
    return err;    
}

struct inode_operations rdfs_symlink_inode_operations = 
{
	.readlink	= rdfs_readlink,
	//.follow_link	= rdfs_follow_link,
    .put_link = page_put_link,
	.setattr	= rdfs_notify_change,
#ifdef CONFIG_rdfsFS_XATTR
	.setxattr	= generic_setxattr,
	.getxattr	= generic_getxattr,
	.listxattr	= rdfs_listxattr,
	.removexattr	= generic_removexattr,
#endif
};

