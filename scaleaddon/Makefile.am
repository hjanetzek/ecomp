## Process this file with automake to produce Makefile.in
PFLAGS=-module -avoid-version -no-undefined


libscaleaddon_la_LDFLAGS = $(PFLAGS)
libscaleaddon_la_LIBADD = @ECOMP_LIBS@
libscaleaddon_la_SOURCES = scaleaddon_options.c scaleaddon_options.h scaleaddon.c


INCLUDES =                              \
	-I$(top_srcdir)/include         \
	@ECOMP_CFLAGS@                  \
	-DDATADIR='"$(compdatadir)"'        \
	-DLIBDIR='"$(libdir)"'              \
	-DLOCALEDIR="\"@datadir@/locale\""  \
	-DIMAGEDIR='"$(imagedir)"'          

moduledir = $(plugindir)

module_LTLIBRARIES = libscaleaddon.la

CLEANFILES = *_options.c *_options.h

scaleaddon_options.h: ../metadata/scaleaddon.xml
		$(BCOP_BIN) --header $@ $<

scaleaddon_options.c: ../metadata/scaleaddon.xml scaleaddon_options.h
		$(BCOP_BIN) --source $@ $<
