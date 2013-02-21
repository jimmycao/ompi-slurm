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

#include "orte/mca/urm/urm.h"
#include "orte/mca/urm/base/base.h"


int orte_urm_base_close(void)
{
    /* if not initialized, then skip this action. */
    if (!orte_urm_base.initialized) {
        return ORTE_SUCCESS;
    }

    /* Close selected component */
    if (NULL != orte_urm.finalize) {
        orte_urm.finalize();
    }

    /* Close all remaining available components (may be one if this is a
     * OMPI RTE program, or [possibly] multiple if this is ompi_info)
     */
    mca_base_components_close(orte_urm_base.output,
                              &orte_urm_base.components_available,
                              NULL);

    /* Close the framework output */
    opal_output_close (orte_urm_base.output);
    orte_urm_base.output = -1;

    orte_urm_base.initialized = false;
    
    return ORTE_SUCCESS;
}
