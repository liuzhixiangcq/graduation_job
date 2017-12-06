/* @author page
 * @date 2017/12/5
 * rdfs/rdfs_lru_op.h
 */

 #ifndef _RDFS_LRU_OP_H
 #define _RDFS_LRU_OP_H


 void rdfs_delete_node(struct rdfs_inode* ni);
 void rdfs_add_node(struct rdfs_inode* ni);
 void rdfs_flush_list(struct rdfs_inode* ni);
 struct rdfs_inode * rdfs_get_first_node (struct numa_des* numa_des);
 struct rdfs_inode * rdfs_get_next_node (struct rdfs_inode* ni);
 #endif