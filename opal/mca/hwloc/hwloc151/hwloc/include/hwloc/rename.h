/*
 * Copyright © 2009-2011 Cisco Systems, Inc.  All rights reserved.
 * Copyright © 2010-2012 inria.  All rights reserved.
 * See COPYING in top-level directory.
 */

#ifndef HWLOC_RENAME_H
#define HWLOC_RENAME_H

#include <hwloc/autogen/config.h>


#ifdef __cplusplus
extern "C" {
#endif


/* Only enact these defines if we're actually renaming the symbols
   (i.e., avoid trying to have no-op defines if we're *not*
   renaming). */

#if HWLOC_SYM_TRANSFORM

/* Use a preprocessor two-step in order to get the prefixing right.
   Make 2 macros: HWLOC_NAME and HWLOC_NAME_CAPS for renaming
   things. */

#define HWLOC_MUNGE_NAME(a, b) HWLOC_MUNGE_NAME2(a, b)
#define HWLOC_MUNGE_NAME2(a, b) a ## b
#define HWLOC_NAME(name) HWLOC_MUNGE_NAME(HWLOC_SYM_PREFIX, hwloc_ ## name)
#define HWLOC_NAME_CAPS(name) HWLOC_MUNGE_NAME(HWLOC_SYM_PREFIX_CAPS, hwloc_ ## name)

/* Now define all the "real" names to be the prefixed names.  This
   allows us to use the real names throughout the code base (i.e.,
   "hwloc_<foo>"); the preprocessor will adjust to have the prefixed
   name under the covers. */

/* Names from hwloc.h */

#define hwloc_get_api_version HWLOC_NAME(get_api_version)

#define hwloc_topology HWLOC_NAME(topology)
#define hwloc_topology_t HWLOC_NAME(topology_t)

#define hwloc_cpuset_t HWLOC_NAME(cpuset_t)
#define hwloc_const_cpuset_t HWLOC_NAME(const_cpuset_t)
#define hwloc_nodeset_t HWLOC_NAME(nodeset_t)
#define hwloc_const_nodeset_t HWLOC_NAME(const_nodeset_t)

#define HWLOC_OBJ_SYSTEM HWLOC_NAME_CAPS(OBJ_SYSTEM)
#define HWLOC_OBJ_MACHINE HWLOC_NAME_CAPS(OBJ_MACHINE)
#define HWLOC_OBJ_NODE HWLOC_NAME_CAPS(OBJ_NODE)
#define HWLOC_OBJ_SOCKET HWLOC_NAME_CAPS(OBJ_SOCKET)
#define HWLOC_OBJ_CACHE HWLOC_NAME_CAPS(OBJ_CACHE)
#define HWLOC_OBJ_CORE HWLOC_NAME_CAPS(OBJ_CORE)
#define HWLOC_OBJ_PU HWLOC_NAME_CAPS(OBJ_PU)
#define HWLOC_OBJ_MISC HWLOC_NAME_CAPS(OBJ_MISC)
#define HWLOC_OBJ_GROUP HWLOC_NAME_CAPS(OBJ_GROUP)
#define HWLOC_OBJ_BRIDGE HWLOC_NAME_CAPS(OBJ_BRIDGE)
#define HWLOC_OBJ_PCI_DEVICE HWLOC_NAME_CAPS(OBJ_PCI_DEVICE)
#define HWLOC_OBJ_OS_DEVICE HWLOC_NAME_CAPS(OBJ_OS_DEVICE)
#define HWLOC_OBJ_TYPE_MAX HWLOC_NAME_CAPS(OBJ_TYPE_MAX)
#define hwloc_obj_type_t HWLOC_NAME(obj_type_t)

#define hwloc_obj_cache_type_e HWLOC_NAME(obj_cache_type_e)
#define hwloc_obj_cache_type_t HWLOC_NAME(obj_cache_type_t)
#define HWLOC_OBJ_CACHE_UNIFIED HWLOC_NAME_CAPS(OBJ_CACHE_UNIFIED)
#define HWLOC_OBJ_CACHE_DATA HWLOC_NAME_CAPS(OBJ_CACHE_DATA)
#define HWLOC_OBJ_CACHE_INSTRUCTION HWLOC_NAME_CAPS(OBJ_CACHE_INSTRUCTION)

#define hwloc_obj_bridge_type_e HWLOC_NAME(obj_bridge_type_e)
#define hwloc_obj_bridge_type_t HWLOC_NAME(obj_bridge_type_t)
#define HWLOC_OBJ_BRIDGE_HOST HWLOC_NAME_CAPS(OBJ_BRIDGE_HOST)
#define HWLOC_OBJ_BRIDGE_PCI HWLOC_NAME_CAPS(OBJ_BRIDGE_PCI)

#define hwloc_obj_osdev_type_e HWLOC_NAME(obj_osdev_type_e)
#define hwloc_obj_osdev_type_t HWLOC_NAME(obj_osdev_type_t)
#define HWLOC_OBJ_OSDEV_BLOCK HWLOC_NAME_CAPS(OBJ_OSDEV_BLOCK)
#define HWLOC_OBJ_OSDEV_GPU HWLOC_NAME_CAPS(OBJ_OSDEV_GPU)
#define HWLOC_OBJ_OSDEV_NETWORK HWLOC_NAME_CAPS(OBJ_OSDEV_NETWORK)
#define HWLOC_OBJ_OSDEV_OPENFABRICS HWLOC_NAME_CAPS(OBJ_OSDEV_OPENFABRICS)
#define HWLOC_OBJ_OSDEV_DMA HWLOC_NAME_CAPS(OBJ_OSDEV_DMA)

#define hwloc_compare_types HWLOC_NAME(compare_types)

#define hwloc_compare_types_e HWLOC_NAME(compare_types_e)
#define HWLOC_TYPE_UNORDERED HWLOC_NAME_CAPS(TYPE_UNORDERED)

#define hwloc_obj_memory_s HWLOC_NAME(obj_memory_s)
#define hwloc_obj_memory_page_type_s HWLOC_NAME(obj_memory_page_type_s)

#define hwloc_obj HWLOC_NAME(obj)
#define hwloc_obj_t HWLOC_NAME(obj_t)

#define hwloc_distances_s HWLOC_NAME(distances_s)
#define hwloc_obj_info_s HWLOC_NAME(obj_info_s)

#define hwloc_obj_attr_u HWLOC_NAME(obj_attr_u)
#define hwloc_cache_attr_s HWLOC_NAME(cache_attr_s)
#define hwloc_group_attr_s HWLOC_NAME(group_attr_s)
#define hwloc_pcidev_attr_s HWLOC_NAME(pcidev_attr_s)
#define hwloc_bridge_attr_s HWLOC_NAME(bridge_attr_s)
#define hwloc_osdev_attr_s HWLOC_NAME(osdev_attr_s)

#define hwloc_topology_init HWLOC_NAME(topology_init)
#define hwloc_topology_load HWLOC_NAME(topology_load)
#define hwloc_topology_destroy HWLOC_NAME(topology_destroy)
#define hwloc_topology_check HWLOC_NAME(topology_check)
#define hwloc_topology_ignore_type HWLOC_NAME(topology_ignore_type)
#define hwloc_topology_ignore_type_keep_structure HWLOC_NAME(topology_ignore_type_keep_structure)
#define hwloc_topology_ignore_all_keep_structure HWLOC_NAME(topology_ignore_all_keep_structure)

#define hwloc_topology_flags_e HWLOC_NAME(topology_flags_e)

#define HWLOC_TOPOLOGY_FLAG_WHOLE_SYSTEM HWLOC_NAME_CAPS(TOPOLOGY_FLAG_WHOLE_SYSTEM)
#define HWLOC_TOPOLOGY_FLAG_IS_THISSYSTEM HWLOC_NAME_CAPS(TOPOLOGY_FLAG_IS_THISSYSTEM)
#define HWLOC_TOPOLOGY_FLAG_IO_DEVICES HWLOC_NAME_CAPS(TOPOLOGY_FLAG_IO_DEVICES)
#define HWLOC_TOPOLOGY_FLAG_IO_BRIDGES HWLOC_NAME_CAPS(TOPOLOGY_FLAG_IO_BRIDGES)
#define HWLOC_TOPOLOGY_FLAG_WHOLE_IO HWLOC_NAME_CAPS(TOPOLOGY_FLAG_WHOLE_IO)
#define HWLOC_TOPOLOGY_FLAG_ICACHES HWLOC_NAME_CAPS(TOPOLOGY_FLAG_ICACHES)

#define hwloc_topology_set_flags HWLOC_NAME(topology_set_flags)
#define hwloc_topology_set_fsroot HWLOC_NAME(topology_set_fsroot)
#define hwloc_topology_set_pid HWLOC_NAME(topology_set_pid)
#define hwloc_topology_set_synthetic HWLOC_NAME(topology_set_synthetic)
#define hwloc_topology_set_xml HWLOC_NAME(topology_set_xml)
#define hwloc_topology_set_xmlbuffer HWLOC_NAME(topology_set_xmlbuffer)
#define hwloc_topology_set_custom HWLOC_NAME(topology_set_custom)
#define hwloc_topology_set_distance_matrix HWLOC_NAME(topology_set_distance_matrix)

#define hwloc_topology_discovery_support HWLOC_NAME(topology_discovery_support)
#define hwloc_topology_cpubind_support HWLOC_NAME(topology_cpubind_support)
#define hwloc_topology_membind_support HWLOC_NAME(topology_membind_support)
#define hwloc_topology_support HWLOC_NAME(topology_support)
#define hwloc_topology_get_support HWLOC_NAME(topology_get_support)
#define hwloc_topology_export_xml HWLOC_NAME(topology_export_xml)
#define hwloc_topology_export_xmlbuffer HWLOC_NAME(topology_export_xmlbuffer)
#define hwloc_free_xmlbuffer HWLOC_NAME(free_xmlbuffer)

#define hwloc_topology_insert_misc_object_by_cpuset HWLOC_NAME(topology_insert_misc_object_by_cpuset)
#define hwloc_topology_insert_misc_object_by_parent HWLOC_NAME(topology_insert_misc_object_by_parent)

#define hwloc_custom_insert_topology HWLOC_NAME(custom_insert_topology)
#define hwloc_custom_insert_group_object_by_parent HWLOC_NAME(custom_insert_group_object_by_parent)

#define hwloc_restrict_flags_e HWLOC_NAME(restrict_flags_e)
#define HWLOC_RESTRICT_FLAG_ADAPT_DISTANCES HWLOC_NAME_CAPS(RESTRICT_FLAG_ADAPT_DISTANCES)
#define HWLOC_RESTRICT_FLAG_ADAPT_MISC HWLOC_NAME_CAPS(RESTRICT_FLAG_ADAPT_MISC)
#define HWLOC_RESTRICT_FLAG_ADAPT_IO HWLOC_NAME_CAPS(RESTRICT_FLAG_ADAPT_IO)
#define hwloc_topology_restrict HWLOC_NAME(topology_restrict)

#define hwloc_topology_get_depth HWLOC_NAME(topology_get_depth)
#define hwloc_get_type_depth HWLOC_NAME(get_type_depth)

#define hwloc_get_type_depth_e HWLOC_NAME(get_type_depth_e)
#define HWLOC_TYPE_DEPTH_UNKNOWN HWLOC_NAME_CAPS(TYPE_DEPTH_UNKNOWN)
#define HWLOC_TYPE_DEPTH_MULTIPLE HWLOC_NAME_CAPS(TYPE_DEPTH_MULTIPLE)
#define HWLOC_TYPE_DEPTH_BRIDGE HWLOC_NAME_CAPS(TYPE_DEPTH_BRIDGE)
#define HWLOC_TYPE_DEPTH_PCI_DEVICE HWLOC_NAME_CAPS(TYPE_DEPTH_PCI_DEVICE)
#define HWLOC_TYPE_DEPTH_OS_DEVICE HWLOC_NAME_CAPS(TYPE_DEPTH_OS_DEVICE)

#define hwloc_get_depth_type HWLOC_NAME(get_depth_type)
#define hwloc_get_nbobjs_by_depth HWLOC_NAME(get_nbobjs_by_depth)
#define hwloc_get_nbobjs_by_type HWLOC_NAME(get_nbobjs_by_type)

#define hwloc_topology_is_thissystem HWLOC_NAME(topology_is_thissystem)

#define hwloc_get_obj_by_depth HWLOC_NAME(get_obj_by_depth )
#define hwloc_get_obj_by_type HWLOC_NAME(get_obj_by_type )

#define hwloc_obj_type_string HWLOC_NAME(obj_type_string )
#define hwloc_obj_type_of_string HWLOC_NAME(obj_type_of_string )
#define hwloc_obj_type_snprintf HWLOC_NAME(obj_type_snprintf )
#define hwloc_obj_attr_snprintf HWLOC_NAME(obj_attr_snprintf )
#define hwloc_obj_snprintf HWLOC_NAME(obj_snprintf)
#define hwloc_obj_cpuset_snprintf HWLOC_NAME(obj_cpuset_snprintf)
#define hwloc_obj_get_info_by_name HWLOC_NAME(obj_get_info_by_name)
#define hwloc_obj_add_info HWLOC_NAME(obj_add_info)

#define HWLOC_CPUBIND_PROCESS HWLOC_NAME_CAPS(CPUBIND_PROCESS)
#define HWLOC_CPUBIND_THREAD HWLOC_NAME_CAPS(CPUBIND_THREAD)
#define HWLOC_CPUBIND_STRICT HWLOC_NAME_CAPS(CPUBIND_STRICT)
#define HWLOC_CPUBIND_NOMEMBIND HWLOC_NAME_CAPS(CPUBIND_NOMEMBIND)

#define hwloc_cpubind_flags_t HWLOC_NAME(cpubind_flags_t)

#define hwloc_set_cpubind HWLOC_NAME(set_cpubind)
#define hwloc_get_cpubind HWLOC_NAME(get_cpubind)
#define hwloc_set_proc_cpubind HWLOC_NAME(set_proc_cpubind)
#define hwloc_get_proc_cpubind HWLOC_NAME(get_proc_cpubind)
#define hwloc_set_thread_cpubind HWLOC_NAME(set_thread_cpubind)
#define hwloc_get_thread_cpubind HWLOC_NAME(get_thread_cpubind)

#define hwloc_get_last_cpu_location HWLOC_NAME(get_last_cpu_location)
#define hwloc_get_proc_last_cpu_location HWLOC_NAME(get_proc_last_cpu_location)

#define HWLOC_MEMBIND_DEFAULT HWLOC_NAME_CAPS(MEMBIND_DEFAULT)
#define HWLOC_MEMBIND_FIRSTTOUCH HWLOC_NAME_CAPS(MEMBIND_FIRSTTOUCH)
#define HWLOC_MEMBIND_BIND HWLOC_NAME_CAPS(MEMBIND_BIND)
#define HWLOC_MEMBIND_INTERLEAVE HWLOC_NAME_CAPS(MEMBIND_INTERLEAVE)
#define HWLOC_MEMBIND_REPLICATE HWLOC_NAME_CAPS(MEMBIND_REPLICATE)
#define HWLOC_MEMBIND_NEXTTOUCH HWLOC_NAME_CAPS(MEMBIND_NEXTTOUCH)
#define HWLOC_MEMBIND_MIXED HWLOC_NAME_CAPS(MEMBIND_MIXED)

#define hwloc_membind_policy_t HWLOC_NAME(membind_policy_t)

#define HWLOC_MEMBIND_PROCESS HWLOC_NAME_CAPS(MEMBIND_PROCESS)
#define HWLOC_MEMBIND_THREAD HWLOC_NAME_CAPS(MEMBIND_THREAD)
#define HWLOC_MEMBIND_STRICT HWLOC_NAME_CAPS(MEMBIND_STRICT)
#define HWLOC_MEMBIND_MIGRATE HWLOC_NAME_CAPS(MEMBIND_MIGRATE)
#define HWLOC_MEMBIND_NOCPUBIND HWLOC_NAME_CAPS(MEMBIND_NOCPUBIND)

#define hwloc_membind_flags_t HWLOC_NAME(membind_flags_t)

#define hwloc_set_membind_nodeset HWLOC_NAME(set_membind_nodeset)
#define hwloc_set_membind HWLOC_NAME(set_membind)
#define hwloc_get_membind_nodeset HWLOC_NAME(get_membind_nodeset)
#define hwloc_get_membind HWLOC_NAME(get_membind)
#define hwloc_set_proc_membind_nodeset HWLOC_NAME(set_proc_membind_nodeset)
#define hwloc_set_proc_membind HWLOC_NAME(set_proc_membind)
#define hwloc_get_proc_membind_nodeset HWLOC_NAME(get_proc_membind_nodeset)
#define hwloc_get_proc_membind HWLOC_NAME(get_proc_membind)
#define hwloc_set_area_membind_nodeset HWLOC_NAME(set_area_membind_nodeset)
#define hwloc_set_area_membind HWLOC_NAME(set_area_membind)
#define hwloc_get_area_membind_nodeset HWLOC_NAME(get_area_membind_nodeset)
#define hwloc_get_area_membind HWLOC_NAME(get_area_membind)
#define hwloc_alloc_membind_nodeset HWLOC_NAME(alloc_membind_nodeset)
#define hwloc_alloc_membind HWLOC_NAME(alloc_membind)
#define hwloc_alloc HWLOC_NAME(alloc)
#define hwloc_free HWLOC_NAME(free)

#define hwloc_get_non_io_ancestor_obj HWLOC_NAME(get_non_io_ancestor_obj)
#define hwloc_get_next_pcidev HWLOC_NAME(get_next_pcidev)
#define hwloc_get_pcidev_by_busid HWLOC_NAME(get_pcidev_by_busid)
#define hwloc_get_pcidev_by_busidstring HWLOC_NAME(get_pcidev_by_busidstring)
#define hwloc_get_next_osdev HWLOC_NAME(get_next_osdev)
#define hwloc_get_next_bridge HWLOC_NAME(get_next_bridge)
#define hwloc_bridge_covers_pcibus HWLOC_NAME(bridge_covers_pcibus)
#define hwloc_get_hostbridge_by_pcibus HWLOC_NAME(get_hostbridge_by_pcibus)

/* hwloc/bitmap.h */

#define hwloc_bitmap_s HWLOC_NAME(bitmap_s)
#define hwloc_bitmap_t HWLOC_NAME(bitmap_t)
#define hwloc_const_bitmap_t HWLOC_NAME(const_bitmap_t)

#define hwloc_bitmap_alloc HWLOC_NAME(bitmap_alloc)
#define hwloc_bitmap_alloc_full HWLOC_NAME(bitmap_alloc_full)
#define hwloc_bitmap_free HWLOC_NAME(bitmap_free)
#define hwloc_bitmap_dup HWLOC_NAME(bitmap_dup)
#define hwloc_bitmap_copy HWLOC_NAME(bitmap_copy)
#define hwloc_bitmap_snprintf HWLOC_NAME(bitmap_snprintf)
#define hwloc_bitmap_asprintf HWLOC_NAME(bitmap_asprintf)
#define hwloc_bitmap_sscanf HWLOC_NAME(bitmap_sscanf)
#define hwloc_bitmap_list_snprintf HWLOC_NAME(bitmap_list_snprintf)
#define hwloc_bitmap_list_asprintf HWLOC_NAME(bitmap_list_asprintf)
#define hwloc_bitmap_list_sscanf HWLOC_NAME(bitmap_list_sscanf)
#define hwloc_bitmap_taskset_snprintf HWLOC_NAME(bitmap_taskset_snprintf)
#define hwloc_bitmap_taskset_asprintf HWLOC_NAME(bitmap_taskset_asprintf)
#define hwloc_bitmap_taskset_sscanf HWLOC_NAME(bitmap_taskset_sscanf)
#define hwloc_bitmap_zero HWLOC_NAME(bitmap_zero)
#define hwloc_bitmap_fill HWLOC_NAME(bitmap_fill)
#define hwloc_bitmap_from_ulong HWLOC_NAME(bitmap_from_ulong)

#define hwloc_bitmap_from_ith_ulong HWLOC_NAME(bitmap_from_ith_ulong)
#define hwloc_bitmap_to_ulong HWLOC_NAME(bitmap_to_ulong)
#define hwloc_bitmap_to_ith_ulong HWLOC_NAME(bitmap_to_ith_ulong)
#define hwloc_bitmap_only HWLOC_NAME(bitmap_only)
#define hwloc_bitmap_allbut HWLOC_NAME(bitmap_allbut)
#define hwloc_bitmap_set HWLOC_NAME(bitmap_set)
#define hwloc_bitmap_set_range HWLOC_NAME(bitmap_set_range)
#define hwloc_bitmap_set_ith_ulong HWLOC_NAME(bitmap_set_ith_ulong)
#define hwloc_bitmap_clr HWLOC_NAME(bitmap_clr)
#define hwloc_bitmap_clr_range HWLOC_NAME(bitmap_clr_range)
#define hwloc_bitmap_isset HWLOC_NAME(bitmap_isset)
#define hwloc_bitmap_iszero HWLOC_NAME(bitmap_iszero)
#define hwloc_bitmap_isfull HWLOC_NAME(bitmap_isfull)
#define hwloc_bitmap_isequal HWLOC_NAME(bitmap_isequal)
#define hwloc_bitmap_intersects HWLOC_NAME(bitmap_intersects)
#define hwloc_bitmap_isincluded HWLOC_NAME(bitmap_isincluded)
#define hwloc_bitmap_or HWLOC_NAME(bitmap_or)
#define hwloc_bitmap_and HWLOC_NAME(bitmap_and)
#define hwloc_bitmap_andnot HWLOC_NAME(bitmap_andnot)
#define hwloc_bitmap_xor HWLOC_NAME(bitmap_xor)
#define hwloc_bitmap_not HWLOC_NAME(bitmap_not)
#define hwloc_bitmap_first HWLOC_NAME(bitmap_first)
#define hwloc_bitmap_last HWLOC_NAME(bitmap_last)
#define hwloc_bitmap_next HWLOC_NAME(bitmap_next)
#define hwloc_bitmap_singlify HWLOC_NAME(bitmap_singlify)
#define hwloc_bitmap_compare_first HWLOC_NAME(bitmap_compare_first)
#define hwloc_bitmap_compare HWLOC_NAME(bitmap_compare)
#define hwloc_bitmap_weight HWLOC_NAME(bitmap_weight)

/* hwloc/helper.h */

#define hwloc_get_type_or_below_depth HWLOC_NAME(get_type_or_below_depth)
#define hwloc_get_type_or_above_depth HWLOC_NAME(get_type_or_above_depth)
#define hwloc_get_root_obj HWLOC_NAME(get_root_obj)
#define hwloc_get_system_obj HWLOC_NAME(get_system_obj)
#define hwloc_get_ancestor_obj_by_depth HWLOC_NAME(get_ancestor_obj_by_depth)
#define hwloc_get_ancestor_obj_by_type HWLOC_NAME(get_ancestor_obj_by_type)
#define hwloc_get_next_obj_by_depth HWLOC_NAME(get_next_obj_by_depth)
#define hwloc_get_next_obj_by_type HWLOC_NAME(get_next_obj_by_type)
#define hwloc_get_pu_obj_by_os_index HWLOC_NAME(get_pu_obj_by_os_index)
#define hwloc_get_next_child HWLOC_NAME(get_next_child)
#define hwloc_get_common_ancestor_obj HWLOC_NAME(get_common_ancestor_obj)
#define hwloc_obj_is_in_subtree HWLOC_NAME(obj_is_in_subtree)
#define hwloc_get_first_largest_obj_inside_cpuset HWLOC_NAME(get_first_largest_obj_inside_cpuset)
#define hwloc_get_largest_objs_inside_cpuset HWLOC_NAME(get_largest_objs_inside_cpuset)
#define hwloc_get_next_obj_inside_cpuset_by_depth HWLOC_NAME(get_next_obj_inside_cpuset_by_depth)
#define hwloc_get_next_obj_inside_cpuset_by_type HWLOC_NAME(get_next_obj_inside_cpuset_by_type)
#define hwloc_get_obj_inside_cpuset_by_depth HWLOC_NAME(get_obj_inside_cpuset_by_depth)
#define hwloc_get_obj_inside_cpuset_by_type HWLOC_NAME(get_obj_inside_cpuset_by_type)
#define hwloc_get_nbobjs_inside_cpuset_by_depth HWLOC_NAME(get_nbobjs_inside_cpuset_by_depth)
#define hwloc_get_nbobjs_inside_cpuset_by_type HWLOC_NAME(get_nbobjs_inside_cpuset_by_type)
#define hwloc_get_obj_index_inside_cpuset HWLOC_NAME(get_obj_index_inside_cpuset)
#define hwloc_get_child_covering_cpuset HWLOC_NAME(get_child_covering_cpuset)
#define hwloc_get_obj_covering_cpuset HWLOC_NAME(get_obj_covering_cpuset)
#define hwloc_get_next_obj_covering_cpuset_by_depth HWLOC_NAME(get_next_obj_covering_cpuset_by_depth)
#define hwloc_get_next_obj_covering_cpuset_by_type HWLOC_NAME(get_next_obj_covering_cpuset_by_type)
#define hwloc_get_cache_type_depth HWLOC_NAME(get_cache_type_depth)
#define hwloc_get_cache_covering_cpuset HWLOC_NAME(get_cache_covering_cpuset)
#define hwloc_get_shared_cache_covering_obj HWLOC_NAME(get_shared_cache_covering_obj)
#define hwloc_get_closest_objs HWLOC_NAME(get_closest_objs)
#define hwloc_get_obj_below_by_type HWLOC_NAME(get_obj_below_by_type)
#define hwloc_get_obj_below_array_by_type HWLOC_NAME(get_obj_below_array_by_type)
#define hwloc_distributev HWLOC_NAME(distributev)
#define hwloc_distribute HWLOC_NAME(distribute)
#define hwloc_alloc_membind_policy HWLOC_NAME(alloc_membind_policy)
#define hwloc_alloc_membind_policy_nodeset HWLOC_NAME(alloc_membind_policy_nodeset)
#define hwloc_topology_get_complete_cpuset HWLOC_NAME(topology_get_complete_cpuset)
#define hwloc_topology_get_topology_cpuset HWLOC_NAME(topology_get_topology_cpuset)
#define hwloc_topology_get_online_cpuset HWLOC_NAME(topology_get_online_cpuset)
#define hwloc_topology_get_allowed_cpuset HWLOC_NAME(topology_get_allowed_cpuset)
#define hwloc_topology_get_complete_nodeset HWLOC_NAME(topology_get_complete_nodeset)
#define hwloc_topology_get_topology_nodeset HWLOC_NAME(topology_get_topology_nodeset)
#define hwloc_topology_get_allowed_nodeset HWLOC_NAME(topology_get_allowed_nodeset)
#define hwloc_cpuset_to_nodeset HWLOC_NAME(cpuset_to_nodeset)
#define hwloc_cpuset_to_nodeset_strict HWLOC_NAME(cpuset_to_nodeset_strict)
#define hwloc_cpuset_from_nodeset HWLOC_NAME(cpuset_from_nodeset)
#define hwloc_cpuset_from_nodeset_strict HWLOC_NAME(cpuset_from_nodeset_strict)
#define hwloc_get_whole_distance_matrix_by_depth HWLOC_NAME(get_whole_distance_matrix_by_depth)
#define hwloc_get_whole_distance_matrix_by_type HWLOC_NAME(get_whole_distance_matrix_by_type)
#define hwloc_get_distance_matrix_covering_obj_by_depth HWLOC_NAME(get_distance_matrix_covering_obj_by_depth)
#define hwloc_get_latency HWLOC_NAME(get_latency)

/* glibc-sched.h */

#define hwloc_cpuset_to_glibc_sched_affinity HWLOC_NAME(cpuset_to_glibc_sched_affinity)
#define hwloc_cpuset_from_glibc_sched_affinity HWLOC_NAME(cpuset_from_glibc_sched_affinity)

/* linux-libnuma.h */

#define hwloc_cpuset_to_linux_libnuma_ulongs HWLOC_NAME(cpuset_to_linux_libnuma_ulongs)
#define hwloc_nodeset_to_linux_libnuma_ulongs HWLOC_NAME(nodeset_to_linux_libnuma_ulongs)
#define hwloc_cpuset_from_linux_libnuma_ulongs HWLOC_NAME(cpuset_from_linux_libnuma_ulongs)
#define hwloc_nodeset_from_linux_libnuma_ulongs HWLOC_NAME(nodeset_from_linux_libnuma_ulongs)
#define hwloc_cpuset_to_linux_libnuma_bitmask HWLOC_NAME(cpuset_to_linux_libnuma_bitmask)
#define hwloc_nodeset_to_linux_libnuma_bitmask HWLOC_NAME(nodeset_to_linux_libnuma_bitmask)
#define hwloc_cpuset_from_linux_libnuma_bitmask HWLOC_NAME(cpuset_from_linux_libnuma_bitmask)
#define hwloc_nodeset_from_linux_libnuma_bitmask HWLOC_NAME(nodeset_from_linux_libnuma_bitmask)

/* linux.h */

#define hwloc_linux_parse_cpumap_file HWLOC_NAME(linux_parse_cpumap_file)
#define hwloc_linux_set_tid_cpubind HWLOC_NAME(linux_set_tid_cpubind)
#define hwloc_linux_get_tid_cpubind HWLOC_NAME(linux_get_tid_cpubind)

/* openfabrics-verbs.h */

#define hwloc_ibv_get_device_cpuset HWLOC_NAME(ibv_get_device_cpuset)
#define hwloc_ibv_get_device_osdev_by_name HWLOC_NAME(ibv_get_device_osdev_by_name)

/* myriexpress.h */

#define hwloc_mx_board_get_device_cpuset HWLOC_NAME(mx_board_get_device_cpuset)
#define hwloc_mx_endpoint_get_device_cpuset HWLOC_NAME(mx_endpoint_get_device_cpuset)

/* cuda.h */

#define hwloc_cuda_get_device_pci_ids HWLOC_NAME(cuda_get_device_pci_ids)
#define hwloc_cuda_get_device_cpuset HWLOC_NAME(cuda_get_device_cpuset)
#define hwloc_cuda_get_device_pcidev HWLOC_NAME(cuda_get_device_pcidev)

/* cudart.h */

#define hwloc_cudart_get_device_pci_ids HWLOC_NAME(cudart_get_device_pci_ids)
#define hwloc_cudart_get_device_cpuset HWLOC_NAME(cudart_get_device_cpuset)
#define hwloc_cudart_get_device_pcidev HWLOC_NAME(cudart_get_device_pcidev)

/* private/debug.h */

#define hwloc_debug HWLOC_NAME(debug)

/* private/misc.h */

#define hwloc_snprintf HWLOC_NAME(snprintf)
#define hwloc_namecoloncmp HWLOC_NAME(namecoloncmp)
/* FIXME: hwloc_ffsl may be a macro, but it may not be defined yet */
#define hwloc_ffs32 HWLOC_NAME(ffs32)
/* FIXME: hwloc_flsl may be a macro, but it may not be defined yet */
#define hwloc_fls32 HWLOC_NAME(fls32)
#define hwloc_weight_long HWLOC_NAME(weight_long)

/* private/cpuid.h */

#define hwloc_have_cpuid HWLOC_NAME(have_cpuid)
#define hwloc_cpuid HWLOC_NAME(cpuid)

/* private/xml.h */

#define hwloc__xml_verbose HWLOC_NAME(_xml_verbose)

#define hwloc__xml_import_state_s HWLOC_NAME(_xml_import_state_s)
#define hwloc__xml_import_state_t HWLOC_NAME(_xml_import_state_t)
#define hwloc__xml_export_output_s HWLOC_NAME(_xml_export_output_s)
#define hwloc__xml_export_output_t HWLOC_NAME(_xml_export_output_t)
#define hwloc__xml_export_object HWLOC_NAME(_xml_export_object)

#define hwloc_nolibxml_backend_init HWLOC_NAME(nolibxml_backend_init)
#define hwloc_nolibxml_export_file HWLOC_NAME(nolibxml_export_file)
#define hwloc_nolibxml_export_buffer HWLOC_NAME(nolibxml_export_buffer)
#define hwloc_nolibxml_free_buffer HWLOC_NAME(nolibxml_free_buffer)

#define hwloc_libxml_backend_init HWLOC_NAME(libxml_backend_init)
#define hwloc_libxml_export_file HWLOC_NAME(libxml_export_file)
#define hwloc_libxml_export_buffer HWLOC_NAME(libxml_export_buffer)
#define hwloc_libxml_free_buffer HWLOC_NAME(libxml_free_buffer)

/* private/private.h */

#define hwloc_ignore_type_e HWLOC_NAME(ignore_type_e)

#define HWLOC_IGNORE_TYPE_NEVER HWLOC_NAME_CAPS(IGNORE_TYPE_NEVER)
#define HWLOC_IGNORE_TYPE_KEEP_STRUCTURE HWLOC_NAME_CAPS(IGNORE_TYPE_KEEP_STRUCTURE)
#define HWLOC_IGNORE_TYPE_ALWAYS HWLOC_NAME_CAPS(IGNORE_TYPE_ALWAYS)

#define hwloc_os_distances_s HWLOC_NAME(os_distances_s)
#define hwloc_backend_e HWLOC_NAME(backend_e)
#define hwloc_backend_t HWLOC_NAME(backend_t)

#define HWLOC_BACKEND_NONE HWLOC_NAME_CAPS(BACKEND_NONE)
#define HWLOC_BACKEND_SYNTHETIC HWLOC_NAME_CAPS(BACKEND_SYNTHETIC)
#define HWLOC_BACKEND_LINUXFS HWLOC_NAME_CAPS(BACKEND_LINUXFS)
#define HWLOC_BACKEND_XML HWLOC_NAME_CAPS(BACKEND_XML)
#define HWLOC_BACKEND_CUSTOM HWLOC_NAME_CAPS(BACKEND_CUSTOM)
#define HWLOC_BACKEND_MAX HWLOC_NAME_CAPS(BACKEND_MAX)

#define hwloc_backend_params_u HWLOC_NAME(backend_params_u)
#define hwloc_backend_params_linuxfs_s HWLOC_NAME(backend_params_linuxfs_s)
#define hwloc_backend_params_osf HWLOC_NAME(backend_params_osf)
#define hwloc_backend_params_xml_s HWLOC_NAME(backend_params_xml_s)
#define hwloc_backend_params_synthetic_s HWLOC_NAME(backend_params_synthetic_s)

#define hwloc_xml_imported_distances_s HWLOC_NAME(xml_imported_distances_s)

#define hwloc_setup_pu_level HWLOC_NAME(setup_pu_level)
#define hwloc_get_sysctlbyname HWLOC_NAME(get_sysctlbyname)
#define hwloc_get_sysctl HWLOC_NAME(get_sysctl)
#define hwloc_fallback_nbprocessors HWLOC_NAME(fallback_nbprocessors)
#define hwloc_connect_children HWLOC_NAME(connect_children)
#define hwloc_connect_levels HWLOC_NAME(connect_levels)

#define hwloc_look_linuxfs HWLOC_NAME(look_linuxfs)
#define hwloc_set_linuxfs_hooks HWLOC_NAME(set_linuxfs_hooks)
#define hwloc_backend_linuxfs_init HWLOC_NAME(backend_linuxfs_init)
#define hwloc_backend_linuxfs_exit HWLOC_NAME(backend_linuxfs_exit)
#define hwloc_linuxfs_pci_lookup_osdevices HWLOC_NAME(linuxfs_pci_lookup_osdevices)
#define hwloc_linuxfs_get_pcidev_cpuset HWLOC_NAME(linuxfs_get_pcidev_cpuset)

#define hwloc_backend_xml_init HWLOC_NAME(backend_xml_init)
#define hwloc_look_xml HWLOC_NAME(look_xml)
#define hwloc_backend_xml_exit HWLOC_NAME(backend_xml_exit)

#define hwloc_look_solaris HWLOC_NAME(look_solaris)
#define hwloc_set_solaris_hooks HWLOC_NAME(set_solaris_hooks)

#define hwloc_look_aix HWLOC_NAME(look_aix)
#define hwloc_set_aix_hooks HWLOC_NAME(set_aix_hooks)

#define hwloc_look_osf HWLOC_NAME(look_osf)
#define hwloc_set_osf_hooks HWLOC_NAME(set_osf_hooks)

#define hwloc_look_windows HWLOC_NAME(look_windows)
#define hwloc_set_windows_hooks HWLOC_NAME(set_windows_hooks)

#define hwloc_look_darwin HWLOC_NAME(look_darwin)
#define hwloc_set_darwin_hooks HWLOC_NAME(set_darwin_hooks)

#define hwloc_look_freebsd HWLOC_NAME(look_freebsd)
#define hwloc_set_freebsd_hooks HWLOC_NAME(set_freebsd_hooks)

#define hwloc_look_hpux HWLOC_NAME(look_hpux)
#define hwloc_set_hpux_hooks HWLOC_NAME(set_hpux_hooks)

#define hwloc_look_libpci HWLOC_NAME(look_libpci)

#define hwloc_look_x86 HWLOC_NAME(look_x86)

#define hwloc_backend_synthetic_init HWLOC_NAME(backend_synthetic_init)
#define hwloc_backend_synthetic_exit HWLOC_NAME(backend_synthetic_exit)
#define hwloc_look_synthetic  HWLOC_NAME(look_synthetic )

#define hwloc_insert_object_by_cpuset HWLOC_NAME(insert_object_by_cpuset)
#define hwloc_report_error_t HWLOC_NAME(report_error_t)
#define hwloc_report_os_error HWLOC_NAME(report_os_error)
#define hwloc_hide_errors HWLOC_NAME(hide_errors)
#define hwloc__insert_object_by_cpuset HWLOC_NAME(_insert_object_by_cpuset)
#define hwloc_insert_object_by_parent HWLOC_NAME(insert_object_by_parent)
#define hwloc_add_uname_info HWLOC_NAME(add_uname_info)
#define hwloc_bitmap_printf_value HWLOC_NAME(bitmap_printf_value)
#define hwloc_alloc_setup_object HWLOC_NAME(alloc_setup_object)
#define hwloc_free_unlinked_object HWLOC_NAME(free_unlinked_object)

#define hwloc_alloc_heap HWLOC_NAME(alloc_heap)
#define hwloc_alloc_mmap HWLOC_NAME(alloc_mmap)
#define hwloc_free_heap HWLOC_NAME(free_heap)
#define hwloc_free_mmap HWLOC_NAME(free_mmap)
#define hwloc_alloc_or_fail HWLOC_NAME(alloc_or_fail)

#define hwloc_distances_init HWLOC_NAME(distances_init)
#define hwloc_distances_clear HWLOC_NAME(distances_clear)
#define hwloc_distances_destroy HWLOC_NAME(distances_destroy)
#define hwloc_distances_set HWLOC_NAME(distances_set)
#define hwloc_distances_set_from_env HWLOC_NAME(distances_set_from_env)
#define hwloc_distances_restrict_os HWLOC_NAME(distances_restrict_os)
#define hwloc_distances_restrict HWLOC_NAME(distances_restrict)
#define hwloc_distances_finalize_os HWLOC_NAME(distances_finalize_os)
#define hwloc_distances_finalize_logical HWLOC_NAME(distances_finalize_logical)
#define hwloc_clear_object_distances HWLOC_NAME(clear_object_distances)
#define hwloc_clear_object_distances_one HWLOC_NAME(clear_object_distances_one)
#define hwloc_group_by_distances HWLOC_NAME(group_by_distances)

#endif /* HWLOC_SYM_TRANSFORM */


#ifdef __cplusplus
} /* extern "C" */
#endif


#endif /* HWLOC_RENAME_H */
