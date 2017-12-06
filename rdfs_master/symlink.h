/* @author page
 * @date 2017/12/3
 * rdfs/super.c
 */

 #ifndef _RDFS_SYMLINK_H
 #define _RDFS_SYMLINK_H

 #include <linux/fs.h>
 #include <linux/namei.h>
 #include "rdfs.h"
 #include "xattr.h"


 int rdfs_page_symlink(struct inode *inode, const char *symname, int len);
 
 #endif