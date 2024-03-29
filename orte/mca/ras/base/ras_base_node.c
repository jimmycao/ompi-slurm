/*
 * Copyright (c) 2004-2005 The Trustees of Indiana University and Indiana
 *                         University Research and Technology
 *                         Corporation.  All rights reserved.
 * Copyright (c) 2004-2008 The University of Tennessee and The University
 *                         of Tennessee Research Foundation.  All rights
 *                         reserved.
 * Copyright (c) 2004-2005 High Performance Computing Center Stuttgart,
 *                         University of Stuttgart.  All rights reserved.
 * Copyright (c) 2004-2005 The Regents of the University of California.
 *                         All rights reserved.
 * Copyright (c) 2011-2012 Los Alamos National Security, LLC.  All rights
 *                         reserved. 
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 */

#include "orte_config.h"
#include "orte/constants.h"

#include <string.h>

#include "opal/util/argv.h"
#include "opal/util/if.h"

#include "orte/mca/errmgr/errmgr.h"
#include "orte/util/name_fns.h"
#include "orte/runtime/orte_globals.h"

#include "orte/mca/ras/base/ras_private.h"

static void orte_ras_base_proc_construct(orte_ras_proc_t* proc)
{
    proc->node_name = NULL;
    proc->cpu_list = NULL;
    proc->rank = ORTE_VPID_MAX;
}

static void orte_ras_base_proc_destruct(orte_ras_proc_t* proc)
{
    if (NULL != proc->node_name) {
        free(proc->node_name);
    }
    if (NULL != proc->cpu_list) {
        free(proc->cpu_list);
    }
}


OBJ_CLASS_INSTANCE(
    orte_ras_proc_t,
    opal_list_item_t,
    orte_ras_base_proc_construct,
    orte_ras_base_proc_destruct);


/*
 * Add the specified node definitions to the global data store
 * NOTE: this removes all items from the list!
 */
int orte_ras_base_node_insert(opal_list_t* nodes, orte_job_t *jdata)
{
    opal_list_item_t* item;
    orte_std_cntr_t num_nodes;
    int rc, i;
    orte_node_t *node, *hnp_node;
    char *ptr;
    bool hnp_alone = true;

    /* get the number of nodes */
    num_nodes = (orte_std_cntr_t)opal_list_get_size(nodes);
    if (0 == num_nodes) {
        return ORTE_SUCCESS;  /* nothing to do */
    }
    
    OPAL_OUTPUT_VERBOSE((5, orte_ras_base.ras_output,
                         "%s ras:base:node_insert inserting %ld nodes",
                         ORTE_NAME_PRINT(ORTE_PROC_MY_NAME),
                         (long)num_nodes));
    
    /* set the size of the global array - this helps minimize time
     * spent doing realloc's
     */
    if (ORTE_SUCCESS != (rc = opal_pointer_array_set_size(orte_node_pool, num_nodes))) {
        ORTE_ERROR_LOG(rc);
        return rc;
    }
    
    /* get the hnp node's info */
    hnp_node = (orte_node_t*)opal_pointer_array_get_item(orte_node_pool, 0);

    /* cycle through the list */
    while (NULL != (item = opal_list_remove_first(nodes))) {
        node = (orte_node_t*)item;
        
        /* the HNP had to already enter its node on the array - that entry is in the
         * first position since it is the first one entered. We need to check to see
         * if this node is the same as the HNP's node so we don't double-enter it
         */
        if (NULL != hnp_node &&
            (0 == strcmp(node->name, hnp_node->name) ||
             0 == strcmp(node->name, "localhost") ||
             opal_ifislocal(node->name))) {
            OPAL_OUTPUT_VERBOSE((5, orte_ras_base.ras_output,
                                 "%s ras:base:node_insert updating HNP [%s] info to %ld slots",
                                 ORTE_NAME_PRINT(ORTE_PROC_MY_NAME),
                                 node->name,
                                 (long)node->slots));

            /* flag that hnp has been allocated */
            orte_hnp_is_allocated = true;
            /* update the total slots in the job */
            orte_ras_base.total_slots_alloc += node->slots;
            /* copy the allocation data to that node's info */
            hnp_node->slots += node->slots;
            hnp_node->slots_max = node->slots_max;
            hnp_node->launch_id = node->launch_id;
            if (orte_managed_allocation) {
                /* the slots are always treated as sacred
                 * in managed allocations
                 */
                hnp_node->slots_given = true;
            } else {
                /* in unmanaged allocations, take whatever
                 * was determined by the hostfile or dash-host
                 * parsers
                 */
                hnp_node->slots_given = node->slots_given;
            }
            /* use the local name for our node - don't trust what
             * we got from an RM. If requested, store the resolved
             * nodename info
             */
            if (orte_show_resolved_nodenames) {
                /* if the node name is different, store it as an alias */
                if (0 != strcmp(node->name, hnp_node->name)) {
                    /* add to list of aliases for this node - only add if unique */
                    opal_argv_append_unique_nosize(&hnp_node->alias, node->name, false);
                }
                if (NULL != node->alias) {
                    /* now copy over any aliases that are unique */
                    for (i=0; NULL != node->alias[i]; i++) {
                        opal_argv_append_unique_nosize(&hnp_node->alias, node->alias[i], false);
                    }
                }
            }
            /* don't keep duplicate copy */
            OBJ_RELEASE(node);
        } else {
            /* insert the object onto the orte_nodes global array */
            OPAL_OUTPUT_VERBOSE((5, orte_ras_base.ras_output,
                                 "%s ras:base:node_insert node %s",
                                 ORTE_NAME_PRINT(ORTE_PROC_MY_NAME),
                                 (NULL == node->name) ? "NULL" : node->name));
            if (orte_managed_allocation) {
                /* the slots are always treated as sacred
                 * in managed allocations
                 */
                node->slots_given = true;
            }
            /* insert it into the array */
            node->index = opal_pointer_array_add(orte_node_pool, (void*)node);
            if (ORTE_SUCCESS > (rc = node->index)) {
                ORTE_ERROR_LOG(rc);
                return rc;
            }
            /* update the total slots in the job */
            orte_ras_base.total_slots_alloc += node->slots;
            /* check if we have fqdn names in the allocation */
            if (NULL != strchr(node->name, '.')) {
                orte_have_fqdn_allocation = true;
            }
            /* indicate the HNP is not alone */
            hnp_alone = false;
        }
    }

    /* if we didn't find any fqdn names in the allocation, then
     * ensure we don't have any domain info in the node record
     * for the hnp
     */
    if (!orte_have_fqdn_allocation && !hnp_alone) {
        if (NULL != (ptr = strchr(hnp_node->name, '.'))) {
            *ptr = '\0';
        }
    }
    
    return ORTE_SUCCESS;
}
