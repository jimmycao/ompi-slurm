/*
 * Copyright (c) 2004-2005 The Trustees of Indiana University and Indiana
 *                         University Research and Technology
 *                         Corporation.  All rights reserved.
 * Copyright (c) 2004-2005 The University of Tennessee and The University
 *                         of Tennessee Research Foundation.  All rights
 *                         reserved.
 * Copyright (c) 2004-2005 High Performance Computing Center Stuttgart, 
 *                         University of Stuttgart.  All rights reserved.
 * Copyright (c) 2004-2005 The Regents of the University of California.
 *                         All rights reserved.
 * $COPYRIGHT$
 * 
 * Additional copyrights may follow
 * 
 * $HEADER$
 */

#include "orte_config.h"

#include <stdio.h>

#include "opal/mca/mca.h"
#include "opal/mca/base/base.h"
#include "opal/mca/hwloc/hwloc.h"

#include "orte/mca/grpcomm/base/base.h"


int orte_grpcomm_base_close(void)
{
  /* If we have a selected component and module, then finalize it */

  if (orte_grpcomm_base.selected) {
    orte_grpcomm.finalize();
  }

    /* Close all remaining available components (may be one if this is a
     OpenRTE program, or [possibly] multiple if this is ompi_info) */

  mca_base_components_close(orte_grpcomm_base.output, 
                            &orte_grpcomm_base.components_available, NULL);

#if OPAL_HAVE_HWLOC
  if (NULL != orte_grpcomm_base.working_cpuset) {
      hwloc_bitmap_free(orte_grpcomm_base.working_cpuset);
      orte_grpcomm_base.working_cpuset = NULL;
  }
#endif

  /* Close the framework output */
  opal_output_close (orte_grpcomm_base.output);
  orte_grpcomm_base.output = -1;

  /* All done */

  return ORTE_SUCCESS;
}
