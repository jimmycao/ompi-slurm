/*
 * Copyright (c) 2004-2007 The Trustees of Indiana University and Indiana
 *                         University Research and Technology
 *                         Corporation.  All rights reserved.
 * Copyright (c) 2004-2007 The University of Tennessee and The University
 *                         of Tennessee Research Foundation.  All rights
 *                         reserved.
 * Copyright (c) 2004-2005 High Performance Computing Center Stuttgart, 
 *                         University of Stuttgart.  All rights reserved.
 * Copyright (c) 2004-2005 The Regents of the University of California.
 *                         All rights reserved.
 * Copyright (c) 2008-2009 Cisco Systems, Inc.  All rights reserved.
 * $COPYRIGHT$
 * 
 * Additional copyrights may follow
 * 
 * $HEADER$
 */

#include "ompi_config.h"

#include <stdio.h>

#include "ompi/constants.h"
#include "opal/mca/event/event.h"
#include "opal/mca/mca.h"
#include "opal/mca/base/base.h"
#include "ompi/mca/btl/base/base.h"

int mca_btl_base_close(void)
{
    opal_list_item_t *item;
    mca_btl_base_selected_module_t *sm;

    if( mca_btl_base_already_opened <= 0 ) {
        return OMPI_ERROR;
    } else if (--mca_btl_base_already_opened > 0) {
        return OMPI_SUCCESS;
    }
#if 0
    /* disable event processing while cleaning up btls */
    opal_event_disable();
#endif
    /* Finalize all the btl components and free their list items */

    for (item = opal_list_remove_first(&mca_btl_base_modules_initialized);
         NULL != item; 
         item = opal_list_remove_first(&mca_btl_base_modules_initialized)) {
        sm = (mca_btl_base_selected_module_t *) item;

        /* Blatebtly ignore the return code (what would we do to recover,
           anyway?  This component is going away, so errors don't matter
           anymore) */

        sm->btl_module->btl_finalize(sm->btl_module);
        free(sm);
    }

    /* Close all remaining opened components (may be one if this is a
       OMPI RTE program, or [possibly] multiple if this is ompi_info) */

    mca_base_components_close(mca_btl_base_output,
                              &mca_btl_base_components_opened, NULL);

    /* Close the framework output */
    opal_output_close (mca_btl_base_output);
    mca_btl_base_output = -1;

    /* cleanup */
    if(NULL != mca_btl_base_include)
        free(mca_btl_base_include);
    if(NULL != mca_btl_base_exclude)
        free(mca_btl_base_exclude);

#if 0
    /* restore event processing */
    opal_event_enable();
#endif
    /* All done */
    return OMPI_SUCCESS;
}
