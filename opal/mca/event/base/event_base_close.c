/*
 * Copyright (c) 2010      Cisco Systems, Inc.  All rights reserved.
 * $COPYRIGHT$
 * 
 * Additional copyrights may follow
 * 
 * $HEADER$
 */

#include "opal_config.h"

#include "opal/constants.h"
#include "opal/mca/mca.h"
#include "opal/mca/base/base.h"
#include "opal/mca/event/event.h"
#include "opal/mca/event/base/base.h"

int opal_event_base_close(void)
{
    opal_list_item_t *item;

    opal_event_base_inited--;

    /* no need to close the component as it was statically opened */

    /* for support of tools such as ompi_info */
    for (item = opal_list_remove_first(&opal_event_components);
         NULL != item; 
         item = opal_list_remove_first(&opal_event_components)) {
        OBJ_RELEASE(item);
    }
    OBJ_DESTRUCT(&opal_event_components);

    /* Close the framework output */
    opal_output_close (opal_event_base_output);
    opal_event_base_output = -1;

    /* All done */
    return OPAL_SUCCESS;
}
