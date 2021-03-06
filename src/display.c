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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/poll.h>

#define XK_MISCELLANY
#include <X11/keysymdef.h>

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/extensions/Xcomposite.h>
#include <X11/extensions/Xrandr.h>
#include <X11/extensions/shape.h>

#include <ecomp.h>

/* #define DEBUG 1 */

typedef struct _CompTimeout
{
   struct _CompTimeout *next;
   int                  time;
   int                  left;
   CallBackProc         callBack;
   void                *closure;
   CompTimeoutHandle    handle;
} CompTimeout;

typedef struct _CompWatchFd
{
   struct _CompWatchFd *next;
   int                  fd;
   CallBackProc         callBack;
   void                *closure;
   CompWatchFdHandle    handle;
} CompWatchFd;

static CompTimeout *timeouts = 0;
static struct timeval lastTimeout;
static CompTimeoutHandle lastTimeoutHandle = 1;

static CompWatchFd *watchFds = 0;
static CompWatchFdHandle lastWatchFdHandle = 1;
static struct pollfd *watchPollFds = 0;
static int nWatchFds = 0;

static CompFileWatchHandle lastFileWatchHandle = 1;

static CompScreen *targetScreen = NULL;
static CompOutput *targetOutput;
static Region tmpRegion, outputRegion;

static Bool inHandleEvent = FALSE;

static const CompTransform identity = {
   {
      1.0, 0.0, 0.0, 0.0,
      0.0, 1.0, 0.0, 0.0,
      0.0, 0.0, 1.0, 0.0,
      0.0, 0.0, 0.0, 1.0
   }
};

int lastPointerX = 0;
int lastPointerY = 0;
int pointerX = 0;
int pointerY = 0;

#define NUM_OPTIONS(d) (sizeof ((d)->opt) / sizeof (CompOption))

CompDisplay *compDisplays = 0;

static CompDisplay compDisplay;

static char *displayPrivateIndices = 0;
static int displayPrivateLen = 0;

static int
reallocDisplayPrivate(int   size,
                      void *closure)
{
   CompDisplay *d = compDisplays;
   void *privates;

   if (d)
     {
        privates = realloc (d->privates, size * sizeof (CompPrivate));
        if (!privates)
          return FALSE;

        d->privates = (CompPrivate *)privates;
     }

   return TRUE;
}

int
allocateDisplayPrivateIndex(void)
{
   return allocatePrivateIndex (&displayPrivateLen,
                                &displayPrivateIndices,
                                reallocDisplayPrivate, 0);
}

void
freeDisplayPrivateIndex(int index)
{
   freePrivateIndex (displayPrivateLen, displayPrivateIndices, index);
}

const CompMetadataOptionInfo coreDisplayOptionInfo[COMP_DISPLAY_OPTION_NUM] = {
   { "active_plugins", "list", "<type>string</type>", 0, 0 },
   { "texture_filter", "int", RESTOSTRING (0, 2), 0, 0 }
};

CompOption *
compGetDisplayOptions(CompDisplay *display,
                      int         *count)
{
   *count = NUM_OPTIONS (display);
   return display->opt;
}

static Bool
setDisplayOption(CompDisplay *display, char *name, CompOptionValue *value)
{
   CompOption *o;
   int index;

   o = compFindOption (display->opt, NUM_OPTIONS (display), name, &index);
   if (!o) return FALSE;

   switch (index) {
      case COMP_DISPLAY_OPTION_ACTIVE_PLUGINS:
        if (compSetOptionList (o, value))
          {
             display->dirtyPluginList = TRUE;
             return TRUE;
          }
        break;

      case COMP_DISPLAY_OPTION_TEXTURE_FILTER:
        if (compSetIntOption (o, value))
          {
             CompScreen *s;

             for (s = display->screens; s; s = s->next)
               damageScreen (s);

             if (!o->value.i)
               display->textureFilter = GL_NEAREST;
             else
               display->textureFilter = GL_LINEAR;

             return TRUE;
          }
        break;

      default:
        if (compSetDisplayOption (display, o, value))
          return TRUE;
        break;
     }

   return FALSE;
}

static Bool
setDisplayOptionForPlugin(CompDisplay *display, char *plugin,
                          char *name, CompOptionValue *value)
{
   CompPlugin *p;

   p = findActivePlugin (plugin);
   if (p && p->vTable->setDisplayOption)
     return (*p->vTable->setDisplayOption)(p, display, name, value);

   return FALSE;
}

static void
updatePlugins(CompDisplay *d)
{
   CompOption *o;
   CompPlugin *p, **pop = 0;
   int nPop, i, j;

   d->dirtyPluginList = FALSE;

   o = &d->opt[COMP_DISPLAY_OPTION_ACTIVE_PLUGINS];
   for (i = 0; i < d->plugin.list.nValue && i < o->value.list.nValue; i++)
     {
        if (strcmp (d->plugin.list.value[i].s, o->value.list.value[i].s))
          break;
     }

   nPop = d->plugin.list.nValue - i;

   if (nPop)
     {
        pop = malloc (sizeof (CompPlugin *) * nPop);
        if (!pop)
          {
             (*d->setDisplayOption)(d, o->name, &d->plugin);
             return;
          }
     }

   for (j = 0; j < nPop; j++)
     {
        pop[j] = popPlugin ();
        d->plugin.list.nValue--;
        free (d->plugin.list.value[d->plugin.list.nValue].s);
     }

   for (; i < o->value.list.nValue; i++)
     {
        p = 0;
        for (j = 0; j < nPop; j++)
          {
             if (pop[j] && strcmp (pop[j]->vTable->name,
                                   o->value.list.value[i].s) == 0)
               {
                  if (pushPlugin (pop[j]))
                    {
                       p = pop[j];
                       pop[j] = 0;
                       break;
                    }
               }
          }

        if (p == 0)
          {
             p = loadPlugin (o->value.list.value[i].s);
             if (p)
               {
                  if (!pushPlugin (p))
                    {
                       unloadPlugin (p);
                       p = 0;
                    }
               }
          }

        if (p)
          {
             CompOptionValue *value;

             value = realloc (d->plugin.list.value, sizeof (CompOptionValue) *
                              (d->plugin.list.nValue + 1));
             if (value)
               {
                  value[d->plugin.list.nValue].s = strdup (p->vTable->name);

                  d->plugin.list.value = value;
                  d->plugin.list.nValue++;
               }
             else
               {
                  p = popPlugin ();
                  unloadPlugin (p);
               }
          }
     }

   for (j = 0; j < nPop; j++)
     {
        if (pop[j])
          unloadPlugin (pop[j]);
     }

   if (nPop)
     free (pop);

   (*d->setDisplayOption)(d, o->name, &d->plugin);
}

static void
addTimeout(CompTimeout *timeout)
{
   CompTimeout *p = 0, *t;

   for (t = timeouts; t; t = t->next)
     {
        if (timeout->time < t->left)
          break;

        p = t;
     }

   timeout->next = t;
   timeout->left = timeout->time;

   if (p)
     p->next = timeout;
   else
     timeouts = timeout;
}

CompTimeoutHandle
compAddTimeout(int time, CallBackProc callBack, void *closure)
{
   CompTimeout *timeout;

   timeout = malloc (sizeof (CompTimeout));
   if (!timeout)
     return 0;

   timeout->time = time;
   timeout->callBack = callBack;
   timeout->closure = closure;
   timeout->handle = lastTimeoutHandle++;

   if (lastTimeoutHandle == MAXSHORT)
     lastTimeoutHandle = 1;

   if (!timeouts)
     gettimeofday (&lastTimeout, 0);

   addTimeout (timeout);

   return timeout->handle;
}

void *
compRemoveTimeout(CompTimeoutHandle handle)
{
   CompTimeout *p = 0, *t;
   void *closure = NULL;

   for (t = timeouts; t; t = t->next)
     {
        if (t->handle == handle)
          break;

        p = t;
     }

   if (t)
     {
        if (p)
          p->next = t->next;
        else
          timeouts = t->next;

        closure = t->closure;

        free (t);
     }

   return closure;
}

CompWatchFdHandle
compAddWatchFd(int fd, short int events, CallBackProc callBack, void *closure)
{
   CompWatchFd *watchFd;

   watchFd = malloc (sizeof (CompWatchFd));
   if (!watchFd)
     return 0;

   watchFd->fd = fd;
   watchFd->callBack = callBack;
   watchFd->closure = closure;
   watchFd->handle = lastWatchFdHandle++;

   if (lastWatchFdHandle == MAXSHORT)
     lastWatchFdHandle = 1;

   watchFd->next = watchFds;
   watchFds = watchFd;

   nWatchFds++;

   watchPollFds = realloc (watchPollFds, nWatchFds * sizeof (struct pollfd));

   watchPollFds[nWatchFds - 1].fd = fd;
   watchPollFds[nWatchFds - 1].events = events;

   return watchFd->handle;
}

void
compRemoveWatchFd(CompWatchFdHandle handle)
{
   CompWatchFd *p = 0, *w;
   int i;

   for (i = nWatchFds - 1, w = watchFds; w; i--, w = w->next)
     {
        if (w->handle == handle)
          break;

        p = w;
     }

   if (w)
     {
        if (p)
          p->next = w->next;
        else
          watchFds = w->next;

        nWatchFds--;

        if (i < nWatchFds)
          memmove (&watchPollFds[i], &watchPollFds[i + 1],
                   (nWatchFds - i) * sizeof (struct pollfd));

        free (w);
     }
}

short int
compWatchFdEvents(CompWatchFdHandle handle)
{
   CompWatchFd *w;
   int i;

   for (i = nWatchFds - 1, w = watchFds; w; i--, w = w->next)
     if (w->handle == handle)
       return watchPollFds[i].revents;

   return 0;
}

#define TIMEVALDIFF(tv1, tv2)                                            \
  ((tv1)->tv_sec == (tv2)->tv_sec || (tv1)->tv_usec >= (tv2)->tv_usec) ? \
  ((((tv1)->tv_sec - (tv2)->tv_sec) * 1000000) +                         \
   ((tv1)->tv_usec - (tv2)->tv_usec)) / 1000 :                           \
  ((((tv1)->tv_sec - 1 - (tv2)->tv_sec) * 1000000) +                     \
   (1000000 + (tv1)->tv_usec - (tv2)->tv_usec)) / 1000

static int
getTimeToNextRedraw(CompScreen *s, struct timeval *tv, struct timeval *lastTv, Bool idle)
{
   int diff, next;

   diff = TIMEVALDIFF (tv, lastTv);

   /* handle clock rollback */
   if (diff < 0)
     diff = 0;

   if (idle ||
       (s->getVideoSync && s->opt[COMP_SCREEN_OPTION_SYNC_TO_VBLANK].value.b))
     {
        if (s->timeMult > 1)
          {
             s->frameStatus = -1;
             s->redrawTime = s->optimalRedrawTime;
             s->timeMult--;
          }
     }
   else
     {
        if (diff > s->redrawTime)
          {
             if (s->frameStatus > 0)
               s->frameStatus = 0;

             next = s->optimalRedrawTime * (s->timeMult + 1);
             if (diff > next)
               {
                  s->frameStatus--;
                  if (s->frameStatus < -1)
                    {
                       s->timeMult++;
                       s->redrawTime = diff = next;
                    }
               }
          }
        else if (diff < s->redrawTime)
          {
             if (s->frameStatus < 0)
               s->frameStatus = 0;

             if (s->timeMult > 1)
               {
                  next = s->optimalRedrawTime * (s->timeMult - 1);
                  if (diff < next)
                    {
                       s->frameStatus++;
                       if (s->frameStatus > 4)
                         {
                            s->timeMult--;
                            s->redrawTime = next;
                         }
                    }
               }
          }
     }

   if (diff > s->redrawTime)
     return 0;

   return s->redrawTime - diff;
}

static int
doPoll(int timeout)
{
   int rv;

   rv = poll (watchPollFds, nWatchFds, timeout);
   if (rv)
     {
        CompWatchFd *w;
        int i;

        for (i = nWatchFds - 1, w = watchFds; w; i--, w = w->next)
          {
             if (watchPollFds[i].revents != 0 && w->callBack)
               w->callBack (w->closure);
          }
     }

   return rv;
}

static void
handleTimeouts(struct timeval *tv)
{
   CompTimeout *t;
   int timeDiff;

   timeDiff = TIMEVALDIFF (tv, &lastTimeout);

   /* handle clock rollback */
   if (timeDiff < 0)
     timeDiff = 0;

   for (t = timeouts; t; t = t->next)
     t->left -= timeDiff;

   while (timeouts && timeouts->left <= 0)
     {
        t = timeouts;
        if ((*t->callBack)(t->closure))
          {
             timeouts = t->next;
             addTimeout (t);
          }
        else
          {
             timeouts = t->next;
             free (t);
          }
     }

   lastTimeout = *tv;
}

static void
waitForVideoSync(CompScreen *s)
{
   unsigned int sync;

   if (!s->opt[COMP_SCREEN_OPTION_SYNC_TO_VBLANK].value.b)
     return;

   if (s->getVideoSync)
     {
        glFlush ();

        (*s->getVideoSync)(&sync);
        (*s->waitVideoSync)(2, (sync + 1) % 2, &sync);
     }
}

void
paintScreen(CompScreen *s, CompOutput *outputs, int numOutput, unsigned int mask)
{
   XRectangle r;
   int i;

   for (i = 0; i < numOutput; i++)
     {
        targetScreen = s;
        targetOutput = &outputs[i];

        r.x = outputs[i].region.extents.x1;
        r.y = s->height - outputs[i].region.extents.y2;
        r.width = outputs[i].width;
        r.height = outputs[i].height;

        if (s->lastViewport.x != r.x ||
            s->lastViewport.y != r.y ||
            s->lastViewport.width != r.width ||
            s->lastViewport.height != r.height)
          {
             glViewport (r.x, r.y, r.width, r.height);
             s->lastViewport = r;
          }

        if (mask & COMP_SCREEN_DAMAGE_ALL_MASK)
          {
             (*s->paintOutput)
               (s, &defaultScreenPaintAttrib, &identity,
               &outputs[i].region, &outputs[i],
               PAINT_SCREEN_REGION_MASK | PAINT_SCREEN_FULL_MASK);
          }
        else if (mask & COMP_SCREEN_DAMAGE_REGION_MASK)
          {
             XIntersectRegion (tmpRegion, &outputs[i].region, outputRegion);

             if (!(*s->paintOutput)
                   (s, &defaultScreenPaintAttrib, &identity,
                   outputRegion, &outputs[i],
                   PAINT_SCREEN_REGION_MASK))
               {
                  (*s->paintOutput)
                    (s, &defaultScreenPaintAttrib, &identity,
                    &outputs[i].region, &outputs[i],
                    PAINT_SCREEN_FULL_MASK);

                  XUnionRegion (tmpRegion, &outputs[i].region, tmpRegion);
               }
          }
     }
}

void
eventLoop(void)
{
   XEvent event;
   int timeDiff;
   struct timeval tv;
   CompDisplay *display = compDisplays;
   CompScreen *s;
   int time, timeToNextRedraw = 0;
   CompWindow *w;
   unsigned int damageMask, mask;

   tmpRegion = XCreateRegion ();
   outputRegion = XCreateRegion ();
   if (!tmpRegion || !outputRegion)
     {
        compLogMessage (display, "core", CompLogLevelFatal,
                        "Couldn't create temporary regions");
        return;
     }

   compAddWatchFd (ConnectionNumber (display->display), POLLIN, NULL, NULL);

   for (;; )
     {
        if (display->dirtyPluginList)
          updatePlugins (display);

        if (restartSignal || shutDown)
          {
             while (popPlugin ()) ;
             XSync (display->display, False);
             return;
          }

        while (XPending (display->display))
          {
             XNextEvent (display->display, &event);

             inHandleEvent = TRUE;

             (*display->handleEvent)(display, &event);

             inHandleEvent = FALSE;

             lastPointerX = pointerX;
             lastPointerY = pointerY;
          }

        if (replaceCurrentWm)
          {
             if (timeouts)
               {
                  if (timeouts->left > 0)
                    doPoll (timeouts->left);

                  gettimeofday (&tv, 0);

                  handleTimeouts (&tv);
               }
             else
               {
                  doPoll (1000);
               }
             continue;
          }

        for (s = display->screens; s; s = s->next)
          {
             if (s->damageMask)
               {
                  finishScreenDrawing (s);
               }
             else
               {
                  s->idle = TRUE;
               }
          }

        damageMask = 0;
        timeToNextRedraw = MAXSHORT;

        for (s = display->screens; s; s = s->next)
          {
             if (!s->damageMask)
               continue;

             if (!damageMask)
               {
                  gettimeofday (&tv, 0);
                  damageMask |= s->damageMask;
               }

             s->timeLeft = getTimeToNextRedraw (s, &tv, &s->lastRedraw, s->idle);
             if (s->timeLeft < timeToNextRedraw)
               timeToNextRedraw = s->timeLeft;
          }

        if (!damageMask)
          {
             if (timeouts)
               {
                  if (timeouts->left > 0)
                    doPoll (timeouts->left);

                  gettimeofday (&tv, 0);

                  handleTimeouts (&tv);
               }
             else
               {
                  doPoll (1000);
               }

             continue;
          }

        time = timeToNextRedraw;
        if (time)
          time = doPoll (time);

        if (time)
          continue;

        gettimeofday (&tv, 0);

        if (timeouts)
          handleTimeouts (&tv);

        for (s = display->screens; s; s = s->next)
          {
             if (!s->damageMask || s->timeLeft > timeToNextRedraw)
               continue;

             targetScreen = s;

             timeDiff = TIMEVALDIFF (&tv, &s->lastRedraw);

             /* handle clock rollback */
             if (timeDiff < 0)
               timeDiff = 0;

             makeScreenCurrent (s);

             if (s->slowAnimations)
               {
                  (*s->preparePaintScreen)
                    (s, s->idle ? 2 : (timeDiff * 2) / s->redrawTime);
               }
             else
               {
                  (*s->preparePaintScreen)(s, s->idle ? s->redrawTime : timeDiff);
               }

             /* substract top most overlay window region */
             if (s->overlayWindowCount)
               {
                  for (w = s->reverseWindows; w; w = w->prev)
                    {
                       if (w->destroyed || w->invisible)
                         continue;

                       if (!w->redirected)
                         XSubtractRegion (s->damage, w->region,
                                          s->damage);

                       break;
                    }

                  if (s->damageMask & COMP_SCREEN_DAMAGE_ALL_MASK)
                    {
                       s->damageMask &= ~COMP_SCREEN_DAMAGE_ALL_MASK;
                       s->damageMask |= COMP_SCREEN_DAMAGE_REGION_MASK;
                    }
               }

             if (s->damageMask & COMP_SCREEN_DAMAGE_REGION_MASK)
               {
                  XIntersectRegion (s->damage, &s->region, tmpRegion);

                  if (tmpRegion->numRects == 1 &&
                      tmpRegion->rects->x1 == 0 &&
                      tmpRegion->rects->y1 == 0 &&
                      tmpRegion->rects->x2 == s->width &&
                      tmpRegion->rects->y2 == s->height)
                    damageScreen (s);
               }

             EMPTY_REGION (s->damage);

             mask = s->damageMask;
             s->damageMask = 0;

             if (s->clearBuffers)
               {
                  if (mask & COMP_SCREEN_DAMAGE_ALL_MASK)
                    glClear (GL_COLOR_BUFFER_BIT);
               }

             (*s->paintScreen)(s, s->outputDev,
                               s->nOutputDev,
                               mask);

             targetScreen = NULL;
             targetOutput = &s->outputDev[0];

             waitForVideoSync (s);

             if (mask & COMP_SCREEN_DAMAGE_ALL_MASK)
               {
                  glXSwapBuffers (display->display, s->output);
               }
             else
               {
                  BoxPtr pBox;
                  int nBox, y;

                  pBox = tmpRegion->rects;
                  nBox = tmpRegion->numRects;

                  if (s->copySubBuffer)
                    {
                       while (nBox--)
                         {
                            y = s->height - pBox->y2;

                            (*s->copySubBuffer)
                              (display->display, s->output,
                              pBox->x1, y,
                              pBox->x2 - pBox->x1,
                              pBox->y2 - pBox->y1);

                            pBox++;
                         }
                    }
                  else
                    {
                       glEnable (GL_SCISSOR_TEST);
                       glDrawBuffer (GL_FRONT);

                       while (nBox--)
                         {
                            y = s->height - pBox->y2;

                            glBitmap (0, 0, 0, 0, pBox->x1 - s->rasterX,
                                      y - s->rasterY, NULL);

                            s->rasterX = pBox->x1;
                            s->rasterY = y;

                            glScissor (pBox->x1, y,
                                       pBox->x2 - pBox->x1,
                                       pBox->y2 - pBox->y1);

                            glCopyPixels (pBox->x1, y,
                                          pBox->x2 - pBox->x1,
                                          pBox->y2 - pBox->y1,
                                          GL_COLOR);

                            pBox++;
                         }

                       glDrawBuffer (GL_BACK);
                       glDisable (GL_SCISSOR_TEST);
                       glFlush ();
                    }
               }

             s->lastRedraw = tv;

             (*s->donePaintScreen)(s);

             /* remove destroyed windows */
             while (s->pendingDestroys)
               {
                  CompWindow *w;

                  for (w = s->windows; w; w = w->next)
                    {
                       if (w->destroyed)
                         {
                            addWindowDamage (w);
                            removeWindow (w);
                            break;
                         }
                    }

                  s->pendingDestroys--;
               }

             s->idle = FALSE;
          }
     }
}

static int errors = 0;

static int
errorHandler(Display     *dpy,
             XErrorEvent *e)
{
#ifdef DEBUG
   char str[128];
   char *name = 0;
   int o;
#endif

   errors++;

#ifdef DEBUG
   XGetErrorDatabaseText (dpy, "XlibMessage", "XError", "", str, 128);
   fprintf (stderr, "%s", str);

   o = e->error_code - compDisplays->damageError;
   switch (o) {
      case BadDamage:
        name = "BadDamage";
        break;

      default:
        break;
     }

   if (name)
     {
        fprintf (stderr, ": %s\n  ", name);
     }
   else
     {
        XGetErrorText (dpy, e->error_code, str, 128);
        fprintf (stderr, ": %s\n  ", str);
     }

   XGetErrorDatabaseText (dpy, "XlibMessage", "MajorCode", "%d", str, 128);
   fprintf (stderr, str, e->request_code);

   sprintf (str, "%d", e->request_code);
   XGetErrorDatabaseText (dpy, "XRequest", str, "", str, 128);
   if (strcmp (str, ""))
     fprintf (stderr, " (%s)", str);
   fprintf (stderr, "\n  ");

   XGetErrorDatabaseText (dpy, "XlibMessage", "MinorCode", "%d", str, 128);
   fprintf (stderr, str, e->minor_code);
   fprintf (stderr, "\n  ");

   XGetErrorDatabaseText (dpy, "XlibMessage", "ResourceID", "%d", str, 128);
   fprintf (stderr, str, e->resourceid);
   fprintf (stderr, "\n");

   /* abort (); */
#endif

   return 0;
}

int
compCheckForError(Display *dpy)
{
   int e;

   XSync (dpy, FALSE);

   e = errors;
   errors = 0;

   return e;
}

void
addScreenToDisplay(CompDisplay *display,
                   CompScreen  *s)
{
   CompScreen *prev;

   for (prev = display->screens; prev && prev->next; prev = prev->next) ;

   if (prev)
     prev->next = s;
   else
     display->screens = s;
}

Bool
addDisplay(char *name)
{
   CompDisplay *d;
   Display *dpy;
   int i;
   int compositeMajor, compositeMinor;
   int firstScreen, lastScreen;
   Time wmSnTimestamp = 0;

   d = &compDisplay;

   if (displayPrivateLen)
     {
        d->privates = malloc (displayPrivateLen * sizeof (CompPrivate));
        if (!d->privates)
          return FALSE;
     }
   else
     d->privates = 0;

   d->screens = NULL;

   d->screenPrivateIndices = 0;
   d->screenPrivateLen = 0;

   d->plugin.list.type = CompOptionTypeString;
   d->plugin.list.nValue = 0;
   d->plugin.list.value = 0;

   d->dirtyPluginList = TRUE;

   d->textureFilter = GL_LINEAR;
   d->below = None;

   d->activeWindow = 0;

   d->autoRaiseHandle = 0;
   d->autoRaiseWindow = None;

   d->logMessage = logMessage;

   d->display = dpy = XOpenDisplay (name);
   if (!d->display)
     {
        compLogMessage (d, "core", CompLogLevelFatal,
                        "Couldn't open display %s", XDisplayName (name));
        return FALSE;
     }

   if (!compInitDisplayOptionsFromMetadata
         (d, &coreMetadata, coreDisplayOptionInfo,
         d->opt, COMP_DISPLAY_OPTION_NUM))
     return FALSE;

   snprintf (d->displayString, 255, "DISPLAY=%s", DisplayString (dpy));

#ifdef DEBUG
   XSynchronize (dpy, TRUE);
#endif

   XSetErrorHandler (errorHandler);

   d->setDisplayOption = setDisplayOption;
   d->setDisplayOptionForPlugin = setDisplayOptionForPlugin;

   d->initPluginForDisplay = initPluginForDisplay;
   d->finiPluginForDisplay = finiPluginForDisplay;

   d->handleEvent = handleEvent;
   d->handleEcompEvent = handleEcompEvent;

   d->fileToImage = fileToImage;
   d->imageToFile = imageToFile;

   d->fileWatchAdded = fileWatchAdded;
   d->fileWatchRemoved = fileWatchRemoved;

   d->fileWatch = NULL;

   d->matchInitExp = matchInitExp;
   d->matchExpHandlerChanged = matchExpHandlerChanged;
   d->matchPropertyChanged = matchPropertyChanged;

   d->supportedAtom = XInternAtom (dpy, "_NET_SUPPORTED", 0);

   // nope, this is not a window manager anymore :)
   //d->supportingWmCheckAtom = XInternAtom (dpy, "_NET_SUPPORTING_WM_CHECK", 0);

   d->utf8StringAtom = XInternAtom (dpy, "UTF8_STRING", 0);

   d->wmNameAtom = XInternAtom (dpy, "_NET_WM_NAME", 0);

   d->winTypeAtom = XInternAtom (dpy, "_NET_WM_WINDOW_TYPE", 0);
   d->winTypeDesktopAtom = XInternAtom (dpy, "_NET_WM_WINDOW_TYPE_DESKTOP",
                                        0);
   d->winTypeDockAtom = XInternAtom (dpy, "_NET_WM_WINDOW_TYPE_DOCK", 0);
   d->winTypeToolbarAtom = XInternAtom (dpy, "_NET_WM_WINDOW_TYPE_TOOLBAR",
                                        0);
   d->winTypeMenuAtom = XInternAtom (dpy, "_NET_WM_WINDOW_TYPE_MENU", 0);
   d->winTypeUtilAtom = XInternAtom (dpy, "_NET_WM_WINDOW_TYPE_UTILITY",
                                     0);
   d->winTypeSplashAtom = XInternAtom (dpy, "_NET_WM_WINDOW_TYPE_SPLASH", 0);
   d->winTypeDialogAtom = XInternAtom (dpy, "_NET_WM_WINDOW_TYPE_DIALOG", 0);
   d->winTypeNormalAtom = XInternAtom (dpy, "_NET_WM_WINDOW_TYPE_NORMAL", 0);

   d->winTypeDropdownMenuAtom =
     XInternAtom (dpy, "_NET_WM_WINDOW_TYPE_DROPDOWN_MENU", 0);
   d->winTypePopupMenuAtom =
     XInternAtom (dpy, "_NET_WM_WINDOW_TYPE_POPUP_MENU", 0);
   d->winTypeTooltipAtom =
     XInternAtom (dpy, "_NET_WM_WINDOW_TYPE_TOOLTIP", 0);
   d->winTypeNotificationAtom =
     XInternAtom (dpy, "_NET_WM_WINDOW_TYPE_NOTIFICATION", 0);
   d->winTypeComboAtom =
     XInternAtom (dpy, "_NET_WM_WINDOW_TYPE_COMBO", 0);
   d->winTypeDndAtom =
     XInternAtom (dpy, "_NET_WM_WINDOW_TYPE_DND", 0);

   d->winOpacityAtom = XInternAtom (dpy, "_NET_WM_WINDOW_OPACITY", 0);
   d->winBrightnessAtom = XInternAtom (dpy, "_NET_WM_WINDOW_BRIGHTNESS", 0);
   d->winSaturationAtom = XInternAtom (dpy, "_NET_WM_WINDOW_SATURATION", 0);

   d->winActiveAtom = XInternAtom (dpy, "_NET_ACTIVE_WINDOW", 0);

   d->winDesktopAtom = XInternAtom (dpy, "_NET_WM_DESKTOP", 0);

   d->workareaAtom = XInternAtom (dpy, "_NET_WORKAREA", 0);

   d->desktopViewportAtom = XInternAtom (dpy, "_NET_DESKTOP_VIEWPORT", 0);
   d->desktopGeometryAtom = XInternAtom (dpy, "_NET_DESKTOP_GEOMETRY", 0);
   d->currentDesktopAtom = XInternAtom (dpy, "_NET_CURRENT_DESKTOP", 0);
   d->numberOfDesktopsAtom = XInternAtom (dpy, "_NET_NUMBER_OF_DESKTOPS", 0);

   d->winStateAtom = XInternAtom (dpy, "_NET_WM_STATE", 0);
   d->winStateModalAtom =
     XInternAtom (dpy, "_NET_WM_STATE_MODAL", 0);
   d->winStateStickyAtom =
     XInternAtom (dpy, "_NET_WM_STATE_STICKY", 0);
   d->winStateMaximizedVertAtom =
     XInternAtom (dpy, "_NET_WM_STATE_MAXIMIZED_VERT", 0);
   d->winStateMaximizedHorzAtom =
     XInternAtom (dpy, "_NET_WM_STATE_MAXIMIZED_HORZ", 0);
   d->winStateShadedAtom =
     XInternAtom (dpy, "_NET_WM_STATE_SHADED", 0);
   d->winStateSkipTaskbarAtom =
     XInternAtom (dpy, "_NET_WM_STATE_SKIP_TASKBAR", 0);
   d->winStateSkipPagerAtom =
     XInternAtom (dpy, "_NET_WM_STATE_SKIP_PAGER", 0);
   d->winStateHiddenAtom =
     XInternAtom (dpy, "_NET_WM_STATE_HIDDEN", 0);
   d->winStateFullscreenAtom =
     XInternAtom (dpy, "_NET_WM_STATE_FULLSCREEN", 0);
   d->winStateAboveAtom =
     XInternAtom (dpy, "_NET_WM_STATE_ABOVE", 0);
   d->winStateBelowAtom =
     XInternAtom (dpy, "_NET_WM_STATE_BELOW", 0);
   d->winStateDemandsAttentionAtom =
     XInternAtom (dpy, "_NET_WM_STATE_DEMANDS_ATTENTION", 0);
   d->winStateDisplayModalAtom =
     XInternAtom (dpy, "_NET_WM_STATE_DISPLAY_MODAL", 0);
   d->wmUserTimeAtom = XInternAtom (dpy, "_NET_WM_USER_TIME", 0);

   d->wmIconAtom = XInternAtom (dpy, "_NET_WM_ICON", 0);

   d->clientListAtom = XInternAtom (dpy, "_NET_CLIENT_LIST", 0);
   d->clientListStackingAtom =
     XInternAtom (dpy, "_NET_CLIENT_LIST_STACKING", 0);

   d->frameWindowAtom = XInternAtom (dpy, "_NET_FRAME_WINDOW", 0);

   d->wmStateAtom = XInternAtom (dpy, "WM_STATE", 0);
   d->wmChangeStateAtom = XInternAtom (dpy, "WM_CHANGE_STATE", 0);
   d->wmProtocolsAtom = XInternAtom (dpy, "WM_PROTOCOLS", 0);
   d->wmClientLeaderAtom = XInternAtom (dpy, "WM_CLIENT_LEADER", 0);

   d->wmDeleteWindowAtom = XInternAtom (dpy, "WM_DELETE_WINDOW", 0);
   d->wmTakeFocusAtom = XInternAtom (dpy, "WM_TAKE_FOCUS", 0);
   d->wmSyncRequestAtom = XInternAtom (dpy, "_NET_WM_SYNC_REQUEST", 0);

   d->wmSyncRequestCounterAtom =
     XInternAtom (dpy, "_NET_WM_SYNC_REQUEST_COUNTER", 0);

   d->closeWindowAtom = XInternAtom (dpy, "_NET_CLOSE_WINDOW", 0);
   d->wmMoveResizeAtom = XInternAtom (dpy, "_NET_WM_MOVERESIZE", 0);
   d->moveResizeWindowAtom = XInternAtom (dpy, "_NET_MOVERESIZE_WINDOW", 0);
   d->restackWindowAtom = XInternAtom (dpy, "_NET_RESTACK_WINDOW", 0);

   d->showingDesktopAtom = XInternAtom (dpy, "_NET_SHOWING_DESKTOP", 0);

   d->xBackgroundAtom[0] = XInternAtom (dpy, "_XSETROOT_ID", 0);
   d->xBackgroundAtom[1] = XInternAtom (dpy, "_XROOTPMAP_ID", 0);

   d->mwmHintsAtom = XInternAtom (dpy, "_MOTIF_WM_HINTS", 0);

   d->xdndAwareAtom = XInternAtom (dpy, "XdndAware", 0);
   d->xdndEnterAtom = XInternAtom (dpy, "XdndEnter", 0);
   d->xdndLeaveAtom = XInternAtom (dpy, "XdndLeave", 0);
   d->xdndPositionAtom = XInternAtom (dpy, "XdndPosition", 0);
   d->xdndStatusAtom = XInternAtom (dpy, "XdndStatus", 0);
   d->xdndDropAtom = XInternAtom (dpy, "XdndDrop", 0);

   d->managerAtom = XInternAtom (dpy, "MANAGER", 0);
   d->targetsAtom = XInternAtom (dpy, "TARGETS", 0);
   d->multipleAtom = XInternAtom (dpy, "MULTIPLE", 0);
   d->timestampAtom = XInternAtom (dpy, "TIMESTAMP", 0);
   d->versionAtom = XInternAtom (dpy, "VERSION", 0);
   d->atomPairAtom = XInternAtom (dpy, "ATOM_PAIR", 0);

   d->eManagedAtom = XInternAtom (dpy, "__ECOMORPH_WINDOW_MANAGED", 0);
   d->ecoPluginAtom = XInternAtom (dpy, "__ECOMORPH_PLUGIN", 0);

   if (!XQueryExtension
         (dpy, COMPOSITE_NAME, &d->compositeOpcode, &d->compositeEvent, &d->compositeError))
     {
        compLogMessage (d, "core", CompLogLevelFatal,
                        "No composite extension");
        return FALSE;
     }

   XCompositeQueryVersion (dpy, &compositeMajor, &compositeMinor);
   if (compositeMajor == 0 && compositeMinor < 2)
     {
        compLogMessage (d, "core", CompLogLevelFatal,
                        "Old composite extension");
        return FALSE;
     }

   if (!XDamageQueryExtension (dpy, &d->damageEvent, &d->damageError))
     {
        compLogMessage (d, "core", CompLogLevelFatal,
                        "No damage extension");
        return FALSE;
     }

   if (!XSyncQueryExtension (dpy, &d->syncEvent, &d->syncError))
     {
        compLogMessage (d, "core", CompLogLevelFatal,
                        "No sync extension");
        return FALSE;
     }

   d->randrExtension = XRRQueryExtension
       (dpy, &d->randrEvent, &d->randrError);

   d->shapeExtension = XShapeQueryExtension
       (dpy, &d->shapeEvent, &d->shapeError);

   d->xineramaExtension = XineramaQueryExtension
       (dpy, &d->xineramaEvent, &d->xineramaError);

   d->nScreenInfo = 0;
   if (d->xineramaExtension)
     d->screenInfo = XineramaQueryScreens (dpy, &d->nScreenInfo);
   else
     d->screenInfo = NULL;

   compDisplays = d;

   if (onlyCurrentScreen)
     {
        firstScreen = DefaultScreen (dpy);
        lastScreen = DefaultScreen (dpy);
     }
   else
     {
        firstScreen = 0;
        lastScreen = ScreenCount (dpy) - 1;
     }

   for (i = firstScreen; i <= lastScreen; i++)
     {
        Window newCmSnOwner = None;
        Atom cmSnAtom = 0;
        XSetWindowAttributes attr;
        Window currentCmSnOwner;
        char buf[128];

        sprintf (buf, "_NET_WM_CM_S%d", i);
        cmSnAtom = XInternAtom (dpy, buf, 0);

        currentCmSnOwner = XGetSelectionOwner (dpy, cmSnAtom);

        attr.override_redirect = TRUE;
        attr.event_mask = PropertyChangeMask;

        newCmSnOwner = XCreateWindow
            (dpy, XRootWindow (dpy, i), -100, -100, 1, 1, 0,
            CopyFromParent, CopyFromParent, CopyFromParent,
            CWOverrideRedirect | CWEventMask, &attr);

        XCompositeRedirectSubwindows(dpy, XRootWindow (dpy, i), CompositeRedirectManual);

        if (compCheckForError (dpy))
          {
             compLogMessage (d, "core", CompLogLevelError,
                             "Another composite manager is already "
                             "running on screen: %d", i);
             continue;
          }

        XSetSelectionOwner (dpy, cmSnAtom, newCmSnOwner, wmSnTimestamp);

        if (XGetSelectionOwner (dpy, cmSnAtom) != newCmSnOwner)
          {
             compLogMessage (d, "core", CompLogLevelError,
                             "Could not acquire compositing manager "
                             "selection on screen %d display \"%s\"",
                             i, DisplayString (dpy));
             continue;
          }

        XGrabServer (dpy);

        XSelectInput (dpy, XRootWindow (dpy, i),
                      SubstructureNotifyMask |
                      StructureNotifyMask |
                      PropertyChangeMask |
                      ExposureMask);

        if (compCheckForError (dpy))
          {
             compLogMessage (d, "core", CompLogLevelError,
                             "Another window manager is "
                             "already running on screen: %d", i);

             XUngrabServer (dpy);
             continue;
          }

        if (!addScreen (d, i))
          {
             compLogMessage (d, "core", CompLogLevelError,
                             "Failed to manage screen: %d", i);
          }

        lastPointerX = pointerX = 0;
        lastPointerY = pointerY = 0;

        XUngrabServer (dpy);
     }

   if (!d->screens)
     {
        compLogMessage (d, "core", CompLogLevelFatal,
                        "No manageable screens found on display %s",
                        XDisplayName (name));
        return FALSE;
     }

   return TRUE;
}

void
focusDefaultWindow(CompDisplay *d)
{
}

CompScreen *
findScreenAtDisplay(CompDisplay *d, Window root)
{
   CompScreen *s;

   for (s = d->screens; s; s = s->next)
     {
        if (s->root == root)
          return s;
     }

   return 0;
}

/* void
 * forEachWindowOnDisplay (CompDisplay	*display, ForEachWindowProc proc, void *closure)
 * {
 *  CompScreen *s;
 *
 *  for (s = display->screens; s; s = s->next)
 *    forEachWindowOnScreen (s, proc, closure);
 * } */

CompWindow *
findWindowAtDisplay(CompDisplay *d, Window id)
{
   CompScreen *s;
   CompWindow *w;

   for (s = d->screens; s; s = s->next)
     {
        w = findWindowAtScreen (s, id);
        if (w) return w;
     }

   return 0;
}

CompWindow *
findTopLevelWindowAtDisplay(CompDisplay *d, Window id)
{
   CompScreen *s;
   CompWindow *w;

   for (s = d->screens; s; s = s->next)
     {
        w = findTopLevelWindowAtScreen (s, id);
        if (w) return w;
     }

   return 0;
}

/*XXX hm this could conlfict with e17 */
void
warpPointer(CompScreen *s, int dx, int dy)
{
   return;

   CompDisplay *display = s->display;
   XEvent event;

   pointerX += dx;
   pointerY += dy;

   if (pointerX >= s->width)
     pointerX = s->width - 1;
   else if (pointerX < 0)
     pointerX = 0;

   if (pointerY >= s->height)
     pointerY = s->height - 1;
   else if (pointerY < 0)
     pointerY = 0;

   XWarpPointer (display->display,
                 None, s->root,
                 0, 0, 0, 0,
                 pointerX, pointerY);

   XSync (display->display, FALSE);

   while (XCheckMaskEvent (display->display,
                           LeaveWindowMask |
                           EnterWindowMask |
                           PointerMotionMask,
                           &event)) ;

   if (!inHandleEvent)
     {
        lastPointerX = pointerX;
        lastPointerY = pointerY;
     }
}

void
clearTargetOutput(CompDisplay *display, unsigned int mask)
{
   if (targetScreen)
     clearScreenOutput(targetScreen, targetOutput, mask);
}

#define HOME_IMAGEDIR ".ecomp/images"

Bool
readImageFromFile(CompDisplay *display, const char *name,
                  int *width, int *height, void **data)
{
   Bool status;
   int stride;

   status = (*display->fileToImage)
       (display, NULL, name, width, height, &stride, data);
   if (!status)
     {
        char *home;

        home = getenv ("HOME");
        if (home)
          {
             char *path;

             path = malloc (strlen (home) + strlen (HOME_IMAGEDIR) + 2);
             if (path)
               {
                  sprintf (path, "%s/%s", home, HOME_IMAGEDIR);
                  status = (*display->fileToImage)
                      (display, path, name, width, height, &stride, data);

                  free (path);

                  if (status)
                    return TRUE;
               }
          }

        status = (*display->fileToImage)
            (display, IMAGEDIR, name, width, height, &stride, data);
     }

   return status;
}

Bool
writeImageToFile(CompDisplay *display, const char *path, const char *name,
                 const char *format, int width, int height, void *data)
{
   return (*display->imageToFile)
            (display, path, name, format, width, height, width * 4, data);
}

Bool
fileToImage(CompDisplay *display, const char *path, const char *name,
            int *width, int *height, int *stride, void **data)
{
   return FALSE;
}

Bool
imageToFile(CompDisplay *display, const char *path, const char *name,
            const char *format, int width, int height, int stride, void *data)
{
   return FALSE;
}

CompFileWatchHandle
addFileWatch(CompDisplay *display, const char *path, int mask,
             FileWatchCallBackProc callBack, void *closure)
{
   CompFileWatch *fileWatch;

   fileWatch = malloc (sizeof (CompFileWatch));
   if (!fileWatch)
     return 0;

   fileWatch->path = strdup (path);
   fileWatch->mask = mask;
   fileWatch->callBack = callBack;
   fileWatch->closure = closure;
   fileWatch->handle = lastFileWatchHandle++;

   if (lastFileWatchHandle == MAXSHORT)
     lastFileWatchHandle = 1;

   fileWatch->next = display->fileWatch;
   display->fileWatch = fileWatch;

   (*display->fileWatchAdded)(display, fileWatch);

   return fileWatch->handle;
}

void
removeFileWatch(CompDisplay *display, CompFileWatchHandle handle)
{
   CompFileWatch *p = 0, *w;

   for (w = display->fileWatch; w; w = w->next)
     {
        if (w->handle == handle)
          break;

        p = w;
     }

   if (w)
     {
        if (p)
          p->next = w->next;
        else
          display->fileWatch = w->next;

        (*display->fileWatchRemoved)(display, w);

        if (w->path)
          free (w->path);

        free (w);
     }
}

void
fileWatchAdded(CompDisplay *display, CompFileWatch *fileWatch)
{
}

void
fileWatchRemoved(CompDisplay *display, CompFileWatch *fileWatch)
{
}

