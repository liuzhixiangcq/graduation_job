/* @author page
 * @date 2017/12/3
 * rdfs/super.c
 */

#ifdef CONFIG_rdfs_XIP

#define mapping_is_xip(map) unlikely(map->a_ops->get_xip_mem)

#else

#define mapping_is_xip(map)	0
#define rdfs_use_xip(sb)	0
#define rdfs_get_xip_mem	NULL

#endif

