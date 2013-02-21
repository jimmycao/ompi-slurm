/* -*- C -*-
 *
 * $HEADER$
 *
 * The most basic of MPI applications
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//#include "opal/util/argv.h"
//#include "opal/class/opal_object.h"

#include "orte/util/name_fns.h"
#include "orte/mca/urm/urm.h"
#include "orte/mca/urm/urm_types.h"


#include "opal/mca/if/if.h"
#include "opal/util/argv.h"
#include "opal/util/cmd_line.h"
#include "opal/util/opal_environ.h"
#include "opal/util/show_help.h"
#include "opal/runtime/opal.h"

#include "opal/class/opal_pointer_array.h"

#define ORTE_PROC_NON_MPI       0x0010

static int init(int argc, char *argv[]);
static void set_env();
static void cons_request(allocate_request_t **request);

void free_allocate_request(allocate_request_t *request);

int main(int argc, char* argv[])
{

	allocate_request_t *request;
	allocate_response_t response;

	char *in = "jimmy";

	set_env();
    init(argc, argv);
    /*test1: for print9 */
    orte_urm.print9(in);

    orte_urm.init();

    /*test2: for allocate */
    cons_request(&request);
	orte_urm.allocate(request, &response);
	free_allocate_request(request);

	orte_finalize();
    return 0;
}

/*
 * struct allocate_request{
	uint32_t 		attempt_id;
	opal_pointer_array_t *res_req_array;
	uint32_t		nreq;
};
 */
void free_allocate_request(allocate_request_t *request)
{
	int i;
	void *tmp;

	if(NULL == request)
		return;

	/* free 'request->res_array' */
	if(NULL != request->res_array){
		for(i = 0; i < request->res_array->size; i++){
			if(NULL == (tmp = opal_pointer_array_get_item(request->res_array, i)))
				continue;
			else
				free(tmp);
		}
		free(request->res_array);
	}

	/* free 'request' */
	free(request);
}

static void cons_request(allocate_request_t **request)
{
	resource_request_t *res_req1, *res_req2;

	/* new 'request' */
	*request = (allocate_request_t *)malloc(sizeof(allocate_request_t));

	/* init 'request->res_req_array' */
	//OBJ_CONSTRUCT(request->res_req_array, opal_pointer_array_t);
	(*request)->res_array = OBJ_NEW(opal_pointer_array_t);

	opal_pointer_array_init((*request)->res_array, 1, INT_MAX, 1);
	(*request)->nreq = 0;

	(*request)->attempt_id = 9;

	/* create 'res_req' */
	/* res_req = OBJ_NEW(resource_request_t); */ /* what's the problem? */
	res_req1 = (resource_request_t *)malloc(sizeof(resource_request_t));

	res_req1->appid = 100;
	res_req1->np = 3;
	res_req1->min_nodes = 2;
	res_req1->node_list = strdup("node[1-2]");
	res_req1->mandatory = true;

	/* add 'res_req' to 'request->res_req_array' */
	opal_pointer_array_add((*request)->res_array, res_req1);


	res_req2 = (resource_request_t *)malloc(sizeof(resource_request_t));
	res_req2->appid = 101;
	res_req2->np = 1;
	res_req2->min_nodes = 1;
	res_req2->node_list = strdup("node[3-4]");
	res_req2->mandatory = false;

	opal_pointer_array_add((*request)->res_array, res_req2);

}


static int init(int argc, char *argv[])
{
	int rc;
	if (0 > (rc = orte_init(&argc, &argv, ORTE_PROC_NON_MPI))) {
		fprintf(stderr, "couldn't init orte - error code %d\n", rc);
		return rc;
	}
	return 0;
}


static void set_env()
{
	setenv("OMPI_MCA_urm_base_verbose", "10", 1);
	printf("OMPI_MCA_urm_base_verbose = %s\n", getenv("OMPI_MCA_urm_base_verbose"));

//	setenv("OMPI_MCA_ess_base_verbose", "10", 1);
//	printf("OMPI_MCA_ess_base_verbose = %s\n", getenv("OMPI_MCA_ess_base_verbose"));

#if 1   //if under ralph's new version ras/slurm
	setenv("OMPI_MCA_ras_slurm_config_file", "/usr/local/etc/slurm.conf", 1);
	printf("OMPI_MCA_ras_slurm_config_file = %s\n", getenv("OMPI_MCA_ras_slurm_config_file"));

//	setenv("OMPI_MCA_urm_slurm_config_file", "/usr/local/etc/slurm.conf", 1);
//	printf("OMPI_MCA_urm_slurm_config_file = %s\n", getenv("OMPI_MCA_urm_slurm_config_file"));
#endif

#if 0
	setenv("OMPI_MCA_urm", "slurm", 1);
	printf("OMPI_MCA_urm = %s\n", getenv("OMPI_MCA_urm"));
#endif
}
