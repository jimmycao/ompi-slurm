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
 * Copyright (c) 2007      Los Alamos National Security, LLC.  All rights
 *                         reserved. 
 * Copyright (c) 2009      Cisco Systems, Inc.  All rights reserved.
 * $COPYRIGHT$
 * 
 * Additional copyrights may follow
 * 
 * $HEADER$
 */

#include "opal_config.h"

#include <string.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <errno.h>
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#ifdef HAVE_SYS_SOCKIO_H
#include <sys/sockio.h>
#endif
#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#ifdef HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif
#ifdef HAVE_NET_IF_H
#if defined(__APPLE__) && defined(_LP64)
/* Apple engineering suggested using options align=power as a
   workaround for a bug in OS X 10.4 (Tiger) that prevented ioctl(...,
   SIOCGIFCONF, ...) from working properly in 64 bit mode on Power PC.
   It turns out that the underlying issue is the size of struct
   ifconf, which the kernel expects to be 12 and natural 64 bit
   alignment would make 16.  The same bug appears in 64 bit mode on
   Intel macs, but align=power is a no-op there, so instead, use the
   pack pragma to instruct the compiler to pack on 4 byte words, which
   has the same effect as align=power for our needs and works on both
   Intel and Power PC Macs. */
#pragma pack(push,4)
#endif
#include <net/if.h>
#if defined(__APPLE__) && defined(_LP64)
#pragma pack(pop)
#endif
#endif
#ifdef HAVE_NETDB_H
#include <netdb.h>
#endif
#ifdef HAVE_IFADDRS_H
#include <ifaddrs.h>
#endif

#include "opal/util/net.h"
#include "opal/util/output.h"
#include "opal/util/argv.h"
#include "opal/util/show_help.h"
#include "opal/constants.h"
#include "opal/threads/tsd.h"
#include "opal/mca/base/mca_base_param.h"

/* this function doesn't depend on sockaddr_h */
bool opal_net_isaddr(const char *name)
{
    unsigned int groups[8];
    int i;

    if (4 != sscanf(name, "%u.%u.%u.%u",
                    &groups[0], &groups[1], &groups[2], &groups[3])) {
        /* this isn't an IPv4 address */
        goto checkipv6;
    }

    for (i=0; i < 4; i++) {
        if (255 < groups[i]) {
            return false;
        }
    }
    return true;

 checkipv6:
    /* TODO: deal with all the shorthand notations for IPv6! */
    if (8 != sscanf(name, "%x:%x:%x:%x:%x:%x:%x:%x",
                    &groups[0], &groups[1], &groups[2], &groups[3],
                    &groups[4], &groups[5], &groups[6], &groups[7])) {
        /* this isn't an IPv6 address */
        return false;
    }
    /* there are no value limits on the individual groups */
    return true;
}

#ifdef HAVE_STRUCT_SOCKADDR_IN

typedef struct private_ipv4_t {
    in_addr_t addr;
    uint32_t netmask_bits;
} private_ipv4_t;

static private_ipv4_t* private_ipv4 = NULL;

#if OPAL_WANT_IPV6
static opal_tsd_key_t hostname_tsd_key;


static void
hostname_cleanup(void *value)
{
    if (NULL != value) free(value);
}


static char*
get_hostname_buffer(void)
{
    void *buffer;
    int ret;

    ret = opal_tsd_getspecific(hostname_tsd_key, &buffer);
    if (OPAL_SUCCESS != ret) return NULL;

    if (NULL == buffer) {
        buffer = (void*) malloc((NI_MAXHOST + 1) * sizeof(char));
        ret = opal_tsd_setspecific(hostname_tsd_key, buffer);
    }

    return (char*) buffer;
}
#endif

int
opal_net_init()
{
    char *string_value, **args, *arg;
    uint32_t a, b, c, d, bits, addr;
    int i, count, found_bad = 0;

    /* RFC1918 defines
       - 10.0.0./8
       - 172.16.0.0/12
       - 192.168.0.0/16
       
       RFC3330 also mentiones
       - 169.254.0.0/16 for DHCP onlink iff there's no DHCP server
    */
    mca_base_param_reg_string_name( "opal", "net_private_ipv4",
                                    "Semicolon-delimited list of CIDR notation entries specifying what networks are considered \"private\" (default value based on RFC1918 and RFC3330)",
                                    false, false, "10.0.0.0/8;172.16.0.0/12;192.168.0.0/16;169.254.0.0/16",
                                    &string_value );

    args = opal_argv_split( string_value, ';' );
    free(string_value);
    if( NULL != args ) {
        count = opal_argv_count(args);
        private_ipv4 = (private_ipv4_t*)malloc( (count + 1) * sizeof(private_ipv4_t));
        if( NULL == private_ipv4 ) {
            opal_output(0, "Unable to allocate memory for the private addresses array" );
            goto do_local_init;
        }
        for( i = 0; i < count; i++ ) {
            arg = args[i];

            sscanf( arg, "%u.%u.%u.%u/%u", &a, &b, &c, &d, &bits );

            if( (a > 255) || (b > 255) || (c > 255) || (d > 255) || (bits > 32) ) {
                /* TODO: A reminder to change this to OPAL SOS once this framework
                 * get added to the trunk.
                 */
                if (0 == found_bad) {
                    opal_show_help("help-opal-util.txt", 
                                   "malformed net_private_ipv4",
                                   true, args[i]);
                    found_bad = 1;
                }
                continue;
            }
            addr = (a << 24) | (b << 16) | (c << 8) | d;
            private_ipv4[i].addr = htonl(addr);
            private_ipv4[i].netmask_bits = bits;
        }
        private_ipv4[i].addr         = 0;
        private_ipv4[i].netmask_bits = 0;
        opal_argv_free(args);
    }

 do_local_init:
#if OPAL_WANT_IPV6
    return opal_tsd_key_create(&hostname_tsd_key, hostname_cleanup);
#else
    return OPAL_SUCCESS;
#endif
}


int
opal_net_finalize()
{
    free(private_ipv4);
    private_ipv4 = NULL;

    return OPAL_SUCCESS;
}


/* convert a CIDR prefixlen to netmask (in network byte order) */
uint32_t
opal_net_prefix2netmask(uint32_t prefixlen)
{
    return htonl (((1 << prefixlen) - 1) << (32 - prefixlen));
}


bool
opal_net_islocalhost(const struct sockaddr *addr)
{
    switch (addr->sa_family) {
    case AF_INET:
        {
            const struct sockaddr_in *inaddr = (struct sockaddr_in*) addr;
            /* if it's in the 127. domain, it shouldn't be routed
               (0x7f == 127) */
            if (0x7F000000 == (0x7F000000 & ntohl(inaddr->sin_addr.s_addr))) {
                return true;
            }
            return false;
        }
        break;
#if OPAL_WANT_IPV6
    case AF_INET6:
        {
            const struct sockaddr_in6 *inaddr = (struct sockaddr_in6*) addr;
            if (IN6_IS_ADDR_LOOPBACK (&inaddr->sin6_addr)) {
               return true; /* Bug, FIXME: check for 127.0.0.1/8 */
            }
            return false;
        }
        break;
#endif

    default:
        opal_output(0, "unhandled sa_family %d passed to opal_net_islocalhost",
                    addr->sa_family);
        return false;
        break;
    }
}


bool
opal_net_samenetwork(const struct sockaddr *addr1, 
                     const struct sockaddr *addr2,
                     uint32_t prefixlen)
{
    if(addr1->sa_family != addr2->sa_family) {
        return false; /* address families must be equal */
    }
    
    switch (addr1->sa_family) {
    case AF_INET:
        {
            const struct sockaddr_in *inaddr1 = (struct sockaddr_in*) addr1;
            const struct sockaddr_in *inaddr2 = (struct sockaddr_in*) addr2;
            uint32_t netmask = opal_net_prefix2netmask (prefixlen);

            if((inaddr1->sin_addr.s_addr & netmask) ==
              (inaddr2->sin_addr.s_addr & netmask)) {
                return true;
            }
            return false;
        }
        break;
#if OPAL_WANT_IPV6
    case AF_INET6:
        {
            const struct sockaddr_in6 *inaddr1 = (struct sockaddr_in6*) addr1;
            const struct sockaddr_in6 *inaddr2 = (struct sockaddr_in6*) addr2;
            struct in6_addr *a6_1 = (struct in6_addr*) &inaddr1->sin6_addr;
            struct in6_addr *a6_2 = (struct in6_addr*) &inaddr2->sin6_addr;

            if (64 == prefixlen) {
                /* prefixlen is always /64, any other case would be routing.
                   Compare the first eight bytes (64 bits) and hope that
                   endianess is not an issue on any system as long as
                   addresses are always stored in network byte order.
                */
                if (((const uint32_t *) (a6_1))[0] ==
                    ((const uint32_t *) (a6_2))[0] &&
                    ((const uint32_t *) (a6_1))[1] ==
                    ((const uint32_t *) (a6_2))[1]) {
                    return true;
                }
            }
            return false;
        }
        break;
#endif
    default:
        opal_output(0, "unhandled sa_family %d passed to opal_samenetwork",
                    addr1->sa_family);
    }

    return false;
}


/**
 * Returns true if the given address is a public IPv4 address.
 */
bool
opal_net_addr_isipv4public(const struct sockaddr *addr)
{
    switch (addr->sa_family) {
#if OPAL_WANT_IPV6
        case AF_INET6:
            return false;
#endif
        case AF_INET:
            {
                const struct sockaddr_in *inaddr = (struct sockaddr_in*) addr;
                int i;
                
                if( NULL == private_ipv4 ) {
                    return true;
                }
                
                for( i = 0; private_ipv4[i].addr != 0; i++ ) {
                    if( private_ipv4[i].addr == (inaddr->sin_addr.s_addr &
                                                 opal_net_prefix2netmask(private_ipv4[i].netmask_bits)) )
                        return false;
                }
                
            }
            return true;
        default:
            opal_output (0,
                         "unhandled sa_family %d passed to opal_net_addr_isipv4public\n",
                         addr->sa_family);
    }
    
    return false;
}


char*
opal_net_get_hostname(const struct sockaddr *addr)
{
#if OPAL_WANT_IPV6
    char *name = get_hostname_buffer();
    int error;
    socklen_t addrlen;

    if (NULL == name) {
        opal_output(0, "opal_sockaddr2str: malloc() failed\n");
        return NULL;
    }
    OPAL_DEBUG_ZERO(*name);

    switch (addr->sa_family) {
    case AF_INET:
        addrlen = sizeof (struct sockaddr_in);
        break;
    case AF_INET6:
#if defined( __NetBSD__)         
        /* hotfix for netbsd: on my netbsd machine, getnameinfo
           returns an unkown error code. */
        if(NULL == inet_ntop(AF_INET6, &((struct sockaddr_in6*) addr)->sin6_addr,
                             name, NI_MAXHOST)) {
            opal_output(0, "opal_sockaddr2str failed with error code %d", errno);
            free(name);
            return NULL;
        }
        return name;
#else
        addrlen = sizeof (struct sockaddr_in6);
#endif
        break;
    default:
        free(name);
        return NULL;
    }

    error = getnameinfo(addr, addrlen,
                        name, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);

    if (error) {
       int err = errno;
       opal_output (0, "opal_sockaddr2str failed:%s (return code %i)\n",
                    gai_strerror(err), error);
       free (name);
       return NULL;
    }

    return name;
#else
    return inet_ntoa(((struct sockaddr_in*) addr)->sin_addr);
#endif
}


int
opal_net_get_port(const struct sockaddr *addr)
{
    switch (addr->sa_family) {
    case AF_INET:
        return ntohs(((struct sockaddr_in*) addr)->sin_port);
        break;
#if OPAL_WANT_IPV6
    case AF_INET6:
        return ntohs(((struct sockaddr_in6*) addr)->sin6_port);
        break;
#endif
    }

    return -1;
}


#else /* HAVE_STRUCT_SOCKADDR_IN */

int
opal_net_init()
{
    return OPAL_SUCCESS;
}


int
opal_net_finalize()
{
    return OPAL_SUCCESS;
}


uint32_t
opal_net_prefix2netmask(uint32_t prefixlen)
{
    return 0;
}


bool
opal_net_islocalhost(const struct sockaddr *addr)
{
    return false;
}


bool
opal_net_samenetwork(const struct sockaddr *addr1, 
                     const struct sockaddr *addr2,
                     uint32_t prefixlen)
{
    return false;
}


bool
opal_net_addr_isipv4public(const struct sockaddr *addr)
{
    return false;
}


char*
opal_net_get_hostname(const struct sockaddr *addr)
{
    return NULL;
}


int
opal_net_get_port(const struct sockaddr *addr)
{
    return -1;
}

#endif /* HAVE_STRUCT_SOCKADDR_IN */
