## Process this file with automake to produce Makefile.in
PFLAGS=-module -avoid-version -no-undefined

libring_la_LDFLAGS = $(PFLAGS)
libring_la_LIBADD = @ECOMP_LIBS@
libring_la_SOURCES = ring_options.c ring_options.h ring.c

INCLUDES =                              \
	-I$(top_srcdir)/include         \
	@ECOMP_CFLAGS@                  \
	-DDATADIR='"$(compdatadir)"'        \
	-DLIBDIR='"$(libdir)"'              \
	-DLOCALEDIR="\"@datadir@/locale\""  \
	-DIMAGEDIR='"$(imagedir)"'          

moduledir = $(plugindir)


module_LTLIBRARIES = libring.la

CLEANFILES = *_options.c *_options.h

ring_options.h: ../metadata/ring.xml
		$(BCOP_BIN) --header $@ $<

ring_options.c: ../metadata/ring.xml ring_options.h
		$(BCOP_BIN) --source $@ $<
