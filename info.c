/*------------------------------------------------------------------------------
//
//	info.c
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
#include "TextUtils.h"
#include "ToolUtils.h"
#include "Types.h"

#include "util.h"
#include "segments.h"
#include "uucd.h"

#include "info.h"

/*------------------------------------------------------------------------------
//
//	Definitions
//
*/

#define kInfoWindowWidth 400
#define kInfoWindowHeight 150
#define kInfoStdBorder 5
#define kInfoOKButtonWidth 60
#define kInfoOKButtonHeight 20
#define kInfoCancelButtonWidth 70
#define kInfoCancelButtonHeight 20

#define kInfoStringsListID 400
enum {
	INFOSTR_WindowName = 1,
	INFOSTR_OKButtonTitle,
	INFOSTR_CancelButtonTitle
	};

#define kInfoDescriptionStrListID 401

/*------------------------------------------------------------------------------
//
//	Globales
//
*/

short			  gInfoSegmentNum = -1;
UUCD_SegmentState	  gInfoSegmentState = kUUCD_SEGSTATE_None;

WindowPtr			  gInfoWindowPtr = NULL;
long				  gLastSegStateClick = 0L;

Rect				  gInfoOKBtnRect;
ControlHandle		  gInfoOKButton = NULL;

Rect				  gInfoCancelBtnRect;
ControlHandle		  gInfoCancelButton = NULL;

Rect				  gInfoBeginSegStateRect;
Rect				  gInfoBeginSegStateSelRect;

Rect				  gInfoBodySegStateRect;
Rect				  gInfoBodySegStateSelRect;

Rect				  gInfoEndSegStateRect;
Rect				  gInfoEndSegStateSelRect;

Rect				  gInfoCompleteSegStateRect;
Rect				  gInfoCompleteSegStateSelRect;

Rect				  gInfoSegmentNameRect;

Rect				  gInfoDescriptionRect;
Rect				  gInfoDescriptionFrame;
Str255			  gInfoDescriptionStr;

/*------------------------------------------------------------------------------
//
//	INFO_GetInfoSegmentNum
//
*/

short INFO_GetInfoSegmentNum (void)

	{

	return gInfoSegmentNum;

	} /* INFO_GetInfoSegmentNum */

/*------------------------------------------------------------------------------
//
//	INFO_ShowInfoWindow
//
*/

void INFO_ShowInfoWindow (short theSegmentNum)

	{
	GrafPtr			  mySavePort;
	GrafPtr			  myDeskPort;
	Rect				  myDeskRect;
	Point			  myWindowLoc;
	Rect				  myWindowBounds;
	ushort			  myWindowWidth;
	Str255			  myTempStr255;
	Rect				  myTempRect;
	UUCD_SegmentHdl	  mySegmentHdl;

	if (gInfoWindowPtr != NULL) return;

	gInfoSegmentNum = theSegmentNum;
	gInfoSegmentState = UUCD_GetSegmentState(gInfoSegmentNum);

	/*---- Window ----*/

	GetWMgrPort(&myDeskPort);
	myDeskRect = myDeskPort->portRect;
	myDeskRect.top += GetMBarHeight();

	myWindowLoc.h = 30;
	myWindowLoc.v = 35;
	GetPort(&mySavePort);
	SetPort(UUCD_GetUUCDWindowPtr());
	LocalToGlobal(&myWindowLoc);
	SetPort(mySavePort);

	myWindowBounds.top = myWindowLoc.v;
	myWindowBounds.left = myWindowLoc.h;
	myWindowBounds.bottom = myWindowBounds.top + kInfoWindowHeight;
	myWindowBounds.right = myWindowBounds.left + kInfoWindowWidth;

	GetIndString(myTempStr255, kInfoStringsListID, INFOSTR_WindowName);

	if (mColorQDPresent)
		{
		gInfoWindowPtr = NewCWindow(NULL, &myWindowBounds, myTempStr255,
			false, movableDBoxProc, (WindowPtr)-1L, true, 0L);
		}
	else
		{
		gInfoWindowPtr = NewWindow(NULL, &myWindowBounds, myTempStr255,
			false, movableDBoxProc, (WindowPtr)-1L, true, 0L);
		};
	if (gInfoWindowPtr == NULL) return;

	/*---- Segment Hilite ----*/

	UUCD_DrawSegment(gInfoSegmentNum,
		UUCD_GetSegmentBgPixPat(gInfoSegmentNum, kUUCD_SEGBG_Info));

	/*---- OK Button ----*/

	gInfoOKBtnRect = gInfoWindowPtr->portRect;
	InsetRect(&gInfoOKBtnRect, kInfoStdBorder, kInfoStdBorder);
	gInfoOKBtnRect.top = gInfoOKBtnRect.bottom - kInfoOKButtonHeight;
	gInfoOKBtnRect.left = gInfoOKBtnRect.right - kInfoOKButtonWidth;

	GetIndString(myTempStr255, kInfoStringsListID, INFOSTR_OKButtonTitle);

	GetPort(&mySavePort);
	SetPort(gInfoWindowPtr);
	gInfoOKButton = NewControl(gInfoWindowPtr, &gInfoOKBtnRect,
		myTempStr255, true, 0, 0, 1, pushButProc, 0L);
	SetPort(mySavePort);

	/*---- Cancel Button ----*/

	gInfoCancelBtnRect.bottom = gInfoOKBtnRect.bottom;
	gInfoCancelBtnRect.top = gInfoCancelBtnRect.bottom - kInfoCancelButtonHeight;
	gInfoCancelBtnRect.right = gInfoOKBtnRect.left - (2 * kInfoStdBorder);
	gInfoCancelBtnRect.left = gInfoCancelBtnRect.right - kInfoCancelButtonWidth;

	GetIndString(myTempStr255, kInfoStringsListID, INFOSTR_CancelButtonTitle);

	GetPort(&mySavePort);
	SetPort(gInfoWindowPtr);
	gInfoCancelButton = NewControl(gInfoWindowPtr, &gInfoCancelBtnRect,
		myTempStr255, true, 0, 0, 1, pushButProc, 0L);
	SetPort(mySavePort);

	/*---- Segment States ----*/

	myWindowWidth = gInfoWindowPtr->portRect.right - gInfoWindowPtr->portRect.left;

	myTempRect.left = gInfoWindowPtr->portRect.left;
	myTempRect.left += myWindowWidth / 8;
	myTempRect.left -= kUUCD_StateIcon_Width / 2;
	myTempRect.right = myTempRect.left + kUUCD_StateIcon_Width;
	myTempRect.top = gInfoWindowPtr->portRect.top;
	myTempRect.top += 2 * kInfoStdBorder;
	myTempRect.bottom = myTempRect.top + kUUCD_StateIcon_Height;
	gInfoBeginSegStateRect = myTempRect;
	gInfoBeginSegStateSelRect = myTempRect;
	InsetRect(&gInfoBeginSegStateSelRect, - kInfoStdBorder, - kInfoStdBorder);

	OffsetRect(&myTempRect, myWindowWidth / 4, 0);
	gInfoBodySegStateRect = myTempRect;
	gInfoBodySegStateSelRect = myTempRect;
	InsetRect(&gInfoBodySegStateSelRect, - kInfoStdBorder, - kInfoStdBorder);

	OffsetRect(&myTempRect, myWindowWidth / 4, 0);
	gInfoEndSegStateRect = myTempRect;
	gInfoEndSegStateSelRect = myTempRect;
	InsetRect(&gInfoEndSegStateSelRect, - kInfoStdBorder, - kInfoStdBorder);

	OffsetRect(&myTempRect, myWindowWidth / 4, 0);
	gInfoCompleteSegStateRect = myTempRect;
	gInfoCompleteSegStateSelRect = myTempRect;
	InsetRect(&gInfoCompleteSegStateSelRect, - kInfoStdBorder, - kInfoStdBorder);

	/*---- Segment Name ----*/

	gInfoSegmentNameRect.top = gInfoBeginSegStateSelRect.bottom + kInfoStdBorder;
	gInfoSegmentNameRect.bottom = gInfoSegmentNameRect.top + kUUCD_SegmentHeight;
	gInfoSegmentNameRect.left = gInfoWindowPtr->portRect.left + kInfoStdBorder;
	gInfoSegmentNameRect.right = gInfoWindowPtr->portRect.right - kInfoStdBorder;

	/*---- Description String ----*/

	gInfoDescriptionFrame.top = gInfoSegmentNameRect.bottom + kInfoStdBorder;
	gInfoDescriptionFrame.bottom = gInfoOKBtnRect.top - (2 * kInfoStdBorder);
	gInfoDescriptionFrame.left = gInfoWindowPtr->portRect.left + kInfoStdBorder;
	gInfoDescriptionFrame.right = gInfoWindowPtr->portRect.right - kInfoStdBorder;

	gInfoDescriptionRect = gInfoDescriptionFrame;
	InsetRect(&gInfoDescriptionRect, kInfoStdBorder, kInfoStdBorder);

	if ((mySegmentHdl = UUCD_GetSegmentHdl(theSegmentNum)) != NULL)
		{
		FSSpecPtr			  myTempFSSpecPtr;
		short			  myStrIndex;
		long				  mySrcFileSize;

		HLock((Handle)mySegmentHdl);
		myTempFSSpecPtr = &((**mySegmentHdl).UUCD_SEGMENT_SrcFileFSSpec);

		gInfoDescriptionStr[0] = '\0';
		myTempStr255[0] = '\0';

		for (GetIndString(myTempStr255, kInfoDescriptionStrListID, (myStrIndex = 1));
			myTempStr255[0] != '\0';
			GetIndString(myTempStr255, kInfoDescriptionStrListID, ++myStrIndex))
			{
			if (myTempStr255[1] == (uchar)'•')
				{
				if ((ushort)myTempStr255[0] == 1) myTempStr255[2] = '?';

				switch (myTempStr255[2])
					{
				case (uchar)'•':
					myTempStr255[0] = (char)1;
					myTempStr255[1] = (char)0x0D;
					break;
				case '1':
					BlockMove(myTempFSSpecPtr->name, myTempStr255, 64L);
					break;
				case '2':
					mySrcFileSize = (**mySegmentHdl).UUCD_SEGMENT_SrcFileSize;
					if (mySrcFileSize >= 1024L)
						{
						mySrcFileSize /= 1024L;
						NumToString(mySrcFileSize, myTempStr255);
						myTempStr255[++myTempStr255[0]] = 'K';
						}
					else NumToString(mySrcFileSize, myTempStr255);
					break;
				case '3':
					NumToString((**mySegmentHdl).UUCD_SEGMENT_FrstSrcLineNum, myTempStr255);
					break;
				case '4':
					NumToString((**mySegmentHdl).UUCD_SEGMENT_LastSrcLineNum, myTempStr255);
					break;
					};
				};

			UTIL_PStrCat(gInfoDescriptionStr, myTempStr255);
			};

		HUnlock((Handle)mySegmentHdl);
		};

	/*---- Show It ----*/

	ShowWindow(gInfoWindowPtr);

	} /* INFO_ShowInfoWindow */

/*------------------------------------------------------------------------------
//
//	INFO_HideInfoWindow
//
*/

void INFO_HideInfoWindow (short theButtonClicked)

	{
	short				  myInfoSegmentNum;
	UUCD_SegmentState		  myOldInfoSegmentState;
	UUCD_SegmentState		  myNewInfoSegmentState;

	if (gInfoWindowPtr == NULL) return;
	if ((theButtonClicked != ok) && (theButtonClicked != cancel)) return;

	myInfoSegmentNum = gInfoSegmentNum;
	myOldInfoSegmentState = UUCD_GetSegmentState(gInfoSegmentNum);
	myNewInfoSegmentState = gInfoSegmentState;

	gInfoSegmentNum = -1;
	gInfoSegmentState = kUUCD_SEGSTATE_None;

	if (theButtonClicked == ok)
		{
		if (myNewInfoSegmentState != myOldInfoSegmentState)
			{
			UUCD_SetSegmentState(myInfoSegmentNum, myNewInfoSegmentState);
			UUCD_CheckSegmentStates();
			UUCD_DrawSegments();
			}
		else UUCD_DrawSegment(myInfoSegmentNum, NULL);
		}
	else UUCD_DrawSegment(myInfoSegmentNum, NULL);

	DisposeWindow(gInfoWindowPtr);
	gInfoWindowPtr = NULL;

	} /* INFO_HideInfoWindow */

/*------------------------------------------------------------------------------
//
//	INFO_GetInfoWindowPtr
//
*/

WindowPtr INFO_GetInfoWindowPtr (void)

	{

	return gInfoWindowPtr;

	} /* INFO_GetInfoWindowPtr */

/*------------------------------------------------------------------------------
//
//	INFO_DrawInfoWindow
//
*/

void INFO_DrawInfoWindow (void)

	{
	GrafPtr			  mySavePort;
	PenState			  mySavePenState;
	Rect				  myButtonOutlineRect;

	if (gInfoWindowPtr == NULL) return;

	GetPort(&mySavePort);
	SetPort(gInfoWindowPtr);
	TextFont(kFontIDGeneva);
	TextSize(9);
	TextFace(0);
	PenNormal();

	EraseRect(&gInfoBeginSegStateSelRect);
	CICN_PlotIcon(&gInfoBeginSegStateRect, UUCD_GetSegStateIcon(kUUCD_SEGSTATE_Begin));
	if (gInfoSegmentState == kUUCD_SEGSTATE_Begin) FrameRect(&gInfoBeginSegStateSelRect);

	EraseRect(&gInfoBodySegStateSelRect);
	CICN_PlotIcon(&gInfoBodySegStateRect, UUCD_GetSegStateIcon(kUUCD_SEGSTATE_Body));
	if (gInfoSegmentState == kUUCD_SEGSTATE_Body) FrameRect(&gInfoBodySegStateSelRect);

	EraseRect(&gInfoEndSegStateSelRect);
	CICN_PlotIcon(&gInfoEndSegStateRect, UUCD_GetSegStateIcon(kUUCD_SEGSTATE_End));
	if (gInfoSegmentState == kUUCD_SEGSTATE_End) FrameRect(&gInfoEndSegStateSelRect);

	EraseRect(&gInfoCompleteSegStateSelRect);
	CICN_PlotIcon(&gInfoCompleteSegStateRect, UUCD_GetSegStateIcon(kUUCD_SEGSTATE_Complete));
	if (gInfoSegmentState == kUUCD_SEGSTATE_Complete) FrameRect(&gInfoCompleteSegStateSelRect);

	INFO_DrawSegmentName();

	TETextBox(&gInfoDescriptionStr[1], (unsigned long)gInfoDescriptionStr[0],
		&gInfoDescriptionRect, teJustLeft);
	FrameRect(&gInfoDescriptionFrame);

	DrawControls(gInfoWindowPtr);

	GetPenState(&mySavePenState);
	myButtonOutlineRect = gInfoOKBtnRect;
	InsetRect(&myButtonOutlineRect, -4, -4);
	PenSize(3, 3);
	FrameRoundRect(&myButtonOutlineRect, 16, 16);
	SetPenState(&mySavePenState);

	SetPort(mySavePort);

	} /* INFO_DrawInfoWindow */

/*------------------------------------------------------------------------------
//
//	INFO_DrawSegmentName
//
*/

void INFO_DrawSegmentName (void)

	{
	GrafPtr			  mySavePort;
	short			  myMaxWidth;
	Str255			  myTempStr255;
	PixPatHandle		  myPixPatHandle;

	if (gInfoWindowPtr == NULL) return;

	GetPort(&mySavePort);
	SetPort(gInfoWindowPtr);

	UUCD_GetSegmentName(gInfoSegmentNum, myTempStr255);
	myMaxWidth = gInfoSegmentNameRect.right - gInfoSegmentNameRect.left;
	TruncString(myMaxWidth - (2 * kInfoStdBorder), myTempStr255, smTruncMiddle);

	if ((myPixPatHandle = UUCD_GetSegmentBgPixPat(gInfoSegmentNum, kUUCD_SEGBG_Standard)) != NULL)
		{
		FillCRect(&gInfoSegmentNameRect, myPixPatHandle);
		}
	else EraseRect(&gInfoSegmentNameRect);

	MoveTo(gInfoSegmentNameRect.left + kInfoStdBorder, gInfoSegmentNameRect.bottom - 4);
	DrawString(myTempStr255);

	SetPort(mySavePort);

	} /* INFO_DrawSegmentName */

/*------------------------------------------------------------------------------
//
//	INFO_GetSegmentStateRect
//
*/

void INFO_GetSegmentStateRect (UUCD_SegmentState theSegmentState, Rect *theRectPtr)

	{

	SetRect(theRectPtr, 0, 0, 0, 0);
	if (theSegmentState == kUUCD_SEGSTATE_Begin) *theRectPtr = gInfoBeginSegStateSelRect;
	if (theSegmentState == kUUCD_SEGSTATE_Body) *theRectPtr = gInfoBodySegStateSelRect;
	if (theSegmentState == kUUCD_SEGSTATE_End) *theRectPtr = gInfoEndSegStateSelRect;
	if (theSegmentState == kUUCD_SEGSTATE_Complete) *theRectPtr = gInfoCompleteSegStateSelRect;

	} /* INFO_GetSegmentStateRect */

/*------------------------------------------------------------------------------
//
//	INFO_SetSegmentState
//
*/

Boolean INFO_SetSegmentState (UUCD_SegmentState theSegmentState)

	{
	GrafPtr			  mySavePort;
	PenState			  mySavePenState;
	Rect				  myOldSegmentStateRect;
	Rect				  myNewSegmentStateRect;
	long				  myOldTicks;
	long				  myNewTicks;
	Boolean			  myDoubleClick = false;

	INFO_GetSegmentStateRect(gInfoSegmentState, &myOldSegmentStateRect);
	INFO_GetSegmentStateRect(theSegmentState, &myNewSegmentStateRect);

	myOldTicks = gLastSegStateClick;
	myNewTicks = TickCount();
	if (theSegmentState == gInfoSegmentState)
		myDoubleClick = ((myNewTicks - myOldTicks) < GetDblTime());
	gLastSegStateClick = myNewTicks;

	GetPort(&mySavePort);
	SetPort(gInfoWindowPtr);

	GetPenState(&mySavePenState);
	PenMode(patBic);
	FrameRect(&myOldSegmentStateRect);
	SetPenState(&mySavePenState);

	FrameRect(&myNewSegmentStateRect);

	SetPort(mySavePort);

	gInfoSegmentState = theSegmentState;

	return myDoubleClick;

	} /* INFO_SetSegmentState */

/*------------------------------------------------------------------------------
//
//	INFO_DoMouseDown_InContent
//
*/

void INFO_DoMouseDown_InContent (Point theMouseLoc)

	{
	short			  myCtlPartCode;
	ControlHandle		  myEvtControlHandle;
	Boolean			  myClickDone = false;
	short			  myButtonClicked = 0;

	if (gInfoWindowPtr == NULL) return;

	if (!myClickDone)
	if (PtInRect(theMouseLoc, &gInfoBeginSegStateSelRect))
		{
		if (INFO_SetSegmentState(kUUCD_SEGSTATE_Begin)) myButtonClicked = ok;
		myClickDone = true;
		};

	if (!myClickDone)
	if (PtInRect(theMouseLoc, &gInfoBodySegStateSelRect))
		{
		if (INFO_SetSegmentState(kUUCD_SEGSTATE_Body)) myButtonClicked = ok;
		myClickDone = true;
		};

	if (!myClickDone)
	if (PtInRect(theMouseLoc, &gInfoEndSegStateSelRect))
		{
		if (INFO_SetSegmentState(kUUCD_SEGSTATE_End)) myButtonClicked = ok;
		myClickDone = true;
		};

	if (!myClickDone)
	if (PtInRect(theMouseLoc, &gInfoCompleteSegStateSelRect))
		{
		if (INFO_SetSegmentState(kUUCD_SEGSTATE_Complete)) myButtonClicked = ok;
		myClickDone = true;
		};

	if (!myClickDone)
		{
		myCtlPartCode = FindControl(theMouseLoc, gInfoWindowPtr, &myEvtControlHandle);

		if (myCtlPartCode != 0)
			{
			switch (myCtlPartCode)
				{
			case kControlButtonPart:
				if (TrackControl(myEvtControlHandle, theMouseLoc, NULL) == kControlButtonPart)
					{
					if (myEvtControlHandle == gInfoOKButton) myButtonClicked = ok;
					if (myEvtControlHandle == gInfoCancelButton) myButtonClicked = cancel;
					};
				break;
				};
			};
		};

	if (myButtonClicked != 0) INFO_HideInfoWindow(myButtonClicked);

	} /* INFO_DoMouseDown_InContent */

/*------------------------------------------------------------------------------
//
//	INFO_DoKeyDown
//
*/

void INFO_DoKeyDown (EventRecord *theEventPtr)

	{

	switch (theEventPtr->message & charCodeMask)
		{
	case 0x03:
	case 0x0D:
		INFO_HideInfoWindow(ok);
		break;
		};

	} /* INFO_DoKeyDown */

/*------------------------------------------------------------------------------
//
//	That's All, Folks !...
//
*/
