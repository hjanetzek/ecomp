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
 * Modified by Hannes Janetzek <hannes.janetzek at gmail dot com>
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

typedef struct _PlaneDisplay
{
   int             screenPrivateIndex;
   HandleEventProc handleEvent;

   CompOption      opt[PLANE_N_DISPLAY_OPTIONS];
} PlaneDisplay;

typedef struct _PlaneScreen
{
   int windowPrivateIndex;
} PlaneScreen;

#define GET_PLANE_DISPLAY(d) \
  ((PlaneDisplay *)(d)->privates[displayPrivateIndex].ptr)

#define PLANE_DISPLAY(d) \
  PlaneDisplay * pd = GET_PLANE_DISPLAY (d)

#define GET_PLANE_SCREEN(s, pd) \
  ((PlaneScreen *)(s)->privates[(pd)->screenPrivateIndex].ptr)

#define PLANE_SCREEN(s) \
  PlaneScreen * ps = GET_PLANE_SCREEN (s, GET_PLANE_DISPLAY (s->display))

#define NUM_OPTIONS(s) (sizeof ((s)->opt) / sizeof (CompOption))

static void
planeHandleEvent(CompDisplay *d,
                 XEvent      *event)
{
   CompScreen *s;

   PLANE_DISPLAY (d);

   switch (event->type) {
      case ClientMessage:
        if (event->xclient.message_type == d->desktopViewportAtom)
          {
             if (event->xclient.data.l[0]) break;

             int dx, dy;

             s = findScreenAtDisplay (d, event->xclient.window);
             if (!s)
               break;

             if (otherScreenGrabExist (s, "plane", "switcher", "move", 0))
               break;

             dx = (event->xclient.data.l[1] / s->width) - s->x;
             dy = (event->xclient.data.l[2] / s->height) - s->y;

             if (!dx && !dy)
               break;
             moveScreenViewport (s, -dx, -dy, TRUE);
          }
        break;

      default:
        break;
     }

   UNWRAP (pd, d, handleEvent);
   (*d->handleEvent)(d, event);
   WRAP (pd, d, handleEvent, planeHandleEvent);
}

static CompOption *
planeGetDisplayOptions(CompPlugin  *plugin,
                       CompDisplay *display,
                       int         *count)
{
   PLANE_DISPLAY (display);

   *count = NUM_OPTIONS (pd);
   return pd->opt;
}

static Bool
planeSetDisplayOption(CompPlugin      *plugin,
                      CompDisplay     *display,
                      char            *name,
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
planeInitDisplay(CompPlugin  *p,
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
planeFiniDisplay(CompPlugin  *p,
                 CompDisplay *d)
{
   PLANE_DISPLAY (d);

   freeScreenPrivateIndex (d, pd->screenPrivateIndex);

   UNWRAP (pd, d, handleEvent);

   compFiniDisplayOptions (d, pd->opt, PLANE_N_DISPLAY_OPTIONS);

   free (pd);
}

static Bool
planeInitScreen(CompPlugin *p,
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
        free (ps);
        return FALSE;
     }

   s->privates[pd->screenPrivateIndex].ptr = ps;

   return TRUE;
}

static void
planeFiniScreen(CompPlugin *p,
                CompScreen *s)
{
   PLANE_SCREEN (s);

   freeWindowPrivateIndex (s, ps->windowPrivateIndex);

   free (ps);
}

static Bool
planeInit(CompPlugin *p)
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
planeFini(CompPlugin *p)
{
   freeDisplayPrivateIndex (displayPrivateIndex);
   compFiniMetadata (&planeMetadata);
}

static int
planeGetVersion(CompPlugin *plugin,
                int         version)
{
   return ABIVERSION;
}

static CompMetadata *
planeGetMetadata(CompPlugin *plugin)
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
   NULL,  /* planeGetScreenOptions, */
   NULL   /* planeSetScreenOption, */
};

CompPluginVTable *
getCompPluginInfo(void)
{
   return &planeVTable;
}

