/*
 * Copyright (c) 2004-2010 The Trustees of Indiana University and Indiana
 *                         University Research and Technology
 *                         Corporation.  All rights reserved.
 * Copyright (c) 2004-2005 The University of Tennessee and The University
 *                         of Tennessee Research Foundation.  All rights
 *                         reserved.
 * Copyright (c) 2004-2005 High Performance Computing Center Stuttgart,
 *                         University of Stuttgart.  All rights reserved.
 * Copyright (c) 2004-2005 The Regents of the University of California.
 *                         All rights reserved.
 * Copyright (c) 2007-2012 Cisco Systems, Inc.  All rights reserved.
 * Copyright (c) 2007      Sun Microsystems, Inc.  All rights reserved.
 * Copyright (c) 2009      Oak Ridge National Labs.  All rights reserved.
 * Copyright (c) 2010-2012 Los Alamos National Security, LLC.
 *                         All rights reserved.
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 */

/** @file **/

#include "opal_config.h"

#include "opal/util/malloc.h"
#include "opal/util/arch.h"
#include "opal/util/output.h"
#include "opal/util/trace.h"
#include "opal/util/show_help.h"
#include "opal/memoryhooks/memory.h"
#include "opal/mca/base/base.h"
#include "opal/runtime/opal.h"
#include "opal/util/net.h"
#include "opal/datatype/opal_datatype.h"
#include "opal/mca/installdirs/base/base.h"
#include "opal/mca/memory/base/base.h"
#include "opal/mca/memcpy/base/base.h"
#include "opal/mca/hwloc/base/base.h"
#include "opal/mca/timer/base/base.h"
#include "opal/mca/memchecker/base/base.h"
#include "opal/dss/dss.h"
#include "opal/mca/shmem/base/base.h"
#if OPAL_ENABLE_FT_CR    == 1
#include "opal/mca/compress/base/base.h"
#endif

#include "opal/runtime/opal_cr.h"
#include "opal/mca/crs/base/base.h"

#include "opal/runtime/opal_progress.h"
#include "opal/mca/event/base/base.h"
#include "opal/mca/backtrace/base/base.h"

#include "opal/constants.h"
#include "opal/util/error.h"
#include "opal/util/stacktrace.h"
#include "opal/util/keyval_parse.h"
#include "opal/util/sys_limits.h"

#if OPAL_CC_USE_PRAGMA_IDENT
#pragma ident OPAL_IDENT_STRING
#elif OPAL_CC_USE_IDENT
#ident OPAL_IDENT_STRING
#endif
const char opal_version_string[] = OPAL_IDENT_STRING;

int opal_initialized = 0;
int opal_util_initialized = 0;
/* We have to put a guess in here in case hwloc is not available.  If
   hwloc is available, this value will be overwritten when the
   hwloc data is loaded. */
int opal_cache_line_size = 128;

static int
opal_err2str(int errnum, const char **errmsg)
{
    const char *retval;

    switch (errnum) {
    case OPAL_SUCCESS:
        retval = "Success";
        break;
    case OPAL_ERROR:
        retval = "Error";
        break;
    case OPAL_ERR_OUT_OF_RESOURCE:
        retval = "Out of resource";
        break;
    case OPAL_ERR_TEMP_OUT_OF_RESOURCE:
        retval = "Temporarily out of resource";
        break;
    case OPAL_ERR_RESOURCE_BUSY:
        retval = "Resource busy";
        break;
    case OPAL_ERR_BAD_PARAM:
        retval = "Bad parameter";
        break;
    case OPAL_ERR_FATAL:
        retval = "Fatal";
        break;
    case OPAL_ERR_NOT_IMPLEMENTED:
        retval = "Not implemented";
        break;
    case OPAL_ERR_NOT_SUPPORTED:
        retval = "Not supported";
        break;
    case OPAL_ERR_INTERUPTED:
        retval = "Interupted";
        break;
    case OPAL_ERR_WOULD_BLOCK:
        retval = "Would block";
        break;
    case OPAL_ERR_IN_ERRNO:
        retval = "In errno";
        break;
    case OPAL_ERR_UNREACH:
        retval = "Unreachable";
        break;
    case OPAL_ERR_NOT_FOUND:
        retval = "Not found";
        break;
    case OPAL_EXISTS:
        retval = "Exists";
        break;
    case OPAL_ERR_TIMEOUT:
        retval = "Timeout";
        break;
    case OPAL_ERR_NOT_AVAILABLE:
        retval = "Not available";
        break;
    case OPAL_ERR_PERM:
        retval = "No permission";
        break;
    case OPAL_ERR_VALUE_OUT_OF_BOUNDS:
        retval = "Value out of bounds";
        break;
    case OPAL_ERR_FILE_READ_FAILURE:
        retval = "File read failure";
        break;
    case OPAL_ERR_FILE_WRITE_FAILURE:
        retval = "File write failure";
        break;
    case OPAL_ERR_FILE_OPEN_FAILURE:
        retval = "File open failure";
        break;
    case OPAL_ERR_PACK_MISMATCH:
        retval = "Pack data mismatch";
        break;
    case OPAL_ERR_PACK_FAILURE:
        retval = "Data pack failed";
        break;
    case OPAL_ERR_UNPACK_FAILURE:
        retval = "Data unpack failed";
        break;
    case OPAL_ERR_UNPACK_INADEQUATE_SPACE:
        retval = "Data unpack had inadequate space";
        break;
    case OPAL_ERR_UNPACK_READ_PAST_END_OF_BUFFER:
        retval = "Data unpack would read past end of buffer";
        break;
    case OPAL_ERR_OPERATION_UNSUPPORTED:
        retval = "Requested operation is not supported on referenced data type";
        break;
    case OPAL_ERR_UNKNOWN_DATA_TYPE:
        retval = "Unknown data type";
        break;
    case OPAL_ERR_BUFFER:
        retval = "Buffer type (described vs non-described) mismatch - operation not allowed";
        break;
    case OPAL_ERR_DATA_TYPE_REDEF:
        retval = "Attempt to redefine an existing data type";
        break;
    case OPAL_ERR_DATA_OVERWRITE_ATTEMPT:
        retval = "Attempt to overwrite a data value";
        break;
    case OPAL_ERR_MODULE_NOT_FOUND:
        retval = "Framework requires at least one active module, but none found";
        break;
    case OPAL_ERR_TOPO_SLOT_LIST_NOT_SUPPORTED:
        retval = "OS topology does not support slot_list process affinity";
        break;
    case OPAL_ERR_TOPO_SOCKET_NOT_SUPPORTED:
        retval = "Could not obtain socket topology information";
        break;
    case OPAL_ERR_TOPO_CORE_NOT_SUPPORTED:
        retval = "Could not obtain core topology information";
        break;
    case OPAL_ERR_NOT_ENOUGH_SOCKETS:
        retval = "Not enough sockets to meet request";
        break;
    case OPAL_ERR_NOT_ENOUGH_CORES:
        retval = "Not enough cores to meet request";
        break;
    case OPAL_ERR_INVALID_PHYS_CPU:
        retval = "Invalid physical cpu number returned";
        break;
    case OPAL_ERR_MULTIPLE_AFFINITIES:
        retval = "Multiple methods for assigning process affinity were specified";
        break;
    case OPAL_ERR_SLOT_LIST_RANGE:
        retval = "Provided slot_list range is invalid";
        break;
    case OPAL_ERR_NETWORK_NOT_PARSEABLE:
        retval = "Provided network specification is not parseable";
        break;
    case OPAL_ERR_SILENT:
        retval = NULL;
        break;
    case OPAL_ERR_NOT_INITIALIZED:
        retval = "Not initialized";
        break;
    case OPAL_ERR_NOT_BOUND:
        retval = "Not bound";
        break;
    default:
        retval = NULL;
    }

    *errmsg = retval;
    return OPAL_SUCCESS;
}


int
opal_init_util(int* pargc, char*** pargv)
{
    int ret;
    char *error = NULL;

    if( ++opal_util_initialized != 1 ) {
        if( opal_util_initialized < 1 ) {
            return OPAL_ERROR;
        }
        return OPAL_SUCCESS;
    }

    /* initialize the memory allocator */
    opal_malloc_init();

    /* initialize the output system */
    opal_output_init();

    /* initialize install dirs code */
    if (OPAL_SUCCESS != (ret = opal_installdirs_base_open())) {
        fprintf(stderr, "opal_installdirs_base_open() failed -- process will likely abort (%s:%d, returned %d instead of OPAL_INIT)\n",
                __FILE__, __LINE__, ret);
        return ret;
    }
    
    /* initialize the help system */
    opal_show_help_init();

    /* register handler for errnum -> string converstion */
    if (OPAL_SUCCESS != 
        (ret = opal_error_register("OPAL",
                                   OPAL_ERR_BASE, OPAL_ERR_MAX, opal_err2str))) {
        error = "opal_error_register";
        goto return_error;
    }

    /* init the trace function */
    opal_trace_init();

    /* keyval lex-based parser */
    if (OPAL_SUCCESS != (ret = opal_util_keyval_parse_init())) {
        error = "opal_util_keyval_parse_init";
        goto return_error;
    }

    if (OPAL_SUCCESS != (ret = opal_net_init())) {
        error = "opal_net_init";
        goto return_error;
    }

    /* Setup the parameter system */
    if (OPAL_SUCCESS != (ret = mca_base_param_init())) {
        error = "mca_base_param_init";
        goto return_error;
    }

    /* register params for opal */
    if (OPAL_SUCCESS != (ret = opal_register_params())) {
        error = "opal_register_params";
        goto return_error;
    }

    /* pretty-print stack handlers */
    if (OPAL_SUCCESS != (ret = opal_util_register_stackhandlers())) {
        error = "opal_util_register_stackhandlers";
        goto return_error;
    }

    if (OPAL_SUCCESS != (ret = opal_util_init_sys_limits())) {
        error = "opal_util_init_sys_limits";
        goto return_error;
    }

    /* initialize the arch string */
    if (OPAL_SUCCESS != (ret = opal_arch_init ())) {
        error = "opal_arch_init";
        goto return_error;
    }

    /* initialize the datatype engine */
    if (OPAL_SUCCESS != (ret = opal_datatype_init ())) {
        error = "opal_datatype_init";
        goto return_error;
    }

    /* Initialize the data storage service. */
    if (OPAL_SUCCESS != (ret = opal_dss_open())) {
        error = "opal_dss_open";
        goto return_error;
    }

    return OPAL_SUCCESS;

 return_error:
    opal_show_help( "help-opal-runtime.txt",
                    "opal_init:startup:internal-failure", true,
                    error, ret );
    return ret;
}


int
opal_init(int* pargc, char*** pargv)
{
    int ret;
    char *error = NULL;

    if( ++opal_initialized != 1 ) {
        if( opal_initialized < 1 ) {
            return OPAL_ERROR;
        }
        return OPAL_SUCCESS;
    }

    /* initialize util code */
    if (OPAL_SUCCESS != (ret = opal_init_util(pargc, pargv))) {
        return ret;
    }

    /* initialize the mca */
    if (OPAL_SUCCESS != (ret = mca_base_open())) {
        error = "mca_base_open";
        goto return_error;
    }

    /* open hwloc - since this is a static framework, no
     * select is required
     */
    if (OPAL_SUCCESS != (ret = opal_hwloc_base_open())) {
        error = "opal_hwloc_base_open";
        goto return_error;
    }

    /* the memcpy component should be one of the first who get
     * loaded in order to make sure we ddo have all the available
     * versions of memcpy correctly configured.
     */
    if( OPAL_SUCCESS != (ret = opal_memcpy_base_open()) ) {
        error = "opal_memcpy_base_open";
        goto return_error;
    }

    /* open the memory manager components.  Memory hooks may be
       triggered before this (any time after mem_free_init(),
       actually).  This is a hook available for memory manager hooks
       without good initialization routine support */
    if (OPAL_SUCCESS != (ret = opal_memory_base_open())) {
        error = "opal_memory_base_open";
        goto return_error;
    }

    /* initialize the memory manager / tracker */
    if (OPAL_SUCCESS != (ret = opal_mem_hooks_init())) {
        error = "opal_mem_hooks_init";
        goto return_error;
    }

    /* initialize the memory checker, to allow early support for annotation */
    if (OPAL_SUCCESS != (ret = opal_memchecker_base_open())) {
        error = "opal_memchecker_base_open";
        goto return_error;
    }

    /* select the memory checker */
    if (OPAL_SUCCESS != (ret = opal_memchecker_base_select())) {
        error = "opal_memchecker_base_select";
        goto return_error;
    }

    if (OPAL_SUCCESS != (ret = opal_backtrace_base_open())) {
        error = "opal_backtrace_base_open";
        goto return_error;
    }

    if (OPAL_SUCCESS != (ret = opal_timer_base_open())) {
        error = "opal_timer_base_open";
        goto return_error;
    }

    /*
     * Need to start the event and progress engines if noone else is.
     * opal_cr_init uses the progress engine, so it is lumped together
     * into this set as well.
     */
    /*
     * Initialize the event library
     */
    if (OPAL_SUCCESS != (ret = opal_event_base_open())) {
        error = "opal_event_base_open";
        goto return_error;
    }
            
    /*
     * Initialize the general progress engine
     */
    if (OPAL_SUCCESS != (ret = opal_progress_init())) {
        error = "opal_progress_init";
        goto return_error;
    }
    /* we want to tick the event library whenever possible */
    opal_progress_event_users_increment();

    /* setup the shmem framework */
    if (OPAL_SUCCESS != (ret = opal_shmem_base_open())) {
        error = "opal_shmem_base_open";
        goto return_error;
    }

    if (OPAL_SUCCESS != (ret = opal_shmem_base_select())) {
        error = "opal_shmem_base_select";
        goto return_error;
    }

#if OPAL_ENABLE_FT_CR    == 1
    /*
     * Initialize the compression framework
     * Note: Currently only used in C/R so it has been marked to only
     *       initialize when C/R is enabled. If other places in the code
     *       wish to use this framework, it is safe to remove the protection.
     */
    if( OPAL_SUCCESS != (ret = opal_compress_base_open()) ) {
        error = "opal_compress_base_open";
        goto return_error;
    }
    if( OPAL_SUCCESS != (ret = opal_compress_base_select()) ) {
        error = "opal_compress_base_select";
        goto return_error;
    }
#endif

    /*
     * Initalize the checkpoint/restart functionality
     * Note: Always do this so we can detect if the user
     * attempts to checkpoint a non checkpointable job,
     * otherwise the tools may hang or not clean up properly.
     */
    if (OPAL_SUCCESS != (ret = opal_cr_init() ) ) {
        error = "opal_cr_init";
        goto return_error;
    }
    
    return OPAL_SUCCESS;

 return_error:
    opal_show_help( "help-opal-runtime.txt",
                    "opal_init:startup:internal-failure", true,
                    error, ret );
    return ret;
}

