#include <linux/module.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

MODULE_INFO(vermagic, VERMAGIC_STRING);

__visible struct module __this_module
__attribute__((section(".gnu.linkonce.this_module"))) = {
	.name = KBUILD_MODNAME,
	.init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
	.exit = cleanup_module,
#endif
	.arch = MODULE_ARCH_INIT,
};

static const struct modversion_info ____versions[]
__used
__attribute__((section("__versions"))) = {
	{ 0x58dc76f7, __VMLINUX_SYMBOL_STR(module_layout) },
	{ 0x2a6dc336, __VMLINUX_SYMBOL_STR(kmem_cache_destroy) },
	{ 0x4ba1d4ea, __VMLINUX_SYMBOL_STR(iget_failed) },
	{ 0x1fedf0f4, __VMLINUX_SYMBOL_STR(__request_region) },
	{ 0xfee21876, __VMLINUX_SYMBOL_STR(kmalloc_caches) },
	{ 0xd2b09ce5, __VMLINUX_SYMBOL_STR(__kmalloc) },
	{ 0x405c1144, __VMLINUX_SYMBOL_STR(get_seconds) },
	{ 0x53d0cb43, __VMLINUX_SYMBOL_STR(drop_nlink) },
	{ 0xe3d0a501, __VMLINUX_SYMBOL_STR(kernel_sendmsg) },
	{ 0x4bb90cdd, __VMLINUX_SYMBOL_STR(make_bad_inode) },
	{ 0xda3e43d1, __VMLINUX_SYMBOL_STR(_raw_spin_unlock) },
	{ 0x2dc91f92, __VMLINUX_SYMBOL_STR(generic_file_llseek) },
	{ 0x664e5276, __VMLINUX_SYMBOL_STR(__mark_inode_dirty) },
	{ 0x849e278d, __VMLINUX_SYMBOL_STR(ib_query_gid) },
	{ 0xc94431a9, __VMLINUX_SYMBOL_STR(__pud_alloc) },
	{ 0x27864d57, __VMLINUX_SYMBOL_STR(memparse) },
	{ 0x349cba85, __VMLINUX_SYMBOL_STR(strchr) },
	{ 0xa1ec2f2c, __VMLINUX_SYMBOL_STR(file_systems) },
	{ 0x69a358a6, __VMLINUX_SYMBOL_STR(iomem_resource) },
	{ 0x754d539c, __VMLINUX_SYMBOL_STR(strlen) },
	{ 0x9e140a72, __VMLINUX_SYMBOL_STR(sync_global_pgds) },
	{ 0xa13f4f30, __VMLINUX_SYMBOL_STR(ib_dealloc_pd) },
	{ 0xe09b4de0, __VMLINUX_SYMBOL_STR(init_mm) },
	{ 0x346a6548, __VMLINUX_SYMBOL_STR(from_kuid_munged) },
	{ 0x3881c999, __VMLINUX_SYMBOL_STR(kill_anon_super) },
	{ 0x815b5dd4, __VMLINUX_SYMBOL_STR(match_octal) },
	{ 0x237bea3c, __VMLINUX_SYMBOL_STR(posix_acl_to_xattr) },
	{ 0x9469482, __VMLINUX_SYMBOL_STR(kfree_call_rcu) },
	{ 0x79aa04a2, __VMLINUX_SYMBOL_STR(get_random_bytes) },
	{ 0x34184afe, __VMLINUX_SYMBOL_STR(current_kernel_time) },
	{ 0x1b6314fd, __VMLINUX_SYMBOL_STR(in_aton) },
	{ 0x3fb8a666, __VMLINUX_SYMBOL_STR(seq_puts) },
	{ 0xad9c2191, __VMLINUX_SYMBOL_STR(is_bad_inode) },
	{ 0x595ad9a, __VMLINUX_SYMBOL_STR(sock_release) },
	{ 0x91831d70, __VMLINUX_SYMBOL_STR(seq_printf) },
	{ 0x44b1d426, __VMLINUX_SYMBOL_STR(__dynamic_pr_debug) },
	{ 0x44e9a829, __VMLINUX_SYMBOL_STR(match_token) },
	{ 0x596338a9, __VMLINUX_SYMBOL_STR(inc_nlink) },
	{ 0x62e30841, __VMLINUX_SYMBOL_STR(init_user_ns) },
	{ 0xd92c706e, __VMLINUX_SYMBOL_STR(sock_create_kern) },
	{ 0x85df9b6c, __VMLINUX_SYMBOL_STR(strsep) },
	{ 0xdc1e133e, __VMLINUX_SYMBOL_STR(generic_read_dir) },
	{ 0xaff62180, __VMLINUX_SYMBOL_STR(kernel_listen) },
	{ 0x7a2af7b4, __VMLINUX_SYMBOL_STR(cpu_number) },
	{ 0x16db207c, __VMLINUX_SYMBOL_STR(mount_nodev) },
	{ 0x91715312, __VMLINUX_SYMBOL_STR(sprintf) },
	{ 0x26948d96, __VMLINUX_SYMBOL_STR(copy_user_enhanced_fast_string) },
	{ 0xb32cb742, __VMLINUX_SYMBOL_STR(kthread_create_on_node) },
	{ 0x5fad1ef8, __VMLINUX_SYMBOL_STR(ib_modify_qp) },
	{ 0xd3bc530e, __VMLINUX_SYMBOL_STR(out_of_line_wait_on_bit_lock) },
	{ 0x433e3643, __VMLINUX_SYMBOL_STR(from_kgid_munged) },
	{ 0x25a763d4, __VMLINUX_SYMBOL_STR(make_kgid) },
	{ 0xa5277eac, __VMLINUX_SYMBOL_STR(ib_create_qp) },
	{ 0x54fdb55f, __VMLINUX_SYMBOL_STR(__insert_inode_hash) },
	{ 0x67cf03be, __VMLINUX_SYMBOL_STR(inode_owner_or_capable) },
	{ 0x2cf1e1f2, __VMLINUX_SYMBOL_STR(ib_alloc_pd) },
	{ 0x3c80c06c, __VMLINUX_SYMBOL_STR(kstrtoull) },
	{ 0xfb578fc5, __VMLINUX_SYMBOL_STR(memset) },
	{ 0xa90d25f2, __VMLINUX_SYMBOL_STR(from_kuid) },
	{ 0x1d046a6, __VMLINUX_SYMBOL_STR(current_task) },
	{ 0x3cf51ce0, __VMLINUX_SYMBOL_STR(ib_get_dma_mr) },
	{ 0x376d00e2, __VMLINUX_SYMBOL_STR(ib_query_device) },
	{ 0xc124a159, __VMLINUX_SYMBOL_STR(__mutex_init) },
	{ 0x27e1a049, __VMLINUX_SYMBOL_STR(printk) },
	{ 0x20c55ae0, __VMLINUX_SYMBOL_STR(sscanf) },
	{ 0x449ad0a7, __VMLINUX_SYMBOL_STR(memcmp) },
	{ 0xafb8c6ff, __VMLINUX_SYMBOL_STR(copy_user_generic_string) },
	{ 0x4c9d28b0, __VMLINUX_SYMBOL_STR(phys_base) },
	{ 0xa1c76e0a, __VMLINUX_SYMBOL_STR(_cond_resched) },
	{ 0x9c575530, __VMLINUX_SYMBOL_STR(from_kgid) },
	{ 0x4d795d19, __VMLINUX_SYMBOL_STR(blkdev_get_by_path) },
	{ 0x4405ec14, __VMLINUX_SYMBOL_STR(ib_query_port) },
	{ 0x8926f6d9, __VMLINUX_SYMBOL_STR(kmem_cache_free) },
	{ 0x55ee8c33, __VMLINUX_SYMBOL_STR(set_nlink) },
	{ 0x49645768, __VMLINUX_SYMBOL_STR(get_cached_acl) },
	{ 0x393d4de9, __VMLINUX_SYMBOL_STR(crc32_le) },
	{ 0x58e02ce9, __VMLINUX_SYMBOL_STR(setattr_copy) },
	{ 0x60df1e3b, __VMLINUX_SYMBOL_STR(posix_acl_equiv_mode) },
	{ 0xc2cdbf1, __VMLINUX_SYMBOL_STR(synchronize_sched) },
	{ 0xa9617265, __VMLINUX_SYMBOL_STR(insert_inode_locked) },
	{ 0xb2d5b65c, __VMLINUX_SYMBOL_STR(truncate_pagecache) },
	{ 0x4e3567f7, __VMLINUX_SYMBOL_STR(match_int) },
	{ 0xba4f0fe3, __VMLINUX_SYMBOL_STR(unlock_page) },
	{ 0xf7978a56, __VMLINUX_SYMBOL_STR(generic_file_read_iter) },
	{ 0x36a7f14c, __VMLINUX_SYMBOL_STR(inode_init_once) },
	{ 0x72a98fdb, __VMLINUX_SYMBOL_STR(copy_user_generic_unrolled) },
	{ 0x2e86812f, __VMLINUX_SYMBOL_STR(ib_register_client) },
	{ 0xd01cac48, __VMLINUX_SYMBOL_STR(invalidate_inode_buffers) },
	{ 0x16e297c3, __VMLINUX_SYMBOL_STR(bit_wait) },
	{ 0xd63291e8, __VMLINUX_SYMBOL_STR(ib_unregister_event_handler) },
	{ 0xb26e7c3a, __VMLINUX_SYMBOL_STR(kmem_cache_alloc) },
	{ 0x6ac0d829, __VMLINUX_SYMBOL_STR(kmem_cache_alloc_node_trace) },
	{ 0xd071c726, __VMLINUX_SYMBOL_STR(dquot_initialize) },
	{ 0x1ef5f4bb, __VMLINUX_SYMBOL_STR(generic_file_mmap) },
	{ 0xca0f4667, __VMLINUX_SYMBOL_STR(readlink_copy) },
	{ 0x7258c3e3, __VMLINUX_SYMBOL_STR(ib_register_event_handler) },
	{ 0x7511591e, __VMLINUX_SYMBOL_STR(make_kuid) },
	{ 0x7add44b5, __VMLINUX_SYMBOL_STR(posix_acl_valid) },
	{ 0xdb7305a1, __VMLINUX_SYMBOL_STR(__stack_chk_fail) },
	{ 0xa202a8e5, __VMLINUX_SYMBOL_STR(kmalloc_order_trace) },
	{ 0x20eb248e, __VMLINUX_SYMBOL_STR(posix_acl_from_xattr) },
	{ 0xf3adfe52, __VMLINUX_SYMBOL_STR(unlock_new_inode) },
	{ 0x5e95b1cd, __VMLINUX_SYMBOL_STR(current_umask) },
	{ 0x732d1b8, __VMLINUX_SYMBOL_STR(wake_up_process) },
	{ 0xbdfb6dbb, __VMLINUX_SYMBOL_STR(__fentry__) },
	{ 0x7c61340c, __VMLINUX_SYMBOL_STR(__release_region) },
	{ 0xd795f578, __VMLINUX_SYMBOL_STR(inode_change_ok) },
	{ 0xcdedae3e, __VMLINUX_SYMBOL_STR(kmem_cache_alloc_trace) },
	{ 0xd52bf1ce, __VMLINUX_SYMBOL_STR(_raw_spin_lock) },
	{ 0x39964633, __VMLINUX_SYMBOL_STR(kmem_cache_create) },
	{ 0xc0e9ac29, __VMLINUX_SYMBOL_STR(d_tmpfile) },
	{ 0xcf38e133, __VMLINUX_SYMBOL_STR(kernel_recvmsg) },
	{ 0xfa2b34af, __VMLINUX_SYMBOL_STR(register_filesystem) },
	{ 0xd422a6dd, __VMLINUX_SYMBOL_STR(kernel_accept) },
	{ 0x97f2bda0, __VMLINUX_SYMBOL_STR(generic_file_write_iter) },
	{ 0xb3f7646e, __VMLINUX_SYMBOL_STR(kthread_should_stop) },
	{ 0xd3308559, __VMLINUX_SYMBOL_STR(iput) },
	{ 0x4c327f18, __VMLINUX_SYMBOL_STR(__pmd_alloc) },
	{ 0x37a0cba, __VMLINUX_SYMBOL_STR(kfree) },
	{ 0xcf623332, __VMLINUX_SYMBOL_STR(ihold) },
	{ 0x69acdf38, __VMLINUX_SYMBOL_STR(memcpy) },
	{ 0x605c2d10, __VMLINUX_SYMBOL_STR(__sb_end_write) },
	{ 0xa75312bc, __VMLINUX_SYMBOL_STR(call_rcu_sched) },
	{ 0xf98e51e0, __VMLINUX_SYMBOL_STR(pv_mmu_ops) },
	{ 0x23bafc87, __VMLINUX_SYMBOL_STR(d_splice_alias) },
	{ 0xedc03953, __VMLINUX_SYMBOL_STR(iounmap) },
	{ 0x68c7263, __VMLINUX_SYMBOL_STR(ioremap_cache) },
	{ 0xbbd4558d, __VMLINUX_SYMBOL_STR(kernel_bind) },
	{ 0x69cb6699, __VMLINUX_SYMBOL_STR(__sb_start_write) },
	{ 0x38a41eb5, __VMLINUX_SYMBOL_STR(d_make_root) },
	{ 0x6bcd590, __VMLINUX_SYMBOL_STR(inode_needs_sync) },
	{ 0x6dcc4af, __VMLINUX_SYMBOL_STR(unregister_filesystem) },
	{ 0x7de0434b, __VMLINUX_SYMBOL_STR(ib_create_cq) },
	{ 0x3d2bad04, __VMLINUX_SYMBOL_STR(new_inode) },
	{ 0xfe710486, __VMLINUX_SYMBOL_STR(noop_fsync) },
	{ 0x8c957599, __VMLINUX_SYMBOL_STR(clear_inode) },
	{ 0xe49a25fa, __VMLINUX_SYMBOL_STR(page_put_link) },
	{ 0xf25bc0a2, __VMLINUX_SYMBOL_STR(d_instantiate) },
	{ 0x64335b16, __VMLINUX_SYMBOL_STR(ib_unregister_client) },
	{ 0x3aa48231, __VMLINUX_SYMBOL_STR(dma_ops) },
	{ 0xf57816bb, __VMLINUX_SYMBOL_STR(clear_nlink) },
	{ 0x4a925310, __VMLINUX_SYMBOL_STR(iget_locked) },
	{ 0x2c08a588, __VMLINUX_SYMBOL_STR(inode_init_owner) },
	{ 0xe914e41e, __VMLINUX_SYMBOL_STR(strcpy) },
	{ 0xfd00fd0f, __VMLINUX_SYMBOL_STR(truncate_inode_pages) },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=";


MODULE_INFO(srcversion, "6EB19265102E26A66EAA40B");