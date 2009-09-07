
libcube_la_LDFLAGS = -module -avoid-version -no-undefined
libcube_la_SOURCES = cube.c

librotate_la_LDFLAGS = -module -avoid-version -no-undefined
librotate_la_SOURCES = rotate.c

libzoom_la_LDFLAGS = -module -avoid-version -no-undefined
libzoom_la_SOURCES = zoom.c

libscale_la_LDFLAGS = -module -avoid-version -no-undefined
libscale_la_SOURCES = scale.c

libwobbly_la_LDFLAGS = -module -avoid-version -no-undefined
libwobbly_la_SOURCES = wobbly.c

libmove_la_LDFLAGS = -module -avoid-version -no-undefined
libmove_la_SOURCES = move.c

libdecoration_la_LDFLAGS = -module -avoid-version -no-undefined
libdecoration_la_SOURCES = decoration.c libdecor.c

libswitcher_la_LDFLAGS = -module -avoid-version -no-undefined
libswitcher_la_SOURCES = switcher.c

libwater_la_LDFLAGS = -module -avoid-version -no-undefined
libwater_la_SOURCES = water.c

libplane_la_LDFLAGS = -module -avoid-version -no-undefined
libplane_la_SOURCES = plane.c

# libpng_la_LDFLAGS = -module -avoid-version -no-undefined
# libpng_la_LIBADD = @LIBPNG_LIBS@
# libpng_la_SOURCES = png.c

libblur_la_LDFLAGS = -module -avoid-version -no-undefined
libblur_la_LIBADD = -lGLU
libblur_la_SOURCES = blur.c libdecor.c

libregex_la_LDFLAGS = -module -avoid-version -no-undefined
libregex_la_SOURCES = regex.c

libini_la_LDFLAGS = -module -avoid-version -no-undefined
libini_la_SOURCES = ini.c

# if USE_LIBRSVG
# libsvg_la_DEPENDENCIES = $(top_builddir)/libdecoration/libdecoration.la
# libsvg_la_LDFLAGS = -module -avoid-version -no-undefined
# libsvg_la_LIBADD =				       \
# 	$(top_builddir)/libdecoration/libdecoration.la \
# 	@LIBRSVG_LIBS@
# libsvg_la_SOURCES = svg.c
# libsvg_module = libsvg.la
# endif

if DBUS_PLUGIN
libdbus_la_LDFLAGS = -module -avoid-version -no-undefined
libdbus_la_LIBADD = @DBUS_LIBS@
libdbus_la_SOURCES = dbus.c
libdbus_module = libdbus.la
endif

if INOTIFY_PLUGIN
libinotify_la_LDFLAGS = -module -avoid-version -no-undefined
libinotify_la_SOURCES = inotify.c
libinotify_module = libinotify.la
endif

# @LIBPNG_CFLAGS@
# @LIBRSVG_CFLAGS@
# libpng.la		
# @ANNOTATE_CFLAGS@			
# @FUSE_CFLAGS@			

INCLUDES =					\
	@ECOMP_CFLAGS@				\
	@DBUS_CFLAGS@				\
	-DFUSE_USE_VERSION=26			\
	-DALL_LINGUAS="\"@ALL_LINGUAS@\""	\
	-DLOCALEDIR="\"@datadir@/locale\""	\
	-DPLUGINDIR=\"$(plugindir)\"		\
	-DIMAGEDIR=\"$(imagedir)\"		\
	-I$(top_srcdir)/include                 \
	-DMETADATADIR=\"$(metadatadir)\"        \
	-DDATADIR=\"$(datadir)\"

moduledir = $(plugindir)

module_LTLIBRARIES =		\
	$(libdbus_module)	\
	libdecoration.la	\
	libscale.la             \
	libblur.la		\
	libregex.la		\
	libplane.la		\
	libcube.la		\
	librotate.la 		\
	libswitcher.la     	\
	$(libsvg_module)	\
	libwater.la		\
	libmove.la		\
	libwobbly.la		\
	libini.la		\
	$(libinotify_module)	\
	libzoom.la

