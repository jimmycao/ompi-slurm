/*
 * Copyright (c) 2012      Los Alamos National Security, LLC.
 *                         All rights reserved.
 * $COPYRIGHT$
 * 
 * Additional copyrights may follow
 * 
 * $HEADER$
 */

#include "orte_config.h"

#include <sys/types.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif  /* HAVE_UNISTD_H */
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif
#include <sys/stat.h>

#include "opal/util/if.h"
#include "opal/util/output.h"
#include "opal/util/uri.h"
#include "opal/dss/dss.h"

#include "orte/util/error_strings.h"
#include "orte/util/name_fns.h"
#include "orte/util/show_help.h"
#include "orte/runtime/orte_globals.h"
#include "orte/mca/db/db.h"
#include "orte/mca/errmgr/errmgr.h"
#include "orte/mca/rml/rml.h"

#include "orte/mca/test/base/base.h"
#include "test_yyy.h"

/*
 * Module functions: Global
 */
static int init(void);
static int finalize(void);
static void print(char *in);

/******************
 * APP module
 ******************/
orte_test_base_module_t orte_test_yyy_module = {
    init,
    finalize,
    print
};


static void print(char *in)
{
	printf("Hello %s, now you are in yyy module.\n", in);
}

static int init(void)
{
	return ORTE_SUCCESS;
}

static int finalize(void)
{

    return ORTE_SUCCESS;
}
