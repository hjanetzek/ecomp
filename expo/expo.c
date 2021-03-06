/**
 *
 * Ecomp expo plugin
 *
 * expo.c
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 **/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <sys/time.h>

#include <ecomp.h>
#include "expo_options.h"

#include <GL/glu.h>

#define PI 3.14159265359f

#define WIN_X(w) ((w)->attrib.x - (w)->input.left)
#define WIN_Y(w) ((w)->attrib.y - (w)->input.top)
#define WIN_W(w) ((w)->width + (w)->input.left + (w)->input.right)
#define WIN_H(w) ((w)->height + (w)->input.top + (w)->input.bottom)

static int displayPrivateIndex;

typedef enum
{
   DnDNone = 0,
   DnDDuring,
   DnDStart
} DnDState;

typedef enum
{
   VPUpdateNone = 0,
   VPUpdateMouseOver,
   VPUpdatePrevious
} VPUpdateMode;

typedef struct _ExpoDisplay
{
   int             screenPrivateIndex;

   HandleEventProc handleEvent;
} ExpoDisplay;

typedef struct _ExpoScreen
{
   DonePaintScreenProc        donePaintScreen;
   PaintOutputProc            paintOutput;
   PaintScreenProc            paintScreen;
   PreparePaintScreenProc     preparePaintScreen;
   PaintTransformedOutputProc paintTransformedOutput;
   PaintWindowProc            paintWindow;
   DrawWindowProc             drawWindow;
   DamageWindowRectProc       damageWindowRect;

   /*	Used for expo zoom animation */
   float                      expoCam;
   Bool                       expoActive;
   /* In expo mode? */
   Bool                       expoMode;

   /* For expo grab */
   int                        grabIndex;

   GLint                      viewport[4];

   /* Window being dragged in expo mode */
   DnDState                   dndState;
   CompWindow                *dndWindow;

   int                        prevCursorX, prevCursorY;
   int                        newCursorX, newCursorY;

   int                        origVX;
   int                        origVY;
   int                        selectedVX;
   int                        selectedVY;

   VPUpdateMode               vpUpdateMode;

   Bool                       anyClick;

   unsigned int               clickTime;
   Bool                       doubleClick;
   Bool                       moveToDesk;
} ExpoScreen;

typedef struct _xyz_tuple
{
   float x, y, z;
}
Point3d;

/* Helpers */
#define GET_EXPO_DISPLAY(d) \
  ((ExpoDisplay *)(d)->privates[displayPrivateIndex].ptr)
#define EXPO_DISPLAY(d) \
  ExpoDisplay * ed = GET_EXPO_DISPLAY(d);

#define GET_EXPO_SCREEN(s, ed) \
  ((ExpoScreen *)(s)->privates[(ed)->screenPrivateIndex].ptr)
#define EXPO_SCREEN(s) \
  ExpoScreen * es = GET_EXPO_SCREEN(s, GET_EXPO_DISPLAY(s->display))

#define sigmoid(x)         (1.0f / (1.0f + exp (-5.5f * 2 * ((x) - 0.5))))
#define sigmoidProgress(x) ((sigmoid (x) - sigmoid (0)) / \
                            (sigmoid (1) - sigmoid (0)))

static void
expoMoveFocusViewport(CompScreen *s,
                      int         dx,
                      int         dy)
{
   EXPO_SCREEN (s);

   es->selectedVX += dx;
   es->selectedVY += dy;

   es->selectedVX = MIN (s->hsize - 1, es->selectedVX);
   es->selectedVX = MAX (0, es->selectedVX);
   es->selectedVY = MIN (s->vsize - 1, es->selectedVY);
   es->selectedVY = MAX (0, es->selectedVY);

   damageScreen (s);
}

static void
expoFinishWindowMovement(CompWindow *w)
{
   CompScreen *s = w->screen;

   EXPO_SCREEN (s);

   syncWindowPosition (w);
   (*s->windowUngrabNotify)(w);

   moveScreenViewport (s, s->x - es->selectedVX,
                       s->y - es->selectedVY, TRUE);

   sendScreenViewportMessage(s);

   ecompMessage(s, w, ECOMORPH_ECOMP_WINDOW_MOVE,
                w->attrib.x + (s->x * s->width),
                w->attrib.y + (s->y * s->height),
                0, 0);
}

static Bool
termExpo(CompDisplay *d, int cancel)
{
   CompScreen *s;

   for (s = d->screens; s; s = s->next)
     {
        EXPO_SCREEN (s);

        if (!es->expoMode)
          continue;

        es->expoMode = FALSE;

        if (es->dndWindow)
          syncWindowPosition (es->dndWindow);

        if (cancel)
          es->vpUpdateMode = VPUpdatePrevious;
        else
          {
             moveScreenViewport (s, s->x - es->selectedVX,
                                 s->y - es->selectedVY, TRUE);

             sendScreenViewportMessage(s);
             es->vpUpdateMode = VPUpdateNone;
          }

        es->dndState = DnDNone;
        es->dndWindow = 0;

        damageScreen (s);
        ecompActionTerminateNotify (s, 1);
     }

   return TRUE;
}

static void
expoHandleEvent(CompDisplay *d,
                XEvent      *event)
{
   EXPO_DISPLAY (d);
   CompScreen *s;
   Window win = None;

   switch (event->type)
     {
      case ClientMessage:
        if (event->xclient.message_type == d->ecoPluginAtom)
          {
             if(event->xclient.data.l[1] != ECO_PLUGIN_EXPO) break;

             Window win = event->xclient.data.l[0];

             if (!(s = findScreenAtDisplay (d, win)))
               {
                  for (s = d->screens; s; s = s->next)
                    ecompActionTerminateNotify (s, 1);
                  break;
               }
             unsigned int action = event->xclient.data.l[2];
             unsigned int option = event->xclient.data.l[3];
             unsigned int option2 = event->xclient.data.l[4];

             if (action == ECO_ACT_TERMINATE)
               {
                  termExpo(d, (option == ECO_ACT_OPT_TERMINATE_CANCEL));
                  break;
               }
             else if ((action == ECO_ACT_INITIATE) ||
                      (action == ECO_ACT_CYCLE))
               {
                  EXPO_SCREEN (s);
                  if (!es->expoMode)
                    {
                       if (otherScreenGrabExist (s, "expo", 0))
                         {
                            ecompActionTerminateNotify (s, 1);
                            break;
                         }

                       if (!es->grabIndex)
                         es->grabIndex = pushScreenGrab (s, None, "expo");

                       es->expoMode = TRUE;
                       es->anyClick = FALSE;
                       es->dndState = DnDNone;
                       es->dndWindow = None;

                       es->selectedVX = es->origVX = s->x;
                       es->selectedVY = es->origVY = s->y;

                       damageScreen (s);
                    }
                  else if (action == ECO_ACT_CYCLE)
                    {
                       if (option2 == ECO_ACT_OPT_CYCLE_NEXT)
                         {
                            if (es->selectedVX == s->hsize - 1)
                              {
                                 if (es->selectedVY == s->vsize - 1)
                                   expoMoveFocusViewport (s, -(s->hsize - 1), -(s->vsize - 1));
                                 else
                                   expoMoveFocusViewport (s, -(s->hsize - 1), 1);
                              }
                            else
                              {
                                 expoMoveFocusViewport (s, 1, 0);
                              }
                         }
                       else if (option2 == ECO_ACT_OPT_CYCLE_PREV)
                         {
                            if (es->selectedVX == 0)
                              {
                                 if (es->selectedVY == 0)
                                   expoMoveFocusViewport (s, (s->hsize - 1), (s->vsize - 1));
                                 else
                                   expoMoveFocusViewport (s, (s->hsize - 1), -1);
                              }
                            else
                              {
                                 expoMoveFocusViewport (s, -1, 0);
                              }
                         }
                    }
               }
             else if (action == ECO_ACT_MOUSE_DOWN)
               {
                  EXPO_SCREEN (s);

                  if (es->expoMode)
                    {
                       pointerX = ECO_GET_X(option2);
                       pointerY = ECO_GET_Y(option2);

                       switch (ECO_GET_BUTTON(option))
                         {
                          case Button1:
                            es->dndState = DnDStart;

                            es->doubleClick = ECO_GET_DBLCLICK(option);

                            break;

                          case Button3:
                            es->dndState = DnDStart;
                            es->moveToDesk = TRUE;
                            break;

                          default:
                            es->anyClick = TRUE;
                            break;
                         }
                       damageScreen (s);
                    }
               }
             else if (action == ECO_ACT_MOUSE_UP)
               {
                  EXPO_SCREEN (s);

                  es->moveToDesk = FALSE;

                  if ((ECO_GET_BUTTON(option)) == Button2)
                    {
                       es->doubleClick = TRUE;
                    }

                  if (es->dndState == DnDDuring || es->dndState == DnDStart)
                    {
                       if (es->dndWindow)
                         expoFinishWindowMovement (es->dndWindow);

                       es->dndState = DnDNone;
                       es->dndWindow = NULL;
                    }
               }
             else if (action == ECO_ACT_MOUSE_MOVE)
               {
                  pointerX = option;
                  pointerY = option2;
               }
          }
        break;

      case UnmapNotify:
        win = event->xunmap.window;

      case DestroyNotify:
        win = event->xdestroywindow.window;
     }

   if (win)
     {
        CompWindow *w;
        w = findWindowAtDisplay(d, win);

        if (w)
          {
             EXPO_SCREEN(w->screen);
             if ((es->dndState != DnDNone) &&
                 (es->dndWindow == w))
               {
                  es->dndWindow = None;
                  es->dndState = DnDNone;
               }
          }
     }

   UNWRAP (ed, d, handleEvent);
   (*d->handleEvent)(d, event);
   WRAP (ed, d, handleEvent, expoHandleEvent);
}

static void
invertTransformedVertex(CompScreen              *s,
                        const ScreenPaintAttrib *sAttrib,
                        const CompTransform     *transform,
                        CompOutput              *output,
                        int                      vertex[2])
{
   CompTransform sTransform = *transform;
   GLdouble p1[3], p2[3], v[3], alpha;
   GLdouble mvm[16], pm[16];
   int i;

   EXPO_SCREEN (s);

   (*s->applyScreenTransform)(s, sAttrib, output, &sTransform);
   transformToScreenSpace (s, output, -sAttrib->zTranslate, &sTransform);

   glGetIntegerv (GL_VIEWPORT, es->viewport);

   for (i = 0; i < 16; i++)
     {
        mvm[i] = sTransform.m[i];
        pm[i] = s->projection[i];
     }

   gluUnProject (vertex[0], s->height - vertex[1], 0, mvm, pm,
                 es->viewport, &p1[0], &p1[1], &p1[2]);
   gluUnProject (vertex[0], s->height - vertex[1], -1.0, mvm, pm,
                 es->viewport, &p2[0], &p2[1], &p2[2]);

   for (i = 0; i < 3; i++)
     v[i] = p1[i] - p2[i];

   alpha = -p1[2] / v[2];

   vertex[0] = ceil (p1[0] + (alpha * v[0]));
   vertex[1] = ceil (p1[1] + (alpha * v[1]));
}

static Bool
expoDamageWindowRect(CompWindow *w,
                     Bool        initial,
                     BoxPtr      rect)
{
   Bool status;
   EXPO_SCREEN (w->screen);

   UNWRAP (es, w->screen, damageWindowRect);
   status = (*w->screen->damageWindowRect)(w, initial, rect);
   WRAP (es, w->screen, damageWindowRect, expoDamageWindowRect);

   if (es->expoCam > 0.0f)
     damageScreen (w->screen);

   return status;
}

static void
expoPaintScreen(CompScreen  *s,
                CompOutput  *outputs,
                int          numOutputs,
                unsigned int mask)
{
   EXPO_SCREEN (s);

   if (es->expoCam > 0.0 && numOutputs > 1 &&
       expoGetMultioutputMode (s->display) == MultioutputModeOneBigWall)
     {
        outputs = &s->fullscreenOutput;
        numOutputs = 1;
     }

   UNWRAP (es, s, paintScreen);
   (*s->paintScreen)(s, outputs, numOutputs, mask);
   WRAP (es, s, paintScreen, expoPaintScreen);
}

static Bool
expoPaintOutput(CompScreen              *s,
                const ScreenPaintAttrib *sAttrib,
                const CompTransform     *transform,
                Region                   region,
                CompOutput              *output,
                unsigned int             mask)
{
   Bool status;

   EXPO_SCREEN (s);

   if (es->expoCam > 0.0)
     mask |= PAINT_SCREEN_TRANSFORMED_MASK | PAINT_SCREEN_CLEAR_MASK;

   UNWRAP (es, s, paintOutput);
   status = (*s->paintOutput)(s, sAttrib, transform, region, output, mask);
   WRAP (es, s, paintOutput, expoPaintOutput);

   return status;
}

static void
expoPreparePaintScreen(CompScreen *s,
                       int         ms)
{
   EXPO_SCREEN (s);

   float val = ((float)ms / 1000.0) / (expoGetZoomTime (s->display) / 10.0);

   if (es->expoMode)
     es->expoCam = MIN (1.0, es->expoCam + val);
   else
     es->expoCam = MAX (0.0, es->expoCam - val);

   UNWRAP (es, s, preparePaintScreen);
   (*s->preparePaintScreen)(s, ms);
   WRAP (es, s, preparePaintScreen, expoPreparePaintScreen);
}

static void
expoPaintWall(CompScreen              *s,
              const ScreenPaintAttrib *sAttrib,
              const CompTransform     *transform,
              Region                   region,
              CompOutput              *output,
              unsigned int             mask,
              Bool                     reflection)
{
   EXPO_SCREEN (s);

   CompTransform sTransformW, sTransform = *transform;
   int i, j;
   int oldFilter = s->display->textureFilter;

   int origVX = s->x;
   int origVY = s->y;

   float sx = (float)s->width / output->width;
   float sy = (float)s->height / output->height;
   float biasZ;
   float oScale, rotation = 0.0f, progress;
   float aspectX = 1.0f, aspectY = 1.0f;
   float camX, camY, camZ;

   /* amount of gap between viewports */
   const float gapY = 0.01f * es->expoCam;
   const float gapX = 0.01f * s->height / s->width * es->expoCam;

   /* Zoom animation stuff */
   /* camera position for the selected viewport */
   Point3d vpCamPos = { 0, 0, 0 };

   /* camera position during expo mode */
   Point3d expoCamPos = { 0, 0, 0 };

   vpCamPos.x = ((s->x * sx) + 0.5 +
                 (output->region.extents.x1 / output->width)) -
     (s->hsize * 0.5 * sx) + gapX * (s->x);
   vpCamPos.y = -((s->y * sy) + 0.5 +
                  (output->region.extents.y1 / output->height)) +
     (s->vsize * 0.5 * sy) - gapY * (s->y);
   vpCamPos.z = 0;

   if (expoGetRotate (s->display) || expoGetReflection (s->display))
     biasZ = MAX (s->hsize * sx, s->vsize * sy) *
       (0.15 + expoGetDistance (s->display));
   else
     biasZ = MAX (s->hsize * sx, s->vsize * sy) *
       expoGetDistance (s->display);

   expoCamPos.x = gapX * (s->hsize - 1) * 0.5;
   expoCamPos.y = -gapY * (s->vsize - 1) * 0.5;
   expoCamPos.z = -DEFAULT_Z_CAMERA + DEFAULT_Z_CAMERA *
     (MAX (s->hsize + (s->hsize - 1) * gapX,
           s->vsize + (s->vsize - 1) * gapY) + biasZ);

   progress = sigmoidProgress (es->expoCam);

   /* interpolate between vpCamPos and expoCamPos */
   camX = vpCamPos.x * (1 - progress) + expoCamPos.x * progress;
   camY = vpCamPos.y * (1 - progress) + expoCamPos.y * progress;
   camZ = vpCamPos.z * (1 - progress) + expoCamPos.z * progress;

   if (s->hsize > s->vsize)
     {
        aspectY = (float)s->hsize / (float)s->vsize;
        aspectY -= 1.0;
        aspectY *= -expoGetAspectRatio (s->display) + 1.0;
        aspectY *= progress;
        aspectY += 1.0;
     }
   else
     {
        aspectX = (float)s->vsize / (float)s->hsize;
        aspectX -= 1.0;
        aspectX *= -expoGetAspectRatio (s->display) + 1.0;
        aspectX *= progress;
        aspectX += 1.0;
     }

   /* End of Zoom animation stuff */

   moveScreenViewport (s, s->x, s->y, FALSE);

   if (expoGetRotate (s->display))
     {
        if (expoGetExpoAnimation (s->display) == ExpoAnimationZoom)
          rotation = 10.0 * sigmoidProgress (es->expoCam);
        else
          rotation = 10.0 * es->expoCam;
     }

   if (expoGetMipmaps (s->display))
     s->display->textureFilter = GL_LINEAR_MIPMAP_LINEAR;

   /* ALL TRANSFORMATION ARE EXECUTED FROM BOTTOM TO TOP */

   oScale = 1 / (1 + ((MAX (sx, sy) - 1) * progress));

   matrixScale (&sTransform, oScale, oScale, 1.0);

   if (reflection)
     {
        float scaleFactor = expoGetScaleFactor (s->display);

        matrixTranslate (&sTransform, 0.0, -s->vsize * sy * aspectY, 0.0);
        matrixScale (&sTransform, 1.0, -1.0, 1.0);
        matrixTranslate (&sTransform, 0.0,
                         -(1 - scaleFactor) / 2 * s->vsize * sy * aspectY,
                         0.0);
        matrixScale (&sTransform, 1.0, scaleFactor, 1.0);
        glCullFace (GL_FRONT);
     }

   /* zoom out */
   matrixTranslate (&sTransform, -camX, -camY, -camZ - DEFAULT_Z_CAMERA);

   /* rotate */
   matrixRotate (&sTransform, rotation, 0.0f, 1.0f, 0.0f);
   matrixScale (&sTransform, aspectX, aspectY, 1.0);

   /* translate expo to center */
   matrixTranslate (&sTransform, s->hsize * sx * -0.5,
                    s->vsize * sy * 0.5, 0.0f);
   sTransformW = sTransform;

   /* revert prepareXCoords region shift. Now all screens display the same */
   matrixTranslate (&sTransform, 0.5f, -0.5f, DEFAULT_Z_CAMERA);

   es->expoActive = TRUE;

   for (j = 0; j < s->vsize; j++)
     {
        CompTransform sTransform2 = sTransform;
        CompTransform sTransform3;

        for (i = 0; i < s->hsize; i++)
          {
             if (expoGetExpoAnimation (s->display) == ExpoAnimationVortex)
               matrixRotate (&sTransform2, 360 * es->expoCam, 0.0f, 1.0f,
                             2.0f * es->expoCam);

             sTransform3 = sTransform2;
             matrixTranslate (&sTransform3,
                              output->region.extents.x1 / output->width,
                              -output->region.extents.y1 / output->height, 0.0);

             paintTransformedOutput (s, sAttrib, &sTransform3, &s->region,
                                     output, mask);

             if (!reflection)
               {
                  int cursor[2] = { pointerX, pointerY };

                  invertTransformedVertex (s, sAttrib, &sTransform3,
                                           output, cursor);

                  if ((cursor[0] > 0) && (cursor[0] < s->width) &&
                      (cursor[1] > 0) && (cursor[1] < s->height))
                    {
                       es->newCursorX = i * s->width + cursor[0];
                       es->newCursorY = j * s->height + cursor[1];

                       if (es->doubleClick || es->anyClick || es->dndState != DnDNone)
                         {
     /* Used to save last viewport interaction was in */
                             es->selectedVX = i;
                             es->selectedVY = j;
                             es->anyClick = FALSE;
                         }
                    }
               }

             /* not sure this will work with different resolutions */
             matrixTranslate (&sTransform2, ((1.0 * sx) + gapX), 0.0f, 0.0);
             moveScreenViewport (s, -1, 0, FALSE);
          }

        /* not sure this will work with different resolutions */
        matrixTranslate (&sTransform, 0, -((1.0 * sy) + gapY), 0.0f);
        moveScreenViewport (s, 0, -1, FALSE);
     }

   if (reflection)
     {
        glPushMatrix();
        glLoadMatrixf (sTransformW.m);
        glEnable (GL_BLEND);

        glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glBegin (GL_QUADS);
        glColor4f (0.0, 0.0, 0.0, 1.0);
        glVertex2f (0.0, 0.0);
        glColor4f (0.0, 0.0, 0.0, 0.5);
        glVertex2f (0.0, -s->vsize * (1.0 * sy + gapY));
        glVertex2f (s->hsize * sx * (1.0 + gapX),
                    -s->vsize * sy * (1.0 + gapY));
        glColor4f (0.0, 0.0, 0.0, 1.0);
        glVertex2f (s->hsize * sx * (1.0 + gapX), 0.0);
        glEnd ();
        glCullFace (GL_BACK);

        glLoadIdentity ();
        glTranslatef (0.0, 0.0, -DEFAULT_Z_CAMERA);

        if (expoGetGroundSize (s->display) > 0.0)
          {
             glBegin (GL_QUADS);
             glColor4usv (expoGetGroundColor1 (s->display));
             glVertex2f (-0.5, -0.5);
             glVertex2f (0.5, -0.5);
             glColor4usv (expoGetGroundColor2 (s->display));
             glVertex2f (0.5, -0.5 + expoGetGroundSize (s->display));
             glVertex2f (-0.5, -0.5 + expoGetGroundSize (s->display));
             glEnd ();
          }

        glColor4usv (defaultColor);

        glBlendFunc (GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
        glDisable (GL_BLEND);
        glPopMatrix ();
     }

   es->expoActive = FALSE;

   moveScreenViewport (s, -origVX, -origVY, FALSE);

   s->filter[SCREEN_TRANS_FILTER] = oldFilter;
   s->display->textureFilter = oldFilter;
}

static void
expoPaintTransformedOutput(CompScreen              *s,
                           const ScreenPaintAttrib *sAttrib,
                           const CompTransform     *transform,
                           Region                   region,
                           CompOutput              *output,
                           unsigned int             mask)
{
   EXPO_SCREEN (s);

   UNWRAP (es, s, paintTransformedOutput);

   es->expoActive = FALSE;

   if (es->expoCam > 0)
     mask |= PAINT_SCREEN_CLEAR_MASK;

   if (es->expoCam <= 0 || (es->expoCam < 1.0 && es->expoCam > 0.0 &&
                            expoGetExpoAnimation (s->display) != ExpoAnimationZoom))
     {
        (*s->paintTransformedOutput)(s, sAttrib, transform, region,
                                     output, mask);
     }
   else
     clearScreenOutput (s, output, GL_COLOR_BUFFER_BIT);

   mask &= ~PAINT_SCREEN_CLEAR_MASK;

   if (es->expoCam > 0.0)
     {
        if (expoGetReflection (s->display))
          expoPaintWall (s, sAttrib, transform, region, output, mask, TRUE);

        expoPaintWall (s, sAttrib, transform, region, output, mask, FALSE);
     }

   WRAP (es, s, paintTransformedOutput, expoPaintTransformedOutput);
}

static Bool
expoDrawWindow(CompWindow           *w,
               const CompTransform  *transform,
               const FragmentAttrib *fragment,
               Region                region,
               unsigned int          mask)
{
   Bool status;
   CompScreen *s = w->screen;

   EXPO_SCREEN (s);

   if (es->expoCam > 0.0)
     {
        FragmentAttrib fA = *fragment;
        ExpoExpoAnimationEnum expoAnimation;

        expoAnimation = expoGetExpoAnimation (s->display);

        if (es->expoActive)
          {
             if (expoAnimation != ExpoAnimationZoom)
               fA.opacity = fragment->opacity * es->expoCam;

             if (!(w->wmType & CompWindowTypeDesktopMask) &&
                 ((!w->clientId) && expoGetHideDocks (s->display)))
               {
                  if (expoAnimation == ExpoAnimationZoom &&
                      (s->x == es->selectedVX && s->y == es->selectedVY))
                    {
                       fA.opacity = fragment->opacity *
                         (1 - sigmoidProgress (es->expoCam));
                    }
                  else
                    fA.opacity = 0;
               }

             if (s->x != es->selectedVX || s->y != es->selectedVY)
               {
                  fA.brightness = fragment->brightness * .75;
               }
          }
        else
          {
             if (expoAnimation == ExpoAnimationZoom)
               fA.brightness = 0;
             else
               fA.brightness = fragment->brightness *
                 (1 - sigmoidProgress (es->expoCam));
          }

        UNWRAP (es, s, drawWindow);
        status = (*s->drawWindow)(w, transform, &fA, region, mask);
        WRAP (es, s, drawWindow, expoDrawWindow);
     }
   else
     {
        UNWRAP (es, s, drawWindow);
        status = (*s->drawWindow)(w, transform, fragment, region, mask);
        WRAP (es, s, drawWindow, expoDrawWindow);
     }

   return status;
}

static Bool
expoPaintWindow(CompWindow              *w,
                const WindowPaintAttrib *attrib,
                const CompTransform     *transform,
                Region                   region,
                unsigned int             mask)
{
   CompScreen *s = w->screen;
   Bool status;

   EXPO_SCREEN (s);

   if (es->expoActive)
     {
        float opacity = 1.0;
        Bool hideDocks;

        ExpoExpoAnimationEnum expoAnimation;

        expoAnimation = expoGetExpoAnimation (s->display);

        hideDocks = expoGetHideDocks (s->display);

        if (es->expoCam > 0.0 && es->expoCam < 1.0 &&
            expoAnimation != ExpoAnimationZoom)
          mask |= PAINT_WINDOW_TRANSLUCENT_MASK;

        if (es->expoCam > 0.0 && hideDocks &&
            (!(w->wmType & CompWindowTypeDesktopMask) &&
             ((!w->clientId) && expoGetHideDocks (s->display))))
          mask |= PAINT_WINDOW_TRANSLUCENT_MASK;

        if (expoAnimation != ExpoAnimationZoom)
          opacity = attrib->opacity * es->expoCam;

        if (!(w->wmType & CompWindowTypeDesktopMask) &&
            ((!w->clientId) && expoGetHideDocks (s->display)))
          {
             if (expoAnimation == ExpoAnimationZoom &&
                 (s->x == es->selectedVX && s->y == es->selectedVY))
               {
                  opacity = attrib->opacity *
                    (1 - sigmoidProgress (es->expoCam));
               }
             else
               opacity = 0;

             if (opacity <= 0)
               mask |= PAINT_WINDOW_NO_CORE_INSTANCE_MASK;
          }
     }

   UNWRAP (es, s, paintWindow);
   status = (*s->paintWindow)(w, attrib, transform, region, mask);
   WRAP (es, s, paintWindow, expoPaintWindow);

   return status;
}

static void
expoDonePaintScreen(CompScreen *s)
{
   EXPO_SCREEN (s);

   switch (es->vpUpdateMode) {
      case VPUpdateMouseOver:
        moveScreenViewport (s, s->x - es->selectedVX,
                            s->y - es->selectedVY, TRUE);
        /*XXX temporary? */
        sendScreenViewportMessage(s);
        //focusDefaultWindow (s->display);
        es->vpUpdateMode = VPUpdateNone;
        break;

      case VPUpdatePrevious:
        moveScreenViewport (s, s->x - es->origVX, s->y - es->origVY, TRUE);
        /*XXX temporary? */
        sendScreenViewportMessage(s);
        //focusDefaultWindow (s->display);
        es->vpUpdateMode = VPUpdateNone;
        break;

      default:
        break;
     }

   if ((es->expoCam > 0.0f && es->expoCam < 1.0f) || es->dndState != DnDNone)
     damageScreen (s);

   if (es->grabIndex && es->expoCam <= 0.0f && !es->expoMode)
     {
        removeScreenGrab (s, es->grabIndex, 0);
        es->grabIndex = 0;
     }

   UNWRAP (es, s, donePaintScreen);
   (*s->donePaintScreen)(s);
   WRAP (es, s, donePaintScreen, expoDonePaintScreen);

   if (es->doubleClick)
     {
        es->clickTime = 0;
        es->doubleClick = FALSE;

        termExpo (s->display, 0);

        es->anyClick = FALSE;
        return;
     }

   switch (es->dndState) {
      case DnDDuring:
      {
         int dx = es->newCursorX - es->prevCursorX;
         int dy = es->newCursorY - es->prevCursorY;
         int moved = 1;
         CompWindow *w = es->dndWindow;

         if ((dx || dy) && es->dndWindow)
           {
              int vx, vy;
              defaultViewportForWindow (w, &vx, &vy);

              int cx = es->newCursorX / s->width;
              int cy = es->newCursorY / s->height;

              int x = (vx - s->x) + ((cx - (w->attrib.x / s->width)) - vx);
              int y = (vy - s->y) + ((cy - (w->attrib.y / s->height)) - vy);

              if (es->moveToDesk || (w->state & (MAXIMIZE_STATE | CompWindowStateFullscreenMask)) ||
                  ((MOD(w->attrib.x, s->width) == 0) &&
                   (MOD(w->attrib.y, s->height) == 0) &&
                   (w->attrib.width == s->width) &&
                   (w->attrib.height == s->height)))
                {
                   if(x || y)
                     {
                        dx = (x * w->screen->width);
                        dy = (y * w->screen->height);
                     }
                   else
                     {
                        moved = 0;
                     }
                }

              if(moved)
                {
                   int abs_x = w->attrib.x + (s->x * s->width);
                   int abs_y = w->attrib.y + (s->y * s->height);

                   if (abs_x + dx <= 0) dx = -abs_x;
                   if (abs_y + dy <= 0) dy = -abs_y;

                   if (abs_x + w->attrib.width + dx >= s->hsize * s->width)
                     dx = (s->hsize * s->width) - (abs_x + w->attrib.width);
                   if (abs_y + w->attrib.height + dy >= s->vsize * s->height)
                     dy = (s->vsize * s->height) - (abs_y + w->attrib.height);

                   moveWindow (es->dndWindow, dx, dy, TRUE, FALSE);              // TRUE,FALSE

     //expoGetExpoImmediateMove (s->display));
                }
           }
         if(moved)
           {
              es->prevCursorX = es->newCursorX;
              es->prevCursorY = es->newCursorY;
           }

         damageScreen (s);
      }
      break;

      case DnDStart:
      {
         int origView = s->x;
         int origViewY = s->y;
         CompWindow *w;

         moveScreenViewport (s, s->x, s->y, FALSE);

         for (w = s->reverseWindows; w; w = w->prev)
           {
              if (w->attrib.class == InputOnly) continue;

              Bool inWindow;
              int xOffset, yOffset;

              if (w->destroyed)
                continue;

              if (!w->shaded)
                {
                   if (w->attrib.map_state != IsViewable || !w->damaged)
                     continue;
                }

              if(!w->clientId) /* added */
                continue;

              xOffset = s->hsize * s->width;
              yOffset = s->vsize * s->height;

              inWindow = ((es->newCursorX >= WIN_X (w)) &&
                          (es->newCursorX <= WIN_X (w) + WIN_W (w))) ||
                ((es->newCursorX >= (WIN_X (w) + xOffset)) &&
                 (es->newCursorX <= (WIN_X (w) + WIN_W (w) +
                                     xOffset)));

              inWindow &= ((es->newCursorY >= WIN_Y (w)) &&
                           (es->newCursorY <= WIN_Y (w) + WIN_H (w))) ||
                ((es->newCursorY >= (WIN_Y (w) + yOffset)) &&
                 (es->newCursorY <= (WIN_Y (w) + WIN_H (w) +
                                     yOffset)));

              if (!inWindow)
                continue;

              // (*s->windowGrabNotify) (w); /* XXX this disables wobbly i
              // guess*/
              es->dndState = DnDDuring;
              es->dndWindow = w;

              break;
           }

         if (w)
           {
              raiseWindow (es->dndWindow);
              activateWindow(es->dndWindow);
           }
         else
           {
              /* no window was hovered */
               es->dndState = DnDNone;
           }

         moveScreenViewport (s, -origView, -origViewY, FALSE);

         es->prevCursorX = es->newCursorX;
         es->prevCursorY = es->newCursorY;
      }
      break;

      default:
        break;
     }
}

static Bool
expoInitDisplay(CompPlugin  *p,
                CompDisplay *d)
{
   ExpoDisplay *ed;

   ed = malloc (sizeof (ExpoDisplay));

   if (!ed)
     return FALSE;

   ed->screenPrivateIndex = allocateScreenPrivateIndex (d);

   if (ed->screenPrivateIndex < 0)
     {
        free (ed);
        return FALSE;
     }

   /* expoSetExpoInitiate (d, expoExpo);
    * expoSetExpoTerminate (d, expoTermExpo); */

   WRAP (ed, d, handleEvent, expoHandleEvent);
   d->privates[displayPrivateIndex].ptr = ed;

   return TRUE;
}

static void
expoFiniDisplay(CompPlugin  *p,
                CompDisplay *d)
{
   EXPO_DISPLAY (d);

   UNWRAP (ed, d, handleEvent);

   freeScreenPrivateIndex (d, ed->screenPrivateIndex);
   free (ed);
}

static Bool
expoInitScreen(CompPlugin *p,
               CompScreen *s)
{
   ExpoScreen *es;

   EXPO_DISPLAY (s->display);

   es = malloc (sizeof (ExpoScreen));

   if (!es)
     return FALSE;

   es->anyClick = FALSE;
   es->vpUpdateMode = VPUpdateNone;

   es->selectedVX = es->origVX = s->x;
   es->selectedVY = es->origVY = s->y;

   es->grabIndex = 0;

   es->expoCam = 0.0f;
   es->expoMode = 0;

   es->dndState = DnDNone;
   es->dndWindow = NULL;

   es->clickTime = 0;
   es->doubleClick = FALSE;

   WRAP (es, s, paintOutput, expoPaintOutput);
   WRAP (es, s, paintScreen, expoPaintScreen);
   WRAP (es, s, donePaintScreen, expoDonePaintScreen);
   WRAP (es, s, paintTransformedOutput, expoPaintTransformedOutput);
   WRAP (es, s, preparePaintScreen, expoPreparePaintScreen);
   WRAP (es, s, drawWindow, expoDrawWindow);
   WRAP (es, s, damageWindowRect, expoDamageWindowRect);
   WRAP (es, s, paintWindow, expoPaintWindow);

   s->privates[ed->screenPrivateIndex].ptr = es;

   return TRUE;
}

static void
expoFiniScreen(CompPlugin *p,
               CompScreen *s)
{
   EXPO_SCREEN (s);

   if (es->grabIndex)
     {
        removeScreenGrab (s, es->grabIndex, 0);
        es->grabIndex = 0;
     }

   UNWRAP (es, s, paintOutput);
   UNWRAP (es, s, paintScreen);
   UNWRAP (es, s, donePaintScreen);
   UNWRAP (es, s, paintTransformedOutput);
   UNWRAP (es, s, preparePaintScreen);
   UNWRAP (es, s, drawWindow);
   UNWRAP (es, s, damageWindowRect);
   UNWRAP (es, s, paintWindow);

   free (es);
}

static Bool
expoInit(CompPlugin *p)
{
   displayPrivateIndex = allocateDisplayPrivateIndex();

   if (displayPrivateIndex < 0)
     return FALSE;

   return TRUE;
}

static void
expoFini(CompPlugin *p)
{
   freeDisplayPrivateIndex (displayPrivateIndex);
}

static int
expoGetVersion(CompPlugin *p,
               int         version)
{
   return ABIVERSION;
}

CompPluginVTable expoVTable = {
   "expo",
   expoGetVersion,
   0,
   expoInit,
   expoFini,
   expoInitDisplay,
   expoFiniDisplay,
   expoInitScreen,
   expoFiniScreen,
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
   return &expoVTable;
}

