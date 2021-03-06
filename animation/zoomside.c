/*
 * Animation plugin for ecomp/beryl
 *
 * animation.c
 *
 * Copyright : (C) 2006 Erkin Bahceci
 * E-mail    : erkinbah@gmail.com
 *
 * Based on Wobbly and Minimize plugins by
 *           : David Reveman
 * E-mail    : davidr@novell.com>
 *
 * Particle system added by : (C) 2006 Dennis Kasprzyk
 * E-mail                   : onestone@beryl-project.org
 *
 * Beam-Up added by : Florencio Guimaraes
 * E-mail           : florencio@nexcorp.com.br
 *
 * Hexagon tessellator added by : Mike Slegeir
 * E-mail                       : mikeslegeir@mail.utexas.edu>
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "animation-internal.h"

// =====================  Effect: Zoom and Sidekick  =========================

void
fxSidekickInit(CompScreen *s, CompWindow *w)
{
   ANIM_SCREEN(s);
   ANIM_WINDOW(w);

   // determine number of rotations randomly in [0.75, 1.25] range
   aw->numZoomRotations =
     animGetF(as, aw, ANIM_SCREEN_OPTION_SIDEKICK_NUM_ROTATIONS) *
     (1.0f + 0.2f * rand() / RAND_MAX - 0.1f);

   fxZoomInit(s, w);
}

static float
fxZoomGetSpringiness(AnimScreen *as, AnimWindow *aw)
{
   if (aw->curAnimEffect == AnimEffectZoom)
     return 2 * animGetF(as, aw, ANIM_SCREEN_OPTION_ZOOM_SPRINGINESS);
   else if (aw->curAnimEffect == AnimEffectSidekick)
     return 1.6 * animGetF(as, aw, ANIM_SCREEN_OPTION_SIDEKICK_SPRINGINESS);
   else
     return 0.0f;
}

void
fxZoomInit(CompScreen *s, CompWindow *w)
{
   ANIM_SCREEN(s);
   ANIM_WINDOW(w);

   if ((aw->curAnimEffect == AnimEffectSidekick &&
        (animGetI(as, aw, ANIM_SCREEN_OPTION_SIDEKICK_ZOOM_FROM_CENTER) ==
         ZoomFromCenterOn ||
         ((aw->curWindowEvent == WindowEventMinimize ||
           aw->curWindowEvent == WindowEventUnminimize) &&
           animGetI(as, aw, ANIM_SCREEN_OPTION_SIDEKICK_ZOOM_FROM_CENTER) ==
           ZoomFromCenterMin) ||
         ((aw->curWindowEvent == WindowEventOpen ||
           aw->curWindowEvent == WindowEventClose) &&
           animGetI(as, aw, ANIM_SCREEN_OPTION_SIDEKICK_ZOOM_FROM_CENTER) ==
           ZoomFromCenterCreate))) ||
       (aw->curAnimEffect == AnimEffectZoom &&
        (animGetI(as, aw, ANIM_SCREEN_OPTION_ZOOM_FROM_CENTER) ==
          ZoomFromCenterOn ||
          ((aw->curWindowEvent == WindowEventMinimize ||
            aw->curWindowEvent == WindowEventUnminimize) &&
           animGetI(as, aw, ANIM_SCREEN_OPTION_ZOOM_FROM_CENTER) ==
           ZoomFromCenterMin) ||
          ((aw->curWindowEvent == WindowEventOpen ||
            aw->curWindowEvent == WindowEventClose) &&
           animGetI(as, aw, ANIM_SCREEN_OPTION_ZOOM_FROM_CENTER) ==
           ZoomFromCenterCreate))))
     {
        aw->icon.x =
          WIN_X(w) + WIN_W(w) / 2 - aw->icon.width / 2;
        aw->icon.y =
          WIN_Y(w) + WIN_H(w) / 2 - aw->icon.height / 2;
     }

   // allow extra time for spring damping / deceleration
   if ((aw->curWindowEvent == WindowEventUnminimize ||
        aw->curWindowEvent == WindowEventOpen) &&
       fxZoomGetSpringiness(as, aw) > 1e-4)
     {
        aw->animTotalTime /= SPRINGY_ZOOM_PERCEIVED_T;
     }
   else if ((aw->curAnimEffect == AnimEffectZoom ||
             aw->curAnimEffect == AnimEffectSidekick) &&
            (aw->curWindowEvent == WindowEventOpen ||
             aw->curWindowEvent == WindowEventClose))
     {
        aw->animTotalTime /= NONSPRINGY_ZOOM_PERCEIVED_T;
     }
   else
     {
        aw->animTotalTime /= ZOOM_PERCEIVED_T;
     }
   aw->animRemainingTime = aw->animTotalTime;

   defaultAnimInit(s, w);
}

void
fxZoomAnimProgress(AnimScreen *as,
                   AnimWindow *aw,
                   float      *moveProgress,
                   float      *scaleProgress,
                   Bool        neverSpringy)
{
   float forwardProgress =
     1 - aw->animRemainingTime /
     (aw->animTotalTime - aw->timestep);
   forwardProgress = MIN(forwardProgress, 1);
   forwardProgress = MAX(forwardProgress, 0);

   float x = forwardProgress;
   Bool backwards = FALSE;
   int animProgressDir = 1;

   if (aw->curWindowEvent == WindowEventUnminimize ||
       aw->curWindowEvent == WindowEventOpen)
     animProgressDir = 2;
   if (aw->animOverrideProgressDir != 0)
     animProgressDir = aw->animOverrideProgressDir;
   if ((animProgressDir == 1 &&
        (aw->curWindowEvent == WindowEventUnminimize ||
         aw->curWindowEvent == WindowEventOpen)) ||
       (animProgressDir == 2 &&
        (aw->curWindowEvent == WindowEventMinimize ||
          aw->curWindowEvent == WindowEventClose)))
     backwards = TRUE;
   if (backwards)
     x = 1 - x;

   float dampBase = (pow(1 - pow(x, 1.2) * 0.5, 10) - pow(0.5, 10)) / (1 - pow(0.5, 10));
   float nonSpringyProgress =
     1 - pow(decelerateProgressCustom(1 - x, .5f, .8f), 1.7f);

   if (moveProgress && scaleProgress)
     {
        float damping =
          pow(dampBase, 0.5);

        float damping2 =
          ((pow(1 - (pow(x, 0.7) * 0.5), 10) - pow(0.5, 10)) / (1 - pow(0.5, 10))) *
          0.7 + 0.3;
        float springiness = 0;

        // springy only when appearing
        if ((aw->curWindowEvent == WindowEventUnminimize ||
             aw->curWindowEvent == WindowEventOpen) &&
            !neverSpringy)
          {
             springiness = fxZoomGetSpringiness(as, aw);
          }

        float springyMoveProgress =
          cos(2 * M_PI * pow(x, 1) * 1.25) * damping * damping2;

        if (springiness > 1e-4f)
          {
             if (x > 0.2)
               {
                  springyMoveProgress *= springiness;
               }
             else
               {
     // interpolate between (springyMoveProgress * springiness)
                  // and springyMoveProgress for smooth transition at 0.2
     // (where it crosses y=0)
                        float progressUpto02 = x / 0.2f;
                        springyMoveProgress =
                          (1 - progressUpto02) * springyMoveProgress +
                          progressUpto02 * springyMoveProgress * springiness;
               }
             *moveProgress = 1 - springyMoveProgress;
          }
        else
          {
             *moveProgress = nonSpringyProgress;
          }
        if (aw->curWindowEvent == WindowEventUnminimize ||
            aw->curWindowEvent == WindowEventOpen)
          *moveProgress = 1 - *moveProgress;
        if (backwards)
          *moveProgress = 1 - *moveProgress;

        float scProgress = nonSpringyProgress;
        if (aw->curWindowEvent == WindowEventUnminimize ||
            aw->curWindowEvent == WindowEventOpen)
          scProgress = 1 - scProgress;
        if (backwards)
          scProgress = 1 - scProgress;

        *scaleProgress =
          pow(scProgress, 1.25);
     }
}

void
fxZoomUpdateWindowAttrib(AnimScreen        *as,
                         CompWindow        *w,
                         WindowPaintAttrib *wAttrib)
{
   ANIM_WINDOW(w);

   float forwardProgress;
   float dummy;

   fxZoomAnimProgress(as, aw, &dummy, &forwardProgress, FALSE);

   wAttrib->opacity =
     (GLushort)(aw->storedOpacity * (1 - forwardProgress));
}

void
applyZoomTransform(CompWindow *w, CompTransform *transform)
{
   ANIM_SCREEN(w->screen);
   ANIM_WINDOW(w);

   Point winCenter =
   {(WIN_X(w) + WIN_W(w) / 2.0),
    (WIN_Y(w) + WIN_H(w) / 2.0)};
   Point iconCenter =
   {aw->icon.x + aw->icon.width / 2.0,
    aw->icon.y + aw->icon.height / 2.0};
   Point winSize =
   {WIN_W(w), WIN_H(w)};
   winSize.x = (winSize.x == 0 ? 1 : winSize.x);
   winSize.y = (winSize.y == 0 ? 1 : winSize.y);

   float scaleProgress;
   float moveProgress;
   float rotateProgress = 0;

   if (aw->curAnimEffect == AnimEffectSidekick)
     {
        fxZoomAnimProgress(as, aw, &moveProgress, &scaleProgress, FALSE);
        rotateProgress = moveProgress;
     }
   else if (aw->curAnimEffect == AnimEffectZoom)
     {
        fxZoomAnimProgress(as, aw, &moveProgress, &scaleProgress, FALSE);
     }
   else
     {
        // other effects use this for minimization
          fxZoomAnimProgress(as, aw, &moveProgress, &scaleProgress, TRUE);
     }

   Point curCenter =
   {(1 - moveProgress) * winCenter.x + moveProgress * iconCenter.x,
    (1 - moveProgress) * winCenter.y + moveProgress * iconCenter.y};
   Point curScale =
   {((1 - scaleProgress) * winSize.x + scaleProgress * aw->icon.width) /
    winSize.x,
    ((1 - scaleProgress) * winSize.y + scaleProgress * aw->icon.height) /
    winSize.y};

   if (fxZoomGetSpringiness(as, aw) == 0.0f &&
       (aw->curAnimEffect == AnimEffectZoom ||
        aw->curAnimEffect == AnimEffectSidekick) &&
       (aw->curWindowEvent == WindowEventOpen ||
        aw->curWindowEvent == WindowEventClose))
     {
        matrixTranslate (transform,
                         iconCenter.x, iconCenter.y, 0);
        matrixScale (transform, curScale.x, curScale.y, 1.0f);
        matrixTranslate (transform,
                         -iconCenter.x, -iconCenter.y, 0);

        if (aw->curAnimEffect == AnimEffectSidekick)
          {
             matrixTranslate (transform, winCenter.x, winCenter.y, 0);
             matrixRotate (transform, rotateProgress * 360 * aw->numZoomRotations,
                           0.0f, 0.0f, 1.0f);
             matrixTranslate (transform, -winCenter.x, -winCenter.y, 0);
          }
     }
   else
     {
        matrixTranslate (transform, winCenter.x, winCenter.y, 0);
        float tx, ty;
        if (aw->curAnimEffect != AnimEffectZoom)
          {
             // avoid parallelogram look
               float maxScale = MAX(curScale.x, curScale.y);
               matrixScale (transform, maxScale, maxScale, 1.0f);
               tx = (curCenter.x - winCenter.x) / maxScale;
               ty = (curCenter.y - winCenter.y) / maxScale;
          }
        else
          {
             matrixScale (transform, curScale.x, curScale.y, 1.0f);
             tx = (curCenter.x - winCenter.x) / curScale.x;
             ty = (curCenter.y - winCenter.y) / curScale.y;
          }
        matrixTranslate (transform, tx, ty, 0);
        if (aw->curAnimEffect == AnimEffectSidekick)
          {
             matrixRotate (transform, rotateProgress * 360 * aw->numZoomRotations,
                           0.0f, 0.0f, 1.0f);
          }
        matrixTranslate (transform, -winCenter.x, -winCenter.y, 0);
     }
}

void
fxZoomUpdateWindowTransform(CompScreen    *s,
                            CompWindow    *w,
                            CompTransform *wTransform)
{
   ANIM_WINDOW(w);

   // Apply transform to wTransform
   matmul4 (wTransform->m, wTransform->m, aw->transform.m);
}

