/*
 * Copyright (c) 2010      Cisco Systems, Inc.  All rights reserved. 
 * Copyright (c) 2012      Los Alamos National Security, Inc.  All rights reserved. 
 * $COPYRIGHT$
 * 
 * Additional copyrights may follow
 * 
 * $HEADER$
 */

#include "orte_config.h"
#include "orte/constants.h"

#include "opal/class/opal_list.h"
#include "opal/mca/mca.h"
#include "opal/mca/base/base.h"
#include "opal/mca/base/mca_base_component_repository.h"

#include "orte/util/name_fns.h"
#include "orte/mca/errmgr/errmgr.h"

#include "orte/mca/db/base/base.h"

static bool selected = false;

int 
orte_db_base_select(void)
{
    opal_list_item_t *item, *itm2;
    mca_base_component_list_item_t *cli = NULL;
    mca_base_component_t *component = NULL;
    mca_base_module_t *module = NULL;
    orte_db_base_module_t *nmodule;
    orte_db_active_module_t *newmodule, *mod;
    int rc, priority;
    bool inserted;

    if (selected) {
        /* ensure we don't do this twice */
        return ORTE_SUCCESS;
    }
    selected = true;
    
    /* Query all available components and ask if they have a module */
    for (item = opal_list_get_first(&orte_db_base.available_components);
         opal_list_get_end(&orte_db_base.available_components) != item;
         item = opal_list_get_next(item)) {
        cli = (mca_base_component_list_item_t *) item;
        component = (mca_base_component_t *) cli->cli_component;

        opal_output_verbose(5, orte_db_base.output,
                            "mca:db:select: checking available component %s", component->mca_component_name);

        /* If there's no query function, skip it */
        if (NULL == component->mca_query_component) {
            opal_output_verbose(5, orte_db_base.output,
                                "mca:db:select: Skipping component [%s]. It does not implement a query function",
                                component->mca_component_name );
            continue;
        }

        /* Query the component */
        opal_output_verbose(5, orte_db_base.output,
                            "mca:db:select: Querying component [%s]",
                            component->mca_component_name);
        rc = component->mca_query_component(&module, &priority);

        /* If no module was returned, then skip component */
        if (ORTE_SUCCESS != rc || NULL == module) {
            opal_output_verbose(5, orte_db_base.output,
                                "mca:db:select: Skipping component [%s]. Query failed to return a module",
                                component->mca_component_name );
            continue;
        }

        nmodule = (orte_db_base_module_t*) module;
        /* attempt to initialize the module */
        if (NULL != nmodule->init) {
            if (ORTE_SUCCESS != (rc = nmodule->init())) {
                /* skip the module */
                continue;
            }
        }

        /* If we got a module, add to the list of selected modules */
        newmodule = OBJ_NEW(orte_db_active_module_t);
        newmodule->pri = priority;
        newmodule->module = nmodule;
        newmodule->component = component;

        /* maintain priority order */
        inserted = false;
        for (itm2 = opal_list_get_first(&orte_db_base.active_modules);
             itm2 != opal_list_get_end(&orte_db_base.active_modules);
             itm2 = opal_list_get_next(itm2)) {
            mod = (orte_db_active_module_t*)itm2;
            if (priority > mod->pri) {
                opal_list_insert_pos(&orte_db_base.active_modules,
                                     itm2, &newmodule->super);
                inserted = true;
                break;
            }
        }
        if (!inserted) {
            /* must be lowest priority - add to end */
            opal_list_append(&orte_db_base.active_modules, &newmodule->super);
        }
    }

    if (4 < opal_output_get_verbosity(orte_db_base.output)) {
        opal_output(0, "%s: Final db priorities", ORTE_NAME_PRINT(ORTE_PROC_MY_NAME));
        /* show the prioritized list */
        for (itm2 = opal_list_get_first(&orte_db_base.active_modules);
             itm2 != opal_list_get_end(&orte_db_base.active_modules);
             itm2 = opal_list_get_next(itm2)) {
            mod = (orte_db_active_module_t*)itm2;
            opal_output(0, "\tComponent: %s Priority: %d", mod->component->mca_component_name, mod->pri);
        }
    }

    return ORTE_SUCCESS;;
}
