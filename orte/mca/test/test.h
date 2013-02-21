/*
 * Copyright (c) 2012      Los Alamos National Security, LLC.
 *                         All rights reserved.
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 */

#ifndef ORTE_MCA_TEST_H
#define ORTE_MCA_TEST_H

#include "orte_config.h"
#include "orte/types.h"

#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif

#include "opal/mca/mca.h"
#include "opal/mca/base/base.h"

#include "orte/mca/test/test_types.h"

BEGIN_C_DECLS

/*
 * Framework Interfaces
 */
/**
 * Module initialization function.
 *
 * @retval ORTE_SUCCESS The operation completed successfully
 * @retval ORTE_ERROR   An unspecifed error occurred
 */
typedef int (*orte_test_base_module_init_fn_t)(void);

/**
 * Module finalization function.
 *
 * @retval ORTE_SUCCESS The operation completed successfully
 * @retval ORTE_ERROR   An unspecifed error occurred
 */
typedef int (*orte_test_base_module_finalize_fn_t)(void);


typedef void (*orte_test_base_module_print_fn_t)(char *in);


/*
 * Module Structure
 */
struct orte_test_base_module_1_0_0_t {
    /** Initialization Function */
    orte_test_base_module_init_fn_t             init;
    /** Finalization Function */
    orte_test_base_module_finalize_fn_t         finalize;

    orte_test_base_module_print_fn_t             print;
};
typedef struct orte_test_base_module_1_0_0_t orte_test_base_module_1_0_0_t;
typedef orte_test_base_module_1_0_0_t orte_test_base_module_t;

//THE REAL HANDLER, defined in base/test_base_open.c
ORTE_DECLSPEC extern orte_test_base_module_t orte_test;

/*
 * TEST Component
 */
struct orte_test_base_component_1_0_0_t {
    /** MCA base component */
    mca_base_component_t base_version;
    /** MCA base data */
    mca_base_component_data_t base_data;
};
typedef struct orte_test_base_component_1_0_0_t orte_test_base_component_1_0_0_t;
typedef orte_test_base_component_1_0_0_t orte_test_base_component_t;

/*
 * Macro for use in components that are of type errmgr
 */
#define ORTE_TEST_BASE_VERSION_1_0_0 \
  MCA_BASE_VERSION_2_0_0, \
  "test", 1, 0, 0

END_C_DECLS

#endif
