/*------------------------------------------------------------------------------
//
//	about.c
//
*/

#include "Devices.h"
#include "Dialogs.h"
#include "Errors.h"
#include "Events.h"
#include "Files.h"
#include "Fonts.h"
#include "Memory.h"
#include "Menus.h"
#include "OSUtils.h"
#include "Packages.h"
#include "QuickDraw.h"
#include "Resources.h"
#include "Script.h"
#include "TextEdit.h"
#include "TextUtils.h"
#include "ToolUtils.h"
#include "Types.h"

#include "util.h"

#include "about.h"

/*------------------------------------------------------------------------------
//
//	Definitions
//
*/

#define kAboutTextRsrcID 128
#define kAboutWindowWidth 420
#define kAboutWindowHeight 270
#define kAboutStdBorder 5
#define kAboutWOKButtonWidth 70
#define kAboutWOKButtonHeight 20
#define kAboutFortunesListID 129
#define kAboutFortuneHeight 36

#define kAboutStringsListID 128
enum {
	ABOUTSTR_WindowName = 1,
	ABOUTSTR_OKButtonTitle
	};

/*------------------------------------------------------------------------------
//
//	Globales
//
*/

WindowPtr			  gAboutWindowPtr = NULL;
TEHandle			  gAboutTEHandle = NULL;
ControlHandle		  gAboutScrollBar = NULL;
ControlActionUPP	  gAboutSBarActionUPP = NULL;
ControlHandle		  gAboutOKButton = NULL;
Str255			  gAboutFortune = "";
ushort			  gAboutFortuneNum = 0;

/*------------------------------------------------------------------------------
//
//	ABOUT_ShowAboutBox
//
*/

void ABOUT_ShowAboutBox (void)

	{
	GrafPtr			  mySavePort;
	GrafPtr			  myDeskPort;
	Rect				  myDeskRect;
	Rect				  myWindowBounds;
	Rect				  myTextDestRect;
	Rect				  myTextViewRect;
	Handle			  myTextHandle;
	Handle			  myStylHandle;
	char				  myDefaultText[4] = "???";
	Rect				  myScrollBarRect;
	short			  myScrollBarMax = 0;
	Str255			  myTempStr255;
	Rect				  myOKButtonRect;

	if (gAboutWindowPtr != NULL) return;

	/*---- Window ----*/

	GetWMgrPort(&myDeskPort);
	myDeskRect = myDeskPort->portRect;
	myDeskRect.top += GetMBarHeight();

	myWindowBounds.top = myDeskRect.top + 30;
	myWindowBounds.left = myDeskRect.left + 50;
	myWindowBounds.bottom = myWindowBounds.top + kAboutWindowHeight;
	myWindowBounds.right = myWindowBounds.left + kAboutWindowWidth;

	GetIndString(myTempStr255, kAboutStringsListID, ABOUTSTR_WindowName);

	if (mColorQDPresent)
		{
		gAboutWindowPtr = NewCWindow(NULL, &myWindowBounds, myTempStr255,
			false, movableDBoxProc, (WindowPtr)-1L, true, 0L);
		}
	else
		{
		gAboutWindowPtr = NewWindow(NULL, &myWindowBounds, myTempStr255,
			false, movableDBoxProc, (WindowPtr)-1L, true, 0L);
		};
	if (gAboutWindowPtr == NULL) return;

	/*---- TEHandle ----*/

	GetPort(&mySavePort);
	SetPort(gAboutWindowPtr);
	TextFont(kFontIDMonaco);
	TextSize(9);
	ABOUT_GetAboutTextRect(&myTextViewRect);
	myTextDestRect = myTextViewRect;
	myTextDestRect.bottom = 32000;
	gAboutTEHandle = TEStyleNew(&myTextDestRect, &myTextViewRect);
	if (gAboutTEHandle != NULL)
		{
		TEActivate(gAboutTEHandle);
		myTextHandle = GetResource('TEXT', kAboutTextRsrcID);
		myStylHandle = GetResource('styl', kAboutTextRsrcID);
		if (myTextHandle != NULL)
			{
			DetachResource(myTextHandle);
			if (myStylHandle != NULL) DetachResource(myStylHandle);
			HLock(myTextHandle);
			TEStyleInsert(*myTextHandle, GetHandleSize(myTextHandle),
				(StScrpHandle)myStylHandle, gAboutTEHandle);
			HUnlock(myTextHandle);
			DisposeHandle(myTextHandle);
			if (myStylHandle != NULL) DisposeHandle(myStylHandle);
			}
		else TEInsert((Ptr)myDefaultText, 3, gAboutTEHandle);
		};
	SetPort(mySavePort);

	/*---- ScrollBar ----*/

	if (gAboutTEHandle != NULL)
		{
		myScrollBarMax = TEGetHeight(32000, 0, gAboutTEHandle);
		myScrollBarMax -= myTextViewRect.bottom - myTextViewRect.top;
		};

	myScrollBarRect = myTextViewRect;
	myScrollBarRect.left = myTextViewRect.right;
	myScrollBarRect.right = myScrollBarRect.left + 16;

	GetPort(&mySavePort);
	SetPort(gAboutWindowPtr);
	gAboutScrollBar = NewControl(gAboutWindowPtr, &myScrollBarRect,
		"\p", true, 0, 0, myScrollBarMax, scrollBarProc, 0L);
	SetPort(mySavePort);

	gAboutSBarActionUPP = NewControlActionProc(ABOUT_ScrollBarCtlAction);

	/*---- OK Button ----*/

	ABOUT_GetAboutOKButtonRect(&myOKButtonRect);
	GetIndString(myTempStr255, kAboutStringsListID, ABOUTSTR_OKButtonTitle);

	GetPort(&mySavePort);
	SetPort(gAboutWindowPtr);
	gAboutOKButton = NewControl(gAboutWindowPtr, &myOKButtonRect,
		myTempStr255, true, 0, 0, 1, pushButProc, 0L);
	SetPort(mySavePort);

	/*---- Fortune ----*/

	ABOUT_PickAFortune(true);

	/*---- Show It ----*/

	ShowWindow(gAboutWindowPtr);

	} /* ABOUT_ShowAboutBox */

/*------------------------------------------------------------------------------
//
//	ABOUT_HideAboutBox
//
*/

void ABOUT_HideAboutBox (void)

	{

	if (gAboutWindowPtr == NULL) return;

	if (gAboutTEHandle != NULL)
		{
		TEDeactivate(gAboutTEHandle);
		TEDispose(gAboutTEHandle);
		};

	DisposeWindow(gAboutWindowPtr);

	gAboutWindowPtr = NULL;
	gAboutTEHandle = NULL;
	gAboutScrollBar = NULL;

	DisposeRoutineDescriptor(gAboutSBarActionUPP);
	gAboutSBarActionUPP = NULL;

	} /* ABOUT_HideAboutBox */

/*------------------------------------------------------------------------------
//
//	ABOUT_DrawAboutBox
//
*/

void ABOUT_DrawAboutBox (void)

	{
	GrafPtr			  mySavePort;
	Rect				  myTextRect;
	Rect				  myTempRect;
	Rect				  myFortuneRect;

	if (gAboutWindowPtr == NULL) return;

	GetPort(&mySavePort);
	SetPort(gAboutWindowPtr);

	ABOUT_GetAboutTextRect(&myTextRect);
	myTempRect = myTextRect;
	myTempRect.right += 16;
	InsetRect(&myTempRect, -2, -2);
	FrameRect(&myTempRect);

	if (gAboutTEHandle != NULL)
		{
		TEUpdate(&gAboutWindowPtr->portRect, gAboutTEHandle);
		};

	ABOUT_GetAboutFortuneRect(&myFortuneRect);
	TextFont(kFontIDGeneva);
	TextSize(9);
	TETextBox(&gAboutFortune[1], (unsigned long)gAboutFortune[0],
		&myFortuneRect, teJustLeft);

	DrawControls(gAboutWindowPtr);

	SetPort(mySavePort);

	} /* ABOUT_DrawAboutBox */

/*------------------------------------------------------------------------------
//
//	ABOUT_DoMouseDown_InContent
//
*/

void ABOUT_DoMouseDown_InContent (Point theMouseLoc)

	{
	GrafPtr			  mySavePort;
	Point			  myMouseLoc;
	Rect				  myTextRect;
	Point			  myOldMouseLoc;
	short			  myCtlPartCode;
	ControlHandle		  myEvtControlHandle;
	short			  myOldScrollBarValue;
	short			  myNewScrollBarValue;
	Rect				  myFortuneRect;

	if (gAboutWindowPtr == NULL) return;

	ABOUT_GetAboutFortuneRect(&myFortuneRect);
	if (PtInRect(theMouseLoc, &myFortuneRect))
		{
		ABOUT_PickAFortune(false);
		GetPort(&mySavePort);
		SetPort(gAboutWindowPtr);
		InvalRect(&myFortuneRect);
		SetPort(mySavePort);
		return;
		};

	if (gAboutTEHandle != NULL)
		{
		ABOUT_GetAboutTextRect(&myTextRect);

		if (PtInRect(theMouseLoc, &myTextRect))
			{
			myMouseLoc = theMouseLoc;
			myOldMouseLoc = myMouseLoc;

			while(StillDown())
				{
				GetPort(&mySavePort);
				SetPort(gAboutWindowPtr);
				GetMouse(&myMouseLoc);
				SetPort(mySavePort);

				TEPinScroll(0,
					myMouseLoc.v - myOldMouseLoc.v,
					gAboutTEHandle);
				ABOUT_UpdateScrollBarValue();

				myOldMouseLoc = myMouseLoc;
				};

			return;
			};
		};

	myCtlPartCode = FindControl(theMouseLoc, gAboutWindowPtr, &myEvtControlHandle);

	if (myCtlPartCode != 0)
		{
		switch (myCtlPartCode)
			{
		case kControlButtonPart:
			if (TrackControl(myEvtControlHandle, theMouseLoc, NULL) == kControlButtonPart)
				{
				ABOUT_HideAboutBox();
				};
			break;
		case kControlUpButtonPart:
		case kControlDownButtonPart:
		case kControlPageUpPart:
		case kControlPageDownPart:
			TrackControl(myEvtControlHandle, theMouseLoc, gAboutSBarActionUPP);
			break;
		case kControlIndicatorPart:
			myOldScrollBarValue = GetControlValue(gAboutScrollBar);

			if (TrackControl(myEvtControlHandle, theMouseLoc, NULL) == kControlIndicatorPart)
				{
				myNewScrollBarValue = GetControlValue(gAboutScrollBar);
				TEPinScroll(0,
					myOldScrollBarValue - myNewScrollBarValue,
					gAboutTEHandle);
				ABOUT_UpdateScrollBarValue();
				};
			break;
			};
		return;
		};

	} /* ABOUT_DoMouseDown_InContent */

/*------------------------------------------------------------------------------
//
//	ABOUT_ScrollBarCtlAction
//
*/

pascal void ABOUT_ScrollBarCtlAction (ControlHandle theControlHandle, short theControlPart)

	{
	short			  myScrollAmount = 0;

	if (theControlHandle != gAboutScrollBar) return;
	if (theControlPart == 0) return;

	switch (theControlPart)
		{
	case kControlUpButtonPart:	myScrollAmount =  10 ; break;
	case kControlDownButtonPart:	myScrollAmount = -10 ; break;
	case kControlPageUpPart:		myScrollAmount =  100; break;
	case kControlPageDownPart:	myScrollAmount = -100; break;
	default: return;
		};

	TEPinScroll(0, myScrollAmount, gAboutTEHandle);
	ABOUT_UpdateScrollBarValue();

	} /* ABOUT_ScrollBarCtlAction */

/*------------------------------------------------------------------------------
//
//	ABOUT_UpdateScrollBarValue
//
*/

void ABOUT_UpdateScrollBarValue (void)

	{
	short			  myScrollBarValue;

	if (gAboutTEHandle == NULL) return;
	if (gAboutScrollBar == NULL) return;

	myScrollBarValue  = (*gAboutTEHandle)->viewRect.top;
	myScrollBarValue -= (*gAboutTEHandle)->destRect.top;

	SetControlValue(gAboutScrollBar, myScrollBarValue);

	} /* ABOUT_UpdateScrollBarValue */

/*------------------------------------------------------------------------------
//
//	ABOUT_DoKeyDown
//
*/

void ABOUT_DoKeyDown (EventRecord *theEventPtr)

	{
	char				  myKeyChar = theEventPtr->message & charCodeMask;

	if (gAboutWindowPtr == NULL) return;

	switch (myKeyChar)
		{
	case 0x03: /* return */
	case 0x0D: /* return */
	case 0x1B: /* escape */
		ABOUT_HideAboutBox();
		break;
	case 0x1E: /* Up Arrow */
		if (gAboutTEHandle == NULL) break;
		TEPinScroll(0, 20, gAboutTEHandle);
		ABOUT_UpdateScrollBarValue();
		break;
	case 0x1F: /* Down Arrow */
		if (gAboutTEHandle == NULL) break;
		TEPinScroll(0, -20, gAboutTEHandle);
		ABOUT_UpdateScrollBarValue();
		break;
		};

	} /* ABOUT_DoKeyDown */

/*------------------------------------------------------------------------------
//
//	ABOUT_IsMouseInTextRect
//
*/

Boolean ABOUT_IsMouseInTextRect (void)

	{
	GrafPtr			  mySavePort;
	Point			  myMouseLoc;
	Rect				  myTextRect;

	if (gAboutWindowPtr == NULL) return false;
	if (gAboutWindowPtr != FrontWindow()) return false;

	GetPort(&mySavePort);
	SetPort(gAboutWindowPtr);
	GetMouse(&myMouseLoc);
	SetPort(mySavePort);

	ABOUT_GetAboutTextRect(&myTextRect);

	return PtInRect(myMouseLoc, &myTextRect);

	} /* ABOUT_IsMouseInTextRect */

/*------------------------------------------------------------------------------
//
//	ABOUT_GetAboutTextRect
//
*/

void ABOUT_GetAboutTextRect (Rect *theRectPtr)

	{

	SetRect(theRectPtr, 0, 0, 0, 0);

	if (gAboutWindowPtr == NULL) return;

	*theRectPtr = gAboutWindowPtr->portRect;
	InsetRect(theRectPtr, kAboutStdBorder, kAboutStdBorder);
	theRectPtr->right -= 16;
	theRectPtr->bottom -= kAboutFortuneHeight + kAboutStdBorder;

	} /* ABOUT_GetAboutTextRect */

/*------------------------------------------------------------------------------
//
//	ABOUT_GetAboutOKButtonRect
//
*/

void ABOUT_GetAboutOKButtonRect (Rect *theRectPtr)

	{

	SetRect(theRectPtr, 0, 0, 0, 0);

	if (gAboutWindowPtr == NULL) return;

	*theRectPtr = gAboutWindowPtr->portRect;
	InsetRect(theRectPtr, kAboutStdBorder, kAboutStdBorder);
	theRectPtr->top = theRectPtr->bottom - kAboutWOKButtonHeight;
	theRectPtr->left = theRectPtr->right - kAboutWOKButtonWidth;
	OffsetRect(theRectPtr, 2, 2);

	} /* ABOUT_GetAboutOKButtonRect */

/*------------------------------------------------------------------------------
//
//	ABOUT_GetAboutFortuneRect
//
*/

void ABOUT_GetAboutFortuneRect (Rect *theRectPtr)

	{

	SetRect(theRectPtr, 0, 0, 0, 0);

	if (gAboutWindowPtr == NULL) return;

	*theRectPtr = gAboutWindowPtr->portRect;
	InsetRect(theRectPtr, kAboutStdBorder, kAboutStdBorder);
	theRectPtr->top = theRectPtr->bottom - kAboutFortuneHeight;
	theRectPtr->right -= kAboutWOKButtonWidth + kAboutStdBorder;
	OffsetRect(theRectPtr, 2, 2);

	} /* ABOUT_GetAboutFortuneRect */

/*------------------------------------------------------------------------------
//
//	ABOUT_GetAboutWindowPtr
//
*/

WindowPtr ABOUT_GetAboutWindowPtr (void)

	{

	return gAboutWindowPtr;

	} /* ABOUT_GetAboutWindowPtr */

/*------------------------------------------------------------------------------
//
//	ABOUT_PickAFortune
//
*/

void ABOUT_PickAFortune (Boolean theRandomFlag)

	{
	Handle			  myStrListHandle;
	ushort			  myNbOfStrings;

	myStrListHandle = GetResource('STR#', kAboutFortunesListID);
	if (myStrListHandle == NULL) return;

	HLock(myStrListHandle);
	myNbOfStrings = *((ushort *)*myStrListHandle);
	HUnlock(myStrListHandle);

	if (theRandomFlag)
		{
		GetDateTime((unsigned long *)&qd.randSeed);
		gAboutFortuneNum = ((ushort)Random()) % myNbOfStrings;
		}
	else if (++gAboutFortuneNum >= myNbOfStrings) gAboutFortuneNum = 0;

	GetIndString(gAboutFortune, kAboutFortunesListID, 1 + gAboutFortuneNum);

	} /* ABOUT_PickAFortune */

/*------------------------------------------------------------------------------
//
//	That's All, Folks !...
//
*/
