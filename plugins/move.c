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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <X11/cursorfont.h>

#include <ecomp.h>

static CompMetadata moveMetadata;

struct _MoveKeys {
    char *name;
    int  dx;
    int  dy;
} mKeys[] = {
    { "Left",  -1,  0 },
    { "Right",  1,  0 },
    { "Up",     0, -1 },
    { "Down",   0,  1 }
};

#define NUM_KEYS (sizeof (mKeys) / sizeof (mKeys[0]))

#define KEY_MOVE_INC 24

#define SNAP_BACK 20
#define SNAP_OFF  100

static int displayPrivateIndex;

#define MOVE_DISPLAY_OPTION_INITIATE	      0
#define MOVE_DISPLAY_OPTION_OPACITY	      1
#define MOVE_DISPLAY_OPTION_CONSTRAIN_Y	      2
#define MOVE_DISPLAY_OPTION_SNAPOFF_MAXIMIZED 3
#define MOVE_DISPLAY_OPTION_LAZY_POSITIONING  4
#define MOVE_DISPLAY_OPTION_NUM		      5

typedef struct _MoveDisplay {
    int		    screenPrivateIndex;
    HandleEventProc handleEvent;

    CompOption opt[MOVE_DISPLAY_OPTION_NUM];

    CompWindow *w;
    int	       savedX;
    int	       savedY;
    int	       x;
    int	       y;
    Region     region;
    int        status;
    KeyCode    key[NUM_KEYS];

    GLushort moveOpacity;
} MoveDisplay;

typedef struct _MoveScreen {
    PaintWindowProc paintWindow;
	PreparePaintScreenProc preparePaintScreen;
    int grabIndex;

    Cursor moveCursor;

    unsigned int origState;
	double progress;
	int active;
    int	snapOffY;
    int	snapBackY;
} MoveScreen;

#define GET_MOVE_DISPLAY(d)										\
    ((MoveDisplay *) (d)->privates[displayPrivateIndex].ptr)

#define MOVE_DISPLAY(d)							\
    MoveDisplay *md = GET_MOVE_DISPLAY (d)

#define GET_MOVE_SCREEN(s, md)										\
    ((MoveScreen *) (s)->privates[(md)->screenPrivateIndex].ptr)

#define MOVE_SCREEN(s)													\
    MoveScreen *ms = GET_MOVE_SCREEN (s, GET_MOVE_DISPLAY (s->display))

#define NUM_OPTIONS(s) (sizeof ((s)->opt) / sizeof (CompOption))

static Bool
moveInitiate (CompDisplay     *d,
			  CompAction      *action,
			  CompActionState state,
			  CompOption      *option,
			  int	      nOption)
{
    CompWindow *w;
    Window     xid;
     
    MOVE_DISPLAY (d);
    xid = getIntOptionNamed (option, nOption, "window", 0);

    w = findWindowAtDisplay (d, xid);
    if (w)
    {
		/* unsigned int mods;
		 * int          x, y; */

		MOVE_SCREEN (w->screen);

		/* mods = getIntOptionNamed (option, nOption, "modifiers", 0); */

		/* x = getIntOptionNamed (option, nOption, "x",
		 * 					   w->attrib.x + (w->width / 2));
		 * y = getIntOptionNamed (option, nOption, "y",
		 * 					   w->attrib.y + (w->height / 2)); */

		if (otherScreenGrabExist (w->screen, "move", 0))
			return FALSE;
	
		if (md->w)
			return FALSE;
	
		if (md->region)
		{
			XDestroyRegion (md->region);
			md->region = NULL;
		}

		md->status = RectangleOut;

		/* md->savedX = w->serverX;
		 * md->savedY = w->serverY; */

		md->x = 0;
		md->y = 0;

		/* lastPointerX = x;
		 * lastPointerY = y; */

		ms->origState = w->state;

		if (!ms->grabIndex)
			ms->grabIndex = pushScreenGrab (w->screen, ms->moveCursor, "move");
	
		if (ms->grabIndex)
		{
			md->w = w;
	    
			/* (w->screen->windowGrabNotify) (w, x, y, mods,
			 * 							   CompWindowGrabMoveMask |
			 * 							   CompWindowGrabButtonMask); */

			if (md->moveOpacity != OPAQUE)
				addWindowDamage (w);
		}
    }

    return FALSE;
}

static Bool
moveTerminate (CompDisplay     *d,
			   CompAction      *action,
			   CompActionState state,
			   CompOption      *option,
			   int	       nOption)
{
    MOVE_DISPLAY (d);

    if (md->w)
    {
		MOVE_SCREEN (md->w->screen);

		/* (md->w->screen->windowUngrabNotify) (md->w); */

		if (ms->grabIndex)
		{
			removeScreenGrab (md->w->screen, ms->grabIndex, NULL);
			ms->grabIndex = 0;
		}

		if (md->moveOpacity != OPAQUE)
			addWindowDamage (md->w);

		md->w = 0;
    }

    return FALSE;
}



static void
moveHandleEvent (CompDisplay *d,
				 XEvent      *event)
{
    CompScreen *s;

    MOVE_DISPLAY (d);

    switch (event->type) 
	{
	case ClientMessage:
		if (event->xclient.message_type == d->eManagedAtom)
		{ 
			CompWindow *w;
	    
			Window win = event->xclient.window;
			unsigned int type = event->xclient.data.l[0];
			if (type != 4) break;
	    
			w = findWindowAtDisplay (d, win);
			if (w)
			{
				unsigned int state = event->xclient.data.l[2];
				s = w->screen;

				printf("move %p %d\n", (void *)w->id, state);
				
				if(state)
				{ 
					CompOption o[1];
					/* int	       xRoot, yRoot; */
					CompAction *action = NULL;

					o[0].type    = CompOptionTypeInt;
					o[0].name    = "window";
					o[0].value.i = w->id;


					/* unsigned int mods;
					 * Window	     root, child; */
					/* int	     i; */

					/* XQueryPointer (d->display, w->screen->root,
					 * 			   &root, &child, &xRoot, &yRoot,
					 * 			   &i, &i, &mods);
					 * 
					 * o[1].type	 = CompOptionTypeInt;
					 * o[1].name	 = "modifiers";
					 * o[1].value.i = mods; */

					moveInitiate (d,
								  action,
								  CompActionStateInitButton,
								  o, 1);
					MOVE_SCREEN(s);
					
					ms->active = 1;
				}
				else
				{ 
					moveTerminate (d,
								   NULL,
								   0, NULL, 0);  
				}
			}
		}
	
		break;
	case DestroyNotify:
		if (md->w && md->w->id == event->xdestroywindow.window)
			moveTerminate (d,
						   &md->opt[MOVE_DISPLAY_OPTION_INITIATE].value.action,
						   0, NULL, 0);
		break;
	case UnmapNotify:
		if (md->w && md->w->id == event->xunmap.window)
			moveTerminate (d,
						   &md->opt[MOVE_DISPLAY_OPTION_INITIATE].value.action,
						   0, NULL, 0);
	default:
		break;
	}

    UNWRAP (md, d, handleEvent);
    (*d->handleEvent) (d, event);
    WRAP (md, d, handleEvent, moveHandleEvent);
}

/* #define sigmoid(x) (1.0f / (1.0f + exp (-5.5f * 2 * ((x) - 0.5))))
 * #define sigmoidProgress(x) ((sigmoid (x) - sigmoid (0)) /	\
 * 							(sigmoid (1) - sigmoid (0)))
 * 
 * float sigmoidAnimProgress(AnimWindow * aw)
 * {
 *     float forwardProgress =
 * 		1 - aw->animRemainingTime / (aw->animTotalTime - aw->timestep);
 *     forwardProgress = MIN(forwardProgress, 1);
 *     forwardProgress = MAX(forwardProgress, 0);
 * 
 *     // Apply sigmoid and normalize
 *     forwardProgress =
 * 		(sigmoid(forwardProgress) - sigmoid(0)) /
 * 		(sigmoid(1) - sigmoid(0));
 * 
 *     if (aw->curWindowEvent == WindowEventOpen ||
 * 		aw->curWindowEvent == WindowEventUnminimize ||
 * 		aw->curWindowEvent == WindowEventUnshade ||
 * 		aw->curWindowEvent == WindowEventFocus)
 * 		forwardProgress = 1 - forwardProgress;
 * 
 *     return forwardProgress;
 * } */

static void
movePreparePaintScreen (CompScreen *s,
						int		   _ms)
{
	MOVE_SCREEN(s);

	if (ms->grabIndex || ms->active)
	{
		float val = (((float) _ms / 1000.0) / 0.2);

		if (ms->grabIndex)
			ms->progress = MIN (1.0, ms->progress + val);
		else
			ms->progress = MAX (0.0, ms->progress - val);
	}

	UNWRAP (ms, s, preparePaintScreen);
	(*s->preparePaintScreen) (s, _ms);
	WRAP (ms, s, preparePaintScreen, movePreparePaintScreen);
}


static Bool
movePaintWindow (CompWindow		 *w,
				 const WindowPaintAttrib *attrib,
				 const CompTransform	 *transform,
				 Region			 region,
				 unsigned int		 mask)
{
    WindowPaintAttrib sAttrib;
    CompScreen	      *s = w->screen;
    Bool	      status;

    MOVE_SCREEN (s);

    if (ms->active)
    {
		MOVE_DISPLAY (s->display);

		addWindowDamage(w);
		
		if (md->w == w && md->moveOpacity != OPAQUE)
		{
			sAttrib = *attrib;
			attrib  = &sAttrib;

			sAttrib.opacity = (sAttrib.opacity +
							   ((int)(sAttrib.opacity *
									  md->moveOpacity * ms->progress))) >> 16;
		}
		if (!ms->grabIndex && ms->progress == 0.0){
			ms->active = 0;
			md->w = 0;
		}
    }

    UNWRAP (ms, s, paintWindow);
    status = (*s->paintWindow) (w, attrib, transform, region, mask);
    WRAP (ms, s, paintWindow, movePaintWindow);

    return status;
}

static CompOption *
moveGetDisplayOptions (CompPlugin  *plugin,
					   CompDisplay *display,
					   int	   *count)
{
    MOVE_DISPLAY (display);

    *count = NUM_OPTIONS (md);
    return md->opt;
}

static Bool
moveSetDisplayOption (CompPlugin      *plugin,
					  CompDisplay     *display,
					  char	      *name,
					  CompOptionValue *value)
{
    CompOption *o;
    int	       index;

    MOVE_DISPLAY (display);

    o = compFindOption (md->opt, NUM_OPTIONS (md), name, &index);
    if (!o)
		return FALSE;

    switch (index) {
    case MOVE_DISPLAY_OPTION_OPACITY:
		if (compSetIntOption (o, value))
		{
			md->moveOpacity = (o->value.i * OPAQUE) / 100;
			return TRUE;
		}
		break;
    default:
		return compSetDisplayOption (display, o, value);
    }

    return FALSE;
}

static const CompMetadataOptionInfo moveDisplayOptionInfo[] = {
    { "initiate", "action", 0, moveInitiate, moveTerminate },
    { "opacity", "int", "<min>0</min><max>100</max>", 0, 0 },
    { "constrain_y", "bool", 0, 0, 0 },
    { "snapoff_maximized", "bool", 0, 0, 0 },
    { "lazy_positioning", "bool", 0, 0, 0 }
};

static Bool
moveInitDisplay (CompPlugin  *p,
				 CompDisplay *d)
{
    MoveDisplay *md;
    int	        i;

    md = malloc (sizeof (MoveDisplay));
    if (!md)
		return FALSE;

    if (!compInitDisplayOptionsFromMetadata (d,
											 &moveMetadata,
											 moveDisplayOptionInfo,
											 md->opt,
											 MOVE_DISPLAY_OPTION_NUM))
    {
		free (md);
		return FALSE;
    }

    md->screenPrivateIndex = allocateScreenPrivateIndex (d);
    if (md->screenPrivateIndex < 0)
    {
		compFiniDisplayOptions (d, md->opt, MOVE_DISPLAY_OPTION_NUM);
		free (md);
		return FALSE;
    }

    md->moveOpacity =
		(md->opt[MOVE_DISPLAY_OPTION_OPACITY].value.i * OPAQUE) / 100;

    md->w      = 0;
    md->region = NULL;
    md->status = RectangleOut;

    for (i = 0; i < NUM_KEYS; i++)
		md->key[i] = XKeysymToKeycode (d->display,
									   XStringToKeysym (mKeys[i].name));

    WRAP (md, d, handleEvent, moveHandleEvent);

    d->privates[displayPrivateIndex].ptr = md;

    return TRUE;
}

static void
moveFiniDisplay (CompPlugin  *p,
				 CompDisplay *d)
{
    MOVE_DISPLAY (d);

    freeScreenPrivateIndex (d, md->screenPrivateIndex);

    UNWRAP (md, d, handleEvent);

    compFiniDisplayOptions (d, md->opt, MOVE_DISPLAY_OPTION_NUM);

    free (md);
}

static Bool
moveInitScreen (CompPlugin *p,
				CompScreen *s)
{
    MoveScreen *ms;

    MOVE_DISPLAY (s->display);

    ms = malloc (sizeof (MoveScreen));
    if (!ms)
		return FALSE;

    ms->grabIndex = 0;
	ms->active = 0;
	ms->progress = 0.0;
	
    ms->moveCursor = XCreateFontCursor (s->display->display, XC_fleur);

    WRAP (ms, s, paintWindow, movePaintWindow);
    WRAP (ms, s, preparePaintScreen, movePreparePaintScreen);

    s->privates[md->screenPrivateIndex].ptr = ms;

    return TRUE;
}

static void
moveFiniScreen (CompPlugin *p,
				CompScreen *s)
{
    MOVE_SCREEN (s);

    UNWRAP (ms, s, paintWindow);
    UNWRAP (ms, s, preparePaintScreen);

    if (ms->moveCursor)
		XFreeCursor (s->display->display, ms->moveCursor);

    free (ms);
}

static Bool
moveInit (CompPlugin *p)
{
    if (!compInitPluginMetadataFromInfo (&moveMetadata,
										 p->vTable->name,
										 moveDisplayOptionInfo,
										 MOVE_DISPLAY_OPTION_NUM,
										 0, 0))
		return FALSE;

    displayPrivateIndex = allocateDisplayPrivateIndex ();
    if (displayPrivateIndex < 0)
    {
		compFiniMetadata (&moveMetadata);
		return FALSE;
    }

    compAddMetadataFromFile (&moveMetadata, p->vTable->name);

    return TRUE;
}

static void
moveFini (CompPlugin *p)
{
    freeDisplayPrivateIndex (displayPrivateIndex);
    compFiniMetadata (&moveMetadata);
}

static int
moveGetVersion (CompPlugin *plugin,
				int	   version)
{
    return ABIVERSION;
}

static CompMetadata *
moveGetMetadata (CompPlugin *plugin)
{
    return &moveMetadata;
}

CompPluginVTable moveVTable = {
    "move",
    moveGetVersion,
    moveGetMetadata,
    moveInit,
    moveFini,
    moveInitDisplay,
    moveFiniDisplay,
    moveInitScreen,
    moveFiniScreen,
    0, /* InitWindow */
    0, /* FiniWindow */
    moveGetDisplayOptions,
    moveSetDisplayOption,
    0, /* GetScreenOptions */
    0  /* SetScreenOption */
};

CompPluginVTable *
getCompPluginInfo (void)
{
    return &moveVTable;
}
