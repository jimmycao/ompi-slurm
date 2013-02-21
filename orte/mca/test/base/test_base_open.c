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
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#include "opal/mca/mca.h"
#include "opal/mca/base/base.h"
#include "opal/mca/base/mca_base_param.h"

#include "opal/util/opal_environ.h"
#include "opal/util/output.h"
#include "opal/util/trace.h"
#include "opal/util/output.h"

#include "orte/util/show_help.h"
#include "orte/mca/test/base/base.h"

#include "orte/mca/test/base/static-components.h"

/*
 * Globals
 */
opal_list_t orte_test_base_components_available;   // not used

orte_test_base_t orte_test_base;    //to be initialized when open

orte_test_base_module_t orte_test = {
    NULL,
    NULL,
    NULL
};

/**
 * Function for finding and opening either all MCA components, or the one
 * that was specifically requested via a MCA parameter.
 */
int orte_test_base_open(void)
{
    /* Only pass this way once */
    if( orte_test_base.initialized ) {
        return ORTE_SUCCESS;
    }

    orte_test_base.output = opal_output_open(NULL);

    /*
     * Open up all available components
     */
    if (ORTE_SUCCESS != 
        mca_base_components_open("test",
                                 orte_test_base.output,
                                 mca_test_base_static_components,
                                 &orte_test_base.components_available,
                                 true)) {
        return ORTE_ERROR;
    }
    
    orte_test_base.initialized = true;
    
    return ORTE_SUCCESS;
}
