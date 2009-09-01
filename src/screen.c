/*
 * Copyright © 2005 Novell, Inc.
 *
 * Permission to use, copy, modify, distribute, and sell this software
 * and its documentation for any purpose is hereby granted without
 * fee, provided that the above copyright notice appear in all copies
 * and that both that copyright notice and this permission notice
 * appear in supporting documentation, and that the name of
 * Novell, Inc. not be used in advertising or publicity pertaining to
 * distribution of the software without specific, written prior permission.
 * Novell, Inc. makes no representations about the suitability of this
 * software for any purpose. It is provided "as is" without express or
 * implied warranty.
 *
 * NOVELL, INC. DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN
 * NO EVENT SHALL NOVELL, INC. BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
 * NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION
 * WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * Author: David Reveman <davidr@novell.com>
 */

#ifdef HAVE_CONFIG_H
#  include "../config.h"
#endif

//#define D(x) do { printf(__FILE__ ":%d:\t", __LINE__); printf x; fflush(stdout); } while(0)
//#define C(x) do { printf(__FILE__ ":%d:\t", __LINE__); printf x; fflush(stdout); } while(0)
#define D(x)
#define C(x)


#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <math.h>
#include <dlfcn.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xproto.h>
#include <X11/extensions/Xrandr.h>
#include <X11/extensions/shape.h>
#include <X11/cursorfont.h>

#include <ecomp.h>

#define NUM_OPTIONS(s) (sizeof ((s)->opt) / sizeof (CompOption))

void
ecompActionTerminateNotify (CompScreen *s, int plugin)
{
	XEvent ev;

	ev.type			= ClientMessage;
	ev.xclient.window		= s->root;
	ev.xclient.display = s->display->display;
	ev.xclient.message_type = s->display->eManagedAtom;
	ev.xclient.format		= 32;
	ev.xclient.data.l[0]	= 2;
	ev.xclient.data.l[1]	= plugin;
	ev.xclient.data.l[2]	= 0;
	ev.xclient.data.l[3]	= 0;
	ev.xclient.data.l[4]	= 0;

	XSendEvent (s->display->display, s->root, FALSE,
				SubstructureRedirectMask | SubstructureNotifyMask,
				&ev);
}


static int
reallocScreenPrivate (int  size,
					  void *closure)
{
	CompDisplay *d = (CompDisplay *) closure;
	CompScreen	*s;
	void		*privates;

	for (s = d->screens; s; s = s->next)
	{
		privates = realloc (s->privates, size * sizeof (CompPrivate));
		if (!privates)
			return FALSE;

		s->privates = (CompPrivate *) privates;
	}

	return TRUE;
}


int
allocateScreenPrivateIndex (CompDisplay *display)
{
	return allocatePrivateIndex
		(&display->screenPrivateLen, &display->screenPrivateIndices,
		 reallocScreenPrivate, (void *) display);
}


void
freeScreenPrivateIndex (CompDisplay *display,
						int		index)
{
	freePrivateIndex (display->screenPrivateLen,
					  display->screenPrivateIndices,
					  index);
}


static void
updateOutputDevices (CompScreen	*s)
{
	CompOutput	  *o, *output = NULL;
	CompListValue *list = &s->opt[COMP_SCREEN_OPTION_OUTPUTS].value.list;
	int		  nOutput = 0;
	int		  x, y, i, bits;
	unsigned int  width, height;
	int		  x1, y1, x2, y2;
	Region	  region;

	for (i = 0; i < list->nValue; i++)
	{
		if (!list->value[i].s)
			continue;

		x	   = 0;
		y	   = 0;
		width  = s->width;
		height = s->height;

		bits = XParseGeometry (list->value[i].s, &x, &y, &width, &height);

		if (bits & XNegative)
			x = s->width + x - width;

		if (bits & YNegative)
			y = s->height + y - height;

		x1 = x;
		y1 = y;
		x2 = x + width;
		y2 = y + height;

		if (x1 < 0)
			x1 = 0;
		if (y1 < 0)
			y1 = 0;
		if (x2 > s->width)
			x2 = s->width;
		if (y2 > s->height)
			y2 = s->height;

		if (x1 < x2 && y1 < y2)
		{
			o = realloc (output, sizeof (CompOutput) * (nOutput + 1));
			if (o)
			{
				o[nOutput].region.extents.x1 = x1;
				o[nOutput].region.extents.y1 = y1;
				o[nOutput].region.extents.x2 = x2;
				o[nOutput].region.extents.y2 = y2;

				output = o;
				nOutput++;
			}
		}
	}

	/* make sure we have at least one output */
	if (!nOutput)
	{
		output = malloc (sizeof (CompOutput));
		if (!output)
			return;

		output->region.extents.x1 = 0;
		output->region.extents.y1 = 0;
		output->region.extents.x2 = s->width;
		output->region.extents.y2 = s->height;

		nOutput = 1;
	}

	/* set name, width, height and update rect pointers in all regions */
	for (i = 0; i < nOutput; i++)
	{
		output[i].name = malloc (sizeof (char) * 10);
		if (output[i].name)
			snprintf (output[i].name, 10, "Output %d", nOutput);

		output[i].region.rects = &output[i].region.extents;
		output[i].region.numRects = 1;

		output[i].width	 = output[i].region.extents.x2 -
			output[i].region.extents.x1;
		output[i].height = output[i].region.extents.y2 -
			output[i].region.extents.y1;

		output[i].workArea.x	  = output[i].region.extents.x1;
		output[i].workArea.y	  = output[i].region.extents.x1;
		output[i].workArea.width  = output[i].width;
		output[i].workArea.height = output[i].height;

		output[i].id = i;
	}

	if (s->outputDev)
	{
		for (i = 0; i < s->nOutputDev; i++)
			if (s->outputDev[i].name)
				free (s->outputDev[i].name);

		free (s->outputDev);
	}

	s->outputDev  = output;
	s->nOutputDev = nOutput;

	setCurrentOutput (s, s->currentOutputDev);

	//updateWorkareaForScreen (s);

	setDefaultViewport (s);
	damageScreen (s);

	region = XCreateRegion ();
	if (region)
	{
		REGION r;

		r.rects = &r.extents;
		r.numRects = 1;

		if (s->display->nScreenInfo)
		{
			for (i = 0; i < s->display->nScreenInfo; i++)
			{
				r.extents.x1 = s->display->screenInfo[i].x_org;
				r.extents.y1 = s->display->screenInfo[i].y_org;
				r.extents.x2 = r.extents.x1 + s->display->screenInfo[i].width;
				r.extents.y2 = r.extents.y1 + s->display->screenInfo[i].height;

				XUnionRegion (region, &r, region);
			}
		}
		else
		{
			r.extents.x1 = 0;
			r.extents.y1 = 0;
			r.extents.x2 = s->width;
			r.extents.y2 = s->height;

			XUnionRegion (region, &r, region);
		}

		/* remove all output regions from visible screen region */
		for (i = 0; i < s->nOutputDev; i++)
			XSubtractRegion (region, &s->outputDev[i].region, region);

		/* we should clear color buffers before swapping if we have visible
		   regions without output */
		s->clearBuffers = REGION_NOT_EMPTY (region);

		XDestroyRegion (region);
	}

	(*s->outputChangeNotify) (s);
}

static void
detectOutputDevices (CompScreen *s)
{
	if (!noDetection && s->opt[COMP_SCREEN_OPTION_DETECT_OUTPUTS].value.b)
	{
		char		*name;
		CompOptionValue	value;
		char		output[1024];
		int		i, size = sizeof (output);

		if (s->display->nScreenInfo)
		{
			int n = s->display->nScreenInfo;

			value.list.nValue = n;
			value.list.value  = malloc (sizeof (CompOptionValue) * n);
			if (!value.list.value)
				return;

			for (i = 0; i < n; i++)
			{
				snprintf (output, size, "%dx%d+%d+%d",
						  s->display->screenInfo[i].width,
						  s->display->screenInfo[i].height,
						  s->display->screenInfo[i].x_org,
						  s->display->screenInfo[i].y_org);

				value.list.value[i].s = strdup (output);
			}
		}
		else
		{
			value.list.nValue = 1;
			value.list.value  = malloc (sizeof (CompOptionValue));
			if (!value.list.value)
				return;

			snprintf (output, size, "%dx%d+%d+%d", s->width, s->height, 0, 0);

			value.list.value->s = strdup (output);
		}

		name = s->opt[COMP_SCREEN_OPTION_OUTPUTS].name;

		s->opt[COMP_SCREEN_OPTION_DETECT_OUTPUTS].value.b = FALSE;
		(*s->setScreenOption) (s, name, &value);
		s->opt[COMP_SCREEN_OPTION_DETECT_OUTPUTS].value.b = TRUE;

		for (i = 0; i < value.list.nValue; i++)
			if (value.list.value[i].s)
				free (value.list.value[i].s);

		free (value.list.value);
	}
	else
	{
		updateOutputDevices (s);
	}
}

CompOption *
compGetScreenOptions (CompScreen *screen,
					  int	 *count)
{
	*count = NUM_OPTIONS (screen);
	return screen->opt;
}

static Bool
setScreenOption (CompScreen		 *screen,
				 char			 *name,
				 CompOptionValue *value)
{
	CompOption *o;
	int		   index;

	o = compFindOption (screen->opt, NUM_OPTIONS (screen), name, &index);
	if (!o)
		return FALSE;

	switch (index) {
	case COMP_SCREEN_OPTION_DETECT_REFRESH_RATE:
		if (compSetBoolOption (o, value))
		{
			if (value->b)
				detectRefreshRateOfScreen (screen);

			return TRUE;
		}
		break;
	case COMP_SCREEN_OPTION_DETECT_OUTPUTS:
		if (compSetBoolOption (o, value))
		{
			if (value->b)
				detectOutputDevices (screen);

			return TRUE;
		}
		break;
	case COMP_SCREEN_OPTION_REFRESH_RATE:
		if (screen->opt[COMP_SCREEN_OPTION_DETECT_REFRESH_RATE].value.b)
			return FALSE;

		if (compSetIntOption (o, value))
		{
			screen->redrawTime = 1000 / o->value.i;
			screen->optimalRedrawTime = screen->redrawTime;
			return TRUE;
		}
		break;
		/* case COMP_SCREEN_OPTION_HSIZE:
		 *	 if (compSetIntOption (o, value))
		 *	 {
		 *	 CompOption *vsize;
		 *
		 *	 vsize = compFindOption (screen->opt, NUM_OPTIONS (screen),
		 *	 "vsize", NULL);
		 *
		 *	 if (!vsize)
		 *	 return FALSE;
		 *
		 *	 if (o->value.i * screen->width > MAXSHORT)
		 *	 return FALSE;
		 *
		 *	 setVirtualScreenSize (screen, o->value.i, vsize->value.i);
		 *	 return TRUE;
		 *	 }
		 *	 break;
		 *	 case COMP_SCREEN_OPTION_VSIZE:
		 *	 if (compSetIntOption (o, value))
		 *	 {
		 *	 CompOption *hsize;
		 *
		 *	 hsize = compFindOption (screen->opt, NUM_OPTIONS (screen),
		 *	 "hsize", NULL);
		 *
		 *	 if (!hsize)
		 *	 return FALSE;
		 *
		 *	 if (o->value.i * screen->height > MAXSHORT)
		 *	 return FALSE;
		 *
		 *	 setVirtualScreenSize (screen, hsize->value.i, o->value.i);
		 *	 return TRUE;
		 *	 }
		 *	 break; */
	case COMP_SCREEN_OPTION_DEFAULT_ICON:
		if (compSetStringOption (o, value))
			return updateDefaultIcon (screen);
		break;
	case COMP_SCREEN_OPTION_OUTPUTS:
		if (!noDetection &&
			screen->opt[COMP_SCREEN_OPTION_DETECT_OUTPUTS].value.b)
			return FALSE;

		if (compSetOptionList (o, value))
		{
			updateOutputDevices (screen);
			return TRUE;
		}
		break;
	case COMP_SCREEN_OPTION_OPACITY_MATCHES:
		if (compSetOptionList (o, value))
		{
			CompWindow *w;
			int		   i;

			for (i = 0; i < o->value.list.nValue; i++)
				matchUpdate (screen->display, &o->value.list.value[i].match);

			for (w = screen->windows; w; w = w->next)
				updateWindowOpacity (w);

			return TRUE;
		}
		break;
	case COMP_SCREEN_OPTION_OPACITY_VALUES:
		if (compSetOptionList (o, value))
		{
			CompWindow *w;

			for (w = screen->windows; w; w = w->next)
				updateWindowOpacity (w);

			return TRUE;
		}
		break;
	default:
		if (compSetScreenOption (screen, o, value))
			return TRUE;
		break;
	}

	return FALSE;
}

static Bool
setScreenOptionForPlugin (CompScreen	  *screen,
						  char			  *plugin,
						  char			  *name,
						  CompOptionValue *value)
{
	CompPlugin *p;

	p = findActivePlugin (plugin);
	if (p && p->vTable->setScreenOption)
		return (*p->vTable->setScreenOption) (p, screen, name, value);

	return FALSE;
}

const CompMetadataOptionInfo coreScreenOptionInfo[COMP_SCREEN_OPTION_NUM] = {
	{ "detect_refresh_rate", "bool", 0, 0, 0 },
	{ "lighting", "bool", 0, 0, 0 },
	{ "refresh_rate", "int", "<min>1</min>", 0, 0 },
	/* { "hsize", "int", "<min>1</min><max>32</max>", 0, 0 },
	 * { "vsize", "int", "<min>1</min><max>32</max>", 0, 0 }, */
	{ "opacity_step", "int", "<min>1</min>", 0, 0 },
	{ "unredirect_fullscreen_windows", "bool", 0, 0, 0 },
	{ "default_icon", "string", 0, 0, 0 },
	{ "sync_to_vblank", "bool", 0, 0, 0 },
	{ "number_of_desktops", "int", "<min>1</min>", 0, 0 },
	{ "detect_outputs", "bool", 0, 0, 0 },
	{ "outputs", "list", "<type>string</type>", 0, 0 },
	{ "focus_prevention_match", "match", 0, 0, 0 },
	{ "opacity_matches", "list", "<type>match</type>", 0, 0 },
	{ "opacity_values", "list", "<type>int</type>", 0, 0 }
};


static void
frustum (GLfloat *m,
		 GLfloat left,
		 GLfloat right,
		 GLfloat bottom,
		 GLfloat top,
		 GLfloat nearval,
		 GLfloat farval)
{
	GLfloat x, y, a, b, c, d;

	x = (2.0 * nearval) / (right - left);
	y = (2.0 * nearval) / (top - bottom);
	a = (right + left) / (right - left);
	b = (top + bottom) / (top - bottom);
	c = -(farval + nearval) / ( farval - nearval);
	d = -(2.0 * farval * nearval) / (farval - nearval);

#define M(row,col)	m[col*4+row]
	M(0,0) = x;		M(0,1) = 0.0f;	M(0,2) = a;		 M(0,3) = 0.0f;
	M(1,0) = 0.0f;	M(1,1) = y;		M(1,2) = b;		 M(1,3) = 0.0f;
	M(2,0) = 0.0f;	M(2,1) = 0.0f;	M(2,2) = c;		 M(2,3) = d;
	M(3,0) = 0.0f;	M(3,1) = 0.0f;	M(3,2) = -1.0f;	 M(3,3) = 0.0f;
#undef M

}

static void
perspective (GLfloat *m,
			 GLfloat fovy,
			 GLfloat aspect,
			 GLfloat zNear,
			 GLfloat zFar)
{
	GLfloat xmin, xmax, ymin, ymax;

	ymax = zNear * tan (fovy * M_PI / 360.0);
	ymin = -ymax;
	xmin = ymin * aspect;
	xmax = ymax * aspect;

	frustum (m, xmin, xmax, ymin, ymax, zNear, zFar);
}

void
setCurrentOutput (CompScreen *s,
				  int		 outputNum)
{
	if (outputNum >= s->nOutputDev)
		outputNum = 0;

	s->currentOutputDev = outputNum;
}

static void
reshape (CompScreen *s,
		 int		w,
		 int		h)
{

#ifdef USE_COW
	if (useCow)
		XMoveResizeWindow (s->display->display, s->overlay, 0, 0, w, h);
#endif

	if (s->display->xineramaExtension)
	{
		CompDisplay *d = s->display;

		if (d->screenInfo)
			XFree (d->screenInfo);

		d->nScreenInfo = 0;
		d->screenInfo = XineramaQueryScreens (d->display, &d->nScreenInfo);
	}

	glMatrixMode (GL_PROJECTION);
	glLoadIdentity ();
	glMatrixMode (GL_MODELVIEW);
	glLoadIdentity ();
	glDepthRange (0, 1);
	glViewport (-1, -1, 2, 2);
	glRasterPos2f (0, 0);

	s->rasterX = s->rasterY = 0;

	perspective (s->projection, 60.0f, 1.0f, 0.1f, 100.0f);

	glMatrixMode (GL_PROJECTION);
	glLoadIdentity ();
	glMultMatrixf (s->projection);
	glMatrixMode (GL_MODELVIEW);

	s->region.rects = &s->region.extents;
	s->region.numRects = 1;
	s->region.extents.x1 = 0;
	s->region.extents.y1 = 0;
	s->region.extents.x2 = w;
	s->region.extents.y2 = h;
	s->region.size = 1;

	s->width  = w;
	s->height = h;

	s->fullscreenOutput.name			 = "fullscreen";
	s->fullscreenOutput.id				 = ~0;
	s->fullscreenOutput.width			 = w;
	s->fullscreenOutput.height			 = h;
	s->fullscreenOutput.region			 = s->region;
	s->fullscreenOutput.workArea.x		 = 0;
	s->fullscreenOutput.workArea.y		 = 0;
	s->fullscreenOutput.workArea.width	 = w;
	s->fullscreenOutput.workArea.height	 = h;

	/* updateScreenEdges (s); */
}

void
configureScreen (CompScreen	 *s,
				 XConfigureEvent *ce)
{
	if (s->attrib.width	 != ce->width ||
		s->attrib.height != ce->height)
	{
		s->attrib.width	 = ce->width;
		s->attrib.height = ce->height;

		reshape (s, ce->width, ce->height);

		detectOutputDevices (s);

		damageScreen (s);
	}
}

static FuncPtr
getProcAddress (CompScreen *s,
				const char *name)
{
	static void *dlhand = NULL;
	FuncPtr		funcPtr = NULL;

	if (s->getProcAddress)
		funcPtr = s->getProcAddress ((GLubyte *) name);

	if (!funcPtr)
	{
		if (!dlhand)
			dlhand = dlopen (NULL, RTLD_LAZY);

		if (dlhand)
		{
			dlerror ();
			funcPtr = (FuncPtr) dlsym (dlhand, name);
			if (dlerror () != NULL)
				funcPtr = NULL;
		}
	}

	return funcPtr;
}

/* void
 * updateScreenBackground (CompScreen  *screen,
 *			CompTexture *texture)
 * {
 *	   Display	  *dpy = screen->display->display;
 *	   Atom	  pixmapAtom, actualType;
 *	   int		  actualFormat, i, status;
 *	   unsigned int	 width = 1, height = 1, depth = 0;
 *	   unsigned long nItems;
 *	   unsigned long bytesAfter;
 *	   unsigned char *prop;
 *	   Pixmap	  pixmap = 0;
 *
 *	   pixmapAtom = XInternAtom (dpy, "PIXMAP", FALSE);
 *
 *	   for (i = 0; pixmap == 0 && i < 2; i++)
 *	   {
 *	status = XGetWindowProperty (dpy, screen->root,
 *					 screen->display->xBackgroundAtom[i],
 *					 0, 4, FALSE, AnyPropertyType,
 *					 &actualType, &actualFormat, &nItems,
 *					 &bytesAfter, &prop);
 *
 *	if (status == Success && nItems && prop)
 *	{
 *		if (actualType	 == pixmapAtom &&
 *		actualFormat == 32		   &&
 *		nItems		 == 1)
 *		{
 *		Pixmap p;
 *
 *		memcpy (&p, prop, 4);
 *
 *		if (p)
 *		{
 *			unsigned int ui;
 *			int		 i;
 *			Window	 w;
 *
 *			if (XGetGeometry (dpy, p, &w, &i, &i,
 *					  &width, &height, &ui, &depth))
 *			{
 *			if (depth == screen->attrib.depth)
 *				pixmap = p;
 *			}
 *		}
 *		}
 *
 *		XFree (prop);
 *	}
 *	   }
 *
 *	   if (pixmap)
 *	   {
 *	if (pixmap == texture->pixmap)
 *		return;
 *
 *	finiTexture (screen, texture);
 *	initTexture (screen, texture);
 *
 *	if (!bindPixmapToTexture (screen, texture, pixmap,
 *				  width, height, depth))
 *	{
 *		compLogMessage (NULL, "core", CompLogLevelWarn,
 *				"Couldn't bind background pixmap 0x%x to "
 *				"texture", (int) pixmap);
 *	}
 *	   }
 *	   else
 *	   {
 *	finiTexture (screen, texture);
 *	initTexture (screen, texture);
 *	   }
 *
 *	   if (!texture->name && backgroundImage)
 *	readImageToTexture (screen, texture, backgroundImage, &width, &height);
 *
 *	   if (texture->target == GL_TEXTURE_2D)
 *	   {
 *	glBindTexture (texture->target, texture->name);
 *	glTexParameteri (texture->target, GL_TEXTURE_WRAP_S, GL_REPEAT);
 *	glTexParameteri (texture->target, GL_TEXTURE_WRAP_T, GL_REPEAT);
 *	glBindTexture (texture->target, 0);
 *	   }
 * } */

void
detectRefreshRateOfScreen (CompScreen *s)
{
	if (!noDetection && s->opt[COMP_SCREEN_OPTION_DETECT_REFRESH_RATE].value.b)
	{
		char		*name;
		CompOptionValue	value;

		value.i = 0;

		if (s->display->randrExtension)
		{
			XRRScreenConfiguration *config;

			config	= XRRGetScreenInfo (s->display->display, s->root);
			value.i = (int) XRRConfigCurrentRate (config);

			XRRFreeScreenConfigInfo (config);
		}

		if (value.i == 0)
			value.i = defaultRefreshRate;

		name = s->opt[COMP_SCREEN_OPTION_REFRESH_RATE].name;

		s->opt[COMP_SCREEN_OPTION_DETECT_REFRESH_RATE].value.b = FALSE;
		(*s->setScreenOption) (s, name, &value);
		s->opt[COMP_SCREEN_OPTION_DETECT_REFRESH_RATE].value.b = TRUE;
	}
	else
	{
		s->redrawTime = 1000 / s->opt[COMP_SCREEN_OPTION_REFRESH_RATE].value.i;
		s->optimalRedrawTime = s->redrawTime;
	}
}


/* static voida
 * setSupportingWmCheck (CompScreen *s)
 * {
 *	 //CompDisplay *d = s->display;
 *
 *	   /\*XChangeProperty (d->display, s->grabWindow, d->supportingWmCheckAtom,
 *			 XA_WINDOW, 32, PropModeReplace,
 *			 (unsigned char *) &s->grabWindow, 1);
 *	   *\/
 *	   /\*XChangeProperty (d->display, s->grabWindow, d->wmNameAtom,
 *			 d->utf8StringAtom, 8, PropModeReplace,
 *			 (unsigned char *) PACKAGE, strlen (PACKAGE));*\/
 *
 *	   /\*
 *	  XChangeProperty (d->display, s->grabWindow, d->winStateAtom,
 *			 XA_ATOM, 32, PropModeReplace,
 *			 (unsigned char *) &d->winStateSkipTaskbarAtom, 1);
 *	   XChangeProperty (d->display, s->grabWindow, d->winStateAtom,
 *			 XA_ATOM, 32, PropModeAppend,
 *			 (unsigned char *) &d->winStateSkipPagerAtom, 1);
 *	   XChangeProperty (d->display, s->grabWindow, d->winStateAtom,
 *			 XA_ATOM, 32, PropModeAppend,
 *			 (unsigned char *) &d->winStateHiddenAtom, 1);
 *	   *\/
 *	   /\*
 *	   XChangeProperty (d->display, s->root, d->supportingWmCheckAtom,
 *			 XA_WINDOW, 32, PropModeReplace,
 *			 (unsigned char *) &s->grabWindow, 1);
 *	   *\/
 * } */


static void
getDesktopHints (CompScreen *s)
{
	CompDisplay	  *d = s->display;
	unsigned long data[2];
	Atom	  actual;
	int		  result, format;
	unsigned long n, left;
	unsigned char *propData;

	result = XGetWindowProperty (s->display->display, s->root,
								 d->desktopGeometryAtom, 0L, 2L,
								 FALSE, XA_CARDINAL, &actual, &format,
								 &n, &left, &propData);

	if (result == Success && n && propData)
	{
		if (n == 2)
		{
			memcpy (data, propData, sizeof (unsigned long) * 2);

			s->hsize = data[0] / s->width;
			s->vsize = data[1] / s->height;
			printf("Got Desktop Geometry %dx%d\n", s->hsize, s->vsize);

		}

		XFree (propData);
	}

	result = XGetWindowProperty (s->display->display, s->root,
								 d->desktopViewportAtom, 0L, 2L,
								 FALSE, XA_CARDINAL, &actual, &format,
								 &n, &left, &propData);

	if (result == Success && n && propData)
	{
		if (n == 2)
		{
			memcpy (data, propData, sizeof (unsigned long) * 2);

			if (data[0] / s->width < s->hsize - 1)
				s->x = data[0] / s->width;

			if (data[1] / s->height < s->vsize - 1)
				s->y = data[1] / s->height;
		}

		XFree (propData);
	}


	/*	 result = XGetWindowProperty (s->display->display, s->root,
	 *	 d->currentDesktopAtom, 0L, 1L, FALSE,
	 *	 XA_CARDINAL, &actual, &format,
	 *	 &n, &left, &propData);
	 *
	 *	 if (result == Success && n && propData && useDesktopHints)
	 *	 {
	 *	 memcpy (data, propData, sizeof (unsigned long));
	 *	 XFree (propData);
	 *
	 *	 if (data[0] < s->nDesktop)
	 *	 s->currentDesktop = data[0];
	 *	 }
	 *
	 * result = XGetWindowProperty (s->display->display, s->root,
	 *	  d->showingDesktopAtom, 0L, 1L, FALSE,
	 *	  XA_CARDINAL, &actual, &format,
	 *	  &n, &left, &propData);
	 *
	 *	  if (result == Success && n && propData)
	 *	  {
	 *	  memcpy (data, propData, sizeof (unsigned long));
	 *	  XFree (propData);
	 *
	 *	  if (data[0])
	 *	  (*s->enterShowDesktopMode) (s);
	 *	  }
	 *
	 * data[0] = s->currentDesktop;
	 *
	 *	 XChangeProperty (d->display, s->root, d->currentDesktopAtom,
	 *	 XA_CARDINAL, 32, PropModeReplace,
	 *	 (unsigned char *) data, 1);
	 *
	 *	 data[0] = s->showingDesktopMask ? TRUE : FALSE;
	 *
	 *	 XChangeProperty (d->display, s->root, d->showingDesktopAtom,
	 *	 XA_CARDINAL, 32, PropModeReplace,
	 *	 (unsigned char *) data, 1); */
}

void
showOutputWindow (CompScreen *s)
{

#ifdef USE_COW
	if (useCow)
	{
		Display		  *dpy = s->display->display;
		XserverRegion region;

		region = XFixesCreateRegion (dpy, NULL, 0);

		XFixesSetWindowShapeRegion (dpy,
									s->output,
									ShapeBounding,
									0, 0, 0);
		XFixesSetWindowShapeRegion (dpy,
									s->output,
									ShapeInput,
									0, 0, region);

		XFixesDestroyRegion (dpy, region);

		damageScreen (s);
	}
#endif

}

void
hideOutputWindow (CompScreen *s)
{

#ifdef USE_COW
	if (useCow)
	{
		Display		  *dpy = s->display->display;
		XserverRegion region;

		region = XFixesCreateRegion (dpy, NULL, 0);

		XFixesSetWindowShapeRegion (dpy,
									s->output,
									ShapeBounding,
									0, 0, region);

		XFixesDestroyRegion (dpy, region);
	}
#endif

}

static void
makeOutputWindow (CompScreen *s)
{

#ifdef USE_COW
	if (useCow)
	{
		s->overlay = XCompositeGetOverlayWindow (s->display->display, s->root);
		s->output  = s->overlay;

		XSelectInput (s->display->display, s->output, ExposureMask);
	}
	else
#endif

		s->output = s->overlay = s->root;

	showOutputWindow (s);
}

static void
enterShowDesktopMode (CompScreen *s)
{
	/* CompDisplay	 *d = s->display;
	 *	  CompWindow	*w;
	 *	  unsigned long data = 1;
	 *	  int		  count = 0;
	 *	  CompOption	*st = &d->opt[COMP_DISPLAY_OPTION_HIDE_SKIP_TASKBAR_WINDOWS];
	 *
	 *	  s->showingDesktopMask = ~(CompWindowTypeDesktopMask |
	 *	  CompWindowTypeDockMask);
	 *
	 *	  for (w = s->windows; w; w = w->next)
	 *	  {
	 *	  if ((s->showingDesktopMask & w->type) &&
	 *	  (!(w->state & CompWindowStateSkipTaskbarMask) || st->value.b))
	 *	  {
	 *	  if (!w->inShowDesktopMode && (*s->focusWindow) (w))
	 *	  {
	 *	  w->inShowDesktopMode = TRUE;
	 *	  hideWindow (w);
	 *	  }
	 *	  }
	 *
	 *	  if (w->inShowDesktopMode)
	 *	  count++;
	 *	  }
	 *
	 *	  if (!count)
	 *	  {
	 *	  s->showingDesktopMask = 0;
	 *	  data = 0;
	 *	  }
	 *
	 *	  XChangeProperty (s->display->display, s->root,
	 *	  s->display->showingDesktopAtom,
	 *	  XA_CARDINAL, 32, PropModeReplace,
	 *	  (unsigned char *) &data, 1); */
}

static void
leaveShowDesktopMode (CompScreen *s,
					  CompWindow *window)
{
	/* CompWindow	 *w;
	 * unsigned long data = 0;
	 *
	 * if (window)
	 * {
	 *	if (!window->inShowDesktopMode)
	 *		return;
	 *
	 *	window->inShowDesktopMode = FALSE;
	 *	showWindow (window);
	 *
	 *	\* return if some other window is still in show desktop mode *\/
	 *	for (w = s->windows; w; w = w->next)
	 *		if (w->inShowDesktopMode)
	 *		return;
	 *
	 *	s->showingDesktopMask = 0;
	 * }
	 * else
	 * {
	 *	s->showingDesktopMask = 0;
	 *
	 *	for (w = s->windows; w; w = w->next)
	 *	{
	 *		if (!w->inShowDesktopMode)
	 *		continue;
	 *
	 *		w->inShowDesktopMode = FALSE;
	 *		showWindow (w);
	 *	}
	 *
	 *	/\* focus default window - most likely this will be the window
	 *	   which had focus before entering showdesktop mode *\/
	 *	//focusDefaultWindow (s->display);
	 * }
	 *
	 * XChangeProperty (s->display->display, s->root,
	 *			 s->display->showingDesktopAtom,
	 *			 XA_CARDINAL, 32, PropModeReplace,
	 *			 (unsigned char *) &data, 1); */
}

static CompWindow *
walkFirst (CompScreen *s)
{
	return s->windows;
}

static CompWindow *
walkLast (CompScreen *s)
{
	return s->reverseWindows;
}

static CompWindow *
walkNext (CompWindow *w)
{
	return w->next;
}

static CompWindow *
walkPrev (CompWindow *w)
{
	return w->prev;
}

static void
initWindowWalker (CompScreen *screen,
				  CompWalker *walker)
{
	walker->fini  = NULL;
	walker->first = walkFirst;
	walker->last  = walkLast;
	walker->next  = walkNext;
	walker->prev  = walkPrev;
}

Bool
addScreen (CompDisplay *display,
		   int		   screenNum,
		   Window	   wmSnSelectionWindow,
		   Atom		   wmSnAtom,
		   Time		   wmSnTimestamp)
{
	D(("addScreen - begin %d\n", screenNum));

	CompScreen		 *s;
	Display		 *dpy = display->display;
	static char		 data = 0;
	XColor		 black;
	Pixmap		 bitmap;
	XVisualInfo		 templ;
	XVisualInfo		 *visinfo;
	GLXFBConfig		 *fbConfigs;
	Window		 rootReturn, parentReturn;
	Window		 *children;
	unsigned int	 nchildren;
	int			 defaultDepth, nvisinfo, nElements, value, i;
	const char		 *glxExtensions, *glExtensions;
	/* XSetWindowAttributes attrib; */
	GLfloat		 globalAmbient[]  = { 0.1f, 0.1f,  0.1f, 0.1f };
	GLfloat		 ambientLight[]	  = { 0.0f, 0.0f,  0.0f, 0.0f };
	GLfloat		 diffuseLight[]	  = { 0.9f, 0.9f,  0.9f, 0.9f };
	GLfloat		 light0Position[] = { -0.5f, 0.5f, -9.0f, 1.0f };
	CompWindow		 *w;

	s = malloc (sizeof (CompScreen));
	if (!s)
		return FALSE;

	s->windowPrivateIndices = 0;
	s->windowPrivateLen		= 0;

	if (display->screenPrivateLen)
	{
		s->privates = malloc (display->screenPrivateLen *
							  sizeof (CompPrivate));
		if (!s->privates)
		{
			free (s);
			return FALSE;
		}
	}
	else
		s->privates = 0;

	s->display = display;

	if (!compInitScreenOptionsFromMetadata (s,
											&coreMetadata,
											coreScreenOptionInfo,
											s->opt,
											COMP_SCREEN_OPTION_NUM))
		return FALSE;

	s->damage = XCreateRegion ();
	if (!s->damage)
		return FALSE;

	s->x	 = 0;
	s->y	 = 0;

	/* get this info from e */
	s->hsize = 1;
	s->vsize = 1;

	s->nDesktop		  = 1;
	s->currentDesktop = 0;

	/* for (i = 0; i < SCREEN_EDGE_NUM; i++)
	 * {
	 *	s->screenEdge[i].id	   = None;
	 *	s->screenEdge[i].count = 0;
	 * } */

	/* s->buttonGrab  = 0;
	 * s->nButtonGrab = 0;
	 * s->keyGrab	  = 0;
	 * s->nKeyGrab	  = 0; */

	s->grabs	= 0;
	s->grabSize = 0;
	s->maxGrab	= 0;

	s->pendingDestroys = 0;

	s->clientList  = 0;
	s->nClientList = 0;

	s->screenNum = screenNum;
	s->colormap	 = DefaultColormap (dpy, screenNum);
	s->root	 = XRootWindow (dpy, screenNum);

	s->mapNum	 = 1;
	s->activeNum = 1;

	/* s->groups = NULL; */

	s->damageMask  = COMP_SCREEN_DAMAGE_ALL_MASK;
	s->next	   = 0;
	s->exposeRects = 0;
	s->sizeExpose  = 0;
	s->nExpose	   = 0;

	s->rasterX = 0;
	s->rasterY = 0;

	s->outputDev	= NULL;
	s->nOutputDev	= 0;
	s->currentOutputDev = 0;

	s->windows = 0;
	s->reverseWindows = 0;

	s->nextRedraw  = 0;
	s->frameStatus = 0;
	s->timeMult	   = 1;
	s->idle	   = TRUE;
	s->timeLeft	   = 0;

	s->pendingCommands = TRUE;

	s->lastFunctionId = 0;

	s->fragmentFunctions = NULL;
	s->fragmentPrograms = NULL;

	memset (s->saturateFunction, 0, sizeof (s->saturateFunction));

	s->showingDesktopMask = 0;

	memset (s->history, 0, sizeof (s->history));
	s->currentHistory = 0;

	s->overlayWindowCount = 0;

	s->desktopHintData = NULL;
	s->desktopHintSize = 0;

	s->cursors = NULL;

	s->clearBuffers = TRUE;

	gettimeofday (&s->lastRedraw, 0);

	s->setScreenOption			= setScreenOption;
	s->setScreenOptionForPlugin = setScreenOptionForPlugin;

	s->initPluginForScreen = initPluginForScreen;
	s->finiPluginForScreen = finiPluginForScreen;

	s->preparePaintScreen	  = preparePaintScreen;
	s->donePaintScreen		  = donePaintScreen;
	s->paintScreen		  = paintScreen;
	s->paintOutput		  = paintOutput;
	s->paintTransformedOutput	  = paintTransformedOutput;
	s->applyScreenTransform	  = applyScreenTransform;
	/* s->paintBackground		  = paintBackground; */
	s->paintWindow		  = paintWindow;
	s->drawWindow		  = drawWindow;
	s->addWindowGeometry	  = addWindowGeometry;
	s->drawWindowTexture	  = drawWindowTexture;
	s->damageWindowRect		  = damageWindowRect;
	s->getOutputExtentsForWindow  = getOutputExtentsForWindow;
	s->getAllowedActionsForWindow = getAllowedActionsForWindow;
	s->focusWindow		  = focusWindow;
	s->placeWindow				  = placeWindow;

	/* s->paintCursor	   = paintCursor;
	 * s->damageCursorRect	= damageCursorRect; */

	s->windowAddNotify	  = windowAddNotify;
	s->windowResizeNotify = windowResizeNotify;
	s->windowMoveNotify	  = windowMoveNotify;
	s->windowGrabNotify	  = windowGrabNotify;
	s->windowUngrabNotify = windowUngrabNotify;

	s->enterShowDesktopMode = enterShowDesktopMode;
	s->leaveShowDesktopMode = leaveShowDesktopMode;

	s->windowStateChangeNotify = windowStateChangeNotify;

	s->outputChangeNotify = outputChangeNotify;

	s->initWindowWalker = initWindowWalker;

	s->getProcAddress = 0;

	if (!XGetWindowAttributes (dpy, s->root, &s->attrib))
		return FALSE;

	s->workArea.x	   = 0;
	s->workArea.y	   = 0;
	s->workArea.width  = s->attrib.width;
	s->workArea.height = s->attrib.height;

	//	  s->grabWindow = None;

	makeOutputWindow (s);

	templ.visualid = XVisualIDFromVisual (s->attrib.visual);

	visinfo = XGetVisualInfo (dpy, VisualIDMask, &templ, &nvisinfo);
	if (!nvisinfo)
	{
		compLogMessage (display, "core", CompLogLevelFatal,
						"Couldn't get visual info for default visual");
		return FALSE;
	}

	defaultDepth = visinfo->depth;

	black.red = black.green = black.blue = 0;

	if (!XAllocColor (dpy, s->colormap, &black))
	{
		compLogMessage (display, "core", CompLogLevelFatal,
						"Couldn't allocate color");
		XFree (visinfo);
		return FALSE;
	}

	bitmap = XCreateBitmapFromData (dpy, s->root, &data, 1, 1);
	if (!bitmap)
	{
		compLogMessage (display, "core", CompLogLevelFatal,
						"Couldn't create bitmap");
		XFree (visinfo);
		return FALSE;
	}

	s->invisibleCursor = XCreatePixmapCursor (dpy, bitmap, bitmap,
											  &black, &black, 0, 0);
	if (!s->invisibleCursor)
	{
		compLogMessage (display, "core", CompLogLevelFatal,
						"Couldn't create invisible cursor");
		XFree (visinfo);
		return FALSE;
	}

	XFreePixmap (dpy, bitmap);
	XFreeColors (dpy, s->colormap, &black.pixel, 1, 0);

	glXGetConfig (dpy, visinfo, GLX_USE_GL, &value);
	if (!value)
	{
		compLogMessage (display, "core", CompLogLevelFatal,
						"Root visual is not a GL visual");
		XFree (visinfo);
		return FALSE;
	}

	glXGetConfig (dpy, visinfo, GLX_DOUBLEBUFFER, &value);
	if (!value)
	{
		compLogMessage (display, "core", CompLogLevelFatal,
						"Root visual is not a double buffered GL visual");
		XFree (visinfo);
		return FALSE;
	}

	s->ctx = glXCreateContext (dpy, visinfo, NULL, !indirectRendering);
	if (!s->ctx)
	{
		compLogMessage (display, "core", CompLogLevelFatal,
						"glXCreateContext failed");
		XFree (visinfo);

		return FALSE;
	}

	glxExtensions = glXQueryExtensionsString (dpy, screenNum);
	if (!strstr (glxExtensions, "GLX_EXT_texture_from_pixmap"))
	{
		compLogMessage (display, "core", CompLogLevelFatal,
						"GLX_EXT_texture_from_pixmap is missing");
		XFree (visinfo);

		return FALSE;
	}

	XFree (visinfo);

	if (!strstr (glxExtensions, "GLX_SGIX_fbconfig"))
	{
		compLogMessage (display, "core", CompLogLevelFatal,
						"GLX_SGIX_fbconfig is missing");
		return FALSE;
	}

	s->getProcAddress = (GLXGetProcAddressProc)
		getProcAddress (s, "glXGetProcAddressARB");
	s->bindTexImage = (GLXBindTexImageProc)
		getProcAddress (s, "glXBindTexImageEXT");
	s->releaseTexImage = (GLXReleaseTexImageProc)
		getProcAddress (s, "glXReleaseTexImageEXT");
	s->queryDrawable = (GLXQueryDrawableProc)
		getProcAddress (s, "glXQueryDrawable");
	s->getFBConfigs = (GLXGetFBConfigsProc)
		getProcAddress (s, "glXGetFBConfigs");
	s->getFBConfigAttrib = (GLXGetFBConfigAttribProc)
		getProcAddress (s, "glXGetFBConfigAttrib");
	s->createPixmap = (GLXCreatePixmapProc)
		getProcAddress (s, "glXCreatePixmap");

	if (!s->bindTexImage)
	{
		compLogMessage (display, "core", CompLogLevelFatal,
						"glXBindTexImageEXT is missing");
		return FALSE;
	}

	if (!s->releaseTexImage)
	{
		compLogMessage (display, "core", CompLogLevelFatal,
						"glXReleaseTexImageEXT is missing");
		return FALSE;
	}

	if (!s->queryDrawable	  ||
		!s->getFBConfigs	  ||
		!s->getFBConfigAttrib ||
		!s->createPixmap)
	{
		compLogMessage (display, "core", CompLogLevelFatal,
						"fbconfig functions missing");
		return FALSE;
	}

	s->copySubBuffer = NULL;
	if (strstr (glxExtensions, "GLX_MESA_copy_sub_buffer"))
		s->copySubBuffer = (GLXCopySubBufferProc)
			getProcAddress (s, "glXCopySubBufferMESA");

	s->getVideoSync = NULL;
	s->waitVideoSync = NULL;
	if (strstr (glxExtensions, "GLX_SGI_video_sync"))
	{
		s->getVideoSync = (GLXGetVideoSyncProc)
			getProcAddress (s, "glXGetVideoSyncSGI");

		s->waitVideoSync = (GLXWaitVideoSyncProc)
			getProcAddress (s, "glXWaitVideoSyncSGI");
	}

	glXMakeCurrent (dpy, s->output, s->ctx);
	currentRoot = s->root;

	glExtensions = (const char *) glGetString (GL_EXTENSIONS);
	if (!glExtensions)
	{
		compLogMessage (display, "core", CompLogLevelFatal,
						"No valid GL extensions string found.");
		return FALSE;
	}

	s->textureNonPowerOfTwo = 0;
	if (strstr (glExtensions, "GL_ARB_texture_non_power_of_two"))
		s->textureNonPowerOfTwo = 1;

	glGetIntegerv (GL_MAX_TEXTURE_SIZE, &s->maxTextureSize);

	s->textureRectangle = 0;
	if (strstr (glExtensions, "GL_NV_texture_rectangle")  ||
		strstr (glExtensions, "GL_EXT_texture_rectangle") ||
		strstr (glExtensions, "GL_ARB_texture_rectangle"))
	{
		s->textureRectangle = 1;

		if (!s->textureNonPowerOfTwo)
		{
			GLint maxTextureSize;

			glGetIntegerv (GL_MAX_RECTANGLE_TEXTURE_SIZE_NV, &maxTextureSize);
			if (maxTextureSize > s->maxTextureSize)
				s->maxTextureSize = maxTextureSize;
		}
	}

	if (!(s->textureRectangle || s->textureNonPowerOfTwo))
	{
		compLogMessage (display, "core", CompLogLevelFatal,
						"Support for non power of two textures missing");
		return FALSE;
	}

	s->textureEnvCombine = s->textureEnvCrossbar = 0;
	if (strstr (glExtensions, "GL_ARB_texture_env_combine"))
	{
		s->textureEnvCombine = 1;

		/* XXX: GL_NV_texture_env_combine4 need special code but it seams to
		   be working anyway for now... */
		if (strstr (glExtensions, "GL_ARB_texture_env_crossbar") ||
			strstr (glExtensions, "GL_NV_texture_env_combine4"))
			s->textureEnvCrossbar = 1;
	}

	s->textureBorderClamp = 0;
	if (strstr (glExtensions, "GL_ARB_texture_border_clamp") ||
		strstr (glExtensions, "GL_SGIS_texture_border_clamp"))
		s->textureBorderClamp = 1;

	s->maxTextureUnits = 1;
	if (strstr (glExtensions, "GL_ARB_multitexture"))
	{
		s->activeTexture = (GLActiveTextureProc)
			getProcAddress (s, "glActiveTexture");
		s->clientActiveTexture = (GLClientActiveTextureProc)
			getProcAddress (s, "glClientActiveTexture");

		if (s->activeTexture && s->clientActiveTexture)
			glGetIntegerv (GL_MAX_TEXTURE_UNITS_ARB, &s->maxTextureUnits);
	}

	s->fragmentProgram = 0;
	if (strstr (glExtensions, "GL_ARB_fragment_program"))
	{
		s->genPrograms = (GLGenProgramsProc)
			getProcAddress (s, "glGenProgramsARB");
		s->deletePrograms = (GLDeleteProgramsProc)
			getProcAddress (s, "glDeleteProgramsARB");
		s->bindProgram = (GLBindProgramProc)
			getProcAddress (s, "glBindProgramARB");
		s->programString = (GLProgramStringProc)
			getProcAddress (s, "glProgramStringARB");
		s->programEnvParameter4f = (GLProgramParameter4fProc)
			getProcAddress (s, "glProgramEnvParameter4fARB");
		s->programLocalParameter4f = (GLProgramParameter4fProc)
			getProcAddress (s, "glProgramLocalParameter4fARB");

		if (s->genPrograms		 &&
			s->deletePrograms		 &&
			s->bindProgram		 &&
			s->programString		 &&
			s->programEnvParameter4f &&
			s->programLocalParameter4f)
			s->fragmentProgram = 1;
	}

	s->fbo = 0;
	if (strstr (glExtensions, "GL_EXT_framebuffer_object"))
	{
		s->genFramebuffers = (GLGenFramebuffersProc)
			getProcAddress (s, "glGenFramebuffersEXT");
		s->deleteFramebuffers = (GLDeleteFramebuffersProc)
			getProcAddress (s, "glDeleteFramebuffersEXT");
		s->bindFramebuffer = (GLBindFramebufferProc)
			getProcAddress (s, "glBindFramebufferEXT");
		s->checkFramebufferStatus = (GLCheckFramebufferStatusProc)
			getProcAddress (s, "glCheckFramebufferStatusEXT");
		s->framebufferTexture2D = (GLFramebufferTexture2DProc)
			getProcAddress (s, "glFramebufferTexture2DEXT");
		s->generateMipmap = (GLGenerateMipmapProc)
			getProcAddress (s, "glGenerateMipmapEXT");

		if (s->genFramebuffers		  &&
			s->deleteFramebuffers	  &&
			s->bindFramebuffer		  &&
			s->checkFramebufferStatus &&
			s->framebufferTexture2D	  &&
			s->generateMipmap)
			s->fbo = 1;
	}

	fbConfigs = (*s->getFBConfigs) (dpy,
									screenNum,
									&nElements);

	for (i = 0; i <= MAX_DEPTH; i++)
	{
		int j, db, stencil, depth, alpha, mipmap, rgba;

		s->glxPixmapFBConfigs[i].fbConfig		= NULL;
		s->glxPixmapFBConfigs[i].mipmap			= 0;
		s->glxPixmapFBConfigs[i].yInverted		= 0;
		s->glxPixmapFBConfigs[i].textureFormat	= 0;
		s->glxPixmapFBConfigs[i].textureTargets = 0;

		db		= MAXSHORT;
		stencil = MAXSHORT;
		depth	= MAXSHORT;
		mipmap	= 0;
		rgba	= 0;

		for (j = 0; j < nElements; j++)
		{
			XVisualInfo *vi;
			int		visualDepth;

			vi = glXGetVisualFromFBConfig (dpy, fbConfigs[j]);
			if (vi == NULL)
				continue;

			visualDepth = vi->depth;

			XFree (vi);

			if (visualDepth != i)
				continue;

			(*s->getFBConfigAttrib) (dpy, fbConfigs[j], GLX_ALPHA_SIZE, &alpha);
			(*s->getFBConfigAttrib) (dpy, fbConfigs[j], GLX_BUFFER_SIZE, &value);
			if (value != i && (value - alpha) != i)
				continue;

			value = 0;
			if (i == 32)
			{
				(*s->getFBConfigAttrib) (dpy, fbConfigs[j], GLX_BIND_TO_TEXTURE_RGBA_EXT, &value);

				if (value)
				{
					rgba = 1;

					s->glxPixmapFBConfigs[i].textureFormat =
						GLX_TEXTURE_FORMAT_RGBA_EXT;
				}
			}

			if (!value)
			{
				if (rgba)
					continue;

				(*s->getFBConfigAttrib) (dpy, fbConfigs[j], GLX_BIND_TO_TEXTURE_RGB_EXT, &value);
				if (!value)
					continue;

				s->glxPixmapFBConfigs[i].textureFormat =
					GLX_TEXTURE_FORMAT_RGB_EXT;
			}

			(*s->getFBConfigAttrib) (dpy, fbConfigs[j], GLX_DOUBLEBUFFER, &value);
			if (value > db)
				continue;

			db = value;

			(*s->getFBConfigAttrib) (dpy, fbConfigs[j], GLX_STENCIL_SIZE, &value);
			if (value > stencil)
				continue;

			stencil = value;

			(*s->getFBConfigAttrib) (dpy, fbConfigs[j], GLX_DEPTH_SIZE, &value);
			if (value > depth)
				continue;

			depth = value;

			if (s->fbo)
			{
				(*s->getFBConfigAttrib) (dpy, fbConfigs[j], GLX_BIND_TO_MIPMAP_TEXTURE_EXT, &value);
				if (value < mipmap)
					continue;

				mipmap = value;
			}

			(*s->getFBConfigAttrib) (dpy, fbConfigs[j], GLX_Y_INVERTED_EXT, &value);

			s->glxPixmapFBConfigs[i].yInverted = value;

			(*s->getFBConfigAttrib) (dpy, fbConfigs[j], GLX_BIND_TO_TEXTURE_TARGETS_EXT, &value);

			s->glxPixmapFBConfigs[i].textureTargets = value;

			s->glxPixmapFBConfigs[i].fbConfig = fbConfigs[j];
			s->glxPixmapFBConfigs[i].mipmap	  = mipmap;
		}
	}

	if (nElements)
		XFree (fbConfigs);

	if (!s->glxPixmapFBConfigs[defaultDepth].fbConfig)
	{
		compLogMessage (display, "core", CompLogLevelFatal,
						"No GLXFBConfig for default depth, "
						"this isn't going to work.");
		return FALSE;
	}

	/* initTexture (s, &s->backgroundTexture);
	 * s->backgroundLoaded = FALSE; */

	s->defaultIcon = NULL;

	s->desktopWindowCount = 0;

	glClearColor (0.0, 0.0, 0.0, 1.0);
	glBlendFunc (GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
	glEnable (GL_CULL_FACE);
	glDisable (GL_BLEND);
	glTexEnvi (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
	glColor4usv (defaultColor);
	glEnableClientState (GL_VERTEX_ARRAY);
	glEnableClientState (GL_TEXTURE_COORD_ARRAY);

	s->canDoSaturated = s->canDoSlightlySaturated = FALSE;
	if (s->textureEnvCombine && s->maxTextureUnits >= 2)
	{
		s->canDoSaturated = TRUE;
		if (s->textureEnvCrossbar && s->maxTextureUnits >= 4)
			s->canDoSlightlySaturated = TRUE;
	}

	s->redrawTime = 1000 / defaultRefreshRate;
	s->optimalRedrawTime = s->redrawTime;

	reshape (s, s->attrib.width, s->attrib.height);

	detectRefreshRateOfScreen (s);
	detectOutputDevices (s);
	updateOutputDevices (s);

	glLightModelfv (GL_LIGHT_MODEL_AMBIENT, globalAmbient);

	glEnable (GL_LIGHT0);
	glLightfv (GL_LIGHT0, GL_AMBIENT, ambientLight);
	glLightfv (GL_LIGHT0, GL_DIFFUSE, diffuseLight);
	glLightfv (GL_LIGHT0, GL_POSITION, light0Position);

	glColorMaterial (GL_FRONT, GL_AMBIENT_AND_DIFFUSE);

	glNormal3f (0.0f, 0.0f, -1.0f);

	s->lighting		  = FALSE;
	s->slowAnimations = FALSE;

	addScreenToDisplay (display, s);

	getDesktopHints (s);

	screenInitPlugins (s);

	XQueryTree (dpy, s->root,
				&rootReturn, &parentReturn,
				&children, &nchildren);

	for (i = 0; i < nchildren; i++)
		addWindow (s, children[i], i ? children[i - 1] : 0);

	for (w = s->windows; w; w = w->next)
	{
		if (w->attrib.map_state == IsViewable)
		{
			w->activeNum = s->activeNum++;
			w->damaged	 = TRUE;
			w->invisible = WINDOW_INVISIBLE (w);
		}
	}

	XFree (children);

	/*	  setDesktopHints (s);
	 * setSupportingWmCheck (s);
	 * setSupported (s); */

	s->filter[NOTHING_TRANS_FILTER] = COMP_TEXTURE_FILTER_FAST;
	s->filter[SCREEN_TRANS_FILTER]	= COMP_TEXTURE_FILTER_GOOD;
	s->filter[WINDOW_TRANS_FILTER]	= COMP_TEXTURE_FILTER_GOOD;

	D(("addScreen - end\n"));

	return TRUE;
}

void
damageScreenRegion (CompScreen *screen, Region region)
{
	if (screen->damageMask & COMP_SCREEN_DAMAGE_ALL_MASK)
		return;

	XUnionRegion (screen->damage, region, screen->damage);

	screen->damageMask |= COMP_SCREEN_DAMAGE_REGION_MASK;
}

void
damageScreen (CompScreen *s)
{
	s->damageMask |= COMP_SCREEN_DAMAGE_ALL_MASK;
	s->damageMask &= ~COMP_SCREEN_DAMAGE_REGION_MASK;
}

void
damagePendingOnScreen (CompScreen *s)
{
	s->damageMask |= COMP_SCREEN_DAMAGE_PENDING_MASK;
}

void
forEachWindowOnScreen (CompScreen *screen, ForEachWindowProc proc, void *closure)
{
	CompWindow *w;

	for (w = screen->windows; w; w = w->next)
		(*proc) (w, closure);
}

CompWindow *
findWindowAtScreen (CompScreen *s, Window id)
{
	if (lastFoundWindow && lastFoundWindow->id == id)
	{
		return lastFoundWindow;
	}
	else
	{
		CompWindow *w;

		for (w = s->windows; w; w = w->next)
			if (w->id == id)
				return (lastFoundWindow = w);
	}

	return 0;
}

CompWindow *
findTopLevelWindowAtScreen (CompScreen *s, Window id)
{
	CompWindow *w;

	w = findWindowAtScreen (s, id);
	if (!w)
		return NULL;

	return w;
}

void
insertWindowIntoScreen (CompScreen *s, CompWindow *w, Window aboveId)
{
	CompWindow *p;

	if (s->windows)
	{
		if (!aboveId)
		{
			w->next = s->windows;
			w->prev = NULL;
			s->windows->prev = w;
			s->windows = w;
		}
		else
		{
			for (p = s->windows; p; p = p->next)
			{
				if (p->id == aboveId)
				{
					if (p->next)
					{
						w->next = p->next;
						w->prev = p;
						p->next->prev = w;
						p->next = w;
					}
					else
					{
						p->next = w;
						w->next = NULL;
						w->prev = p;
						s->reverseWindows = w;
					}
					break;
				}
			}

#ifdef DEBUG
			if (!p)
				abort ();
#endif

		}
	}
	else
	{
		s->reverseWindows = s->windows = w;
		w->prev = w->next = NULL;
	}
}

void
unhookWindowFromScreen (CompScreen *s, CompWindow *w)
{
	CompWindow *next, *prev;

	next = w->next;
	prev = w->prev;

	if (next || prev)
	{
		if (next)
		{
			if (prev)
			{
				next->prev = prev;
			}
			else
			{
				s->windows = next;
				next->prev = NULL;
			}
		}

		if (prev)
		{
			if (next)
			{
				prev->next = next;
			}
			else
			{
				s->reverseWindows = prev;
				prev->next = NULL;
			}
		}
	}
	else
	{
		s->windows = s->reverseWindows = NULL;
	}

	if (w == lastFoundWindow)
		lastFoundWindow = NULL;
	if (w == lastDamagedWindow)
		lastDamagedWindow = NULL;
}

int
pushScreenGrab (CompScreen *s, Cursor cursor, const char *name)
{
	C(("pushScreenGrab\n"));
	if (s->grabSize <= s->maxGrab)
	{
		s->grabs = realloc (s->grabs, sizeof (CompGrab) * (s->maxGrab + 1));
		if (!s->grabs)
			return 0;

		s->grabSize = s->maxGrab + 1;
	}

	s->grabs[s->maxGrab].cursor = cursor;
	s->grabs[s->maxGrab].active = TRUE;
	s->grabs[s->maxGrab].name	= name;

	s->maxGrab++;

	return s->maxGrab;
}

void
updateScreenGrab (CompScreen *s, int index, Cursor cursor)
{
	D(("updateScreenGrab\n"));
}

void
removeScreenGrab (CompScreen *s, int index, XPoint *restorePointer)
{
	D(("removeScreenGrab\n"));

	int maxGrab;

	index--;

/* #ifdef DEBUG
 *	   if (index < 0 || index >= s->maxGrab)
 *	abort ();
 * #endif */

	s->grabs[index].cursor = None;
	s->grabs[index].active = FALSE;

	for (maxGrab = s->maxGrab; maxGrab; maxGrab--)
		if (s->grabs[maxGrab - 1].active)
			break;

	if (maxGrab != s->maxGrab)
	{
		s->maxGrab = maxGrab;
	}
}

/* otherScreenGrabExist takes a series of strings terminated by a NULL.
   It returns TRUE if a grab exists but it is NOT held by one of the
   plugins listed, returns FALSE otherwise. */

Bool
otherScreenGrabExist (CompScreen *s, ...)
{
	va_list ap;
	char	*name;
	int		i;

	for (i = 0; i < s->maxGrab; i++)
	{
		if (s->grabs[i].active)
		{
			va_start (ap, s);

			name = va_arg (ap, char *);
			while (name)
			{
				if (strcmp (name, s->grabs[i].name) == 0)
					break;

				name = va_arg (ap, char *);
			}

			va_end (ap);

			if (!name)
				return TRUE;
		}
	}

	return FALSE;
}

/* XXX check if this is set by e17 */
Window
getActiveWindow (CompDisplay *display,
				 Window		 root)
{
	Atom	  actual;
	int		  result, format;
	unsigned long n, left;
	unsigned char *data;
	Window	  w = None;

	result = XGetWindowProperty (display->display, root,
								 display->winActiveAtom, 0L, 1L, FALSE,
								 XA_WINDOW, &actual, &format,
								 &n, &left, &data);

	if (result == Success && n && data)
	{
		memcpy (&w, data, sizeof (Window));
		XFree (data);
	}

	return w;
}

/* XXX use ECOMORPH_ATOM message  */
void
sendScreenViewportMessage(CompScreen *s)
{

	//printf ("sendMoveScreenViewportMessage %d:%d\n", s->x, s->y);

	CompDisplay *d = s->display;
	XEvent ev;

	ev.type			= ClientMessage;
	ev.xclient.window		= s->root;
	ev.xclient.message_type = d->desktopViewportAtom;
	ev.xclient.format		= 32;
	ev.xclient.data.l[0]	= 2; /* from ecomp to wm */
	ev.xclient.data.l[1]	= s->x;
	ev.xclient.data.l[2]	= s->y;
	ev.xclient.data.l[3]	= 0;
	ev.xclient.data.l[4]	= 0;

	XSendEvent (d->display, s->root, FALSE,
				SubstructureRedirectMask | StructureNotifyMask, &ev);
}


void
moveScreenViewport (CompScreen *s,
					int		   tx,
					int		   ty,
					Bool	   sync)
{
	CompWindow *w;
	int			m, wx, wy, vWidth, vHeight;

	tx = s->x - tx;
	tx = MOD (tx, s->hsize);
	tx -= s->x;

	ty = s->y - ty;
	ty = MOD (ty, s->vsize);
	ty -= s->y;

	s->x += tx;
	s->y += ty;

	tx *= -s->width;
	ty *= -s->height;

	vWidth = s->width * s->hsize;
	vHeight = s->height * s->vsize;

	for (w = s->windows; w; w = w->next)
	{
		if (!w->clientId)
			continue;

		if (!w->placed) continue;
		
		if (w->state & CompWindowStateStickyMask)
			continue;

		/* x */
		if (s->hsize == 1)
		{
			wx = tx;
		}
		else
		{
			m = w->attrib.x + tx;
			if (m - w->input.left < s->width - vWidth)
				wx = tx + vWidth;
			else if (m + w->width + w->input.right > vWidth)
				wx = tx - vWidth;
			else
				wx = tx;
		}

		/* y */
		if (s->vsize == 1)
		{
			wy = ty;
		}
		else
		{
			m = w->attrib.y + ty;
			if (m - w->input.top < s->height - vHeight)
				wy = ty + vHeight;
			else if (m + w->height + w->input.bottom > vHeight)
				wy = ty - vHeight;
			else
				wy = ty;
		}

		/* move */
		moveWindow (w, wx, wy, sync, TRUE);

		if (sync)
			syncWindowPosition (w);
	}
}

/* TODO cant this be handled by e? */
void
moveWindowToViewportPosition (CompWindow *w,
							  int	 x,
							  int		 y,
							  Bool		 sync)
{
	int	tx, vWidth = w->screen->width * w->screen->hsize;
	int ty, vHeight = w->screen->height * w->screen->vsize;

	if (w->screen->hsize != 1)
	{
		x += w->screen->x * w->screen->width;
		x = MOD (x, vWidth);
		x -= w->screen->x * w->screen->width;
	}

	if (w->screen->vsize != 1)
	{
		y += w->screen->y * w->screen->height;
		y = MOD (y, vHeight);
		y -= w->screen->y * w->screen->height;
	}

	tx = x - w->attrib.x;
	ty = y - w->attrib.y;

	if (tx || ty)
	{
		int m, wx, wy;

		wx = tx;
		wy = ty;

		if (w->screen->hsize != 1)
		{
			m = w->attrib.x + tx;

			if (m - w->output.left < w->screen->width - vWidth)
				wx = tx + vWidth;
			else if (m + w->width + w->output.right > vWidth)
				wx = tx - vWidth;
		}

		if (w->screen->vsize != 1)
		{
			m = w->attrib.y + ty;

			if (m - w->output.top < w->screen->height - vHeight)
				wy = ty + vHeight;
			else if (m + w->height + w->output.bottom > vHeight)
				wy = ty - vHeight;
		}

		moveWindow (w, wx, wy, sync, TRUE);

		if (sync)
			syncWindowPosition (w);
	}
}

/* XXX use ECOMORPH_ATOM message  */
void
sendWindowActivationRequest (CompScreen *s,
							 Window	id)
{
	XEvent xev;

	xev.xclient.type	= ClientMessage;
	xev.xclient.display = s->display->display;
	xev.xclient.format	= 32;

	xev.xclient.message_type = s->display->winActiveAtom;
	xev.xclient.window		 = id;

	xev.xclient.data.l[0] = 2;
	xev.xclient.data.l[1] = 0;
	xev.xclient.data.l[2] = 0;
	xev.xclient.data.l[3] = 0;
	xev.xclient.data.l[4] = 0;

	XSendEvent (s->display->display, s->root, FALSE,
				SubstructureRedirectMask | SubstructureNotifyMask,
				&xev);
}

void
screenTexEnvMode (CompScreen *s,
				  GLenum	 mode)
{
	if (s->lighting)
		glTexEnvi (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	else
		glTexEnvi (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, mode);
}

void
screenLighting (CompScreen *s, Bool lighting)
{
	if (s->lighting != lighting)
	{
		if (!s->opt[COMP_SCREEN_OPTION_LIGHTING].value.b)
			lighting = FALSE;

		if (lighting)
		{
			glEnable (GL_COLOR_MATERIAL);
			glEnable (GL_LIGHTING);
		}
		else
		{
			glDisable (GL_COLOR_MATERIAL);
			glDisable (GL_LIGHTING);
		}

		s->lighting = lighting;

		screenTexEnvMode (s, GL_REPLACE);
	}
}

Window
getTopWindow (CompScreen *s)
{
	CompWindow *w;

	/* return first window that has not been destroyed */
	for (w = s->reverseWindows; w; w = w->prev)
	{
		if (w->id > 1)
			return w->id;
	}

	return None;
}

void
makeScreenCurrent (CompScreen *s)
{
	if (currentRoot != s->root)
	{
		glXMakeCurrent (s->display->display, s->output, s->ctx);
		currentRoot = s->root;
	}

	s->pendingCommands = TRUE;
}

void
finishScreenDrawing (CompScreen *s)
{
	if (s->pendingCommands)
	{
		makeScreenCurrent (s);
		glFinish ();

		s->pendingCommands = FALSE;
	}
}

int
outputDeviceForPoint (CompScreen *s, int x, int y)
{
	int i, x1, y1, x2, y2;

	i = s->nOutputDev;
	while (i--)
	{
		x1 = s->outputDev[i].region.extents.x1;
		y1 = s->outputDev[i].region.extents.y1;
		x2 = s->outputDev[i].region.extents.x2;
		y2 = s->outputDev[i].region.extents.y2;

		if (x1 <= x && x2 > x && y1 <= y && y2 > y)
			return i;
	}

	return s->currentOutputDev;
}

void
getCurrentOutputExtents (CompScreen *s, int *x1, int *y1, int *x2, int *y2)
{
	if (x1) *x1 = s->outputDev[s->currentOutputDev].region.extents.x1;
	if (y1) *y1 = s->outputDev[s->currentOutputDev].region.extents.y1;
	if (x2) *x2 = s->outputDev[s->currentOutputDev].region.extents.x2;
	if (y2) *y2 = s->outputDev[s->currentOutputDev].region.extents.y2;
}


void
getWorkareaForOutput (CompScreen *s, int output, XRectangle *area)
{
	*area = s->outputDev[output].workArea;
}

void
setDefaultViewport (CompScreen *s)
{
	s->lastViewport.x	   = s->outputDev->region.extents.x1;
	s->lastViewport.y	   = s->height - s->outputDev->region.extents.y2;
	s->lastViewport.width  = s->outputDev->width;
	s->lastViewport.height = s->outputDev->height;

	glViewport (s->lastViewport.x,
				s->lastViewport.y,
				s->lastViewport.width,
				s->lastViewport.height);
}

void
outputChangeNotify (CompScreen *s)
{
}

void
clearScreenOutput (CompScreen	*s,
				   CompOutput	*output,
				   unsigned int mask)
{
	BoxPtr pBox = &output->region.extents;

	if (pBox->x1 != 0		 ||
		pBox->y1 != 0		 ||
		pBox->x2 != s->width ||
		pBox->y2 != s->height)
	{
		glPushAttrib (GL_SCISSOR_BIT);

		glEnable (GL_SCISSOR_TEST);
		glScissor (pBox->x1,
				   s->height - pBox->y2,
				   pBox->x2 - pBox->x1,
				   pBox->y2 - pBox->y1);
		glClear (mask);

		glPopAttrib ();
	}
	else
	{
		glClear (mask);
	}
}

/* Returns default viewport for some window geometry. If the window spans
   more than one viewport the most appropriate viewport is returned. How the
   most appropriate viewport is computed can be made optional if necessary. It
   is currently computed as the viewport where the center of the window is
   located, except for when the window is visible in the current viewport as
   the current viewport is then always returned. */
void
viewportForGeometry (CompScreen *s,
					 int	x,
					 int	y,
					 int	width,
					 int	height,
					 int	borderWidth,
					 int	*viewportX,
					 int	*viewportY)
{
	int	centerX;
	int	centerY;

	width  += borderWidth * 2;
	height += borderWidth * 2;

	if ((x < s->width  && x + width	 > 0) &&
		(y < s->height && y + height > 0))
	{
		if (viewportX)
			*viewportX = s->x;

		if (viewportY)
			*viewportY = s->y;

		return;
	}

	if (viewportX)
	{
		centerX = x + (width >> 1);
		if (centerX < 0)
			*viewportX = s->x + ((centerX / s->width) - 1) % s->hsize;
		else
			*viewportX = s->x + (centerX / s->width) % s->hsize;
	}

	if (viewportY)
	{
		centerY = y + (height >> 1);
		if (centerY < 0)
			*viewportY = s->y + ((centerY / s->height) - 1) % s->vsize;
		else
			*viewportY = s->y + (centerY / s->height) % s->vsize;
	}
}

int
outputDeviceForGeometry (CompScreen *s,
						 int		x,
						 int		y,
						 int		width,
						 int		height,
						 int		borderWidth)
{
	int output = s->currentOutputDev;
	int x1, y1, x2, y2;

	width  += borderWidth * 2;
	height += borderWidth * 2;

	x1 = s->outputDev[output].region.extents.x1;
	y1 = s->outputDev[output].region.extents.y1;
	x2 = s->outputDev[output].region.extents.x2;
	y2 = s->outputDev[output].region.extents.y2;

	if (x1 >= x + width	 ||
		y1 >= y + height ||
		x2 <= x		 ||
		y2 <= y)
	{
		output = outputDeviceForPoint (s, x + width	 / 2, y + height / 2);
	}

	return output;
}

Bool
updateDefaultIcon (CompScreen *screen)
{
	CompIcon *icon;
	char	 *file = screen->opt[COMP_SCREEN_OPTION_DEFAULT_ICON].value.s;
	void	 *data;
	int		 width, height;

	if (screen->defaultIcon)
	{
		finiTexture (screen, &screen->defaultIcon->texture);
		free (screen->defaultIcon);
		screen->defaultIcon = NULL;
	}

	if (!readImageFromFile (screen->display, file, &width, &height, &data))
		return FALSE;

	icon = malloc (sizeof (CompIcon) + width * height * sizeof (CARD32));
	if (!icon)
	{
		free (data);
		return FALSE;
	}

	initTexture (screen, &icon->texture);

	icon->width	 = width;
	icon->height = height;

	memcpy (icon + 1, data, + width * height * sizeof (CARD32));

	screen->defaultIcon = icon;

	free (data);

	return TRUE;
}

CompCursor *
findCursorAtScreen (CompScreen *screen)
{
	return screen->cursors;
}

CompCursorImage *
findCursorImageAtScreen (CompScreen	   *screen,
						 unsigned long serial)
{
	CompCursorImage *image;

	for (image = screen->cursorImages; image; image = image->next)
		if (image->serial == serial)
			return image;

	return NULL;
}

void
setCurrentActiveWindowHistory (CompScreen *s,
							   int	  x,
							   int	  y)
{
	int	i, min = 0;

	for (i = 0; i < ACTIVE_WINDOW_HISTORY_NUM; i++)
	{
		if (s->history[i].x == x && s->history[i].y == y)
		{
			s->currentHistory = i;
			return;
		}
	}

	for (i = 1; i < ACTIVE_WINDOW_HISTORY_NUM; i++)
		if (s->history[i].activeNum < s->history[min].activeNum)
			min = i;

	s->currentHistory = min;

	s->history[min].activeNum = s->activeNum;
	s->history[min].x		  = x;
	s->history[min].y		  = y;

	memset (s->history[min].id, 0, sizeof (s->history[min].id));
}

void
addToCurrentActiveWindowHistory (CompScreen *s,
								 Window		id)
{
	CompActiveWindowHistory *history = &s->history[s->currentHistory];
	Window			tmp, next = id;
	int				i;

	/* walk and move history */
	for (i = 0; i < ACTIVE_WINDOW_HISTORY_SIZE; i++)
	{
		tmp = history->id[i];
		history->id[i] = next;
		next = tmp;

		/* we're done when we find an old instance or an empty slot */
		if (tmp == id || tmp == None)
			break;
	}

	history->activeNum = s->activeNum;
}
