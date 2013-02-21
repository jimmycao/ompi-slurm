/* -*- C -*-
 *
 * $HEADER$
 *
 * The most basic of MPI applications
 */

#include <stdio.h>
#include <stdlib.h>

#include "opal/util/argv.h"

//#include "orte/util/proc_info.h"
//#include "orte/mca/plm/plm.h"
//#include "orte/mca/rml/rml.h"
//#include "orte/mca/errmgr/errmgr.h"
//#include "orte/runtime/runtime.h"
//#include "orte/runtime/orte_globals.h"
#include "orte/util/name_fns.h"

#include "orte/mca/test/test.h"

//export OMPI_MCA_test=xxx

#define ORTE_PROC_NON_MPI       0x0010

int main(int argc, char* argv[])
{
    int rc;

    setenv("OMPI_MCA_test_base_verbose", "10", 1);
    printf("OMPI_MCA_test_base_verbose = %s\n", getenv("OMPI_MCA_test_base_verbose"));
#if 0   //if under ralph's new version ras/slurm
    setenv("OMPI_MCA_ras_slurm_config_file", "/usr/local/etc/slurm.conf", 1);
    printf("OMPI_MCA_ras_slurm_config_file = %s\n", getenv("OMPI_MCA_ras_slurm_config_file"));
#endif
#if 0
    setenv("OMPI_MCA_test", "xxx", 1);
    printf("OMPI_MCA_test = %s\n", getenv("OMPI_MCA_test"));
#endif

    if (0 > (rc = orte_init(&argc, &argv, ORTE_PROC_NON_MPI))) {
        fprintf(stderr, "couldn't init orte - error code %d\n", rc);
        return rc;
    }

	char *in = "jimmy";
	orte_test.print(in);


    /* All done */
    orte_finalize();
    return 0;
}
