dnl -*- shell-script -*-
dnl
dnl Copyright (c) 2004-2007 The Trustees of Indiana University and Indiana
dnl                         University Research and Technology
dnl                         Corporation.  All rights reserved.
dnl Copyright (c) 2004-2005 The University of Tennessee and The University
dnl                         of Tennessee Research Foundation.  All rights
dnl                         reserved.
dnl Copyright (c) 2004-2005 High Performance Computing Center Stuttgart,
dnl                         University of Stuttgart.  All rights reserved.
dnl Copyright (c) 2004-2005 The Regents of the University of California.
dnl                         All rights reserved.
dnl Copyright (c) 2006-2010 Cisco Systems, Inc.  All rights reserved.
dnl Copyright (c) 2007      Sun Microsystems, Inc.  All rights reserved.
dnl Copyright (c) 2009      IBM Corporation.  All rights reserved.
dnl Copyright (c) 2009-2012 Los Alamos National Security, LLC.  All rights
dnl                         reserved.
dnl Copyright (c) 2009      Oak Ridge National Labs.  All rights reserved.
dnl
dnl $COPYRIGHT$
dnl 
dnl Additional copyrights may follow
dnl 
dnl $HEADER$
dnl


AC_DEFUN([ORTE_CONFIGURE_OPTIONS],[
ompi_show_subtitle "ORTE Configuration options"

#
# Minimal RTE support
#
AC_MSG_CHECKING([if want full RTE support])
AC_ARG_WITH([rte-support],
    [AC_HELP_STRING([--without-rte-support],
                    [Build without RTE support for systems that do not require it (default: full RTE support built)])])
if test "$with_rte_support" = "no"; then
    AC_MSG_RESULT([no])
    orte_without_full_support=1
    list_of_frameworks="db,errmgr,ess-singleton,ess-hnp,ess-tool,ess-env,filem,grpcomm-basic,grpcomm-bad,iof,odls,oob,plm,ras,urm,test,rmaps,rml,routed,snapc,btl-sm,coll-sm,common-sm,mpool-sm,dpm-orte,pubsub-orte,rmcast,routed"
    if test -z $enable_mca_no_build ; then
      enable_mca_no_build="$list_of_frameworks"
    else
      enable_mca_no_build="$enable_mca_no_build,$list_of_frameworks"
    fi
else
    AC_MSG_RESULT([yes])
    orte_without_full_support=0
fi
AC_DEFINE_UNQUOTED([ORTE_DISABLE_FULL_SUPPORT], [$orte_without_full_support],
                   [Build full RTE support])
AM_CONDITIONAL(ORTE_DISABLE_FULL_SUPPORT, test "$with_rte_support" = "no")


#
# Do we want orterun's --prefix behavior to be enabled by default?
#
AC_MSG_CHECKING([if want orterun "--prefix" behavior to be enabled by default])
AC_ARG_ENABLE([orterun-prefix-by-default],
    [AC_HELP_STRING([--enable-orterun-prefix-by-default],
        [Make "orterun ..." behave exactly the same as "orterun --prefix \$prefix" (where \$prefix is the value given to --prefix in configure)])])
AC_ARG_ENABLE([mpirun-prefix-by-default],
    [AC_HELP_STRING([--enable-mpirun-prefix-by-default],
        [Synonym for --enable-orterun-prefix-by-default])])
if test "$enable_orterun_prefix_by_default" = ""; then
    enable_orterun_prefix_by_default=$enable_mpirun_prefix_by_default
fi
if test "$enable_orterun_prefix_by_default" = "yes"; then
    AC_MSG_RESULT([yes])
    orte_want_orterun_prefix_by_default=1
else
    AC_MSG_RESULT([no])
    orte_want_orterun_prefix_by_default=0
fi
AC_DEFINE_UNQUOTED([ORTE_WANT_ORTERUN_PREFIX_BY_DEFAULT],
                   [$orte_want_orterun_prefix_by_default],
                   [Whether we want orterun to effect "--prefix $prefix" by default])

#
# Do we want sensors enabled?

AC_MSG_CHECKING([if want sensors])
AC_ARG_ENABLE([sensors],
    [AC_HELP_STRING([--enable-sensors],
                    [Enable internal sensors (default: disabled)])])
if test "$enable_sensors" = "yes"; then
    AC_MSG_RESULT([yes])
    orte_want_sensors=1
else
    AC_MSG_RESULT([no])
    orte_want_sensors=0
fi
AC_DEFINE_UNQUOTED([ORTE_ENABLE_SENSORS],
                   [$orte_want_sensors],
                   [Whether we want sensors enabled])

#
# Do we want daemon heartbeats enabled?

AC_MSG_CHECKING([if want daemon heartbeats])
AC_ARG_ENABLE([heartbeat],
    [AC_HELP_STRING([--enable-heartbeat],
                    [Enable heartbeat monitoring of daemons (default: disabled)])])
if test "$enable_heartbeat" = "yes"; then
    AC_MSG_RESULT([yes])
    orte_want_heartbeats=1
else
    AC_MSG_RESULT([no])
    orte_want_heartbeats=0
fi
AC_DEFINE_UNQUOTED([ORTE_ENABLE_HEARTBEAT],
                   [$orte_want_heartbeats],
                   [Whether we want daemon heartbeat monitoring enabled])

#
# Do we want a separate orte progress thread?
AC_MSG_CHECKING([if want orte progress threads])
AC_ARG_ENABLE([orte-progress-threads],
              [AC_HELP_STRING([--enable-orte-progress-threads],
              [Enable orte progress thread - for experiment by developers only! (default: disabled)])])
if test "$enable_orte_progress_threads" = "yes"; then
    AC_MSG_RESULT([yes])
    orte_enable_progress_threads=1
    # require libevent thread support
    if test "$enable_event_thread_support" != "yes" ; then
        AC_MSG_WARN([ORTE progress threads require libevent thread])
        AC_MSG_WARN([be enabled. Please configure with])
        AC_MSG_WARN([--enable-event-thread-support])
        AC_MSG_ERROR([Cannot continue])
    fi
else
    AC_MSG_RESULT([no])
    orte_enable_progress_threads=0
fi
AC_DEFINE_UNQUOTED([ORTE_ENABLE_PROGRESS_THREADS],
                   [$orte_enable_progress_threads],
                   [Whether we want orte progress threads enabled])

AC_MSG_CHECKING([if want orte static ports])
AC_ARG_ENABLE([orte-static-ports],
              [AC_HELP_STRING([--enable-orte-static-ports],
	        [Enable orte static ports for tcp oob. (default: enabled)])])
if test "$enable_orte_static_ports" = "no"; then
    AC_MSG_RESULT([no])
    orte_enable_static_ports=0
else
    AC_MSG_RESULT([yes])
    orte_enable_static_ports=1
fi
AC_DEFINE_UNQUOTED([ORTE_ENABLE_STATIC_PORTS],
                   [$orte_enable_static_ports],
		   [Whether we want static ports enabled])

])dnl
