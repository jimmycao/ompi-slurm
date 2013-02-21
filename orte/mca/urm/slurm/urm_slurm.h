/*
 * Copyright (c) 2012      Los Alamos National Security, LLC.  All rights reserved.
 *
 * $COPYRIGHT$
 * 
 * Additional copyrights may follow
 * 
 * $HEADER$
 */

/**
 * @file
 * 
 */

#ifndef MCA_URM_SLURM_EXPORT_H
#define MCA_URM_SLURM_EXPORT_H

#include "orte_config.h"

#include "orte/mca/urm/urm.h"

BEGIN_C_DECLS

/*
 * Local Component structures
 */
typedef struct {
	orte_urm_base_component_t super;
    int 	timeout;
    char 	*config_file;
    bool 	rolling_alloc;
} orte_urm_slurm_component_t;

ORTE_DECLSPEC extern orte_urm_slurm_component_t mca_urm_slurm_component;

ORTE_DECLSPEC extern orte_urm_base_module_t orte_urm_slurm_module;

END_C_DECLS

#endif /* MCA_dfs_slurm_EXPORT_H */
