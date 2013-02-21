/*
 * Copyright (c) 2012      Los Alamos National Security, LLC.
 *                         All rights reserved.
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 */

#ifndef ORTE_MCA_URM_TYPES_H
#define ORTE_MCA_URM_TYPES_H

#include "orte_config.h"

#include "opal/class/opal_list.h"
#include "opal/dss/dss_types.h"
#include "opal/class/opal_pointer_array.h"

BEGIN_C_DECLS

//#define S_SIZE	128
//#define M_SIZE	1024
//#define L_SIZE	8192

//typedef opal_list_t urm_list_t;

//char *allocate_str = "allocate jobid=100 return=all timeout=10:app=0 np=5 N=2 node_list=vm2,vm3 flag=mandatory:app=1 N=2";
//job=100:app=0 slurm_jobid=200 nodes=vm1:app=1 slurm_jobid=201 nodes=vm2:app=2 slurm_jobid=202 nodes=vm[3-4]

struct resource_request{
	uint32_t 	appid;
	uint32_t 	np;
	uint32_t 	min_nodes;
	char 		*node_list;
	bool 		mandatory;
};
typedef struct resource_request resource_request_t;

struct resource_response{
	uint32_t 	appid;
	uint32_t 	slurm_jobid;
	char  		*allocated_node_list;
};
typedef struct resource_response resource_response_t;

struct allocate_request{
	uint32_t 		attempt_id;
	opal_pointer_array_t *res_array;
	uint32_t		nreq;
};
typedef struct allocate_request allocate_request_t;


struct allocate_response{
	uint32_t 		attempt_id;
//	opal_list_t 	*resource_response_list;
};
typedef struct allocate_response allocate_response_t;

END_C_DECLS

#endif
