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

#ifndef _RING_OPTIONS_H
#define _RING_OPTIONS_H

#ifndef _RING_OPTIONS_INTERNAL
#define getCompPluginInfo ringOptionsGetCompPluginInfo
#endif

#ifdef  __cplusplus
extern "C" {
#endif

CompPluginVTable *ringOptionsGetCompPluginInfo (void);

typedef enum
{
    RingDisplayOptionNum
} RingDisplayOptions;

typedef void (*ringDisplayOptionChangeNotifyProc) (CompDisplay *display, CompOption *opt, RingDisplayOptions num);

CompOption *ringGetDisplayOption (CompDisplay *d, RingDisplayOptions num);

typedef enum
{
    RingScreenOptionSpeed,
    RingScreenOptionTimestep,
    RingScreenOptionInactiveOpacity,
    RingScreenOptionWindowMatch,
    RingScreenOptionOverlayIcon,
    RingScreenOptionDarkenBack,
    RingScreenOptionMinimized,
    RingScreenOptionSelectWithMouse,
    RingScreenOptionRingClockwise,
    RingScreenOptionRingWidth,
    RingScreenOptionRingHeight,
    RingScreenOptionThumbWidth,
    RingScreenOptionThumbHeight,
    RingScreenOptionMinBrightness,
    RingScreenOptionMinScale,
    RingScreenOptionWindowTitle,
    RingScreenOptionTitleFontBold,
    RingScreenOptionTitleFontSize,
    RingScreenOptionTitleBackColor,
    RingScreenOptionTitleFontColor,
    RingScreenOptionTitleTextPlacement,
    RingScreenOptionNum
} RingScreenOptions;

typedef void (*ringScreenOptionChangeNotifyProc) (CompScreen *screen, CompOption *opt, RingScreenOptions num);

CompOption *ringGetScreenOption (CompScreen *s, RingScreenOptions num);

typedef enum
{
    OverlayIconNone = 0,
    OverlayIconEmblem = 1,
    OverlayIconBig = 2,
} RingOverlayIconEnum;

typedef enum
{
    TitleTextPlacementCenteredOnScreen = 0,
    TitleTextPlacementAboveRing = 1,
    TitleTextPlacementBelowRing = 2,
} RingTitleTextPlacementEnum;

float            ringGetSpeed (CompScreen *s);
CompOption *     ringGetSpeedOption (CompScreen *s);
void             ringSetSpeedNotify (CompScreen *s, ringScreenOptionChangeNotifyProc notify);
            
float            ringGetTimestep (CompScreen *s);
CompOption *     ringGetTimestepOption (CompScreen *s);
void             ringSetTimestepNotify (CompScreen *s, ringScreenOptionChangeNotifyProc notify);
            
int              ringGetInactiveOpacity (CompScreen *s);
CompOption *     ringGetInactiveOpacityOption (CompScreen *s);
void             ringSetInactiveOpacityNotify (CompScreen *s, ringScreenOptionChangeNotifyProc notify);
            
CompMatch *      ringGetWindowMatch (CompScreen *s);
CompOption *     ringGetWindowMatchOption (CompScreen *s);
void             ringSetWindowMatchNotify (CompScreen *s, ringScreenOptionChangeNotifyProc notify);
            
int              ringGetOverlayIcon (CompScreen *s);
CompOption *     ringGetOverlayIconOption (CompScreen *s);
void             ringSetOverlayIconNotify (CompScreen *s, ringScreenOptionChangeNotifyProc notify);
            
Bool             ringGetDarkenBack (CompScreen *s);
CompOption *     ringGetDarkenBackOption (CompScreen *s);
void             ringSetDarkenBackNotify (CompScreen *s, ringScreenOptionChangeNotifyProc notify);
            
Bool             ringGetMinimized (CompScreen *s);
CompOption *     ringGetMinimizedOption (CompScreen *s);
void             ringSetMinimizedNotify (CompScreen *s, ringScreenOptionChangeNotifyProc notify);
            
Bool             ringGetSelectWithMouse (CompScreen *s);
CompOption *     ringGetSelectWithMouseOption (CompScreen *s);
void             ringSetSelectWithMouseNotify (CompScreen *s, ringScreenOptionChangeNotifyProc notify);
            
Bool             ringGetRingClockwise (CompScreen *s);
CompOption *     ringGetRingClockwiseOption (CompScreen *s);
void             ringSetRingClockwiseNotify (CompScreen *s, ringScreenOptionChangeNotifyProc notify);
            
int              ringGetRingWidth (CompScreen *s);
CompOption *     ringGetRingWidthOption (CompScreen *s);
void             ringSetRingWidthNotify (CompScreen *s, ringScreenOptionChangeNotifyProc notify);
            
int              ringGetRingHeight (CompScreen *s);
CompOption *     ringGetRingHeightOption (CompScreen *s);
void             ringSetRingHeightNotify (CompScreen *s, ringScreenOptionChangeNotifyProc notify);
            
int              ringGetThumbWidth (CompScreen *s);
CompOption *     ringGetThumbWidthOption (CompScreen *s);
void             ringSetThumbWidthNotify (CompScreen *s, ringScreenOptionChangeNotifyProc notify);
            
int              ringGetThumbHeight (CompScreen *s);
CompOption *     ringGetThumbHeightOption (CompScreen *s);
void             ringSetThumbHeightNotify (CompScreen *s, ringScreenOptionChangeNotifyProc notify);
            
float            ringGetMinBrightness (CompScreen *s);
CompOption *     ringGetMinBrightnessOption (CompScreen *s);
void             ringSetMinBrightnessNotify (CompScreen *s, ringScreenOptionChangeNotifyProc notify);
            
float            ringGetMinScale (CompScreen *s);
CompOption *     ringGetMinScaleOption (CompScreen *s);
void             ringSetMinScaleNotify (CompScreen *s, ringScreenOptionChangeNotifyProc notify);
            
Bool             ringGetWindowTitle (CompScreen *s);
CompOption *     ringGetWindowTitleOption (CompScreen *s);
void             ringSetWindowTitleNotify (CompScreen *s, ringScreenOptionChangeNotifyProc notify);
            
Bool             ringGetTitleFontBold (CompScreen *s);
CompOption *     ringGetTitleFontBoldOption (CompScreen *s);
void             ringSetTitleFontBoldNotify (CompScreen *s, ringScreenOptionChangeNotifyProc notify);
            
int              ringGetTitleFontSize (CompScreen *s);
CompOption *     ringGetTitleFontSizeOption (CompScreen *s);
void             ringSetTitleFontSizeNotify (CompScreen *s, ringScreenOptionChangeNotifyProc notify);
            
unsigned short * ringGetTitleBackColor (CompScreen *s);
unsigned short   ringGetTitleBackColorRed (CompScreen *s);
unsigned short   ringGetTitleBackColorGreen (CompScreen *s);
unsigned short   ringGetTitleBackColorBlue (CompScreen *s);
unsigned short   ringGetTitleBackColorAlpha (CompScreen *s);
CompOption *     ringGetTitleBackColorOption (CompScreen *s);
void             ringSetTitleBackColorNotify (CompScreen *s, ringScreenOptionChangeNotifyProc notify);
            
unsigned short * ringGetTitleFontColor (CompScreen *s);
unsigned short   ringGetTitleFontColorRed (CompScreen *s);
unsigned short   ringGetTitleFontColorGreen (CompScreen *s);
unsigned short   ringGetTitleFontColorBlue (CompScreen *s);
unsigned short   ringGetTitleFontColorAlpha (CompScreen *s);
CompOption *     ringGetTitleFontColorOption (CompScreen *s);
void             ringSetTitleFontColorNotify (CompScreen *s, ringScreenOptionChangeNotifyProc notify);
            
int              ringGetTitleTextPlacement (CompScreen *s);
CompOption *     ringGetTitleTextPlacementOption (CompScreen *s);
void             ringSetTitleTextPlacementNotify (CompScreen *s, ringScreenOptionChangeNotifyProc notify);
            
#ifdef  __cplusplus
}
#endif

#endif
