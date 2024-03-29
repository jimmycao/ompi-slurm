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
 * Copyright (c) 2010      Los Alamos National Security, LLC.
 *                         All rights reserved.
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
#include "opal/mca/shmem/shmem.h"
#include "opal/mca/shmem/base/base.h"
#include "opal/util/output.h"

/* ////////////////////////////////////////////////////////////////////////// */
int
opal_shmem_base_close(void)
{
    /* if there is a selected shmem module, finalize it */
    if (NULL != opal_shmem_base_module &&
        NULL != opal_shmem_base_module->module_finalize) {
        opal_shmem_base_module->module_finalize();
    }

    /**
     * close all components that are still open (this should only
     *  happen during ompi_info).
     */
    if (opal_shmem_base_components_opened_valid) {
        mca_base_components_close(opal_shmem_base_output,
                                  &opal_shmem_base_components_opened, NULL);
        OBJ_DESTRUCT(&opal_shmem_base_components_opened);
        opal_shmem_base_components_opened_valid = false;
    }

    /* Close the framework output */
    opal_output_close (opal_shmem_base_output);
    opal_shmem_base_output = -1;

    /* all done */
    return OPAL_SUCCESS;
}

