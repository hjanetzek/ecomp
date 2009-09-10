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

#define A(x) do { printf(__FILE__ ":%d:\t", __LINE__); printf x; fflush(stdout); } while(0)
#define D(x)
#define B(x)
#define C(x)
#define E(x)


#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xproto.h>
#include <X11/extensions/shape.h>
#include <X11/extensions/Xcomposite.h>

#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <stdint.h>

#include <ecomp.h>

/* #define MwmHintsFunctions	(1L << 0)
 * #define MwmHintsDecorations (1L << 1)
 * 
 * #define PropMotifWmHintElements 3
 * 
 * typedef struct {
 * 	unsigned long flags;
 * 	unsigned long functions;
 * 	unsigned long decorations;
 * } MwmHints; */

static int
reallocWindowPrivates (int	size, void *closure)
{
	CompScreen *s = (CompScreen *) closure;
	CompWindow *w;
	void *privates;

	for (w = s->windows; w; w = w->next)
	{
		privates = realloc (w->privates, size * sizeof (CompPrivate));
		if (!privates)
			return FALSE;

		w->privates = (CompPrivate *) privates;
	}

	return TRUE;
}

int
allocateWindowPrivateIndex (CompScreen *screen)
{
	return allocatePrivateIndex (&screen->windowPrivateLen,
								 &screen->windowPrivateIndices,
								 reallocWindowPrivates,
								 (void *) screen);
}

void
freeWindowPrivateIndex (CompScreen *screen, int index)
{
	freePrivateIndex (screen->windowPrivateLen,
					  screen->windowPrivateIndices,
					  index);
}

void
updateWindowClassHints (CompWindow *w)
{
	XClassHint classHint;
	int status;

	if(w->clientId)
		status = XGetClassHint (w->screen->display->display, w->clientId, &classHint);
	else
		status = XGetClassHint (w->screen->display->display, w->id, &classHint);

    if (w->resName)
    {
		free (w->resName);
		w->resName = NULL;
    }

    if (w->resClass)
    {
		free (w->resClass);
		w->resClass = NULL;
    }

    status = XGetClassHint (w->screen->display->display, w->id, &classHint);
    if (status)
    {
		if (classHint.res_name)
		{
			w->resName = strdup (classHint.res_name);
			XFree (classHint.res_name);
		}

		if (classHint.res_class)
		{
			w->resClass = strdup (classHint.res_class);
			XFree (classHint.res_class);
		}
    }
}


int
getWmState (CompDisplay *display, Window id)
{
	Atom actual;
	int result, format;
	unsigned long n, left;
	unsigned char *data;
	unsigned long state = NormalState;

	result = XGetWindowProperty
		(display->display, id, display->wmStateAtom, 0L, 2L, FALSE,
		 display->wmStateAtom, &actual, &format, &n, &left, &data);

	if (result == Success && n && data)
	{
		memcpy (&state, data, sizeof (unsigned long));
		XFree ((void *) data);
	}

	return state;
}


unsigned int
windowStateMask (CompDisplay *display, Atom state)
{
	if (state == display->winStateModalAtom)
		return CompWindowStateModalMask;
	else if (state == display->winStateStickyAtom)
		return CompWindowStateStickyMask;
	else if (state == display->winStateMaximizedVertAtom)
		return CompWindowStateMaximizedVertMask;
	else if (state == display->winStateMaximizedHorzAtom)
		return CompWindowStateMaximizedHorzMask;
	else if (state == display->winStateShadedAtom)
		return CompWindowStateShadedMask;
	else if (state == display->winStateSkipTaskbarAtom)
		return CompWindowStateSkipTaskbarMask;
	else if (state == display->winStateSkipPagerAtom)
		return CompWindowStateSkipPagerMask;
	else if (state == display->winStateHiddenAtom)
		return CompWindowStateHiddenMask;
	else if (state == display->winStateFullscreenAtom)
		return CompWindowStateFullscreenMask;
	else if (state == display->winStateAboveAtom)
		return CompWindowStateAboveMask;
	else if (state == display->winStateBelowAtom)
		return CompWindowStateBelowMask;
	else if (state == display->winStateDemandsAttentionAtom)
		return CompWindowStateDemandsAttentionMask;
	else if (state == display->winStateDisplayModalAtom)
		return CompWindowStateDisplayModalMask;

	return 0;
}

unsigned int
windowStateFromString (const char *str)
{
	if (strcasecmp (str, "modal") == 0)
		return CompWindowStateModalMask;
	else if (strcasecmp (str, "sticky") == 0)
		return CompWindowStateStickyMask;
	else if (strcasecmp (str, "maxvert") == 0)
		return CompWindowStateMaximizedVertMask;
	else if (strcasecmp (str, "maxhorz") == 0)
		return CompWindowStateMaximizedHorzMask;
	else if (strcasecmp (str, "shaded") == 0)
		return CompWindowStateShadedMask;
	else if (strcasecmp (str, "skiptaskbar") == 0)
		return CompWindowStateSkipTaskbarMask;
	else if (strcasecmp (str, "skippager") == 0)
		return CompWindowStateSkipPagerMask;
	else if (strcasecmp (str, "hidden") == 0)
		return CompWindowStateHiddenMask;
	else if (strcasecmp (str, "fullscreen") == 0)
		return CompWindowStateFullscreenMask;
	else if (strcasecmp (str, "above") == 0)
		return CompWindowStateAboveMask;
	else if (strcasecmp (str, "below") == 0)
		return CompWindowStateBelowMask;
	else if (strcasecmp (str, "demandsattention") == 0)
		return CompWindowStateDemandsAttentionMask;

	return 0;
}

/*TODO this can be send from e */
unsigned int
getWindowState (CompDisplay *display, Window id)
{
	Atom actual;
	int result, format;
	unsigned long n, left;
	unsigned char *data;
	unsigned int state = 0;

	result = XGetWindowProperty
		(display->display, id, display->winStateAtom, 0L, 1024L,
		 FALSE, XA_ATOM, &actual, &format, &n, &left, &data);

	if (result == Success && n && data)
	{
		Atom *a = (Atom *) data;

		while (n--)
			state |= windowStateMask (display, *a++);

		XFree ((void *) data);
	}

	return state;
}


/* TODO remove this*/
void
changeWindowState (CompWindow *w, unsigned int newState)
{
	CompDisplay	 *d = w->screen->display;
	unsigned int oldState = w->state;

	w->state = newState;

	//	  setWindowState (d, w->state, w->id);

	(*w->screen->windowStateChangeNotify) (w, oldState);

	(*d->matchPropertyChanged) (d, w);
}


/* FIXME where is this used ? */
/* void
 * getAllowedActionsForWindow (CompWindow *w, unsigned int *setActions, unsigned int *clearActions)
 * {
 * 	*setActions	 = 0;
 * 	*clearActions = 0;
 * } */

unsigned int
windowTypeFromString (const char *str)
{
	if (strcasecmp (str, "desktop") == 0) // 0
		return CompWindowTypeDesktopMask;
	else if (strcasecmp (str, "dock") == 0) // 2
		return CompWindowTypeDockMask;
	else if (strcasecmp (str, "toolbar") == 0) // 4
		return CompWindowTypeToolbarMask;
	else if (strcasecmp (str, "menu") == 0) // 8
		return CompWindowTypeMenuMask;
	else if (strcasecmp (str, "utility") == 0) // 16
		return CompWindowTypeUtilMask;
	else if (strcasecmp (str, "splash") == 0) // 32
		return CompWindowTypeSplashMask;
	else if (strcasecmp (str, "dialog") == 0) // 64
		return CompWindowTypeDialogMask;
	else if (strcasecmp (str, "normal") == 0) //128
		return CompWindowTypeNormalMask;
	else if (strcasecmp (str, "dropdownmenu") == 0)
		return CompWindowTypeDropdownMenuMask;
	else if (strcasecmp (str, "popupmenu") == 0)
		return CompWindowTypePopupMenuMask;
	else if (strcasecmp (str, "tooltip") == 0)
		return CompWindowTypeTooltipMask;
	else if (strcasecmp (str, "notification") == 0)
		return CompWindowTypeNotificationMask;
	else if (strcasecmp (str, "combo") == 0)
		return CompWindowTypeComboMask;
	else if (strcasecmp (str, "dnd") == 0)
		return CompWindowTypeDndMask;
	else if (strcasecmp (str, "modaldialog") == 0)
		return CompWindowTypeModalDialogMask;
	else if (strcasecmp (str, "fullscreen") == 0)
		return CompWindowTypeFullscreenMask;
	else if (strcasecmp (str, "unknown") == 0)
		return CompWindowTypeUnknownMask;
	else if (strcasecmp (str, "any") == 0)
		return ~0;

	return 0;
}

/* TODO send this from e  */
unsigned int
getWindowType (CompDisplay *display, Window id)
{
	Atom actual;
	int result, format;
	unsigned long n, left;
	unsigned char *data;

	result = XGetWindowProperty
		(display->display, id, display->winTypeAtom, 0L, 1L,
		 FALSE, XA_ATOM, &actual, &format, &n, &left, &data);

	if (result == Success && n && data)
	{
		Atom a;

		memcpy (&a, data, sizeof (Atom));
		XFree ((void *) data);

		if (a == display->winTypeNormalAtom)
			return CompWindowTypeNormalMask;
		else if (a == display->winTypeMenuAtom)
			return CompWindowTypeMenuMask;
		else if (a == display->winTypeDesktopAtom)
			return CompWindowTypeDesktopMask;
		else if (a == display->winTypeDockAtom)
			return CompWindowTypeDockMask;
		else if (a == display->winTypeToolbarAtom)
			return CompWindowTypeToolbarMask;
		else if (a == display->winTypeUtilAtom)
			return CompWindowTypeUtilMask;
		else if (a == display->winTypeSplashAtom)
			return CompWindowTypeSplashMask;
		else if (a == display->winTypeDialogAtom)
			return CompWindowTypeDialogMask;
		else if (a == display->winTypeDropdownMenuAtom)
			return CompWindowTypeDropdownMenuMask;
		else if (a == display->winTypePopupMenuAtom)
			return CompWindowTypePopupMenuMask;
		else if (a == display->winTypeTooltipAtom)
			return CompWindowTypeTooltipMask;
		else if (a == display->winTypeNotificationAtom)
			return CompWindowTypeNotificationMask;
		else if (a == display->winTypeComboAtom)
			return CompWindowTypeComboMask;
		else if (a == display->winTypeDndAtom)
			return CompWindowTypeDndMask;
	}

	return CompWindowTypeUnknownMask;
}

void
recalcWindowType (CompWindow *w)
{
	unsigned int type;

	type = w->wmType;

	/* XXX workarounds */
	if (w->resName)
	{
		if (!w->clientId && !(w->wmType & CompWindowTypeDropdownMenuMask))
		{
			if ((!strcmp (w->resName, "gecko")) ||
				(!strcmp (w->resName, "Popup")) ||
				(!strcmp (w->resName, "VCLSalFrame")))
			{
				type = CompWindowTypeDropdownMenuMask;
			}
		}
		else if ((!strcmp (w->resName, "sun-awt-X11-XMenuWindow")) ||
				 (!strcmp (w->resName, "sun-awt-X11-XWindowPeer")))
		{
			type = CompWindowTypeDropdownMenuMask;
		}
		else if (!strcmp (w->resName, "sun-awt-X11-XDialogPeer"))
		{
			type = CompWindowTypeDialogMask;
		}
		else if (!strcmp (w->resName, "sun-awt-X11-XFramePeer"))
		{
			type = CompWindowTypeNormalMask;
		}
	}

	if (w->clientId && (w->wmType == CompWindowTypeUnknownMask))
	{
		type = CompWindowTypeNormalMask;
	}
	else if (!w->clientId && (w->wmType == CompWindowTypeNormalMask))
	{
		type = CompWindowTypeUnknownMask;
	}

	if (w->state & CompWindowStateFullscreenMask)
		type = CompWindowTypeFullscreenMask;

	w->type = type;
	/* w->wmType = type; */
}

/* void
 * getMwmHints (CompDisplay *display,
 * 			 Window	id,
 * 			 unsigned int *func,
 * 			 unsigned int *decor)
 * {
 * 	Atom actual;
 * 	int result, format;
 * 	unsigned long n, left;
 * 	unsigned char *data;
 * 
 * 	*func  = MwmFuncAll;
 * 	*decor = MwmDecorAll;
 * 
 * 	result = XGetWindowProperty
 * 		(display->display, id, display->mwmHintsAtom,
 * 		 0L, 20L, FALSE, display->mwmHintsAtom,
 * 		 &actual, &format, &n, &left, &data);
 * 
 * 	if (result == Success && n && data)
 * 	{
 * 		MwmHints *mwmHints = (MwmHints *) data;
 * 
 * 		if (n >= PropMotifWmHintElements)
 * 		{
 * 			if (mwmHints->flags & MwmHintsDecorations)
 * 				*decor = mwmHints->decorations;
 * 
 * 			if (mwmHints->flags & MwmHintsFunctions)
 * 				*func = mwmHints->functions;
 * 		}
 * 
 * 		XFree (data);
 * 	}
 * } */


unsigned int
getWindowProp (CompDisplay *display,
			   Window id,
			   Atom property,
			   unsigned int defaultValue)
{
	Atom actual;
	int result, format;
	unsigned long n, left;
	unsigned char *data;

	result = XGetWindowProperty
		(display->display, id, property, 0L, 1L, FALSE,
		 XA_CARDINAL, &actual, &format, &n, &left, &data);

	if (result == Success && n && data)
	{
		unsigned long value;

		memcpy (&value, data, sizeof (unsigned long));

		XFree (data);

		return (unsigned int) value;
	}

	return defaultValue;
}

void
setWindowProp (CompDisplay	*display,
			   Window id,
			   Atom property,
			   unsigned int value)
{
	unsigned long data = value;

	XChangeProperty (display->display, id, property,
					 XA_CARDINAL, 32, PropModeReplace,
					 (unsigned char *) &data, 1);
}

Bool
readWindowProp32 (CompDisplay *display,
				  Window id,
				  Atom property,
				  unsigned short *returnValue)
{
	Atom actual;
	int result, format;
	unsigned long n, left;
	unsigned char *data;

	result = XGetWindowProperty
		(display->display, id, property, 0L, 1L, FALSE,
		 XA_CARDINAL, &actual, &format, &n, &left, &data);

	if (result == Success && n && data)
	{
		CARD32 value;

		memcpy (&value, data, sizeof (CARD32));

		XFree (data);

		*returnValue = value >> 16;

		return TRUE;
	}

	return FALSE;
}

unsigned short
getWindowProp32 (CompDisplay *display,
				 Window id,
				 Atom property,
				 unsigned short defaultValue)
{
	unsigned short result;

	if (readWindowProp32 (display, id, property, &result))
		return result;

	return defaultValue;
}

void
setWindowProp32 (CompDisplay *display,
				 Window id,
				 Atom property,
				 unsigned short value)
{
	CARD32 value32;

	value32 = value << 16 | value;

	XChangeProperty (display->display, id, property,
					 XA_CARDINAL, 32, PropModeReplace,
					 (unsigned char *) &value32, 1);
}

void
updateWindowOpacity (CompWindow *w)
{
	CompScreen *s = w->screen;
	int opacity = w->opacity;

	if (!w->opacityPropSet && !(w->type & CompWindowTypeDesktopMask))
	{
		CompOption *matches = &s->opt[COMP_SCREEN_OPTION_OPACITY_MATCHES];
		CompOption *values = &s->opt[COMP_SCREEN_OPTION_OPACITY_VALUES];
		int	   i, min;

		min = MIN (matches->value.list.nValue, values->value.list.nValue);

		for (i = 0; i < min; i++)
		{
			if (matchEval (&matches->value.list.value[i].match, w))
			{
				opacity = (values->value.list.value[i].i * OPAQUE) / 100;
				break;
			}
		}
	}

	opacity = (opacity * w->opacityFactor) / 0xff;
	if (opacity != w->paint.opacity)
	{
		w->paint.opacity = opacity;
		addWindowDamage (w);
	}
}


void
updateWindowOutputExtents (CompWindow *w)
{
	CompWindowExtents output;

	(*w->screen->getOutputExtentsForWindow) (w, &output);

	if (output.left	  != w->output.left	 ||
		output.right  != w->output.right ||
		output.top	  != w->output.top	 ||
		output.bottom != w->output.bottom)
	{
		w->output = output;

		(*w->screen->windowResizeNotify) (w, 0, 0, 0, 0);
	}
}

static void
setWindowMatrix (CompWindow *w)
{
	w->matrix = w->texture->matrix;
	w->matrix.x0 -= (w->attrib.x * w->matrix.xx);
	w->matrix.y0 -= (w->attrib.y * w->matrix.yy);
}

Bool
bindWindow (CompWindow *w)
{
	redirectWindow (w);

	if (!w->pixmap)
	{
		XWindowAttributes attr;

		/* don't try to bind window again if it failed previously */
		if (w->bindFailed)
			return FALSE;

		/* We have to grab the server here to make sure that window
		   is mapped when getting the window pixmap */
		XGrabServer (w->screen->display->display);
		XGetWindowAttributes (w->screen->display->display, w->id, &attr);

		if (attr.map_state != IsViewable)
		{
			XUngrabServer (w->screen->display->display);
			finiTexture (w->screen, w->texture);
			w->bindFailed = TRUE;
			return FALSE;
		}

		w->pixmap = XCompositeNameWindowPixmap (w->screen->display->display, w->id);

		XUngrabServer (w->screen->display->display);
	}

	if (!bindPixmapToTexture (w->screen, w->texture, w->pixmap,
							  w->width, w->height,
							  w->attrib.depth))
	{
		compLogMessage (w->screen->display, "core", CompLogLevelInfo,
						"Couldn't bind redirected window 0x%x to "
						"texture\n", (int) w->id);
	}

	setWindowMatrix (w);

	return TRUE;
}

void
releaseWindow (CompWindow *w)
{
	if (w->pixmap)
	{
		CompTexture *texture;

		texture = createTexture (w->screen);
		if (texture)
		{
			destroyTexture (w->screen, w->texture);

			w->texture = texture;
		}

		XFreePixmap (w->screen->display->display, w->pixmap);

		w->pixmap = None;
	}
}

static void
freeWindow (CompWindow *w)
{
	releaseWindow (w);

	destroyTexture (w->screen, w->texture);

	if (w->clip)
		XDestroyRegion (w->clip);

	if (w->region)
		XDestroyRegion (w->region);

	if (w->privates)
		free (w->privates);

	if (w->sizeDamage)
		free (w->damageRects);

	if (w->vertices)
		free (w->vertices);

	if (w->indices)
		free (w->indices);

	if (w->struts)
		free (w->struts);

	if (w->icon)
		freeWindowIcons (w);

	if (w->resName)
	{
		free (w->resName);
	}

	if (w->resClass)
		free (w->resClass);

	free (w);
}

void
damageTransformedWindowRect (CompWindow *w,
							 float xScale,
							 float yScale,
							 float xTranslate,
							 float yTranslate,
							 BoxPtr rect)
{
	REGION reg;

	reg.rects	 = &reg.extents;
	reg.numRects = 1;

	reg.extents.x1 = (rect->x1 * xScale) - 1;
	reg.extents.y1 = (rect->y1 * yScale) - 1;
	reg.extents.x2 = (rect->x2 * xScale + 0.5f) + 1;
	reg.extents.y2 = (rect->y2 * yScale + 0.5f) + 1;

	reg.extents.x1 += xTranslate;
	reg.extents.y1 += yTranslate;
	reg.extents.x2 += (xTranslate + 0.5f);
	reg.extents.y2 += (yTranslate + 0.5f);

	if (reg.extents.x2 > reg.extents.x1 && reg.extents.y2 > reg.extents.y1)
	{
		reg.extents.x1 += w->attrib.x + w->attrib.border_width;
		reg.extents.y1 += w->attrib.y + w->attrib.border_width;
		reg.extents.x2 += w->attrib.x + w->attrib.border_width;
		reg.extents.y2 += w->attrib.y + w->attrib.border_width;

		damageScreenRegion (w->screen, &reg);
	}
}

void
damageWindowOutputExtents (CompWindow *w)
{
	if (w->screen->damageMask & COMP_SCREEN_DAMAGE_ALL_MASK)
		return;

	if (w->shaded || (w->attrib.map_state == IsViewable && w->damaged))
	{
		BoxRec box;

		/* top */
		box.x1 = -w->output.left;
		box.y1 = -w->output.top;
		box.x2 = w->width + w->output.right;
		box.y2 = 0;

		if (box.x1 < box.x2 && box.y1 < box.y2)
			addWindowDamageRect (w, &box);

		/* bottom */
		box.y1 = w->height;
		box.y2 = box.y1 + w->output.bottom;

		if (box.x1 < box.x2 && box.y1 < box.y2)
			addWindowDamageRect (w, &box);

		/* left */
		box.x1 = -w->output.left;
		box.y1 = 0;
		box.x2 = 0;
		box.y2 = w->height;

		if (box.x1 < box.x2 && box.y1 < box.y2)
			addWindowDamageRect (w, &box);

		/* right */
		box.x1 = w->width;
		box.x2 = box.x1 + w->output.right;

		if (box.x1 < box.x2 && box.y1 < box.y2)
			addWindowDamageRect (w, &box);
	}
}

Bool
damageWindowRect (CompWindow *w, Bool initial, BoxPtr rect)
{
	return FALSE;
}

void
addWindowDamageRect (CompWindow *w, BoxPtr rect)
{
	REGION region;

	if (w->screen->damageMask & COMP_SCREEN_DAMAGE_ALL_MASK)
		return;

	region.extents = *rect;

	if (!(*w->screen->damageWindowRect) (w, FALSE, &region.extents))
	{
		region.extents.x1 += w->attrib.x + w->attrib.border_width;
		region.extents.y1 += w->attrib.y + w->attrib.border_width;
		region.extents.x2 += w->attrib.x + w->attrib.border_width;
		region.extents.y2 += w->attrib.y + w->attrib.border_width;

		region.rects = &region.extents;
		region.numRects = region.size = 1;

		damageScreenRegion (w->screen, &region);
	}
}

void
getOutputExtentsForWindow (CompWindow *w, CompWindowExtents *output)
{
	output->left   = 0;
	output->right  = 0;
	output->top	   = 0;
	output->bottom = 0;
}

void
addWindowDamage (CompWindow *w)
{
	if (w->screen->damageMask & COMP_SCREEN_DAMAGE_ALL_MASK)
		return;

	if (w->shaded || (w->attrib.map_state == IsViewable && w->damaged))
	{
		BoxRec box;

		box.x1 = -w->output.left - w->attrib.border_width;
		box.y1 = -w->output.top - w->attrib.border_width;
		box.x2 = w->width + w->output.right;
		box.y2 = w->height + w->output.bottom;

		addWindowDamageRect (w, &box);
	}
}

void
updateWindowRegion (CompWindow *w)
{
	REGION rect;
	XRectangle r, *rects, *shapeRects = 0;
	int i, n = 0;

	EMPTY_REGION (w->region);

	if (w->screen->display->shapeExtension)
	{
		int order;

		shapeRects = XShapeGetRectangles (w->screen->display->display, w->id,
										  ShapeBounding, &n, &order);
	}

	if (n < 2)
	{
		r.x		 = -w->attrib.border_width;
		r.y		 = -w->attrib.border_width;
		r.width	 = w->width;
		r.height = w->height;

		rects = &r;
		n = 1;
	}
	else
	{
		rects = shapeRects;
	}

	rect.rects = &rect.extents;
	rect.numRects = rect.size = 1;

	for (i = 0; i < n; i++)
	{
		rect.extents.x1 = rects[i].x + w->attrib.border_width;
		rect.extents.y1 = rects[i].y + w->attrib.border_width;
		rect.extents.x2 = rect.extents.x1 + rects[i].width;
		rect.extents.y2 = rect.extents.y1 + rects[i].height;

		if (rect.extents.x1 < 0)
			rect.extents.x1 = 0;
		if (rect.extents.y1 < 0)
			rect.extents.y1 = 0;
		if (rect.extents.x2 > w->width)
			rect.extents.x2 = w->width;
		if (rect.extents.y2 > w->height)
			rect.extents.y2 = w->height;

		if (rect.extents.y1 < rect.extents.y2 &&
			rect.extents.x1 < rect.extents.x2)
		{
			rect.extents.x1 += w->attrib.x;
			rect.extents.y1 += w->attrib.y;
			rect.extents.x2 += w->attrib.x;
			rect.extents.y2 += w->attrib.y;

			XUnionRegion (&rect, w->region, w->region);
		}
	}

	if (shapeRects)
		XFree (shapeRects);
}

/* static void
 * setDefaultWindowAttributes (XWindowAttributes *wa)
 * {
 * 	wa->x			  = 0;
 * 	wa->y			  = 0;
 * 	wa->width			  = 1;
 * 	wa->height			  = 1;
 * 	wa->border_width		  = 0;
 * 	wa->depth			  = 0;
 * 	wa->visual			  = NULL;
 * 	wa->root			  = None;
 * 	wa->class			  = InputOnly;
 * 	wa->bit_gravity		  = NorthWestGravity;
 * 	wa->win_gravity		  = NorthWestGravity;
 * 	wa->backing_store		  = NotUseful;
 * 	wa->backing_planes		  = 0;
 * 	wa->backing_pixel		  = 0;
 * 	wa->save_under		  = FALSE;
 * 	wa->colormap		  = None;
 * 	wa->map_installed		  = FALSE;
 * 	wa->map_state		  = IsUnviewable;
 * 	wa->all_event_masks		  = 0;
 * 	wa->your_event_mask		  = 0;
 * 	wa->do_not_propagate_mask = 0;
 * 	wa->override_redirect	  = TRUE;
 * 	wa->screen			  = NULL;
 * } */

void
updateWindowViewport(CompWindow *w, int initial)
{
	int desk = 0;
	CompScreen *s = w->screen;
	CompDisplay *d = s->display;
	Atom actual;
	int result, format;
	unsigned long n, left;
	unsigned char *data;

	result = XGetWindowProperty
		(d->display, w->id, d->winDesktopAtom, 0L, 1L, FALSE,
		 XA_CARDINAL, &actual, &format, &n, &left, &data);

	if (result == Success && n == 1 && data)
	{
		unsigned long *_desk = (unsigned long *) data;
		desk = _desk[0];

		XFree (data);

		int dy = (int)desk / s->hsize;
		int dx = (int)desk - (dy * s->hsize);

		w->initialViewportX = dx;
		w->initialViewportY = dy;

		int x = MOD(w->attrib.x, s->width)  + ((dx - s->x) * s->width);
		int y = MOD(w->attrib.y, s->height) + ((dy - s->y) * s->height);

		w->serverX		     = x;
		w->serverY		     = y;
		w->attrib.x		     = x;
		w->attrib.y		     = y;
	}
}



void
addWindow (CompScreen *screen, Window id, Window aboveId)
{
	B(("0x%x :addWindow\n", (unsigned int) id));
	CompWindow *w;
	CompDisplay *d = screen->display;
	
	w = (CompWindow *) malloc (sizeof (CompWindow));
	if (!w) return;

	w->next = NULL;
	w->prev = NULL;

	w->mapNum	 = 0;
	w->activeNum = 0;

	w->frame = None;

	/*TODO remove */
	w->placed		 = TRUE;
	
	w->minimized	 = FALSE;
	w->inShowDesktopMode = FALSE;
	w->shaded		 = FALSE;
	w->hidden		 = FALSE;
	w->grabbed		 = FALSE;
	w->added		 = FALSE;

	w->desktop = screen->currentDesktop;

	/* XXX rename to viewportX/Y */
	w->initialViewportX = screen->x;
	w->initialViewportY = screen->y;

	/* w->initialTimestamp	   = 0;
	 * w->initialTimestampSet = FALSE; */

	w->pendingUnmaps = 0;
	w->pendingMaps	 = 0;

	w->resName	 = NULL;
	w->resClass	 = NULL;

	w->texture = createTexture (screen);
	if (!w->texture)
	{
		free (w);
		return;
	}

	w->screen	  = screen;
	w->pixmap	  = None;
	w->destroyed  = FALSE;
	w->damaged	  = FALSE;
	w->redirected = TRUE;

	w->bindFailed = FALSE;

	w->destroyRefCnt = 1;
	w->unmapRefCnt	 = 1;

	w->damageRects = 0;
	w->sizeDamage  = 0;
	w->nDamage	   = 0;

	w->vertices		= 0;
	w->vertexSize	= 0;
	w->indices		= 0;
	w->indexSize	= 0;
	w->vCount		= 0;
	w->indexCount	= 0;
	w->texCoordSize = 2;

	w->drawWindowGeometry = NULL;

	w->struts = 0;

	w->icon	 = 0;
	w->nIcon = 0;

	w->input.left	= 0;
	w->input.right	= 0;
	w->input.top	= 0;
	w->input.bottom = 0;

	w->output.left	 = 0;
	w->output.right	 = 0;
	w->output.top	 = 0;
	w->output.bottom = 0;

	w->paint.opacity	= w->opacity	= OPAQUE;
	w->paint.brightness = w->brightness = 0xffff;
	w->paint.saturation = w->saturation = COLOR;
	w->paint.xScale	= 1.0f;
	w->paint.yScale	= 1.0f;
	w->paint.xTranslate	= 0.0f;
	w->paint.yTranslate	= 0.0f;

	w->opacityFactor = 0xff;
	w->opacityPropSet = FALSE;

	w->lastPaint = w->paint;
	w->alive = TRUE;

	if (screen->windowPrivateLen)
	{
		w->privates = malloc (screen->windowPrivateLen * sizeof (CompPrivate));
		if (!w->privates)
		{
			destroyTexture (screen, w->texture);
			free (w);
			return;
		}
	}
	else
		w->privates = 0;

	w->region = XCreateRegion ();
	if (!w->region)
	{
		freeWindow (w);
		return;
	}

	w->clip = XCreateRegion ();
	if (!w->clip)
	{
		freeWindow (w);
		return;
	}

	/* Failure means that window has been destroyed. We still have to add the
	   window to the window list as we might get configure requests which
	   require us to stack other windows relative to it. Setting some default
	   values if this is the case. */
	if (!XGetWindowAttributes (d->display, id, &w->attrib))
		/* || (w->attrib.class == InputOnly)) */
	{
		freeWindow (w);
		return;
		/* setDefaultWindowAttributes (&w->attrib); */
	}

	w->serverWidth	 = w->attrib.width;
	w->serverHeight	 = w->attrib.height;
	w->serverBorderWidth = w->attrib.border_width;

	w->width  = w->attrib.width	 + w->attrib.border_width * 2;
	w->height = w->attrib.height + w->attrib.border_width * 2;

	w->sizeHints.flags = 0;

	w->clientMapped = FALSE;
	w->clientId		= None;

	w->serverX = w->attrib.x;
	w->serverY = w->attrib.y;

	/* w->syncWait		   = FALSE;
	 * w->syncX		   = w->attrib.x;
	 * w->syncY		   = w->attrib.y;
	 * w->syncWidth	   = w->attrib.width;
	 * w->syncHeight	   = w->attrib.height;
	 * w->syncBorderWidth = w->attrib.border_width; */

	w->saveMask = 0;

	XSelectInput (d->display, id, PropertyChangeMask);
		/* |EnterWindowMask	 |
		 * FocusChangeMask); */

	w->id = id;

	w->alpha	 = (w->attrib.depth == 32);
	w->wmType	 = 0;
	w->state	 = 0;
	/* w->actions	 = 0; */
	/* w->protocols = 0; */
	w->type		 = CompWindowTypeUnknownMask;

	if (d->shapeExtension)
		XShapeSelectInput (d->display, id, ShapeNotifyMask);

	insertWindowIntoScreen (screen, w, aboveId);

	EMPTY_REGION (w->region);

	if (w->attrib.class == InputOnly)	
	{
		w->invisible = TRUE;
		w->damage = None;
		w->attrib.map_state = IsUnmapped;
		return;
	}

	w->invisible = TRUE;

	/* check if this is an enlightenment frame window */
	unsigned int clientId = 0;
	{
		/* TODO use windowProperty32 instead! */
		unsigned char	   *prop_ret = NULL;
		Atom				type_ret;
		unsigned long		bytes_after, num_ret;
		int					format_ret;

		XGetWindowProperty(d->display, w->id,
						   d->eManagedAtom,
						   0, 0x7fffffff, False,
						   XA_CARDINAL, &type_ret, &format_ret,
						   &num_ret, &bytes_after, &prop_ret);

		if (prop_ret && type_ret == XA_CARDINAL && format_ret == 32 && num_ret == 1)
		{
			clientId = (((unsigned long*)prop_ret)[0]);
		}
		if (prop_ret)
			XFree(prop_ret);
	}

	if(clientId)
	{
		printf("window managed: 0x%x : 0x%x\n", (unsigned int)w->id, clientId);

		XSelectInput (d->display, w->id,
					  SubstructureNotifyMask |
					  PropertyChangeMask);
		
		w->attrib.override_redirect = FALSE;
		w->clientId = clientId;

		w->wmType = getWindowType (d, w->clientId);
		if (w->wmType == CompWindowTypeUnknownMask)
			w->wmType = CompWindowTypeNormalMask;
		w->state = getWindowState (d, w->clientId);
		updateWindowClassHints (w);
		if(w->resClass) printf ("- %s\n", w->resClass);

		recalcWindowType (w);

		updateWindowViewport(w, 1);

		w->opacityPropSet = readWindowProp32
			(d, w->id, d->winOpacityAtom, &w->opacity);

		w->brightness = getWindowProp32(d, w->id, d->winBrightnessAtom, BRIGHT);

		w->clientMapped = (w->attrib.map_state == IsViewable) ? 1 : 0;
		
		if (w->alive)
		{
			w->paint.opacity    = w->opacity;
			w->paint.brightness = w->brightness;
		}

		if (screen->canDoSaturated)
		{
			w->saturation = getWindowProp32 (d, w->id, d->winSaturationAtom, COLOR);
			if (w->alive)
				w->paint.saturation = w->saturation;
		}	
	}
	else
	{
		w->wmType = getWindowType (d, w->id);
		updateWindowClassHints (w);
		recalcWindowType (w);
	}

	{
		REGION rect;

		rect.rects = &rect.extents;
		rect.numRects = rect.size = 1;

		rect.extents.x1 = w->attrib.x;
		rect.extents.y1 = w->attrib.y;
		rect.extents.x2 = w->attrib.x + w->width;
		rect.extents.y2 = w->attrib.y + w->height;

		XUnionRegion (&rect, w->region, w->region);

		w->damage = XDamageCreate (d->display, id,
								   XDamageReportRawRectangles);

		/* need to check for DisplayModal state on all windows */
		//w->state = getWindowState (screen->display, w->id);
		//updateWindowClassHints (w);
	}

	
	/* XXX: think this whole section over again */
	if (w->attrib.map_state == IsViewable)
	{
		w->attrib.map_state = IsUnmapped;
		w->pendingMaps++;

		mapWindow (w);

		updateWindowAttributes (w, CompStackingUpdateModeNormal);
	}
	else if (w->clientId)
	{
		if (getWmState (d, w->id) == IconicState)
		{
			if (w->state & CompWindowStateHiddenMask)
			{
				if (w->state & CompWindowStateShadedMask)
					w->shaded = TRUE;
				else
					w->minimized = TRUE;
			}
		}
	}

	windowInitPlugins (w);

	(*w->screen->windowAddNotify) (w);

	updateWindowOpacity (w);
}

void
removeWindow (CompWindow *w)
{
	D(("0x%x : removeWindow\n", (unsigned int)w->id));

	unhookWindowFromScreen (w->screen, w);

	if (!w->destroyed)
	{
		CompDisplay *d = w->screen->display;

		if (w->damage)
			XDamageDestroy (d->display, w->damage);

		if (d->shapeExtension)
			XShapeSelectInput (d->display, w->id, NoEventMask);

		//	XSelectInput (d->display, w->id, NoEventMask);
		//	XUngrabButton (d->display, AnyButton, AnyModifier, w->id);
	}

	if (w->attrib.map_state == IsViewable && w->damaged)
	{
		if (w->type == CompWindowTypeDesktopMask)
			w->screen->desktopWindowCount--;
	}

	if (!w->redirected)
	{
		w->screen->overlayWindowCount--;

		if (w->screen->overlayWindowCount < 1)
			showOutputWindow (w->screen);
	}

	windowFiniPlugins (w);

	freeWindow (w);
}

void
destroyWindow (CompWindow *w)
{
	D(("0x%x : destroyWindow\n", (unsigned int)w->id));

	w->id = 1;
	w->mapNum = 0;

	w->destroyRefCnt--;
	if (w->destroyRefCnt)
		return;

	if (!w->destroyed)
	{
		w->destroyed = TRUE;
		w->screen->pendingDestroys++;
	}
}


void
mapWindow (CompWindow *w)
{
	if (w->attrib.map_state == IsViewable)
		return;

	w->pendingMaps--;

	w->mapNum = w->screen->mapNum++;

	if (w->attrib.class == InputOnly)
		return;

	w->unmapRefCnt = 1;

	w->attrib.map_state = IsViewable;

	w->invisible = TRUE;
	w->damaged = FALSE;
	w->alive = TRUE;
	w->bindFailed = FALSE;

	D(("0x%x : mapWindow\n", (unsigned int) w->id));
	updateWindowRegion (w);
	//updateWindowSize (w);

	if (w->type & CompWindowTypeDesktopMask)
		w->screen->desktopWindowCount++;
}

void
unmapWindow (CompWindow *w)
{
	B(("- unmapWindow\n"));

	if (w->mapNum)
	{
		w->mapNum = 0;
	}

	w->unmapRefCnt--;
	if (w->unmapRefCnt > 0)
		return;

	if (w->attrib.map_state != IsViewable)
		return;

	if (w->type == CompWindowTypeDesktopMask)
		w->screen->desktopWindowCount--;

	addWindowDamage (w);

	w->attrib.map_state = IsUnmapped;

	w->invisible = TRUE;

	releaseWindow (w);

	if (!w->redirected)
		redirectWindow (w);
}

static int
restackWindow (CompWindow *w, Window aboveId)
{
	D(("0x%x : restack window above 0x%x\n", (unsigned int)w->id, aboveId ? (unsigned int) aboveId : 0));

	if (w->prev)
	{
		if (aboveId && aboveId == w->prev->id)
			return 0;
	}
	else if (aboveId == None && !w->next)
		return 0;

	unhookWindowFromScreen (w->screen, w);
	insertWindowIntoScreen (w->screen, w, aboveId);

	return 1;
}

Bool
resizeWindow (CompWindow *w, int x, int y, int width, int height, int borderWidth)
{
	D(("0x%x : resizeWindow: %d:%d %dx%d\n", (unsigned int)w->id, x,y,width,height));
	if (w->attrib.class == InputOnly)
		return TRUE;
	
	if (w->attrib.width		   != width	 ||
		w->attrib.height	   != height ||
		w->attrib.border_width != borderWidth)
	{
		unsigned int pw, ph, actualWidth, actualHeight, ui;
		int dx, dy, dwidth, dheight;
		Pixmap pixmap = None;
		Window root;
		Status result;
		int i;

		pw = width	+ borderWidth * 2;
		ph = height + borderWidth * 2;

		if ((!w->clientId || w->clientMapped) && w->mapNum && w->redirected)
		{			
			pixmap = XCompositeNameWindowPixmap (w->screen->display->display,
												 w->id);
			result = XGetGeometry (w->screen->display->display, pixmap, &root,
								   &i, &i, &actualWidth, &actualHeight,
								   &ui, &ui);

			if (!result || actualWidth != pw || actualHeight != ph)
			{
				XFreePixmap (w->screen->display->display, pixmap);

				return FALSE;
			}
		}
		else if (w->shaded)
		{
			ph = 0;
		}

		addWindowDamage (w);
		
		dx		= x - w->attrib.x;
		dy		= y - w->attrib.y;
		dwidth	= width - w->attrib.width;
		dheight = height - w->attrib.height;

		w->attrib.x		       = x;
		w->attrib.y		       = y;
		w->attrib.width		   = width;
		w->attrib.height	   = height;
		w->attrib.border_width = borderWidth;

		w->width  = pw;
		w->height = ph;

		releaseWindow (w);

		w->pixmap = pixmap;

		if (w->mapNum)
			updateWindowRegion (w);

		(*w->screen->windowResizeNotify) (w, dx, dy, dwidth, dheight);

		addWindowDamage (w);

		w->invisible = WINDOW_INVISIBLE (w);
	}
	else if (w->attrib.x != x || w->attrib.y != y)
	{
		int dx, dy;
		dx = x - w->attrib.x;
		dy = y - w->attrib.y;
		moveWindow (w, dx, dy, TRUE, !w->grabbed); /*XXX !w->grabbed
													 -> was TRUE*/
	}

	return TRUE;
}

void
configureWindow (CompWindow *w, XConfigureEvent *ce)
{
	CompScreen *s = w->screen;

	int x = ce->x;
	int y = ce->y;
	int dx, dy;
	
	if (w->clientId)		
	{
		/* ignore deskwitch window movements */
		if ((ce->width == w->attrib.width) &&
			(ce->height == w->attrib.height) &&
			(MOD(ce->x - w->attrib.x, s->width) == 0) &&
			(MOD(ce->y - w->attrib.y, s->height) == 0))
		{
			if (restackWindow (w, ce->above))
				addWindowDamage (w);
			return;
		}
		else
		{
			dx = w->initialViewportX;
			dy = w->initialViewportY;
			/* //printf("configure %p %d:%d  %d:%d  %d:%d\n", w->id, x, y, dx, dy, (dx - s->x), (dy - s->y)); */
				
			x = ce->x; /* + ((dx - s->x) * s->width); */
			y = ce->y; /* + ((dy - s->y) * s->height); */
		}
	}
	else
	{
		// check if still needed
		w->attrib.override_redirect = FALSE;
	}
	
	w->serverX		     = x;
	w->serverY		     = y;
	w->serverWidth	     = ce->width;
	w->serverHeight	     = ce->height;
	w->serverBorderWidth = ce->border_width;
		
	resizeWindow (w, x, y, ce->width, ce->height, ce->border_width);
	
	if (restackWindow (w, ce->above))
		addWindowDamage (w);
}


void
circulateWindow (CompWindow	 *w, XCirculateEvent *ce)
{
	Window newAboveId;

	if (ce->place == PlaceOnTop)
		newAboveId = getTopWindow (w->screen);
	else
		newAboveId = 0;

	if (restackWindow (w, newAboveId))
		addWindowDamage (w);
}

void
moveWindow (CompWindow *w, int dx, int dy, Bool damage, Bool immediate)
{
	if (dx || dy)
	{
		if (damage)
			addWindowDamage (w);

		w->attrib.x += dx;
		w->attrib.y += dy;

		XOffsetRegion (w->region, dx, dy);

		setWindowMatrix (w);

		w->invisible = WINDOW_INVISIBLE (w);

		(*w->screen->windowMoveNotify) (w, dx, dy, immediate);

		if (damage)
			addWindowDamage (w);
	}
}

void
syncWindowPosition (CompWindow *w)
{
	/* printf("0x%x : syncWindowPosition: %d:%d\n", (unsigned int) w->id, w->attrib.x,  w->attrib.y); */

	w->serverX = w->attrib.x;
	w->serverY = w->attrib.y;
	/* XMoveWindow (w->screen->display->display, w->id, w->attrib.x, w->attrib.y); */

	/* we moved without resizing, so we have to send a configure notify */
	// XXX TESTING sendConfigureNotify (w);
}


/* TODO remove? */
Bool
focusWindow (CompWindow *w)
{
	if (!w->clientId)
		return FALSE;

	if (!w->shaded && (w->state & CompWindowStateHiddenMask))
		return FALSE;

	if (w->attrib.x + w->width	<= 0	||
		w->attrib.y + w->height <= 0	||
		w->attrib.x >= w->screen->width ||
		w->attrib.y >= w->screen->height)
		return FALSE;

	return TRUE;
}

Bool
placeWindow (CompWindow *w, int x, int y, int *newX, int *newY)
{
	return FALSE;
}

void
windowResizeNotify (CompWindow *w, int dx, int dy, int dwidth, int dheight)
{
}

void
windowMoveNotify (CompWindow *w, int dx, int dy, Bool immediate)
{
}

void
windowGrabNotify (CompWindow *w, int x, int y, unsigned int state, unsigned int mask)
{
	w->grabbed = TRUE;
}

void
windowUngrabNotify (CompWindow *w)
{
	w->grabbed = FALSE;
}

void
windowStateChangeNotify (CompWindow *w, unsigned int lastState)
{
}

void
moveInputFocusToWindow (CompWindow *w)
{
	CompScreen *s = w->screen;
	CompDisplay *d = s->display;
	XEvent ev;

	ev.type = ClientMessage;
	ev.xclient.window = w->id;
	ev.xclient.message_type = d->winActiveAtom;
	ev.xclient.format = 32;
	ev.xclient.data.l[0] = 2;
	ev.xclient.data.l[1] = 0; //getCurrentTimeFromDisplay (d);
	ev.xclient.data.l[2] = 0;
	ev.xclient.data.l[3] = 0;
	ev.xclient.data.l[4] = 0;

	XSendEvent (d->display, w->id, FALSE, NoEventMask, &ev);
}
/* TODO */
/*void
  uniconfiyWindow (CompWindow *w)
  {
  CompScreen  *s = w->screen;
  CompDisplay *d = s->display;
  XEvent ev;

  ev.type			= ClientMessage;
  ev.xclient.window		= w->id;
  ev.xclient.message_type = d->winActiveAtom;
  ev.xclient.format		= 32;
  ev.xclient.data.l[0]	  = 2;
  ev.xclient.data.l[1]	  = getCurrentTimeFromDisplay (d);
  ev.xclient.data.l[2]	  = 1;
  ev.xclient.data.l[3]	  = 0;
  ev.xclient.data.l[4]	  = 0;

  XSendEvent (d->display, w->id, FALSE, NoEventMask, &ev);
  }
*/
/* void
 * configureXWindow (CompWindow *w, unsigned int valueMask, XWindowChanges *xwc)
 * {
 * 	D(("configureXWindow, %d:%d %dx%d\n", xwc->x, xwc->y, xwc->width, xwc->height));
 * 
 * 	if (valueMask & CWX)
 * 		w->serverX = xwc->x;
 * 
 * 	if (valueMask & CWY)
 * 		w->serverY = xwc->y;
 * 
 * 	if (valueMask & CWWidth)
 * 		w->serverWidth = xwc->width;
 * 
 * 	if (valueMask & CWHeight)
 * 		w->serverHeight	= xwc->height;
 * 
 * 	if (valueMask & CWBorderWidth)
 * 		w->serverBorderWidth = xwc->border_width;
 * } */


void
raiseWindow (CompWindow *w)
{
	D(("0x%x : raiseWindow\n", (unsigned int)w->id));
	XEvent ev;

	ev.type = ClientMessage;
	ev.xclient.window = w->id;
	ev.xclient.message_type = w->screen->display->restackWindowAtom;
	ev.xclient.format = 32;
	ev.xclient.data.l[0] = 2;
	ev.xclient.data.l[1] = 0;
	ev.xclient.data.l[2] = Above;
	ev.xclient.data.l[3] = 1;
	ev.xclient.data.l[4] = 0;

	XSendEvent (w->screen->display->display,
				w->screen->root,
				FALSE,
				SubstructureRedirectMask | StructureNotifyMask,
				&ev);
}

void
lowerWindow (CompWindow *w)
{
	D(("0x%x : lowerWindow\n", (unsigned int)w->id));
	XEvent ev;

	ev.type = ClientMessage;
	ev.xclient.window	  = w->id;
	ev.xclient.message_type = w->screen->display->restackWindowAtom;
	ev.xclient.format	  = 32;
	ev.xclient.data.l[0]	  = 2;
	ev.xclient.data.l[1]	  = 0;
	ev.xclient.data.l[2]	  = Below;
	ev.xclient.data.l[3]	  = 1;
	ev.xclient.data.l[4]	  = 0;

	XSendEvent (w->screen->display->display,
				w->screen->root,
				FALSE,
				SubstructureRedirectMask | StructureNotifyMask,
				&ev);
}

void
restackWindowAbove (CompWindow *w, CompWindow *sibling)
{
	D(("0x%x : restackWindowAbove: 0x%x\n", (unsigned int)w->id, (unsigned int) sibling->id));

	XEvent ev;

	ev.type		  = ClientMessage;
	ev.xclient.window	  = w->id;
	ev.xclient.message_type = w->screen->display->restackWindowAtom;
	ev.xclient.format	  = 32;
	ev.xclient.data.l[0]	  = 2;
	ev.xclient.data.l[1]	  = sibling->id;
	ev.xclient.data.l[2]	  = Above;
	ev.xclient.data.l[3]	  = 1;
	ev.xclient.data.l[4]	  = 0;

	XSendEvent (w->screen->display->display,
				w->screen->root,
				FALSE,
				SubstructureRedirectMask | StructureNotifyMask,
				&ev);
}


void
restackWindowBelow (CompWindow *w, CompWindow *sibling)
{
	D(("0x%x : restackWindowBelow: 0x%x\n", (unsigned int)w->id, (unsigned int) sibling->id));
	XEvent ev;

	ev.type		  = ClientMessage;
	ev.xclient.window	  = w->id;
	ev.xclient.message_type = w->screen->display->restackWindowAtom;
	ev.xclient.format	  = 32;
	ev.xclient.data.l[0]	  = 2;
	ev.xclient.data.l[1]	  = sibling->id;
	ev.xclient.data.l[2]	  = Below;
	ev.xclient.data.l[3]	  = 1;
	ev.xclient.data.l[4]	  = 0;

	XSendEvent (w->screen->display->display,
				w->screen->root,
				FALSE,
				SubstructureRedirectMask | StructureNotifyMask,
				&ev);
}


void
updateWindowAttributes (CompWindow *w, CompStackingUpdateMode stackingMode)
{
	D(("0x%x : updateWindowAttributes\n", (unsigned int) w->id));

	if (!w->clientId)
		return;

	if (w->state & CompWindowStateShadedMask)
	{
		hideWindow (w);
	}

	/* XXX huh?! test this */
	/*else if (w->shaded)
	  {
	  showWindow (w);
	  }*/
}


/* static void
 * sendViewportMoveRequest (CompScreen *s,
 *			 int		x,
 *			 int		y)
 * {
 *	 D(("sendViewportMoveRequest %d:%d\n",x,y));
 *
 *	   XEvent xev;
 *
 *	   xev.xclient.type	   = ClientMessage;
 *	   xev.xclient.display = s->display->display;
 *	   xev.xclient.format  = 32;
 *
 *	   xev.xclient.message_type = s->display->desktopViewportAtom;
 *	   xev.xclient.window		 = s->root;
 *
 *	   xev.xclient.data.l[0] = 0; /\* from ecomp *\/
 *	   xev.xclient.data.l[1] = x;
 *	   xev.xclient.data.l[2] = y;
 *	   xev.xclient.data.l[3] = 0;
 *	   xev.xclient.data.l[4] = 0;
 *
 *	   XSendEvent (s->display->display,
 *		s->root,
 *		FALSE,
 *		SubstructureRedirectMask | SubstructureNotifyMask,
 *		&xev);
 * } */

void
activateWindow (CompWindow *w)
{
	/* raiseWindow(w); */

	moveInputFocusToWindow (w);
}


void
hideWindow (CompWindow *w)
{
	Bool onDesktop = onCurrentDesktop (w);

	if (!w->clientId)
		return;

	if (!w->minimized && !w->inShowDesktopMode && !w->hidden && onDesktop)
	{
		if (w->state & CompWindowStateShadedMask)
		{
			w->shaded = TRUE;
		}
		else
		{
			return;
		}
	}
	else
	{
		addWindowDamage (w);

		w->shaded = FALSE;
	}

	if (!w->pendingMaps && w->attrib.map_state != IsViewable)
		return;

	w->pendingUnmaps++;


	//XUnmapWindow (w->screen->display->display, w->id);

	if (w->minimized || w->inShowDesktopMode || w->hidden || w->shaded)
		changeWindowState (w, w->state | CompWindowStateHiddenMask);
}

void
showWindow (CompWindow *w)
{
	D(("- showWindow\n"));

	Bool onDesktop = onCurrentDesktop (w);

	if (!w->clientId)
		return;

	if (w->minimized || w->inShowDesktopMode || w->hidden || !onDesktop)
	{
		/* no longer hidden but not on current desktop */
		if (!w->minimized && !w->inShowDesktopMode && !w->hidden)
		{
			if (w->state & CompWindowStateHiddenMask)
				changeWindowState (w, w->state & ~CompWindowStateHiddenMask);
		}

		return;
	}

	/* transition from minimized to shaded */
	if (w->state & CompWindowStateShadedMask)
	{
		w->shaded = TRUE;

		/* if (w->height)
		 * 	resizeWindow (w, w->attrib.x, w->attrib.y,
		 * 				  w->attrib.width, ++w->attrib.height - 1,
		 * 				  w->attrib.border_width); */
		
		addWindowDamage (w);

		return;
	}
	else
	{
		w->shaded = FALSE;
	}

	w->pendingMaps++;

	//XMapWindow (w->screen->display->display, w->id);

	changeWindowState (w, w->state & ~CompWindowStateHiddenMask);
}

void
unredirectWindow (CompWindow *w)
{
	if (!w->redirected)
		return;

	releaseWindow (w);

	XCompositeUnredirectWindow (w->screen->display->display, w->id, //(w->clientId ? w->clientId : w->id),
								CompositeRedirectManual);

	w->redirected = FALSE;
	w->screen->overlayWindowCount++;

	if (w->screen->overlayWindowCount > 0)
		hideOutputWindow (w->screen);
}

void
redirectWindow (CompWindow *w)
{
	if (w->redirected)
		return;

	XCompositeRedirectWindow (w->screen->display->display, w->id, //(w->clientId ? w->clientId : w->id),
							  CompositeRedirectManual);

	w->redirected = TRUE;
	w->screen->overlayWindowCount--;

	if (w->screen->overlayWindowCount < 1)
		showOutputWindow (w->screen);
}

void
defaultViewportForWindow (CompWindow *w, int *vx, int *vy)
{
	viewportForGeometry (w->screen,
						 w->serverX, w->serverY,
						 w->serverWidth, w->serverHeight,
						 w->serverBorderWidth,
						 vx, vy);
	D(("0x%x : defaultViewportForWindow %d:%d %dx%d -> %d:%d\n", (unsigned int)w->id,	 w->serverX, w->serverY,
	   w->serverWidth, w->serverHeight, vx ? *vx : 0,vy ? *vy : 0));
}

/* returns icon with dimensions as close as possible to width and height
   but never greater. */
CompIcon *
getWindowIcon (CompWindow *w, int width, int height)
{
	CompIcon *icon;
	int		 i, wh, diff, oldDiff;

	/* need to fetch icon property */
	if (w->nIcon > 0)
		freeWindowIcons(w);
	
	if (w->nIcon == 0)
	{
		Atom		  actual;
		int		  result, format;
		unsigned long n, left;
		unsigned char *data;

		result = XGetWindowProperty
			(w->screen->display->display, w->id,
			 w->screen->display->wmIconAtom,
			 0L, 65536L, FALSE, XA_CARDINAL,
			 &actual, &format, &n, &left, &data);

		if (result == Success && n && data)
		{
			CompIcon **pIcon;
			CARD32	 *p;
			int		 iw, ih, j;

			for (i = 0; i + 2 < n; i += iw * ih + 2)
			{
				unsigned long *idata = (unsigned long *) data;

				iw	= idata[i];
				ih = idata[i + 1];

				if (iw * ih + 2 > n - i)
					break;

				if (iw && ih)
				{
					icon = malloc (sizeof (CompIcon) +
								   iw * ih * sizeof (CARD32));
					if (!icon)
						continue;

					pIcon = realloc (w->icon,
									 sizeof (CompIcon *) * (w->nIcon + 1));
					if (!pIcon)
					{
						free (icon);
						continue;
					}

					w->icon = pIcon;
					w->icon[w->nIcon] = icon;
					w->nIcon++;

					icon->width	 = iw;
					icon->height = ih;

					initTexture (w->screen, &icon->texture);

					p = (CARD32 *) (icon + 1);

					for (j = 0; j < iw * ih; j++)
					{
						p[j] = idata[i + j + 2];
					}
				}
			}

			XFree (data);
		}

		/* don't fetch property again */
		if (w->nIcon == 0)
			w->nIcon = -1;
	}

	/* no icons available for this window */
	if (w->nIcon == -1)
		return NULL;

	icon = NULL;
	wh	 = width + height;

	for (i = 0; i < w->nIcon; i++)
	{
		/* we also pass icons if they are greater */
		/*if (w->icon[i]->width > width || w->icon[i]->height > height)
		 *	 continue;
		 */
		if (icon)
		{
			diff	  = abs(wh - (w->icon[i]->width + w->icon[i]->height));
			oldDiff = abs(wh - (icon->width + icon->height));

			if (diff < oldDiff)
				icon = w->icon[i];
		}
		else
			icon = w->icon[i];
	}

	return icon;
}

void
freeWindowIcons (CompWindow *w)
{
	int i;

	for (i = 0; i < w->nIcon; i++)
	{
		finiTexture (w->screen, &w->icon[i]->texture);
		free (w->icon[i]);
	}

	if (w->icon)
	{
		free (w->icon);
		w->icon = NULL;
	}

	w->nIcon = 0;
}

int
outputDeviceForWindow (CompWindow *w)
{
	int output = w->screen->currentOutputDev;
	int	width = w->serverWidth + w->serverBorderWidth * 2;
	int	height = w->serverHeight + w->serverBorderWidth * 2;
	int x1, y1, x2, y2;

	x1 = w->screen->outputDev[output].region.extents.x1;
	y1 = w->screen->outputDev[output].region.extents.y1;
	x2 = w->screen->outputDev[output].region.extents.x2;
	y2 = w->screen->outputDev[output].region.extents.y2;

	if (x1 > w->serverX + width	 ||
		y1 > w->serverY + height ||
		x2 < w->serverX		 ||
		y2 < w->serverY)
	{
		output = outputDeviceForPoint (w->screen,
									   w->serverX + width  / 2,
									   w->serverY + height / 2);
	}

	return output;
}

Bool
onCurrentDesktop (CompWindow *w)
{
	//if (w->desktop == 0xffffffff || w->desktop == w->screen->currentDesktop)
	return TRUE;

	//return FALSE;
}

/*void
  setDesktopForWindow (CompWindow	  *w,
  unsigned int desktop)
  {
  if (desktop != 0xffffffff)
  {
  if (w->type & (CompWindowTypeDesktopMask | CompWindowTypeDockMask))
  return;

  if (desktop >= w->screen->nDesktop)
  return;
  }

  if (desktop == w->desktop)
  return;

  w->desktop = desktop;

  if (desktop == 0xffffffff || desktop == w->screen->currentDesktop)
  showWindow (w);
  else
  hideWindow (w);

  setWindowProp (w->screen->display, w->id,
  w->screen->display->winDesktopAtom,
  w->desktop);
  }
*/
/* The compareWindowActiveness function compares the two windows 'w1'
   and 'w2'. It returns an integer less than, equal to, or greater
   than zero if 'w1' is found, respectively, to activated longer time
   ago than, to be activated at the same time, or be activated more
   recently than 'w2'. */
/* int
 * compareWindowActiveness (CompWindow *w1, CompWindow *w2)
 * {
 * 	CompScreen			*s = w1->screen;
 * 	CompActiveWindowHistory *history = &s->history[s->currentHistory];
 * 	int				i;
 * 
 * 	/\* check current window history first *\/
 * 	for (i = 0; i < ACTIVE_WINDOW_HISTORY_SIZE; i++)
 * 	{
 * 		if (history->id[i] == w1->id)
 * 			return 1;
 * 
 * 		if (history->id[i] == w2->id)
 * 			return -1;
 * 
 * 		if (!history->id[i])
 * 			break;
 * 	}
 * 
 * 	return w1->activeNum - w2->activeNum;
 * } */

void
windowAddNotify (CompWindow *w)
{
	w->added = TRUE;
}

