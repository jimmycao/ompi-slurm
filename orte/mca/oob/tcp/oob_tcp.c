/*
 * Copyright (c) 2004-2010 The Trustees of Indiana University and Indiana
 *                         University Research and Technology
 *                         Corporation.  All rights reserved.
 * Copyright (c) 2004-2011 The University of Tennessee and The University
 *                         of Tennessee Research Foundation.  All rights
 *                         reserved.
 * Copyright (c) 2004-2005 High Performance Computing Center Stuttgart,
 *                         University of Stuttgart.  All rights reserved.
 * Copyright (c) 2004-2005 The Regents of the University of California.
 *                         All rights reserved.
 * Copyright (c) 2006-2012 Los Alamos National Security, LLC. 
 *                         All rights reserved.
 * Copyright (c) 2009-2012 Cisco Systems, Inc.  All rights reserved.
 * Copyright (c) 2011      Oak Ridge National Labs.  All rights reserved.
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 *
 * In windows, many of the socket functions return an EWOULDBLOCK
 * instead of things like EAGAIN, EINPROGRESS, etc. It has been
 * verified that this will not conflict with other error codes that
 * are returned by these functions under UNIX/Linux environments 
 */

#include "orte_config.h"
#include "orte/types.h"
#include "opal/types.h"

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#include <fcntl.h>
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#ifdef HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif
#ifdef HAVE_NETDB_H
#include <netdb.h>
#endif
#include <ctype.h>

#include "opal/util/show_help.h"
#include "opal/util/error.h"
#include "opal/util/output.h"
#include "opal/opal_socket_errno.h"
#include "opal/util/if.h"
#include "opal/util/net.h"
#include "opal/util/argv.h"
#include "opal/class/opal_hash_table.h"

#include "orte/mca/errmgr/errmgr.h"
#include "orte/mca/ess/ess.h"
#include "orte/util/name_fns.h"
#include "orte/util/parse_options.h"
#include "orte/util/show_help.h"
#include "orte/runtime/orte_globals.h"

#include "orte/mca/oob/tcp/oob_tcp.h"

/*
 * Data structure for accepting connections.
 */
struct mca_oob_tcp_event_t {
    opal_list_item_t item;
    opal_event_t *event;
};
typedef struct mca_oob_tcp_event_t mca_oob_tcp_event_t;

static void mca_oob_tcp_event_construct(mca_oob_tcp_event_t* event)
{
    OPAL_THREAD_LOCK(&mca_oob_tcp_component.tcp_lock);
    /* get an event */
    event->event = opal_event_alloc();
    /* track our events */
    opal_list_append(&mca_oob_tcp_component.tcp_events, &event->item);
    OPAL_THREAD_UNLOCK(&mca_oob_tcp_component.tcp_lock);
}

static void mca_oob_tcp_event_destruct(mca_oob_tcp_event_t* event)
{
    OPAL_THREAD_LOCK(&mca_oob_tcp_component.tcp_lock);
    /* release the event for re-use */
    opal_event_free(event->event);
    /* remove it from our list */
    opal_list_remove_item(&mca_oob_tcp_component.tcp_events, &event->item);
    OPAL_THREAD_UNLOCK(&mca_oob_tcp_component.tcp_lock);
}

OBJ_CLASS_INSTANCE(
    mca_oob_tcp_event_t,
    opal_list_item_t,
    mca_oob_tcp_event_construct,
    mca_oob_tcp_event_destruct);

/*
 * Local utility functions
 */

static int mca_oob_tcp_component_register(void);
static int mca_oob_tcp_component_open(void);
static int mca_oob_tcp_component_close(void);
static int  mca_oob_tcp_create_listen(int *target_sd, unsigned short *port, uint16_t af_family);
static int  mca_oob_tcp_create_listen_thread(void);
static void mca_oob_tcp_recv_handler(int sd, short flags, void* user);
static void mca_oob_tcp_accept(int incoming_sd);

OBJ_CLASS_INSTANCE(
                   mca_oob_tcp_pending_connection_t,
                   opal_free_list_item_t,
                   NULL,
                   NULL);

OBJ_CLASS_INSTANCE(mca_oob_tcp_device_t, opal_list_item_t, NULL, NULL);

int mca_oob_tcp_output_handle = -1;
static int verbose_value = 0;

/*
 * Struct of function pointers and all that to let us be initialized
 */
mca_oob_tcp_component_t mca_oob_tcp_component = {
  {
    {
        MCA_OOB_BASE_VERSION_2_0_0,
        "tcp", /* MCA module name */
        ORTE_MAJOR_VERSION,
        ORTE_MINOR_VERSION,
        ORTE_RELEASE_VERSION,
        mca_oob_tcp_component_open,  /* component open */
        mca_oob_tcp_component_close, /* component close */
        NULL, /* component query */
        mca_oob_tcp_component_register, /* component register */
    },
    {
        /* The component is checkpoint ready */
        MCA_BASE_METADATA_PARAM_CHECKPOINT
    },
    mca_oob_tcp_component_init
  }
};

mca_oob_t mca_oob_tcp = {
    mca_oob_tcp_init,
    mca_oob_tcp_fini,

    mca_oob_tcp_get_addr,
    mca_oob_tcp_set_addr,

    mca_oob_tcp_ping,

    mca_oob_tcp_send_nb,

    mca_oob_tcp_recv_nb,
    mca_oob_tcp_recv_cancel,

    mca_oob_tcp_ft_event
};

static int mca_oob_tcp_component_register(void)
{
    int tmp;
    char *listen_type, *str = NULL;
    bool ip4_ports_given = false;

    mca_base_param_reg_int(&mca_oob_tcp_component.super.oob_base,
                           "verbose",
                           "Verbose level for the OOB tcp component",
                           false, false,
                           0,
                           &verbose_value);

    /* register oob module parameters */
    mca_base_param_reg_int(&mca_oob_tcp_component.super.oob_base,
                           "peer_limit",
                           "Maximum number of peer connections to simultaneously maintain (-1 = infinite)",
                           false, false, -1,
                           &mca_oob_tcp_component.tcp_peer_limit);

    mca_base_param_reg_int(&mca_oob_tcp_component.super.oob_base,
                           "peer_retries",
                           "Number of times to try shutting down a connection before giving up",
                           false, false, 60,
                           &mca_oob_tcp_component.tcp_peer_retries);

    mca_base_param_reg_int(&mca_oob_tcp_component.super.oob_base,
                           "debug",
                           "Enable (1) / disable (0) debugging output for this component",
                           false, false, 0,
                           &mca_oob_tcp_component.tcp_debug);

    mca_base_param_reg_int(&mca_oob_tcp_component.super.oob_base,
                           "sndbuf",
                           "TCP socket send buffering size (in bytes)",
                           false, false, 128 * 1024,
                           &mca_oob_tcp_component.tcp_sndbuf);

    mca_base_param_reg_int(&mca_oob_tcp_component.super.oob_base,
                           "rcvbuf",
                           "TCP socket receive buffering size (in bytes)",
                           false, false, 128 * 1024,
                           &mca_oob_tcp_component.tcp_rcvbuf);

    mca_base_param_reg_string(&mca_oob_tcp_component.super.oob_base,
                              "if_include",
                              "Comma-delimited list of devices and/or CIDR notation of networks to use for Open MPI bootstrap communication (e.g., \"eth0,192.168.0.0/16\").  Mutually exclusive with oob_tcp_if_exclude.",
                              false, false, NULL, 
                              &mca_oob_tcp_component.tcp_include);
    mca_base_param_reg_string(&mca_oob_tcp_component.super.oob_base,
                              "include",
                              "Obsolete synonym for oob_tcp_if_include",
                              true, false, NULL, &str);
    if (NULL != str) {
        if (NULL == mca_oob_tcp_component.tcp_include) {
            mca_oob_tcp_component.tcp_include = str;
        } else {
            free(str);
            str = NULL;  /* reset to NULL so we can use it again later */
        }
    }

    mca_base_param_reg_string(&mca_oob_tcp_component.super.oob_base,
                              "if_exclude",
                              "Comma-delimited list of devices and/or CIDR notation of networks to NOT use for Open MPI bootstrap communication -- all devices not matching these specifications will be used (e.g., \"eth0,192.168.0.0/16\").  If set to a non-default value, it is mutually exclusive with oob_tcp_if_include.",
                              false, false, NULL, 
                              &mca_oob_tcp_component.tcp_exclude);
    mca_base_param_reg_string(&mca_oob_tcp_component.super.oob_base,
                              "exclude",
                              "Obsolete synonym for oob_tcp_if_exclude",
                              true, false, NULL, &str);
    if (NULL != str) {
        if (NULL == mca_oob_tcp_component.tcp_exclude) {
            mca_oob_tcp_component.tcp_exclude = str;
        } else {
            free(str);
            str = NULL;  /* reset to NULL so we can use it again later */
        }
    }

    mca_base_param_reg_int(&mca_oob_tcp_component.super.oob_base,
                           "connect_sleep",
                           "Enable (1) / disable (0) random sleep for "
                           "connection wireup.",
                           false,
                           false,
                           1,
                           &mca_oob_tcp_component.connect_sleep);

    mca_base_param_reg_string(&mca_oob_tcp_component.super.oob_base,
                              "listen_mode",
                              "Mode for HNP to accept incoming connections: "
                              "event, listen_thread.",
                              false,
                              false,
                              "event",
                              &listen_type);
    
    if (0 == strcmp(listen_type, "event")) {
        mca_oob_tcp_component.tcp_listen_type = OOB_TCP_EVENT;
    } else if (0 == strcmp(listen_type, "listen_thread")) {
        mca_oob_tcp_component.tcp_listen_type = OOB_TCP_LISTEN_THREAD;
    } else {
        opal_output(0, "Invalid value for oob_tcp_listen_mode parameter: %s",
                    listen_type);
        return ORTE_ERROR;
    }
    free(listen_type);

    mca_base_param_reg_int(&mca_oob_tcp_component.super.oob_base,
                           "listen_thread_max_queue",
                           "High water mark for queued accepted socket "
                           "list size.  Used only when listen_mode is "
                           "listen_thread.",
                           false,
                           false,
                           10,
                           &mca_oob_tcp_component.tcp_copy_max_size);
    mca_base_param_reg_int(&mca_oob_tcp_component.super.oob_base,
                           "listen_thread_wait_time",
                           "Time in milliseconds to wait before "
                           "actively checking for new connections when "
                           "listen_mode is listen_thread.",
                           false,
                           false,
                           10,
                           &tmp);
    mca_oob_tcp_component.tcp_listen_thread_tv.tv_sec = tmp / (1000);
    mca_oob_tcp_component.tcp_listen_thread_tv.tv_usec = (tmp % 1000) * 1000; 

#if ORTE_ENABLE_STATIC_PORTS
    mca_base_param_reg_string(&mca_oob_tcp_component.super.oob_base,
                              "static_ports", "Static ports for daemons and procs (IPv4)",
                              false, false,
                              orte_oob_static_ports,
                              &str);
    /* if ports were provided, parse the provided range */
    if (NULL != str) {
        ip4_ports_given = true;
        orte_static_ports = true;
        orte_util_parse_range_options(str, &mca_oob_tcp_component.tcp4_static_ports);
        if (0 == strcmp(mca_oob_tcp_component.tcp4_static_ports[0], "-1")) {
            opal_argv_free(mca_oob_tcp_component.tcp4_static_ports);
            mca_oob_tcp_component.tcp4_static_ports = NULL;
            orte_static_ports = false;
        }
    } else {
        orte_static_ports = false;
        mca_oob_tcp_component.tcp4_static_ports = NULL;
    }
#endif
    
    mca_base_param_reg_string(&mca_oob_tcp_component.super.oob_base,
                              "dynamic_ports", "Range of ports to be dynamically used by daemons and procs (IPv4)",
                              false, false,
                              NULL,
                              &str);
    /* if ports were provided, parse the provided range */
    if (NULL != str) {
        /* can't have both static and dynamic ports! */
        if (orte_static_ports) {
            opal_show_help("help-oob-tcp.txt", "static-and-dynamic", true,
                           mca_oob_tcp_component.tcp4_static_ports, str);
            return ORTE_ERROR;
        }
        orte_util_parse_range_options(str, &mca_oob_tcp_component.tcp4_dyn_ports);
        if (0 == strcmp(mca_oob_tcp_component.tcp4_dyn_ports[0], "-1")) {
            opal_argv_free(mca_oob_tcp_component.tcp4_dyn_ports);
            mca_oob_tcp_component.tcp4_dyn_ports = NULL;
        }
    } else {
        mca_oob_tcp_component.tcp4_dyn_ports = NULL;
    }
    
    mca_base_param_reg_int(&mca_oob_tcp_component.super.oob_base,
                           "disable_family", "Disable IPv4 (4) or IPv6 (6)",
                           false, false,
                           0,
                           &mca_oob_tcp_component.disable_family);
#if OPAL_WANT_IPV6
#if ORTE_ENABLE_STATIC_PORTS
    mca_base_param_reg_string(&mca_oob_tcp_component.super.oob_base,
                              "static_ports_v6", "Static ports for daemons and procs (IPv6)",
                              false, false,
                              orte_oob_static_ports,
                              &str);
    if (NULL != str) {
        orte_static_ports = true;
        orte_util_parse_range_options(str, &mca_oob_tcp_component.tcp6_static_ports);
        if (0 == strcmp(mca_oob_tcp_component.tcp6_static_ports[0], "-1")) {
            if (ip4_ports_given) {
                opal_output(0, "OOB:TCP:Error: IP4 static ports given, but IPv6 is enabled and an incorrect static port range was provided for it");
                return ORTE_ERR_FATAL;
            }
            opal_argv_free(mca_oob_tcp_component.tcp6_static_ports);
            mca_oob_tcp_component.tcp6_static_ports = NULL;
            orte_static_ports = false;
        }
    } else {
        if (ip4_ports_given) {
            opal_output(0, "OOB:TCP:Error: IP4 static ports given, but IPv6 is enabled and no static ports provided for it");
            return ORTE_ERR_FATAL;
        }
        orte_static_ports = false;
        mca_oob_tcp_component.tcp6_static_ports = NULL;
    }
#endif

    mca_base_param_reg_string(&mca_oob_tcp_component.super.oob_base,
                              "dynamic_ports_v6", "Range of ports to be dynamically used by daemons and procs (IPv4)",
                              false, false,
                              NULL,
                              &str);
    /* if ports were provided, parse the provided range */
    if (NULL != str) {
        /* can't have both static and dynamic ports! */
        if (orte_static_ports) {
            opal_show_help("help-oob-tcp.txt", "static-and-dynamic", true,
                           mca_oob_tcp_component.tcp6_static_ports, str);
            return ORTE_ERROR;
        }
        orte_util_parse_range_options(str, &mca_oob_tcp_component.tcp6_dyn_ports);
        if (0 == strcmp(mca_oob_tcp_component.tcp6_dyn_ports[0], "-1")) {
            opal_argv_free(mca_oob_tcp_component.tcp6_dyn_ports);
            mca_oob_tcp_component.tcp6_dyn_ports = NULL;
        }
    } else {
        mca_oob_tcp_component.tcp6_dyn_ports = NULL;
    }
    
    mca_oob_tcp_component.tcp6_listen_sd = -1;
#endif  /* OPAL_WANT_IPV6 */

    return ORTE_SUCCESS;
}

/*
 * Initialize global variables used w/in this module.
 */
static int mca_oob_tcp_component_open(void)
{
#ifdef __WINDOWS__
    WSADATA win_sock_data;
    if (WSAStartup(MAKEWORD(2,2), &win_sock_data) != 0) {
        opal_output (0, "mca_oob_tcp_component_init: failed to initialise windows sockets: error %d\n", WSAGetLastError());
        return ORTE_ERROR;
    }
#endif

    mca_oob_tcp_output_handle = opal_output_open(NULL);
    opal_output_set_verbosity(mca_oob_tcp_output_handle, verbose_value);

    OBJ_CONSTRUCT(&mca_oob_tcp_component.tcp_peer_list,     opal_list_t);
    OBJ_CONSTRUCT(&mca_oob_tcp_component.tcp_peers,         opal_hash_table_t);
    OBJ_CONSTRUCT(&mca_oob_tcp_component.tcp_peer_names,    opal_hash_table_t);
    OBJ_CONSTRUCT(&mca_oob_tcp_component.tcp_peer_free,     opal_free_list_t);
    OBJ_CONSTRUCT(&mca_oob_tcp_component.tcp_msgs,          opal_free_list_t);
    OBJ_CONSTRUCT(&mca_oob_tcp_component.tcp_lock,          opal_mutex_t);
    OBJ_CONSTRUCT(&mca_oob_tcp_component.tcp_events,        opal_list_t);
    OBJ_CONSTRUCT(&mca_oob_tcp_component.tcp_msg_post,      opal_list_t);
    OBJ_CONSTRUCT(&mca_oob_tcp_component.tcp_msg_recv,      opal_list_t);
    OBJ_CONSTRUCT(&mca_oob_tcp_component.tcp_msg_completed, opal_list_t);
    OBJ_CONSTRUCT(&mca_oob_tcp_component.tcp_match_lock,    opal_mutex_t);
    OBJ_CONSTRUCT(&mca_oob_tcp_component.tcp_match_cond,    opal_condition_t);
    OBJ_CONSTRUCT(&mca_oob_tcp_component.tcp_available_devices, opal_list_t);
    OBJ_CONSTRUCT(&mca_oob_tcp_component.tcp_listen_thread, opal_thread_t);
    OBJ_CONSTRUCT(&mca_oob_tcp_component.tcp_pending_connections, opal_list_t);
    OBJ_CONSTRUCT(&mca_oob_tcp_component.tcp_connections_return, opal_list_t);
    OBJ_CONSTRUCT(&mca_oob_tcp_component.tcp_connections_lock, opal_mutex_t);

    mca_oob_tcp_component.tcp_recv_event = NULL;
#if OPAL_WANT_IPV6
    mca_oob_tcp_component.tcp6_recv_event = NULL;
#endif

    mca_oob_tcp_component.tcp_listen_thread_event = NULL;
    mca_oob_tcp_component.tcp_listen_thread_num_sockets = 0;
    mca_oob_tcp_component.tcp_listen_thread_sds[0] = -1;
    mca_oob_tcp_component.tcp_listen_thread_sds[1] = -1;

    /* initialize state */
    mca_oob_tcp_component.tcp_shutdown = false;
    mca_oob_tcp_component.tcp_listen_sd = -1;
    mca_oob_tcp_component.tcp_match_count = 0;

    /* if_include and if_exclude need to be mutually exclusive */
    if (OPAL_SUCCESS != 
        mca_base_param_check_exclusive_string(
        mca_oob_tcp_component.super.oob_base.mca_type_name,
        mca_oob_tcp_component.super.oob_base.mca_component_name,
        "if_include",
        mca_oob_tcp_component.super.oob_base.mca_type_name,
        mca_oob_tcp_component.super.oob_base.mca_component_name,
        "if_exclude")) {
        /* Return ERR_NOT_AVAILABLE so that a warning message about
           "open" failing is not printed */
        return ORTE_ERR_NOT_AVAILABLE;
    }
    
    return ORTE_SUCCESS;
}

/*
 * Cleanup of global variables used by this module.
 */

static int mca_oob_tcp_component_close(void)
{
    opal_list_item_t *item;

#if defined(__WINDOWS__)
    WSACleanup();
#endif  /* defined(__WINDOWS__) */

    /* cleanup resources */
    while (NULL != (item = opal_list_remove_first(&mca_oob_tcp_component.tcp_available_devices))) {
        OBJ_RELEASE(item);
    }
#if 0
    OBJ_DESTRUCT(&mca_oob_tcp_component.tcp_connections_lock);
    OBJ_DESTRUCT(&mca_oob_tcp_component.tcp_connections_return);
    OBJ_DESTRUCT(&mca_oob_tcp_component.tcp_pending_connections);
    OBJ_DESTRUCT(&mca_oob_tcp_component.tcp_listen_thread);
    OBJ_DESTRUCT(&mca_oob_tcp_component.tcp_available_devices);
    OBJ_DESTRUCT(&mca_oob_tcp_component.tcp_match_cond);
    OBJ_DESTRUCT(&mca_oob_tcp_component.tcp_match_lock);
    OBJ_DESTRUCT(&mca_oob_tcp_component.tcp_msg_completed);
    OBJ_DESTRUCT(&mca_oob_tcp_component.tcp_msg_recv);
    OBJ_DESTRUCT(&mca_oob_tcp_component.tcp_msg_post);
    OBJ_DESTRUCT(&mca_oob_tcp_component.tcp_events);
    OBJ_DESTRUCT(&mca_oob_tcp_component.tcp_lock);
    OBJ_DESTRUCT(&mca_oob_tcp_component.tcp_msgs);
    OBJ_DESTRUCT(&mca_oob_tcp_component.tcp_peer_free);
    OBJ_DESTRUCT(&mca_oob_tcp_component.tcp_peer_names);
    OBJ_DESTRUCT(&mca_oob_tcp_component.tcp_peers);
    OBJ_DESTRUCT(&mca_oob_tcp_component.tcp_peer_list);

    opal_output_close(mca_oob_tcp_output_handle);
#endif
    
    return ORTE_SUCCESS;
}


/* Called by mca_oob_tcp_accept() and mca_oob_tcp_thread_handler() on
 * a socket that has been accepted.  This call finishes processing the
 * socket, including setting socket options and registering for the
 * OOB-level connection handshake.  Used by both the threaded and
 * event listen modes.
 */
static void
mca_oob_tcp_create_connection(const int accepted_fd,
                              const struct sockaddr *addr)
{
    mca_oob_tcp_event_t* event;

    /* setup socket options */
    mca_oob_tcp_set_socket_options(accepted_fd);

    /* log the accept */
    if (mca_oob_tcp_component.tcp_debug >= OOB_TCP_DEBUG_CONNECT) {
        opal_output(0, "%s mca_oob_tcp_accept: %s:%d\n",
                    ORTE_NAME_PRINT(ORTE_PROC_MY_NAME),
                    opal_net_get_hostname(addr),
                    opal_net_get_port(addr));
    }

    /* wait for receipt of peers process identifier to complete this connection */
    event = OBJ_NEW(mca_oob_tcp_event_t);
    opal_event_set(orte_event_base, event->event, accepted_fd, OPAL_EV_READ, mca_oob_tcp_recv_handler, event);
    opal_event_set_priority(event->event, ORTE_MSG_PRI);
    opal_event_add(event->event, 0);
}


/*
 *  Called by mca_oob_tcp_recv_handler() when the TCP listen socket
 *  has pending connection requests. Accept incoming requests and
 *  queue for completion of the connection handshake.  Will not be
 *  called when listen_mode is listen_thread.
*/
static void mca_oob_tcp_accept(int incoming_sd)
{
    while(true) {
        struct sockaddr_storage addr;
        opal_socklen_t addrlen = sizeof(struct sockaddr_storage);
        int sd;

        sd = accept(incoming_sd, (struct sockaddr*)&addr, &addrlen);
        if(sd < 0) {
            if(EINTR == opal_socket_errno) {
                continue;
            }
            if(opal_socket_errno != EAGAIN && opal_socket_errno != EWOULDBLOCK) {
                if(EMFILE == opal_socket_errno) {
                    /*
                     * Close incoming_sd so that orte_show_help will have a file
                     * descriptor with which to open the help file.  We will be
                     * exiting anyway, so we don't need to keep it open.
                     */
                    CLOSE_THE_SOCKET(incoming_sd);
                    ORTE_ERROR_LOG(ORTE_ERR_SYS_LIMITS_SOCKETS);
                    orte_show_help("help-orterun.txt", "orterun:sys-limit-sockets", true);
                } else {
                    opal_output(0, "mca_oob_tcp_accept: accept() failed: %s (%d).", 
                                strerror(opal_socket_errno), opal_socket_errno);
                }
                orte_errmgr.abort(ORTE_ERROR_DEFAULT_EXIT_CODE, NULL);
            }
            return;
        }

        mca_oob_tcp_create_connection(sd, (struct sockaddr*) &addr);
    }
}


/*
 * Create a listen socket in the specified af_family and bind to all
 * interfaces.  
 * 
 * At one time, this also registered a callback with the event library
 * for when connections were received on the listen socket.  This is
 * no longer the case -- the caller must register any events required.
 *
 * Called by both the threaded and event based listen modes.
 */
static int
mca_oob_tcp_create_listen(int *target_sd, unsigned short *target_port, uint16_t af_family)
{
    int flags, i;
    uint16_t port=0;
    struct sockaddr_storage inaddr;
    opal_socklen_t addrlen;
    char **ports=NULL;

    /* create a listen socket for incoming connections */
    *target_sd = socket(af_family, SOCK_STREAM, 0);
    if(*target_sd < 0) {
        if (EAFNOSUPPORT != opal_socket_errno) {
            opal_output(0,"mca_oob_tcp_component_init: socket() failed: %s (%d)", 
                        strerror(opal_socket_errno), opal_socket_errno);
        }
        return ORTE_ERR_IN_ERRNO;
    }

    /* setup socket options */
    mca_oob_tcp_set_socket_options(*target_sd);

    /* Set some more options and fill in the family / address
       information.  Set to bind to the any address */
#if OPAL_WANT_IPV6
    {
        struct addrinfo hints, *res = NULL;
        int error;

        memset(&hints, 0, sizeof(hints));
        hints.ai_family = af_family;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_flags = AI_PASSIVE;
        
        if ((error = getaddrinfo(NULL, "0", &hints, &res))) {
            opal_output (0,
                        "mca_oob_tcp_create_listen: unable to resolve. %s\n",
                        gai_strerror (error));
            return ORTE_ERROR;
        }
        
        memcpy (&inaddr, res->ai_addr, res->ai_addrlen);
        addrlen = res->ai_addrlen;
        freeaddrinfo (res);
        
#ifdef IPV6_V6ONLY
        /* in case of AF_INET6, disable v4-mapped addresses */
        if (AF_INET6 == af_family) {
            int flg = 0;
            if (setsockopt (*target_sd, IPPROTO_IPV6, IPV6_V6ONLY,
                            (char *) &flg, sizeof (flg)) < 0) {
                opal_output(0,
                            "mca_oob_tcp_create_listen: unable to disable v4-mapped addresses\n");
            }
        }
#endif /* IPV6_V6ONLY */
    }
#else
    if (AF_INET != af_family) {
        return ORTE_ERROR;
    }
    ((struct sockaddr_in*) &inaddr)->sin_family = af_family;
    ((struct sockaddr_in*) &inaddr)->sin_addr.s_addr = INADDR_ANY;
    addrlen = sizeof(struct sockaddr_in);
#endif

    /* If an explicit range of ports was given, find the first open
       port in the range.  Otherwise, tcp_port_min will be 0, which
       means "pick any port" */
    if (AF_INET == af_family) {
        if (ORTE_PROC_IS_DAEMON && orte_use_common_port) {
            /* use the same port as the HNP */
            char *ptr, *portptr;
            portptr = strdup(orte_process_info.my_hnp_uri);
            ptr = strrchr(portptr, ':');
            ptr++;
            opal_argv_append_nosize(&ports, ptr);
            free(portptr);
        } else if (ORTE_PROC_IS_HNP || ORTE_PROC_IS_DAEMON ||
                   ORTE_PROC_IS_CM || ORTE_PROC_IS_CMSLAVE) {
            if (NULL != mca_oob_tcp_component.tcp4_static_ports) {
                /* if static ports were provided, the daemon takes the
                 * first entry in the list
                 */
                opal_argv_append_nosize(&ports, mca_oob_tcp_component.tcp4_static_ports[0]);
                /* flag that we are using static ports */
                orte_static_ports = true;
            } else if (NULL != mca_oob_tcp_component.tcp4_dyn_ports) {
                /* take the entire range */
                ports = opal_argv_copy(mca_oob_tcp_component.tcp4_dyn_ports);
                orte_static_ports = false;
            } else {
                /* flag the system to dynamically take any available port */
                opal_argv_append_nosize(&ports, "0");
                orte_static_ports = false;
            }
        } else if (ORTE_PROC_IS_MPI) {
            if (NULL != mca_oob_tcp_component.tcp4_static_ports) {
                /* if static ports were provided, an mpi proc takes its
                 * node_local_rank entry in the list IF it has that info
                 * AND enough ports were provided - otherwise, we "pick any port"
                 */
                orte_node_rank_t nrank;
                /* do I know my node_local_rank yet? */
                if (ORTE_NODE_RANK_INVALID != (nrank = orte_process_info.my_node_rank) &&
                    (nrank+1) < opal_argv_count(mca_oob_tcp_component.tcp4_static_ports)) {
                    /* any daemon takes the first entry, so we start with the second */
                    opal_argv_append_nosize(&ports, mca_oob_tcp_component.tcp4_static_ports[nrank+1]);
                    /* flag that we are using static ports */
                    orte_static_ports = true;
                } else {
                    /* flag the system to dynamically take any available port */
                    opal_argv_append_nosize(&ports, "0");
                    orte_static_ports = false;
                }
            } else if (NULL != mca_oob_tcp_component.tcp4_dyn_ports) {
                /* take the entire range */
                ports = opal_argv_copy(mca_oob_tcp_component.tcp4_dyn_ports);
                orte_static_ports = false;
            } else {
                /* flag the system to dynamically take any available port */
                opal_argv_append_nosize(&ports, "0");
                orte_static_ports = false;
            }
        } else {
            /* if we are a tool, then we must let the
             * system pick any port
             */
            opal_argv_append_nosize(&ports, "0");
            /* if static ports were specified, flag it
             * so the HNP does the right thing
             */
            if (NULL != mca_oob_tcp_component.tcp4_static_ports) {
                orte_static_ports = true;
            } else {
                orte_static_ports = false;
            }
        }
    }

#if OPAL_WANT_IPV6
    if (AF_INET6 == af_family) {
        if (ORTE_PROC_IS_HNP || ORTE_PROC_IS_DAEMON ||
            ORTE_PROC_IS_CM || ORTE_PROC_IS_CMSLAVE) {
            if (NULL != mca_oob_tcp_component.tcp6_static_ports) {
                /* if static ports were provided, the daemon takes the
                 * first entry in the list
                 */
                opal_argv_append_nosize(&ports, mca_oob_tcp_component.tcp6_static_ports[0]);
                /* flag that we are using static ports */
                orte_static_ports = true;
            } else if (NULL != mca_oob_tcp_component.tcp6_dyn_ports) {
                /* take the entire range */
                ports = opal_argv_copy(mca_oob_tcp_component.tcp6_dyn_ports);
                orte_static_ports = false;
            } else {
                /* flag the system to dynamically take any available port */
                opal_argv_append_nosize(&ports, "0");
                orte_static_ports = false;
            }
        } else if (ORTE_PROC_IS_MPI) {
            if (NULL != mca_oob_tcp_component.tcp6_static_ports) {
                /* if static ports were provided, an mpi proc takes its
                 * node_local_rank entry in the list IF it has that info
                 * AND enough ports were provided - otherwise, we "pick any port"
                 */
                orte_node_rank_t nrank;
                /* do I know my node_local_rank yet? */
                if (ORTE_NODE_RANK_INVALID != (nrank = orte_process_info.my_node_rank) &&
                    (nrank+1) < opal_argv_count(mca_oob_tcp_component.tcp6_static_ports)) {
                    /* any daemon takes the first entry, so we start with the second */
                    opal_argv_append_nosize(&ports, mca_oob_tcp_component.tcp6_static_ports[nrank+1]);
                    /* flag that we are using static ports */
                    orte_static_ports = true;
                } else {
                    /* flag the system to dynamically take any available port */
                    opal_argv_append_nosize(&ports, "0");
                    orte_static_ports = false;
                }
            } else if (NULL != mca_oob_tcp_component.tcp6_dyn_ports) {
                /* take the entire range */
                ports = opal_argv_copy(mca_oob_tcp_component.tcp6_dyn_ports);
                orte_static_ports = false;
            } else {
                /* flag the system to dynamically take any available port */
                opal_argv_append_nosize(&ports, "0");
                orte_static_ports = false;
            }
        } else {
            /* if we are a tool, then we must let the
             * system pick any port
             */
            opal_argv_append_nosize(&ports, "0");
            /* if static ports were specified, flag it
             * so the HNP does the right thing
             */
            if (NULL != mca_oob_tcp_component.tcp6_static_ports) {
                orte_static_ports = true;
            } else {
                orte_static_ports = false;
            }
        }
    }
#endif  /* OPAL_WANT_IPV6 */

    /* bozo check - this should be impossible, but... */
    if (NULL == ports) {
        return ORTE_ERROR;
    }
    
    /* Enable/disable reusing ports */
    if (orte_static_ports) {
        flags = 1;
    } else {
        flags = 0;
    }
    if (setsockopt (*target_sd, SOL_SOCKET, SO_REUSEADDR, (const char *)&flags, sizeof(flags)) < 0) {
        opal_output(0, "mca_oob_tcp_create_listen: unable to set the "
                    "SO_REUSEADDR option (%s:%d)\n",
                    strerror(opal_socket_errno), opal_socket_errno);
        CLOSE_THE_SOCKET(*target_sd);
        opal_argv_free(ports);
        return ORTE_ERROR;
    }
    
    for (i=0; i < opal_argv_count(ports); i++) {
        /* get the port number */
        port = strtol(ports[i], NULL, 10);
        /* convert it to network-byte-order */
        port = htons(port);

        if (AF_INET == af_family) {
            ((struct sockaddr_in*) &inaddr)->sin_port = port;
        } else if (AF_INET6 == af_family) {
            ((struct sockaddr_in6*) &inaddr)->sin6_port = port;
        } else {
            opal_argv_free(ports);
            return ORTE_ERROR;
        }
        
        if (bind(*target_sd, (struct sockaddr*)&inaddr, addrlen) < 0) {
            if( (EADDRINUSE == opal_socket_errno) || (EADDRNOTAVAIL == opal_socket_errno) ) {
                continue;
            }
            opal_output(0, "%s bind() failed for port %d: %s (%d)",
                        ORTE_NAME_PRINT(ORTE_PROC_MY_NAME),
                        (int)ntohs(*target_port),
                        strerror(opal_socket_errno),
                        opal_socket_errno );
            CLOSE_THE_SOCKET(*target_sd);
            opal_argv_free(ports);
            return ORTE_ERROR;
        }
        goto socket_binded;

    }
    
    /* cleanup */
    CLOSE_THE_SOCKET(*target_sd);
    opal_argv_free(ports);
    return ORTE_ERR_SOCKET_NOT_AVAILABLE;

socket_binded:
    /* done with this, so release it */
    opal_argv_free(ports);
    
    /* resolve assigned port */
    if (getsockname(*target_sd, (struct sockaddr*)&inaddr, &addrlen) < 0) {
        opal_output(0, "mca_oob_tcp_create_listen: getsockname(): %s (%d)", 
                    strerror(opal_socket_errno), opal_socket_errno);
        CLOSE_THE_SOCKET(*target_sd);
        return ORTE_ERROR;
    }

    /* record the assigned port */
    if (AF_INET == af_family) {
        *target_port = ((struct sockaddr_in*) &inaddr)->sin_port;
    } else {
        *target_port = ((struct sockaddr_in6*) &inaddr)->sin6_port;
    }
    
    /* save the port in a global place as well,
     * remembering to convert it back from network byte order first
     */
    orte_process_info.my_port = ntohs(*target_port);
    if (mca_oob_tcp_component.tcp_debug >= OOB_TCP_DEBUG_CONNECT) {
        opal_output(0, "%s assigned port %d", ORTE_NAME_PRINT(ORTE_PROC_MY_NAME), orte_process_info.my_port);
    }
    
    /* setup listen backlog to maximum allowed by kernel */
    if(listen(*target_sd, SOMAXCONN) < 0) {
        opal_output(0, "mca_oob_tcp_component_init: listen(): %s (%d)", 
                    strerror(opal_socket_errno), opal_socket_errno);
        return ORTE_ERROR;
    }

    /* set socket up to be non-blocking, otherwise accept could block */
    if((flags = fcntl(*target_sd, F_GETFL, 0)) < 0) {
        opal_output(0, "mca_oob_tcp_component_init: fcntl(F_GETFL) failed: %s (%d)", 
                    strerror(opal_socket_errno), opal_socket_errno);
        return ORTE_ERROR;
    } else {
        flags |= O_NONBLOCK;
        if(fcntl(*target_sd, F_SETFL, flags) < 0) {
            opal_output(0, "mca_oob_tcp_component_init: fcntl(F_SETFL) failed: %s (%d)", 
                        strerror(opal_socket_errno), opal_socket_errno);
            return ORTE_ERROR;
        }
    }

    return ORTE_SUCCESS;
}


/*
 * The listen thread created when listen_mode is threaded.  Accepts
 * incoming connections and places them in a queue for further
 * processing.  Finishing the accepted connection is done in the main
 * thread to maintain thread safety even when the event library and
 * most of ORTE is in single threaded mode.
 *
 * Runs until mca_oob_tcp_compnent.tcp_shutdown is set to true.
 */
static void*
mca_oob_tcp_listen_thread(opal_object_t *obj)
{
    int rc, i, max, accepted_connections, need_write;
    opal_socklen_t addrlen = sizeof(struct sockaddr_storage);
    opal_free_list_item_t *fl_item;
    mca_oob_tcp_pending_connection_t *pending_connection;
    struct timeval timeout;
    fd_set readfds;
    opal_list_t local_accepted_list;
    opal_free_list_t pending_connections_fl;

    OBJ_CONSTRUCT(&local_accepted_list, opal_list_t);
    OBJ_CONSTRUCT(&pending_connections_fl, opal_free_list_t);
    opal_free_list_init(&pending_connections_fl,
                        sizeof(mca_oob_tcp_pending_connection_t),
                        OBJ_CLASS(mca_oob_tcp_pending_connection_t),
                        16,  /* initial number */
                        -1,  /* maximum number */
                        16);  /* increment to grow by */

    while (false == mca_oob_tcp_component.tcp_shutdown) {
        FD_ZERO(&readfds);
        max = -1;
        for (i = 0 ; i < mca_oob_tcp_component.tcp_listen_thread_num_sockets ; ++i) {
            int sd = mca_oob_tcp_component.tcp_listen_thread_sds[i];
            FD_SET(sd, &readfds);
            max = (sd > max) ? sd : max;
        }
        /* XXX - FIX ME - should really slowly back this off as
           connections are done.  Will reduce amount of polling in the
           HNP after the initial connection storm.  Would also require
           some type of wakeup mechanism for when shutdown happens */
        timeout.tv_sec = mca_oob_tcp_component.tcp_listen_thread_tv.tv_sec;
        timeout.tv_usec = mca_oob_tcp_component.tcp_listen_thread_tv.tv_usec;

        /* Block in a select for a short (10ms) amount of time to give
           the other thread a chance to do some work.  If a connection
           comes in, we'll get woken up right away. */
        rc = select(max + 1, &readfds, NULL, NULL, &timeout);
        if (rc < 0) {
            if (EAGAIN != opal_socket_errno && EINTR != opal_socket_errno) {
                perror("select");
            }
            continue;
        }

        /* Spin accepting connections until either our queue is full
           or all active listen sockets do not have any incoming
           connections */
        do {
            accepted_connections = 0;
            for (i = 0 ; i < mca_oob_tcp_component.tcp_listen_thread_num_sockets ; ++i) {
                int sd = mca_oob_tcp_component.tcp_listen_thread_sds[i];

                /* make sure we have space for an accepted connection */
                if (opal_list_get_size(&local_accepted_list) >= 
                    (size_t) mca_oob_tcp_component.tcp_copy_max_size) {
                    goto recover;
                }

                /* Can't wait because our thread is the only one that
                   can put things back in the free list */
                OPAL_FREE_LIST_GET(&pending_connections_fl, fl_item, rc);
                if (NULL == fl_item) goto recover;

                pending_connection = (mca_oob_tcp_pending_connection_t*) fl_item;
                pending_connection->fd = accept(sd,
                                                (struct sockaddr*)&(pending_connection->addr),
                                                &addrlen);
                if (pending_connection->fd < 0) {
                    OPAL_FREE_LIST_RETURN(&pending_connections_fl, fl_item);
                    if (mca_oob_tcp_component.tcp_shutdown) goto done;

                    if (opal_socket_errno != EAGAIN || 
                        opal_socket_errno != EWOULDBLOCK) {
                        CLOSE_THE_SOCKET(pending_connection->fd);
                        if(EMFILE == opal_socket_errno) {
                            ORTE_ERROR_LOG(ORTE_ERR_SYS_LIMITS_SOCKETS);
                            orte_show_help("help-orterun.txt", "orterun:sys-limit-sockets", true);
                        } else {
                            opal_output(0, "mca_oob_tcp_accept: accept() failed: %s (%d).",
                                        strerror(opal_socket_errno), opal_socket_errno);
                        }
                        goto done;
                    }

                    continue;
                }

                if (mca_oob_tcp_component.tcp_debug >= OOB_TCP_DEBUG_CONNECT) {
                    opal_output(0, 
                                "%s mca_oob_tcp_listen_thread: new connection: "
                                "(%d, %d) %s:%d\n",
                                ORTE_NAME_PRINT(ORTE_PROC_MY_NAME),
                                pending_connection->fd, opal_socket_errno,
                                opal_net_get_hostname((struct sockaddr*) &pending_connection->addr),
                                opal_net_get_port((struct sockaddr*) &pending_connection->addr));
                }

                opal_list_append(&local_accepted_list, (opal_list_item_t*) pending_connection);
                accepted_connections++;
            }
        } while (accepted_connections > 0);

        /* recover from a loop of accepting resources.  Give any new
           connections to the main thread and reap any available
           connection fragments */
    recover:
        need_write = 0;
        if (0 != opal_list_get_size(&local_accepted_list) ||
            0 != opal_list_get_size(&mca_oob_tcp_component.tcp_connections_return)) {
            opal_mutex_lock(&mca_oob_tcp_component.tcp_connections_lock);
            /* copy local accepted list into shared list */
            if (0 != opal_list_get_size(&local_accepted_list)) {
                opal_list_join(&mca_oob_tcp_component.tcp_pending_connections,
                               opal_list_get_end(&mca_oob_tcp_component.tcp_pending_connections),
                               &local_accepted_list);
            }

            /* If the pending connection list is now at high
               watermark, will signal the other thread */
            if (opal_list_get_size(&mca_oob_tcp_component.tcp_pending_connections) >=
                (size_t) mca_oob_tcp_component.tcp_copy_max_size) {
                need_write = 1;
            }
            /* As an optimization, we could probably copy into a local
               list, exit the lock, then free the pending connections,
               but I'm not convinced that would be any faster */
            while (NULL != (fl_item = (opal_free_list_item_t*) 
                            opal_list_remove_first(&mca_oob_tcp_component.tcp_connections_return))) {
                OPAL_FREE_LIST_RETURN(&pending_connections_fl, fl_item);
            }

            opal_mutex_unlock(&mca_oob_tcp_component.tcp_connections_lock);
        }

        if (need_write) {
            char buf[1] = { '\0' };
#ifdef HAVE_PIPE
            write(mca_oob_tcp_component.tcp_connections_pipe[1], buf, 1);
#endif
        }
    }

 done:
    OBJ_DESTRUCT(&local_accepted_list);
    OBJ_DESTRUCT(&pending_connections_fl);

    return NULL;
}


/*
 * Handler for accepting connections from the listen thread. Called by
 * timer or pipe signal.
 */
static void
mca_oob_tcp_accept_thread_handler(int sd, short flags, void* user)
{
    /* probably more efficient to use the user pointer for this rather
       than always recreating the list.  Future work. */
    opal_list_t local_accepted_list;
    opal_list_t local_return_list;
    mca_oob_tcp_pending_connection_t *new_connection;
    struct timeval tv;

    if (mca_oob_tcp_component.tcp_debug >= OOB_TCP_DEBUG_INFO) {
        opal_output(0, "%s in accept_thread_handler: %d",
                    ORTE_NAME_PRINT(ORTE_PROC_MY_NAME), flags);
    }

    OBJ_CONSTRUCT(&local_accepted_list, opal_list_t);
    OBJ_CONSTRUCT(&local_return_list, opal_list_t);

    /* read the byte waiting - if we don't have pipe, this can't ever
       happen, so no need for yet another #if */
    if (OPAL_EV_READ == flags) {
        char buf[1];
        ssize_t ret;
        
        ret = read(sd, buf, 1); /* do this as separate step so non-debug compiles don't bark */
        assert (ret == 1);
    }

    /* Copy in all pending connections.  opal_list_join is O(1), so
       this is pretty cheap.  Size is pretty friendly to thread
       safety, and join will properly handle the case where the list
       magically got shorter.  */
    if (0 != opal_list_get_size(&mca_oob_tcp_component.tcp_pending_connections)) {
        opal_mutex_lock(&mca_oob_tcp_component.tcp_connections_lock);
        opal_list_join(&local_accepted_list,
                       opal_list_get_end(&local_accepted_list),
                       &mca_oob_tcp_component.tcp_pending_connections);
        opal_mutex_unlock(&mca_oob_tcp_component.tcp_connections_lock);
    }

    /* process all the connections */
    while (NULL != (new_connection = (mca_oob_tcp_pending_connection_t*) 
                    opal_list_remove_first(&local_accepted_list))) {
        mca_oob_tcp_create_connection(new_connection->fd,
                                      (struct sockaddr*) &(new_connection->addr));

        opal_list_append(&local_return_list, (opal_list_item_t*) new_connection);
    }

    /* Copy all processed connections into the return list */
    if (0 != opal_list_get_size(&local_return_list)) {
        opal_mutex_lock(&mca_oob_tcp_component.tcp_connections_lock);
        opal_list_join(&mca_oob_tcp_component.tcp_connections_return,
                       opal_list_get_end(&mca_oob_tcp_component.tcp_connections_return),
                       &local_return_list);
        opal_mutex_unlock(&mca_oob_tcp_component.tcp_connections_lock);
    }

    OBJ_DESTRUCT(&local_accepted_list);
    OBJ_DESTRUCT(&local_return_list);

    tv.tv_sec = mca_oob_tcp_component.tcp_listen_thread_tv.tv_sec;
    tv.tv_usec = mca_oob_tcp_component.tcp_listen_thread_tv.tv_usec;
#ifdef HAVE_PIPE
    if (NULL == mca_oob_tcp_component.tcp_listen_thread_event) {
        /* get an event */
        mca_oob_tcp_component.tcp_listen_thread_event = opal_event_alloc();
        opal_event_set(orte_event_base, mca_oob_tcp_component.tcp_listen_thread_event,
                       mca_oob_tcp_component.tcp_connections_pipe[0],
                       OPAL_EV_READ, 
                       mca_oob_tcp_accept_thread_handler, NULL);
    }
#else
    if (NULL == mca_oob_tcp_component.tcp_listen_thread_event) {
        /* get an event */
        mca_oob_tcp_component.tcp_listen_thread_event = opal_event_alloc();
        opal_event_set(orte_event_base, mca_oob_tcp_component.tcp_listen_thread_event,
                       -1, 0,
                       mca_oob_tcp_accept_thread_handler, NULL);
    }
#endif
    opal_event_set_priority(mca_oob_tcp_component.tcp_listen_thread_event, ORTE_MSG_PRI);
    opal_event_add(mca_oob_tcp_component.tcp_listen_thread_event, &tv);
}


/*
 * Create the actual listen thread.  Should only be called once.
 */
static int
mca_oob_tcp_create_listen_thread(void)
{
    struct timeval tv;

#ifdef HAVE_PIPE
    if (pipe(mca_oob_tcp_component.tcp_connections_pipe) < 0) {
        opal_output(0, "mca_oob_tcp_create_listen_thread: pipe failed: %d", errno);
        return ORTE_ERROR;
    }
#endif

    /* start the listen thread */
    mca_oob_tcp_component.tcp_listen_thread.t_run = mca_oob_tcp_listen_thread;
    mca_oob_tcp_component.tcp_listen_thread.t_arg = NULL;

    /* register event for read and timeout */
    tv.tv_sec = mca_oob_tcp_component.tcp_listen_thread_tv.tv_sec;
    tv.tv_usec = mca_oob_tcp_component.tcp_listen_thread_tv.tv_usec;
#ifdef HAVE_PIPE
    if (NULL == mca_oob_tcp_component.tcp_listen_thread_event) {
        /* get an event */
        mca_oob_tcp_component.tcp_listen_thread_event = opal_event_alloc();
        opal_event_set(orte_event_base, mca_oob_tcp_component.tcp_listen_thread_event,
                       mca_oob_tcp_component.tcp_connections_pipe[0],
                       OPAL_EV_READ, 
                       mca_oob_tcp_accept_thread_handler, NULL);
    }
#else
    if (NULL == mca_oob_tcp_component.tcp_listen_thread_event) {
        /* get an event */
        mca_oob_tcp_component.tcp_listen_thread_event = opal_event_alloc();
        opal_event_set(orte_event_base, mca_oob_tcp_component.tcp_listen_thread_event,
                       -1, 0,
                       mca_oob_tcp_accept_thread_handler, NULL);
    }
#endif
    opal_event_set_priority(mca_oob_tcp_component.tcp_listen_thread_event, ORTE_MSG_PRI);
    opal_event_add(mca_oob_tcp_component.tcp_listen_thread_event, &tv);

    return opal_thread_start(&mca_oob_tcp_component.tcp_listen_thread);
}


/*
 * Handle probe
 */
static void mca_oob_tcp_recv_probe(int sd, mca_oob_tcp_hdr_t* hdr)
{
    unsigned char* ptr = (unsigned char*)hdr;
    size_t cnt = 0;

    hdr->msg_type = MCA_OOB_TCP_PROBE;
    hdr->msg_dst = hdr->msg_src;
    hdr->msg_src = *ORTE_PROC_MY_NAME;
    MCA_OOB_TCP_HDR_HTON(hdr);

    while(cnt < sizeof(mca_oob_tcp_hdr_t)) {
        int retval = send(sd, (char *)ptr+cnt, sizeof(mca_oob_tcp_hdr_t)-cnt, 0);
        if(retval < 0) {
            if(opal_socket_errno != EINTR && opal_socket_errno != EAGAIN && opal_socket_errno != EWOULDBLOCK) {
                opal_output(0, "%s-%s mca_oob_tcp_peer_recv_probe: send() failed: %s (%d)\n",
                    ORTE_NAME_PRINT(ORTE_PROC_MY_NAME),
                    ORTE_NAME_PRINT(&(hdr->msg_src)),
                    strerror(opal_socket_errno),
                    opal_socket_errno);
                CLOSE_THE_SOCKET(sd);
                return;
            }
            continue;
        }
        cnt += retval;
    }
    CLOSE_THE_SOCKET(sd);
}


/*
 * Complete the OOB-level handshake to establish a connection with
 * another peer.  Called when the remote peer replies with his process
 * identifier.  Used in both the threaded and event listen modes.
 */
static void mca_oob_tcp_recv_connect(int sd, mca_oob_tcp_hdr_t* hdr)
{
    mca_oob_tcp_peer_t* peer;
    int flags;
    int cmpval;

    /* now set socket up to be non-blocking */
    if((flags = fcntl(sd, F_GETFL, 0)) < 0) {
        opal_output(0, "%s mca_oob_tcp_recv_handler: fcntl(F_GETFL) failed: %s (%d)",
               ORTE_NAME_PRINT(ORTE_PROC_MY_NAME), strerror(opal_socket_errno), opal_socket_errno);
    } else {
        flags |= O_NONBLOCK;
        if(fcntl(sd, F_SETFL, flags) < 0) {
            opal_output(0, "%s mca_oob_tcp_recv_handler: fcntl(F_SETFL) failed: %s (%d)",
                ORTE_NAME_PRINT(ORTE_PROC_MY_NAME), strerror(opal_socket_errno), opal_socket_errno);
        }
    }

    /* check for invalid name - if this is true, then we have an error
     */
    cmpval = orte_util_compare_name_fields(ORTE_NS_CMP_ALL, &hdr->msg_src, ORTE_NAME_INVALID);
    if (cmpval == OPAL_EQUAL) {
        ORTE_ERROR_LOG(ORTE_ERR_VALUE_OUT_OF_BOUNDS);
        return;
    }

    /* lookup the corresponding process */
    peer = mca_oob_tcp_peer_lookup(&hdr->msg_src);
    if(NULL == peer) {
        opal_output(0, "%s mca_oob_tcp_recv_handler: unable to locate peer",
                ORTE_NAME_PRINT(ORTE_PROC_MY_NAME));
        CLOSE_THE_SOCKET(sd);
        return;
    }
    /* is the peer instance willing to accept this connection */
    if(mca_oob_tcp_peer_accept(peer, sd) == false) {
        if(mca_oob_tcp_component.tcp_debug >= OOB_TCP_DEBUG_CONNECT_FAIL) {
            opal_output(0, "%s-%s mca_oob_tcp_recv_handler: "
                    "rejected connection from %s connection state %d",
                    ORTE_NAME_PRINT(ORTE_PROC_MY_NAME),
                    ORTE_NAME_PRINT(&(peer->peer_name)),
                    ORTE_NAME_PRINT(&(hdr->msg_src)),
                    peer->peer_state);
        }
        CLOSE_THE_SOCKET(sd);
        return;
    }
}


/*
 * Event callback when there is data available on the registered
 * socket to recv.  This is called for the listen sockets to accept an
 * incoming connection, on new sockets trying to complete the software
 * connection process, and for probes.  Data on an established
 * connection is handled elsewhere. 
 */
static void mca_oob_tcp_recv_handler(int sd, short flags, void* user)
{
    mca_oob_tcp_hdr_t hdr;
    mca_oob_tcp_event_t* event = (mca_oob_tcp_event_t *)user;
    int rc;

    /* accept new connections on the listen socket */
    if( mca_oob_tcp_component.tcp_listen_sd == sd
#if OPAL_WANT_IPV6
        || mca_oob_tcp_component.tcp6_listen_sd == sd
#endif  /* OPAL_WANT_IPV6 */
       ) {
        mca_oob_tcp_accept(sd);
        return;
    }
    OBJ_RELEASE(event);

    /* Some mem checkers don't realize that hdr will guarantee to be
       fully filled in during the read(), below :-( */
    OPAL_DEBUG_ZERO(hdr);

    /* recv the process identifier */
    while((rc = recv(sd, (char *)&hdr, sizeof(hdr), 0)) != sizeof(hdr)) {
        if(rc >= 0) {
            if(mca_oob_tcp_component.tcp_debug >= OOB_TCP_DEBUG_CONNECT_FAIL) {
                opal_output(0, "%s mca_oob_tcp_recv_handler: peer closed connection",
                    ORTE_NAME_PRINT(ORTE_PROC_MY_NAME));
            }
            CLOSE_THE_SOCKET(sd);
            return;
        }
        if(opal_socket_errno != EINTR) {
            opal_output(0, "%s mca_oob_tcp_recv_handler: recv() failed: %s (%d)\n",
                ORTE_NAME_PRINT(ORTE_PROC_MY_NAME), strerror(opal_socket_errno), opal_socket_errno);
            CLOSE_THE_SOCKET(sd);
            return;
        }
    }
    MCA_OOB_TCP_HDR_NTOH(&hdr);

    /* dispatch based on message type */
    switch(hdr.msg_type) {
        case MCA_OOB_TCP_PROBE:
            mca_oob_tcp_recv_probe(sd, &hdr);
            break;
        case MCA_OOB_TCP_CONNECT:
            mca_oob_tcp_recv_connect(sd, &hdr);
            break;
        default:
            opal_output(0, "%s mca_oob_tcp_recv_handler: invalid message type: %d\n",
                ORTE_NAME_PRINT(ORTE_PROC_MY_NAME), hdr.msg_type);
            CLOSE_THE_SOCKET(sd);
            break;
    }

}


/*
 * Go through a list of argv; if there are any subnet specifications
 * (a.b.c.d/e), resolve them to an interface name (Currently only
 * supporting IPv4).  If unresolvable, warn and remove.
 */
static char **split_and_resolve(char **orig_str, char *name)
{
    int i, ret, save, if_index;
    char **argv, *str, *tmp;
    char if_name[IF_NAMESIZE];
    struct sockaddr_storage argv_inaddr, if_inaddr;
    uint32_t argv_prefix;

    /* Sanity check */
    if (NULL == orig_str || NULL == *orig_str) {
        return NULL;
    }

    argv = opal_argv_split(*orig_str, ',');
    if (NULL == argv) {
        return NULL;
    }
    for (save = i = 0; NULL != argv[i]; ++i) {
        if (isalpha(argv[i][0])) {
            argv[save++] = argv[i];
            continue;
        }

        /* Found a subnet notation.  Convert it to an IP
           address/netmask.  Get the prefix first. */
        argv_prefix = 0;
        tmp = strdup(argv[i]);
        str = strchr(argv[i], '/');
        if (NULL == str) {
            orte_show_help("help-oob-tcp.txt", "invalid if_inexclude",
                           true, name, orte_process_info.nodename, 
                           tmp, "Invalid specification (missing \"/\")");
            free(argv[i]);
            free(tmp);
            continue;
        }
        *str = '\0';
        argv_prefix = atoi(str + 1);

        /* Now convert the IPv4 address */
        ((struct sockaddr*) &argv_inaddr)->sa_family = AF_INET;
        ret = inet_pton(AF_INET, argv[i], 
                        &((struct sockaddr_in*) &argv_inaddr)->sin_addr);
        free(argv[i]);

        if (1 != ret) {
            orte_show_help("help-oob-tcp.txt", "invalid if_inexclude",
                           true, name, orte_process_info.nodename, tmp,
                           "Invalid specification (inet_pton() failed)");
            free(tmp);
            continue;
        }
        opal_output_verbose(20, mca_oob_tcp_output_handle,
                            "%s oob:tcp: Searching for %s address+prefix: %s / %u",
                            ORTE_NAME_PRINT(ORTE_PROC_MY_NAME),
                            name,
                            opal_net_get_hostname((struct sockaddr*) &argv_inaddr),
                            argv_prefix);
            
        /* Go through all interfaces and see if we can find a match */
        for (if_index = opal_ifbegin(); if_index >= 0; 
             if_index = opal_ifnext(if_index)) {
            opal_ifindextoaddr(if_index, 
                               (struct sockaddr*) &if_inaddr,
                               sizeof(if_inaddr));
            if (opal_net_samenetwork((struct sockaddr*) &argv_inaddr,
                                     (struct sockaddr*) &if_inaddr,
                                     argv_prefix)) {
                break;
            }
        }
        
        /* If we didn't find a match, keep trying */
        if (if_index < 0) {
            orte_show_help("help-oob-tcp.txt", "invalid if_inexclude",
                           true, name, orte_process_info.nodename, tmp,
                           "Did not find interface matching this subnet");
            free(tmp);
            continue;
        }

        /* We found a match; get the name and replace it in the
           argv */
        opal_ifindextoname(if_index, if_name, sizeof(if_name));
        opal_output_verbose(20, mca_oob_tcp_output_handle,
                            "%s oob:tcp: Found match: %s (%s)",
                            ORTE_NAME_PRINT(ORTE_PROC_MY_NAME),
                            opal_net_get_hostname((struct sockaddr*) &if_inaddr),
                            if_name);
        argv[save++] = strdup(if_name);
        free(tmp);
    }

    /* The list may have been compressed if there were invalid
       entries, so ensure we end it with a NULL entry */
    argv[save] = NULL;
    free(*orig_str);
    *orig_str = opal_argv_join(argv, ',');
    return argv;
}


/*
 * Component initialization - create a module and initialize the
 * static resources associated with that module.
 *
 * Also initializes the list of devices that will be used/supported by
 * the module, using the if_include and if_exclude variables.  This is
 * the only place that this sorting should occur -- all other places
 * should use the tcp_avaiable_devices list.  This is a change from
 * previous versions of this component.
 */
mca_oob_t* mca_oob_tcp_component_init(int* priority)
{
    int i, rc;
    bool found_local = false;
    bool found_nonlocal = false;
    char **interfaces = NULL;
    mca_oob_tcp_device_t *dev;
    bool including = false, excluding = false;
    char name[32];

    *priority = 50;

    /* are there any interfaces? */
    if (opal_ifcount() <= 0) {
        return NULL;
    }

    /* did someone mistakenly specify both includes AND excludes? */
    if (NULL != mca_oob_tcp_component.tcp_include &&
        NULL != mca_oob_tcp_component.tcp_exclude) {
        orte_show_help("help-oob-tcp.txt", "include-exclude", true,
                       mca_oob_tcp_component.tcp_include,
                       mca_oob_tcp_component.tcp_exclude);
        return NULL;
    }

    /* if interface include was given, construct a list
     * of those interfaces which match the specifications - remember,
     * the includes could be given as named interfaces, IP addrs, or
     * subnet+mask
     */
    if (NULL != mca_oob_tcp_component.tcp_include) {
        interfaces = split_and_resolve(&mca_oob_tcp_component.tcp_include,
                                       "include");
        including = true;
        excluding = false;
    } else if (NULL != mca_oob_tcp_component.tcp_exclude) {
        interfaces = split_and_resolve(&mca_oob_tcp_component.tcp_exclude,
                                       "exclude");
        including = false;
        excluding = true;
    }

    /* look at all available interfaces */ 
    for (i = opal_ifbegin() ; i > 0 ; i = opal_ifnext(i)) {

        /* get the name for diagnostic purposes */
        opal_ifindextoname(i, name, sizeof(name));

        /* handle include/exclude directives */
        if (NULL != interfaces) {
            /* check for match */
            rc = opal_ifmatches(i, interfaces);
            /* if one of the network specifications isn't parseable, then
             * error out as we can't do what was requested
             */
            if (OPAL_ERR_NETWORK_NOT_PARSEABLE == rc) {
                        orte_show_help("help-oob-tcp.txt", "not-parseable", true);
                        opal_argv_free(interfaces);
                        return NULL;
            }
            /* if we are including, then ignore this if not present */
            if (including) {
                if (OPAL_SUCCESS != rc) {
                    opal_output_verbose(20, mca_oob_tcp_output_handle,
                                        "%s oob:tcp:init rejecting interface %s (not in include list)",
                                        ORTE_NAME_PRINT(ORTE_PROC_MY_NAME), name);
                    continue;
                }
            } else {
                /* we are excluding, so ignore if present */
                if (OPAL_SUCCESS == rc) {
                    opal_output_verbose(20, mca_oob_tcp_output_handle,
                                        "%s oob:tcp:init rejecting interface %s (in exclude list)",
                                        ORTE_NAME_PRINT(ORTE_PROC_MY_NAME), name);
                    continue;
                }
            }
        }

        dev = OBJ_NEW(mca_oob_tcp_device_t);
        dev->if_index = i;

        opal_output_verbose(20, mca_oob_tcp_output_handle,
                            "%s oob:tcp:init setting up interface %s",
                            ORTE_NAME_PRINT(ORTE_PROC_MY_NAME), name);
        
        opal_ifindextoaddr(i, (struct sockaddr*) &dev->if_addr, sizeof(struct sockaddr_storage));
        if(opal_net_islocalhost((struct sockaddr*) &dev->if_addr)) {
            dev->if_local = true;
            found_local = true;
        } else {
            dev->if_local = false;
            found_nonlocal = true;
        }

        opal_list_append(&mca_oob_tcp_component.tcp_available_devices,
                         &dev->super);
    }

    /* cleanup */
    if (NULL != interfaces) {
        opal_argv_free(interfaces);
    }

    /* remove all the local devices if we have non-local devices to use. */
    if (found_local && found_nonlocal) {
        opal_list_item_t *item, *next;
        for (item = opal_list_get_first(&mca_oob_tcp_component.tcp_available_devices) ;
             item != opal_list_get_end(&mca_oob_tcp_component.tcp_available_devices) ;
             item = next) {
            mca_oob_tcp_device_t *dev = (mca_oob_tcp_device_t*) item;
            next = opal_list_get_next(item);
            if (dev->if_local) {
                item = opal_list_remove_item(&mca_oob_tcp_component.tcp_available_devices,
                                             item);
                OBJ_RELEASE(dev);
            }
        }
    }

    if (opal_list_get_size(&mca_oob_tcp_component.tcp_available_devices) == 0) {
        if (including) {
            orte_show_help("help-oob-tcp.txt", "no-included-found", true, mca_oob_tcp_component.tcp_include);
        } else if (excluding) {
            orte_show_help("help-oob-tcp.txt", "excluded-all", true, mca_oob_tcp_component.tcp_exclude);
        } else {
            orte_show_help("help-oob-tcp.txt", "no-interfaces-avail", true);
        }
        return NULL;
    }

    /* initialize data structures */
    opal_hash_table_init(&mca_oob_tcp_component.tcp_peers, 128);
    opal_hash_table_init(&mca_oob_tcp_component.tcp_peer_names, 128);

    opal_free_list_init(&mca_oob_tcp_component.tcp_peer_free,
                        sizeof(mca_oob_tcp_peer_t),
                        OBJ_CLASS(mca_oob_tcp_peer_t),
                        8,  /* initial number */
                        mca_oob_tcp_component.tcp_peer_limit, /* maximum number */
                        8);  /* increment to grow by */

    opal_free_list_init(&mca_oob_tcp_component.tcp_msgs,
                        sizeof(mca_oob_tcp_msg_t),
                        OBJ_CLASS(mca_oob_tcp_msg_t),
                        8,  /* initial number */
                        -1,  /* maximum number */
                        8);  /* increment to grow by */


    return &mca_oob_tcp;
}


/*
 * Attempt to resolve peer name.
 */
int mca_oob_tcp_resolve(mca_oob_tcp_peer_t* peer)
{
    mca_oob_tcp_addr_t* addr = NULL;
    int rc=ORTE_ERR_ADDRESSEE_UNKNOWN;

    /* if the address is already cached - simply return it */
    OPAL_THREAD_LOCK(&mca_oob_tcp_component.tcp_lock);
    opal_hash_table_get_value_uint64(&mca_oob_tcp_component.tcp_peer_names,
                                     orte_util_hash_name(&peer->peer_name), (void**)&addr);
    OPAL_THREAD_UNLOCK(&mca_oob_tcp_component.tcp_lock);
    if (NULL != addr) {
        mca_oob_tcp_peer_resolved(peer, addr);
        return ORTE_SUCCESS;
    }

#if ORTE_ENABLE_STATIC_PORTS
    {
        char *host, *haddr=NULL;
        orte_node_rank_t nrank;
        struct hostent *h;
        int port;
        char *uri;

        /* if we don't know it, and we are using static ports, try
         * to compute the address and port
         */
        if (orte_static_ports) {
            if (NULL != (host = orte_get_proc_hostname(&peer->peer_name))) {
                /* lookup the address of this node */
                if (NULL == (h = gethostbyname(host))) {
                    /* this isn't an error - it just means we don't know
                     * how to compute a contact info for this proc.
                     * if we are trying to talk to a process on our own node, try
                     * looking for the loopback interface before giving up
                     */
                    goto unlock;
                }
                if (0 == strcasecmp(host, orte_process_info.nodename) ||
                    0 == strncasecmp(host, orte_process_info.nodename, strlen(host)) ||
                    opal_ifislocal(host)) {
                    int idx;
                    struct sockaddr addr;
                    struct in_addr inaddr;
                    for (idx = opal_ifbegin(); 0 < idx; idx = opal_ifnext(idx)) {
                        if (opal_ifisloopback(idx)) {
                            if (OPAL_SUCCESS != (rc = opal_ifindextoaddr(idx, &addr, sizeof(addr)))) {
                                ORTE_ERROR_LOG(rc);
                                goto unlock;
                            }
                            inaddr = ((struct sockaddr_in*)(&addr))->sin_addr;
                            haddr = inet_ntoa(inaddr);
                            break;
                        }
                    }
                } else {
                    haddr = inet_ntoa(*(struct in_addr*)h->h_addr_list[0]);
                }
            } else {
                goto unlock;
            }
            /* we can't know which af_family we are using, so for now, let's
             * just look to see which static port family was provided
             */
            if (NULL != mca_oob_tcp_component.tcp4_static_ports) {
                if (ORTE_JOBID_IS_DAEMON(peer->peer_name.jobid)) {
                    /* we are trying to talk to a daemon, which will always
                     * be listening on the first port in the range
                     */
                    port = strtol(mca_oob_tcp_component.tcp4_static_ports[0], NULL, 10);
                } else {
                    /* lookup the node rank of the proc */
                    if (ORTE_NODE_RANK_INVALID == (nrank = orte_get_proc_node_rank(&peer->peer_name)) ||
                        (nrank+1) > opal_argv_count(mca_oob_tcp_component.tcp4_static_ports)) {
                        /* this isn't an error - it just means we don't know
                         * how to compute a contact info for this proc
                         */
                        rc = ORTE_ERR_ADDRESSEE_UNKNOWN;
                        goto unlock;
                    }
                    /* any daemon takes the first entry, so we start with the second */
                    port = strtol(mca_oob_tcp_component.tcp4_static_ports[nrank+1], NULL, 10);
                }
                /* create the uri */
                asprintf(&uri, "tcp://%s:%d", (NULL == haddr) ? "NULL" : haddr, port);
#if OPAL_WANT_IPV6
            } else if (NULL != mca_oob_tcp_component.tcp6_static_ports) {
                if (ORTE_JOBID_IS_DAEMON(peer->peer_name.jobid)) {
                    /* we are trying to talk to a daemon, which will always
                     * be listening on the first port in the range
                     */
                    port = strtol(mca_oob_tcp_component.tcp6_static_ports[0], NULL, 10);
                } else {
                    /* lookup the node rank of the proc */
                    
                    if (ORTE_NODE_RANK_INVALID == (nrank = orte_get_proc_node_rank(&peer->peer_name)) ||
                        (nrank+1) > opal_argv_count(mca_oob_tcp_component.tcp6_static_ports)) {
                        /* this isn't an error - it just means we don't know
                         * how to compute a contact info for this proc
                         */
                        rc = ORTE_ERR_ADDRESSEE_UNKNOWN;
                        goto unlock;
                    }
                    /* any daemon takes the first entry, so we start with the second */
                    port = strtol(mca_oob_tcp_component.tcp6_static_ports[nrank+1], NULL, 10);
                }
                /* create the uri */
                asprintf(&uri, "tcp6://%s:%d", (NULL == haddr) ? "NULL" : haddr, port);
#endif  /* OPAL_WANT_IPV6 */
            } else {
                /* can't do anything with this - no idea how static ports got set! */
                rc = ORTE_ERR_ADDRESSEE_UNKNOWN;
                goto unlock;
            }
            /* set the contact info */
            rc = mca_oob_tcp_set_addr(&peer->peer_name, uri);
            /* release memory */
            free(uri);
        }
    }

 unlock:
#endif
    return rc;
}


/*
 * Ready the TCP module for connections.  This includes creating
 * listen sockets for both IPv4 and IPv6 and (possibly) starting the
 * connection listen thread.
 */
int mca_oob_tcp_init(void)
{
    orte_jobid_t jobid;
    int rc;
    int randval = orte_process_info.num_procs;

    if (0 == randval) randval = 10; 

    /* random delay to stagger connections back to seed */
#if defined(__WINDOWS__)
    if(1 == mca_oob_tcp_component.connect_sleep) {
        Sleep((ORTE_PROC_MY_NAME->vpid % randval % 1000) * 100);
    }
#elif defined(HAVE_USLEEP)
    if(1 == mca_oob_tcp_component.connect_sleep) {
        usleep((ORTE_PROC_MY_NAME->vpid % randval % 1000) * 1000);
    }
#endif

    /* get my jobid */
    jobid = ORTE_PROC_MY_NAME->jobid;

    /* Fix up the listen type.  This is the first call into the OOB in
       which the ORTE_PROC_IS_HNP field is reliably set.  The
       listen_mode should only be listen_thread for the HNP -- all
       others should use the traditional event library. */
    if (!ORTE_PROC_IS_HNP) {
        mca_oob_tcp_component.tcp_listen_type = OOB_TCP_EVENT;
    }

    /* Create an IPv4 listen socket and either register with the event
       engine or give to the listen thread */
    rc = mca_oob_tcp_create_listen(&mca_oob_tcp_component.tcp_listen_sd,
                                   &mca_oob_tcp_component.tcp_listen_port,
                                   AF_INET);
    if (ORTE_SUCCESS != rc) {
        /* Don't complain if just not supported unless want connect debugging */
        if (EAFNOSUPPORT != opal_socket_errno ||
            mca_oob_tcp_component.tcp_debug >= OOB_TCP_DEBUG_CONNECT) {
            opal_output(0,
                        "mca_oob_tcp_init: unable to create IPv4 listen socket: %s\n",
                        opal_strerror(rc));
        }
        mca_oob_tcp_component.tcp_listen_sd = -1;
        mca_oob_tcp_component.tcp_listen_port = 0;
    } else {
        if (OOB_TCP_LISTEN_THREAD == mca_oob_tcp_component.tcp_listen_type) {
            int idx = mca_oob_tcp_component.tcp_listen_thread_num_sockets++;
            mca_oob_tcp_component.tcp_listen_thread_sds[idx] = 
                mca_oob_tcp_component.tcp_listen_sd;
        } else {
            if (NULL == mca_oob_tcp_component.tcp_recv_event) {
                mca_oob_tcp_component.tcp_recv_event = opal_event_alloc();
                opal_event_set(orte_event_base, mca_oob_tcp_component.tcp_recv_event,
                               mca_oob_tcp_component.tcp_listen_sd,
                               OPAL_EV_READ|OPAL_EV_PERSIST,
                               mca_oob_tcp_recv_handler,
                               0);
                opal_event_set_priority(mca_oob_tcp_component.tcp_recv_event, ORTE_MSG_PRI);
                opal_event_add(mca_oob_tcp_component.tcp_recv_event, 0);
            }
        }
    }

    /* Create an IPv6 listen socket (if IPv6 is enabled, of course)
       and either register with the event engine or give to the listen
       thread */
#if OPAL_WANT_IPV6
    rc = mca_oob_tcp_create_listen(&mca_oob_tcp_component.tcp6_listen_sd,
                                   &mca_oob_tcp_component.tcp6_listen_port,
                                   AF_INET6);
    if (ORTE_SUCCESS != rc) {
        /* Don't complain if just not supported unless want connect debugging */
        if (EAFNOSUPPORT != opal_socket_errno ||
            mca_oob_tcp_component.tcp_debug >= OOB_TCP_DEBUG_CONNECT) {
            opal_output(0,
                        "mca_oob_tcp_init: unable to create IPv6 listen socket: %s\n",
                        opal_strerror(rc));
        }
        mca_oob_tcp_component.tcp6_listen_sd = -1;
        mca_oob_tcp_component.tcp6_listen_port = 0;
    } else {
        if (OOB_TCP_LISTEN_THREAD == mca_oob_tcp_component.tcp_listen_type) {
            int idx = mca_oob_tcp_component.tcp_listen_thread_num_sockets++;
            mca_oob_tcp_component.tcp_listen_thread_sds[idx] = 
                mca_oob_tcp_component.tcp6_listen_sd;
        } else {
            if (NULL == mca_oob_tcp_component.tcp6_recv_event) {
                mca_oob_tcp_component.tcp6_recv_event = opal_event_alloc();
                opal_event_set(orte_event_base, mca_oob_tcp_component.tcp6_recv_event,
                               mca_oob_tcp_component.tcp6_listen_sd,
                               OPAL_EV_READ|OPAL_EV_PERSIST,
                               mca_oob_tcp_recv_handler,
                               0);
                opal_event_set_priority(mca_oob_tcp_component.tcp6_recv_event, ORTE_MSG_PRI);
                opal_event_add(mca_oob_tcp_component.tcp6_recv_event, 0);
            }
        }
    }
#endif

    if (mca_oob_tcp_component.tcp_listen_sd < 0
#if OPAL_WANT_IPV6
        && mca_oob_tcp_component.tcp6_listen_sd < 0
#endif
        ) {
        return ORTE_ERR_NOT_SUPPORTED;
    }

    /* Finish up by either printing a nice message (event library) or
       initializing the listen thread (listen thread) */
    if (OOB_TCP_LISTEN_THREAD == mca_oob_tcp_component.tcp_listen_type) {
        rc = mca_oob_tcp_create_listen_thread();
        if (ORTE_SUCCESS != rc) {
            opal_output(0, "Unable to create listen thread: %d\n", rc);
            return rc;
        }

        if (mca_oob_tcp_component.tcp_debug >= OOB_TCP_DEBUG_INFO) {
            opal_output(0, "%s accepting connections via listen thread",
                        ORTE_NAME_PRINT(ORTE_PROC_MY_NAME));
        }
    } else {
        if (mca_oob_tcp_component.tcp_debug >= OOB_TCP_DEBUG_INFO) {
            opal_output(0, "%s accepting connections via event library",
                        ORTE_NAME_PRINT(ORTE_PROC_MY_NAME));
        }
    }

    return ORTE_SUCCESS;
}


/*
 * Module cleanup.
 */
int mca_oob_tcp_fini(void)
{
    opal_list_item_t *item;
    void *data;

    OPAL_THREAD_LOCK(&mca_oob_tcp_component.tcp_lock);
#if 0
    opal_event_disable(); /* disable event processing */
#endif
    /* shut down the listening system */
    if (OOB_TCP_LISTEN_THREAD == mca_oob_tcp_component.tcp_listen_type) {
        mca_oob_tcp_component.tcp_shutdown = true;
        opal_thread_join(&mca_oob_tcp_component.tcp_listen_thread, &data);
        if (NULL != mca_oob_tcp_component.tcp_listen_thread_event) {
            opal_event_free(mca_oob_tcp_component.tcp_listen_thread_event);
            mca_oob_tcp_component.tcp_listen_thread_event = NULL;
        }
    } else {
        if (mca_oob_tcp_component.tcp_listen_sd >= 0 &&
               NULL != mca_oob_tcp_component.tcp_recv_event) {
        opal_event_free(mca_oob_tcp_component.tcp_recv_event);
        mca_oob_tcp_component.tcp_recv_event = NULL;
        }
#if OPAL_WANT_IPV6
        if (mca_oob_tcp_component.tcp6_listen_sd >= 0 &&
            NULL != mca_oob_tcp_component.tcp6_recv_event) {
            opal_event_free(mca_oob_tcp_component.tcp6_recv_event);
            mca_oob_tcp_component.tcp6_recv_event = NULL;
        }
#endif
    }

    /* close listen socket */
    if (mca_oob_tcp_component.tcp_listen_sd >= 0) {
        CLOSE_THE_SOCKET(mca_oob_tcp_component.tcp_listen_sd);
        mca_oob_tcp_component.tcp_listen_sd = -1;
    }
#if OPAL_WANT_IPV6
    if (mca_oob_tcp_component.tcp6_listen_sd >= 0) {
        CLOSE_THE_SOCKET(mca_oob_tcp_component.tcp6_listen_sd);
        mca_oob_tcp_component.tcp6_listen_sd = -1;
    }
#endif  /* OPAL_WANT_IPV6 */

    /* cleanup all peers */
    for(item = opal_list_remove_first(&mca_oob_tcp_component.tcp_peer_list);
        item != NULL;
        item = opal_list_remove_first(&mca_oob_tcp_component.tcp_peer_list)) {
        mca_oob_tcp_peer_t* peer = (mca_oob_tcp_peer_t*)item;
        MCA_OOB_TCP_PEER_RETURN(peer);
    }

    /* delete any pending events */
    for( item = opal_list_get_first(&mca_oob_tcp_component.tcp_events);
        item != opal_list_get_end(&mca_oob_tcp_component.tcp_events);
        item = opal_list_get_first(&mca_oob_tcp_component.tcp_events) ) {
        mca_oob_tcp_event_t* event = (mca_oob_tcp_event_t*)item;
        OBJ_RELEASE(event);
    }

    OPAL_THREAD_UNLOCK(&mca_oob_tcp_component.tcp_lock);
    return ORTE_SUCCESS;
}


/*
* Compare two process names for equality.
*
* @param  n1  Process name 1.
* @param  n2  Process name 2.
* @return     (-1 for n1<n2 0 for equality, 1 for n1>n2)
*
* Note that the definition of < or > is somewhat arbitrary -
* just needs to be consistently applied to maintain an ordering
* when process names are used as indices.
*
* Currently, this function is ONLY used in one place - in oob_tcp_send.c to
* determine if the recipient of the message-to-be-sent is ourselves. Hence,
* this comparison is okay to be LITERAL and can/should use the ns.compare_fields
* function
*/


int mca_oob_tcp_process_name_compare(const orte_process_name_t* n1, const orte_process_name_t* n2)
{
    return orte_util_compare_name_fields(ORTE_NS_CMP_ALL, n1, n2);
}


/*
* Return local process address as a URI string.
*/

char* mca_oob_tcp_get_addr(void)
{
    char *contact_info = (char *) malloc(opal_list_get_size(&mca_oob_tcp_component.tcp_available_devices) * 128);
    char *ptr = contact_info;
    opal_list_item_t *item;
    *ptr = 0;

    for (item = opal_list_get_first(&mca_oob_tcp_component.tcp_available_devices) ;
         item != opal_list_get_end(&mca_oob_tcp_component.tcp_available_devices) ;
         item = opal_list_get_next(item)) {
        mca_oob_tcp_device_t *dev = (mca_oob_tcp_device_t*) item;

        if (ptr != contact_info) {
            ptr += sprintf(ptr, ";");
        }

        if (dev->if_addr.ss_family == AF_INET &&
            4 != mca_oob_tcp_component.disable_family) {
            ptr += sprintf(ptr, "tcp://%s:%d", opal_net_get_hostname((struct sockaddr*) &dev->if_addr),
                           ntohs(mca_oob_tcp_component.tcp_listen_port));
        }
#if OPAL_WANT_IPV6
        if (dev->if_addr.ss_family == AF_INET6 &&
            6 != mca_oob_tcp_component.disable_family) {
            ptr += sprintf(ptr, "tcp6://%s:%d", opal_net_get_hostname((struct sockaddr*) &dev->if_addr),
                           ntohs(mca_oob_tcp_component.tcp6_listen_port));
        }
#endif  /* OPAL_WANT_IPV6 */
    }
    return contact_info;
}

/*
* Parse a URI string into an IP address and port number.
*/

int
mca_oob_tcp_parse_uri(const char* uri, struct sockaddr* inaddr)
{
    char *dup_uri = strdup(uri);
    char *host, *port;
    uint16_t af_family = AF_UNSPEC;
    int ret;
#if OPAL_WANT_IPV6
    struct addrinfo hints, *res;
#endif

    if (NULL == dup_uri) return ORTE_ERR_OUT_OF_RESOURCE;

    if (strncmp(dup_uri, "tcp6://", strlen("tcp6://")) == 0) {
#if OPAL_WANT_IPV6
        af_family = AF_INET6;
        host = dup_uri + strlen("tcp6://");
#else
        ret = ORTE_ERR_NOT_SUPPORTED;
        goto cleanup;
#endif
    } else if (strncmp(dup_uri, "tcp://", strlen("tcp://")) == 0) {
        af_family = AF_INET;
        host = dup_uri + strlen("tcp://");
    } else {
        ret = ORTE_ERR_BAD_PARAM;
        goto cleanup;
    }
    
    /* mutate the host string so that the port number is not in the
       same string as the host. */
    port = strrchr(host, ':');
    if (NULL == port) {
        ret = ORTE_ERR_BAD_PARAM;
        goto cleanup;
    }
    *port = '\0';
    port++;

    switch (af_family) {
    case AF_INET:
        memset(inaddr, 0, sizeof(struct sockaddr_in));
        break;
    case AF_INET6:
        memset(inaddr, 0, sizeof(struct sockaddr_in6));
        break;
    default:
        ret = ORTE_ERR_BAD_PARAM;
        goto cleanup;
    }

#if OPAL_WANT_IPV6
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = af_family;
    hints.ai_socktype = SOCK_STREAM;
    ret = getaddrinfo (host, NULL, &hints, &res);

    if (ret) {
        opal_output (0, "oob_tcp_parse_uri: Could not resolve %s. [Error: %s]\n",
                     host, gai_strerror (ret));
        ret = ORTE_ERR_BAD_PARAM;
        goto cleanup;
    }
        
    if (res->ai_family != af_family) {
        /* should never happen */
        opal_output (0, "oob_tcp_parse_uri: getaddrinfo returned wrong af_family for %s",
                     host);
        ret = ORTE_ERROR;
        goto cleanup;
    }
        
    memcpy(inaddr, res->ai_addr, res->ai_addrlen);
    freeaddrinfo(res);
#else
    if (AF_INET == af_family) {
        struct sockaddr_in *in = (struct sockaddr_in*) inaddr;
        in->sin_family = af_family;
        in->sin_addr.s_addr = inet_addr(host);
        if (in->sin_addr.s_addr == INADDR_ANY) {
            ret = ORTE_ERR_BAD_PARAM;
            goto cleanup;
        }
    } else {
        ret = ORTE_ERR_BAD_PARAM;
        goto cleanup;
    }
#endif

    switch (af_family) {
    case AF_INET:
        ((struct sockaddr_in*) inaddr)->sin_port = htons(atoi(port));
        break;
    case AF_INET6:
        ((struct sockaddr_in6*) inaddr)->sin6_port = htons(atoi(port));
        break;
    default:
        ret = ORTE_ERR_BAD_PARAM;
        goto cleanup;
    }

    ret = ORTE_SUCCESS;

 cleanup:
    if (NULL != dup_uri) free(dup_uri);
    return ret;
}


/*
 * Setup address in the cache. Note that this could be called multiple
 * times if a given destination exports multiple addresses.
 */

int mca_oob_tcp_set_addr(const orte_process_name_t* name, const char* uri)
{
    struct sockaddr_storage inaddr;
    mca_oob_tcp_addr_t* addr = NULL;
    mca_oob_tcp_peer_t* peer = NULL;
    opal_list_item_t *item;
    int rc;
    
    if (NULL == uri) {
        /* purge the hash table entry for this proc */
        OPAL_THREAD_LOCK(&mca_oob_tcp_component.tcp_lock);
        /* get the peer object */
        opal_hash_table_get_value_uint64(&mca_oob_tcp_component.tcp_peers,
                                         orte_util_hash_name(name), 
                                         (void**)&peer);
        if (NULL != peer) {
            OPAL_THREAD_LOCK(&peer->peer_lock);
            /* flag the state as closed */
            peer->peer_state = MCA_OOB_TCP_CLOSED;
            /* clear any pending sends */
            while (NULL != (item = opal_list_remove_first(&peer->peer_send_queue))) {
                OBJ_RELEASE(item);
            }
            peer->peer_send_msg = NULL;
            /* clear any pending recvs */
            peer->peer_recv_msg = NULL;
            OPAL_THREAD_UNLOCK(&peer->peer_lock);
            mca_oob_tcp_peer_shutdown(peer);
        }
        /* delete the entry from the hash table */
        opal_hash_table_set_value_uint64(&mca_oob_tcp_component.tcp_peer_names,
                                         orte_util_hash_name(name), NULL);
        /* all done */
        OPAL_THREAD_UNLOCK(&mca_oob_tcp_component.tcp_lock);
        return ORTE_SUCCESS;
    }
    
    if((rc = mca_oob_tcp_parse_uri(uri, (struct sockaddr*) &inaddr)) != ORTE_SUCCESS) {
        return rc;
    }

    OPAL_THREAD_LOCK(&mca_oob_tcp_component.tcp_lock);
    opal_hash_table_get_value_uint64(&mca_oob_tcp_component.tcp_peer_names,
                                     orte_util_hash_name(name), (void**)&addr);
    if(NULL == addr) {
        addr = OBJ_NEW(mca_oob_tcp_addr_t);
        addr->addr_name = *name;
        opal_hash_table_set_value_uint64(&mca_oob_tcp_component.tcp_peer_names,
                                         orte_util_hash_name(&addr->addr_name), addr);
    }
    rc = mca_oob_tcp_addr_insert(addr, (struct sockaddr*) &inaddr);
    opal_hash_table_get_value_uint64(&mca_oob_tcp_component.tcp_peers,
                                     orte_util_hash_name(&addr->addr_name), 
                                     (void**)&peer);
    if(NULL != peer) {
        mca_oob_tcp_peer_resolved(peer, addr);
    }
    OPAL_THREAD_UNLOCK(&mca_oob_tcp_component.tcp_lock);
    return rc;
}


/* Dummy function for when we are not using FT. */
#if OPAL_ENABLE_FT_CR == 0
int mca_oob_tcp_ft_event(int state) {
    return ORTE_SUCCESS;
}
#else
int mca_oob_tcp_ft_event(int state) {
    int  exit_status = ORTE_SUCCESS;
    opal_list_item_t *item;

    if(OPAL_CRS_CHECKPOINT == state) {
#if 0
        /*
         * Disable event processing while we are working
         */
        OPAL_THREAD_LOCK(&mca_oob_tcp_component.tcp_lock);
        opal_event_disable();
        OPAL_THREAD_UNLOCK(&mca_oob_tcp_component.tcp_lock);
#endif
    }
    else if(OPAL_CRS_CONTINUE == state) {
#if 0
        /*
         * Resume event processing
         */
        OPAL_THREAD_LOCK(&mca_oob_tcp_component.tcp_lock);
        opal_event_enable();
        OPAL_THREAD_UNLOCK(&mca_oob_tcp_component.tcp_lock);
#endif
    }
    else if(OPAL_CRS_RESTART == state) {
        /*
         * Clean out cached connection information
         * Select pieces of finalize/init
         */
        for(item = opal_list_remove_first(&mca_oob_tcp_component.tcp_peer_list);
            item != NULL;
            item = opal_list_remove_first(&mca_oob_tcp_component.tcp_peer_list)) {
            mca_oob_tcp_peer_t* peer = (mca_oob_tcp_peer_t*)item;
            /* JJH: Use the below command for debugging restarts with invalid sockets
             * mca_oob_tcp_peer_dump(peer, "RESTART CLEAN")
             */
            MCA_OOB_TCP_PEER_RETURN(peer);
        }

        OPAL_THREAD_LOCK(&mca_oob_tcp_component.tcp_lock);

        OBJ_DESTRUCT(&mca_oob_tcp_component.tcp_peer_free);
        OBJ_DESTRUCT(&mca_oob_tcp_component.tcp_peer_names);
        OBJ_DESTRUCT(&mca_oob_tcp_component.tcp_peers);
        OBJ_DESTRUCT(&mca_oob_tcp_component.tcp_peer_list);

        OBJ_CONSTRUCT(&mca_oob_tcp_component.tcp_peer_list,     opal_list_t);
        OBJ_CONSTRUCT(&mca_oob_tcp_component.tcp_peers,         opal_hash_table_t);
        OBJ_CONSTRUCT(&mca_oob_tcp_component.tcp_peer_names,    opal_hash_table_t);
        OBJ_CONSTRUCT(&mca_oob_tcp_component.tcp_peer_free,     opal_free_list_t);

        /*
         * Resume event processing
         */
#if 0
        opal_event_enable();
#endif
        OPAL_THREAD_UNLOCK(&mca_oob_tcp_component.tcp_lock);
    }
    else if(OPAL_CRS_TERM == state ) {
        ;
    }
    else {
        ;
    }

    return exit_status;
}
#endif
