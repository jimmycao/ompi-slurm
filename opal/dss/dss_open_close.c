/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 * Copyright (c) 2004-2007 The Trustees of Indiana University and Indiana
 *                         University Research and Technology
 *                         Corporation.  All rights reserved.
 * Copyright (c) 2004-2009 The University of Tennessee and The University
 *                         of Tennessee Research Foundation.  All rights
 *                         reserved.
 * Copyright (c) 2004-2005 High Performance Computing Center Stuttgart,
 *                         University of Stuttgart.  All rights reserved.
 * Copyright (c) 2004-2005 The Regents of the University of California.
 *                         All rights reserved.
 * Copyright (c) 2012-2013 Los Alamos National Security, Inc.  All rights reserved. 
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 */
/** @file:
 *
 */
#include "opal_config.h"

#include "opal/mca/base/mca_base_param.h"

#include "opal/dss/dss_internal.h"

/**
 * globals
 */
bool opal_dss_initialized = false;
int opal_dss_verbose = -1;  /* by default disabled */
int opal_dss_initial_size;
int opal_dss_threshold_size;
opal_pointer_array_t opal_dss_types;
opal_data_type_t opal_dss_num_reg_types;
opal_dss_buffer_type_t default_buf_type;

opal_dss_t opal_dss = {
    opal_dss_pack,
    opal_dss_unpack,
    opal_dss_copy,
    opal_dss_compare,
    opal_dss_print,
    opal_dss_structured,
    opal_dss_peek,
    opal_dss_unload,
    opal_dss_load,
    opal_dss_copy_payload,
    opal_dss_register,
    opal_dss_lookup_data_type,
    opal_dss_dump_data_types,
    opal_dss_dump
};

/**
 * Object constructors, destructors, and instantiations
 */
/** Value **/
static void opal_value_construct(opal_value_t* ptr)
{
    ptr->key = NULL;
    ptr->type = OPAL_UNDEF;
}
static void opal_value_destruct(opal_value_t* ptr)
{
    if (NULL != ptr->key) {
        free(ptr->key);
    }
}
OBJ_CLASS_INSTANCE(opal_value_t,
                   opal_list_item_t,
                   opal_value_construct,
                   opal_value_destruct);


static void opal_buffer_construct (opal_buffer_t* buffer)
{
    /** set the default buffer type */
    buffer->type = default_buf_type;

    /* Make everything NULL to begin with */

    buffer->base_ptr = buffer->pack_ptr = buffer->unpack_ptr = NULL;
    buffer->bytes_allocated = buffer->bytes_used = 0;
}

static void opal_buffer_destruct (opal_buffer_t* buffer)
{
    if (NULL != buffer->base_ptr) {
        free (buffer->base_ptr);
    }
}

OBJ_CLASS_INSTANCE(opal_buffer_t,
                   opal_object_t,
                   opal_buffer_construct,
                   opal_buffer_destruct);


static void opal_dss_type_info_construct(opal_dss_type_info_t *obj)
{
    obj->odti_name = NULL;
    obj->odti_pack_fn = NULL;
    obj->odti_unpack_fn = NULL;
    obj->odti_copy_fn = NULL;
    obj->odti_compare_fn = NULL;
    obj->odti_print_fn = NULL;
    obj->odti_structured = false;
}

static void opal_dss_type_info_destruct(opal_dss_type_info_t *obj)
{
    if (NULL != obj->odti_name) {
        free(obj->odti_name);
    }
}

OBJ_CLASS_INSTANCE(opal_dss_type_info_t, opal_object_t,
                   opal_dss_type_info_construct,
                   opal_dss_type_info_destruct);


static void opal_pstat_construct(opal_pstats_t *obj)
{
    memset(obj->node, 0, sizeof(obj->node));
    memset(obj->cmd, 0, sizeof(obj->cmd));
    obj->rank = 0;
    obj->pid = 0;
    obj->state[0] = 'U';
    obj->state[1] = '\0';
    obj->percent_cpu = 0.0;
    obj->time.tv_sec = 0;
    obj->time.tv_usec = 0;
    obj->priority = -1;
    obj->num_threads = -1;
    obj->vsize = 0.0;
    obj->rss = 0.0;
    obj->peak_vsize = 0.0;
    obj->processor = -1;
    obj->sample_time.tv_sec = 0;
    obj->sample_time.tv_usec = 0;
}
OBJ_CLASS_INSTANCE(opal_pstats_t, opal_list_item_t,
                   opal_pstat_construct,
                   NULL);

static void diskstat_cons(opal_diskstats_t *ptr)
{
    ptr->disk = NULL;
}
static void diskstat_dest(opal_diskstats_t *ptr)
{
    if (NULL != ptr->disk) {
        free(ptr->disk);
    }
}
OBJ_CLASS_INSTANCE(opal_diskstats_t,
                   opal_list_item_t,
                   diskstat_cons, diskstat_dest);

static void netstat_cons(opal_netstats_t *ptr)
{
    ptr->interface = NULL;
}
static void netstat_dest(opal_netstats_t *ptr)
{
    if (NULL != ptr->interface) {
        free(ptr->interface);
    }
}
OBJ_CLASS_INSTANCE(opal_netstats_t,
                   opal_list_item_t,
                   netstat_cons, netstat_dest);

static void opal_node_stats_construct(opal_node_stats_t *obj)
{
    obj->la = 0.0;
    obj->la5 = 0.0;
    obj->la15 = 0.0;
    obj->total_mem = 0;
    obj->free_mem = 0.0;
    obj->buffers = 0.0;
    obj->cached = 0.0;
    obj->swap_cached = 0.0;
    obj->swap_total = 0.0;
    obj->swap_free = 0.0;
    obj->mapped = 0.0;
    obj->sample_time.tv_sec = 0;
    obj->sample_time.tv_usec = 0;
    OBJ_CONSTRUCT(&obj->diskstats, opal_list_t);
    OBJ_CONSTRUCT(&obj->netstats, opal_list_t);
}
static void opal_node_stats_destruct(opal_node_stats_t *obj)
{
    opal_list_item_t *item;
    while (NULL != (item = opal_list_remove_first(&obj->diskstats))) {
        OBJ_RELEASE(item);
    }
    OBJ_DESTRUCT(&obj->diskstats);
    while (NULL != (item = opal_list_remove_first(&obj->netstats))) {
        OBJ_RELEASE(item);
    }
    OBJ_DESTRUCT(&obj->netstats);
}
OBJ_CLASS_INSTANCE(opal_node_stats_t, opal_object_t,
                   opal_node_stats_construct,
                   opal_node_stats_destruct);


int opal_dss_open(void)
{
    char *enviro_val;
    int rc;
    opal_data_type_t tmp;
    int def_type;

    if (opal_dss_initialized) {
        return OPAL_SUCCESS;
    }

    enviro_val = getenv("OPAL_dss_debug");
    if (NULL != enviro_val) {  /* debug requested */
        opal_dss_verbose = 0;
    }

    /** set the default buffer type. If we are in debug mode, then we default
     * to fully described buffers. Otherwise, we default to non-described for brevity
     * and performance
     */
#if OPAL_ENABLE_DEBUG
    def_type = OPAL_DSS_BUFFER_FULLY_DESC;
#else
    def_type = OPAL_DSS_BUFFER_NON_DESC;
#endif

    (void) mca_base_param_reg_int_name ("dss", "buffer_type",
					"Set the default mode for OpenRTE buffers (0=non-described, 1=described)",
					false, false, def_type, &rc);
    default_buf_type = rc;

    /* setup the initial size of the buffer. */
    (void) mca_base_param_reg_int_name ("dss", "buffer_initial_size", NULL,
					false, false, OPAL_DSS_DEFAULT_INITIAL_SIZE,
					&opal_dss_initial_size);

    /* the threshold as to where to stop doubling the size of the buffer 
     * allocated memory and start doing additive increases */
    (void) mca_base_param_reg_int_name ("dss", "buffer_threshold_size", NULL,
					false, false, OPAL_DSS_DEFAULT_THRESHOLD_SIZE,
					&opal_dss_threshold_size);

    /* Setup the types array */
    OBJ_CONSTRUCT(&opal_dss_types, opal_pointer_array_t);
    if (OPAL_SUCCESS != (rc = opal_pointer_array_init(&opal_dss_types,
                                                      OPAL_DSS_ID_DYNAMIC,
                                                      OPAL_DSS_ID_MAX,
                                                      OPAL_DSS_ID_MAX))) {
        return rc;
    }
    opal_dss_num_reg_types = 0;

    /* Register all the intrinsic types */

    tmp = OPAL_NULL;
    if (OPAL_SUCCESS != (rc = opal_dss.register_type(opal_dss_pack_null,
                                          opal_dss_unpack_null,
                                          (opal_dss_copy_fn_t)opal_dss_copy_null,
                                          (opal_dss_compare_fn_t)opal_dss_compare_null,
                                          (opal_dss_print_fn_t)opal_dss_print_null,
                                          OPAL_DSS_UNSTRUCTURED,
                                          "OPAL_NULL", &tmp))) {
        return rc;
    }
    tmp = OPAL_BYTE;
    if (OPAL_SUCCESS != (rc = opal_dss.register_type(opal_dss_pack_byte,
                                          opal_dss_unpack_byte,
                                          (opal_dss_copy_fn_t)opal_dss_std_copy,
                                          (opal_dss_compare_fn_t)opal_dss_compare_byte,
                                          (opal_dss_print_fn_t)opal_dss_print_byte,
                                          OPAL_DSS_UNSTRUCTURED,
                                          "OPAL_BYTE", &tmp))) {
        return rc;
    }
    tmp = OPAL_BOOL;
    if (OPAL_SUCCESS != (rc = opal_dss.register_type(opal_dss_pack_bool,
                                          opal_dss_unpack_bool,
                                          (opal_dss_copy_fn_t)opal_dss_std_copy,
                                          (opal_dss_compare_fn_t)opal_dss_compare_bool,
                                          (opal_dss_print_fn_t)opal_dss_print_bool,
                                          OPAL_DSS_UNSTRUCTURED,
                                          "OPAL_BOOL", &tmp))) {
        return rc;
    }
    tmp = OPAL_INT;
    if (OPAL_SUCCESS != (rc = opal_dss.register_type(opal_dss_pack_int,
                                          opal_dss_unpack_int,
                                          (opal_dss_copy_fn_t)opal_dss_std_copy,
                                          (opal_dss_compare_fn_t)opal_dss_compare_int,
                                          (opal_dss_print_fn_t)opal_dss_print_int,
                                          OPAL_DSS_UNSTRUCTURED,
                                          "OPAL_INT", &tmp))) {
        return rc;
    }
    tmp = OPAL_UINT;
    if (OPAL_SUCCESS != (rc = opal_dss.register_type(opal_dss_pack_int,
                                          opal_dss_unpack_int,
                                          (opal_dss_copy_fn_t)opal_dss_std_copy,
                                          (opal_dss_compare_fn_t)opal_dss_compare_uint,
                                          (opal_dss_print_fn_t)opal_dss_print_uint,
                                          OPAL_DSS_UNSTRUCTURED,
                                          "OPAL_UINT", &tmp))) {
        return rc;
    }
    tmp = OPAL_INT8;
    if (OPAL_SUCCESS != (rc = opal_dss.register_type(opal_dss_pack_byte,
                                          opal_dss_unpack_byte,
                                          (opal_dss_copy_fn_t)opal_dss_std_copy,
                                          (opal_dss_compare_fn_t)opal_dss_compare_int8,
                                          (opal_dss_print_fn_t)opal_dss_print_int8,
                                          OPAL_DSS_UNSTRUCTURED,
                                          "OPAL_INT8", &tmp))) {
        return rc;
    }
    tmp = OPAL_UINT8;
    if (OPAL_SUCCESS != (rc = opal_dss.register_type(opal_dss_pack_byte,
                                          opal_dss_unpack_byte,
                                          (opal_dss_copy_fn_t)opal_dss_std_copy,
                                          (opal_dss_compare_fn_t)opal_dss_compare_uint8,
                                          (opal_dss_print_fn_t)opal_dss_print_uint8,
                                          OPAL_DSS_UNSTRUCTURED,
                                          "OPAL_UINT8", &tmp))) {
        return rc;
    }
    tmp = OPAL_INT16;
    if (OPAL_SUCCESS != (rc = opal_dss.register_type(opal_dss_pack_int16,
                                          opal_dss_unpack_int16,
                                          (opal_dss_copy_fn_t)opal_dss_std_copy,
                                          (opal_dss_compare_fn_t)opal_dss_compare_int16,
                                          (opal_dss_print_fn_t)opal_dss_print_int16,
                                          OPAL_DSS_UNSTRUCTURED,
                                          "OPAL_INT16", &tmp))) {
        return rc;
    }
    tmp = OPAL_UINT16;
    if (OPAL_SUCCESS != (rc = opal_dss.register_type(opal_dss_pack_int16,
                                          opal_dss_unpack_int16,
                                          (opal_dss_copy_fn_t)opal_dss_std_copy,
                                          (opal_dss_compare_fn_t)opal_dss_compare_uint16,
                                          (opal_dss_print_fn_t)opal_dss_print_uint16,
                                          OPAL_DSS_UNSTRUCTURED,
                                          "OPAL_UINT16", &tmp))) {
        return rc;
    }
    tmp = OPAL_INT32;
    if (OPAL_SUCCESS != (rc = opal_dss.register_type(opal_dss_pack_int32,
                                          opal_dss_unpack_int32,
                                          (opal_dss_copy_fn_t)opal_dss_std_copy,
                                          (opal_dss_compare_fn_t)opal_dss_compare_int32,
                                          (opal_dss_print_fn_t)opal_dss_print_int32,
                                          OPAL_DSS_UNSTRUCTURED,
                                          "OPAL_INT32", &tmp))) {
        return rc;
    }
    tmp = OPAL_UINT32;
    if (OPAL_SUCCESS != (rc = opal_dss.register_type(opal_dss_pack_int32,
                                          opal_dss_unpack_int32,
                                          (opal_dss_copy_fn_t)opal_dss_std_copy,
                                          (opal_dss_compare_fn_t)opal_dss_compare_uint32,
                                          (opal_dss_print_fn_t)opal_dss_print_uint32,
                                          OPAL_DSS_UNSTRUCTURED,
                                          "OPAL_UINT32", &tmp))) {
        return rc;
    }
    tmp = OPAL_INT64;
    if (OPAL_SUCCESS != (rc = opal_dss.register_type(opal_dss_pack_int64,
                                          opal_dss_unpack_int64,
                                          (opal_dss_copy_fn_t)opal_dss_std_copy,
                                          (opal_dss_compare_fn_t)opal_dss_compare_int64,
                                          (opal_dss_print_fn_t)opal_dss_print_int64,
                                          OPAL_DSS_UNSTRUCTURED,
                                          "OPAL_INT64", &tmp))) {
        return rc;
    }
    tmp = OPAL_UINT64;
    if (OPAL_SUCCESS != (rc = opal_dss.register_type(opal_dss_pack_int64,
                                          opal_dss_unpack_int64,
                                          (opal_dss_copy_fn_t)opal_dss_std_copy,
                                          (opal_dss_compare_fn_t)opal_dss_compare_uint64,
                                          (opal_dss_print_fn_t)opal_dss_print_uint64,
                                          OPAL_DSS_UNSTRUCTURED,
                                          "OPAL_UINT64", &tmp))) {
        return rc;
    }
    tmp = OPAL_SIZE;
    if (OPAL_SUCCESS != (rc = opal_dss.register_type(opal_dss_pack_sizet,
                                          opal_dss_unpack_sizet,
                                          (opal_dss_copy_fn_t)opal_dss_std_copy,
                                          (opal_dss_compare_fn_t)opal_dss_compare_size,
                                          (opal_dss_print_fn_t)opal_dss_print_size,
                                          OPAL_DSS_UNSTRUCTURED,
                                          "OPAL_SIZE", &tmp))) {
        return rc;
    }
    tmp = OPAL_PID;
    if (OPAL_SUCCESS != (rc = opal_dss.register_type(opal_dss_pack_pid,
                                          opal_dss_unpack_pid,
                                          (opal_dss_copy_fn_t)opal_dss_std_copy,
                                          (opal_dss_compare_fn_t)opal_dss_compare_pid,
                                          (opal_dss_print_fn_t)opal_dss_print_pid,
                                          OPAL_DSS_UNSTRUCTURED,
                                          "OPAL_PID", &tmp))) {
        return rc;
    }
    tmp = OPAL_STRING;
    if (OPAL_SUCCESS != (rc = opal_dss.register_type(opal_dss_pack_string,
                                          opal_dss_unpack_string,
                                          (opal_dss_copy_fn_t)opal_dss_copy_string,
                                          (opal_dss_compare_fn_t)opal_dss_compare_string,
                                          (opal_dss_print_fn_t)opal_dss_print_string,
                                          OPAL_DSS_STRUCTURED,
                                          "OPAL_STRING", &tmp))) {
        return rc;
    }
    tmp = OPAL_DATA_TYPE;
    if (OPAL_SUCCESS != (rc = opal_dss.register_type(opal_dss_pack_data_type,
                                          opal_dss_unpack_data_type,
                                          (opal_dss_copy_fn_t)opal_dss_std_copy,
                                          (opal_dss_compare_fn_t)opal_dss_compare_dt,
                                          (opal_dss_print_fn_t)opal_dss_print_data_type,
                                          OPAL_DSS_UNSTRUCTURED,
                                          "OPAL_DATA_TYPE", &tmp))) {
        return rc;
    }

    tmp = OPAL_BYTE_OBJECT;
    if (OPAL_SUCCESS != (rc = opal_dss.register_type(opal_dss_pack_byte_object,
                                          opal_dss_unpack_byte_object,
                                          (opal_dss_copy_fn_t)opal_dss_copy_byte_object,
                                          (opal_dss_compare_fn_t)opal_dss_compare_byte_object,
                                          (opal_dss_print_fn_t)opal_dss_print_byte_object,
                                          OPAL_DSS_STRUCTURED,
                                          "OPAL_BYTE_OBJECT", &tmp))) {
        return rc;
    }

    tmp = OPAL_PSTAT;
    if (OPAL_SUCCESS != (rc = opal_dss.register_type(opal_dss_pack_pstat,
                                                     opal_dss_unpack_pstat,
                                                     (opal_dss_copy_fn_t)opal_dss_copy_pstat,
                                                     (opal_dss_compare_fn_t)opal_dss_compare_pstat,
                                                     (opal_dss_print_fn_t)opal_dss_print_pstat,
                                                     OPAL_DSS_STRUCTURED,
                                                     "OPAL_PSTAT", &tmp))) {
        return rc;
    }

    tmp = OPAL_NODE_STAT;
    if (OPAL_SUCCESS != (rc = opal_dss.register_type(opal_dss_pack_node_stat,
                                                     opal_dss_unpack_node_stat,
                                                     (opal_dss_copy_fn_t)opal_dss_copy_node_stat,
                                                     (opal_dss_compare_fn_t)opal_dss_compare_node_stat,
                                                     (opal_dss_print_fn_t)opal_dss_print_node_stat,
                                                     OPAL_DSS_STRUCTURED,
                                                     "OPAL_NODE_STAT", &tmp))) {
        return rc;
    }
    tmp = OPAL_VALUE;
    if (OPAL_SUCCESS != (rc = opal_dss.register_type(opal_dss_pack_value,
                                                     opal_dss_unpack_value,
                                                     (opal_dss_copy_fn_t)opal_dss_copy_value,
                                                     (opal_dss_compare_fn_t)opal_dss_compare_value,
                                                     (opal_dss_print_fn_t)opal_dss_print_value,
                                                     OPAL_DSS_STRUCTURED,
                                                     "OPAL_VALUE", &tmp))) {
        return rc;
    }
    tmp = OPAL_BUFFER;
    if (OPAL_SUCCESS != (rc = opal_dss.register_type(opal_dss_pack_buffer_contents,
                                                     opal_dss_unpack_buffer_contents,
                                                     (opal_dss_copy_fn_t)opal_dss_copy_buffer_contents,
                                                     (opal_dss_compare_fn_t)opal_dss_compare_buffer_contents,
                                                     (opal_dss_print_fn_t)opal_dss_print_buffer_contents,
                                                     OPAL_DSS_STRUCTURED,
                                                     "OPAL_BUFFER", &tmp))) {
        return rc;
    }
    tmp = OPAL_FLOAT;
    if (OPAL_SUCCESS != (rc = opal_dss.register_type(opal_dss_pack_float,
                                                     opal_dss_unpack_float,
                                                     (opal_dss_copy_fn_t)opal_dss_std_copy,
                                                     (opal_dss_compare_fn_t)opal_dss_compare_float,
                                                     (opal_dss_print_fn_t)opal_dss_print_float,
                                                     OPAL_DSS_UNSTRUCTURED,
                                                     "OPAL_FLOAT", &tmp))) {
        return rc;
    }
    tmp = OPAL_TIMEVAL;
    if (OPAL_SUCCESS != (rc = opal_dss.register_type(opal_dss_pack_timeval,
                                                     opal_dss_unpack_timeval,
                                                     (opal_dss_copy_fn_t)opal_dss_std_copy,
                                                     (opal_dss_compare_fn_t)opal_dss_compare_timeval,
                                                     (opal_dss_print_fn_t)opal_dss_print_timeval,
                                                     OPAL_DSS_UNSTRUCTURED,
                                                     "OPAL_TIMEVAL", &tmp))) {
        return rc;
    }
    /* All done */

    opal_dss_initialized = true;
    return OPAL_SUCCESS;
}


int opal_dss_close(void)
{
    int32_t i;

    if (!opal_dss_initialized) {
        return OPAL_SUCCESS;
    }
    opal_dss_initialized = false;

    for (i = 0 ; i < opal_pointer_array_get_size(&opal_dss_types) ; ++i) {
        opal_dss_type_info_t *info = (opal_dss_type_info_t*)opal_pointer_array_get_item(&opal_dss_types, i);
        if (NULL != info) {
            opal_pointer_array_set_item(&opal_dss_types, i, NULL);
            OBJ_RELEASE(info);
        }
    }

    OBJ_DESTRUCT(&opal_dss_types);

    return OPAL_SUCCESS;
}

bool opal_dss_structured(opal_data_type_t type)
{
    int i;

    /* find the type */
    for (i = 0 ; i < opal_dss_types.size ; ++i) {
        opal_dss_type_info_t *info = (opal_dss_type_info_t*)opal_pointer_array_get_item(&opal_dss_types, i);
        if (NULL != info && info->odti_type == type) {
            return info->odti_structured;
        }
    }

    /* default to false */
    return false;
}
