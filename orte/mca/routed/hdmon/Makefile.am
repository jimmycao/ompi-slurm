#
# Copyright (c) 2012      Los Alamos National Security, LLC.
#                         All rights reserved. 
# $COPYRIGHT$
# 
# Additional copyrights may follow
# 
# $HEADER$
#

sources = \
	routed_hdmon.h \
	routed_hdmon.c \
	routed_hdmon_component.c

# Make the output library in this directory, and name it either
# mca_<type>_<name>.la (for DSO builds) or libmca_<type>_<name>.la
# (for static builds).

if MCA_BUILD_orte_routed_hdmon_DSO
component_noinst =
component_install = mca_routed_hdmon.la
else
component_noinst = libmca_routed_hdmon.la
component_install =
endif

mcacomponentdir = $(pkglibdir)
mcacomponent_LTLIBRARIES = $(component_install)
mca_routed_hdmon_la_SOURCES = $(sources)
mca_routed_hdmon_la_LDFLAGS = -module -avoid-version

noinst_LTLIBRARIES = $(component_noinst)
libmca_routed_hdmon_la_SOURCES = $(sources)
libmca_routed_hdmon_la_LDFLAGS = -module -avoid-version

