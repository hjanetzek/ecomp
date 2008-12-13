## Process this file with automake to produce Makefile.in
PFLAGS=-module -avoid-version -no-undefined

#if CUBEREFLEX_PLUGIN
libcubereflex_la_LDFLAGS = $(PFLAGS)
libcubereflex_la_LIBADD = @ECOMP_LIBS@
libcubereflex_la_SOURCES = cubereflex_options.c cubereflex_options.h cubereflex.c
#endif

INCLUDES =                              \
	@ECOMP_CFLAGS@                  \
	-DDATADIR='"$(compdatadir)"'        \
	-DLIBDIR='"$(libdir)"'              \
	-DLOCALEDIR="\"@datadir@/locale\""  \
	-DIMAGEDIR='"$(imagedir)"'          \
	-I$(top_srcdir)/include

moduledir = $(plugindir)

#if CUBEREFLEX_PLUGIN
module_LTLIBRARIES = libcubereflex.la
#endif

CLEANFILES = *_options.c *_options.h

cubereflex_options.h: ../metadata/cubereflex.xml
		$(BCOP_BIN) --header $@ $<

cubereflex_options.c: ../metadata/cubereflex.xml cubereflex_options.h
		$(BCOP_BIN) --source $@ $<
