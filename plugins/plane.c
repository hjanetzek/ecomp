/*
 * Copyright © 2006 Red Hat, Inc.
 *
 * Permission to use, copy, modify, distribute, and sell this software
 * and its documentation for any purpose is hereby granted without
 * fee, provided that the above copyright notice appear in all copies
 * and that both that copyright notice and this permission notice
 * appear in supporting documentation, and that the name of
 * Red Hat, Inc. not be used in advertising or publicity pertaining to
 * distribution of the software without specific, written prior permission.
 * Red Hat, Inc. makes no representations about the suitability of this
 * software for any purpose. It is provided "as is" without express or
 * implied warranty.
 *
 * RED HAT, INC. DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN
 * NO EVENT SHALL RED HAT, INC. BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
 * NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION
 * WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * Author: Søren Sandmann <sandmann@redhat.com>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <sys/time.h>

#include <X11/Xatom.h>
#include <X11/Xproto.h>

#include <ecomp.h>

static CompMetadata planeMetadata;

static int displayPrivateIndex;

enum
{
    PLANE_N_DISPLAY_OPTIONS
};

typedef struct _PlaneDisplay {
    int			screenPrivateIndex;
    HandleEventProc	handleEvent;

    CompOption		opt[PLANE_N_DISPLAY_OPTIONS];
} PlaneDisplay;

typedef struct _PlaneScreen {
    int windowPrivateIndex;

    PaintTransformedOutputProc		paintTransformedOutput;
    PreparePaintScreenProc		preparePaintScreen;
    DonePaintScreenProc			donePaintScreen;
    PaintOutputProc			paintOutput;
    PaintWindowProc                     paintWindow;

    WindowGrabNotifyProc		windowGrabNotify;
    WindowUngrabNotifyProc		windowUngrabNotify;

    CompTimeoutHandle			timeoutHandle;
    int					timer;

    double				cur_x;
    double				cur_y;
    double				dest_x;
    double				dest_y;
} PlaneScreen;

#define GET_PLANE_DISPLAY(d)				       \
    ((PlaneDisplay *) (d)->privates[displayPrivateIndex].ptr)

#define PLANE_DISPLAY(d)		       \
    PlaneDisplay *pd = GET_PLANE_DISPLAY (d)

#define GET_PLANE_SCREEN(s, pd)				   \
    ((PlaneScreen *) (s)->privates[(pd)->screenPrivateIndex].ptr)

#define PLANE_SCREEN(s)						      \
    PlaneScreen *ps = GET_PLANE_SCREEN (s, GET_PLANE_DISPLAY (s->display))

#define NUM_OPTIONS(s) (sizeof ((s)->opt) / sizeof (CompOption))

static Bool
endMove (void *data)
{
    CompScreen *screen = data;
    PLANE_SCREEN (screen);

    moveScreenViewport (screen, -ps->dest_x, -ps->dest_y, TRUE);
    focusDefaultWindow (screen->display);

    ps->dest_x = 0;
    ps->dest_y = 0;

    ps->timeoutHandle = 0;
    return FALSE;
}

#define SCROLL_TIME	1

static void
computeTranslation (PlaneScreen *ps,
		    double	*x,
		    double      *y)
{
    double dx, dy;
    double elapsed = 1 - (ps->timer / (double)SCROLL_TIME);

    if (elapsed < 0.0)
	elapsed = 0.0;
    if (elapsed > 1.0)
	elapsed = 1.0;

    /* Use temporary variables to you can pass in &ps->cur_x */
    dx = (ps->dest_x - ps->cur_x) * elapsed + ps->cur_x;
    dy = (ps->dest_y - ps->cur_y) * elapsed + ps->cur_y;

    *x = dx;
    *y = dy;
}

static void
moveViewport (CompScreen *screen,
	      int	 dx,
	      int	 dy)
{
    PLANE_SCREEN (screen);

    if (dx == 0 && dy == 0)
	return;

    if (ps->timeoutHandle)
    {
	computeTranslation (ps, &ps->cur_x, &ps->cur_y);

	ps->dest_x += dx;
	ps->dest_y += dy;

	compRemoveTimeout (ps->timeoutHandle);
    }
    else
    {
	ps->cur_x = 0.0;
	ps->cur_y = 0.0;
	ps->dest_x = dx;
	ps->dest_y = dy;
    }

    if (ps->dest_x + screen->x > screen->hsize - 1)
	ps->dest_x = screen->hsize - screen->x - 1;

    if (ps->dest_x + screen->x < 0)
	ps->dest_x = -screen->x;

    if (ps->dest_y + screen->y > screen->vsize - 1)
	ps->dest_y = screen->vsize - screen->y - 1;

    if (ps->dest_y + screen->y < 0)
	ps->dest_y = -screen->y;

    ps->timer = SCROLL_TIME;
    ps->timeoutHandle = compAddTimeout (SCROLL_TIME, endMove, screen);

    damageScreen (screen);
}

static void
planePreparePaintScreen (CompScreen *s,
			 int	    msSinceLastPaint)
{
    PLANE_SCREEN (s);

    ps->timer -= msSinceLastPaint;

    UNWRAP (ps, s, preparePaintScreen);

    (* s->preparePaintScreen) (s, msSinceLastPaint);

    WRAP (ps, s, preparePaintScreen, planePreparePaintScreen);
}

static void
planePaintTransformedOutput (CompScreen		     *screen,
			     const ScreenPaintAttrib *sAttrib,
			     const CompTransform     *transform,
			     Region		     region,
			     CompOutput              *output,
			     unsigned int	     mask)
{
    PLANE_SCREEN (screen);

    UNWRAP (ps, screen, paintTransformedOutput);

    if (0)//(ps->timeoutHandle) // TODO move only windows option
    {
	CompTransform sTransform = *transform;
	double dx, dy;
	int vx, vy;

	clearTargetOutput (screen->display, GL_COLOR_BUFFER_BIT);

	computeTranslation (ps, &dx, &dy);

	dx *= -1;
	dy *= -1;

	vx = 0;
	vy = 0;

	while (dx > 1)
	{
	    dx -= 1.0;
	    moveScreenViewport (screen, 1, 0, FALSE);
	    vx++;
	}

	while (dx < -1)
	{
	    dx += 1.0;
	    moveScreenViewport (screen, -1, 0, FALSE);
	    vx--;
	}

	while (dy > 1)
	{
	    dy -= 1.0;
	    moveScreenViewport (screen, 0, 1, FALSE);
	    vy++;
	}

	while (dy < -1)
	{
	    dy += 1.0;
	    moveScreenViewport (screen, 0, -1, FALSE);
	    vy--;
	}

	matrixTranslate (&sTransform, dx, -dy, 0.0);

	(*screen->paintTransformedOutput) (screen, sAttrib, &sTransform,
					   region, output, mask);

	if (dx > 0)
	{
	    matrixTranslate (&sTransform, -1.0, 0.0, 0.0);
	    moveScreenViewport (screen, 1, 0, FALSE);
	}
	else
	{
	    matrixTranslate (&sTransform, 1.0, 0.0, 0.0);
	    moveScreenViewport (screen, -1, 0, FALSE);
	}

	(*screen->paintTransformedOutput) (screen, sAttrib, &sTransform,
					   region, output, mask);

	if (dy > 0)
	{
	    matrixTranslate (&sTransform, 0.0, 1.0, 0.0);
	    moveScreenViewport (screen, 0, 1, FALSE);
	}
	else
	{
	    matrixTranslate (&sTransform, 0.0, -1.0, 0.0);
	    moveScreenViewport (screen, 0, -1, FALSE);
	}

	(*screen->paintTransformedOutput) (screen, sAttrib, &sTransform,
					   region, output, mask);

	if (dx > 0)
	{
	    matrixTranslate (&sTransform, 1.0, 0.0, 0.0);
	    moveScreenViewport (screen, -1, 0, FALSE);
	}
	else
	{
	    matrixTranslate (&sTransform, -1.0, 0.0, 0.0);
	    moveScreenViewport (screen, 1, 0, FALSE);
	}

	(*screen->paintTransformedOutput) (screen, sAttrib, &sTransform,
					   region, output, mask);

	if (dy > 0)
	{
	    moveScreenViewport (screen, 0, -1, FALSE);
	}
	else
	{
	    moveScreenViewport (screen, 0, 1, FALSE);
	}

	moveScreenViewport (screen, -vx, -vy, FALSE);
    }
    else
    {
	(*screen->paintTransformedOutput) (screen, sAttrib, transform,
					   region, output, mask);
    }

    WRAP (ps, screen, paintTransformedOutput, planePaintTransformedOutput);
}

static Bool
planePaintWindow (CompWindow		  *w,
		  const WindowPaintAttrib *attrib,
		  const CompTransform	  *transform,
		  Region		  region,
		  unsigned int		  mask)
{
    CompScreen *s = w->screen;
    Bool       status;

    PLANE_SCREEN (s);

    if (ps->timeoutHandle && w->clientId && 
	!(w->state & CompWindowStateStickyMask) &&
	!(w->type & (CompWindowTypeDesktopMask | CompWindowTypeDockMask))) // TODO
										     // move only windows option
    {
	mask |= PAINT_WINDOW_NO_CORE_INSTANCE_MASK;

	UNWRAP (ps, s, paintWindow);
	status = (*s->paintWindow) (w, attrib, transform, region, mask);
	WRAP (ps, s, paintWindow, planePaintWindow);

	FragmentAttrib fragment;
	CompTransform  wTransform = *transform;
	double dx,dy;
	    
	if (mask & PAINT_WINDOW_OCCLUSION_DETECTION_MASK)
	    return FALSE;

	initFragmentAttrib (&fragment, &w->lastPaint);

	if (w->alpha || fragment.opacity != OPAQUE)
	    mask |= PAINT_WINDOW_TRANSLUCENT_MASK;

	matrixTranslate (&wTransform, w->attrib.x, w->attrib.y, 0.0f);

	computeTranslation (ps, &dx, &dy);

	matrixTranslate (&wTransform,
			 (-dx * s->width) - w->attrib.x,
			 (-dy * s->height) - w->attrib.y,
			 0.0f);

	glPushMatrix ();
	glLoadMatrixf (wTransform.m);

	(*s->drawWindow) (w, &wTransform, &fragment, region,
			  mask | PAINT_WINDOW_TRANSFORMED_MASK);

	glPopMatrix ();
    }
    else
    {
	UNWRAP (ps, s, paintWindow);
	status = (*s->paintWindow) (w, attrib, transform, region, mask);
	WRAP (ps, s, paintWindow, planePaintWindow);
    }

    return status;
}

static void
planeDonePaintScreen (CompScreen *s)
{
    PLANE_SCREEN (s);

    if (ps->timeoutHandle)
	damageScreen (s);

    UNWRAP (ps, s, donePaintScreen);

    (*s->donePaintScreen) (s);

    WRAP (ps, s, donePaintScreen, planeDonePaintScreen);
}

static Bool
planePaintOutput (CompScreen		  *s,
		  const ScreenPaintAttrib *sAttrib,
		  const CompTransform	  *transform,
		  Region		  region,
		  CompOutput		  *output,
		  unsigned int		  mask)
{
    Bool status;

    PLANE_SCREEN (s);

    if (ps->timeoutHandle)
    {
	mask &= ~PAINT_SCREEN_REGION_MASK;
	mask |= PAINT_SCREEN_TRANSFORMED_MASK;
    }

    UNWRAP (ps, s, paintOutput);
    status = (*s->paintOutput) (s, sAttrib, transform, region, output, mask);
    WRAP (ps, s, paintOutput, planePaintOutput);

    return status;
}

static void
planeHandleEvent (CompDisplay *d,
		  XEvent      *event)
{
    CompScreen *s;

    PLANE_DISPLAY (d);

    switch (event->type) {
    case ClientMessage:
      /*if (event->xclient.message_type == d->winActiveAtom)
	{
	    CompWindow *w;

	    w = findWindowAtDisplay (d, event->xclient.window);
	    if (w)
	    {
		int dx, dy;

		s = w->screen;

		if (otherScreenGrabExist (s, "plane", "switcher", "cube", 0))
		    break;

		defaultViewportForWindow (w, &dx, &dy);
		dx -= s->x;
		dy -= s->y;

		moveViewport (s, dx, dy);
	    }
	}
	else*/ 
      if (event->xclient.message_type == d->desktopViewportAtom)
	{
	    if (event->xclient.data.l[0]) break;
	    int dx, dy;

	    s = findScreenAtDisplay (d, event->xclient.window);
	    if (!s)
		break;

	    if (otherScreenGrabExist (s, "plane", "switcher", "cube", 0))
		break;

	    dx = (event->xclient.data.l[1] / s->width) - s->x;
	    dy = (event->xclient.data.l[2] / s->height) - s->y;

	    if (!dx && !dy)
		break;

	    moveViewport (s, dx, dy);
	}
	break;

    default:
	break;
    }

    UNWRAP (pd, d, handleEvent);
    (*d->handleEvent) (d, event);
    WRAP (pd, d, handleEvent, planeHandleEvent);
}

static void
planeWindowGrabNotify (CompWindow   *w,
		       int	    x,
		       int	    y,
		       unsigned int state,
		       unsigned int mask)
{
    PLANE_SCREEN (w->screen);

    UNWRAP (ps, w->screen, windowGrabNotify);
    (*w->screen->windowGrabNotify) (w, x, y, state, mask);
    WRAP (ps, w->screen, windowGrabNotify, planeWindowGrabNotify);
}

static void
planeWindowUngrabNotify (CompWindow *w)
{
    PLANE_SCREEN (w->screen);

    UNWRAP (ps, w->screen, windowUngrabNotify);
    (*w->screen->windowUngrabNotify) (w);
    WRAP (ps, w->screen, windowUngrabNotify, planeWindowUngrabNotify);
}

static CompOption *
planeGetDisplayOptions (CompPlugin  *plugin,
			CompDisplay *display,
			int	    *count)
{
    PLANE_DISPLAY (display);

    *count = NUM_OPTIONS (pd);
    return pd->opt;
}

static Bool
planeSetDisplayOption (CompPlugin      *plugin,
		       CompDisplay     *display,
		       char	       *name,
		       CompOptionValue *value)
{
    CompOption *o;

    PLANE_DISPLAY (display);

    o = compFindOption (pd->opt, NUM_OPTIONS (pd), name, NULL);
    if (!o)
	return FALSE;

    return compSetDisplayOption (display, o, value);
}


static const CompMetadataOptionInfo planeDisplayOptionInfo[] = {};

static Bool
planeInitDisplay (CompPlugin  *p,
		  CompDisplay *d)
{
    PlaneDisplay *pd;

    pd = malloc (sizeof (PlaneDisplay));
    if (!pd)
	return FALSE;

    if (!compInitDisplayOptionsFromMetadata (d,
					     &planeMetadata,
					     planeDisplayOptionInfo,
					     pd->opt,
					     PLANE_N_DISPLAY_OPTIONS))
    {
	free (pd);
	return FALSE;
    }

    pd->screenPrivateIndex = allocateScreenPrivateIndex (d);
    if (pd->screenPrivateIndex < 0)
    {
	compFiniDisplayOptions (d, pd->opt, PLANE_N_DISPLAY_OPTIONS);
	free (pd);
	return FALSE;
    }

    WRAP (pd, d, handleEvent, planeHandleEvent);

    d->privates[displayPrivateIndex].ptr = pd;

    return TRUE;
}

static void
planeFiniDisplay (CompPlugin  *p,
		  CompDisplay *d)
{
    PLANE_DISPLAY (d);

    freeScreenPrivateIndex (d, pd->screenPrivateIndex);

    UNWRAP (pd, d, handleEvent);

    compFiniDisplayOptions (d, pd->opt, PLANE_N_DISPLAY_OPTIONS);

    free (pd);
}

static Bool
planeInitScreen (CompPlugin *p,
		 CompScreen *s)
{
    PlaneScreen *ps;

    PLANE_DISPLAY (s->display);

    ps = malloc (sizeof (PlaneScreen));
    if (!ps)
	return FALSE;

    ps->windowPrivateIndex = allocateWindowPrivateIndex (s);
    if (ps->windowPrivateIndex < 0)
    {
      //compFiniScreenOptions (s, ps->opt, SCALE_SCREEN_OPTION_NUM);
	free (ps);
	return FALSE;
    }

    ps->timeoutHandle = 0;

    WRAP (ps, s, paintTransformedOutput, planePaintTransformedOutput);
    WRAP (ps, s, preparePaintScreen, planePreparePaintScreen);
    WRAP (ps, s, donePaintScreen, planeDonePaintScreen);
    WRAP (ps, s, paintOutput, planePaintOutput);
    WRAP (ps, s, paintWindow, planePaintWindow);
    WRAP (ps, s, windowGrabNotify, planeWindowGrabNotify);
    WRAP (ps, s, windowUngrabNotify, planeWindowUngrabNotify);

    s->privates[pd->screenPrivateIndex].ptr = ps;

    return TRUE;
}

static void
planeFiniScreen (CompPlugin *p,
		 CompScreen *s)
{
    PLANE_SCREEN (s);

    UNWRAP (ps, s, paintTransformedOutput);
    UNWRAP (ps, s, preparePaintScreen);
    UNWRAP (ps, s, donePaintScreen);
    UNWRAP (ps, s, paintOutput);
    UNWRAP (ps, s, paintWindow);
    UNWRAP (ps, s, windowGrabNotify);
    UNWRAP (ps, s, windowUngrabNotify);

    freeWindowPrivateIndex (s, ps->windowPrivateIndex);

    free (ps);
}

static Bool
planeInit (CompPlugin *p)
{
    if (!compInitPluginMetadataFromInfo (&planeMetadata,
					 p->vTable->name,
					 planeDisplayOptionInfo,
					 PLANE_N_DISPLAY_OPTIONS,
					 NULL, 0))
	return FALSE;

    displayPrivateIndex = allocateDisplayPrivateIndex ();
    if (displayPrivateIndex < 0)
    {
	compFiniMetadata (&planeMetadata);
	return FALSE;
    }

    compAddMetadataFromFile (&planeMetadata, p->vTable->name);

    return TRUE;
}

static void
planeFini (CompPlugin *p)
{
    freeDisplayPrivateIndex (displayPrivateIndex);
    compFiniMetadata (&planeMetadata);
}

static int
planeGetVersion (CompPlugin *plugin,
		 int	    version)
{
    return ABIVERSION;
}

static CompMetadata *
planeGetMetadata (CompPlugin *plugin)
{
    return &planeMetadata;
}

CompPluginVTable planeVTable = {
    "plane",
    planeGetVersion,
    planeGetMetadata,
    planeInit,
    planeFini,
    planeInitDisplay,
    planeFiniDisplay,
    planeInitScreen,
    planeFiniScreen,
    0,
    0,
    planeGetDisplayOptions,
    planeSetDisplayOption,
    NULL, /* planeGetScreenOptions, */
    NULL  /* planeSetScreenOption, */
};

CompPluginVTable *
getCompPluginInfo (void)
{
    return &planeVTable;
}
