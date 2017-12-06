
#include "rdfs.h"
#include "debug.h"
#include "balloc.h"
#include "inode.h"
static inline struct rdfs_inode* get_lru_head(struct numa_des* numa_des)
{
	rdfs_trace();
	return (struct rdfs_inode*)((unsigned long) numa_des + sizeof(struct numa_des));
}

static inline struct rdfs_inode* get_lru_tail(struct numa_des* numa_des)
{
	rdfs_trace();
	return get_lru_head(numa_des);
}


void __rdfs_delete_node(struct rdfs_inode* ni , struct numa_des* numa_des)
{
	rdfs_trace();
	if (ni -> list_next != NULL)
	{
		((struct rdfs_inode*)(ni->list_pro))->list_next=ni->list_next;
		((struct rdfs_inode*)(ni->list_next))->list_pro=ni->list_pro;
		ni -> list_next = NULL;
		ni -> list_pro  = NULL;
	}
}

void rdfs_delete_node(struct rdfs_inode* ni)
{
	rdfs_trace();
	struct numa_des* numa_des = get_rdfs_inode_numa(ni);
	
	write_lock(&numa_des->lru_rwlock);

	__rdfs_delete_node(ni , numa_des);

	write_unlock(&numa_des->lru_rwlock);

}

void __rdfs_add_node(struct rdfs_inode* ni , struct numa_des* numa_des)
{
	rdfs_trace();
	struct rdfs_inode* tail = get_lru_tail(numa_des);
	ni->list_next=tail;
	ni->list_pro=tail->list_pro;
	((struct rdfs_inode*)(tail->list_pro))->list_next=ni;
	tail->list_pro=ni;
}

void rdfs_add_node(struct rdfs_inode* ni)
{
	rdfs_trace();
	struct numa_des* numa_des = get_rdfs_inode_numa(ni);

	write_lock(&numa_des->lru_rwlock);

	__rdfs_add_node(ni,numa_des);

	write_unlock(&numa_des->lru_rwlock);
}

//when read or write file , we will flush the file lru list 
void rdfs_flush_list(struct rdfs_inode* ni)
{	
	rdfs_trace();
	struct super_block* sb = RDFS_SUPER_BLOCK_ADDRESS;
	struct inode* inode = rdfs_iget(sb,get_rdfs_inodenr(sb,rdfs_pa(ni)));
	int ino = inode->i_ino;
	printk("%s  %d\n",__FUNCTION__,ino);
	struct numa_des* numa_des = get_rdfs_inode_numa(ni);

	write_lock(&numa_des->lru_rwlock);

	__rdfs_delete_node(ni , numa_des);
	__rdfs_add_node(ni , numa_des);

	write_unlock(&numa_des->lru_rwlock);
}

struct rdfs_inode * rdfs_get_first_node (struct numa_des* numa_des)
{
	rdfs_trace();
	struct rdfs_inode* head = get_lru_head(numa_des);
	struct rdfs_inode* tail = get_lru_tail(numa_des);
	struct rdfs_inode* ret = NULL;

	read_lock(&numa_des->lru_rwlock);

	ret = head->list_next;
	if (ret == tail)
	{
		ret = NULL;
	}

	read_unlock(&numa_des->lru_rwlock);
	return ret;
}

struct rdfs_inode * rdfs_get_next_node (struct rdfs_inode* ni)
{
	rdfs_trace();
	struct numa_des* numa_des = get_rdfs_inode_numa(ni);
	struct rdfs_inode* tail = get_lru_tail(numa_des);
	struct rdfs_inode* ret  = NULL;
	read_lock(&numa_des->lru_rwlock);

	if (ni->list_next != tail)
	{
		ret = ni->list_next;
	}

	read_unlock(&numa_des->lru_rwlock);

	return ret;
}

