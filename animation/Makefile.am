PFLAGS=-module -avoid-version -no-undefined -lGLU 

libanimation_la_LDFLAGS = $(PFLAGS)
libanimation_la_LIBADD = @ECOMP_LIBS@ @CAIRO_LIBS@
libanimation_la_SOURCES = animation.c \
			  animation-internal.h \
			  animation_tex.h \
			  airplane3d.c \
			  beamup.c \
			  burn.c \
			  curvedfold.c \
			  dodge.c \
			  domino.c \
			  dream.c \
			  explode3d.c \
			  fade.c \
			  focusfade.c \
			  fold3d.c \
			  glide.c \
			  horizontalfold.c \
			  leafspread.c \
			  magiclamp.c \
			  options.c \
			  particle.c \
			  polygon.c \
			  rollup.c \
			  skewer.c \
			  wave.c \
			  zoomside.c

INCLUDES =                              \
	-I/usr/include                      \
	-I$(top_srcdir)/include             \
	@ECOMP_CFLAGS@                     \
	@CAIRO_CFLAGS@	                    \
	-DDATADIR='"$(compdatadir)"'        \
	-DLIBDIR='"$(libdir)"'              \
	-DLOCALEDIR="\"@datadir@/locale\""  \
	-DIMAGEDIR='"$(imagedir)"'          

# @LIBRSVG_CFLAGS@                   

moduledir = $(plugindir)

module_LTLIBRARIES=libanimation.la
