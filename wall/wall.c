/**
 *
 * Ecomp wall plugin
 *
 * wall.c
 *
 * Copyright (c) 2006 Robert Carr <racarr@beryl-project.org>
 *
 * Authors:
 * Robert Carr <racarr@beryl-project.org>
 * Dennis Kasprzyk <onestone@beryl-project.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 **/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <sys/time.h>

#include <ecomp.h>
#include "wall_options.h"

#include <GL/glu.h>

#include <cairo-xlib-xrender.h>
#include <cairo.h>

#define PI                     3.14159265359f
#define VIEWPORT_SWITCHER_SIZE 70
#define ARROW_SIZE             33

#define WIN_X(w) ((w)->attrib.x - (w)->input.left)
#define WIN_Y(w) ((w)->attrib.y - (w)->input.top)
#define WIN_W(w) ((w)->width + (w)->input.left + (w)->input.right)
#define WIN_H(w) ((w)->height + (w)->input.top + (w)->input.bottom)

#define getColorRGBA(name, _display)             \
  r = wallGet##name##Red(_display) / 65535.0f;   \
  g = wallGet##name##Green(_display) / 65535.0f; \
  b = wallGet##name##Blue(_display) / 65535.0f;  \
  a = wallGet##name##Alpha(_display) / 65535.0f

static int displayPrivateIndex;

/* Enums */
typedef enum
{
   Up = 0,
   Left,
   Down,
   Right
} Direction;

typedef struct _WallCairoContext
{
   Pixmap           pixmap;
   CompTexture      texture;

   cairo_surface_t *surface;
   cairo_t         *cr;

   int              width;
   int              height;
} WallCairoContext;

typedef struct _WallDisplay
{
   int             screenPrivateIndex;

   HandleEventProc handleEvent;
} WallDisplay;

typedef struct _WallScreen
{
   DonePaintScreenProc        donePaintScreen;
   PaintOutputProc            paintOutput;
   PaintScreenProc            paintScreen;
   PreparePaintScreenProc     preparePaintScreen;
   PaintTransformedOutputProc paintTransformedOutput;
   PaintWindowProc            paintWindow;
   SetScreenOptionProc        setScreenOption;

   Bool                       moving; /* Used to track miniview movement */

   float                      curPosX;
   float                      curPosY;
   int                        gotoX;
   int                        gotoY;
   int                        fromX;
   int                        fromY;

   int                        boxTimeout;
   int                        boxOutputDevice;

   int                        timer;

   Window                     moveWindow;

   Bool                       miniScreen;
   WindowPaintAttrib          mSAttribs;
   float                      mSzCamera;

   float                      firstViewportX;
   float                      firstViewportY;
   float                      viewportWidth;
   float                      viewportHeight;
   float                      viewportBorder;

   int                        moveWindowX;
   int                        moveWindowY;

   WallCairoContext           switcherContext;
   WallCairoContext           thumbContext;
   WallCairoContext           highlightContext;
   WallCairoContext           arrowContext;
} WallScreen;

typedef struct _WallWindow
{
} WallWindow;

/* Helpers */
#define GET_WALL_DISPLAY(d) \
  ((WallDisplay *)(d)->privates[displayPrivateIndex].ptr)
#define WALL_DISPLAY(d) \
  WallDisplay * wd = GET_WALL_DISPLAY(d);

#define GET_WALL_SCREEN(s, wd) \
  ((WallScreen *)(s)->privates[(wd)->screenPrivateIndex].ptr)
#define WALL_SCREEN(s) \
  WallScreen * ws = GET_WALL_SCREEN(s, GET_WALL_DISPLAY(s->display))

#define GET_SCREEN                                     \
  CompScreen * s;                                      \
  Window xid;                                          \
  xid = getIntOptionNamed(option, nOption, "root", 0); \
  s = findScreenAtDisplay(d, xid);                     \
  if (!s)                                              \
    return FALSE;

#define sigmoid(x)         (1.0f / (1.0f + exp (-5.5f * 2 * ((x) - 0.5))))
#define sigmoidProgress(x) ((sigmoid (x) - sigmoid (0)) / \
                            (sigmoid (1) - sigmoid (0)))

static void
wallClearCairoLayer(cairo_t *cr)
{
   cairo_save (cr);
   cairo_set_operator (cr, CAIRO_OPERATOR_CLEAR);
   cairo_paint (cr);
   cairo_restore (cr);
   cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
   cairo_save (cr);
}

static void
wallDrawSwitcherBackground(CompScreen *s)
{
   cairo_t *cr;
   cairo_pattern_t *pattern;
   float border = 10.0f, outline = 2.0f;
   float width, height, radius;
   float r, g, b, a;
   int viewportWidth, viewportHeight;
   int i, j;

   WALL_SCREEN (s);

   cr = ws->switcherContext.cr;
   wallClearCairoLayer (cr);

   width = (float)ws->switcherContext.width - outline;
   height = (float)ws->switcherContext.height - outline;

   cairo_translate (cr, outline / 2.0f, outline / 2.0f);

   /* set the pattern for the switcher's background */
   pattern = cairo_pattern_create_linear (0, 0, width, height);
   getColorRGBA (BackgroundGradientBaseColor, s->display);
   cairo_pattern_add_color_stop_rgba (pattern, 0.00f, r, g, b, a);
   getColorRGBA (BackgroundGradientHighlightColor, s->display);
   cairo_pattern_add_color_stop_rgba (pattern, 0.65f, r, g, b, a);
   getColorRGBA (BackgroundGradientShadowColor, s->display);
   cairo_pattern_add_color_stop_rgba (pattern, 0.85f, r, g, b, a);
   cairo_set_source (cr, pattern);

   /* draw the border's shape */
   radius = (float)wallGetEdgeRadius (s->display);
   if (radius)
     {
        cairo_arc (cr, radius, radius, radius, PI, 1.5f * PI);
        cairo_arc (cr, radius + width - 2 * radius,
                   radius, radius, 1.5f * PI, 2.0 * PI);
        cairo_arc (cr, width - radius, height - radius, radius, 0, PI / 2.0f);
        cairo_arc (cr, radius, height - radius, radius, PI / 2.0f, PI);
     }
   else
     cairo_rectangle (cr, 0, 0, width, height);

   cairo_close_path (cr);

   /* apply pattern to background... */
   cairo_fill_preserve (cr);

   /* ... and draw an outline */
   cairo_set_line_width (cr, outline);
   getColorRGBA (OutlineColor, s->display);
   cairo_set_source_rgba (cr, r, g, b, a);
   cairo_stroke (cr);

   cairo_pattern_destroy (pattern);

   viewportWidth = floor ((width - border * (s->hsize + 1)) / s->hsize -
                          outline / 2.0f);
   viewportHeight = floor ((height - border * (s->vsize + 1)) / s->vsize -
                           outline / 2.0f);

   ws->viewportWidth = viewportWidth;
   ws->viewportHeight = viewportHeight;
   ws->viewportBorder = border;

   cairo_translate (cr, border, border);

   for (i = 0; i < s->hsize; i++)
     {
        for (j = 0; j < s->vsize; j++)
          {
             float vpX, vpY;

             vpX = i * (viewportWidth + border + outline / 2.0f);
             vpY = j * (viewportHeight + border + outline / 2.0f);

             /* this cuts a hole into our background */
             cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
             cairo_set_source_rgba (cr, 0.0, 0.0, 0.0, 1.0);
             cairo_rectangle (cr, vpX, vpY, viewportWidth, viewportHeight);

             cairo_fill_preserve (cr);
             cairo_set_operator (cr, CAIRO_OPERATOR_XOR);
             cairo_fill (cr);
          }
     }

   cairo_restore (cr);
}

static void
wallDrawThumb(CompScreen *s)
{
   cairo_t *cr;
   cairo_pattern_t *pattern;
   float r, g, b, a;
   float border = 10.0f, outline = 2.0f;
   float width, height;

   WALL_SCREEN(s);

   cr = ws->thumbContext.cr;
   wallClearCairoLayer (cr);

   width = (float)ws->thumbContext.width;
   height = (float)ws->thumbContext.height;

   ws->viewportWidth = width;
   ws->viewportHeight = height;
   ws->viewportBorder = border;

   width -= outline;
   height -= outline;

   cairo_translate (cr, outline / 2.0f, outline / 2.0f);

   pattern = cairo_pattern_create_linear (0, 0, width, height);
   getColorRGBA (ThumbGradientBaseColor, s->display);
   cairo_pattern_add_color_stop_rgba (pattern, 0.0f, r, g, b, a);
   getColorRGBA (ThumbGradientHighlightColor, s->display);
   cairo_pattern_add_color_stop_rgba (pattern, 1.0f, r, g, b, a);

   /* apply the pattern for thumb background */
   cairo_set_source (cr, pattern);
   cairo_rectangle (cr, 0, 0, width, height);
   cairo_fill_preserve (cr);

   cairo_set_line_width (cr, outline);
   getColorRGBA (OutlineColor, s->display);
   cairo_set_source_rgba (cr, r, g, b, a);
   cairo_stroke (cr);

   cairo_pattern_destroy (pattern);

   cairo_restore (cr);
}

static void
wallDrawHighlight(CompScreen *s)
{
   cairo_t *cr;
   cairo_pattern_t *pattern;
   float width, height;
   float r, g, b, a;
   float outline = 2.0f;

   WALL_SCREEN(s);

   cr = ws->highlightContext.cr;
   wallClearCairoLayer (cr);

   width = (float)ws->highlightContext.width - outline;
   height = (float)ws->highlightContext.height - outline;

   cairo_translate (cr, outline / 2.0f, outline / 2.0f);

   pattern = cairo_pattern_create_linear (0, 0, width, height);
   getColorRGBA (ThumbHighlightGradientBaseColor, s->display);
   cairo_pattern_add_color_stop_rgba (pattern, 0.0f, r, g, b, a);
   getColorRGBA (ThumbHighlightGradientShadowColor, s->display);
   cairo_pattern_add_color_stop_rgba (pattern, 1.0f, r, g, b, a);

   /* apply the pattern for thumb background */
   cairo_set_source (cr, pattern);
   cairo_rectangle (cr, 0, 0, width, height);
   cairo_fill_preserve (cr);

   cairo_set_line_width (cr, outline);
   getColorRGBA (OutlineColor, s->display);
   cairo_set_source_rgba (cr, r, g, b, a);
   cairo_stroke (cr);

   cairo_pattern_destroy (pattern);

   cairo_restore (cr);
}

static void
wallDrawArrow(CompScreen *s)
{
   cairo_t *cr;
   float outline = 2.0f;
   float r, g, b, a;

   WALL_SCREEN(s);

   cr = ws->arrowContext.cr;
   wallClearCairoLayer (cr);

   cairo_translate (cr, outline / 2.0f, outline / 2.0f);

   /* apply the pattern for thumb background */
   cairo_set_line_width (cr, outline);

   /* draw top part of the arrow */
   getColorRGBA (ArrowBaseColor, s->display);
   cairo_set_source_rgba (cr, r, g, b, a);
   cairo_move_to (cr, 15, 0);
   cairo_line_to (cr, 30, 30);
   cairo_line_to (cr, 15, 24.5);
   cairo_line_to (cr, 15, 0);
   cairo_fill (cr);

   /* draw bottom part of the arrow */
   getColorRGBA (ArrowShadowColor, s->display);
   cairo_set_source_rgba (cr, r, g, b, a);
   cairo_move_to (cr, 15, 0);
   cairo_line_to (cr, 0, 30);
   cairo_line_to (cr, 15, 24.5);
   cairo_line_to (cr, 15, 0);
   cairo_fill (cr);

   /* draw the arrow outline */
   getColorRGBA (OutlineColor, s->display);
   cairo_set_source_rgba (cr, r, g, b, a);
   cairo_move_to (cr, 15, 0);
   cairo_line_to (cr, 30, 30);
   cairo_line_to (cr, 15, 24.5);
   cairo_line_to (cr, 0, 30);
   cairo_line_to (cr, 15, 0);
   cairo_stroke (cr);

   cairo_restore (cr);
}

static void
wallSetupCairoContext(CompScreen       *s,
                      WallCairoContext *context)
{
   XRenderPictFormat *format;
   Screen *screen;
   int width, height;

   screen = ScreenOfDisplay (s->display->display, s->screenNum);

   width = context->width;
   height = context->height;

   initTexture (s, &context->texture);

   format = XRenderFindStandardFormat (s->display->display,
                                       PictStandardARGB32);

   context->pixmap = XCreatePixmap (s->display->display, s->root,
                                    width, height, 32);

   if (!bindPixmapToTexture(s, &context->texture, context->pixmap,
                            width, height, 32))
     {
        compLogMessage (s->display, "wall", CompLogLevelError,
                        "Couldn't create cairo context for switcher");
     }

   context->surface =
     cairo_xlib_surface_create_with_xrender_format (s->display->display,
                                                    context->pixmap,
                                                    screen, format,
                                                    width, height);

   context->cr = cairo_create (context->surface);
   wallClearCairoLayer (context->cr);
}

static void
wallDestroyCairoContext(CompScreen       *s,
                        WallCairoContext *context)
{
   if (context->cr)
     cairo_destroy (context->cr);

   if (context->surface)
     cairo_surface_destroy (context->surface);

   finiTexture (s, &context->texture);

   if (context->pixmap)
     XFreePixmap (s->display->display, context->pixmap);
}

static Bool
wallCheckDestination(CompScreen *s,
                     int         destX,
                     int         destY)
{
   if (s->x - destX < 0)
     return FALSE;

   if (s->x - destX >= s->hsize)
     return FALSE;

   if (s->y - destY >= s->vsize)
     return FALSE;

   if (s->y - destY < 0)
     return FALSE;

   return TRUE;
}

static void
wallReleaseMoveWindow(CompScreen *s)
{
   CompWindow *w;
   WALL_SCREEN (s);

   w = findWindowAtScreen (s, ws->moveWindow);
   /* if (w)
    *   syncWindowPosition (w); */
   ws->moveWindow = 0;
}

static void
wallComputeTranslation(CompScreen *s,
                       float      *x,
                       float      *y)
{
   float dx, dy, elapsed;

   WALL_SCREEN (s);

   elapsed = 1 - (ws->timer / (wallGetSlideDuration (s->display) * 1000));

   if (elapsed < 0.0)
     elapsed = 0.0;
   if (elapsed > 1.0)
     elapsed = 1.0;

   dx = (ws->gotoX - ws->curPosX) * elapsed + ws->curPosX;
   dy = (ws->gotoY - ws->curPosY) * elapsed + ws->curPosY;

   *x = dx;
   *y = dy;
}

static Bool
wallMoveViewport(CompScreen *s,
                 int         x,
                 int         y,
                 Window      moveWindow)
{
   WALL_SCREEN (s);

   CompWindow *w = NULL;

   if (!x && !y)
     return FALSE;

   if (wallCheckDestination (s, x, y))
     {
        if (ws->moveWindow != moveWindow)
          {
             wallReleaseMoveWindow (s);
             w = findWindowAtScreen (s, moveWindow);
             if (w)
               {
                  if (!(w->state & CompWindowStateStickyMask))
                    {
                       ws->moveWindow = w->id;
                       ws->moveWindowX = w->attrib.x;
                       ws->moveWindowY = w->attrib.y;
                       raiseWindow (w);
                    }
               }
          }

        ws->fromX = s->x;
        ws->fromY = s->y;

        if (!ws->moving)
          {
             ws->curPosX = s->x;
             ws->curPosY = s->y;
          }
        ws->gotoX = s->x - x;
        ws->gotoY = s->y - y;

        if (w) w->placed = FALSE;
        moveScreenViewport (s, x, y, TRUE);
        if (w) w->placed = TRUE;

        ws->moving = TRUE;
        ws->boxOutputDevice = s->currentOutputDev;
     }
   if (ws->moving)
     {
        if (wallGetShowSwitcher (s->display))
          ws->boxTimeout = wallGetPreviewTimeout (s->display) * 1000;
        else
          ws->boxTimeout = 0;

        if (otherScreenGrabExist (s, "move", "scale", "group-drag", "wall", 0))
          {
             ws->boxTimeout = 0;
             ws->moving = FALSE;
          }

        ws->timer = wallGetSlideDuration (s->display) * 1000;
     }

   damageScreen (s);

   return ws->moving;
}

static void
wallCheckAmount(CompScreen *s,
                int         dx,
                int         dy,
                int        *amountX,
                int        *amountY)
{
   *amountX = -dx;
   *amountY = -dy;

   if ((s->x + dx) < 0)
     *amountX = -(s->hsize + dx);
   else if ((s->x + dx) >= s->hsize)
     *amountX = s->hsize - dx;

   if ((s->y + dy) < 0)
     *amountY = -(s->vsize + dy);
   else if ((s->y + dy) >= s->vsize)
     *amountY = s->vsize - dy;
}

static void
wallHandleEvent(CompDisplay *d,
                XEvent      *event)
{
   WALL_DISPLAY (d);

   switch (event->type)
     {
      case ClientMessage:
        if (event->xclient.message_type == d->desktopViewportAtom)
          {
             int dx, dy, toX, toY;
             int amountX = 0, amountY = 0;
             CompScreen *s;
             Window win;

             if (event->xclient.data.l[0]) break;

             s = findScreenAtDisplay (d, event->xclient.window);
             if (!s)
               break;

             if (otherScreenGrabExist (s, "switcher", "scale", "move", 0))
               break;

             toX = event->xclient.data.l[1] / s->width;
             dx = toX - s->x;

             toY = event->xclient.data.l[2] / s->height;
             dy = toY - s->y;

             if (!dx && !dy)
               break;

             wallCheckAmount(s, dx, dy, &amountX, &amountY);

             win = event->xclient.data.l[3];
             unsigned int moveType = event->xclient.data.l[4];

             if(moveType == 0)
               {
                  wallMoveViewport (s, amountX, amountY, None);
               }
             else if(moveType == 1)  /* window grabbed by mouse */
               {
                  wallMoveViewport (s, amountX, amountY, win);
               }
             else if(moveType == 2)  /* move with window to desk (keybinding) )*/
               {
                  wallMoveViewport (s, amountX, amountY, win);
               }
          }
        break;
     }

   UNWRAP (wd, d, handleEvent);
   (*d->handleEvent)(d, event);
   WRAP (wd, d, handleEvent, wallHandleEvent);
}

static inline void
wallDrawQuad(CompMatrix *matrix, BOX *box)
{
   glTexCoord2f (COMP_TEX_COORD_X (matrix, box->x1),
                 COMP_TEX_COORD_Y (matrix, box->y2));
   glVertex2i (box->x1, box->y2);
   glTexCoord2f (COMP_TEX_COORD_X (matrix, box->x2),
                 COMP_TEX_COORD_Y (matrix, box->y2));
   glVertex2i (box->x2, box->y2);
   glTexCoord2f (COMP_TEX_COORD_X (matrix, box->x2),
                 COMP_TEX_COORD_Y (matrix, box->y1));
   glVertex2i (box->x2, box->y1);
   glTexCoord2f (COMP_TEX_COORD_X (matrix, box->x1),
                 COMP_TEX_COORD_Y (matrix, box->y1));
   glVertex2i (box->x1, box->y1);
}

static void
wallDrawCairoTextureOnScreen(CompScreen *s)
{
   float centerX, centerY;
   float width, height;
   float topLeftX, topLeftY;
   float border = 10.0f;
   int i, j;
   CompMatrix matrix;
   BOX box;

   WALL_SCREEN(s);

   glDisableClientState (GL_TEXTURE_COORD_ARRAY);
   glEnable (GL_BLEND);

   centerX = s->outputDev[ws->boxOutputDevice].region.extents.x1 +
     (s->outputDev[ws->boxOutputDevice].width / 2.0f);
   centerY = s->outputDev[ws->boxOutputDevice].region.extents.y1 +
     (s->outputDev[ws->boxOutputDevice].height / 2.0f);

   width = (float)ws->switcherContext.width;
   height = (float)ws->switcherContext.height;

   topLeftX = centerX - floor (width / 2.0f);
   topLeftY = centerY - floor (height / 2.0f);

   ws->firstViewportX = topLeftX + border;
   ws->firstViewportY = topLeftY + border;

   if (!ws->moving)
     {
        double left;
        left = (float)ws->boxTimeout /
          (wallGetPreviewTimeout (s->display) * 1000.0f);

        if (left < 0)
          left = 0.0f;
        else if (left > 0.5)
          left = 1.0f;
        else
          left = 2 * left;

        screenTexEnvMode (s, GL_MODULATE);

        glColor4f (left, left, left, left);
        glTranslatef (0.0f, 0.0f, -(1 - left));

        ws->mSzCamera = -(1 - left);
     }
   else
     ws->mSzCamera = 0.0f;

   /* draw background */

   matrix = ws->switcherContext.texture.matrix;
   matrix.x0 -= topLeftX * matrix.xx;
   matrix.y0 -= topLeftY * matrix.yy;

   box.x1 = topLeftX;
   box.x2 = box.x1 + width;
   box.y1 = topLeftY;
   box.y2 = box.y1 + height;

   enableTexture (s, &ws->switcherContext.texture, COMP_TEXTURE_FILTER_FAST);
   glBegin (GL_QUADS);
   wallDrawQuad (&matrix, &box);
   glEnd ();
   disableTexture (s, &ws->switcherContext.texture);

   /* draw thumb */
   width = (float)ws->thumbContext.width;
   height = (float)ws->thumbContext.height;

   enableTexture (s, &ws->thumbContext.texture, COMP_TEXTURE_FILTER_FAST);
   glBegin (GL_QUADS);
   for (i = 0; i < s->hsize; i++)
     {
        for (j = 0; j < s->vsize; j++)
          {
             if (i == ws->gotoX && j == ws->gotoY && ws->moving)
               continue;

             box.x1 = i * (width + border);
             box.x1 += topLeftX + border;
             box.x2 = box.x1 + width;
             box.y1 = j * (height + border);
             box.y1 += topLeftY + border;
             box.y2 = box.y1 + height;

             matrix = ws->thumbContext.texture.matrix;
             matrix.x0 -= box.x1 * matrix.xx;
             matrix.y0 -= box.y1 * matrix.yy;

             wallDrawQuad (&matrix, &box);
          }
     }
   glEnd ();
   disableTexture (s, &ws->thumbContext.texture);

   if (ws->moving)
     {
        /* draw highlight */
         float angle = 0.0f;
         float dx, dy;
         int aW, aH;

         box.x1 = s->x * (width + border) + topLeftX + border;
         box.x2 = box.x1 + width;
         box.y1 = s->y * (height + border) + topLeftY + border;
         box.y2 = box.y1 + height;

         matrix = ws->highlightContext.texture.matrix;
         matrix.x0 -= box.x1 * matrix.xx;
         matrix.y0 -= box.y1 * matrix.yy;

         enableTexture (s, &ws->highlightContext.texture,
                        COMP_TEXTURE_FILTER_FAST);
         glBegin (GL_QUADS);
         wallDrawQuad (&matrix, &box);
         glEnd ();
         disableTexture (s, &ws->highlightContext.texture);

         /* draw arrow */
         enableTexture (s, &ws->arrowContext.texture, COMP_TEXTURE_FILTER_GOOD);

         dx = ws->gotoX - ws->curPosX;
         dy = ws->gotoY - ws->curPosY;

         if (dx > 0 && dy == 0)
           angle = 90.0f;
         else if (dx < 0 && dy == 0)
           angle = 270.0f;
         else if (dy > 0)
           {
              angle = 180.0f;

              if (dx < 0)
                angle += 45.0f;
              else if (dx > 0)
                angle -= 45.0f;
           }
         else if (dy < 0)
           {
              if (dx < 0)
                angle = -45.0f;
              else if (dx > 0)
                angle = 45.0f;
           }

         aW = ws->arrowContext.width;
         aH = ws->arrowContext.height;

         /* if we have a viewport preview we just paint the
            arrow outside the switcher */
         if (wallGetMiniscreen (s->display))
           {
              width = (float)ws->switcherContext.width;
              height = (float)ws->switcherContext.height;

              switch ((int)angle)
                {
     /* top left */
                  case -45:
                    box.x1 = topLeftX - aW - border;
                    box.y1 = topLeftY - aH - border;
                    break;

     /* up */
                  case 0:
                    box.x1 = topLeftX + width / 2.0f - aW / 2.0f;
                    box.y1 = topLeftY - aH - border;
                    break;

     /* top right */
                  case 45:
                    box.x1 = topLeftX + width + border;
                    box.y1 = topLeftY - aH - border;
                    break;

     /* right */
                  case 90:
                    box.x1 = topLeftX + width + border;
                    box.y1 = topLeftY + height / 2.0f - aH / 2.0f;
                    break;

     /* bottom right */
                  case 135:
                    box.x1 = topLeftX + width + border;
                    box.y1 = topLeftY + height + border;
                    break;

     /* down */
                  case 180:
                    box.x1 = topLeftX + width / 2.0f - aW / 2.0f;
                    box.y1 = topLeftY + height + border;
                    break;

     /* bottom left */
                  case 225:
                    box.x1 = topLeftX - aW - border;
                    box.y1 = topLeftY + height + border;
                    break;

     /* left */
                  case 270:
                    box.x1 = topLeftX - aW - border;
                    box.y1 = topLeftY + height / 2.0f - aH / 2.0f;
                    break;

                  default:
                    break;
                }
           }
         else
           {
     /* arrow is visible (no preview is painted over it) */
               box.x1 = s->x * (width + border) + topLeftX + border;
               box.x1 += width / 2 - aW / 2;
               box.y1 = s->y * (height + border) + topLeftY + border;
               box.y1 += height / 2 - aH / 2;
           }

         box.x2 = box.x1 + aW;
         box.y2 = box.y1 + aH;

         glTranslatef (box.x1 + aW / 2, box.y1 + aH / 2, 0.0f);
         glRotatef (angle, 0.0f, 0.0f, 1.0f);
         glTranslatef (-box.x1 - aW / 2, -box.y1 - aH / 2, 0.0f);

         matrix = ws->arrowContext.texture.matrix;
         matrix.x0 -= box.x1 * matrix.xx;
         matrix.y0 -= box.y1 * matrix.yy;

         glBegin (GL_QUADS);
         wallDrawQuad (&matrix, &box);
         glEnd ();

         disableTexture (s, &ws->arrowContext.texture);
     }

   glDisable (GL_BLEND);
   glEnableClientState (GL_TEXTURE_COORD_ARRAY);
   screenTexEnvMode (s, GL_REPLACE);
   glColor4usv (defaultColor);
}

static void
wallPaintScreen(CompScreen  *s,
                CompOutput  *outputs,
                int          numOutputs,
                unsigned int mask)
{
   WALL_SCREEN (s);

   if (ws->moving && numOutputs > 1 && wallGetMmmode(s) == MmmodeSwitchAll)
     {
        outputs = &s->fullscreenOutput;
        numOutputs = 1;
     }

   UNWRAP (ws, s, paintScreen);
   (*s->paintScreen)(s, outputs, numOutputs, mask);
   WRAP (ws, s, paintScreen, wallPaintScreen);
}

static Bool
wallPaintOutput(CompScreen              *s,
                const ScreenPaintAttrib *sAttrib,
                const CompTransform     *transform,
                Region                   region,
                CompOutput              *output,
                unsigned int             mask)
{
   Bool status;

   WALL_SCREEN (s);

   ws->miniScreen = FALSE;
   if (ws->moving)
     mask |= PAINT_SCREEN_TRANSFORMED_MASK;

   UNWRAP (ws, s, paintOutput);
   status = (*s->paintOutput)(s, sAttrib, transform, region, output, mask);
   WRAP (ws, s, paintOutput, wallPaintOutput);

   if ((ws->moving || ws->boxTimeout) && wallGetShowSwitcher(s->display) &&
       (output->id == ws->boxOutputDevice || output == &s->fullscreenOutput))
     {
        CompTransform sTransform = *transform;

        transformToScreenSpace (s, output, -DEFAULT_Z_CAMERA, &sTransform);

        glPushMatrix ();
        glLoadMatrixf (sTransform.m);

        wallDrawCairoTextureOnScreen (s);

        glPopMatrix ();

        if (wallGetMiniscreen (s->display))
          {
             int i, j;
             int origVX = s->x;
             int origVY = s->y;

             moveScreenViewport (s, s->x, s->y, FALSE);

             for (j = 0; j < s->vsize; j++)
               {
                  for (i = 0; i < s->hsize; i++)
                    {
                       float mx, my, mw, mh;
                       unsigned int msMask;

                       ws->miniScreen = TRUE;
                       mx = ws->firstViewportX +
                         (i * (ws->viewportWidth + ws->viewportBorder));
                       my = ws->firstViewportY +
                         (j * (ws->viewportHeight + ws->viewportBorder));
                       mw = ws->viewportWidth;
                       mh = ws->viewportHeight;

                       ws->mSAttribs.xTranslate = mx / output->width;
                       ws->mSAttribs.yTranslate = -my / output->height;

                       ws->mSAttribs.xScale = mw / s->width;
                       ws->mSAttribs.yScale = mh / s->height;
                       ws->mSAttribs.opacity = OPAQUE * (1.0 + ws->mSzCamera);
                       ws->mSAttribs.brightness = 0.4f * BRIGHT;
                       ws->mSAttribs.saturation = COLOR;

                       if (i == origVX && j == origVY && ws->moving)
                         ws->mSAttribs.brightness = BRIGHT;

                       if (ws->boxTimeout && !ws->moving &&
                           i == origVX && j == origVY)
                         {
                            ws->mSAttribs.brightness = BRIGHT;
                         }

                       msMask = mask | PAINT_SCREEN_TRANSFORMED_MASK;
                       (*s->paintTransformedOutput)(s, sAttrib, transform,
                                                    region, output, msMask);

                       ws->miniScreen = FALSE;

                       moveScreenViewport (s, -1, 0, FALSE);
                    }
                  moveScreenViewport (s, 0, -1, FALSE);
               }
             moveScreenViewport (s, -origVX, -origVY, FALSE);
          }
     }

   return status;
}

static void
wallPreparePaintScreen(CompScreen *s,
                       int         msSinceLastPaint)
{
   CompWindow *w;

   WALL_SCREEN (s);

   if (!ws->moving && ws->boxTimeout)
     ws->boxTimeout -= msSinceLastPaint;

   if (ws->timer)
     ws->timer -= msSinceLastPaint;

   if (ws->moving)
     {
        float dx, dy;

        dx = ws->gotoX - ws->curPosX;
        dy = ws->gotoY - ws->curPosY;

        wallComputeTranslation (s, &ws->curPosX, &ws->curPosY);
     }

   if (ws->moving && ws->curPosX == ws->gotoX && ws->curPosY == ws->gotoY)
     {
        ws->moving = FALSE;
        ws->timer = 0;

        if (ws->moveWindow)
          {
             w = findWindowAtScreen (s, ws->moveWindow);
             wallReleaseMoveWindow (s);
          }
     }

   UNWRAP (ws, s, preparePaintScreen);
   (*s->preparePaintScreen)(s, msSinceLastPaint);
   WRAP (ws, s, preparePaintScreen, wallPreparePaintScreen);
}

static void
wallPaintTransformedOutput(CompScreen              *s,
                           const ScreenPaintAttrib *sAttrib,
                           const CompTransform     *transform,
                           Region                   region,
                           CompOutput              *output,
                           unsigned int             mask)
{
   CompTransform sTransform = *transform;

   WALL_SCREEN (s);

   if (ws->miniScreen)
     {
        /* move each screen to the correct output position */
         matrixTranslate (&sTransform,
                          -output->region.extents.x1 / output->width,
                          output->region.extents.y1 / output->height, 0.0f);
         matrixTranslate (&sTransform, 0.0f, 0.0f, -DEFAULT_Z_CAMERA);

         matrixTranslate (&sTransform,
                          ws->mSAttribs.xTranslate,
                          ws->mSAttribs.yTranslate,
                          ws->mSzCamera);

         /* move origin to top left */
         matrixTranslate (&sTransform, -0.5f, 0.5f, 0.0f);
         matrixScale (&sTransform,
                      ws->mSAttribs.xScale, ws->mSAttribs.yScale, 1.0);

         /* revert prepareXCoords region shift.
            Now all screens display the same */
         matrixTranslate (&sTransform, 0.5f, 0.5f, DEFAULT_Z_CAMERA);
         matrixTranslate (&sTransform,
                          output->region.extents.x1 / output->width,
                          -output->region.extents.y2 / output->height, 0.0f);

         UNWRAP (ws, s, paintTransformedOutput);
         (*s->paintTransformedOutput)(s, sAttrib, &sTransform,
                                      &s->region, output, mask);
         WRAP (ws, s, paintTransformedOutput, wallPaintTransformedOutput);
         return;
     }

   UNWRAP (ws, s, paintTransformedOutput);

   if (!ws->moving)
     (*s->paintTransformedOutput)(s, sAttrib, &sTransform,
                                  region, output, mask);

   mask &= ~PAINT_SCREEN_CLEAR_MASK;

   if (ws->moving)
     {
        ScreenPaintAttrib sA = *sAttrib;
        int origx = s->x;
        int origy = s->y;
        float px, py, gx, gy, fx, fy;
        int tx, ty;
        CompWindow *w;

        clearTargetOutput (s->display, GL_COLOR_BUFFER_BIT);

        if(!wallGetMoveBackground(s->display))
          {
             float curX = ws->curPosX;
             float curY = ws->curPosY;

             int wrap = 0;

             float elapsed = 1 - (ws->timer / (wallGetSlideDuration (s->display) * 1000));

             if (elapsed < 0.0)
               elapsed = 0.0;
             if (elapsed > 1.0)
               elapsed = 1.0;

             /* if ((ws->gotoY == ws->fromY) &&
              *    (((ws->gotoX == s->hsize - 1) && (ws->fromX == 0)) ||
              *     ((ws->gotoX == 0) && (ws->fromX == s->hsize - 1))))
              *   {
              *     wrap = 1;
              *     curX *= -1.0f;
              *   }
              *
              *
              * if ((ws->gotoY == ws->fromY) &&
              *    (((ws->gotoY == s->vsize - 1) && (ws->fromY == 0)) ||
              *     ((ws->gotoY == 0) && (ws->fromY == s->vsize - 1))))
              *   {
              *     wrap = 2;
              *     curY *= -1.0f;
              *   } */

             gx = 0;
             gy = 0;

             gx = (-1.0 + elapsed) * (float)s->width * ((ws->fromX - ws->gotoX));
             gy = (-1.0 + elapsed) * (float)s->height * ((ws->fromY - ws->gotoY));

             /* px = (curX - (float)origx); // / (float)(abs(ws->fromX - ws->gotoX) + 1);
              * py = (curY - (float)origy); // / (float)(abs(ws->gotoY - ws->gotoY) + 1); */

             /* if (wrap == 1)
              *   {
              *     if (ws->gotoX == 0)
              *       fx = 1;
              *     else
              *       gx -= 1;
              *   } */

             /* gx = -px * (float)s->width;
              * gy = -py * (float)s->height; */
             fx = gx;
             fy = gy;

             /* else if (wrap == 2)
              *   {
              *     if (ws->gotoY == 0)
              *       fx = (float)s->vsize + fx;
              *     else
              *       gx = (float)s->vsize + gx;
              *   } */
//	     printf("%1.3f \t-  %1.3f, %1.3f\n", gx, fx, px);

             for (w = s->windows; w; w = w->next)
               {
                  if (w->clientId && (w->id != ws->moveWindow || !w->grabbed) &&
                      (!(w->state & CompWindowStateStickyMask)))
                    {
                       if (w->initialViewportX == ws->fromX &&
                           w->initialViewportY == ws->fromY)
                         {
                            moveWindow(w, fx, fy, FALSE, FALSE);
     //printf("from - %d\n", w->attrib.x);
                         }
                       else if (w->initialViewportX == ws->gotoX &&
                                w->initialViewportY == ws->gotoY)
                         {
                            moveWindow(w, gx, gy, FALSE, FALSE);
     //printf("goto - %d\n", w->attrib.x);
                         }
                    }
               }

             (*s->paintTransformedOutput)(s, sAttrib, &sTransform,
                                          region, output, mask);

             for (w = s->windows; w; w = w->next)
               {
                  if (w->clientId && (w->id != ws->moveWindow || !w->grabbed) &&
                      (!(w->state & CompWindowStateStickyMask)))
                    {
                       if (w->initialViewportX == ws->fromX &&
                           w->initialViewportY == ws->fromY)
                         {
                            moveWindow(w, -fx, -fy, FALSE, FALSE);
                         }
                       else if (w->initialViewportX == ws->gotoX &&
                                w->initialViewportY == ws->gotoY)
                         {
                            moveWindow(w, -gx, -gy, FALSE, FALSE);
                         }
                    }
               }
          }
        else
          {
             px = ws->curPosX;
             py = ws->curPosY;
             // viewport moves vertically
             if (floor (py) != ceil (py))
               {
                  ty = ceil (py) - s->y;
                  sA.yTranslate = fmod (py, 1) - 1;
                  if (floor (px) != ceil (px))
                    {
                       tx = ceil (px) - s->x;
                       moveScreenViewport (s, -tx, -ty, FALSE);
                       sA.xTranslate = 1 - fmod (px, 1);

                       (*s->paintTransformedOutput)(s, &sA, &sTransform,
                                                    &output->region, output, mask);
                       moveScreenViewport (s, tx, ty, FALSE);
                    }
                  tx = floor (px) - s->x;
                  moveScreenViewport (s, -tx, -ty, FALSE);
                  sA.xTranslate = -fmod (px, 1);

                  (*s->paintTransformedOutput)(s, &sA, &sTransform,
                                               &output->region, output, mask);

                  moveScreenViewport (s, tx, ty, FALSE);
               }

             ty = floor (py) - s->y;
             sA.yTranslate = fmod (py, 1);

             // viewport moves horizontally
             if (floor (px) != ceil (px))
               {
                  tx = ceil (px) - s->x;
                  moveScreenViewport (s, -tx, -ty, FALSE);
                  sA.xTranslate = 1 - fmod (px, 1);

                  (*s->paintTransformedOutput)(s, &sA, &sTransform,
                                               &output->region, output, mask);
                  moveScreenViewport (s, tx, ty, FALSE);
               }
             tx = floor (px) - s->x;
             moveScreenViewport (s, -tx, -ty, FALSE);
             sA.xTranslate = -fmod (px, 1);

             (*s->paintTransformedOutput)(s, &sA, &sTransform,
                                          &output->region, output, mask);

             moveScreenViewport (s, tx, ty, FALSE);

             while (s->x != origx)
               moveScreenViewport (s, -1, 0, FALSE);
             while (s->y != origy)
               moveScreenViewport (s, 0, 1, FALSE);
          }
     }

   WRAP (ws, s, paintTransformedOutput, wallPaintTransformedOutput);
}

static Bool
wallPaintWindow(CompWindow              *w,
                const WindowPaintAttrib *attrib,
                const CompTransform     *transform,
                Region                   region,
                unsigned int             mask)
{
   Bool status;
   CompScreen *s = w->screen;

   WALL_SCREEN (s);

   if (ws->miniScreen)
     {
        WindowPaintAttrib pA = *attrib;

        pA.opacity = attrib->opacity *
          ((float)ws->mSAttribs.opacity / OPAQUE);
        pA.brightness = attrib->brightness *
          ((float)ws->mSAttribs.brightness / BRIGHT);
        pA.saturation = attrib->saturation *
          ((float)ws->mSAttribs.saturation / COLOR);

        if (!pA.opacity || !pA.brightness)
          mask |= PAINT_WINDOW_NO_CORE_INSTANCE_MASK;

        UNWRAP (ws, s, paintWindow);
        status = (*s->paintWindow)(w, &pA, transform, region, mask);
        WRAP (ws, s, paintWindow, wallPaintWindow);
     }
   else
     {
        UNWRAP (ws, s, paintWindow);
        status = (*s->paintWindow)(w, attrib, transform, region, mask);
        WRAP (ws, s, paintWindow, wallPaintWindow);
     }

   return status;
}

static void
wallDonePaintScreen(CompScreen *s)
{
   WALL_SCREEN (s);

   if (ws->moving || ws->boxTimeout)
     damageScreen (s);

   if (ws->boxTimeout < 0)
     {
        ws->boxTimeout = 0;
        damageScreen (s);
     }

   UNWRAP (ws, s, donePaintScreen);
   (*s->donePaintScreen)(s);
   WRAP (ws, s, donePaintScreen, wallDonePaintScreen);
}

static void
wallCreateCairoContexts(CompScreen *s,
                        Bool        initial)
{
   float border = 10.0f;
   float width, height;

   WALL_SCREEN (s);

   width = VIEWPORT_SWITCHER_SIZE * s->hsize + (2 * border * (s->hsize - 1));
   width *= (float)s->width / (float)s->height;
   height = VIEWPORT_SWITCHER_SIZE * s->vsize + (2 * border * (s->vsize - 1));

   wallDestroyCairoContext (s, &ws->switcherContext);
   ws->switcherContext.width = width;
   ws->switcherContext.height = height;
   wallSetupCairoContext (s, &ws->switcherContext);
   wallDrawSwitcherBackground (s);

   wallDestroyCairoContext (s, &ws->thumbContext);
   ws->thumbContext.width = (width - border * (s->hsize + 1)) / s->hsize;
   ws->thumbContext.height = (height - border * (s->vsize + 1)) / s->vsize;
   wallSetupCairoContext (s, &ws->thumbContext);
   wallDrawThumb (s);

   wallDestroyCairoContext (s, &ws->highlightContext);
   ws->highlightContext.width = (width - border * (s->hsize + 1)) / s->hsize;
   ws->highlightContext.height = (height - border * (s->vsize + 1)) / s->vsize;
   wallSetupCairoContext (s, &ws->highlightContext);
   wallDrawHighlight (s);

   if (initial)
     {
        ws->arrowContext.width = ARROW_SIZE;
        ws->arrowContext.height = ARROW_SIZE;
        wallSetupCairoContext (s, &ws->arrowContext);
        wallDrawArrow (s);
     }
}

static void
wallDisplayOptionChanged(CompDisplay       *display,
                         CompOption        *opt,
                         WallDisplayOptions num)
{
   CompScreen *s;

   switch(num)
     {
      case WallDisplayOptionOutlineColor:
        for (s = display->screens; s; s = s->next)
          {
             wallDrawSwitcherBackground (s);
             wallDrawHighlight (s);
             wallDrawThumb (s);
          }
        break;

      case WallDisplayOptionEdgeRadius:
      case WallDisplayOptionBackgroundGradientBaseColor:
      case WallDisplayOptionBackgroundGradientHighlightColor:
      case WallDisplayOptionBackgroundGradientShadowColor:
        for (s = display->screens; s; s = s->next)
          wallDrawSwitcherBackground (s);
        break;

      case WallDisplayOptionThumbGradientBaseColor:
      case WallDisplayOptionThumbGradientHighlightColor:
        for (s = display->screens; s; s = s->next)
          wallDrawThumb (s);
        break;

      case WallDisplayOptionThumbHighlightGradientBaseColor:
      case WallDisplayOptionThumbHighlightGradientShadowColor:
        for (s = display->screens; s; s = s->next)
          wallDrawHighlight (s);
        break;

      case WallDisplayOptionArrowBaseColor:
      case WallDisplayOptionArrowShadowColor:
        for (s = display->screens; s; s = s->next)
          wallDrawArrow (s);
        break;

      default:
        break;
     }
}

static Bool
wallSetScreenOptionCore(CompScreen      *screen,
                        char            *name,
                        CompOptionValue *value)
{
   Bool status;

   WALL_SCREEN (screen);

   UNWRAP (ws, screen, setScreenOption);
   status = (*screen->setScreenOption)(screen, name, value);
   WRAP (ws, screen, setScreenOption, wallSetScreenOptionCore);

   if (status)
     {
        if (strcmp (name, "hsize") == 0 || strcmp (name, "vsize") == 0)
          wallCreateCairoContexts (screen, FALSE);
     }

   return status;
}

static Bool
wallInitDisplay(CompPlugin  *p,
                CompDisplay *d)
{
   WallDisplay *wd;

   wd = malloc (sizeof (WallDisplay));
   if (!wd)
     return FALSE;

   wd->screenPrivateIndex = allocateScreenPrivateIndex (d);
   if (wd->screenPrivateIndex < 0)
     {
        free (wd);
        return FALSE;
     }

   wallSetEdgeRadiusNotify (d, wallDisplayOptionChanged);
   wallSetOutlineColorNotify (d, wallDisplayOptionChanged);
   wallSetBackgroundGradientBaseColorNotify (d, wallDisplayOptionChanged);
   wallSetBackgroundGradientHighlightColorNotify (d, wallDisplayOptionChanged);
   wallSetBackgroundGradientShadowColorNotify (d, wallDisplayOptionChanged);
   wallSetThumbGradientBaseColorNotify (d, wallDisplayOptionChanged);
   wallSetThumbGradientHighlightColorNotify (d, wallDisplayOptionChanged);
   wallSetThumbHighlightGradientBaseColorNotify (d, wallDisplayOptionChanged);
   wallSetThumbHighlightGradientShadowColorNotify (d,
                                                   wallDisplayOptionChanged);
   wallSetArrowBaseColorNotify (d, wallDisplayOptionChanged);
   wallSetArrowShadowColorNotify (d, wallDisplayOptionChanged);

   WRAP (wd, d, handleEvent, wallHandleEvent);
   d->privates[displayPrivateIndex].ptr = wd;

   return TRUE;
}

static void
wallFiniDisplay(CompPlugin  *p,
                CompDisplay *d)
{
   WALL_DISPLAY (d);

   UNWRAP (wd, d, handleEvent);

   freeScreenPrivateIndex (d, wd->screenPrivateIndex);
   free (wd);
}

static Bool
wallInitScreen(CompPlugin *p,
               CompScreen *s)
{
   WallScreen *ws;

   WALL_DISPLAY (s->display);

   ws = malloc (sizeof (WallScreen));
   if (!ws)
     return FALSE;

   ws->boxTimeout = 0;
   ws->moving = FALSE;

   memset (&ws->switcherContext, 0, sizeof (WallCairoContext));
   memset (&ws->thumbContext, 0, sizeof (WallCairoContext));
   memset (&ws->highlightContext, 0, sizeof (WallCairoContext));
   memset (&ws->arrowContext, 0, sizeof (WallCairoContext));

   WRAP (ws, s, paintScreen, wallPaintScreen);
   WRAP (ws, s, paintOutput, wallPaintOutput);
   WRAP (ws, s, donePaintScreen, wallDonePaintScreen);
   WRAP (ws, s, paintTransformedOutput, wallPaintTransformedOutput);
   WRAP (ws, s, preparePaintScreen, wallPreparePaintScreen);
   WRAP (ws, s, paintWindow, wallPaintWindow);
   WRAP (ws, s, setScreenOption, wallSetScreenOptionCore);

   s->privates[wd->screenPrivateIndex].ptr = ws;

   wallCreateCairoContexts (s, TRUE);

   return TRUE;
}

static void
wallFiniScreen(CompPlugin *p,
               CompScreen *s)
{
   WALL_SCREEN (s);

   wallDestroyCairoContext (s, &ws->switcherContext);
   wallDestroyCairoContext (s, &ws->thumbContext);
   wallDestroyCairoContext (s, &ws->highlightContext);
   wallDestroyCairoContext (s, &ws->arrowContext);

   UNWRAP (ws, s, paintScreen);
   UNWRAP (ws, s, paintOutput);
   UNWRAP (ws, s, donePaintScreen);
   UNWRAP (ws, s, paintTransformedOutput);
   UNWRAP (ws, s, preparePaintScreen);
   UNWRAP (ws, s, paintWindow);
   UNWRAP (ws, s, setScreenOption);

   free(ws);
}

static Bool
wallInit(CompPlugin *p)
{
   displayPrivateIndex = allocateDisplayPrivateIndex ();
   if (displayPrivateIndex < 0)
     return FALSE;

   return TRUE;
}

static void
wallFini(CompPlugin *p)
{
   freeDisplayPrivateIndex (displayPrivateIndex);
}

static int
wallGetVersion(CompPlugin *p,
               int         version)
{
   return ABIVERSION;
}

CompPluginVTable wallVTable = {
   "wall",
   wallGetVersion,
   0,
   wallInit,
   wallFini,
   wallInitDisplay,
   wallFiniDisplay,
   wallInitScreen,
   wallFiniScreen,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL
};

CompPluginVTable *
getCompPluginInfo(void)
{
   return &wallVTable;
}

