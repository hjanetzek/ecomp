## Process this file with automake to produce Makefile.in
PFLAGS=-module -avoid-version -no-undefined

libscalefilter_la_LDFLAGS = $(PFLAGS)
libscalefilter_la_LIBADD = @ECOMP_LIBS@
libscalefilter_la_SOURCES = scalefilter_options.c \
			    scalefilter_options.h \
			    scalefilter.c

INCLUDES =                              \
	@ECOMP_CFLAGS@                  \
	-DDATADIR='"$(compdatadir)"'        \
	-DLIBDIR='"$(libdir)"'              \
	-DLOCALEDIR="\"@datadir@/locale\""  \
	-DIMAGEDIR='"$(imagedir)"'          \
	-I$(top_srcdir)/include

moduledir = $(plugindir)


module_LTLIBRARIES = libscalefilter.la

CLEANFILES = *_options.c *_options.h

scalefilter_options.h: ../metadata/scalefilter.xml
		$(BCOP_BIN) --header $@ $<

scalefilter_options.c: ../metadata/scalefilter.xml scalefilter_options.h
		$(BCOP_BIN) --source $@ $<
