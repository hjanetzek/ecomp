xmldir       = $(metadatadir)
xml_in_files =		  \
	core.xml.in	  \
	annotate.xml.in   \
	blur.xml.in	  \
	clone.xml.in	  \
	cube.xml.in	  \
	dbus.xml.in	  \
	decoration.xml.in \
	fade.xml.in	  \
	minimize.xml.in	  \
	move.xml.in	  \
	plane.xml.in	  \
	png.xml.in	  \
	regex.xml.in	  \
	rotate.xml.in     \
	scale.xml.in      \
	screenshot.xml.in \
	svg.xml.in        \
	switcher.xml.in   \
	video.xml.in      \
	water.xml.in      \
	wobbly.xml.in     \
	zoom.xml.in	  \
	animation.xml.in  \
	imgjpeg.xml.in    \
	ring.xml.in       \
	shift.xml.in      \
	thumbnail.xml.in  \
	wall.xml.in       \
	text.xml.in       \
	expo.xml.in       \
	mblur.xml.in      \
	cubereflex.xml.in \
	staticswitcher.xml.in \
	ini.xml.in \
	inotify.xml.in \
	scaleaddon.xml.in 

xml_files    = $(xml_in_files:.xml.in=.xml)
xml_DATA     = $(xml_files)

@INTLTOOL_XML_RULE@

EXTRA_DIST =		\
	$(xml_files)    \
	$(xml_in_files) \
	$(xslt_files)	\
	$(schema_files)

DISTCLEANFILES =	\
	$(xml_files)    \
	$(schema_files)
