## Process this file with automake to produce Makefile.in
PFLAGS=-module -avoid-version -no-undefined

if JPEG_PLUGIN
libimgjpeg_la_LDFLAGS = $(PFLAGS)
libimgjpeg_la_LIBADD = @ECOMP_LIBS@ -ljpeg
libimgjpeg_la_SOURCES = imgjpeg_options.c imgjpeg_options.h imgjpeg.c
endif

INCLUDES =                              \
	-I$(top_srcdir)/include         \
	@ECOMP_CFLAGS@                  \
	-DDATADIR='"$(compdatadir)"'        \
	-DLIBDIR='"$(libdir)"'              \
	-DLOCALEDIR="\"@datadir@/locale\""  \
	-DIMAGEDIR='"$(imagedir)"'          

moduledir = $(plugindir)

if JPEG_PLUGIN
module_LTLIBRARIES = libimgjpeg.la
endif

CLEANFILES = *_options.c *_options.h

imgjpeg_options.h: ../metadata/imgjpeg.xml
		$(BCOP_BIN) --header $@ $<

imgjpeg_options.c: ../metadata/imgjpeg.xml imgjpeg_options.h
		$(BCOP_BIN) --source $@ $<
