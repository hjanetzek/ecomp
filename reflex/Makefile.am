## Process this file with automake to produce Makefile.in
PFLAGS=-module -avoid-version -no-undefined

libreflex_la_LDFLAGS = $(PFLAGS)
libreflex_la_LIBADD = @ECOMP_LIBS@
libreflex_la_SOURCES = reflex_options.c reflex_options.h reflex.c

INCLUDES =                              \
	@ECOMP_CFLAGS@                  \
	-DDATADIR='"$(compdatadir)"'        \
	-DLIBDIR='"$(libdir)"'              \
	-DLOCALEDIR="\"@datadir@/locale\""  \
	-DIMAGEDIR='"$(imagedir)"'          \
	-I$(top_srcdir)/include

moduledir = $(plugindir)


module_LTLIBRARIES = libreflex.la

CLEANFILES = *_options.c *_options.h

reflex_options.h: ../metadata/reflex.xml
		$(BCOP_BIN) --header $@ $<

reflex_options.c: ../metadata/reflex.xml reflex_options.h
		$(BCOP_BIN) --source $@ $<
