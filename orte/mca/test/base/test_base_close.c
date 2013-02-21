/*
 * Copyright (c) 2012      Los Alamos National Security, Inc.  All rights reserved. 
 * $COPYRIGHT$
 * 
 * Additional copyrights may follow
 * 
 * $HEADER$
 */

#include "orte_config.h"
#include "orte/constants.h"

#include <stdio.h>

#include "opal/mca/mca.h"
#include "opal/mca/base/base.h"

#include "orte/mca/test/test.h"
#include "orte/mca/test/base/base.h"


int orte_test_base_close(void)
{
    /* if not initialized, then skip this action. */
    if (!orte_test_base.initialized) {
        return ORTE_SUCCESS;
    }

    /* Close selected component */
    if (NULL != orte_test.finalize) {
        orte_test.finalize();
    }

    /* Close all remaining available components (may be one if this is a
     * OMPI RTE program, or [possibly] multiple if this is ompi_info)
     */
    mca_base_components_close(orte_test_base.output,
                              &orte_test_base.components_available,
                              NULL);

    /* Close the framework output */
    opal_output_close (orte_test_base.output);
    orte_test_base.output = -1;

    orte_test_base.initialized = false;
    
    return ORTE_SUCCESS;
}
