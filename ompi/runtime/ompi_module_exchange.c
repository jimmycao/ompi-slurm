/*
 * Copyright (c) 2004-2005 The Trustees of Indiana University and Indiana
 *                         University Research and Technology
 *                         Corporation.  All rights reserved.
 * Copyright (c) 2004-2011 The University of Tennessee and The University
 *                         of Tennessee Research Foundation.  All rights
 *                         reserved.
 * Copyright (c) 2004-2005 High Performance Computing Center Stuttgart,
 *                         University of Stuttgart.  All rights reserved.
 * Copyright (c) 2004-2005 The Regents of the University of California.
 *                         All rights reserved.
 * Copyright (c) 2006-2012 Los Alamos National Security, LLC.  All rights
 *                         reserved.
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 */

#include "ompi_config.h"
#include "ompi/constants.h"

#include "opal/mca/mca.h"
#include "opal/mca/base/base.h"
#include "opal/dss/dss.h"

#include "orte/mca/db/db.h"

#include "orte/util/name_fns.h"
#include "orte/runtime/orte_globals.h"

#include "ompi/proc/proc.h"
#include "ompi/runtime/ompi_module_exchange.h"


int ompi_modex_send(const mca_base_component_t *source_component,
                    const void *data, size_t size)
{
    int rc;
    char *key;
    opal_byte_object_t bo;

    key = mca_base_component_to_string(source_component);
    if (NULL == key) {
        return OMPI_ERR_OUT_OF_RESOURCE;
    }

    bo.bytes = (uint8_t *)data;
    bo.size = size;

    /* the store API makes a copy of the provided data */
    rc = orte_db.store(ORTE_PROC_MY_NAME, key, &bo, OPAL_BYTE_OBJECT);
    free(key);
    return rc;
}

int
ompi_modex_recv(const mca_base_component_t *component,
                const ompi_proc_t *proc,
                void **buffer,
                size_t *size)
{
    int rc;
    char *key;
    opal_byte_object_t *boptr;

    /* set defaults */
    *buffer = NULL;
    *size = 0;

    key = mca_base_component_to_string(component);
    if (NULL == key) {
        return OMPI_ERR_OUT_OF_RESOURCE;
    }
    
    /* the fetch API returns a pointer to the data */
    rc = orte_db.fetch(&proc->proc_name, key, (void**)&boptr, OPAL_BYTE_OBJECT);

    if (ORTE_SUCCESS == rc) {
        /* xfer the data - it was allocated in the call */
        *buffer = (void*)boptr->bytes;
        *size = boptr->size;
    }
    /* we no longer require the struct itself since all we
     * wanted was the data inside it
     */
    free(boptr);
    free(key);
    return rc;
}

int
ompi_modex_send_string(const char* key,
                       const void *buffer, size_t size)
{
    int rc;
    opal_byte_object_t bo;

    bo.bytes = (uint8_t *)buffer;
    bo.size = size;

    /* the store API makes a copy of the provided data */
    rc = orte_db.store(ORTE_PROC_MY_NAME, key, &bo, OPAL_BYTE_OBJECT);

    return rc;
}


int
ompi_modex_recv_string(const char* key,
                       const ompi_proc_t *source_proc,
                       void **buffer, size_t *size)
{
    int rc;
    opal_byte_object_t *boptr;

    /* set defaults */
    *buffer = NULL;
    *size = 0;

    /* the fetch API returns a copy of the data */
    rc = orte_db.fetch(&source_proc->proc_name, key, (void**)&boptr, OPAL_BYTE_OBJECT);

    if (ORTE_SUCCESS == rc) {
        /* xfer the data for local use */
        *buffer = boptr->bytes;
        *size = boptr->size;
    }
    /* we no longer require the struct itself since all we
     * wanted was the data inside it
     */
    free(boptr);
    return rc;
}

/* return a pointer to the data, but don't create a new copy of it */
int ompi_modex_recv_string_pointer(const char* key,
                                   const ompi_proc_t *source_proc,
                                   void **buffer, opal_data_type_t type)
{
    int rc;

    /* set defaults */
    *buffer = NULL;

    /* the fetch_pointer API returns a pointer to the data */
    rc = orte_db.fetch_pointer(&source_proc->proc_name, key, (void**)buffer, type);

    return rc;
}

int
ompi_modex_send_key_value(const char* key,
                          const void *value,
                          opal_data_type_t dtype)
{
    int rc;

    /* the store API makes a copy of the provided data */
    rc = orte_db.store(ORTE_PROC_MY_NAME, key, value, dtype);
    
    return rc;
}

int ompi_modex_recv_key_value(const char* key,
                              const ompi_proc_t *source_proc,
                              void **value, opal_data_type_t type)
{
    int rc;

    /* the fetch API returns the data */
    rc = orte_db.fetch(&source_proc->proc_name, key, (void**)value, type);

    return rc;
}
