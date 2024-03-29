/*
 * Copyright (c) 2004-2010 The Trustees of Indiana University and Indiana
 *                         University Research and Technology
 *                         Corporation.  All rights reserved.
 * Copyright (c) 2004-2011 The University of Tennessee and The University
 *                         of Tennessee Research Foundation.  All rights
 *                         reserved.
 * Copyright (c) 2004-2005 High Performance Computing Center Stuttgart, 
 *                         University of Stuttgart.  All rights reserved.
 * Copyright (c) 2004-2005 The Regents of the University of California.
 *                         All rights reserved.
 * Copyright (c) 2010-2012 Oak Ridge National Labs.  All rights reserved.
 * Copyright (c) 2011-2012 Los Alamos National Security, LLC.  All rights
 *                         reserved. 
 * $COPYRIGHT$
 * 
 * Additional copyrights may follow
 * 
 * $HEADER$
 */

#include "orte_config.h"
#include "orte/constants.h"

#include <sys/types.h>
#include <stdio.h>
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "opal/mca/event/event.h"
#include "orte/util/show_help.h"
#include "opal/util/os_path.h"
#include "opal/util/output.h"
#include "opal/runtime/opal.h"
#include "opal/runtime/opal_cr.h"

#include "orte/mca/rml/base/base.h"
#include "orte/mca/routed/base/base.h"
#include "orte/mca/errmgr/errmgr.h"
#include "orte/mca/db/base/base.h"
#include "orte/mca/dfs/base/base.h"
#include "orte/mca/test/base/base.h"  //jimmy
#include "orte/mca/urm/base/base.h"  //
#include "orte/mca/grpcomm/base/base.h"
#include "orte/mca/rml/rml.h"
#include "orte/mca/odls/odls_types.h"
#include "orte/mca/plm/plm.h"
#include "orte/mca/filem/base/base.h"
#include "orte/mca/errmgr/base/base.h"
#if OPAL_ENABLE_FT_CR == 1
#include "orte/mca/snapc/base/base.h"
#endif
#include "orte/mca/state/base/base.h"
#include "orte/util/proc_info.h"
#include "orte/util/session_dir.h"
#include "orte/util/name_fns.h"
#include "orte/util/show_help.h"

#include "orte/runtime/orte_cr.h"
#include "orte/runtime/orte_globals.h"
#include "orte/runtime/orte_wait.h"

#include "orte/mca/ess/base/base.h"

int orte_ess_base_app_setup(void)
{
    int ret;
    char *error = NULL;

    /*
     * stdout/stderr buffering
     * If the user requested to override the default setting then do
     * as they wish.
     */
    if( orte_ess_base_std_buffering > -1 ) {
        if( 0 == orte_ess_base_std_buffering ) {
            setvbuf(stdout, NULL, _IONBF, 0);
            setvbuf(stderr, NULL, _IONBF, 0);
        }
        else if( 1 == orte_ess_base_std_buffering ) {
            setvbuf(stdout, NULL, _IOLBF, 0);
            setvbuf(stderr, NULL, _IOLBF, 0);
        }
        else if( 2 == orte_ess_base_std_buffering ) {
            setvbuf(stdout, NULL, _IOFBF, 0);
            setvbuf(stderr, NULL, _IOFBF, 0);
        }
    }

    /* open and setup the state machine */
    if (ORTE_SUCCESS != (ret = orte_state_base_open())) {
        ORTE_ERROR_LOG(ret);
        error = "orte_state_base_open";
        goto error;
    }
    if (ORTE_SUCCESS != (ret = orte_state_base_select())) {
        ORTE_ERROR_LOG(ret);
        error = "orte_state_base_select";
        goto error;
    }

    /* open the errmgr */
    if (ORTE_SUCCESS != (ret = orte_errmgr_base_open())) {
        ORTE_ERROR_LOG(ret);
        error = "orte_errmgr_base_open";
        goto error;
    }

    /* Setup the communication infrastructure */
    
    /* Runtime Messaging Layer */
    if (ORTE_SUCCESS != (ret = orte_rml_base_open())) {
        ORTE_ERROR_LOG(ret);
        error = "orte_rml_base_open";
        goto error;
    }
    if (ORTE_SUCCESS != (ret = orte_rml_base_select())) {
        ORTE_ERROR_LOG(ret);
        error = "orte_rml_base_select";
        goto error;
    }
    
    /* setup the errmgr */
    if (ORTE_SUCCESS != (ret = orte_errmgr_base_select())) {
        ORTE_ERROR_LOG(ret);
        error = "orte_errmgr_base_select";
        goto error;
    }

    /* Routed system */
    if (ORTE_SUCCESS != (ret = orte_routed_base_open())) {
        ORTE_ERROR_LOG(ret);
        error = "orte_routed_base_open";
        goto error;
    }
    if (ORTE_SUCCESS != (ret = orte_routed_base_select())) {
        ORTE_ERROR_LOG(ret);
        error = "orte_routed_base_select";
        goto error;
    }
    
    /* database */
    if (ORTE_SUCCESS != (ret = orte_db_base_open())) {
        ORTE_ERROR_LOG(ret);
        error = "orte_db_base_open";
        goto error;
    }
    if (ORTE_SUCCESS != (ret = orte_db_base_select())) {
        ORTE_ERROR_LOG(ret);
        error = "orte_db_base_select";
        goto error;
    }

    /*
     * Group communications
     */
    if (ORTE_SUCCESS != (ret = orte_grpcomm_base_open())) {
        ORTE_ERROR_LOG(ret);
        error = "orte_grpcomm_base_open";
        goto error;
    }
    if (ORTE_SUCCESS != (ret = orte_grpcomm_base_select())) {
        ORTE_ERROR_LOG(ret);
        error = "orte_grpcomm_base_select";
        goto error;
    }
    
    /* non-daemon/HNP apps can only have the default proxy PLM
     * module open - provide a chance for it to initialize
     */
    if (ORTE_SUCCESS != (ret = orte_plm.init())) {
        ORTE_ERROR_LOG(ret);
        error = "orte_plm_init";
        goto error;
    }
    
    /* enable communication via the rml */
    if (ORTE_SUCCESS != (ret = orte_rml.enable_comm())) {
        ORTE_ERROR_LOG(ret);
        error = "orte_rml.enable_comm";
        goto error;
    }
    
    /* setup my session directory */
    if (orte_create_session_dirs) {
        OPAL_OUTPUT_VERBOSE((2, orte_ess_base_output,
                             "%s setting up session dir with\n\ttmpdir: %s\n\thost %s",
                             ORTE_NAME_PRINT(ORTE_PROC_MY_NAME),
                             (NULL == orte_process_info.tmpdir_base) ? "UNDEF" : orte_process_info.tmpdir_base,
                             orte_process_info.nodename));
        
        if (ORTE_SUCCESS != (ret = orte_session_dir(true,
                                                    orte_process_info.tmpdir_base,
                                                    orte_process_info.nodename, NULL,
                                                    ORTE_PROC_MY_NAME))) {
            ORTE_ERROR_LOG(ret);
            error = "orte_session_dir";
            goto error;
        }
        
        /* Once the session directory location has been established, set
         the opal_output env file location to be in the
         proc-specific session directory. */
        opal_output_set_output_file_info(orte_process_info.proc_session_dir,
                                         "output-", NULL, NULL);
    }
    
    /* setup the routed info - the selected routed component
     * will know what to do. Some may put us in a blocking
     * receive here so they can get ALL of the contact info
     * from our peers. Others may just find the local daemon's
     * contact info and immediately return.
     */
    if (ORTE_SUCCESS != (ret = orte_routed.init_routes(ORTE_PROC_MY_NAME->jobid, NULL))) {
        ORTE_ERROR_LOG(ret);
        error = "orte_routed.init_routes";
        goto error;
    }
    
    
#if OPAL_ENABLE_FT_CR == 1
    /*
     * Setup the SnapC
     */
    if (ORTE_SUCCESS != (ret = orte_snapc_base_open())) {
        ORTE_ERROR_LOG(ret);
        error = "orte_snapc_base_open";
        goto error;
    }
    if (ORTE_SUCCESS != (ret = orte_snapc_base_select(ORTE_PROC_IS_HNP, !ORTE_PROC_IS_DAEMON))) {
        ORTE_ERROR_LOG(ret);
        error = "orte_snapc_base_select";
        goto error;
    }
    
    /* apps need the OPAL CR stuff */
    opal_cr_set_enabled(true);
#else
    opal_cr_set_enabled(false);
#endif
    
    /* Initalize the CR setup
     * Note: Always do this, even in non-FT builds.
     * If we don't some user level tools may hang.
     */
    if (ORTE_SUCCESS != (ret = orte_cr_init())) {
        ORTE_ERROR_LOG(ret);
        error = "orte_cr_init";
        goto error;
    }

    /* open the distributed file system */
    if (ORTE_SUCCESS != (ret = orte_dfs_base_open())) {
        ORTE_ERROR_LOG(ret);
        error = "orte_dfs_base_open";
        goto error;
    }
    if (ORTE_SUCCESS != (ret = orte_dfs_base_select())) {
        ORTE_ERROR_LOG(ret);
        error = "orte_dfs_base_select";
        goto error;
    }
    //====================jimmy=======test===============
    if (ORTE_SUCCESS != (ret = orte_test_base_open())) {
          ORTE_ERROR_LOG(ret);
          error = "orte_test_base_open";
          goto error;
      }
      if (ORTE_SUCCESS != (ret = orte_test_base_select())) {
          ORTE_ERROR_LOG(ret);
          error = "orte_test_base_select";
          goto error;
      }
    //==========================================

      //===========================urm===============
          if (ORTE_SUCCESS != (ret = orte_urm_base_open())) {
                ORTE_ERROR_LOG(ret);
                error = "orte_urm_base_open";
                goto error;
            }
            if (ORTE_SUCCESS != (ret = orte_urm_base_select())) {
                ORTE_ERROR_LOG(ret);
                error = "orte_urm_base_select";
                goto error;
            }
          //==========================================


    /* if we are an ORTE app - and not an MPI app - then
     * we need to barrier here. MPI_Init has its own barrier,
     * so we don't need to do two of them. However, if we
     * don't do a barrier at all, then one process could
     * finalize before another one called orte_init. This
     * causes ORTE to believe that the proc abnormally
     * terminated
     *
     * NOTE: only do this when the process originally launches.
     * Cannot do this on a restart as the rest of the processes
     * in the job won't be executing this step, so we would hang
     */
    if (ORTE_PROC_IS_NON_MPI && !orte_do_not_barrier) {
        orte_grpcomm_collective_t coll;
        OBJ_CONSTRUCT(&coll, orte_grpcomm_collective_t);
        coll.id = orte_process_info.peer_init_barrier;
        if (ORTE_SUCCESS != (ret = orte_grpcomm.barrier(&coll))) {
            ORTE_ERROR_LOG(ret);
            error = "orte barrier";
            goto error;
        }
        ORTE_WAIT_FOR_COMPLETION(coll.active);
        OBJ_DESTRUCT(&coll);
    }
    
    return ORTE_SUCCESS;
    
error:
    orte_show_help("help-orte-runtime.txt",
                   "orte_init:startup:internal-failure",
                   true, error, ORTE_ERROR_NAME(ret), ret);
    
    return ret;
}

int orte_ess_base_app_finalize(void)
{
    orte_dfs_base_close();
    orte_test_base_close();//jimmy
    orte_urm_base_close();//
    orte_cr_finalize();
    
#if OPAL_ENABLE_FT_CR == 1
    orte_snapc_base_close();
#endif
    orte_filem_base_close();
    
    orte_errmgr_base_close();

    /* now can close the rml and its friendly group comm */
    orte_grpcomm_base_close();
    orte_db_base_close();
    orte_routed_base_close();
    orte_rml_base_close();
    
    orte_session_dir_finalize(ORTE_PROC_MY_NAME);
        
    return ORTE_SUCCESS;    
}

/*
 * We do NOT call the regular C-library "abort" function, even
 * though that would have alerted us to the fact that this is
 * an abnormal termination, because it would automatically cause
 * a core file to be generated. On large systems, that can be
 * overwhelming (imagine a few thousand Gbyte-sized files hitting
                 * a shared file system simultaneously...ouch!).
 *
 * However, this causes a problem for OpenRTE as the system truly
 * needs to know that this actually IS an abnormal termination.
 * To get around the problem, we create a file in the session
 * directory - we don't need to put anything in it, though, as its
 * very existence simply alerts us that this was an abnormal
 * termination.
 *
 * The session directory finalize system will clean this file up
 * for us automagically. However, it needs to stick around long
 * enough for our local daemon to find it! So, we do NOT call
 * session_dir_finalize here!!! Someone will clean up for us.
 *
 * In some cases, however, we DON'T want to create that alert. For
 * example, if an orted detects that the HNP has died, then there
 * is truly nobody to alert! In these cases, we pass report=false
 * to prevent the abort file from being created. This allows the
 * session directory tree to cleanly be eliminated.
 */
static bool sync_waiting = false;

static void report_sync(int status, orte_process_name_t* sender,
                        opal_buffer_t *buffer,
                        orte_rml_tag_t tag, void *cbdata)
{
    /* flag as complete */
    sync_waiting = false;
}

void orte_ess_base_app_abort(int status, bool report)
{
    orte_daemon_cmd_flag_t cmd=ORTE_DAEMON_ABORT_CALLED;
    opal_buffer_t *buf;
    
    /* Exit - do NOT do a normal finalize as this will very likely
     * hang the process. We are aborting due to an abnormal condition
     * that precludes normal cleanup 
     *
     * We do need to do the following bits to make sure we leave a 
     * clean environment. Taken from orte_finalize():
     * - Assume errmgr cleans up child processes before we exit.
     */
    
    /* CRS cleanup since it may have a named pipe and thread active */
    orte_cr_finalize();
    
    /* If we were asked to report this termination, do so */
    if (report) {
        buf = OBJ_NEW(opal_buffer_t);
        opal_dss.pack(buf, &cmd, 1, ORTE_DAEMON_CMD);
        orte_rml.send_buffer_nb(ORTE_PROC_MY_DAEMON, buf, ORTE_RML_TAG_DAEMON, 0, orte_rml_send_callback, NULL);
        OPAL_OUTPUT_VERBOSE((5, orte_debug_output,
                             "%s orte_ess_app_abort: sent abort msg to %s",
                             ORTE_NAME_PRINT(ORTE_PROC_MY_NAME),
                             ORTE_NAME_PRINT(ORTE_PROC_MY_DAEMON)));
        /* get the ack - need this to ensure that the sync communication
         * gets serviced by the event library on the orted prior to the
         * process exiting
         */
        sync_waiting = true;
        if (ORTE_SUCCESS != orte_rml.recv_buffer_nb(ORTE_NAME_WILDCARD, ORTE_RML_TAG_ABORT,
                                                    ORTE_RML_NON_PERSISTENT, report_sync, NULL)) {
            exit(status);
        }
        ORTE_WAIT_FOR_COMPLETION(sync_waiting);
    }
    
    /* - Clean out the global structures 
     * (not really necessary, but good practice) */
    orte_proc_info_finalize();
    
    /* Now Exit */
    exit(status);
}
