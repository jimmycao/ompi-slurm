/*
 * Copyright (c) 2004-2007 The Trustees of Indiana University and Indiana
 *                         University Research and Technology
 *                         Corporation.  All rights reserved.
 * Copyright (c) 2004-2005 The University of Tennessee and The University
 *                         of Tennessee Research Foundation.  All rights
 *                         reserved.
 * Copyright (c) 2004-2005 High Performance Computing Center Stuttgart,
 *                         University of Stuttgart.  All rights reserved.
 * Copyright (c) 2004-2005 The Regents of the University of California.
 *                         All rights reserved.
 * Copyright (c) 2006-2012 Los Alamos National Security, LLC.  All rights
 *                         reserved. 
 * Copyright (c) 2007-2012 Cisco Systems, Inc.  All rights reserved.
 * Copyright (c) 2007-2008 Sun Microsystems, Inc.  All rights reserved.
 *
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 */

/** @file **/

#include "orte_config.h"
#include "orte/constants.h"

#include <sys/types.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "opal/util/error.h"
#include "opal/util/output.h"
#include "opal/runtime/opal.h"
#include "opal/threads/threads.h"

#include "orte/util/show_help.h"
#include "orte/mca/ess/base/base.h"
#include "orte/mca/ess/ess.h"
#include "orte/mca/errmgr/errmgr.h"
#include "orte/util/proc_info.h"
#include "orte/util/error_strings.h"

#include "orte/runtime/runtime.h"
#include "orte/runtime/orte_globals.h"
#include "orte/runtime/orte_locks.h"

/*
 * Whether we have completed orte_init or we are in orte_finalize
 */
int orte_initialized = 0;
bool orte_finalizing = false;
bool orte_debug_flag = false;
int orte_debug_verbosity;
char *orte_prohibited_session_dirs = NULL;
bool orte_create_session_dirs = true;
opal_event_base_t *orte_event_base;
bool orte_event_base_active = true;
bool orte_proc_is_bound = false;
int orte_progress_thread_debug = -1;
#if OPAL_HAVE_HWLOC
hwloc_cpuset_t orte_proc_applied_binding = NULL;
#endif

orte_process_name_t orte_name_wildcard = {ORTE_JOBID_WILDCARD, ORTE_VPID_WILDCARD};

orte_process_name_t orte_name_invalid = {ORTE_JOBID_INVALID, ORTE_VPID_INVALID}; 

#if ORTE_ENABLE_PROGRESS_THREADS
static void* orte_progress_thread_engine(opal_object_t *obj);
#endif

#if OPAL_CC_USE_PRAGMA_IDENT
#pragma ident ORTE_IDENT_STRING
#elif OPAL_CC_USE_IDENT
#ident ORTE_IDENT_STRING
#endif
const char orte_version_string[] = ORTE_IDENT_STRING;

int orte_init(int* pargc, char*** pargv, orte_proc_type_t flags)
{
    int ret;
    char *error = NULL;

    if (0 < orte_initialized) {
        /* track number of times we have been called */
        orte_initialized++;
        return ORTE_SUCCESS;
    }
    orte_initialized++;

    /* initialize the opal layer */
    if (ORTE_SUCCESS != (ret = opal_init(pargc, pargv))) {
        error = "opal_init";
        goto error;
    }
    
    /* ensure we know the type of proc for when we finalize */
    orte_process_info.proc_type = flags;

    /* setup the locks */
    if (ORTE_SUCCESS != (ret = orte_locks_init())) {
        error = "orte_locks_init";
        goto error;
    }
    
    /* Register all MCA Params */
    if (ORTE_SUCCESS != (ret = orte_register_params())) {
        error = "orte_register_params";
        goto error;
    }
    
    /* setup the orte_show_help system */
    if (ORTE_SUCCESS != (ret = orte_show_help_init())) {
        error = "opal_output_init";
        goto error;
    }
    
    /* register handler for errnum -> string conversion */
    opal_error_register("ORTE", ORTE_ERR_BASE, ORTE_ERR_MAX, orte_err2str);

    /* Ensure the rest of the process info structure is initialized */
    if (ORTE_SUCCESS != (ret = orte_proc_info())) {
        error = "orte_proc_info";
        goto error;
    }

    /* open the ESS and select the correct module for this environment */
    if (ORTE_SUCCESS != (ret = orte_ess_base_open())) {
        error = "orte_ess_base_open";
        goto error;
    }
    if (ORTE_SUCCESS != (ret = orte_ess_base_select())) {
        error = "orte_ess_base_select";
        goto error;
    }

#if ORTE_ENABLE_PROGRESS_THREADS
#if OPAL_EVENT_HAVE_THREAD_SUPPORT
        /* get a separate orte event base */
        orte_event_base = opal_event_base_create();
       /* construct the thread object */
        OBJ_CONSTRUCT(&orte_progress_thread, opal_thread_t);
        /* fork off a thread to progress it */
        orte_progress_thread.t_run = orte_progress_thread_engine;
        if (OPAL_SUCCESS != (ret = opal_thread_start(&orte_progress_thread))) {
            error = "orte progress thread start";
            goto error;
        }
#else
        error = "event thread support is not configured";
        ret = ORTE_ERROR;
        goto error;
#endif
#else
        /* set the event base to the opal one */
        orte_event_base = opal_event_base;
#endif

    /* initialize the RTE for this environment */
    if (ORTE_SUCCESS != (ret = orte_ess.init())) {
        error = "orte_ess_init";
        goto error;
    }
    
    /* All done */
    return ORTE_SUCCESS;
    
 error:
    if (ORTE_ERR_SILENT != ret) {
        orte_show_help("help-orte-runtime",
                       "orte_init:startup:internal-failure",
                       true, error, ORTE_ERROR_NAME(ret), ret);
    }

    return ret;
}


#if ORTE_ENABLE_PROGRESS_THREADS
static void* orte_progress_thread_engine(opal_object_t *obj)
{
    while (orte_event_base_active) {
        opal_event_loop(orte_event_base, OPAL_EVLOOP_ONCE);
    }
    return OPAL_THREAD_CANCELLED;
}
#endif
