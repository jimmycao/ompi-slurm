/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2004-2010 The Trustees of Indiana University and Indiana
 *                         University Research and Technology
 *                         Corporation.  All rights reserved.
 * Copyright (c) 2004-2005 The University of Tennessee and The University
 *                         of Tennessee Research Foundation.  All rights
 *                         reserved.
 * Copyright (c) 2004-2005 High Performance Computing Center Stuttgart, 
 *                         University of Stuttgart.  All rights reserved.
 * Copyright (c) 2004-2005 The Regents of the University of California.
 *                         All rights reserved.
 * Copyright (c) 2006-2012 Cisco Systems, Inc.  All rights reserved.
 * Copyright (c) 2010-2012 Los Alamos National Security, LLC.
 *                         All rights reserved.
 * $COPYRIGHT$
 * 
 * Additional copyrights may follow
 * 
 * $HEADER$
 */

#include "orte_config.h"

#include <stdlib.h>
#include <string.h>

#include "orte/runtime/runtime.h"

#include "opal/util/argv.h"
#include "opal/runtime/opal_info_support.h"
#include "opal/mca/event/base/base.h"
#include "opal/util/output.h"

#include "orte/runtime/orte_info_support.h"
#include "orte/tools/orte-info/orte-info.h"
/*
 * Public variables
 */

static void component_map_construct(orte_info_component_map_t *map)
{
    map->type = NULL;
}
static void component_map_destruct(orte_info_component_map_t *map)
{
    if (NULL != map->type) {
        free(map->type);
    }
    /* the type close functions will release the
     * list of components
     */
}
OBJ_CLASS_INSTANCE(orte_info_component_map_t,
                   opal_list_item_t,
                   component_map_construct,
                   component_map_destruct);

opal_pointer_array_t component_map;

/*
 * Private variables
 */

static bool opened_components = false;


/*
 * Open all MCA components so that they can register their MCA
 * parameters.  Take a shotgun approach here and indiscriminately open
 * all components -- don't be selective.  To this end, we need to clear
 * out the environment of all OMPI_MCA_<type> variables to ensure
 * that the open algorithms don't try to only open one component.
 */
void orte_info_components_open(void)
{
    if (opened_components) {
        return;
    }

    opened_components = true;

    /* init the map */
    OBJ_CONSTRUCT(&component_map, opal_pointer_array_t);
    opal_pointer_array_init(&component_map, 256, INT_MAX, 128);

    opal_info_register_components (&mca_types, &component_map);
    orte_info_register_components (&mca_types, &component_map);
}

/* 
 * Not to be confused with orte_info_close_components.
 */
void orte_info_components_close(void)
{
    int i;
    orte_info_component_map_t *map;

    if (!opened_components) {
        return;
    }

    orte_info_close_components ();
    opal_info_close_components ();
    
    for (i=0; i < component_map.size; i++) {
        if (NULL != (map = (orte_info_component_map_t*)opal_pointer_array_get_item(&component_map, i))) {
            OBJ_RELEASE(map);
        }
    }

    OBJ_DESTRUCT(&component_map);
    
    opened_components = false;
}
