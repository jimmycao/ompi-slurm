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

#include "orte/mca/urm/urm.h"
#include "orte/mca/urm/base/base.h"
#include "urm_slurm.h"

/*
 * Public string for version number
 */
const char *orte_urm_slurm_component_version_string =
    "ORTE URM slurm MCA component version " ORTE_VERSION;

/*
 * Local functionality
 */
static int urm_slurm_open(void);
static int urm_slurm_close(void);
static int urm_slurm_component_query(mca_base_module_t **module, int *priority);

/*
 * Instantiate the public struct with all of our public information
 * and pointer to our public functions in it
 */
orte_urm_slurm_component_t mca_urm_slurm_component =
{
    /* Handle the general mca_component_t struct containing 
     *  meta information about the component
     */
    {
        ORTE_URM_BASE_VERSION_1_0_0,
        /* Component name and version */
        "slurm",
        ORTE_MAJOR_VERSION,
        ORTE_MINOR_VERSION,
        ORTE_RELEASE_VERSION,
        
        /* Component open and close functions */
        urm_slurm_open,
        urm_slurm_close,
        urm_slurm_component_query
    },
    {
        /* The component is checkpoint ready */
        MCA_BASE_METADATA_PARAM_CHECKPOINT
    },
};

static int urm_slurm_open(void)
{
	 mca_base_param_reg_int(&mca_urm_slurm_component.super.base_version,
	                           "dyn_allocate_timeout",
	                           "Number of seconds to wait for Slurm dynamic allocation",
	                           false, false, 30, &mca_urm_slurm_component.timeout);

	 mca_base_param_reg_string(&mca_urm_slurm_component.super.base_version,
	                               "config_file",
	                               "Path to Slurm configuration file",
	                               false, false, NULL, &mca_urm_slurm_component.config_file);

    return ORTE_SUCCESS;
}

static int urm_slurm_close(void)
{
	if (NULL != mca_urm_slurm_component.config_file) {
	        free(mca_urm_slurm_component.config_file);
	}
    return ORTE_SUCCESS;
}

static int urm_slurm_component_query(mca_base_module_t **module, int *priority)
{
	*priority = 50;
	*module = (mca_base_module_t *) &orte_urm_slurm_module;
	return ORTE_SUCCESS;
}
