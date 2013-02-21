/*
 * Copyright (c) 2012      Los Alamos National Security, Inc.  All rights reserved. 
 * $COPYRIGHT$
 * 
 * Additional copyrights may follow
 * 
 * $HEADER$
 */
/** @file:
 */

#ifndef ORTE_MCA_TEST_BASE_H
#define ORTE_MCA_TEST_BASE_H

/*
 * includes
 */
#include "orte_config.h"
#include "orte/types.h"
#include "orte/constants.h"

#include "opal/class/opal_list.h"
#include "opal/mca/event/event.h"

#include "opal/mca/mca.h"
#include "orte/mca/test/test.h"


BEGIN_C_DECLS

/*
 * MCA Framework functions
 */
ORTE_DECLSPEC    int orte_test_base_open(void);
ORTE_DECLSPEC    int orte_test_base_select(void);
ORTE_DECLSPEC    int orte_test_base_close(void);

/* define a struct to hold framework-global values */
typedef struct {
    int output;
    bool initialized;
    opal_list_t components_available;
} orte_test_base_t;

ORTE_DECLSPEC extern orte_test_base_t orte_test_base;

END_C_DECLS

#endif
