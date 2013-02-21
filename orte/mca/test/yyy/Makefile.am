#
# Copyright (c) 2012      Los Alamos National Security, LLC.  All rights reserved.
# $COPYRIGHT$
# 
# Additional copyrights may follow
# 
# $HEADER$
#

sources = \
        test_yyy.h \
        test_yyy_component.c \
        test_yyy.c

# Make the output library in this directory, and name it either
# mca_<type>_<name>.la (for DSO builds) or libmca_<type>_<name>.la
# (for static builds).

if MCA_BUILD_orte_test_yyy_DSO
component_noinst =
component_install = mca_test_yyy.la
else
component_noinst = libmca_test_yyy.la
component_install =
endif

mcacomponentdir = $(pkglibdir)
mcacomponent_LTLIBRARIES = $(component_install)
mca_test_yyy_la_SOURCES = $(sources)
mca_test_yyy_la_LDFLAGS = -module -avoid-version

noinst_LTLIBRARIES = $(component_noinst)
libmca_test_yyy_la_SOURCES =$(sources)
libmca_test_yyy_la_LDFLAGS = -module -avoid-version
