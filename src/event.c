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
 * hacks by: Hannes Janetzek <hannes.janetzek@gmail.com>
 */
//
#define A(x)
#define D(x) //do { printf(__FILE__ ":%d:\t", __LINE__); printf x; fflush(stdout); } while(0)
#define C(x) //do { printf(__FILE__ ":%d:\t", __LINE__); printf x; fflush(stdout); } while(0)
#define E(x)

#include <stdlib.h>
#include <string.h>

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/extensions/shape.h>
#include <X11/extensions/Xrandr.h>
#include <X11/extensions/Xfixes.h>

#include <ecomp.h>

/* static Window xdndWindow = None;
 * static Window edgeWindow = None; */

static Bool initialDamageTimeout(void *closure);
static Bool restartWmTimeout(void *closure);

static Bool
restartWmTimeout(void *closure)
{
   replaceCurrentWm = FALSE;

   return FALSE;
}

static void
handleWindowDamageRect(CompWindow *w,
                       int         x,
                       int         y,
                       int         width,
                       int         height)
{
   REGION region;
   Bool initial = FALSE;

   if (!w->redirected || w->bindFailed)
     return;

   if (!w->damaged)
     {
        w->damaged = initial = TRUE;
        w->invisible = WINDOW_INVISIBLE (w);
        compAddTimeout(0, initialDamageTimeout, w);
        if (w->mapNum)
          updateWindowRegion (w);
     }

   region.extents.x1 = x;
   region.extents.y1 = y;
   region.extents.x2 = region.extents.x1 + width;
   region.extents.y2 = region.extents.y1 + height;

   if (!(*w->screen->damageWindowRect)(w, initial, &region.extents))
     {
        region.extents.x1 += w->attrib.x + w->attrib.border_width;
        region.extents.y1 += w->attrib.y + w->attrib.border_width;
        region.extents.x2 += w->attrib.x + w->attrib.border_width;
        region.extents.y2 += w->attrib.y + w->attrib.border_width;

        region.rects = &region.extents;
        region.numRects = region.size = 1;

        damageScreenRegion (w->screen, &region);
     }

   if (initial)
     damageWindowOutputExtents (w);
}

static Bool
initialDamageTimeout(void *closure)
{
   CompWindow *w = closure;

   handleWindowDamageRect (w,
                           w->attrib.x,
                           w->attrib.y,
                           w->attrib.width,
                           w->attrib.height);
   return FALSE;
}

void
handleEcompEvent(CompDisplay *d, char *pluginName, char *eventName, CompOption *option, int nOption)
{
}

/*TODO move to plugin */
static void
changeWindowOpacity(CompWindow *w, int direction)
{
   CompScreen *s = w->screen;
   int step, opacity;

   if (!w->clientId)
     return;

   step = (0xff * s->opt[COMP_SCREEN_OPTION_OPACITY_STEP].value.i) / 100;

   w->opacityFactor = w->opacityFactor + step * direction;
   if (w->opacityFactor > 0xff)
     {
        w->opacityFactor = 0xff;
     }
   else if (w->opacityFactor < step)
     {
        w->opacityFactor = step;
     }

   opacity = (w->opacity * w->opacityFactor) / 0xff;
   if (opacity != w->paint.opacity)
     {
        w->paint.opacity = opacity;
        addWindowDamage (w);
     }
}

void
handleEvent(CompDisplay *d, XEvent *event)
{
   CompScreen *s;
   CompWindow *w;

   switch (event->type) {
      case Expose:
        D(("0x%x : Expose event\n", (unsigned int)event->xexpose.window));
        for (s = d->screens; s; s = s->next)
          if (s->output == event->xexpose.window)
            break;

        if (s)
          {
             int more = event->xexpose.count + 1;

             if (s->nExpose == s->sizeExpose)
               {
                  if (s->exposeRects)
                    {
                       s->exposeRects = realloc (s->exposeRects,
                                                 (s->sizeExpose + more) *
                                                 sizeof (XRectangle));
                       s->sizeExpose += more;
                    }
                  else
                    {
                       s->exposeRects = malloc (more * sizeof (XRectangle));
                       s->sizeExpose = more;
                    }
               }

             s->exposeRects[s->nExpose].x = event->xexpose.x;
             s->exposeRects[s->nExpose].y = event->xexpose.y;
             s->exposeRects[s->nExpose].width = event->xexpose.width;
             s->exposeRects[s->nExpose].height = event->xexpose.height;
             s->nExpose++;

             if (event->xexpose.count == 0)
               {
                  REGION rect;

                  rect.rects = &rect.extents;
                  rect.numRects = rect.size = 1;

                  while (s->nExpose--)
                    {
                       rect.extents.x1 = s->exposeRects[s->nExpose].x;
                       rect.extents.y1 = s->exposeRects[s->nExpose].y;
                       rect.extents.x2 = rect.extents.x1 +
                         s->exposeRects[s->nExpose].width;
                       rect.extents.y2 = rect.extents.y1 +
                         s->exposeRects[s->nExpose].height;

                       damageScreenRegion (s, &rect);
                    }
                  s->nExpose = 0;
               }
          }
        break;

      case ConfigureNotify:
        C(("0x%x : ConfigureNotify event \n", (unsigned int)event->xconfigure.window));

        w = findWindowAtDisplay (d, event->xconfigure.window);
        if (w)
          {
             configureWindow (w, &event->xconfigure);
          }
        else
          {
             s = findScreenAtDisplay (d, event->xconfigure.window);
             if (s)
               configureScreen (s, &event->xconfigure);
          }
        break;

      case CreateNotify:
        C(("0x%x : CreateNotify event\n", (unsigned int)event->xcreatewindow.window));
        s = findScreenAtDisplay (d, event->xcreatewindow.parent);
        if (s)
          {
             /* The first time some client asks for the composite
              * overlay window, the X server creates it, which causes
              * an errorneous CreateNotify event.  We catch it and
              * ignore it. */
               if (s->overlay != event->xcreatewindow.window)
                 addWindow (s, event->xcreatewindow.window, getTopWindow (s));
          }
        break;

      case DestroyNotify:
        D(("0x%x : DestroyNotify event\n", (unsigned int)event->xdestroywindow.window));
        w = findWindowAtDisplay (d, event->xdestroywindow.window);
        if (w)
          {
             destroyWindow (w);
          }
        break;

      case MapNotify:
        w = findWindowAtDisplay (d, event->xmap.window);
        if (w)
          {
             C(("0x%x : MapNotify event\n", (unsigned int)event->xmap.window));
             mapWindow (w);
          }
        break;

      case UnmapNotify:
        w = findWindowAtDisplay (d, event->xunmap.window);
        if (w)
          {
             C(("0x%x : UnmapNotify\n", (unsigned int)event->xunmap.window));

             /* Normal -> Iconic */
             if (w->pendingUnmaps)
               {
                  w->pendingUnmaps--;
               }
             else /* X -> Withdrawn */
               {
     /* Iconic -> Withdrawn */
                   if (w->state & CompWindowStateHiddenMask)
                     {
                        w->minimized = FALSE;

                        changeWindowState (w, w->state & ~CompWindowStateHiddenMask);
                     }
               }

             unmapWindow (w);
          }
        break;

      case ReparentNotify:
        C(("0x%x : ReparentNotify\n", (unsigned int)event->xreparent.window));
        w = findWindowAtDisplay (d, event->xreparent.window);
        s = findScreenAtDisplay (d, event->xreparent.parent);
        if (s && !w)
          {
             addWindow (s, event->xreparent.window, getTopWindow (s));
          }
        else if (w)
          {
             /* This is the only case where a window is removed but not
                destroyed. We must remove our event mask and all passive
                grabs. */
               destroyWindow (w);
          }
        break;

      case CirculateNotify:
        w = findWindowAtDisplay (d, event->xcirculate.window);
        if (w)
          circulateWindow (w, &event->xcirculate);
        break;

      case PropertyNotify:

        w = findWindowAtDisplay (d, event->xproperty.window);
        if (!w) break;

        if (event->xproperty.atom == d->winStateAtom)
          {
             unsigned int state = getWindowState (d, w->id);
             if(state & CompWindowStateHiddenMask)
               {
                  w->clientMapped = 0;
               }
             if (state != w->state)
               {
                  w->state = state;
                  (*d->matchPropertyChanged)(d, w);
               }
          }
        else if (event->xproperty.atom == d->winTypeAtom)
          {
             unsigned int type;

             type = getWindowType (d, w->id);

             if (type != w->wmType)
               {
                  if (w->attrib.map_state == IsViewable)
                    {
                       if (w->type == CompWindowTypeDesktopMask)
                         w->screen->desktopWindowCount--;
                       else if (type == CompWindowTypeDesktopMask)
                         w->screen->desktopWindowCount++;
                    }

                  if (w->clientId && type == CompWindowTypeUnknownMask)
                    w->wmType = CompWindowTypeNormalMask;
                  else
                    w->wmType = type;

                  recalcWindowType (w);

                  if (w->type & CompWindowTypeDesktopMask)
                    {
                       w->paint.opacity = OPAQUE;
                    }

                  (*d->matchPropertyChanged)(d, w);
               }
          }
        else if (event->xproperty.atom == d->winOpacityAtom)
          {
             if ((w->type & CompWindowTypeDesktopMask) == 0)
               {
                  w->opacity = OPAQUE;
                  w->opacityPropSet =
                    readWindowProp32 (d, w->id,
                                      d->winOpacityAtom,
                                      &w->opacity);

                  updateWindowOpacity (w);
               }
          }
        else if (event->xproperty.atom == d->winBrightnessAtom)
          {
             GLushort brightness;

             brightness = getWindowProp32 (d, w->id, d->winBrightnessAtom, BRIGHT);

             if (brightness != w->brightness)
               {
                  w->brightness = brightness;
                  if (w->alive)
                    {
                       w->paint.brightness = w->brightness;
                       addWindowDamage (w);
                    }
               }
          }
        else if (event->xproperty.atom == d->winSaturationAtom)
          {
             if (w->screen->canDoSaturated)
               {
                  GLushort saturation;

                  saturation = getWindowProp32 (d, w->id, d->winSaturationAtom, COLOR);

                  if (saturation != w->saturation)
                    {
                       w->saturation = saturation;
                       if (w->alive)
                         {
                            w->paint.saturation = w->saturation;
                            addWindowDamage (w);
                         }
                    }
               }
          }
        else if (event->xproperty.atom == d->wmIconAtom)
          {
             freeWindowIcons (w);
          }
        else if (event->xproperty.atom == XA_WM_CLASS)
          {
             updateWindowClassHints (w);
          }
        break;

      case ClientMessage:
        if (event->xclient.message_type == d->eManagedAtom)
          {
             Window win = event->xclient.window; //data.l[0];
             unsigned int type = event->xclient.data.l[0];

             if (type == ECOMORPH_EVENT_RESTART) /* RESTART */
               {
                  unsigned int restart = event->xclient.data.l[2];
                  C(("event restart %d\n", restart));

                  replaceCurrentWm = restart;
                  if(replaceCurrentWm)
                    compAddTimeout(10000, restartWmTimeout, NULL);
                  break;
               }

             w = findWindowAtDisplay (d, win);
             if (w)
               {
                  if(type == ECOMORPH_EVENT_MAPPED)
                    {
     /*   unsigned int mapped = event->xclient.data.l[2];
      *   w->clientMapped = mapped;
      *   C(("______________map_event %p %d________________\n", w, mapped));
      *
      *   if(mapped)
      * {
      *    resizeWindow (w, w->serverX, w->serverY,
      *      w->serverWidth, w->serverHeight, 0);
      *
      *    handleWindowDamageRect
      *      (w, 0, 0, w->attrib.width, w->attrib.height);
      * } */
                         break;
                    }
                  else if(type == ECOMORPH_EVENT_DESK)
                    {
                       s = w->screen;

                       int dx = event->xclient.data.l[2];
                       int dy = event->xclient.data.l[3];

                       w->initialViewportX = dx;
                       w->initialViewportY = dy;

     /* window is grabbed and moved */
                       if(event->xclient.data.l[4]) break;

                       int x = MOD(w->attrib.x, s->width) + ((dx - s->x) * s->width);
                       int y = MOD(w->attrib.y, s->height) + ((dy - s->y) * s->height);

                       if (x == w->attrib.x && y == w->attrib.y) break;

                       int immediate = 1;
                       int old_x = w->attrib.x;
                       int old_y = w->attrib.y;

     /*if visible ?*/
                       addWindowDamage (w);

                       w->attrib.x = x;
                       w->attrib.y = y;

                       XOffsetRegion (w->region, x - old_x, y - old_y);

                       w->matrix = w->texture->matrix;
                       w->matrix.x0 -= (w->attrib.x * w->matrix.xx);
                       w->matrix.y0 -= (w->attrib.y * w->matrix.yy);

                       w->invisible = WINDOW_INVISIBLE (w);

                       (*s->windowMoveNotify)(w, x - old_x, y - old_y, immediate);

     /*if visible ?*/
                       addWindowDamage (w);

                       syncWindowPosition (w);

                       break;
                    }
                  else if (type == ECOMORPH_EVENT_FOCUS)
                    {
                       int focus = event->xclient.data.l[2];

                       if (focus)
                         d->activeWindow = w->id;
                       else if (w->id == d->activeWindow)
                         d->activeWindow = None;
                    }
               }
          }
        /*TODO move to plugin */
        else if (event->xclient.message_type == d->ecoPluginAtom)
          {
             /* Window win = event->xclient.data.; */
              if(event->xclient.data.l[1] != ECO_PLUGIN_OPACITY) break;
              unsigned int option2 = event->xclient.data.l[4];
              w = findWindowAtDisplay(d, d->activeWindow);
              if (w)
                {
                   if (option2 == ECO_ACT_OPT_CYCLE_NEXT)
                     changeWindowOpacity (w, 1);
                   else if (option2 == ECO_ACT_OPT_CYCLE_PREV)
                     changeWindowOpacity (w, -1);
                }
          }
        else if (event->xclient.message_type == d->winActiveAtom)
          {
             w = findWindowAtDisplay (d, event->xclient.window);
             C(("win active event %p %d\n", w, (int)event->xclient.data.l[0]));
             if (w)
               {
                  if (event->xclient.data.l[0] == 0) //||
                    {
                       activateWindow (w);
                    }
               }
          }
        else if (event->xclient.message_type == d->winOpacityAtom)
          {
             w = findWindowAtDisplay (d, event->xclient.window);
             if (w && (w->type & CompWindowTypeDesktopMask) == 0)
               {
                  GLushort opacity = event->xclient.data.l[0] >> 16;

                  setWindowProp32 (d, w->id, d->winOpacityAtom, opacity);
               }
          }
        else if (event->xclient.message_type == d->winBrightnessAtom)
          {
             w = findWindowAtDisplay (d, event->xclient.window);
             if (w)
               {
                  GLushort brightness = event->xclient.data.l[0] >> 16;

                  setWindowProp32 (d, w->id, d->winBrightnessAtom, brightness);
               }
          }
        else if (event->xclient.message_type == d->winSaturationAtom)
          {
             w = findWindowAtDisplay (d, event->xclient.window);
             if (w && w->screen->canDoSaturated)
               {
                  GLushort saturation = event->xclient.data.l[0] >> 16;

                  setWindowProp32 (d, w->id, d->winSaturationAtom, saturation);
               }
          }
        else if (event->xclient.message_type == d->desktopGeometryAtom)
          {
             s = findScreenAtDisplay (d, event->xclient.window);
             if (s)
               {
                  int v, h;
                  CompOptionValue value;

                  h = event->xclient.data.l[0] / s->width;
                  v = event->xclient.data.l[1] / s->height;

                  s->vsize = v;
                  s->hsize = h;

                  value.i = v;
                  s->setScreenOption(s, "vsize", &value);

                  value.i = h;
                  s->setScreenOption(s, "hsize", &value);

                  C(("got desktop geometry %d %d\n", v, h));
               }
          }
        break;

      default:
        if (event->type == d->damageEvent + XDamageNotify)
          {
             XDamageNotifyEvent *de = (XDamageNotifyEvent *)event;

             if (lastDamagedWindow && de->drawable == lastDamagedWindow->id)
               {
                  w = lastDamagedWindow;
               }
             else
               {
                  w = findWindowAtDisplay (d, de->drawable);
                  if (w)
                    lastDamagedWindow = w;
               }

             if (w) // && (!w->clientId || w->clientMapped))
               {
                  w->texture->oldMipmaps = TRUE;

                  {
                     handleWindowDamageRect (w,
                                             de->area.x,
                                             de->area.y,
                                             de->area.width,
                                             de->area.height);
                  }
               }
          }
        else if (d->shapeExtension &&
                 event->type == d->shapeEvent + ShapeNotify)
          {
             w = findWindowAtDisplay (d, ((XShapeEvent *)event)->window);
             if (w)
               {
                  if (w->mapNum)
                    {
                       addWindowDamage (w);
                       updateWindowRegion (w);
                       addWindowDamage (w);
                    }
               }
          }
        else if (d->randrExtension &&
                 event->type == d->randrEvent + RRScreenChangeNotify)
          {
             XRRScreenChangeNotifyEvent *rre;

             rre = (XRRScreenChangeNotifyEvent *)event;

             s = findScreenAtDisplay (d, rre->root);
             if (s)
               detectRefreshRateOfScreen (s);
          }
        break;
     }
}

