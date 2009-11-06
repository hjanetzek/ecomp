AC_PREREQ(2.57)

dnl ecomp package version number
dnl An odd micro number indicates in-progress development.
dnl An even micro number indicates a released version.
m4_define(ecomp_version_major, 0)
m4_define(ecomp_version_minor, 6)
m4_define(ecomp_version_micro, 2)

AC_INIT([ecomp],
	[ecomp_version_major().ecomp_version_minor().ecomp_version_micro()],
	[hannes.janetzek@gmail.com])

ECOMP_VERSION_MAJOR=ecomp_version_major()
ECOMP_VERSION_MINOR=ecomp_version_minor()
ECOMP_VERSION_MICRO=ecomp_version_micro()
AC_SUBST(ECOMP_VERSION_MAJOR)
AC_SUBST(ECOMP_VERSION_MINOR)
AC_SUBST(ECOMP_VERSION_MICRO)

AM_INIT_AUTOMAKE([1.7])
AC_CONFIG_HEADER([config.h])
AC_PROG_INTLTOOL([0.23])
AM_MAINTAINER_MODE

dnl decorator interface version
AC_DEFINE(DECOR_INTERFACE_VERSION, 20070319, [Decorator interface version])

AC_ISC_POSIX
AC_PROG_CC
AC_PROG_CPP
AC_PROG_CXX
AC_PROG_LIBTOOL
AC_HEADER_STDC
AC_CHECK_HEADERS([stdlib.h sys/time.h unistd.h])
# AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_MACRO_DIR([m4])

ALL_LINGUAS="cs de es fi fr hu it ja pl pt_BR sv zh_CN zh_TW af ar bg bn bs ca cy da el en_GB en_US et gl gu he hi hr id ka km ko lo lt mk mr nb nl pa pt ro ru sk sl sr ta tr uk vi xh zu"
AC_SUBST(ALL_LINGUAS)
AM_GLIB_GNU_GETTEXT
GETTEXT_PACKAGE=ecomp
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE,"$GETTEXT_PACKAGE", [Gettext package.])
AC_SUBST(GETTEXT_PACKAGE)

if test "x$GCC" = "xyes"; then
  case " $CFLAGS " in
  *[[\ \	]]-Wall[[\ \	]]*) ;;
  *) CFLAGS="$CFLAGS -Wall" ;;
  esac

  case " $CFLAGS " in
  *[[\ \	]]-Wpointer-arith[[\ \	]]*) ;;
  *) CFLAGS="$CFLAGS -Wpointer-arith" ;;
  esac

  case " $CFLAGS " in
  *[[\ \	]]-Wstrict-prototypes[[\ \	]]*) ;;
  *) CFLAGS="$CFLAGS -Wstrict-prototypes" ;;
  esac

  case " $CFLAGS " in
  *[[\ \	]]-Wmissing-prototypes[[\ \	]]*) ;;
  *) CFLAGS="$CFLAGS -Wmissing-prototypes" ;;
  esac

  case " $CFLAGS " in
  *[[\ \	]]-Wmissing-declarations[[\ \	]]*) ;;
  *) CFLAGS="$CFLAGS -Wmissing-declarations" ;;
  esac

  case " $CFLAGS " in
  *[[\ \	]]-Wnested-externs[[\ \	]]*) ;;
  *) CFLAGS="$CFLAGS -Wnested-externs" ;;
  esac

  case " $CFLAGS " in
  *[[\ \	]]-D_FORTIFY_SOURCE=2[[\ \	]]*) ;;
  *) CFLAGS="$CFLAGS -D_FORTIFY_SOURCE=2" ;;
  esac

  if test "x$enable_ansi" = "xyes"; then
    case " $CFLAGS " in
    *[[\ \	]]-ansi[[\ \	]]*) ;;
    *) CFLAGS="$CFLAGS -ansi" ;;
    esac

    case " $CFLAGS " in
    *[[\ \	]]-pedantic[[\ \	]]*) ;;
    *) CFLAGS="$CFLAGS -pedantic" ;;
    esac
  fi
fi

if test "x$GCC" = "xyes"; then
  case " $CXXFLAGS " in
  *[[\ \	]]-Wall[[\ \	]]*) ;;
  *) CXXFLAGS="$CXXFLAGS -Wall" ;;
  esac

  case " $CXXFLAGS " in
  *[[\ \	]]-D_FORTIFY_SOURCE=2[[\ \	]]*) ;;
  *) CXXFLAGS="$CXXFLAGS -D_FORTIFY_SOURCE=2" ;;
  esac
fi

AC_C_BIGENDIAN

plugindir=$libdir/ecomp
AC_SUBST(plugindir)

AC_ARG_WITH(default-plugins, [  --with-default-plugins=PLUGINS ],
		       [default_plugins="$withval"],
		       [default_plugins=""])
AC_SUBST(default_plugins)

imagedir=$datadir/ecomp
AC_SUBST(imagedir)

metadatadir=$datadir/ecomp
AC_SUBST(metadatadir)

ECOMP_REQUIRES="xcomposite \
		 xfixes	    \
		 xdamage    \
		 xrandr	    \
		 xinerama   \
		 ice	    \
		 libxml-2.0 \
		 libxslt \
		 pango \
		 eina-0 \
		 eet"


PKG_CHECK_MODULES(ECOMP, $ECOMP_REQUIRES)
AC_SUBST(ECOMP_REQUIRES)

DECORATION_REQUIRES="xrender"

PKG_CHECK_MODULES(DECORATION, $DECORATION_REQUIRES)
AC_SUBST(DECORATION_REQUIRES)

PKG_CHECK_EXISTS(xrender >= 0.9.3,
		 [have_xrender_0_9_3=yes], [have_xrender_0_9_3=no])

if test "$have_xrender_0_9_3" = yes; then
  AC_DEFINE(HAVE_XRENDER_0_9_3, 1,
	    [Define to 1 if xrender version >= 0.9.3])
fi

AC_MSG_CHECKING(for GL_CFLAGS)
AC_ARG_WITH(gl-cflags, [  --with-gl-cflags=CFLAGS ],
		       [GL_CFLAGS="$withval"],
		       [GL_CFLAGS=""])

AC_MSG_RESULT($GL_CFLAGS)
AC_MSG_CHECKING(for GL_LIBS)
AC_ARG_WITH(gl-libs, [  --with-gl-libs=LIBS ],
		     [GL_LIBS="$withval"],
		     [GL_LIBS="-lGL"])
AC_MSG_RESULT($GL_LIBS)

AC_SUBST(GL_CFLAGS)
AC_SUBST(GL_LIBS)

AC_CHECK_PROGS(XSLTPROC, xsltproc)
if test -z "$XSLTPROC"; then
  AC_MSG_ERROR([Couldn't find xsltproc])
fi


stylesheetdir=$datadir/ecomp
AC_SUBST(stylesheetdir)

AC_ARG_ENABLE(inotify,
  [  --disable-inotify       Disable inotify plugin],
  [use_inotify=$enableval], [use_inotify=yes])

if test "x$use_inotify" = "xyes"; then
  AC_CHECK_HEADERS([sys/inotify.h], [use_inotify=yes], [use_inotify=no])
fi

AM_CONDITIONAL(INOTIFY_PLUGIN, test "x$use_inotify" = "xyes")
if test "$use_inotify" = yes; then
  AC_DEFINE(USE_INOTIFY, 1, [Build inotify plugin])
fi

PKG_CHECK_MODULES(CAIRO, cairo-xlib-xrender cairo >= 1.0, [have_cairo=yes], [have_cairo=no])
PKG_CHECK_MODULES(PANGO, cairo-xlib-xrender pangocairo cairo >= 1.0, [have_pango=yes], [have_pango=no])

AM_CONDITIONAL(TEXT_PLUGIN, test "x$have_pango" = "xyes")
if test "x$have_pango" = "xyes"; then
  AC_DEFINE(USE_TEXT, 1, [Build text plugin])
fi

AM_CONDITIONAL(WALL_PLUGIN, test "x$have_cairo" = "xyes")
if test "x$have_cairo" = "xyes"; then
  AC_DEFINE(USE_WALL, 1, [Build wall plugin])
fi

AC_ARG_ENABLE(jpeg,
  [  --disable-jpeg      Disable jpeg plugin],
  [use_jpeg=$enableval], [use_jpeg=yes])

AM_CONDITIONAL(JPEG_PLUGIN, test "x$use_jpeg" = "xyes")
if test "$use_jpeg" = yes; then
  AC_DEFINE(USE_JPEG, 1, [Build jpeg plugin])
fi

AC_ARG_WITH(max-desktops,
  [  --with-max-desktops     Max reasonable desktops],
  [MAX_DESKTOPS=$withval],
  [MAX_DESKTOPS=36])

if test -n "$MAX_DESKTOPS"; then
  AC_DEFINE_UNQUOTED(MAX_DESKTOPS, $MAX_DESKTOPS, [Max reasonable desktops])
fi


PKG_CHECK_MODULES(LIBXSLT, libxslt)
xsltdir=$datadir/bcop
AC_SUBST(xsltdir)

BCOP_BIN=../bcop/src/bcop.in
AC_SUBST(BCOP_BIN)

intltoolize --force || die "intltoolize failed"


AC_OUTPUT([
ecomp.pc
ecomp-cube.pc
ecomp-scale.pc
Makefile
src/Makefile
include/Makefile
plugins/Makefile
text/Makefile
expo/Makefile
jpeg/Makefile
ring/Makefile
shift/Makefile
thumbnail/Makefile
wall/Makefile
animation/Makefile
mblur/Makefile
cubereflex/Makefile
scaleaddon/Makefile
images/Makefile
metadata/Makefile
bcop/bcop.pc
bcop/Makefile
bcop/src/Makefile
bcop/src/bcop
po/Makefile.in
data/Makefile
])

echo ""
echo "the following plugins will are required:"
echo "  inotify:     $use_inotify"
echo ""
echo ""
