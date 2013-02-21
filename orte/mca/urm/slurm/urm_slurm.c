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

#include "orte/mca/urm/base/base.h"
#include "urm_slurm.h"
#include "orte/mca/urm/urm_types.h"

/*
 * Module functions: Global
 */
static int init(void);
static int finalize(void);
static void print9(char *in);
static int allocate(allocate_request_t *request, allocate_response_t *response);

/******************
 * APP module
 ******************/
orte_urm_base_module_t orte_urm_slurm_module = {
    init,
    finalize,
    print9,
    allocate
};


/* local vars */
static int socket_fd;
static opal_list_t jobs;
static opal_event_t recv_ev;



/* init the module */
static int init(void)
{

	puts("+++++++++++++++++++++++++++++++");
	if (NULL == mca_urm_slurm_component.config_file) {
		opal_output(0, "Cannot perform allocation as no Slurm configuration file provided");
		return ORTE_ERR_NOT_FOUND;
	}
#if 0
    char *slurm_host;
    uint16_t port;
    struct sockaddr_in address;
    int flags;
    struct hostent *h;

    if (mca_ras_slurm_component.dyn_alloc_enabled) {
        if (NULL == mca_ras_slurm_component.config_file) {
            orte_show_help("help-ras-slurm.txt", "dyn-alloc-no-config", true);
            return ORTE_ERR_SILENT;
        }
        /* setup the socket */
        if (ORTE_SUCCESS != read_ip_port(mca_ras_slurm_component.config_file,
                                         &slurm_host, &port)) {
            return ORTE_ERR_SILENT;
        }
        OPAL_OUTPUT_VERBOSE((2, orte_ras_base.ras_output,
                             "ras:slurm got [ ip = %s, port = %u ] from %s\n",
                             slurm_host, port, mca_ras_slurm_component.config_file));

        /* obtain a socket for our use */
        if ((socket_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
            ORTE_ERROR_LOG(ORTE_ERR_OUT_OF_RESOURCE);
            return ORTE_ERR_OUT_OF_RESOURCE;
        }

        /* connect to the Slurm dynamic allocation port */
        bzero(&address, sizeof(address));
        address.sin_family = AF_INET;
        if (!opal_net_isaddr(slurm_host)) {
            /* if the ControlMachine was not specified as an IP address,
             * we need to resolve it here
             */
            if (NULL == (h = gethostbyname(slurm_host))) {
                /* could not resolve it */
                orte_show_help("help-ras-slurm.txt", "host-not-resolved",
                               true, slurm_host);
                free(slurm_host);
                return ORTE_ERR_SILENT;
            }
            free(slurm_host);
            slurm_host = strdup(inet_ntoa(*(struct in_addr*)h->h_addr_list[0]));
        }
        address.sin_addr.s_addr = inet_addr(slurm_host);
        address.sin_port =  htons(port);
        if (connect(socket_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
            orte_show_help("help-ras-slurm.txt", "connection-failed",
                           true, slurm_host, (int)port);
            return ORTE_ERR_SILENT;
        }

        /* set socket up to be non-blocking */
        if ((flags = fcntl(socket_fd, F_GETFL, 0)) < 0) {
            opal_output(0, "ras:slurm:dyn: fcntl(F_GETFL) failed: %s (%d)",
                        strerror(opal_socket_errno), opal_socket_errno);
            return ORTE_ERROR;
        } else {
            flags |= O_NONBLOCK;
            if (fcntl(socket_fd, F_SETFL, flags) < 0) {
                opal_output(0, "ras:slurm:dyn: fcntl(F_SETFL) failed: %s (%d)",
                            strerror(opal_socket_errno), opal_socket_errno);
                return ORTE_ERROR;
            }
        }

        /* setup to recv data */
        opal_event_set(orte_event_base, &recv_ev, socket_fd,
                       OPAL_EV_READ, recv_data, NULL);
        opal_event_add(&recv_ev, 0);

        /* initialize the list of jobs for tracking dynamic allocations */
        OBJ_CONSTRUCT(&jobs, opal_list_t);
    }
#endif
    return ORTE_SUCCESS;
}



static void print9(char *in)
{
	printf("Hello %s, now you are in urm9 slurm module.\n", in);
}

//char *allocate_str = "allocate jobid=100 return=all timeout=10:app=0 np=5 N=2 node_list=vm2,vm3 flag=mandatory:app=1 N=2";
//job=100:app=0 slurm_jobid=200 nodes=vm1:app=1 slurm_jobid=201 nodes=vm2:app=2 slurm_jobid=202 nodes=vm[3-4]

static int allocate(allocate_request_t *request, allocate_response_t *response)
{
	char *cmd_str, **cmd=NULL, *tmp;
	resource_request_t *res_req;
	char *node_list;
	struct timeval tv;
	int i;


//	if (NULL == mca_urm_slurm_component.config_file) {
//		opal_output(0, "Cannot perform allocation as no Slurm configuration file provided");
//		return ORTE_ERR_NOT_FOUND;
//	}

	opal_argv_append_nosize(&cmd, "allocate");
	opal_argv_append_nosize(&cmd, "return=all");

	asprintf(&tmp, "jobid=%u", request->attempt_id);
	opal_argv_append_nosize(&cmd, tmp);
	free(tmp);

	asprintf(&tmp, "timeout=%u", mca_urm_slurm_component.timeout);
	opal_argv_append_nosize(&cmd, tmp);
	free(tmp);

	 for (i=0; i < request->res_array->size; i++) {
		if (NULL == (res_req = (resource_request_t*)opal_pointer_array_get_item(request->res_array, i))) {
			continue;
		}

		asprintf(&tmp, ": app=%u", res_req->appid);
		opal_argv_append_nosize(&cmd, tmp);
		free(tmp);

		asprintf(&tmp, "np=%u", res_req->np);
		opal_argv_append_nosize(&cmd, tmp);
		free(tmp);

		if (0 < res_req->min_nodes) {
			asprintf(&tmp, "N=%u", res_req->min_nodes);
			opal_argv_append_nosize(&cmd, tmp);
			free(tmp);
		}

		if (NULL != res_req->node_list) {
			asprintf(&tmp, "node_list=%s", res_req->node_list);
			opal_argv_append_nosize(&cmd, tmp);
			free(tmp);
		}

		/* add the mandatory/optional flag */
		if (res_req->mandatory) {
			opal_argv_append_nosize(&cmd, "flag=mandatory");
		} else {
			opal_argv_append_nosize(&cmd, "flag=optional");
		}
	 }

	 /* assemble it into the final cmd to be sent */
	 cmd_str = opal_argv_join(cmd, ' ');
	 opal_argv_free(cmd);

	 printf("cmd_str = %s\n", cmd_str);

//	 response->attempt_id = request->attempt_id;
}

static int finalize(void)
{
	//todo
    return ORTE_SUCCESS;
}
