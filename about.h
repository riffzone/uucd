/*------------------------------------------------------------------------------
**
**	about.h
**
*/

#ifndef _about_h_
#define _about_h_

/*------------------------------------------------------------------------------
**
**	Prototypes
**
*/

void ABOUT_ShowAboutBox (void);
void ABOUT_HideAboutBox (void);
void ABOUT_DrawAboutBox (void);
void ABOUT_DoMouseDown_InContent (Point theMouseLoc);
pascal void ABOUT_ScrollBarCtlAction
	(ControlHandle theControlHandle, short theControlPart);
void ABOUT_UpdateScrollBarValue (void);
void ABOUT_DoKeyDown (EventRecord *theEventPtr);
Boolean ABOUT_IsMouseInTextRect (void);
void ABOUT_GetAboutTextRect (Rect *theRectPtr);
void ABOUT_GetAboutOKButtonRect (Rect *theRectPtr);
void ABOUT_GetAboutFortuneRect (Rect *theRectPtr);
WindowPtr ABOUT_GetAboutWindowPtr (void);
void ABOUT_PickAFortune (Boolean theRandomFlag);

/*------------------------------------------------------------------------------
*/

#endif

/*------------------------------------------------------------------------------
**
**	That's All, Folks !...
**
*/
