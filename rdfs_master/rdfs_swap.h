/* @author page
 * @date 2017/12/3
 * rdfs/balloc.c
 */

#ifndef _RDFS_SWAP_H
#define _RDFS_SWAP_H

#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/sched.h>
#include <linux/bio.h>
#include <linux/rwlock.h>

#include <asm/barrier.h>
#include <asm/tlbflush.h>
#include <linux/delay.h>

#include "rdfs.h"

struct rdfs_inode* get_first_openning_file(struct numa_des* numa_des,struct rdfs_inode_info** p_ni_info);
struct rdfs_inode* get_next_openning_file(struct numa_des* numa_des,struct rdfs_inode_info** p_ni_info,struct rdfs_inode* ni);
struct rdfs_inode* get_first_closed_file(struct numa_des* numa_des,struct rdfs_inode_info** p_ni_info);
struct rdfs_inode* get_next_closed_file(struct numa_des* numa_des,struct rdfs_inode_info** p_ni_info,struct rdfs_inode* ni);
void rdfs_swap_buffer_init(struct swap_buffer* buf);
void update_swap_bufffer(struct rdfs_inode_info* info,pte_t* pte,unsigned long offset);
unsigned long swap_threshold(struct rdfs_inode* ni);
void clear_swap_buffer_accessed(struct rdfs_inode_info* ni_info);
unsigned long rdfs_swap_out(pte_t *pte,int client_idx);
unsigned long rdfs_swap_pte_range(pmd_t* pmd,unsigned long *start,unsigned long *size,unsigned long threshold,struct swap_buffer* swap_buffer,int client_idx);
unsigned long swap_pmd_range(pud_t* pud,unsigned long *start,unsigned long *size,unsigned long threshold, struct swap_buffer* swap_buffer,int client_idx);
unsigned long swap_pud_range(pud_t* pud,unsigned long *start,unsigned long *size,unsigned long threshold,struct swap_buffer* swap_buffer,int client_idx);
unsigned long swap_file_NVM(struct rdfs_inode* ni,struct rdfs_inode_info* ni_info,int clean,int lock,int client_idx);
unsigned long update_openning_hot_page(struct rdfs_inode* ni,struct rdfs_inode_info* ni_info);
unsigned long swap_openning_cold_page(struct rdfs_inode* ni,struct rdfs_inode_info* ni_info,unsigned long count,int client_idx);
unsigned long swap_openning_file(struct numa_des* numa_des, unsigned long count);
unsigned long swap_closed_file_rdfs(struct numa_des* numa_des, unsigned long count, int clean);
unsigned long swap_closed_file(struct numa_des* numa_des,unsigned long count);
unsigned long swap_oneself(struct rdfs_inode* ni,unsigned long count);
unsigned long rdfs_swap(struct numa_des* numa_des, struct rdfs_inode* ni,unsigned long count);
#endif