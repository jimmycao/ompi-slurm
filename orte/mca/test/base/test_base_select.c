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

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#include "opal/mca/mca.h"
#include "opal/mca/base/base.h"
#include "opal/mca/base/mca_base_param.h"
#include "opal/util/output.h"

#include "orte/mca/test/base/base.h"
//select the best component from those candidates obtained from test_base_open.c
//then save the winner to HANDLER (orte_test)

int orte_test_base_select(void)
{
    orte_test_base_component_t *best_component = NULL;
    orte_test_base_module_t *best_module = NULL;

    /*
     * Select the best component
     */
    if (OPAL_SUCCESS != mca_base_select("test", orte_test_base.output,
                                        &orte_test_base.components_available,
                                        (mca_base_module_t **) &best_module,
                                        (mca_base_component_t **) &best_component)) {
        /* This will only happen if no component was selected, which
         * is okay - we don't have to select anything
         *
         */
        return ORTE_SUCCESS;
    }

    /* Save the winner */
    orte_test = *best_module;

    return ORTE_SUCCESS;
}
