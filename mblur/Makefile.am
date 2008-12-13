## Process this file with automake to produce Makefile.in
PFLAGS=-module -avoid-version -no-undefined

libmblur_la_LDFLAGS = $(PFLAGS)
libmblur_la_LIBADD = @ECOMP_LIBS@
libmblur_la_SOURCES = mblur_options.c mblur_options.h mblur.c

INCLUDES =                              \
	@ECOMP_CFLAGS@                  \
	-DDATADIR='"$(compdatadir)"'        \
	-DLIBDIR='"$(libdir)"'              \
	-DLOCALEDIR="\"@datadir@/locale\""  \
	-DIMAGEDIR='"$(imagedir)"'          \
	-I$(top_srcdir)/include

moduledir = $(plugindir)


module_LTLIBRARIES = libmblur.la

CLEANFILES = *_options.c *_options.h

mblur_options.h: ../metadata/mblur.xml
		$(BCOP_BIN) --header $@ $<

mblur_options.c: ../metadata/mblur.xml mblur_options.h
		$(BCOP_BIN) --source $@ $<
