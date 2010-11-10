/*
 * This file is autogenerated with bcop:
 * The Ecomp option code generator
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <ecomp.h>

#define _THUMBNAIL_OPTIONS_INTERNAL
#include "thumbnail_options.h"

static int displayPrivateIndex;

static CompMetadata thumbnailOptionsMetadata;

static CompPluginVTable *thumbnailPluginVTable = NULL;
CompPluginVTable thumbnailOptionsVTable;

#define GET_THUMBNAIL_OPTIONS_DISPLAY(d) \
  ((ThumbnailOptionsDisplay *)(d)->privates[displayPrivateIndex].ptr)

#define THUMBNAIL_OPTIONS_DISPLAY(d) \
  ThumbnailOptionsDisplay * od = GET_THUMBNAIL_OPTIONS_DISPLAY (d)

#define GET_THUMBNAIL_OPTIONS_SCREEN(s, od) \
  ((ThumbnailOptionsScreen *)(s)->privates[(od)->screenPrivateIndex].ptr)

#define THUMBNAIL_OPTIONS_SCREEN(s) \
  ThumbnailOptionsScreen * os = GET_THUMBNAIL_OPTIONS_SCREEN (s, GET_THUMBNAIL_OPTIONS_DISPLAY (s->display))

typedef struct _ThumbnailOptionsDisplay
{
   int screenPrivateIndex;
} ThumbnailOptionsDisplay;

typedef struct _ThumbnailOptionsScreen
{
   CompOption                            opt[ThumbnailScreenOptionNum];
   thumbnailScreenOptionChangeNotifyProc notify[ThumbnailScreenOptionNum];
} ThumbnailOptionsScreen;

int
thumbnailGetThumbSize(CompScreen *s)
{
   THUMBNAIL_OPTIONS_SCREEN(s);
   return os->opt[ThumbnailScreenOptionThumbSize].value.i;
}

CompOption *
thumbnailGetThumbSizeOption(CompScreen *s)
{
   THUMBNAIL_OPTIONS_SCREEN(s);
   return &os->opt[ThumbnailScreenOptionThumbSize];
}

void
thumbnailSetThumbSizeNotify(CompScreen *s, thumbnailScreenOptionChangeNotifyProc notify)
{
   THUMBNAIL_OPTIONS_SCREEN(s);
   os->notify[ThumbnailScreenOptionThumbSize] = notify;
}

int
thumbnailGetShowDelay(CompScreen *s)
{
   THUMBNAIL_OPTIONS_SCREEN(s);
   return os->opt[ThumbnailScreenOptionShowDelay].value.i;
}

CompOption *
thumbnailGetShowDelayOption(CompScreen *s)
{
   THUMBNAIL_OPTIONS_SCREEN(s);
   return &os->opt[ThumbnailScreenOptionShowDelay];
}

void
thumbnailSetShowDelayNotify(CompScreen *s, thumbnailScreenOptionChangeNotifyProc notify)
{
   THUMBNAIL_OPTIONS_SCREEN(s);
   os->notify[ThumbnailScreenOptionShowDelay] = notify;
}

int
thumbnailGetBorder(CompScreen *s)
{
   THUMBNAIL_OPTIONS_SCREEN(s);
   return os->opt[ThumbnailScreenOptionBorder].value.i;
}

CompOption *
thumbnailGetBorderOption(CompScreen *s)
{
   THUMBNAIL_OPTIONS_SCREEN(s);
   return &os->opt[ThumbnailScreenOptionBorder];
}

void
thumbnailSetBorderNotify(CompScreen *s, thumbnailScreenOptionChangeNotifyProc notify)
{
   THUMBNAIL_OPTIONS_SCREEN(s);
   os->notify[ThumbnailScreenOptionBorder] = notify;
}

unsigned short *
thumbnailGetThumbColor(CompScreen *s)
{
   THUMBNAIL_OPTIONS_SCREEN(s);
   return os->opt[ThumbnailScreenOptionThumbColor].value.c;
}

unsigned short
thumbnailGetThumbColorRed(CompScreen *s)
{
   THUMBNAIL_OPTIONS_SCREEN(s);
   return os->opt[ThumbnailScreenOptionThumbColor].value.c[0];
}

unsigned short
thumbnailGetThumbColorGreen(CompScreen *s)
{
   THUMBNAIL_OPTIONS_SCREEN(s);
   return os->opt[ThumbnailScreenOptionThumbColor].value.c[1];
}

unsigned short
thumbnailGetThumbColorBlue(CompScreen *s)
{
   THUMBNAIL_OPTIONS_SCREEN(s);
   return os->opt[ThumbnailScreenOptionThumbColor].value.c[2];
}

unsigned short
thumbnailGetThumbColorAlpha(CompScreen *s)
{
   THUMBNAIL_OPTIONS_SCREEN(s);
   return os->opt[ThumbnailScreenOptionThumbColor].value.c[3];
}

CompOption *
thumbnailGetThumbColorOption(CompScreen *s)
{
   THUMBNAIL_OPTIONS_SCREEN(s);
   return &os->opt[ThumbnailScreenOptionThumbColor];
}

void
thumbnailSetThumbColorNotify(CompScreen *s, thumbnailScreenOptionChangeNotifyProc notify)
{
   THUMBNAIL_OPTIONS_SCREEN(s);
   os->notify[ThumbnailScreenOptionThumbColor] = notify;
}

float
thumbnailGetFadeSpeed(CompScreen *s)
{
   THUMBNAIL_OPTIONS_SCREEN(s);
   return os->opt[ThumbnailScreenOptionFadeSpeed].value.f;
}

CompOption *
thumbnailGetFadeSpeedOption(CompScreen *s)
{
   THUMBNAIL_OPTIONS_SCREEN(s);
   return &os->opt[ThumbnailScreenOptionFadeSpeed];
}

void
thumbnailSetFadeSpeedNotify(CompScreen *s, thumbnailScreenOptionChangeNotifyProc notify)
{
   THUMBNAIL_OPTIONS_SCREEN(s);
   os->notify[ThumbnailScreenOptionFadeSpeed] = notify;
}

Bool
thumbnailGetCurrentViewport(CompScreen *s)
{
   THUMBNAIL_OPTIONS_SCREEN(s);
   return os->opt[ThumbnailScreenOptionCurrentViewport].value.b;
}

CompOption *
thumbnailGetCurrentViewportOption(CompScreen *s)
{
   THUMBNAIL_OPTIONS_SCREEN(s);
   return &os->opt[ThumbnailScreenOptionCurrentViewport];
}

void
thumbnailSetCurrentViewportNotify(CompScreen *s, thumbnailScreenOptionChangeNotifyProc notify)
{
   THUMBNAIL_OPTIONS_SCREEN(s);
   os->notify[ThumbnailScreenOptionCurrentViewport] = notify;
}

Bool
thumbnailGetAlwaysOnTop(CompScreen *s)
{
   THUMBNAIL_OPTIONS_SCREEN(s);
   return os->opt[ThumbnailScreenOptionAlwaysOnTop].value.b;
}

CompOption *
thumbnailGetAlwaysOnTopOption(CompScreen *s)
{
   THUMBNAIL_OPTIONS_SCREEN(s);
   return &os->opt[ThumbnailScreenOptionAlwaysOnTop];
}

void
thumbnailSetAlwaysOnTopNotify(CompScreen *s, thumbnailScreenOptionChangeNotifyProc notify)
{
   THUMBNAIL_OPTIONS_SCREEN(s);
   os->notify[ThumbnailScreenOptionAlwaysOnTop] = notify;
}

Bool
thumbnailGetWindowLike(CompScreen *s)
{
   THUMBNAIL_OPTIONS_SCREEN(s);
   return os->opt[ThumbnailScreenOptionWindowLike].value.b;
}

CompOption *
thumbnailGetWindowLikeOption(CompScreen *s)
{
   THUMBNAIL_OPTIONS_SCREEN(s);
   return &os->opt[ThumbnailScreenOptionWindowLike];
}

void
thumbnailSetWindowLikeNotify(CompScreen *s, thumbnailScreenOptionChangeNotifyProc notify)
{
   THUMBNAIL_OPTIONS_SCREEN(s);
   os->notify[ThumbnailScreenOptionWindowLike] = notify;
}

Bool
thumbnailGetMipmap(CompScreen *s)
{
   THUMBNAIL_OPTIONS_SCREEN(s);
   return os->opt[ThumbnailScreenOptionMipmap].value.b;
}

CompOption *
thumbnailGetMipmapOption(CompScreen *s)
{
   THUMBNAIL_OPTIONS_SCREEN(s);
   return &os->opt[ThumbnailScreenOptionMipmap];
}

void
thumbnailSetMipmapNotify(CompScreen *s, thumbnailScreenOptionChangeNotifyProc notify)
{
   THUMBNAIL_OPTIONS_SCREEN(s);
   os->notify[ThumbnailScreenOptionMipmap] = notify;
}

Bool
thumbnailGetTitleEnabled(CompScreen *s)
{
   THUMBNAIL_OPTIONS_SCREEN(s);
   return os->opt[ThumbnailScreenOptionTitleEnabled].value.b;
}

CompOption *
thumbnailGetTitleEnabledOption(CompScreen *s)
{
   THUMBNAIL_OPTIONS_SCREEN(s);
   return &os->opt[ThumbnailScreenOptionTitleEnabled];
}

void
thumbnailSetTitleEnabledNotify(CompScreen *s, thumbnailScreenOptionChangeNotifyProc notify)
{
   THUMBNAIL_OPTIONS_SCREEN(s);
   os->notify[ThumbnailScreenOptionTitleEnabled] = notify;
}

Bool
thumbnailGetFontBold(CompScreen *s)
{
   THUMBNAIL_OPTIONS_SCREEN(s);
   return os->opt[ThumbnailScreenOptionFontBold].value.b;
}

CompOption *
thumbnailGetFontBoldOption(CompScreen *s)
{
   THUMBNAIL_OPTIONS_SCREEN(s);
   return &os->opt[ThumbnailScreenOptionFontBold];
}

void
thumbnailSetFontBoldNotify(CompScreen *s, thumbnailScreenOptionChangeNotifyProc notify)
{
   THUMBNAIL_OPTIONS_SCREEN(s);
   os->notify[ThumbnailScreenOptionFontBold] = notify;
}

int
thumbnailGetFontSize(CompScreen *s)
{
   THUMBNAIL_OPTIONS_SCREEN(s);
   return os->opt[ThumbnailScreenOptionFontSize].value.i;
}

CompOption *
thumbnailGetFontSizeOption(CompScreen *s)
{
   THUMBNAIL_OPTIONS_SCREEN(s);
   return &os->opt[ThumbnailScreenOptionFontSize];
}

void
thumbnailSetFontSizeNotify(CompScreen *s, thumbnailScreenOptionChangeNotifyProc notify)
{
   THUMBNAIL_OPTIONS_SCREEN(s);
   os->notify[ThumbnailScreenOptionFontSize] = notify;
}

unsigned short *
thumbnailGetFontColor(CompScreen *s)
{
   THUMBNAIL_OPTIONS_SCREEN(s);
   return os->opt[ThumbnailScreenOptionFontColor].value.c;
}

unsigned short
thumbnailGetFontColorRed(CompScreen *s)
{
   THUMBNAIL_OPTIONS_SCREEN(s);
   return os->opt[ThumbnailScreenOptionFontColor].value.c[0];
}

unsigned short
thumbnailGetFontColorGreen(CompScreen *s)
{
   THUMBNAIL_OPTIONS_SCREEN(s);
   return os->opt[ThumbnailScreenOptionFontColor].value.c[1];
}

unsigned short
thumbnailGetFontColorBlue(CompScreen *s)
{
   THUMBNAIL_OPTIONS_SCREEN(s);
   return os->opt[ThumbnailScreenOptionFontColor].value.c[2];
}

unsigned short
thumbnailGetFontColorAlpha(CompScreen *s)
{
   THUMBNAIL_OPTIONS_SCREEN(s);
   return os->opt[ThumbnailScreenOptionFontColor].value.c[3];
}

CompOption *
thumbnailGetFontColorOption(CompScreen *s)
{
   THUMBNAIL_OPTIONS_SCREEN(s);
   return &os->opt[ThumbnailScreenOptionFontColor];
}

void
thumbnailSetFontColorNotify(CompScreen *s, thumbnailScreenOptionChangeNotifyProc notify)
{
   THUMBNAIL_OPTIONS_SCREEN(s);
   os->notify[ThumbnailScreenOptionFontColor] = notify;
}

CompOption *
thumbnailGetScreenOption(CompScreen *s, ThumbnailScreenOptions num)
{
   THUMBNAIL_OPTIONS_SCREEN(s);
   return &os->opt[num];
}

static const CompMetadataOptionInfo thumbnailOptionsScreenOptionInfo[] = {
   { "thumb_size", "int", "<min>50</min><max>1500</max>", 0, 0 },
   { "show_delay", "int", "<min>100</min><max>10000</max>", 0, 0 },
   { "border", "int", "<min>1</min><max>32</max>", 0, 0 },
   { "thumb_color", "color", 0, 0, 0 },
   { "fade_speed", "float", "<min>0.0</min><max>5.0</max>", 0, 0 },
   { "current_viewport", "bool", 0, 0, 0 },
   { "always_on_top", "bool", 0, 0, 0 },
   { "window_like", "bool", 0, 0, 0 },
   { "mipmap", "bool", 0, 0, 0 },
   { "title_enabled", "bool", 0, 0, 0 },
   { "font_bold", "bool", 0, 0, 0 },
   { "font_size", "int", "<min>6</min><max>36</max>", 0, 0 },
   { "font_color", "color", 0, 0, 0 },
};

static Bool
thumbnailOptionsSetScreenOption(CompPlugin *plugin, CompScreen *s, char *name, CompOptionValue *value)
{
   THUMBNAIL_OPTIONS_SCREEN(s);
   CompOption *o;
   int index;

   o = compFindOption (os->opt, ThumbnailScreenOptionNum, name, &index);

   if (!o)
     return FALSE;

   switch (index)
     {
      case ThumbnailScreenOptionThumbSize:
        if (compSetScreenOption (s, o, value))
          {
             if (os->notify[ThumbnailScreenOptionThumbSize])
               (*os->notify[ThumbnailScreenOptionThumbSize])(s, o, ThumbnailScreenOptionThumbSize);
             return TRUE;
          }
        break;

      case ThumbnailScreenOptionShowDelay:
        if (compSetScreenOption (s, o, value))
          {
             if (os->notify[ThumbnailScreenOptionShowDelay])
               (*os->notify[ThumbnailScreenOptionShowDelay])(s, o, ThumbnailScreenOptionShowDelay);
             return TRUE;
          }
        break;

      case ThumbnailScreenOptionBorder:
        if (compSetScreenOption (s, o, value))
          {
             if (os->notify[ThumbnailScreenOptionBorder])
               (*os->notify[ThumbnailScreenOptionBorder])(s, o, ThumbnailScreenOptionBorder);
             return TRUE;
          }
        break;

      case ThumbnailScreenOptionThumbColor:
        if (compSetScreenOption (s, o, value))
          {
             if (os->notify[ThumbnailScreenOptionThumbColor])
               (*os->notify[ThumbnailScreenOptionThumbColor])(s, o, ThumbnailScreenOptionThumbColor);
             return TRUE;
          }
        break;

      case ThumbnailScreenOptionFadeSpeed:
        if (compSetScreenOption (s, o, value))
          {
             if (os->notify[ThumbnailScreenOptionFadeSpeed])
               (*os->notify[ThumbnailScreenOptionFadeSpeed])(s, o, ThumbnailScreenOptionFadeSpeed);
             return TRUE;
          }
        break;

      case ThumbnailScreenOptionCurrentViewport:
        if (compSetScreenOption (s, o, value))
          {
             if (os->notify[ThumbnailScreenOptionCurrentViewport])
               (*os->notify[ThumbnailScreenOptionCurrentViewport])(s, o, ThumbnailScreenOptionCurrentViewport);
             return TRUE;
          }
        break;

      case ThumbnailScreenOptionAlwaysOnTop:
        if (compSetScreenOption (s, o, value))
          {
             if (os->notify[ThumbnailScreenOptionAlwaysOnTop])
               (*os->notify[ThumbnailScreenOptionAlwaysOnTop])(s, o, ThumbnailScreenOptionAlwaysOnTop);
             return TRUE;
          }
        break;

      case ThumbnailScreenOptionWindowLike:
        if (compSetScreenOption (s, o, value))
          {
             if (os->notify[ThumbnailScreenOptionWindowLike])
               (*os->notify[ThumbnailScreenOptionWindowLike])(s, o, ThumbnailScreenOptionWindowLike);
             return TRUE;
          }
        break;

      case ThumbnailScreenOptionMipmap:
        if (compSetScreenOption (s, o, value))
          {
             if (os->notify[ThumbnailScreenOptionMipmap])
               (*os->notify[ThumbnailScreenOptionMipmap])(s, o, ThumbnailScreenOptionMipmap);
             return TRUE;
          }
        break;

      case ThumbnailScreenOptionTitleEnabled:
        if (compSetScreenOption (s, o, value))
          {
             if (os->notify[ThumbnailScreenOptionTitleEnabled])
               (*os->notify[ThumbnailScreenOptionTitleEnabled])(s, o, ThumbnailScreenOptionTitleEnabled);
             return TRUE;
          }
        break;

      case ThumbnailScreenOptionFontBold:
        if (compSetScreenOption (s, o, value))
          {
             if (os->notify[ThumbnailScreenOptionFontBold])
               (*os->notify[ThumbnailScreenOptionFontBold])(s, o, ThumbnailScreenOptionFontBold);
             return TRUE;
          }
        break;

      case ThumbnailScreenOptionFontSize:
        if (compSetScreenOption (s, o, value))
          {
             if (os->notify[ThumbnailScreenOptionFontSize])
               (*os->notify[ThumbnailScreenOptionFontSize])(s, o, ThumbnailScreenOptionFontSize);
             return TRUE;
          }
        break;

      case ThumbnailScreenOptionFontColor:
        if (compSetScreenOption (s, o, value))
          {
             if (os->notify[ThumbnailScreenOptionFontColor])
               (*os->notify[ThumbnailScreenOptionFontColor])(s, o, ThumbnailScreenOptionFontColor);
             return TRUE;
          }
        break;

      default:
        break;
     }
   return FALSE;
}

static CompOption *
thumbnailOptionsGetScreenOptions(CompPlugin *plugin, CompScreen *s, int *count)
{
   THUMBNAIL_OPTIONS_SCREEN(s);
   *count = ThumbnailScreenOptionNum;
   return os->opt;
}

static Bool
thumbnailOptionsInitScreen(CompPlugin *p, CompScreen *s)
{
   ThumbnailOptionsScreen *os;

   THUMBNAIL_OPTIONS_DISPLAY (s->display);

   os = calloc (1, sizeof(ThumbnailOptionsScreen));
   if (!os)
     return FALSE;

   s->privates[od->screenPrivateIndex].ptr = os;

   if (!compInitScreenOptionsFromMetadata (s, &thumbnailOptionsMetadata, thumbnailOptionsScreenOptionInfo, os->opt, ThumbnailScreenOptionNum))
     {
        free (os);
        return FALSE;
     }
   if (thumbnailPluginVTable && thumbnailPluginVTable->initScreen)
     return thumbnailPluginVTable->initScreen (p, s);
   return TRUE;
}

static void
thumbnailOptionsFiniScreen(CompPlugin *p, CompScreen *s)
{
   if (thumbnailPluginVTable && thumbnailPluginVTable->finiScreen)
     return thumbnailPluginVTable->finiScreen (p, s);

   THUMBNAIL_OPTIONS_SCREEN (s);

   compFiniScreenOptions (s, os->opt, ThumbnailScreenOptionNum);

   free (os);
}

static Bool
thumbnailOptionsInitDisplay(CompPlugin *p, CompDisplay *d)
{
   ThumbnailOptionsDisplay *od;

   od = calloc (1, sizeof(ThumbnailOptionsDisplay));
   if (!od)
     return FALSE;

   od->screenPrivateIndex = allocateScreenPrivateIndex(d);
   if (od->screenPrivateIndex < 0)
     {
        free(od);
        return FALSE;
     }

   d->privates[displayPrivateIndex].ptr = od;

   if (thumbnailPluginVTable && thumbnailPluginVTable->initDisplay)
     return thumbnailPluginVTable->initDisplay (p, d);
   return TRUE;
}

static void
thumbnailOptionsFiniDisplay(CompPlugin *p, CompDisplay *d)
{
   if (thumbnailPluginVTable && thumbnailPluginVTable->finiDisplay)
     return thumbnailPluginVTable->finiDisplay (p, d);

   THUMBNAIL_OPTIONS_DISPLAY (d);

   freeScreenPrivateIndex(d, od->screenPrivateIndex);
   free (od);
}

static Bool
thumbnailOptionsInit(CompPlugin *p)
{
   displayPrivateIndex = allocateDisplayPrivateIndex();
   if (displayPrivateIndex < 0)
     return FALSE;

   if (!compInitPluginMetadataFromInfo (&thumbnailOptionsMetadata, "thumbnail", 0, 0, thumbnailOptionsScreenOptionInfo, ThumbnailScreenOptionNum))
     return FALSE;

   compAddMetadataFromFile (&thumbnailOptionsMetadata, "thumbnail");
   if (thumbnailPluginVTable && thumbnailPluginVTable->init)
     return thumbnailPluginVTable->init (p);
   return TRUE;
}

static void
thumbnailOptionsFini(CompPlugin *p)
{
   if (thumbnailPluginVTable && thumbnailPluginVTable->fini)
     return thumbnailPluginVTable->fini (p);

   if (displayPrivateIndex >= 0)
     freeDisplayPrivateIndex(displayPrivateIndex);

   compFiniMetadata (&thumbnailOptionsMetadata);
}

static CompMetadata *
thumbnailOptionsGetMetadata(CompPlugin *plugin)
{
   return &thumbnailOptionsMetadata;
}

CompPluginVTable *
getCompPluginInfo(void)
{
   if (!thumbnailPluginVTable)
     {
        thumbnailPluginVTable = thumbnailOptionsGetCompPluginInfo ();
        memcpy(&thumbnailOptionsVTable, thumbnailPluginVTable, sizeof(CompPluginVTable));
        thumbnailOptionsVTable.getMetadata = thumbnailOptionsGetMetadata;
        thumbnailOptionsVTable.init = thumbnailOptionsInit;
        thumbnailOptionsVTable.fini = thumbnailOptionsFini;
        thumbnailOptionsVTable.initDisplay = thumbnailOptionsInitDisplay;
        thumbnailOptionsVTable.finiDisplay = thumbnailOptionsFiniDisplay;
        thumbnailOptionsVTable.initScreen = thumbnailOptionsInitScreen;
        thumbnailOptionsVTable.finiScreen = thumbnailOptionsFiniScreen;
        thumbnailOptionsVTable.getScreenOptions = thumbnailOptionsGetScreenOptions;
        thumbnailOptionsVTable.setScreenOption = thumbnailOptionsSetScreenOption;
     }
   return &thumbnailOptionsVTable;
}

