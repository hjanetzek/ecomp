/*
 *
 * Ecomp scale window title filter plugin
 *
 * scalefilter.c
 *
 * Copyright : (C) 2007 by Danny Baumann
 * E-mail    : maniac@opencompositing.org
 *
 * Copyright : (C) 2006 Diogo Ferreira
 * E-mail    : diogo@underdev.org
 *
 * Rounded corner drawing taken from wall.c:
 * Copyright : (C) 2007 Robert Carr
 * E-mail    : racarr@beryl-project.org
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
 *
 */

#define _GNU_SOURCE
#include <math.h>
#include <string.h>
#include <wchar.h>
#include <locale.h>

#include <X11/Xlib.h>
#include <X11/keysymdef.h>

#include <ecomp.h>
#include <scale.h>
#include <text.h>

#include "scalefilter_options.h"

static int displayPrivateIndex;
static int scaleDisplayPrivateIndex;

#define MAX_FILTER_SIZE 32
#define MAX_FILTER_STRING_LEN (MAX_FILTER_SIZE+1)
#define MAX_FILTER_TEXT_LEN (MAX_FILTER_SIZE+8)

typedef struct _ScaleFilterInfo {
    CompTimeoutHandle timeoutHandle;

    Pixmap      textPixmap;
    CompTexture textTexture;

    unsigned int outputDevice;

    int textWidth;
    int textHeight;

    CompMatch match;
    CompMatch *origMatch;

    wchar_t filterString[2 * MAX_FILTER_STRING_LEN];
    int     filterStringLength;
} ScaleFilterInfo;

typedef struct _ScaleFilterDisplay {
    int screenPrivateIndex;

    XIM xim;
    XIC xic;

    HandleEventProc       handleEvent;
    HandleEcompEventProc handleEcompEvent;
} ScaleFilterDisplay;

typedef struct _ScaleFilterScreen {
    PaintOutputProc                   paintOutput;
    ScaleSetScaledPaintAttributesProc setScaledPaintAttributes;

    CompMatch scaleMatch;
    Bool matchApplied;

    ScaleFilterInfo *filterInfo;
} ScaleFilterScreen;

#define GET_FILTER_DISPLAY(d)				          \
    ((ScaleFilterDisplay *) (d)->privates[displayPrivateIndex].ptr)

#define FILTER_DISPLAY(d)		          \
    ScaleFilterDisplay *fd = GET_FILTER_DISPLAY (d)

#define GET_FILTER_SCREEN(s, fd)				              \
    ((ScaleFilterScreen *) (s)->privates[(fd)->screenPrivateIndex].ptr)

#define FILTER_SCREEN(s)						               \
    ScaleFilterScreen *fs = GET_FILTER_SCREEN (s, GET_FILTER_DISPLAY (s->display))

static void
scalefilterFreeFilterText (CompScreen *s)
{
    FILTER_SCREEN (s);

    if (!fs->filterInfo)
	return;

    if (!fs->filterInfo->textPixmap)
	return;

    releasePixmapFromTexture(s, &fs->filterInfo->textTexture);
    XFreePixmap (s->display->display, fs->filterInfo->textPixmap);
    initTexture (s, &fs->filterInfo->textTexture);
    fs->filterInfo->textPixmap = None;
}

static void
scalefilterRenderFilterText (CompScreen *s)
{
    CompDisplay    *d = s->display;
    CompTextAttrib tA;
    int            stride;
    void*          data;
    int            x1, x2, y1, y2;
    int            width, height;
    REGION         reg;
    char           buffer[2 * MAX_FILTER_STRING_LEN];

    FILTER_SCREEN (s);

    if (!fs->filterInfo)
	return;

    x1 = s->outputDev[fs->filterInfo->outputDevice].region.extents.x1;
    x2 = s->outputDev[fs->filterInfo->outputDevice].region.extents.x2;
    y1 = s->outputDev[fs->filterInfo->outputDevice].region.extents.y1;
    y2 = s->outputDev[fs->filterInfo->outputDevice].region.extents.y2;

    reg.rects    = &reg.extents;
    reg.numRects = 1;

    /* damage the old draw rectangle */
    width  = fs->filterInfo->textWidth + (2 * scalefilterGetBorderSize (s));
    height = fs->filterInfo->textHeight + (2 * scalefilterGetBorderSize (s));

    reg.extents.x1 = x1 + ((x2 - x1) / 2) - (width / 2) - 1;
    reg.extents.x2 = reg.extents.x1 + width + 1;
    reg.extents.y1 = y1 + ((y2 - y1) / 2) - (height / 2) - 1;
    reg.extents.y2 = reg.extents.y1 + height + 1;

    damageScreenRegion (s, &reg);

    scalefilterFreeFilterText (s);

    if (!scalefilterGetFilterDisplay (s))
	return;

    if (fs->filterInfo->filterStringLength == 0)
	return;

    tA.maxwidth = x2 - x1 - (2 * scalefilterGetBorderSize (s));
    tA.maxheight = y2 - y1 - (2 * scalefilterGetBorderSize (s));
    tA.screen = s;
    tA.size = scalefilterGetFontSize (s);
    tA.color[0] = scalefilterGetFontColorRed (s);
    tA.color[1] = scalefilterGetFontColorGreen (s);
    tA.color[2] = scalefilterGetFontColorBlue (s);
    tA.color[3] = scalefilterGetFontColorAlpha (s);
    tA.style = (scalefilterGetFontBold (s)) ?
	       TEXT_STYLE_BOLD : TEXT_STYLE_NORMAL;
    tA.family = "Sans";
    tA.ellipsize = TRUE;

    wcstombs (buffer, fs->filterInfo->filterString, MAX_FILTER_STRING_LEN);
    tA.renderMode = TextRenderNormal;
    tA.data = (void*)buffer;

    if ((*d->fileToImage) (s->display, TEXT_ID, (char *)&tA,
			   &fs->filterInfo->textWidth,
			   &fs->filterInfo->textHeight,
			   &stride, &data))
    {
	fs->filterInfo->textPixmap = (Pixmap)data;
	if (!bindPixmapToTexture (s, &fs->filterInfo->textTexture,
				  fs->filterInfo->textPixmap,
				  fs->filterInfo->textWidth,
				  fs->filterInfo->textHeight, 32))
	{
	    compLogMessage (d, "scalefilterinfo", CompLogLevelError,
			    "Bind Pixmap to Texture failure");
	    XFreePixmap (d->display, fs->filterInfo->textPixmap);
	    fs->filterInfo->textPixmap = None;
	    return;
	}
    }
    else
    {
	fs->filterInfo->textPixmap = None;
	fs->filterInfo->textWidth = 0;
	fs->filterInfo->textHeight = 0;
    }

    /* damage the new draw rectangle */
    width  = fs->filterInfo->textWidth + (2 * scalefilterGetBorderSize (s));
    height = fs->filterInfo->textHeight + (2 * scalefilterGetBorderSize (s));

    reg.extents.x1 = x1 + ((x2 - x1) / 2) - (width / 2) - 1;
    reg.extents.x2 = reg.extents.x1 + width + 1;
    reg.extents.y1 = y1 + ((y2 - y1) / 2) - (height / 2) - 1;
    reg.extents.y2 = reg.extents.y1 + height + 1;

    damageScreenRegion (s, &reg);
}

static void
scalefilterDrawFilterText (CompScreen *s,
			   CompOutput *output)
{
    FILTER_SCREEN (s);

    GLboolean wasBlend;
    GLint     oldBlendSrc, oldBlendDst;
    int       ox1, ox2, oy1, oy2;

    float width = fs->filterInfo->textWidth;
    float height = fs->filterInfo->textHeight;
    float border = scalefilterGetBorderSize (s);

    ox1 = output->region.extents.x1;
    ox2 = output->region.extents.x2;
    oy1 = output->region.extents.y1;
    oy2 = output->region.extents.y2;
    float x = ox1 + ((ox2 - ox1) / 2) - (width / 2);
    float y = oy1 + ((oy2 - oy1) / 2) + (height / 2);

    x = floor (x);
    y = floor (y);

    wasBlend = glIsEnabled (GL_BLEND);
    glGetIntegerv (GL_BLEND_SRC, &oldBlendSrc);
    glGetIntegerv (GL_BLEND_DST, &oldBlendDst);

    if (!wasBlend)
	glEnable (GL_BLEND);

    glBlendFunc (GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

    glColor4us (scalefilterGetBackColorRed (s),
		scalefilterGetBackColorGreen (s),
		scalefilterGetBackColorBlue (s),
		scalefilterGetBackColorAlpha (s));

    glPushMatrix ();

    glTranslatef (x, y - height, 0.0f);
    glRectf (0.0f, height, width, 0.0f);
    glRectf (0.0f, 0.0f, width, -border);
    glRectf (0.0f, height + border, width, height);
    glRectf (-border, height, 0.0f, 0.0f);
    glRectf (width, height, width + border, 0.0f);
    glTranslatef (-border, -border, 0.0f);

#define CORNER(a,b) \
    for (k = a; k < b; k++) \
    {\
	float rad = k* (3.14159 / 180.0f);\
	glVertex2f (0.0f, 0.0f);\
	glVertex2f (cos (rad) * border, sin (rad) * border);\
	glVertex2f (cos ((k - 1) * (3.14159 / 180.0f)) * border, \
		    sin ((k - 1) * (3.14159 / 180.0f)) * border);\
    }

    /* Rounded corners */
    int k;

    glTranslatef (border, border, 0.0f);
    glBegin (GL_TRIANGLES);
    CORNER (180, 270) glEnd();
    glTranslatef (-border, -border, 0.0f);

    glTranslatef (width + border, border, 0.0f);
    glBegin (GL_TRIANGLES);
    CORNER (270, 360) glEnd();
    glTranslatef (-(width + border), -border, 0.0f);

    glTranslatef (border, height + border, 0.0f);
    glBegin (GL_TRIANGLES);
    CORNER (90, 180) glEnd();
    glTranslatef (-border, -(height + border), 0.0f);

    glTranslatef (width + border, height + border, 0.0f);
    glBegin (GL_TRIANGLES);
    CORNER (0, 90) glEnd();
    glTranslatef (-(width + border), -(height + border), 0.0f);

    glPopMatrix ();

#undef CORNER

    glTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glColor4f (1.0, 1.0, 1.0, 1.0);

    enableTexture (s, &fs->filterInfo->textTexture, COMP_TEXTURE_FILTER_GOOD);

    CompMatrix *m = &fs->filterInfo->textTexture.matrix;

    glBegin (GL_QUADS);

    glTexCoord2f (COMP_TEX_COORD_X(m, 0),COMP_TEX_COORD_Y(m ,0));
    glVertex2f (x, y - height);
    glTexCoord2f (COMP_TEX_COORD_X(m, 0),COMP_TEX_COORD_Y(m, height));
    glVertex2f (x, y);
    glTexCoord2f (COMP_TEX_COORD_X(m, width),COMP_TEX_COORD_Y(m, height));
    glVertex2f (x + width, y);
    glTexCoord2f (COMP_TEX_COORD_X(m, width),COMP_TEX_COORD_Y(m, 0));
    glVertex2f (x + width, y - height);

    glEnd ();

    disableTexture (s, &fs->filterInfo->textTexture);
    glColor4usv (defaultColor);

    if (!wasBlend)
	glDisable (GL_BLEND);
    glBlendFunc (oldBlendSrc, oldBlendDst);
}

static void
scalefilterUpdateFilter (CompScreen *s,
	   		 CompMatch  *match)
{
    char         filterMatch[2 * MAX_FILTER_TEXT_LEN];
    unsigned int offset;

    FILTER_SCREEN (s);

    matchFini (match);
    matchInit (match);

    if (scalefilterGetFilterCaseInsensitive (s))
    {
	strncpy (filterMatch, "ititle=", MAX_FILTER_TEXT_LEN);
	offset = 7;
    }
    else
    {
    	strncpy (filterMatch, "title=", MAX_FILTER_TEXT_LEN);
	offset = 6;
    }

    wcstombs (filterMatch + offset, fs->filterInfo->filterString,
	      MAX_FILTER_STRING_LEN);
    matchAddExp (match, 0, filterMatch);
    matchAddGroup (match, MATCH_OP_AND_MASK, &fs->scaleMatch);
    matchUpdate (s->display, match);
}

static void
scalefilterRelayout (CompScreen *s)
{
    CompOption o[1];
    CompAction *action;

    SCALE_DISPLAY (s->display);

    sd->selectedWindow = None;
    sd->hoveredWindow = None;

    action = &sd->opt[SCALE_DISPLAY_OPTION_RELAYOUT].value.action;

    o[0].type    = CompOptionTypeInt;
    o[0].name    = "root";
    o[0].value.i = s->root;

    if (action->initiate)
    {
	if ((*action->initiate) (s->display, NULL, 0, o, 1))
	    damageScreen (s);
    }
}

static void
scalefilterInitFilterInfo (CompScreen *s)
{
    FILTER_SCREEN (s);
    SCALE_SCREEN (s);

    ScaleFilterInfo *info = fs->filterInfo;

    memset (info->filterString, 0, sizeof (info->filterString));
    info->filterStringLength = 0;

    info->textPixmap = None;
    info->textWidth  = 0;
    info->textHeight = 0;

    info->timeoutHandle = 0;

    info->outputDevice = s->currentOutputDev;

    initTexture (s, &info->textTexture);

    matchInit (&info->match);
    matchCopy (&info->match, &fs->scaleMatch);

    info->origMatch  = ss->currentMatch;
    ss->currentMatch = &info->match;
}

static void
scalefilterFiniFilterInfo (CompScreen *s,
			   Bool freeTimeout)
{
    FILTER_SCREEN (s);

    scalefilterFreeFilterText (s);

    matchFini (&fs->filterInfo->match);

    if (freeTimeout && fs->filterInfo->timeoutHandle)
	compRemoveTimeout (fs->filterInfo->timeoutHandle);

    free (fs->filterInfo);
    fs->filterInfo = NULL;
}

static Bool
scalefilterFilterTimeout (void *closure)
{
    CompScreen *s = (CompScreen *) closure;

    FILTER_SCREEN (s);
    SCALE_SCREEN (s);

    if (fs->filterInfo)
    {
	ss->currentMatch = fs->filterInfo->origMatch;
	scalefilterFiniFilterInfo (s, FALSE);
	scalefilterRelayout (s);
    }

    return FALSE;
}

static void
scalefilterHandleKeyPress (CompScreen *s,
			   XKeyEvent  *event)
{
    ScaleFilterInfo *info;
    Bool            needRelayout = FALSE;
    Bool            dropKeyEvent = FALSE;
    int             count, timeout;
    char            buffer[10];
    wchar_t         wbuffer[10];
    KeySym          ks;

    FILTER_DISPLAY (s->display);
    FILTER_SCREEN (s);
    SCALE_SCREEN (s);

    info = fs->filterInfo;
    memset (buffer, 0, sizeof (buffer));
    memset (wbuffer, 0, sizeof (wbuffer));

    if (fd->xic)
    {
	Status status;

	XSetICFocus (fd->xic);
	count = Xutf8LookupString (fd->xic, event, buffer, 9, &ks, &status);
	XUnsetICFocus (fd->xic);
    }
    else
    {
	count = XLookupString (event, buffer, 9, &ks, NULL);
    }

    mbstowcs (wbuffer, buffer, 9);

    if (ks == XK_Escape)
    {
	if (info)
	{
	    /* Escape key - drop current filter */
	    ss->currentMatch = info->origMatch;
	    scalefilterFiniFilterInfo (s, TRUE);
	    needRelayout = TRUE;
	    dropKeyEvent = TRUE;
	}
	else if (fs->matchApplied)
	{
	    /* remove filter applied previously
	       if currently not in input mode */
	    matchFini (&ss->match);
	    matchInit (&ss->match);
	    matchCopy (&ss->match, &fs->scaleMatch);
	    matchUpdate (s->display, &ss->match);

	    ss->currentMatch = &ss->match;
	    fs->matchApplied = FALSE;
	    needRelayout = TRUE;
	    dropKeyEvent = TRUE;
	}
    }
    else if (ks == XK_Return)
    {
	if (info)
	{
	    /* Return key - apply current filter persistently */
	    matchFini (&ss->match);
	    matchInit (&ss->match);
	    matchCopy (&ss->match, &info->match);
	    matchUpdate (s->display, &ss->match);

	    ss->currentMatch = &ss->match;
	    fs->matchApplied = TRUE;
	    dropKeyEvent = TRUE;
	    needRelayout = TRUE;
	    scalefilterFiniFilterInfo (s, TRUE);
	}
    }
    else if (ks == XK_BackSpace)
    {
	if (info && info->filterStringLength > 0)
	{
	    /* remove last character in string */
	    info->filterString[--(info->filterStringLength)] = '\0';
	    needRelayout = TRUE;
	}
    }
    else if (count > 0)
    {
	if (!info)
	{
	    fs->filterInfo = info = malloc (sizeof (ScaleFilterInfo));
	    scalefilterInitFilterInfo (s);
	}
	else if (info->timeoutHandle)
	    compRemoveTimeout (info->timeoutHandle);

	timeout = scalefilterGetTimeout (s);
	if (timeout > 0)
	    info->timeoutHandle = compAddTimeout (timeout,
				     		  scalefilterFilterTimeout, s);

	if (info->filterStringLength < MAX_FILTER_SIZE)
	{
	    info->filterString[info->filterStringLength++] = wbuffer[0];
	    info->filterString[info->filterStringLength] = '\0';
	    needRelayout = TRUE;
	}
    }

    /* set the event type invalid if we
       don't want other plugins see it */
    if (dropKeyEvent)
	event->type = LASTEvent+1;

    if (needRelayout)
    {
	scalefilterRenderFilterText (s);

	if (fs->filterInfo)
	    scalefilterUpdateFilter (s, &fs->filterInfo->match);

	scalefilterRelayout (s);
    }
}

static void
scalefilterHandleEvent (CompDisplay *d,
	 		XEvent      *event)
{
    FILTER_DISPLAY (d);

    switch (event->type)
    {
    case KeyPress:
    	{
	    CompScreen *s;
	    s = findScreenAtDisplay (d, event->xkey.root);
	    if (s)
	    {
	        SCALE_SCREEN (s);
		if (ss->grabIndex)
		{
		    XKeyEvent *keyEvent = (XKeyEvent *) event;
		    scalefilterHandleKeyPress (s, keyEvent);
		}
	    }
	}
	break;
    default:
	break;
    }

    UNWRAP (fd, d, handleEvent);
    (*d->handleEvent) (d, event);
    WRAP (fd, d, handleEvent, scalefilterHandleEvent);
}

static void
scalefilterHandleEcompEvent (CompDisplay *d,
	 		      char        *pluginName,
	 		      char        *eventName,
	 		      CompOption  *option,
	 		      int         nOption)
{
    FILTER_DISPLAY (d);

    UNWRAP (fd, d, handleEcompEvent);
    (*d->handleEcompEvent) (d, pluginName, eventName, option, nOption);
    WRAP (fd, d, handleEcompEvent, scalefilterHandleEcompEvent);

    if ((strcmp (pluginName, "scale") == 0) &&
	(strcmp (eventName, "activate") == 0))
    {
	Window xid = getIntOptionNamed (option, nOption, "root", 0);
	Bool activated = getBoolOptionNamed (option, nOption, "active", FALSE);
	CompScreen *s = findScreenAtDisplay (d, xid);

	if (s)
	{
	    FILTER_SCREEN (s);
	    SCALE_SCREEN (s);

	    if (activated)
	    {
		matchFini (&fs->scaleMatch);
		matchInit (&fs->scaleMatch);
		matchCopy (&fs->scaleMatch, ss->currentMatch);
		matchUpdate (d, &fs->scaleMatch);
		fs->matchApplied = FALSE;
	    }

	    if (!activated)
	    {
		if (fs->filterInfo)
		{
		    ss->currentMatch = fs->filterInfo->origMatch;
		    scalefilterFiniFilterInfo (s, TRUE);
		}
		fs->matchApplied = FALSE;
	    }
	}
    }
}

static Bool
scalefilterPaintOutput (CompScreen              *s,
			const ScreenPaintAttrib *sAttrib,
			const CompTransform     *transform,
			Region                  region,
			CompOutput              *output,
			unsigned int            mask)
{
    Bool status;

    FILTER_SCREEN (s);

    UNWRAP (fs, s, paintOutput);
    status = (*s->paintOutput) (s, sAttrib, transform, region, output, mask);
    WRAP (fs, s, paintOutput, scalefilterPaintOutput);

    if (status && fs->filterInfo)
    {
	if ((output->id == ~0 || output->id == fs->filterInfo->outputDevice) &&
	    fs->filterInfo->textPixmap)
	{
	    CompTransform sTransform = *transform;
	    transformToScreenSpace (s, output, -DEFAULT_Z_CAMERA, &sTransform);

	    glPushMatrix ();
	    glLoadMatrixf (sTransform.m);

	    scalefilterDrawFilterText (s, output);

	    glPopMatrix ();
	}
    }

    return status;
}

static Bool
scalefilterSetScaledPaintAttributes (CompWindow        *w,
				     WindowPaintAttrib *attrib)
{
    Bool ret;

    FILTER_SCREEN (w->screen);
    SCALE_SCREEN (w->screen);

    UNWRAP (fs, ss, setScaledPaintAttributes);
    ret = (*ss->setScaledPaintAttributes) (w, attrib);
    WRAP (fs, ss, setScaledPaintAttributes, 
	  scalefilterSetScaledPaintAttributes);

    if (fs->matchApplied ||
	(fs->filterInfo && fs->filterInfo->filterStringLength))
    {
	SCALE_WINDOW (w);

	if (ret && !sw->slot)
	{
	    ret = FALSE;
    	    attrib->opacity = 0;
	}
    }

    return ret;
}

static void
scalefilterScreenOptionChanged (CompScreen               *s,
				CompOption               *opt,
	 			ScalefilterScreenOptions num)
{
    switch (num)
    {
	case ScalefilterScreenOptionFontBold:
	case ScalefilterScreenOptionFontSize:
	case ScalefilterScreenOptionFontColor:
	case ScalefilterScreenOptionBackColor:
	    {
		FILTER_SCREEN (s);

		if (fs->filterInfo)
		    scalefilterRenderFilterText (s);
	    }
	    break;
	default:
	    break;
    }
}

static Bool
scalefilterInitDisplay (CompPlugin  *p,
	    		CompDisplay *d)
{
    ScaleFilterDisplay *fd;
    CompPlugin         *scale = findActivePlugin ("scale");
    CompOption         *option;
    int                nOption;

    if (!scale || !scale->vTable->getDisplayOptions)
	return FALSE;

    option = (*scale->vTable->getDisplayOptions) (scale, d, &nOption);

    if (getIntOptionNamed (option, nOption, "abi", 0) != SCALE_ABIVERSION)
    {
	compLogMessage (d, "scalefilter", CompLogLevelError,
			"scale ABI version mismatch");
	return FALSE;
    }

    scaleDisplayPrivateIndex = getIntOptionNamed (option, nOption, "index", -1);
    if (scaleDisplayPrivateIndex < 0)
	return FALSE;

    fd = malloc (sizeof (ScaleFilterDisplay));
    if (!fd)
	return FALSE;

    fd->screenPrivateIndex = allocateScreenPrivateIndex (d);
    if (fd->screenPrivateIndex < 0)
    {
	free (fd);
	return FALSE;
    }

    fd->xim = XOpenIM (d->display, NULL, NULL, NULL);
    if (fd->xim)
	fd->xic = XCreateIC (fd->xim,
			     XNClientWindow, d->screens->root,
			     XNInputStyle,
			     XIMPreeditNothing  | XIMStatusNothing,
			     NULL);
    else
	fd->xic = NULL;

    if (fd->xic)
	setlocale (LC_CTYPE, "");

    WRAP (fd, d, handleEvent, scalefilterHandleEvent);
    WRAP (fd, d, handleEcompEvent, scalefilterHandleEcompEvent);

    d->privates[displayPrivateIndex].ptr = fd;

    return TRUE;
}

static void
scalefilterFiniDisplay (CompPlugin  *p,
	    		CompDisplay *d)
{
    FILTER_DISPLAY (d);

    UNWRAP (fd, d, handleEvent);
    UNWRAP (fd, d, handleEcompEvent);

    if (fd->xic)
	XDestroyIC (fd->xic);
    if (fd->xim)
	XCloseIM (fd->xim);

    freeScreenPrivateIndex (d, fd->screenPrivateIndex);

    free (fd);
}

static Bool
scalefilterInitScreen (CompPlugin *p,
	    	       CompScreen *s)
{
    ScaleFilterScreen *fs;

    FILTER_DISPLAY (s->display);
    SCALE_SCREEN (s);

    fs = malloc (sizeof (ScaleFilterScreen));
    if (!fs)
	return FALSE;

    fs->filterInfo = NULL;
    matchInit (&fs->scaleMatch);
    fs->matchApplied = FALSE;

    WRAP (fs, s, paintOutput, scalefilterPaintOutput);
    WRAP (fs, ss, setScaledPaintAttributes,
	  scalefilterSetScaledPaintAttributes);

    scalefilterSetFontBoldNotify (s, scalefilterScreenOptionChanged);
    scalefilterSetFontSizeNotify (s, scalefilterScreenOptionChanged);
    scalefilterSetFontColorNotify (s, scalefilterScreenOptionChanged);
    scalefilterSetBackColorNotify (s, scalefilterScreenOptionChanged);

    s->privates[fd->screenPrivateIndex].ptr = fs;

    return TRUE;
}

static void
scalefilterFiniScreen (CompPlugin *p,
	    	       CompScreen *s)
{
    FILTER_SCREEN (s);
    SCALE_SCREEN (s);

    UNWRAP (fs, s, paintOutput);
    UNWRAP (fs, ss, setScaledPaintAttributes);

    if (fs->filterInfo)
    {
	ss->currentMatch = fs->filterInfo->origMatch;
	scalefilterFiniFilterInfo (s, TRUE);
    }

    free (fs);
}

static Bool
scalefilterInit (CompPlugin *p)
{
    displayPrivateIndex = allocateDisplayPrivateIndex ();
    if (displayPrivateIndex < 0)
	return FALSE;

    return TRUE;
}

static void
scalefilterFini (CompPlugin *p)
{
    freeDisplayPrivateIndex (displayPrivateIndex);
}

static int
scalefilterGetVersion (CompPlugin *plugin,
	    	       int	 version)
{
    return ABIVERSION;
}

CompPluginVTable scalefilterVTable = {
    "scalefilter",
    scalefilterGetVersion,
    0,
    scalefilterInit,
    scalefilterFini,
    scalefilterInitDisplay,
    scalefilterFiniDisplay,
    scalefilterInitScreen,
    scalefilterFiniScreen,
    0,
    0,
    0,
    0,
    0,
    0
};

CompPluginVTable *
getCompPluginInfo (void)
{
    return &scalefilterVTable;
}
