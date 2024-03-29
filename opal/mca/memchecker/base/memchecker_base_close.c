/*
 * Copyright (c) 2004-2007 High Performance Computing Center Stuttgart, 
 *                         University of Stuttgart.  All rights reserved.
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
#include "opal/mca/memchecker/base/base.h"
#include "opal/mca/memchecker/memchecker.h"
#include "opal/util/output.h"

int opal_memchecker_base_close(void)
{
    /* Close all components that are still open (this should only
       happen during laminfo). */
    mca_base_components_close(opal_memchecker_base_output,
                              &opal_memchecker_base_components_opened, NULL);
    OBJ_DESTRUCT(&opal_memchecker_base_components_opened);

    /* Close the framework output */
    opal_output_close (opal_memchecker_base_output);
    opal_memchecker_base_output = -1;

    /* All done */
    return OPAL_SUCCESS;
}
