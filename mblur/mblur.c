/*
 * Ecomp motion blur effect plugin
 *
 * mblur.c
 *
 * Copyright : (C) 2007 by Dennis Kasprzyk
 * E-mail    : onestone@beryl-project.org
 *
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
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <X11/Xatom.h>
#include <X11/extensions/Xrender.h>
#include <sys/stat.h>
#include <unistd.h>

#include <ecomp.h>

#include "mblur_options.h"

#define GET_MBLUR_DISPLAY(d) \
  ((MblurDisplay *)(d)->privates[displayPrivateIndex].ptr)

#define MBLUR_DISPLAY(d) \
  MblurDisplay * md = GET_MBLUR_DISPLAY (d)

#define GET_MBLUR_SCREEN(s, md) \
  ((MblurScreen *)(s)->privates[(md)->screenPrivateIndex].ptr)

#define MBLUR_SCREEN(s) \
  MblurScreen * ms = GET_MBLUR_SCREEN (s, GET_MBLUR_DISPLAY (s->display))

static int displayPrivateIndex = 0;

typedef struct _MblurDisplay
{
   int screenPrivateIndex;
}
MblurDisplay;

typedef struct _MblurScreen
{
   /* functions that we will intercept */
    PreparePaintScreenProc     preparePaintScreen;
    PaintScreenProc            paintScreen;
    PaintTransformedOutputProc paintTransformedOutput;

    Bool                       active;
    Bool                       update; /* is an update of the motion blut texture needed */

    float                      alpha; /* motion blur blending value */
    float                      timer; /* motion blur fadeout time */
    Bool                       activated;

    GLuint                     texture;
}
MblurScreen;

/* activate/deactivate motion blur */

static Bool
mblurToggle(CompDisplay    *d,
            CompAction     *ac,
            CompActionState state,
            CompOption     *option,
            int             nOption)
{
   CompScreen *s;

   s = findScreenAtDisplay (d, getIntOptionNamed (option, nOption, "root", 0));

   if (s)
     {
        MBLUR_SCREEN (s);
        ms->activated = !ms->activated;
     }

   return FALSE;
}

static void
mblurPreparePaintScreen(CompScreen *s,
                        int         msec)
{
   MBLUR_SCREEN (s);

   ms->active |= ms->activated;

   /* fade motion blur out if no longer active */

   if (ms->activated)
     {
        ms->timer = 500;
     }
   else
     {
        ms->timer -= msec;
     }

   // calculate motion blur strength dependent on framerate
   float val = 101 - MIN (100, MAX (1, msec));
   float a_val = mblurGetStrength (s) / 20.0;

   a_val = a_val * a_val;
   a_val /= 100.0;

   ms->alpha = 1.0 - pow (a_val, 1.0 / val);

   if (ms->active && ms->timer <= 0)
     damageScreen (s);

   if (ms->timer <= 0)
     {
        ms->active = FALSE;
     }

   if (ms->update && ms->active)
     damageScreen (s);

   UNWRAP (ms, s, preparePaintScreen);
   (*s->preparePaintScreen)(s, msec);
   WRAP (ms, s, preparePaintScreen, mblurPreparePaintScreen);
}

static void
mblurPaintScreen(CompScreen  *s,
                 CompOutput  *outputs,
                 int          numOutput,
                 unsigned int mask)
{
   MBLUR_SCREEN (s);

   if (!ms->active)
     ms->update = TRUE;

   UNWRAP (ms, s, paintScreen);
   (*s->paintScreen)(s, outputs, numOutput, mask);
   WRAP (ms, s, paintScreen, mblurPaintScreen);

   Bool enable_scissor = FALSE;

   if (ms->active && glIsEnabled (GL_SCISSOR_TEST))
     {
        glDisable (GL_SCISSOR_TEST);
        enable_scissor = TRUE;
     }

   if (ms->active && mblurGetMode (s) == ModeTextureCopy)
     {
        float tx, ty;
        GLuint target;

        if (s->textureNonPowerOfTwo ||
            (POWER_OF_TWO (s->width) && POWER_OF_TWO (s->height)))
          {
             target = GL_TEXTURE_2D;
             tx = 1.0f / s->width;
             ty = 1.0f / s->height;
          }
        else
          {
             target = GL_TEXTURE_RECTANGLE_NV;
             tx = 1;
             ty = 1;
          }

        if (!ms->texture)
          {
             glGenTextures (1, &ms->texture);
             glBindTexture (target, ms->texture);

             glTexParameteri (target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
             glTexParameteri (target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

             glTexParameteri (target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
             glTexParameteri (target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

             glBindTexture (target, 0);
          }

        // blend motion blur texture to screen
        glPushAttrib (GL_COLOR_BUFFER_BIT | GL_TEXTURE_BIT | GL_VIEWPORT_BIT);
        glPushMatrix ();
        glLoadIdentity ();

        glViewport (0, 0, s->width, s->height);
        glTranslatef (-0.5f, -0.5f, -DEFAULT_Z_CAMERA);
        glScalef (1.0f / s->width, -1.0f / s->height, 1.0f);
        glTranslatef (0.0f, -s->height, 0.0f);
        glBindTexture (target, ms->texture);
        glEnable (target);

        if (!ms->update)
          {
             glEnable (GL_BLEND);

             glBlendFunc (GL_ONE_MINUS_SRC_ALPHA, GL_SRC_ALPHA);
             ms->alpha = (ms->timer / 500.0) *
               ms->alpha + (1.0 - (ms->timer / 500.0)) * 0.5;

             glColor4f (1, 1, 1, ms->alpha);

             glTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

             glBegin (GL_QUADS);
             glTexCoord2f (0, s->height * ty);
             glVertex2f (0, 0);
             glTexCoord2f (0, 0);
             glVertex2f (0, s->height);
             glTexCoord2f (s->width * tx, 0);
             glVertex2f (s->width, s->height);
             glTexCoord2f (s->width * tx, s->height * ty);
             glVertex2f (s->width, 0);
             glEnd ();

             glTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

             glBlendFunc (GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

             glDisable (GL_BLEND);

             // copy new screen to motion blur texture
             glCopyTexSubImage2D (target, 0, 0, 0, 0, 0, s->width, s->height);
          }
        else
          {
             glCopyTexImage2D (target, 0, GL_RGB, 0, 0,
                               s->width, s->height, 0);
          }

        glBindTexture (target, 0);

        glDisable (target);

        glPopMatrix ();
        glPopAttrib ();

        ms->update = FALSE;
        damageScreen (s);
     }

   if (ms->active && mblurGetMode (s) == ModeAccumulationBuffer)
     {
        // create motion blur effect using accumulation buffer
          ms->alpha = (ms->timer / 500.0) *
            ms->alpha + (1.0 - (ms->timer / 500.0)) * 0.5;

          if (ms->update)
            {
               glAccum (GL_LOAD, 1.0);
            }
          else
            {
               glAccum (GL_MULT, 1.0 - ms->alpha);
               glAccum (GL_ACCUM, ms->alpha);
               glAccum (GL_RETURN, 1.0);
            }

          ms->update = FALSE;

          damageScreen (s);
     }

   if (enable_scissor)
     glEnable (GL_SCISSOR_TEST);
}

static void
mblurPaintTransformedOutput(CompScreen              *s,
                            const ScreenPaintAttrib *sa,
                            const CompTransform     *transform,
                            Region                   region,
                            CompOutput              *output,
                            unsigned int             mask)
{
   MBLUR_SCREEN (s);

   if (mblurGetOnTransformedScreen (s) &&
       (mask & PAINT_SCREEN_TRANSFORMED_MASK))
     {
        ms->active = TRUE;
        ms->timer = 500;
     }

   UNWRAP (ms, s, paintTransformedOutput);
   (*s->paintTransformedOutput)(s, sa, transform, region, output, mask);
   WRAP (ms, s, paintTransformedOutput, mblurPaintTransformedOutput);
}

static Bool
mblurInit(CompPlugin *p)
{
   displayPrivateIndex = allocateDisplayPrivateIndex ();

   if (displayPrivateIndex < 0)
     return FALSE;

   return TRUE;
}

static void
mblurFini(CompPlugin *p)
{
   if (displayPrivateIndex >= 0)
     freeDisplayPrivateIndex (displayPrivateIndex);
}

static Bool
mblurInitDisplay(CompPlugin  *p,
                 CompDisplay *d)
{
   /* Generate a blur display */
    MblurDisplay *md = (MblurDisplay *)calloc (1, sizeof (MblurDisplay));

    /* Allocate a private index */
    md->screenPrivateIndex = allocateScreenPrivateIndex (d);

    /* Check if its valid */
    if (md->screenPrivateIndex < 0)
      {
         /* It's invalid so free memory and return */
          free (md);
          return FALSE;
      }

    /* Record the display */
    d->privates[displayPrivateIndex].ptr = md;

    mblurSetInitiateInitiate (d, mblurToggle);

    return TRUE;
}

static void
mblurFiniDisplay(CompPlugin  *p,
                 CompDisplay *d)
{
   MBLUR_DISPLAY (d);

   /*Free the private index */
   freeScreenPrivateIndex (d, md->screenPrivateIndex);

   /*Free the pointer */
   free (md);
}

static Bool
mblurInitScreen(CompPlugin *p,
                CompScreen *s)
{
   MBLUR_DISPLAY (s->display);

   /* Create a blur screen */
   MblurScreen *ms = (MblurScreen *)calloc (1, sizeof (MblurScreen));

   s->privates[md->screenPrivateIndex].ptr = ms;

   ms->activated = FALSE;
   ms->update = TRUE;
   ms->texture = 0;

   /* Take over the window draw function */
   WRAP (ms, s, paintScreen, mblurPaintScreen);
   WRAP (ms, s, preparePaintScreen, mblurPreparePaintScreen);
   WRAP (ms, s, paintTransformedOutput, mblurPaintTransformedOutput);

   damageScreen (s);

   return TRUE;
}

static void
mblurFiniScreen(CompPlugin *p,
                CompScreen *s)
{
   MBLUR_SCREEN (s);

   if (ms->texture)
     glDeleteTextures (1, &ms->texture);

   /* restore the original function */
   UNWRAP (ms, s, paintScreen);
   UNWRAP (ms, s, preparePaintScreen);
   UNWRAP (ms, s, paintTransformedOutput);

   /* free the screen pointer */
   free (ms);
}

static int
mblurGetVersion(CompPlugin *plugin,
                int         version)
{
   return ABIVERSION;
}

CompPluginVTable mblurVTable = {
   "mblur",
   mblurGetVersion,
   0,
   mblurInit,
   mblurFini,
   mblurInitDisplay,
   mblurFiniDisplay,
   mblurInitScreen,
   mblurFiniScreen,
   0,
   0,
   0,
   0,
   0,
   0
};

CompPluginVTable *
getCompPluginInfo(void)
{
   return &mblurVTable;
}

