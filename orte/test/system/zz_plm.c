/* -*- C -*-
 *
 * $HEADER$
 *
 * The most basic of MPI applications
 */

#include <stdio.h>

#include "opal/util/argv.h"

#include "orte/util/proc_info.h"
#include "orte/mca/plm/plm.h"
#include "orte/mca/rml/rml.h"
#include "orte/mca/errmgr/errmgr.h"
#include "orte/runtime/runtime.h"
#include "orte/runtime/orte_globals.h"
#include "orte/util/name_fns.h"

#define MY_TAG 12345

int main(int argc, char* argv[])
{
    int rc;
//    orte_job_t *jdata;
//    orte_app_context_t *app;
//    char cwd[1024];
//    orte_process_name_t name;
//    struct iovec msg;
//    orte_vpid_t i;

    
    if (0 > (rc = orte_init(&argc, &argv, ORTE_PROC_NON_MPI))) {
        fprintf(stderr, "couldn't init orte - error code %d\n", rc);
        return rc;
    }
    /* launch the job */
    fprintf(stderr, "Parent: spawning children!\n");
    if (ORTE_SUCCESS != (rc = orte_plm.spawn(NULL))) {
        ORTE_ERROR_LOG(rc);
        orte_finalize();
        return 1;
    }
    fprintf(stderr, "Parent: children spawned!\n");


    /* All done */
    orte_finalize();
    return 0;
}
