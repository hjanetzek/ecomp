
libcube_la_LDFLAGS = --as-needed -module -avoid-version -no-undefined
libcube_la_SOURCES = cube.c

librotate_la_LDFLAGS = --as-needed -module -avoid-version -no-undefined
librotate_la_SOURCES = rotate.c

libzoom_la_LDFLAGS = --as-needed -module -avoid-version -no-undefined
libzoom_la_SOURCES = zoom.c

libscale_la_LDFLAGS = --as-needed -module -avoid-version -no-undefined
libscale_la_SOURCES = scale.c

libwobbly_la_LDFLAGS = -module -avoid-version -no-undefined
libwobbly_la_SOURCES = wobbly.c

libmove_la_LDFLAGS = --as-needed -module -avoid-version -no-undefined
libmove_la_SOURCES = move.c

libdecoration_la_LDFLAGS = --as-needed -module -avoid-version -no-undefined
libdecoration_la_SOURCES = decoration.c libdecor.c

libswitcher_la_LDFLAGS = --as-needed -module -avoid-version -no-undefined
libswitcher_la_SOURCES = switcher.c

libwater_la_LDFLAGS = --as-needed -module -avoid-version -no-undefined
libwater_la_SOURCES = water.c

libplane_la_LDFLAGS = --as-needed -module -avoid-version -no-undefined
libplane_la_SOURCES = plane.c

libblur_la_LDFLAGS = --as-needed -module -avoid-version -no-undefined
libblur_la_LIBADD = -lGLU
libblur_la_SOURCES = blur.c libdecor.c

libregex_la_LDFLAGS = --as-needed -module -avoid-version -no-undefined
libregex_la_SOURCES = regex.c

libini_la_LDFLAGS = --as-needed -module -avoid-version -no-undefined
libini_la_SOURCES = ini.c

if INOTIFY_PLUGIN
libinotify_la_LDFLAGS = --as-needed -module -avoid-version -no-undefined
libinotify_la_SOURCES = inotify.c
libinotify_module = libinotify.la
endif

INCLUDES =					\
	@ECOMP_CFLAGS@				\
	-DALL_LINGUAS="\"@ALL_LINGUAS@\""	\
	-DLOCALEDIR="\"@datadir@/locale\""	\
	-DPLUGINDIR=\"$(plugindir)\"		\
	-DIMAGEDIR=\"$(imagedir)\"		\
	-I$(top_srcdir)/include                 \
	-DMETADATADIR=\"$(metadatadir)\"        \
	-DDATADIR=\"$(datadir)\"

moduledir = $(plugindir)

module_LTLIBRARIES =		\
	libdecoration.la	\
	libscale.la             \
	libblur.la		\
	libregex.la		\
	libplane.la		\
	libcube.la		\
	librotate.la 		\
	libswitcher.la     	\
	libwater.la		\
	libmove.la		\
	libwobbly.la		\
	libini.la		\
	$(libinotify_module)	\
	libzoom.la

