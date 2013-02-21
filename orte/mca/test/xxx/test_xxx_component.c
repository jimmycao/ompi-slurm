/*
 * Copyright (c) 2012      Los Alamos National Security, LLC. All rights reserved.
 *
 * $COPYRIGHT$
 * 
 * Additional copyrights may follow
 * 
 * $HEADER$
 */

#include "orte_config.h"
#include "opal/util/output.h"

#include "orte/runtime/orte_globals.h"

#include "orte/mca/test/test.h"
#include "orte/mca/test/base/base.h"
#include "test_xxx.h"

/*
 * Public string for version number
 */
const char *orte_test_xxx_component_version_string =
    "ORTE TEST xxx MCA component version " ORTE_VERSION;

/*
 * Local functionality
 */
static int test_xxx_open(void);
static int test_xxx_close(void);
static int test_xxx_component_query(mca_base_module_t **module, int *priority);

/*
 * Instantiate the public struct with all of our public information
 * and pointer to our public functions in it
 */
orte_test_base_component_t mca_test_xxx_component =
{
    /* Handle the general mca_component_t struct containing 
     *  meta information about the component
     */
    {
        ORTE_TEST_BASE_VERSION_1_0_0,
        /* Component name and version */
        "xxx",
        ORTE_MAJOR_VERSION,
        ORTE_MINOR_VERSION,
        ORTE_RELEASE_VERSION,
        
        /* Component open and close functions */
        test_xxx_open,
        test_xxx_close,
        test_xxx_component_query
    },
    {
        /* The component is checkpoint ready */
        MCA_BASE_METADATA_PARAM_CHECKPOINT
    },
};

static int test_xxx_open(void)
{
    return ORTE_SUCCESS;
}

static int test_xxx_close(void)
{
    return ORTE_SUCCESS;
}

static int test_xxx_component_query(mca_base_module_t **module, int *priority)
{
	    *priority = 50;
	    *module = (mca_base_module_t *) &orte_test_xxx_module;
	    return ORTE_SUCCESS;
}
