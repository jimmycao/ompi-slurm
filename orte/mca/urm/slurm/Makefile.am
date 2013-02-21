#
# Copyright (c) 2012      Los Alamos National Security, LLC.  All rights reserved.
# $COPYRIGHT$
# 
# Additional copyrights may follow
# 
# $HEADER$
#

dist_pkgdata_DATA = help-urm-slurm.txt

sources = \
        urm_slurm.h \
        urm_slurm_component.c \
        urm_slurm.c

# Make the output library in this directory, and name it either
# mca_<type>_<name>.la (for DSO builds) or libmca_<type>_<name>.la
# (for static builds).

if MCA_BUILD_orte_urm_slurm_DSO
component_noinst =
component_install = mca_urm_slurm.la
else
component_noinst = libmca_urm_slurm.la
component_install =
endif

mcacomponentdir = $(pkglibdir)
mcacomponent_LTLIBRARIES = $(component_install)
mca_urm_slurm_la_SOURCES = $(sources)
mca_urm_slurm_la_LDFLAGS = -module -avoid-version

noinst_LTLIBRARIES = $(component_noinst)
libmca_urm_slurm_la_SOURCES =$(sources)
libmca_urm_slurm_la_LDFLAGS = -module -avoid-version
