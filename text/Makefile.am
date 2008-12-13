## Process this file with automake to produce Makefile.in
PFLAGS=-module -avoid-version -no-undefined

if TEXT_PLUGIN
libtext_la_LDFLAGS = $(PFLAGS)
libtext_la_LIBADD = @ECOMP_LIBS@ @PANGO_LIBS@
libtext_la_SOURCES = text.c
endif

INCLUDES =                              \
	-I$(top_srcdir)/include         \
	@ECOMP_CFLAGS@                  \
	@PANGO_CFLAGS@			\
	-DDATADIR='"$(compdatadir)"'        \
	-DLIBDIR='"$(libdir)"'              \
	-DLOCALEDIR="\"@datadir@/locale\""  \
	-DIMAGEDIR='"$(imagedir)"'          

moduledir = $(plugindir)

if TEXT_PLUGIN
module_LTLIBRARIES = libtext.la
endif

CLEANFILES = *_options.c *_options.h

text_options.h: ../metadata/text.xml
		$(BCOP_BIN) --header $@ $<

text_options.c: ../metadata/text.xml text_options.h
		$(BCOP_BIN) --source $@ $<
