AM_CPPFLAGS = -I$(top_builddir)/otflib -I$(top_srcdir)/otflib

if AMBUILDBINARIES
bin_PROGRAMS = \
	otfconfig
endif

otfconfig_LDADD = $(top_builddir)/otflib/libotf.la
otfconfig_DEPENDENCIES = $(otfconfig_LDADD)
otfconfig_SOURCES = \
	otfconfig.c

AM_CFLAGS = -DOTFCONFIG_INCLUDEDIR="\"$(includedir)\"" -DOTFCONFIG_LIBDIR="\"$(libdir)\"" -DOTFCONFIG_LIBS="\"-lotfaux -lotf $(ZLIB_LIB_LINE) $(ZOIDFS_LIB_LINE)\""
