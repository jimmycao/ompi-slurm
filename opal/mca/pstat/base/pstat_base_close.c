/*
 * Copyright (c) 2004-2005 The Trustees of Indiana University and Indiana
 *                         University Research and Technology
 *                         Corporation.  All rights reserved.
 * Copyright (c) 2004-2006 The University of Tennessee and The University
 *                         of Tennessee Research Foundation.  All rights
 *                         reserved.
 * Copyright (c) 2004-2005 High Performance Computing Center Stuttgart, 
 *                         University of Stuttgart.  All rights reserved.
 * Copyright (c) 2004-2005 The Regents of the University of California.
 *                         All rights reserved.
 * Copyright (c) 2007      Cisco Systems, Inc.  All rights reserved.
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
#include "opal/mca/pstat/pstat.h"
#include "opal/mca/pstat/base/base.h"
#include "opal/util/output.h"

int opal_pstat_base_close(void)
{
    /* Close all components that are still open (this should only
     happen during ompi_info). */
    
    mca_base_components_close(opal_pstat_base_output,
                              &opal_pstat_base_components_opened, NULL);
    OBJ_DESTRUCT(&opal_pstat_base_components_opened);

    /* Close the framework output */
    opal_output_close (opal_pstat_base_output);
    opal_pstat_base_output = -1;
    
    /* All done */
    
    return OPAL_SUCCESS;
}
