/*
 * Copyright (c) 2012      Los Alamos National Security, LLC.
 *                         All rights reserved.
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

#include "orte/mca/dfs/dfs.h"
#include "orte/mca/dfs/base/base.h"
#include "dfs_orted.h"

/*
 * Public string for version number
 */
const char *orte_dfs_orted_component_version_string = 
    "ORTE DFS orted MCA component version " ORTE_VERSION;

int orte_dfs_orted_num_worker_threads = 0;

/*
 * Local functionality
 */
static int dfs_orted_open(void);
static int dfs_orted_close(void);
static int dfs_orted_component_query(mca_base_module_t **module, int *priority);

/*
 * Instantiate the public struct with all of our public information
 * and pointer to our public functions in it
 */
orte_dfs_base_component_t mca_dfs_orted_component =
{
    /* Handle the general mca_component_t struct containing 
     *  meta information about the component itdefault_orted
     */
    {
        ORTE_DFS_BASE_VERSION_1_0_0,
        /* Component name and version */
        "orted",
        ORTE_MAJOR_VERSION,
        ORTE_MINOR_VERSION,
        ORTE_RELEASE_VERSION,
        
        /* Component open and close functions */
        dfs_orted_open,
        dfs_orted_close,
        dfs_orted_component_query
    },
    {
        /* The component is checkpoint ready */
        MCA_BASE_METADATA_PARAM_CHECKPOINT
    }
};

static int dfs_orted_open(void) 
{
    mca_base_component_t *c = &mca_dfs_orted_component.base_version;

    mca_base_param_reg_int(c, "num_worker_threads",
                           "Number of worker threads to use for processing file requests",
                           false, false, 0, &orte_dfs_orted_num_worker_threads);
    return ORTE_SUCCESS;
}

static int dfs_orted_close(void)
{
    return ORTE_SUCCESS;
}

static int dfs_orted_component_query(mca_base_module_t **module, int *priority)
{
    if (ORTE_PROC_IS_DAEMON || ORTE_PROC_IS_HNP) {
        /* we are the default component for daemons and HNP */
        *priority = 1000;
        *module = (mca_base_module_t *)&orte_dfs_orted_module;
        return ORTE_SUCCESS;        
    }
    
    *priority = -1;
    *module = NULL;
    return ORTE_ERROR;
}

