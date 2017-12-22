/* @author page
 * @date 2017/12/3
 * rdfs/acl.c
 */

#include <linux/capability.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include "xattr.h"
#include "acl.h"

#include "rdfs.h"

static inline int rdfs_acl_count(size_t size)
{
	//rdfs_trace();
	ssize_t s;
	size -= sizeof(struct rdfs_acl_header);
	s = size - 4 * sizeof(struct rdfs_acl_entry_short);
	if (s < 0) {
		printk("error :%s \n",__FUNCTION__);
		if (size % sizeof(struct rdfs_acl_entry_short))
			return -1;
		return size / sizeof(struct rdfs_acl_entry_short);
	} else {
		printk("error :%s \n",__FUNCTION__);
		if (s % sizeof(struct rdfs_acl_entry))
			return -1;
		return s / sizeof(struct rdfs_acl_entry) + 4;
	}
}
static struct posix_acl *rdfs_acl_load(const void *value, size_t size)
{
	//rdfs_trace();
	const char *end = (char *)value + size;
	int n, count;
	struct posix_acl *acl;

	if (!value)
		{
			printk("error :%s \n",__FUNCTION__);
			return NULL;
		}
	if (size < sizeof(struct rdfs_acl_header))
		{
			printk("error :%s \n",__FUNCTION__);
			return ERR_PTR(-EINVAL);
		}
	if (((struct rdfs_acl_header *)value)->a_version !=
	    cpu_to_be32(rdfs_ACL_VERSION))
		{
			printk("error :%s \n",__FUNCTION__);
			return ERR_PTR(-EINVAL);
		}
	value = (char *)value + sizeof(struct rdfs_acl_header);
	count = rdfs_acl_count(size);
	if (count < 0)
		{
			printk("error :%s \n",__FUNCTION__);
			return ERR_PTR(-EINVAL);
		}
	if (count == 0)
		{
			printk("error :%s \n",__FUNCTION__);
			return NULL;
		}
	acl = posix_acl_alloc(count, GFP_KERNEL);
	if (!acl)
		{
			printk("error :%s \n",__FUNCTION__);
			return ERR_PTR(-ENOMEM);
		}
	for (n = 0; n < count; n++) {
		struct rdfs_acl_entry *entry = (struct rdfs_acl_entry *)value;
		if ((char *)value + sizeof(struct rdfs_acl_entry_short) > end)
			goto fail;
		acl->a_entries[n].e_tag  = be16_to_cpu(entry->e_tag);
		acl->a_entries[n].e_perm = be16_to_cpu(entry->e_perm);
		switch (acl->a_entries[n].e_tag) {
		case ACL_USER_OBJ:
		case ACL_GROUP_OBJ:
		case ACL_MASK:
		case ACL_OTHER:
			value = (char *)value +
				sizeof(struct rdfs_acl_entry_short);
			break;
		case ACL_USER:
			value = (char *)value + sizeof(struct rdfs_acl_entry);
			if ((char *)value > end)
				goto fail;
			acl->a_entries[n].e_uid = make_kuid(&init_user_ns,
						be32_to_cpu(entry->e_id));
			break;
		case ACL_GROUP:
			value = (char *)value + sizeof(struct rdfs_acl_entry);
			if ((char *)value > end)
				goto fail;
			acl->a_entries[n].e_gid = make_kgid(&init_user_ns,
						be32_to_cpu(entry->e_id));
			break;
		default:
			goto fail;
		}
	}
	if (value != end)
		goto fail;
	return acl;

fail:
	printk("error :%s \n",__FUNCTION__);
	posix_acl_release(acl);
	return ERR_PTR(-EINVAL);
}
static inline size_t rdfs_acl_size(int count)
{
	//rdfs_trace();
	if (count <= 4) {
		return sizeof(struct rdfs_acl_header) +
		       count * sizeof(struct rdfs_acl_entry_short);
	} else {
		return sizeof(struct rdfs_acl_header) +
		       4 * sizeof(struct rdfs_acl_entry_short) +
		       (count - 4) * sizeof(struct rdfs_acl_entry);
	}
}
static void *rdfs_acl_save(const struct posix_acl *acl, size_t *size)
{
	//rdfs_trace();
	struct rdfs_acl_header *ext_acl;
	char *e;
	size_t n;

	*size = rdfs_acl_size(acl->a_count);
	ext_acl = kmalloc(sizeof(struct rdfs_acl_header) + acl->a_count *
			sizeof(struct rdfs_acl_entry), GFP_KERNEL);
	if (!ext_acl)
		{
			printk("error :%s \n",__FUNCTION__);
			return ERR_PTR(-ENOMEM);
		}
	ext_acl->a_version = cpu_to_be32(rdfs_ACL_VERSION);
	e = (char *)ext_acl + sizeof(struct rdfs_acl_header);
	for (n = 0; n < acl->a_count; n++) {
		const struct posix_acl_entry *acl_e = &acl->a_entries[n];
		struct rdfs_acl_entry *entry = (struct rdfs_acl_entry *)e;
		entry->e_tag  = cpu_to_le16(acl_e->e_tag);
		entry->e_perm = cpu_to_le16(acl_e->e_perm);
		switch(acl_e->e_tag) {
		case ACL_USER:
			entry->e_id = cpu_to_be32(
				from_kuid(&init_user_ns, acl_e->e_uid));
			e += sizeof(struct rdfs_acl_entry);
			break;
		case ACL_GROUP:
			entry->e_id = cpu_to_be32(
				from_kgid(&init_user_ns, acl_e->e_gid));
			e += sizeof(struct rdfs_acl_entry);
			break;
		case ACL_USER_OBJ:
		case ACL_GROUP_OBJ:
		case ACL_MASK:
		case ACL_OTHER:
			e += sizeof(struct rdfs_acl_entry_short);
			break;
		default:
			goto fail;
		}
	}
	return (char *)ext_acl;

fail:
	printk("error :%s \n",__FUNCTION__);
	kfree(ext_acl);
	return ERR_PTR(-EINVAL);
}

struct posix_acl *rdfs_get_acl(struct inode *inode, int type)
{
	//rdfs_trace();
	int name_index;
	char *value = NULL;
	struct posix_acl *acl;
	int retval;

	if (!test_opt(inode->i_sb, POSIX_ACL))
	{
		printk("error :%s \n",__FUNCTION__);
		return NULL;
	}

	acl = get_cached_acl(inode, type);
	if (acl != ACL_NOT_CACHED)
		return acl;

	switch (type) {
	case ACL_TYPE_ACCESS:
		name_index = RDFS_XATTR_INDEX_POSIX_ACL_ACCESS;
		break;
	case ACL_TYPE_DEFAULT:
		name_index = RDFS_XATTR_INDEX_POSIX_ACL_DEFAULT;
		break;
	default:
		BUG();
	}
	retval = rdfs_xattr_get(inode, name_index, "", NULL, 0);
	if (retval > 0) {
		value = kmalloc(retval, GFP_KERNEL);
		if (!value)
		{
			printk("error :%s \n",__FUNCTION__);
			return ERR_PTR(-ENOMEM);
		}
		retval = rdfs_xattr_get(inode, name_index, "", value, retval);
	}
	if (retval > 0)
		acl = rdfs_acl_load(value, retval);
	else if (retval == -ENODATA || retval == -ENOSYS)
		{
			printk("error :%s \n",__FUNCTION__);
			acl = NULL;
		}
	else
		{
			printk("error :%s \n",__FUNCTION__);
			acl = ERR_PTR(retval);
		}
	kfree(value);

	if (!IS_ERR(acl))
		{
			printk("error :%s \n",__FUNCTION__);
			set_cached_acl(inode, type, acl);
		}
	return acl;
}

static int rdfs_set_acl(struct inode *inode, int type, struct posix_acl *acl)
{
	//rdfs_trace();
	int name_index;
	void *value = NULL;
	size_t size = 0;
	int error;

	if (S_ISLNK(inode->i_mode))
		{
			printk("error :%s \n",__FUNCTION__);
			return -EOPNOTSUPP;
		}
	if (!test_opt(inode->i_sb, POSIX_ACL))
		return 0;

	switch (type) {
	case ACL_TYPE_ACCESS:
		name_index = RDFS_XATTR_INDEX_POSIX_ACL_ACCESS;
		if (acl) {
			error = posix_acl_equiv_mode(acl, &inode->i_mode);
			if (error < 0)
				{
					printk("error :%s \n",__FUNCTION__);
					return error;
				}
			else {
				inode->i_ctime = CURRENT_TIME_SEC;
				mark_inode_dirty(inode);
				if (error == 0)
					acl = NULL;
			}
		}
		break;
	case ACL_TYPE_DEFAULT:
		name_index = RDFS_XATTR_INDEX_POSIX_ACL_DEFAULT;
		if (!S_ISDIR(inode->i_mode))
			return acl ? -EACCES : 0;
		break;
	default:
		printk("error :%s \n",__FUNCTION__);
		return -EINVAL;
	}
	if (acl) {
		value = rdfs_acl_save(acl, &size);
		if (IS_ERR(value))
			{
				printk("error :%s \n",__FUNCTION__);
				return (int)PTR_ERR(value);
			}
	}

	error = rdfs_xattr_set(inode, name_index, "", value, size, 0);

	kfree(value);
	if (!error)
		set_cached_acl(inode, type, acl);
	return error;
}

int rdfs_init_acl(struct inode *inode, struct inode *dir)
{
	//rdfs_trace();
	struct posix_acl *acl = NULL;
	int error = 0;

	if (!S_ISLNK(inode->i_mode)) {
		if (test_opt(dir->i_sb, POSIX_ACL)) {
			acl = rdfs_get_acl(dir, ACL_TYPE_DEFAULT);
			if (IS_ERR(acl))
				{
					printk("error :%s \n",__FUNCTION__);
					return PTR_ERR(acl);
				}
		}
		if (!acl)
			inode->i_mode &= ~current_umask();
	}

	if (test_opt(inode->i_sb, POSIX_ACL) && acl) {
		umode_t mode = inode->i_mode;
		if (S_ISDIR(inode->i_mode)) {
			error = rdfs_set_acl(inode, ACL_TYPE_DEFAULT, acl);
			if (error)
				goto cleanup;
		}
		//error = posix_acl_create(&acl, 0,GFP_KERNEL, &mode);
		if (error < 0)
			{
				printk("error :%s \n",__FUNCTION__);
				return error;
			}
		inode->i_mode = mode;
		if (error > 0) {
			/* This is an extended ACL */
			error = rdfs_set_acl(inode, ACL_TYPE_ACCESS, acl);
		}
	}
cleanup:
	posix_acl_release(acl);
	return error;
}

int rdfs_acl_chmod(struct inode *inode)
{
	//rdfs_trace();
	struct posix_acl *acl;
	int error=0;

	if (!test_opt(inode->i_sb, POSIX_ACL))
		return 0;
	if (S_ISLNK(inode->i_mode))
		{
			printk("error :%s \n",__FUNCTION__);
			return -EOPNOTSUPP;
		}
	acl = rdfs_get_acl(inode, ACL_TYPE_ACCESS);
	if (IS_ERR(acl) || !acl)
		{
			printk("error :%s \n",__FUNCTION__);
			return PTR_ERR(acl);
		}
	//error = posix_acl_chmod(&acl,inode->i_mode);
	if (error)
		return error;
	error = rdfs_set_acl(inode, ACL_TYPE_ACCESS, acl); 
	posix_acl_release(acl);
	return error;
}

static size_t rdfs_xattr_list_acl_access(struct dentry *dentry, char *list,size_t list_size, const char *name,size_t name_len, int type)
{
//	rdfs_trace();
	const size_t size = sizeof(POSIX_ACL_XATTR_ACCESS);

	if (!test_opt(dentry->d_sb, POSIX_ACL))
		return 0;
	if (list && size <= list_size)
		memcpy(list, POSIX_ACL_XATTR_ACCESS, size);
	return size;
}

static size_t rdfs_xattr_list_acl_default(struct dentry *dentry, char *list,size_t list_size, const char *name,size_t name_len, int type)
{
//	rdfs_trace();
	const size_t size = sizeof(POSIX_ACL_XATTR_DEFAULT);

	if (!test_opt(dentry->d_sb, POSIX_ACL))
		return 0;
	if (list && size <= list_size)
		memcpy(list, POSIX_ACL_XATTR_DEFAULT, size);
	return size;
}

static int rdfs_xattr_get_acl(struct dentry *dentry, const char *name, void *buffer, size_t size, int type)
{
//	rdfs_trace();
	struct posix_acl *acl;
	int error;

	if (strcmp(name, "") != 0)
		{
			printk("error :%s \n",__FUNCTION__);
			return -EINVAL;
		}
	if (!test_opt(dentry->d_sb, POSIX_ACL))
		{
			printk("error :%s \n",__FUNCTION__);
			return -EOPNOTSUPP;
		}
	acl = rdfs_get_acl(dentry->d_inode, type);
	if (IS_ERR(acl))
		{
			printk("error :%s \n",__FUNCTION__);
			return PTR_ERR(acl);
		}
	if (acl == NULL)
		{
			printk("error :%s \n",__FUNCTION__);
			return -ENODATA;
		}
	error = posix_acl_to_xattr(&init_user_ns, acl, buffer, size);
	posix_acl_release(acl);

	return error;
}

static int rdfs_xattr_set_acl(struct dentry *dentry, const char *name,const void *value, size_t size, int flags,int type)
{
	//rdfs_trace();
	struct posix_acl *acl;
	int error;

	if (strcmp(name, "") != 0)
		{
			printk("error :%s \n",__FUNCTION__);
			return -EINVAL;
		}
	if (!test_opt(dentry->d_sb, POSIX_ACL))
		{
			printk("error :%s \n",__FUNCTION__);
			return -EOPNOTSUPP;
		}
	if (!inode_owner_or_capable(dentry->d_inode))
		{
			printk("error :%s \n",__FUNCTION__);
			return -EPERM;
		}
	if (value) {
		acl = posix_acl_from_xattr(&init_user_ns, value, size);
		if (IS_ERR(acl))
			{
				printk("error :%s \n",__FUNCTION__);
				return PTR_ERR(acl);
			}
		else if (acl) {
			error = posix_acl_valid(acl);
			if (error)
				goto release_and_out;
		}
	} else
		acl = NULL;

	error = rdfs_set_acl(dentry->d_inode, type, acl);

release_and_out:
	posix_acl_release(acl);
	return error;
}



const struct xattr_handler rdfs_xattr_acl_access_handler = {
	.prefix	= POSIX_ACL_XATTR_ACCESS,
	.flags	= ACL_TYPE_ACCESS,
	.list	= rdfs_xattr_list_acl_access,
	.get	= rdfs_xattr_get_acl,
	.set	= rdfs_xattr_set_acl,
};

const struct xattr_handler rdfs_xattr_acl_default_handler = {
	.prefix	= POSIX_ACL_XATTR_DEFAULT,
	.flags	= ACL_TYPE_DEFAULT,
	.list	= rdfs_xattr_list_acl_default,
	.get	= rdfs_xattr_get_acl,
	.set	= rdfs_xattr_set_acl,
};
