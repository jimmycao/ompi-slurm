/*
 * Copyright (c) 2004-2007 The Trustees of the University of Tennessee.
 *                         All rights reserved.
 * Copyright (c) 2009      Cisco Systems, Inc.  All rights reserved.
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 */

#include "ompi_config.h"
#include "base.h"
#include "opal/mca/mca.h"
#include "opal/mca/base/base.h"
#include "ompi/mca/vprotocol/base/static-components.h"

opal_list_t mca_vprotocol_base_components_available;
char *mca_vprotocol_base_include_list;
mca_pml_v_t mca_pml_v = {-1, 0, 0};

/* Load any vprotocol MCA component and call open function of all those 
 * components.
 * 
 * Also fill the mca_vprotocol_base_include_list with components that exists
 */
int mca_vprotocol_base_open(char *vprotocol_include_list)
{
    OBJ_CONSTRUCT(&mca_vprotocol_base_components_available, opal_list_t);
    if (NULL == vprotocol_include_list ||
        vprotocol_include_list[0] == 0) {
        return OMPI_SUCCESS;
    }
    if (NULL != vprotocol_include_list) {
      mca_vprotocol_base_include_list = strdup (vprotocol_include_list);
    } else {
      mca_vprotocol_base_include_list = NULL;
    }

    return mca_base_components_open("vprotocol", 0, 
                                    mca_vprotocol_base_static_components, 
                                    &mca_vprotocol_base_components_available, 
                                    true);
}

/* Close and unload any vprotocol MCA component loaded.
 */
int mca_vprotocol_base_close(void)
{
    int ret;
    ret = mca_base_components_close(mca_pml_v.output, 
                                    &mca_vprotocol_base_components_available, 
                                    NULL);
    if (NULL != mca_vprotocol_base_include_list) {
        free(mca_vprotocol_base_include_list);
    }
    OBJ_DESTRUCT(&mca_vprotocol_base_components_available);
    return ret;
}
