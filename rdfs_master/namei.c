/* @author page
 * @date 2017/12/3
 * rdfs/namei.c
 */

#include <linux/pagemap.h>
#include <linux/quotaops.h>
#include "rdfs.h"
#include "acl.h"
#include "xattr.h"
#include "xip.h"
#include "dir.h"
#include "inode.h"
#include "pgtable.h"
#include "symlink.h"
#include "debug.h"

extern struct inode_operations rdfs_file_inode_operations;
extern struct address_space_operations rdfs_aops_xip;
extern struct file_operations rdfs_xip_file_operations;
extern struct address_space_operations rdfs_aops;
extern struct file_operations rdfs_file_operations;
extern struct inode_operations rdfs_symlink_inode_operations ;
extern struct file_operations rdfs_dir_operations;
//extern struct inode_operations rdfs_dir_inode_operations;


static inline int rdfs_add_nondir(struct dentry *dentry, struct inode *inode)
{
	rdfs_trace();
	int err = 0;

	err = rdfs_add_link(dentry, inode);
	if(!err){
		unlock_new_inode(inode);
		d_instantiate(dentry, inode);
		return 0;
	}
	inode_dec_link_count(inode);
	unlock_new_inode(inode);
	iput(inode);
	return err;
}

static struct dentry *rdfs_lookup(struct inode *dir,struct dentry *dentry,unsigned int flags)
{
	rdfs_trace();
    struct inode *inode;
    ino_t ino;

    if(unlikely(dentry->d_name.len > RDFS_NAME_LEN))
        return ERR_PTR(-ENAMETOOLONG);

    ino = rdfs_inode_by_name(dir,&dentry->d_name);
    inode = NULL;
    if(likely(ino)){
        inode = rdfs_iget(dir->i_sb,ino);
        if(unlikely(inode == ERR_PTR(-ESTALE))){
            rdfs_error(dir->i_sb,__func__,
                       "deleted inode referenced:%u",
                       (unsigned long)ino);
            return ERR_PTR(-EIO);
        }
    }
    return d_splice_alias(inode,dentry);
}

static int rdfs_create(struct inode *dir, struct dentry *dentry, umode_t mode, bool excl)
{
	rdfs_trace();
	struct inode *inode;
	int err = 0;

	dquot_initialize(dir);

	inode = rdfs_new_inode(dir, mode, &dentry->d_name);

	if(IS_ERR(inode))
		return PTR_ERR(inode);

	inode->i_op = &rdfs_file_inode_operations;
	if(rdfs_use_xip(inode->i_sb)){
		inode->i_mapping->a_ops =  &rdfs_aops_xip;
		inode->i_fop = &rdfs_xip_file_operations;
	}else{
		inode->i_mapping->a_ops = &rdfs_aops;
		inode->i_fop = &rdfs_file_operations;
	}
	mark_inode_dirty(inode);

	err = rdfs_establish_mapping(dir);
	err = rdfs_add_nondir(dentry, inode);
	err = rdfs_destroy_mapping(dir);
	return err;
}

static int rdfs_tmpfile(struct inode *dir, struct dentry *dentry, umode_t mode)
{ 
	rdfs_trace();
	struct inode *inode = rdfs_new_inode(dir, mode, NULL);
	if(IS_ERR(inode))
		return PTR_ERR(inode);

	inode->i_op = &rdfs_file_inode_operations;
	if(rdfs_use_xip(inode->i_sb)){
		inode->i_mapping->a_ops = &rdfs_aops_xip;
		inode->i_fop = &rdfs_xip_file_operations;
	}else{
		inode->i_mapping->a_ops = &rdfs_aops;
		inode->i_fop = &rdfs_file_operations;
	}
	d_tmpfile(dentry, inode);
	unlock_new_inode(inode);
	return 0;
}

static int rdfs_mknod(struct inode *dir, struct dentry *dentry, umode_t mode, dev_t rdev)
{
	rdfs_trace();
	struct inode *inode;
	int err;
	if(!new_valid_dev(rdev))
		return -EINVAL;

	dquot_initialize(dir);

	inode = rdfs_new_inode(dir, mode, &dentry->d_name);
	err = PTR_ERR(inode);
	if(!IS_ERR(inode)){
		mark_inode_dirty(inode);
		err = rdfs_add_nondir(dentry, inode);	
	}
	return err;

}

static int rdfs_symlink(struct inode *dir, struct dentry *dentry,const char *symname)
{
	rdfs_trace();
    struct super_block *sb = dir->i_sb;
    int err = -ENAMETOOLONG;
    unsigned l = strlen(symname);
    struct inode *inode;

    if(unlikely(l+1 > sb->s_blocksize))
        goto out;

    dquot_initialize(dir);

    inode = rdfs_new_inode(dir, S_IFLNK | S_IRWXUGO , &dentry->d_name);
    err = PTR_ERR(inode);
    if(unlikely(IS_ERR(inode)))
        goto out;

    inode->i_op = &rdfs_symlink_inode_operations;
    inode->i_mapping->a_ops = &rdfs_aops;

    err = rdfs_page_symlink(inode, symname, l);
    if(unlikely(err))
        goto out_fail;

    inode->i_size = l;
    rdfs_write_inode(inode, 0);

    rdfs_establish_mapping(dir);
    
    err = rdfs_add_nondir(dentry, inode);

    rdfs_destroy_mapping(dir);
    
 out:
    return err;

 out_fail:
    inode_dec_link_count(inode);
    unlock_new_inode(inode);
    iput(inode);
    goto out;
}

static int rdfs_link(struct dentry * old_entry, struct inode *dir,struct dentry *dentry)
{
	rdfs_trace();
    struct inode *inode = old_entry->d_inode;
    int err;

    dquot_initialize(dir);
    inode->i_ctime = CURRENT_TIME_SEC;
    inode_inc_link_count(inode);
    ihold(inode);

    rdfs_establish_mapping(dir);
    
    err = rdfs_add_link(dentry, inode);
    if(!err){
        d_instantiate(dentry, inode);
        rdfs_destroy_mapping(dir);
        return 0;
    }
    inode_dec_link_count(inode);
    iput(inode);
    rdfs_destroy_mapping(dir);
    return err;
}

static int rdfs_unlink(struct inode *dir, struct dentry *dentry)
{
	rdfs_trace();
    struct inode *inode = dentry->d_inode;
    struct rdfs_dir_entry *de;
    struct rdfs_dir_entry *pde;
	int err = -ENOENT;

    dquot_initialize(dir);
    
    rdfs_establish_mapping(dir);
    de = rdfs_find_entry2(dir,&dentry->d_name,&pde);
    if(unlikely(!de))
        goto out;

    err = rdfs_delete_entry(de,&pde,dir);
    if(unlikely(err))
        goto out;

    inode->i_ctime = dir->i_ctime;
    inode_dec_link_count(inode);
    err = 0;
 out:
    rdfs_destroy_mapping(dir);
	return err;
}

static int rdfs_mkdir(struct inode *dir,struct dentry *dentry,umode_t mode);
static int rdfs_rmdir(struct inode *dir,struct dentry *dentry);
static int rdfs_rename(struct inode *old_dir, struct dentry *old_dentry,struct inode *new_dir, struct dentry *new_dentry);

const struct inode_operations rdfs_dir_inode_operations = 
{
	.create		= rdfs_create,
	.lookup		= rdfs_lookup,
	.link		= rdfs_link,
	.unlink		= rdfs_unlink,
	.symlink	= rdfs_symlink,
	.mkdir		= rdfs_mkdir,
	.rmdir		= rdfs_rmdir,
	.mknod		= rdfs_mknod,
    .rename		= rdfs_rename,
#ifdef CONFIG_rdfsFS_XATTR
#endif
	.setattr	= rdfs_notify_change,
	.get_acl	= rdfs_get_acl,
	.tmpfile	= rdfs_tmpfile,
};
static int rdfs_mkdir(struct inode *dir,struct dentry *dentry,umode_t mode)
{
	rdfs_trace();
    struct inode *inode;
    int err;
    
    dquot_initialize(dir);
    
    inode_inc_link_count(dir);

    inode = rdfs_new_inode(dir,S_IFDIR | mode,&dentry->d_name);
    err = PTR_ERR(inode);
    if(unlikely(IS_ERR(inode)))
        goto out_dir;

    inode->i_op = &rdfs_dir_inode_operations;
    inode->i_fop = &rdfs_dir_operations;
    inode->i_mapping->a_ops = &rdfs_aops;

    inode_inc_link_count(inode);

    rdfs_establish_mapping(dir);

    err = rdfs_make_empty(inode,dir);//set the first fragment of directory
    if(unlikely(err))
        goto out_fail;
    err = rdfs_add_link(dentry,inode);
    if(unlikely(err))
        goto out_fail;

    unlock_new_inode(inode);
    d_instantiate(dentry,inode);//indicate that it is now in use by the dcache
 out:

    rdfs_destroy_mapping(dir);
    return err;

 out_fail:
    inode_dec_link_count(inode);
    inode_dec_link_count(inode);
    unlock_new_inode(inode);
    iput(inode);
 out_dir:
    inode_dec_link_count(dir);
    goto out;
}

static int rdfs_rmdir(struct inode *dir,struct dentry *dentry)
{
	rdfs_trace();
    struct inode *inode = dentry->d_inode;
    int err = -ENOTEMPTY;

    rdfs_establish_mapping(inode);
    
    if(rdfs_empty_dir(inode)){
        err = rdfs_unlink(dir,dentry);  //remove the dentry from dir
        if(!err){
            inode->i_size = 0;
            inode_dec_link_count(inode);
            inode_dec_link_count(dir);
        }
    }
    
    rdfs_destroy_mapping(inode);
    return err;
}

static int rdfs_rename(struct inode *old_dir, struct dentry *old_dentry,struct inode *new_dir, struct dentry *new_dentry)
{
	rdfs_trace();
    struct inode *old_inode = old_dentry->d_inode;
    struct inode *new_inode = new_dentry->d_inode;
    struct rdfs_dir_entry *dir_de = NULL;
    struct rdfs_dir_entry *old_de;
    struct rdfs_dir_entry *old_pde = NULL;
	struct rdfs_dir_entry *new_pde = NULL;
    int err = -ENOENT;
	int exist = 0;
	int isdir = 0;

    rdfs_establish_mapping(old_dir);
    old_de = rdfs_find_entry2(old_dir, &old_dentry->d_name, &old_pde);
    if(!old_de)
        goto out_old_dir;

    if(S_ISDIR(old_inode->i_mode)){
		isdir = 1;
        err = -EIO;
        rdfs_establish_mapping(old_inode);
        dir_de = rdfs_dotdot(old_inode);
        if(!dir_de)
            goto out_old_inode;
    }

    if(new_inode){
        struct rdfs_dir_entry *new_de;
        exist = 1;
		
        err = -ENOTEMPTY;
        rdfs_establish_mapping(new_inode);
        if(dir_de && !rdfs_empty_dir(new_inode))// old_inode is dir & new_inode is not empty
            goto out_new_inode;

        err = -ENOENT;
		rdfs_establish_mapping(new_dir);
        new_de = rdfs_find_entry2(new_dir, &new_dentry->d_name, &new_pde);
        if(!new_de)
            goto out_new_dir;
        rdfs_set_link(new_dir, new_de, old_inode, 1);
        new_inode->i_ctime = CURRENT_TIME_SEC;
        if(dir_de)
            drop_nlink(new_inode);
        inode_dec_link_count(new_inode);
    } else {
    	rdfs_establish_mapping(new_dir);
        err = rdfs_add_link(new_dentry, old_inode);
        if(err)
            goto out_new_dir;
        if(dir_de)
            inode_inc_link_count(new_dir);
    }

    old_inode->i_ctime = CURRENT_TIME_SEC;
    mark_inode_dirty(old_inode);
	
	old_de = rdfs_find_entry2(old_dir, &old_dentry->d_name, &old_pde);
    rdfs_delete_entry(old_de, &old_pde, old_dir);

    if(dir_de){
        if(old_dir != new_dir)
            rdfs_set_link(old_inode, dir_de, new_dir, 0);
        inode_dec_link_count(old_dir);
    }
	
	if(exist)
		rdfs_destroy_mapping(new_inode);
	if(isdir)
		rdfs_destroy_mapping(old_inode);
	
	rdfs_destroy_mapping(old_dir);
	rdfs_destroy_mapping(new_dir);
    return 0;

 out_new_dir:
 	rdfs_destroy_mapping(new_dir);
	
 out_new_inode:   
 	if(exist)
    rdfs_destroy_mapping(new_inode);
    
 out_old_inode:
 	if(isdir )
    rdfs_destroy_mapping(old_inode);
	
 out_old_dir:
    rdfs_destroy_mapping(old_dir);
    return err;
}

