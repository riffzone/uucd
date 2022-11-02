/*------------------------------------------------------------------------------
//
//	uucd.c
//
*/

/* Mac includes */
#include "Aliases.h"
#include "AppleEvents.h"
#include "Controls.h"
#include "Devices.h"
#include "Dialogs.h"
#include "DiskInit.h"
#include "Drag.h"
#include "Errors.h"
#include "Events.h"
#include "Files.h"
#include "Finder.h"
#include "FixMath.h"
#include "Folders.h"
#include "Fonts.h"
#include "Gestalt.h"
#include "Icons.h"
#include "Lists.h"
#include "LowMem.h"
#include "Memory.h"
#include "Menus.h"
#include "Notification.h"
#include "OSUtils.h"
#include "Packages.h"
#include "QuickDraw.h"
#include "Resources.h"
#include "Script.h"
#include "Sound.h"
#include "StandardFile.h"
#include "TextUtils.h"
#include "ToolUtils.h"
#include "Types.h"

/* uucd includes */
#include "util.h"
#include "about.h"
#include "info.h"
#include "segments.h"
#include "prefs.h"
#include "blobs.h"
#include "uucd.h"

/*------------------------------------------------------------------------------
//
//	Documentation
//
*/

/*
//   Format uuencode/uudecode [UNIX]
//   -------------------------------
//
//   Files output by uuencode consist of a header line, followed by a
//   number of body lines, and a trailer line.  The uudecode command ignores
//   any lines preceding the header or following the trailer.  Lines preced-
//   ing a header must not, of course, look like a header.
//
//   The header line is distinguished by having the first six characters by
//   the word ``begin'', followed by a space.  The next item on the line is a
//   mode (in octal) and a string which names the remote file.  A space
//   separates the three items in the header line.
//
//   The body consists of a number of lines, each at most 62 characters long
//   including the trailing new line.  These consist of a character count,
//   followed by encoded characters, followed by a new line.  The character
//   count is a single printing character and represents an integer, the
//   number of bytes the rest of the line represents.  Such integers are
//   always in the range from 0 to 63 and can be determined by subtracting
//   the character space (octal 40) from the character.
//
//   Groups of 3 bytes are stored in 4 characters, with 6 bits per character.
//   All are offset by a space to make the characters print.  The last line
//   may be shorter than the normal 45 bytes.  If the size is not a multiple
//   of 3, this fact can be determined by the value of the count on the last
//   line.  Extra dummy characters are included to make the character count a
//   multiple of 4.  The body is terminated by a line with a count of zero.
//   This line consists of one ASCII space.
//
//   The trailer line consists of "end" on a line by itself.
//
//   Exemple
//   -------
//   
//   begin 400 toto.jpg
//   M_]C_X  02D9)1@ ! 0   0 !  #_VP!#  @&!@<&!0@'!P<)"0@*#!0-# L+
//   M#!D2$P\4'1H?'AT:'!P@)"XG("(L(QP<*#<I+# Q-#0T'R<Y/3@R/"XS-#+_
//   ..............
//   M @@_/^59U*PBM).6,N0 #YC_ %ZTJTC_ %<O_"O\&HUS_7G_ (5_2@CO( S-
//   MOD9KO;1B:"Y9R28TYE^>0/UKD?\ :?,THL?_ )M??]E^JT$HX"N;BU^(VFK;
//   MSR1"2\\)PAV9.;!!]J]:1CR ^U>1N"?_ +1]*_\ [#_SBO74?W!\A0;T444'
//   "_]E"
//    
//   end
//   
//   
//   On voit mieux ci-dessous la composition d'une ligne :
//   
//   M#!D2$P\4'1H?'AT:'!P@)"XG("(L(QP<*#<I+# Q-#0T'R<Y/3@R/"XS-#+_
//   +---------+---------+---------+---------+---------+---------+
//   0123456789012345678901234567890123456789012345678901234567890
//   0         1         2         3         4         5         6
//
//   'M' a pour code #77, c'est-a-dire (' ' + #45)
*/

/*
// Segmentation
// ------------
//
// BEGIN --------- cut here --------- CUT HERE --------- PART n
// ...
// END ----------- cut here --------- CUT HERE --------- PART n
//
*/

/*------------------------------------------------------------------------------
//
//	Definitions
//
*/

#define kOSErrAlertID 666
#define kMemFullAlertID 667
#define kSystem7AlertID 668

#define kUUCDReceivedSoundID 256

#define kDragOutlinePatID 129

#define kUUCDWindowGrowIconSize 16
#define kUUCDWindowGrowCIconID 1999
#define kUUCDButtonHeight 14
#define kUUCDRAZButtonWidth 50
#define kUUCDRAZButtonUpCIconID 2000
#define kUUCDRAZButtonDownCIconID 2001
#define kUUCDSaveAllButtonWidth 50
#define kUUCDSaveAllButtonUpCIconID 2002
#define kUUCDSaveAllButtonDownCIconID 2003
#define kUUCDPrefsButtonWidth 40
#define kUUCDPrefsButtonUpCIconID 2004
#define kUUCDPrefsButtonDownCIconID 2005
#define kUUCDAboutButtonWidth 20
#define kUUCDAboutButtonUpCIconID 2006
#define kUUCDAboutButtonDownCIconID 2007
#define kUUCDSortButtonWidth 40
#define kUUCDSortButtonUpCIconID 2008
#define kUUCDSortButtonDownCIconID 2009
#define kUUCDViewButtonWidth 20
#define kUUCDViewButtonUpCIconID 2010
#define kUUCDViewButtonDownCIconID 2011
#define kUUCDSoundButtonWidth 16
#define kUUCDSoundButtonUpCIconID 2012
#define kUUCDSoundButtonDownCIconID 2013

#define kUUCDEncodeDialogID 512

#define kUUCDCustomSaveDialogID 256
#define kUUCDCustomSaveDialogExtraBtnItem 13
#define kUUCDCustomSaveDialogExtraTxtItem 14

#define kUUCDCmdDot1CursorID 520
#define kUUCDCmdDot2CursorID 521
#define kUUCDMoveCellCursorID 1024
#define kUUCDLoop1CursorID 1025
#define kUUCDLoop2CursorID 1026
#define kUUCDLoop3CursorID 1027
#define kUUCDLoop4CursorID 1028

enum {
	UUCDCURS_None,
	UUCDCURS_Arrow,
	UUCDCURS_Watch,
	UUCDCURS_Plus,
	UUCDCURS_CmdDot1,
	UUCDCURS_CmdDot2,
	UUCDCURS_MoveCell,
	UUCDCURS_Loop1,
	UUCDCURS_Loop2,
	UUCDCURS_Loop3,
	UUCDCURS_Loop4
	};

#define kUUCDStringsListID 256
enum {
	UUCDSTR_Untitled,
	UUCDSTR_SaveAs,
	UUCDSTR_SaveAll,
	UUCDSTR_Skip,
	UUCDSTR_FileAlreadyExists,
	UUCDSTR_UUEncodeAs,
	UUCDSTR_UUEncodeExtension,
	UUCDSTR_UUEncodeBeginCut,
	UUCDSTR_UUEncodeEndCut,
	UUCDSTR_Nb
	};

enum {
	kUUCD_DECODE_STATE_Idle,
	kUUCD_DECODE_STATE_Running,
	kUUCD_DECODE_STATE_Done
	};

typedef struct
	{
	Handle			  DOCLIST_ITEM_Next;
	FSSpec			  DOCLIST_ITEM_FSSpec;

	} DOCLIST_Item, *DOCLIST_ItemPtr, **DOCLIST_ItemHdl;

enum {
	HQXLINETYPE_None,
	HQXLINETYPE_First,
	HQXLINETYPE_Full,
	HQXLINETYPE_Last
	};
typedef ushort HQXLineType;

enum {
	B64LINETYPE_None,
	B64LINETYPE_Full,
	B64LINETYPE_Last
	};
typedef ushort B64LineType;

enum {
	LINETYPE_None,
	LINETYPE_UUCode,
	LINETYPE_Base64,
	LINETYPE_HQX
	};
typedef ushort LineType;

/*------------------------------------------------------------------------------
//
//	Menus
//
*/

#define MENU_Apple 0
#define MENUID_Apple 128
#define MENU_APPLE_About 1

#define MENU_File 1
#define MENUID_File 129
#define MENU_FILE_UUDecode 1
#define MENU_FILE_UUEncode 2
#define MENU_FILE_Preferences 4
#define MENU_FILE_Register 6
#define MENU_FILE_Quit 8

#define MENU_Mark 2
#define MENUID_Mark 130
#define MENU_MARK_ClearMarked 1
#define MENU_MARK_SaveUnmarked 2
#define MENU_MARK_MarkAll 4
#define MENU_MARK_UnmarkAll 5

/*------------------------------------------------------------------------------
//
//	Globales
//
*/

Boolean				  gQuit = false;
EventRecord			  gLastEvent;
Boolean				  gApplicationIsFront = true;
MenuHandle			  gApplicationMenus[7];
short				  gApplicationVRefNum = 0;
long					  gApplicationDirID = 0L;

short				  gUUCD_DECODE_State = kUUCD_DECODE_STATE_Idle;

Boolean				  gDragManagerPresent = false;
RgnHandle				  gDragOutlineRgnHandle = NULL;
PixPatHandle			  gDragOutlinePixPatHandle = NULL;
DragTrackingHandlerUPP	  gDragTrackingHandlerUPP;
DragReceiveHandlerUPP	  gDragReceiveHandlerUPP;

DOCLIST_ItemHdl		  gDocList = NULL;

Str255				  gUUCDString;

short				  gUUCDCursor = UUCDCURS_Arrow;

long					  gUUCDStartFreeMemory = 0L;

Handle				  gUUCDReceivedSoundHandle = NULL;

WindowPtr				  gUUCDWindowPtr = NULL;
RgnHandle				  gUUCDWindowBgRgnHandle = NULL;
PixPatHandle			  gUUCDWindowBgPixPatHandle = NULL;
Rect					  gUUCDProgressRect;
Rect					  gUUCDMemoryRect;
Rect					  gUUCDGrowIconRect;
CICNHandle			  gUUCDGrowCIconHandle = NULL;

Rect					  gUUCDSaveAllButtonRect;
CICNHandle			  gUUCDSaveAllButtonUpCIconHandle = NULL;
CICNHandle			  gUUCDSaveAllButtonDownCIconHandle = NULL;

Rect					  gUUCDRAZButtonRect;
CICNHandle			  gUUCDRAZButtonUpCIconHandle = NULL;
CICNHandle			  gUUCDRAZButtonDownCIconHandle = NULL;

Rect					  gUUCDPrefsButtonRect;
CICNHandle			  gUUCDPrefsButtonUpCIconHandle = NULL;
CICNHandle			  gUUCDPrefsButtonDownCIconHandle = NULL;

Rect					  gUUCDAboutButtonRect;
CICNHandle			  gUUCDAboutButtonUpCIconHandle = NULL;
CICNHandle			  gUUCDAboutButtonDownCIconHandle = NULL;

Rect					  gUUCDSortButtonRect;
CICNHandle			  gUUCDSortButtonUpCIconHandle = NULL;
CICNHandle			  gUUCDSortButtonDownCIconHandle = NULL;

Rect					  gUUCDViewButtonRect;
CICNHandle			  gUUCDViewButtonUpCIconHandle = NULL;
CICNHandle			  gUUCDViewButtonDownCIconHandle = NULL;

Rect					  gUUCDSoundButtonRect;
CICNHandle			  gUUCDSoundButtonUpCIconHandle = NULL;
CICNHandle			  gUUCDSoundButtonDownCIconHandle = NULL;

PixPatHandle			  gUUCDProgressPixPatHandle = NULL;
PixPatHandle			  gUUCDMemoryPixPatHandle = NULL;

ControlHandle			  gUUCDSegListVScrollBar = NULL;
Rect					  gUUCDSegListVSBarRect;
ControlActionUPP		  gUUCDSegListVSBarActionUPP = NULL;
short				  gUUCDFocusSegmentNum = -1;

Rect					  gUUCDCellMoveRect;
Cell					  gUUCDSrcMoveCell;
Cell					  gUUCDDstMoveCell;
Boolean				  gUUCDLClickRunning;

uchar				  gHQXCharSet[80] = "!\"#$%&'()*+,-012345689@ABCDEFGHIJKLMNPQRSTUVXYZ[`abcdefhijklmpqr";
uchar				  gIsHQXChar[256]; /* gIsHQXChar[ASCIICode] = 0xFF if in HQXCharSet, 0x00 if not */

uchar				  gB64CharSet[80] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
uchar				  gIsB64Char[256]; /* gB64CharSet[ASCIICode] = 0xFF if in B64CharSet, 0x00 if not */
char					  gB64HeadMark[40] = "CONTENT-TRANSFER-ENCODING:BASE64";
char					  gB64CDISPMark[40] = "CONTENT-DISPOSITION:";
char					  gB64CFNAMMark[40] = "FILENAME=\"";

/*------------------------------------------------------------------------------
//
//	Prototypes
//
*/

void MAIN_SetUpApplication (void);
void MAIN_CleanUpApplication (void);

void MAIN_HandleEvent (void);
long MAIN_GetSleep (void);
void MAIN_DoIdle (void);
void MAIN_SelectWindow (WindowPtr theWindowPtr);
void MAIN_DoMouseDown (void);
void MAIN_DoKeyDown (void);
void MAIN_DoActivate (WindowPtr theWindowPtr, Boolean theActiveFlag);
Boolean MAIN_DoUpdate (WindowPtr theWindowPtr);
void MAIN_DoOSEvent (void);
void MAIN_DoSuspendResumeEvent (void);
void MAIN_DoHighLevelEvent (void);
void MAIN_DoCommand (long theMenuResult);
void MAIN_ShowAboutBox (void);

void InstallAEHandlers (void);
OSErr AEGotRequiredParams (AppleEvent theAppleEvent);
OSErr SendAEOpenDoc (FSSpec *theFSSpecPtr);
pascal OSErr AEHandler_OAPP (AEDescList *theAppleEvent, AEDescList *theReply, long theHandlerRefCon);
pascal OSErr AEHandler_ODOC (AEDescList *theAppleEvent, AEDescList *theReply, long theHandlerRefCon);
pascal OSErr AEHandler_PDOC (AEDescList *theAppleEvent, AEDescList *theReply, long theHandlerRefCon);
pascal OSErr AEHandler_QUIT (AEDescList *theAppleEvent, AEDescList *theReply, long theHandlerRefCon);

OSErr AESendFinderOpenDoc (FSSpecPtr theFSSpecPtr);

void DRAG_HANDLERS_Install (Rect *theOutlineRectPtr);
void DRAG_HANDLERS_Remove (void);
Boolean DRAG_CanAcceptItems (DragReference theDragReference);
Boolean DRAG_MouseIsInContentRgn (DragReference theDragReference, WindowPtr theWindowPtr);
Boolean DRAG_IsNotInSourceWindow (DragReference theDragReference);
pascal OSErr DRAG_HANDLER_Track (DragTrackingMessage theMessage,
	WindowPtr theWindowPtr, void *theHandlerRefCon, DragReference theDragReference);
pascal OSErr DRAG_HANDLER_Receive (WindowPtr theWindowPtr,
	void *theHandlerRefCon, DragReference theDragReference);

void DOCLIST_Push (FSSpecPtr theFSSpecPtr);
Boolean DOCLIST_Pull (FSSpecPtr theFSSpecPtr);

void UUCD_CalcUUCDWindowRects (short theNbOfVisibleSegments,
		short theSegmentWidth, RectPtr theUUCDWindowBoundsPtr);
Boolean UUCD_TrackCICNButton (Rect *theRectPtr,
	CICNHandle theButtonUpCICNHandle, CICNHandle theButtonDownCICNHandle);
short UUCD_GetCursor (void);
void UUCD_SetCursor (short theUUCDCursorNum);
void UUCD_SetLoopCursor (short theLoopCount);
StringPtr UUCD_GetString (short theStringNum);
void UUCD_OSErrAlert (OSErr theOSErr);
Boolean UUCD_CheckMemory (long theMemoryWanted);
long UUCD_GetFreeMemory (void);

void UUCD_UUDecode (FSSpecPtr theFSSpecPtr);
void UUCD_UUEncode (FSSpecPtr theFSSpecPtr);

void UUCD_DrawProgress (ulong theProgressMax, ulong theProgressVal);
void UUCD_DrawMemory (void);

pascal void UUCD_SegListVSBarActionProc (ControlHandle theControlHandle, short theControlPart);

short UUCD_FindSegment (Point theLocalPoint);
Boolean UUCD_SaveLinkedSegments (short theLinkedBeginSegmentNum);
void UUCD_SaveAllLinkedSegments (void);
void UUCD_RemoveMarkedSegments (void);

OSErr UUCD_Decode (FSSpecPtr theFSSpecPtr);
OSErr UUCD_DECODE_Start (FSSpecPtr theFSSpecPtr);
OSErr UUCD_DECODE_Body (void);
void UUCD_DECODE_End (OSErr theOSErr);
void UUCD_DECODE_NewSegment (UUCD_SegmentType theSegmentType);

short UUCD_IsB64Line (char *theLineCharsPtr, short *theLineLenPtr);
HQXLineType UUCD_IsHQXLine (char *theLineCharsPtr, short *theLineLenPtr);
Boolean UUCD_IsUUFullLine (char *theLineCharsPtr, short *theLineLenPtr);

void UUCD_PlayReceivedSound (void);
void UUCD_DbgWrite (StringPtr theStringPtr);

/*------------------------------------------------------------------------------
//
//	MAIN
//
*/

main ()

	{

	MaxApplZone();
	MoreMasters();
	MoreMasters();
	MoreMasters();
	MoreMasters();

	MAIN_SetUpApplication();

	while (!gQuit) MAIN_HandleEvent();

	MAIN_CleanUpApplication();

	return 0;

	} /* main */

/*------------------------------------------------------------------------------
//
//	MAIN_SetUpApplication
//
*/

void MAIN_SetUpApplication (void)

	{
	GrafPtr			  mySavePort;
	GrafPtr			  myDeskPort;
	Rect				  myDeskRect;
	Rect				  mySectRect;
	SysEnvRec			  mySysEnvRec;
	long				  myGestaltResponse;
	short			  myCharNum;
	Rect				  myWindowBounds;
	Point			  myTempPoint;

	InitGraf(&qd.thePort);
	InitFonts();
	InitWindows();
	InitMenus();
	TEInit();
	InitDialogs(0L);
	InitCursor();

	SysEnvirons(1, &mySysEnvRec);

	if (mySysEnvRec.systemVersion < 0x700)
		{
		Alert(kSystem7AlertID, NULL);
		ExitToShell();
		};

	if (Gestalt(gestaltDragMgrAttr, &myGestaltResponse) == noErr)
		if (BitTst(&myGestaltResponse, 31 - gestaltDragMgrPresent))
			{
			gDragManagerPresent = true;
			};

	/*---- Application VRefNum & DirID ----*/

	gApplicationVRefNum = UTIL_GetAppVRefNum();
	gApplicationDirID = UTIL_GetAppDirID();

	/*---- Transcript ----*/

#if kUsesTranscript
	UTIL_DeleteTranscript();
#endif

	/*---- Blobs ----*/

	UUCD_InitBlobs();

	/*---- Menus ----*/

	gApplicationMenus[MENU_Apple] = GetMenu(MENUID_Apple);
	if (gApplicationMenus[MENU_Apple] != NULL)
		{
		AppendResMenu(gApplicationMenus[MENU_Apple], 'DRVR');
		InsertMenu(gApplicationMenus[MENU_Apple], 0);
		};

	gApplicationMenus[MENU_File] = GetMenu(MENUID_File);
	if (gApplicationMenus[MENU_File] != NULL)
		InsertMenu(gApplicationMenus[MENU_File], 0);

	gApplicationMenus[MENU_Mark] = GetMenu(MENUID_Mark);
	if (gApplicationMenus[MENU_Mark] != NULL)
		InsertMenu(gApplicationMenus[MENU_Mark], 0);

	DrawMenuBar();

	/*---- AppleEvents ----*/

	InstallAEHandlers();

	/*---- Preferences ----*/

	PREF_LoadPreferences();

	/*---- HQX Data ----*/

	for (myCharNum = 0; myCharNum < 256; myCharNum++) gIsHQXChar[myCharNum] = 0x00;

	for (myCharNum = 0; myCharNum < 80; myCharNum++)
		{
		if (gHQXCharSet[myCharNum] == 0x00) break;
		gIsHQXChar[(ushort)gHQXCharSet[myCharNum]] = 0xFF;
		};

	/*---- B64 Data ----*/

	for (myCharNum = 0; myCharNum < 256; myCharNum++) gIsB64Char[myCharNum] = 0x00;

	for (myCharNum = 0; myCharNum < 80; myCharNum++)
		{
		if (gB64CharSet[myCharNum] == 0x00) break;
		gIsB64Char[(ushort)gB64CharSet[myCharNum]] = 0xFF;
		};

	/*---- UUCDWindow ----*/

	PREF_GetUUCDWindowSiz(&myTempPoint);
	UUCD_CalcUUCDWindowRects(myTempPoint.v, myTempPoint.h, &myWindowBounds);

	GetWMgrPort(&myDeskPort);
	myDeskRect = myDeskPort->portRect;
	SectRect(&myWindowBounds, &myDeskRect, &mySectRect);
	if (!EqualRect(&myWindowBounds, &mySectRect))
		{
		myTempPoint.h = kUUCDDefaultWindowPosH;
		myTempPoint.v = kUUCDDefaultWindowPosV;
		PREF_SetUUCDWindowPos(myTempPoint);
		myTempPoint.h = kUUCDDefaultSegmentWidth;
		myTempPoint.v = kUUCDDefaultNbOfVisibleSegments;
		PREF_SetUUCDWindowSiz(myTempPoint);
		UUCD_CalcUUCDWindowRects(myTempPoint.v, myTempPoint.h, &myWindowBounds);
		};

	if (mColorQDPresent)
		{
		gUUCDWindowPtr = NewCWindow(NULL, &myWindowBounds, "\puucd",
			true, documentProc, (WindowPtr)-1L, true, 0L);
		}
	else gUUCDWindowPtr = NewWindow(NULL, &myWindowBounds, "\puucd",
			true, documentProc, (WindowPtr)-1L, true, 0L);

	if (gUUCDWindowPtr == NULL) return;

	GetPort(&mySavePort);
	SetPort(gUUCDWindowPtr);
	gUUCDSegListVScrollBar = NewControl(gUUCDWindowPtr, &gUUCDSegListVSBarRect,
		"\p", true, 0, 0, 0, scrollBarProc, kUUCDSegListVSBarRefCon);
	gUUCDSegListVSBarActionUPP = NewControlActionProc(UUCD_SegListVSBarActionProc);
	SetPort(mySavePort);

	/*---- Icones ----*/

	gUUCDGrowCIconHandle = CICN_GetIcon(kUUCDWindowGrowCIconID);
	gUUCDSaveAllButtonUpCIconHandle = CICN_GetIcon(kUUCDSaveAllButtonUpCIconID);
	gUUCDSaveAllButtonDownCIconHandle = CICN_GetIcon(kUUCDSaveAllButtonDownCIconID);
	gUUCDRAZButtonUpCIconHandle = CICN_GetIcon(kUUCDRAZButtonUpCIconID);
	gUUCDRAZButtonDownCIconHandle = CICN_GetIcon(kUUCDRAZButtonDownCIconID);
	gUUCDAboutButtonUpCIconHandle = CICN_GetIcon(kUUCDAboutButtonUpCIconID);
	gUUCDAboutButtonDownCIconHandle = CICN_GetIcon(kUUCDAboutButtonDownCIconID);
	gUUCDPrefsButtonUpCIconHandle = CICN_GetIcon(kUUCDPrefsButtonUpCIconID);
	gUUCDPrefsButtonDownCIconHandle = CICN_GetIcon(kUUCDPrefsButtonDownCIconID);
	gUUCDSortButtonUpCIconHandle = CICN_GetIcon(kUUCDSortButtonUpCIconID);
	gUUCDSortButtonDownCIconHandle = CICN_GetIcon(kUUCDSortButtonDownCIconID);
	gUUCDViewButtonUpCIconHandle = CICN_GetIcon(kUUCDViewButtonUpCIconID);
	gUUCDViewButtonDownCIconHandle = CICN_GetIcon(kUUCDViewButtonDownCIconID);
	gUUCDSoundButtonUpCIconHandle = CICN_GetIcon(kUUCDSoundButtonUpCIconID);
	gUUCDSoundButtonDownCIconHandle = CICN_GetIcon(kUUCDSoundButtonDownCIconID);

	/*---- Patterns ----*/

	if (mColorQDPresent)
		{
		gUUCDProgressPixPatHandle = GetPixPat(kUUCDProgressPixPatID);
		gUUCDMemoryPixPatHandle = GetPixPat(kUUCDMemoryPixPatID);
		gUUCDWindowBgPixPatHandle = GetPixPat(kUUCDWindowBgPatID);
		gDragOutlinePixPatHandle = GetPixPat(kDragOutlinePatID);
		}
	else
		{
		gUUCDProgressPixPatHandle = NULL;
		gUUCDMemoryPixPatHandle = NULL;
		gUUCDWindowBgPixPatHandle = NULL;
		gDragOutlinePixPatHandle = NULL;
		};

	/*---- Sons ----*/

	if ((gUUCDReceivedSoundHandle = GetResource('snd ', kUUCDReceivedSoundID)) != NULL)
		DetachResource(gUUCDReceivedSoundHandle);

	/*---- DragHandlers ----*/

	if (gDragManagerPresent)
		{
		if ((gDragOutlineRgnHandle = NewRgn()) != NULL)
			{
			Rect				  myTempRect;
			RgnHandle			  myTempRgn;
	
			myTempRect = gUUCDWindowPtr->portRect;
			myTempRect.top = gUUCDProgressRect.bottom;
	
			RectRgn(gDragOutlineRgnHandle, &myTempRect);
	
			if ((myTempRgn = NewRgn()) != NULL)
				{
				InsetRect(&myTempRect, 3, 3);
				RectRgn(myTempRgn, &myTempRect);
				DiffRgn(gDragOutlineRgnHandle, myTempRgn, gDragOutlineRgnHandle);
				}
			else
				{
				DisposeRgn(gDragOutlineRgnHandle);
				gDragOutlineRgnHandle = NULL;
				};
			};

		gDragTrackingHandlerUPP = NewDragTrackingHandlerProc(DRAG_HANDLER_Track);
		InstallTrackingHandler(gDragTrackingHandlerUPP, gUUCDWindowPtr, NULL);
		gDragReceiveHandlerUPP = NewDragReceiveHandlerProc(DRAG_HANDLER_Receive);
		InstallReceiveHandler(gDragReceiveHandlerUPP, gUUCDWindowPtr, NULL);
		};

	/*---- FreeMem ----*/

	gUUCDStartFreeMemory = UUCD_GetFreeMemory();

	} /* MAIN_SetUpApplication */

/*------------------------------------------------------------------------------
//
//	MAIN_CleanUpApplication
//
*/

#define mCICN_WipeOut(theCicn) if ((theCicn) != NULL) { CICN_DisposIcon(theCicn); (theCicn) = NULL; };
#define mPPAT_WipeOut(thePPat) if ((thePPat) != NULL) { DisposePixPat(thePPat); (thePPat) = NULL; };

void MAIN_CleanUpApplication (void)

	{

	PREF_SavePreferences();

	UUCD_ExitBlobs();

	if (gUUCDWindowPtr != NULL)
		{
		if (gDragManagerPresent)
			{
			RemoveTrackingHandler(gDragTrackingHandlerUPP, gUUCDWindowPtr);
			RemoveReceiveHandler(gDragReceiveHandlerUPP, gUUCDWindowPtr);
			};

		DisposeWindow(gUUCDWindowPtr);
		gUUCDWindowPtr = NULL;
		};

	mCICN_WipeOut(gUUCDGrowCIconHandle);
	mCICN_WipeOut(gUUCDSaveAllButtonUpCIconHandle);
	mCICN_WipeOut(gUUCDSaveAllButtonDownCIconHandle);
	mCICN_WipeOut(gUUCDRAZButtonUpCIconHandle);
	mCICN_WipeOut(gUUCDRAZButtonDownCIconHandle);
	mCICN_WipeOut(gUUCDAboutButtonUpCIconHandle);
	mCICN_WipeOut(gUUCDAboutButtonDownCIconHandle);
	mCICN_WipeOut(gUUCDPrefsButtonUpCIconHandle);
	mCICN_WipeOut(gUUCDPrefsButtonDownCIconHandle);
	mCICN_WipeOut(gUUCDSortButtonUpCIconHandle);
	mCICN_WipeOut(gUUCDSortButtonDownCIconHandle);
	mCICN_WipeOut(gUUCDViewButtonUpCIconHandle);
	mCICN_WipeOut(gUUCDViewButtonDownCIconHandle);
	mCICN_WipeOut(gUUCDSoundButtonUpCIconHandle);
	mCICN_WipeOut(gUUCDSoundButtonDownCIconHandle);

	mPPAT_WipeOut(gUUCDProgressPixPatHandle);
	mPPAT_WipeOut(gUUCDMemoryPixPatHandle);
	mPPAT_WipeOut(gUUCDWindowBgPixPatHandle);
	mPPAT_WipeOut(gDragOutlinePixPatHandle);

	if (gUUCDReceivedSoundHandle != NULL)
		{
		DisposeHandle(gUUCDReceivedSoundHandle);
		gUUCDReceivedSoundHandle = NULL;
		};

	} /* MAIN_CleanUpApplication */

/*------------------------------------------------------------------------------
//
//	MAIN_HandleEvent
//
*/

void MAIN_HandleEvent (void)

	{

	if (!WaitNextEvent(everyEvent, &gLastEvent, MAIN_GetSleep(), NULL))
		{
		MAIN_DoIdle();
		return;
		};

	switch (gLastEvent.what)
		{
	case mouseDown:
		MAIN_DoMouseDown();
		break;
	case keyDown:
	case autoKey:
		MAIN_DoKeyDown();
		break;
	case activateEvt:
		MAIN_DoActivate((WindowPtr)gLastEvent.message,
			gLastEvent.modifiers & activeFlag);
		break;
	case updateEvt:
		MAIN_DoUpdate((WindowPtr)gLastEvent.message);
		break;
	case diskEvt:
		if (HiWord(gLastEvent.message) != noErr)
			{
			Point myTopLeft;
			SetPt(&myTopLeft, 70, 50);
			DIBadMount(myTopLeft, gLastEvent.message);
			};
		break;
	case osEvt:
		MAIN_DoOSEvent();
		break;
	case kHighLevelEvent:
		MAIN_DoHighLevelEvent();
		break;
		};

	} /* MAIN_HandleEvent */

/*------------------------------------------------------------------------------
//
//	MAIN_GetSleep
//
*/

long MAIN_GetSleep (void)

	{

	return 10L;

	} /* MAIN_GetSleep */

/*------------------------------------------------------------------------------
//
//	MAIN_DoIdle
//
*/

void MAIN_DoIdle (void)

	{
	short			  myNewCursor = UUCDCURS_Arrow;
	short			  myNewFocusSegmentNum = -1;

	if (ABOUT_IsMouseInTextRect()) myNewCursor = UUCDCURS_MoveCell;

	if ((gUUCDWindowPtr != NULL) && (FrontWindow() == gUUCDWindowPtr))
		{
		GrafPtr			  mySavePort;
		Point			  myMouseLoc;
		short			  myLocalSegmentNum;
		short			  mySegmentNum;
		Rect				  mySegmentRect;
		Rect				  mySaveIconRect;
		Rect				  myInfoIconRect;
		Rect				  myMarkIconRect;

		GetPort(&mySavePort);
		SetPort(gUUCDWindowPtr);
		GetMouse(&myMouseLoc);
		SetPort(mySavePort);

		if (PtInRect(myMouseLoc, UUCD_GetSegListViewRect()))
			{
			myNewCursor = UUCDCURS_Arrow;

			for (myLocalSegmentNum = 0;
				myLocalSegmentNum < UUCD_GetNbOfVisibleSegments();
				myLocalSegmentNum++)
				{
				mySegmentNum = UUCD_GetTopSegmentNum() + myLocalSegmentNum;
				if (!UUCD_GetSegmentRect(mySegmentNum, &mySegmentRect)) break;

				if (PtInRect(myMouseLoc, &mySegmentRect))
					{
					myNewFocusSegmentNum = mySegmentNum;
					myNewCursor = UUCDCURS_Plus;

					if (UUCD_SegmentHasSaveIcon(mySegmentNum))
						{
						UUCD_GetSegmentSaveIconRect(&mySegmentRect, &mySaveIconRect);
						if (PtInRect(myMouseLoc, &mySaveIconRect))
							{
							myNewCursor = UUCDCURS_Arrow;
							};
						};

					UUCD_GetSegmentInfoIconRect(&mySegmentRect, &myInfoIconRect);
					if (PtInRect(myMouseLoc, &myInfoIconRect))
						{
						myNewCursor = UUCDCURS_Arrow;
						};

					UUCD_GetSegmentMarkIconRect(&mySegmentRect, &myMarkIconRect);
					if (PtInRect(myMouseLoc, &myMarkIconRect))
						{
						myNewCursor = UUCDCURS_Arrow;
						};

					break;
					};
				};
			}
		else myNewCursor = UUCDCURS_Arrow;
		};

	if (myNewFocusSegmentNum != gUUCDFocusSegmentNum)
		{
		gUUCDFocusSegmentNum = myNewFocusSegmentNum;
		};

	UUCD_SetCursor(myNewCursor);

	UUCD_Decode(NULL);

	if (gUUCD_DECODE_State == kUUCD_DECODE_STATE_Idle)
		{
		FSSpec			  myFSSpec;

		if (DOCLIST_Pull(&myFSSpec))
			{
			UUCD_Decode(&myFSSpec);
			UTIL_SetSFDir(myFSSpec.vRefNum, myFSSpec.parID);
			};
		};

	UTIL_RemoveNotification();

	} /* MAIN_DoIdle */

/*------------------------------------------------------------------------------
//
//	MAIN_SelectWindow
//
*/

void MAIN_SelectWindow (WindowPtr theWindowPtr)

	{
	WindowPtr			  myFrontWindowPtr;
	WindowPtr			  myAboutWindowPtr;
	WindowPtr			  myInfoWindowPtr;

	myFrontWindowPtr = FrontWindow();

	if ((myAboutWindowPtr = ABOUT_GetAboutWindowPtr()) != NULL)
		if (myAboutWindowPtr == myFrontWindowPtr)
			{
			SysBeep(1);
			return;
			};

	if ((myInfoWindowPtr = INFO_GetInfoWindowPtr()) != NULL)
		if (myInfoWindowPtr == myFrontWindowPtr)
			{
			SysBeep(1);
			return;
			};

	SelectWindow(theWindowPtr);

	} /* MAIN_SelectWindow */

/*------------------------------------------------------------------------------
//
//	MAIN_DoMouseDown
//
*/

void MAIN_DoMouseDown (void)

	{
	GrafPtr			  mySavePort;
	short			  myPartCode;
	WindowPtr			  myEvtWindowPtr;
	Point			  myMouseLoc;
	Rect				  myTempRect;
	WindowPtr			  myAboutWindowPtr;
	WindowPtr			  myInfoWindowPtr;

	myPartCode = FindWindow(gLastEvent.where, &myEvtWindowPtr);

	switch (myPartCode)
		{
	case inSysWindow:

		SystemClick(&gLastEvent, myEvtWindowPtr);

		break;

	case inDrag:

		if (myEvtWindowPtr != FrontWindow())
			{
			MAIN_SelectWindow(myEvtWindowPtr);
			break;
			};

		SetRect(&myTempRect, -32000, -32000, 32000, 32000);
		DragWindow(myEvtWindowPtr, gLastEvent.where, &myTempRect);

		if (myEvtWindowPtr == UUCD_GetUUCDWindowPtr())
			{
			Point		  myUUCDWindowPos;

			GetPort(&mySavePort);
			SetPort(myEvtWindowPtr);
			myUUCDWindowPos.h = 0;
			myUUCDWindowPos.v = 0;
			LocalToGlobal(&myUUCDWindowPos);
			SetPort(mySavePort);

			PREF_SetUUCDWindowPos(myUUCDWindowPos);
			};

		break;

	case inGrow:

		if (myEvtWindowPtr != FrontWindow())
			{
			MAIN_SelectWindow(myEvtWindowPtr);
			break;
			};

		if (myEvtWindowPtr == UUCD_GetUUCDWindowPtr())
			{
			long			  myGrowSize;
			Rect			  myGrowLimit;

			myGrowLimit.top = 100;
			myGrowLimit.left = 400;
			myGrowLimit.bottom = 32000;
			myGrowLimit.right = 32000;

			myGrowSize = GrowWindow(myEvtWindowPtr, gLastEvent.where, &myGrowLimit);
			if (myGrowSize != 0L)
				{
				short		  myWantedWidth = LoWord(myGrowSize);
				short		  myWantedHeight = HiWord(myGrowSize);
				Rect			  myWindowPortRect = myEvtWindowPtr->portRect;
				short		  myActualWidth;
				short		  myActualHeight;
				short		  myOldNbOfVisibleSegments;
				short		  myNewNbOfVisibleSegments;
				Rect			  myNewWindowBounds;
				short		  mySegmentWidth;
				short		  myVSBarMax;
				short		  myVSBarValue;
				Point		  myTempPoint;

				myActualWidth = myWindowPortRect.right - myWindowPortRect.left;
				myActualHeight = myWindowPortRect.bottom - myWindowPortRect.top;
				myOldNbOfVisibleSegments = UUCD_GetNbOfVisibleSegments();

				myNewNbOfVisibleSegments = myOldNbOfVisibleSegments;
				myNewNbOfVisibleSegments += (myWantedHeight - myActualHeight) / kUUCD_SegmentHeight;
				mySegmentWidth = myWantedWidth;
				mySegmentWidth -= kUUCDBorderSize + kUUCDCellMoveRectWidth + 1;
				mySegmentWidth -= kUUCDBorderSize + 16;
				UUCD_CalcUUCDWindowRects(myNewNbOfVisibleSegments, mySegmentWidth, &myNewWindowBounds);
				SizeWindow(myEvtWindowPtr,
					myNewWindowBounds.right - myNewWindowBounds.left,
					myNewWindowBounds.bottom - myNewWindowBounds.top, false);
				GetPort(&mySavePort);
				SetPort(myEvtWindowPtr);
				HideControl(gUUCDSegListVScrollBar);
				MoveControl(gUUCDSegListVScrollBar,
					gUUCDSegListVSBarRect.left, gUUCDSegListVSBarRect.top);
				SizeControl(gUUCDSegListVScrollBar,
					gUUCDSegListVSBarRect.right - gUUCDSegListVSBarRect.left,
					gUUCDSegListVSBarRect.bottom - gUUCDSegListVSBarRect.top);

				myVSBarMax = UUCD_GetNbOfSegments() - UUCD_GetNbOfVisibleSegments();
				if (myVSBarMax < 0) myVSBarMax = 0;
				myVSBarValue = UUCD_GetTopSegmentNum();
				if (myVSBarValue > myVSBarMax) myVSBarValue = myVSBarMax;
				SetControlMaximum(gUUCDSegListVScrollBar, myVSBarMax);
				SetControlValue(gUUCDSegListVScrollBar, myVSBarValue);
				UUCD_SetTopSegment(myVSBarValue);

				ShowControl(gUUCDSegListVScrollBar);
				InvalRect(&myEvtWindowPtr->portRect);
				SetPort(mySavePort);

				myTempPoint.h = mySegmentWidth;
				myTempPoint.v = myNewNbOfVisibleSegments;
				PREF_SetUUCDWindowSiz(myTempPoint);
				};
			};

		break;

	case inGoAway:

		if (myEvtWindowPtr != FrontWindow())
			{
			MAIN_SelectWindow(myEvtWindowPtr);
			break;
			};

		if ((gUUCDWindowPtr != NULL) && (myEvtWindowPtr == gUUCDWindowPtr))
			{
			if (TrackGoAway(gUUCDWindowPtr, gLastEvent.where))
				{
				gQuit = true;
				};
			};

		break;

	case inContent:

		if (myEvtWindowPtr != FrontWindow())
			{
			MAIN_SelectWindow(myEvtWindowPtr);
			break;
			};

		GetPort(&mySavePort);
		SetPort(myEvtWindowPtr);
		myMouseLoc = gLastEvent.where;
		GlobalToLocal(&myMouseLoc);
		SetPort(mySavePort);

		if ((myAboutWindowPtr = ABOUT_GetAboutWindowPtr()) != NULL)
		if (myEvtWindowPtr == myAboutWindowPtr)
			ABOUT_DoMouseDown_InContent(myMouseLoc);

		if ((myInfoWindowPtr = INFO_GetInfoWindowPtr()) != NULL)
		if (myEvtWindowPtr == myInfoWindowPtr)
			INFO_DoMouseDown_InContent(myMouseLoc);

		if (gUUCDWindowPtr != NULL)
		if (myEvtWindowPtr == gUUCDWindowPtr)
			{
			Boolean			  myClickDone = false;

			if (PtInRect(myMouseLoc, UUCD_GetSegListViewRect()))
				{
				short			  mySegmentNum;
				Rect				  mySegmentRect;
				Rect				  mySegmentMarkIconRect;
				Rect				  mySegmentSaveIconRect;
				Rect				  mySegmentInfoIconRect;

				if ((mySegmentNum = UUCD_FindSegment(myMouseLoc)) != -1)
				if (UUCD_GetSegmentRect(mySegmentNum, &mySegmentRect))
					{
					if (!myClickDone)
						{
						UUCD_GetSegmentMarkIconRect(&mySegmentRect, &mySegmentMarkIconRect);
						if (PtInRect(myMouseLoc, &mySegmentMarkIconRect))
							{
							switch (UUCD_GetSegmentMark(mySegmentNum))
								{
							case kUUCD_MARK_Check:
								UUCD_SetSegmentMark(mySegmentNum, kUUCD_MARK_None);
								break;
							default:
								UUCD_SetSegmentMark(mySegmentNum, kUUCD_MARK_Check);
								break;
								};
							UUCD_DrawSegment(mySegmentNum, NULL);
							while (StillDown()) {};
							myClickDone = true;
							};
						};

					if (!myClickDone)
						{
						if (UUCD_SegmentHasSaveIcon(mySegmentNum))
							{
							UUCD_GetSegmentSaveIconRect(&mySegmentRect, &mySegmentSaveIconRect);
							if (PtInRect(myMouseLoc, &mySegmentSaveIconRect))
								{
								Boolean			  myButtonDown = false;

								if (UUCD_TrackCICNButton(&mySegmentSaveIconRect,
									PREF_IsAutoOpenEnabled() ? UUCD_GetViewIcon(kIconUp) : UUCD_GetSaveIcon(kIconUp),
									PREF_IsAutoOpenEnabled() ? UUCD_GetViewIcon(kIconDown) : UUCD_GetSaveIcon(kIconDown)))
									{
									UUCD_SetCursor(UUCDCURS_Watch);
									UUCD_SaveLinkedSegments(mySegmentNum);
									UUCD_SetCursor(UUCDCURS_Arrow);
									};

								myClickDone = true;
								};
							};
						};

					if (!myClickDone)
						{
						UUCD_GetSegmentInfoIconRect(&mySegmentRect, &mySegmentInfoIconRect);
						if (PtInRect(myMouseLoc, &mySegmentInfoIconRect))
							{
							if (UUCD_TrackCICNButton(&mySegmentInfoIconRect,
								UUCD_GetInfoIcon(kIconUp),
								UUCD_GetInfoIcon(kIconDown)))
								{
								INFO_ShowInfoWindow(mySegmentNum);
								};
							myClickDone = true;
							};
						};

					if (!myClickDone)
						{
						short			  mySourceSegmentNum = mySegmentNum;
						short			  myTargetSegmentNum = mySegmentNum;
						short			  myOldTargetSegmentNum = mySegmentNum;
						Point			  myTargetMouseLoc;
						short			  mySourceV = 0;
						short			  myTargetV = 0;
						short			  myLineH = (gUUCDCellMoveRect.left + gUUCDCellMoveRect.right) / 2;

						UUCD_SetCursor(UUCDCURS_MoveCell);

						GetPort(&mySavePort);
						SetPort(gUUCDWindowPtr);

						UUCD_DrawSegment(myTargetSegmentNum,
							UUCD_GetSegmentBgPixPat(myTargetSegmentNum, kUUCD_SEGBG_Target));

						while (StillDown())
							{
							GetMouse(&myTargetMouseLoc);
							myOldTargetSegmentNum = myTargetSegmentNum;
							myTargetSegmentNum = UUCD_FindSegment(myTargetMouseLoc);

							if (myTargetSegmentNum == -1)
							if (myTargetMouseLoc.h >= (UUCD_GetSegListViewRect())->left)
							if (myTargetMouseLoc.h < (UUCD_GetSegListViewRect())->right)
								{
								if (myTargetMouseLoc.v < (UUCD_GetSegListViewRect())->top)
									{
									if (UUCD_GetTopSegmentNum() > 0)
										{
										myTargetSegmentNum = myOldTargetSegmentNum - 1;
										UUCD_SetTopSegment(myTargetSegmentNum);
										};
									}
								else if (myTargetMouseLoc.v >= (UUCD_GetSegListViewRect())->bottom)
										{
										short		  myMaxTopSegmentNum;

										myMaxTopSegmentNum = UUCD_GetNbOfSegments() - UUCD_GetNbOfVisibleSegments();
										if (myMaxTopSegmentNum < 0) myMaxTopSegmentNum = 0;

										if (UUCD_GetTopSegmentNum() < myMaxTopSegmentNum)
											{
											myTargetSegmentNum = myOldTargetSegmentNum + 1;
											UUCD_SetTopSegment(myTargetSegmentNum);
											};
										};
								};

							if (myTargetSegmentNum == myOldTargetSegmentNum) continue;

							if (myOldTargetSegmentNum == mySourceSegmentNum)
								{
								UUCD_DrawSegment(myOldTargetSegmentNum,
									UUCD_GetSegmentBgPixPat(myOldTargetSegmentNum, kUUCD_SEGBG_Source));
								}
							else UUCD_DrawSegment(myOldTargetSegmentNum, NULL);

							UUCD_DrawSegment(myTargetSegmentNum,
								UUCD_GetSegmentBgPixPat(myTargetSegmentNum, kUUCD_SEGBG_Target));

							EraseRect(&gUUCDCellMoveRect);

							mySourceV = mySourceSegmentNum - UUCD_GetTopSegmentNum();
							if (mySourceV >= 0)
								{
								mySourceV *= kUUCD_SegmentHeight;
								mySourceV += kUUCD_SegmentHeight / 2;
								}
							else mySourceV = -1;
							mySourceV += (UUCD_GetSegListViewRect())->top;
							if (mySourceV > (UUCD_GetSegListViewRect())->bottom)
								mySourceV = (UUCD_GetSegListViewRect())->bottom;

							if (myTargetSegmentNum != -1)
								{
								myTargetV = myTargetSegmentNum - UUCD_GetTopSegmentNum();
								myTargetV *= kUUCD_SegmentHeight;
								myTargetV += kUUCD_SegmentHeight / 2;
								myTargetV += (UUCD_GetSegListViewRect())->top;

								MoveTo(gUUCDCellMoveRect.right, mySourceV);
								LineTo(myLineH, mySourceV);
								LineTo(myLineH, myTargetV);
								LineTo(gUUCDCellMoveRect.right, myTargetV);
								MoveTo(gUUCDCellMoveRect.right - 2, myTargetV - 1);
								LineTo(gUUCDCellMoveRect.right - 3, myTargetV + 1);
								MoveTo(gUUCDCellMoveRect.right - 3, myTargetV - 2);
								LineTo(gUUCDCellMoveRect.right - 3, myTargetV + 2);
								};

							}; /* while StillDown */

						EraseRect(&gUUCDCellMoveRect);
						SetPort(mySavePort);

						UUCD_MoveSegment(mySourceSegmentNum, myTargetSegmentNum);

						if (myTargetSegmentNum != mySourceSegmentNum)
							{
							UUCD_CheckSegmentStates();
							UUCD_DrawSegments();
							}
						else UUCD_DrawSegment(mySourceSegmentNum, NULL);

						myClickDone = true;
						}; /* if !myClickDone */

					}; /* if UUCD_GetSegmentRect */

				UUCD_SetCursor(UUCDCURS_Arrow);
				myClickDone = true;
				};

			if (!myClickDone)
			if (PtInRect(myMouseLoc, &gUUCDSaveAllButtonRect))
				{
				if (UUCD_TrackCICNButton(&gUUCDSaveAllButtonRect,
					gUUCDSaveAllButtonUpCIconHandle, gUUCDSaveAllButtonDownCIconHandle))
					{
					UUCD_SaveAllLinkedSegments();
					};
				myClickDone = true;
				};

			if (!myClickDone)
			if (PtInRect(myMouseLoc, &gUUCDRAZButtonRect))
				{
				if (UUCD_TrackCICNButton(&gUUCDRAZButtonRect,
					gUUCDRAZButtonUpCIconHandle, gUUCDRAZButtonDownCIconHandle))
					{
					UUCD_RemoveMarkedSegments();
					};
				myClickDone = true;
				};

			if (!myClickDone)
			if (PtInRect(myMouseLoc, &gUUCDAboutButtonRect))
				{
				if (UUCD_TrackCICNButton(&gUUCDAboutButtonRect,
					gUUCDAboutButtonUpCIconHandle, gUUCDAboutButtonDownCIconHandle))
					{
					ABOUT_ShowAboutBox();
					};
				myClickDone = true;
				};

			if (!myClickDone)
			if (PtInRect(myMouseLoc, &gUUCDPrefsButtonRect))
				{
				if (UUCD_TrackCICNButton(&gUUCDPrefsButtonRect,
					gUUCDPrefsButtonUpCIconHandle, gUUCDPrefsButtonDownCIconHandle))
					{
					UUCD_EditPreferences();
					};
				myClickDone = true;
				};

			if (!myClickDone)
			if (PtInRect(myMouseLoc, &gUUCDSortButtonRect))
				{
				if (UUCD_TrackCICNButton(&gUUCDSortButtonRect,
					gUUCDSortButtonUpCIconHandle, gUUCDSortButtonDownCIconHandle))
					{
					UUCD_SortSegments();
					UUCD_CheckSegmentStates();
					UUCD_DrawSegments();
					};
				myClickDone = true;
				};

			if (!myClickDone)
			if (PtInRect(myMouseLoc, &gUUCDViewButtonRect))
				{
				PREF_SetAutoOpenEnable(!PREF_IsAutoOpenEnabled());
				GetPort(&mySavePort);
				SetPort(gUUCDWindowPtr);
				CICN_PlotIcon(&gUUCDViewButtonRect, PREF_IsAutoOpenEnabled()
					? gUUCDViewButtonDownCIconHandle : gUUCDViewButtonUpCIconHandle);
				SetPort(mySavePort);
				UUCD_DrawSegments();
				myClickDone = true;
				};

			if (!myClickDone)
			if (PtInRect(myMouseLoc, &gUUCDSoundButtonRect))
				{
				PREF_SetSoundEnable(!PREF_IsSoundEnabled());
				GetPort(&mySavePort);
				SetPort(gUUCDWindowPtr);
				CICN_PlotIcon(&gUUCDSoundButtonRect, PREF_IsSoundEnabled()
					? gUUCDSoundButtonDownCIconHandle : gUUCDSoundButtonUpCIconHandle);
				SetPort(mySavePort);
				myClickDone = true;
				};

			if (!myClickDone)
				{
				short			  myControlPart;
				ControlHandle		  myEvtControlHandle;

				myControlPart = FindControl(myMouseLoc, gUUCDWindowPtr, &myEvtControlHandle);
				if (myControlPart != 0)
				if (GetControlReference(myEvtControlHandle) == kUUCDSegListVSBarRefCon)
					{
					if (myControlPart == kControlIndicatorPart)
						{
						if (TrackControl(myEvtControlHandle, myMouseLoc, NULL) == kControlIndicatorPart)
							UUCD_SetTopSegment(GetControlValue(myEvtControlHandle));
						}
					else TrackControl(myEvtControlHandle, myMouseLoc, gUUCDSegListVSBarActionUPP);
					};
				};
			};

		break;

	case inMenuBar:

		MAIN_DoCommand(MenuSelect(gLastEvent.where));

		break;
		};

	} /* MAIN_DoMouseDown */

/*------------------------------------------------------------------------------
//
//	MAIN_DoKeyDown
//
*/

void MAIN_DoKeyDown (void)

	{
     WindowPtr			  myEvtWindowPtr = FrontWindow();

	if ((gLastEvent.modifiers & cmdKey) != 0)
		{
		char			  myCommandKey;

		myCommandKey = gLastEvent.message & charCodeMask;

		if (myCommandKey == '.')
			{
			if (gUUCD_DECODE_State == kUUCD_DECODE_STATE_Running)
				gUUCD_DECODE_State = kUUCD_DECODE_STATE_Done;
			}
		else MAIN_DoCommand(MenuKey(myCommandKey));

		return;
		};

	if (ABOUT_GetAboutWindowPtr() != NULL)
	if (myEvtWindowPtr == ABOUT_GetAboutWindowPtr())
		{
		ABOUT_DoKeyDown(&gLastEvent);
		};

	if (INFO_GetInfoWindowPtr() != NULL)
	if (myEvtWindowPtr == INFO_GetInfoWindowPtr())
		{
		INFO_DoKeyDown(&gLastEvent);
		};

	if (gUUCDWindowPtr != NULL)
	if (myEvtWindowPtr == gUUCDWindowPtr)
		{
		switch (gLastEvent.message & charCodeMask)
			{
		case 0x08: UUCD_RemoveMarkedSegments(); break;
			};
		};

	} /* MAIN_DoKeyDown */

/*------------------------------------------------------------------------------
//
//	MAIN_DoActivate
//
*/

void MAIN_DoActivate (WindowPtr theWindowPtr, Boolean theActiveFlag)

	{
#pragma unused (theWindowPtr)

	if (theActiveFlag)
		{
		UUCD_SetCursor(UUCDCURS_Arrow);
		}
	else UUCD_SetCursor(UUCDCURS_None);

	} /* MAIN_DoActivate */

/*------------------------------------------------------------------------------
//
//	MAIN_DoUpdate
//
*/

Boolean MAIN_DoUpdate (WindowPtr theWindowPtr)

	{
	Boolean			  myUpdateDone = false;
	GrafPtr			  mySavePort;
	RgnHandle			  myUpdateRgn;
	Rect				  myTempRect;

	if ((gUUCDWindowPtr != NULL) && (theWindowPtr == gUUCDWindowPtr))
		{
		Boolean		  myApplicationIsFront = gApplicationIsFront; //UTIL_IsFrontApplication();

		GetPort(&mySavePort);
		SetPort(theWindowPtr);
		BeginUpdate(theWindowPtr);
		myUpdateRgn = theWindowPtr->visRgn;
		EraseRgn(myUpdateRgn);

		if (myApplicationIsFront)
		if (gUUCDWindowBgRgnHandle != NULL)
		if (gUUCDWindowBgPixPatHandle != NULL)
			{
			FillCRgn(gUUCDWindowBgRgnHandle, gUUCDWindowBgPixPatHandle);
			};

		if (myApplicationIsFront)
			{
			myTempRect = gUUCDCellMoveRect;
			InsetRect(&myTempRect, -1, -1);
			FrameRect(&myTempRect);
			};

		UUCD_DrawSegments();
		myTempRect = *UUCD_GetSegListViewRect();
		InsetRect(&myTempRect, -1, -1);
		FrameRect(&myTempRect);

		if (myApplicationIsFront)
		if (RectInRgn(&gUUCDProgressRect, myUpdateRgn))
			UUCD_DrawProgress(0L, 0L);

		if (myApplicationIsFront)
		if (RectInRgn(&gUUCDMemoryRect, myUpdateRgn))
			UUCD_DrawMemory();

		if (myApplicationIsFront)
			{
			if (RectInRgn(&gUUCDSaveAllButtonRect, myUpdateRgn))
				CICN_PlotIcon(&gUUCDSaveAllButtonRect, gUUCDSaveAllButtonUpCIconHandle);

			if (RectInRgn(&gUUCDRAZButtonRect, myUpdateRgn))
				CICN_PlotIcon(&gUUCDRAZButtonRect, gUUCDRAZButtonUpCIconHandle);

			if (RectInRgn(&gUUCDAboutButtonRect, myUpdateRgn))
				CICN_PlotIcon(&gUUCDAboutButtonRect, gUUCDAboutButtonUpCIconHandle);

			if (RectInRgn(&gUUCDPrefsButtonRect, myUpdateRgn))
				CICN_PlotIcon(&gUUCDPrefsButtonRect, gUUCDPrefsButtonUpCIconHandle);

			if (RectInRgn(&gUUCDSortButtonRect, myUpdateRgn))
				CICN_PlotIcon(&gUUCDSortButtonRect, gUUCDSortButtonUpCIconHandle);

			if (RectInRgn(&gUUCDViewButtonRect, myUpdateRgn))
				CICN_PlotIcon(&gUUCDViewButtonRect, PREF_IsAutoOpenEnabled()
					? gUUCDViewButtonDownCIconHandle : gUUCDViewButtonUpCIconHandle);

			if (RectInRgn(&gUUCDSoundButtonRect, myUpdateRgn))
				CICN_PlotIcon(&gUUCDSoundButtonRect, PREF_IsSoundEnabled()
					? gUUCDSoundButtonDownCIconHandle : gUUCDSoundButtonUpCIconHandle);
			};

		if (myApplicationIsFront)
			DrawControls(theWindowPtr);

		if (myApplicationIsFront)
		if (RectInRgn(&gUUCDGrowIconRect, myUpdateRgn))
			CICN_PlotIcon(&gUUCDGrowIconRect, gUUCDGrowCIconHandle);

		EndUpdate(theWindowPtr);
		SetPort(mySavePort);

		myUpdateDone = true;
		};

	if (ABOUT_GetAboutWindowPtr() != NULL)
	if (theWindowPtr == ABOUT_GetAboutWindowPtr())
		{
		GetPort(&mySavePort);
		SetPort(theWindowPtr);
		BeginUpdate(theWindowPtr);
		myUpdateRgn = theWindowPtr->visRgn;
		EraseRgn(myUpdateRgn);

		ABOUT_DrawAboutBox();

		EndUpdate(theWindowPtr);
		SetPort(mySavePort);

		myUpdateDone = true;
		};

	if (INFO_GetInfoWindowPtr() != NULL)
	if (theWindowPtr == INFO_GetInfoWindowPtr())
		{
		GetPort(&mySavePort);
		SetPort(theWindowPtr);
		BeginUpdate(theWindowPtr);
		myUpdateRgn = theWindowPtr->visRgn;
		EraseRgn(myUpdateRgn);

		INFO_DrawInfoWindow();

		EndUpdate(theWindowPtr);
		SetPort(mySavePort);

		myUpdateDone = true;
		};

	return myUpdateDone;

	} /* MAIN_DoUpdate */

/*------------------------------------------------------------------------------
//
//	MAIN_DoOSEvent
//
*/

void MAIN_DoOSEvent (void)

	{

	switch ((gLastEvent.message >> 24) & 0xFF)
		{
	case suspendResumeMessage:
		MAIN_DoSuspendResumeEvent();
		break;
		};

	} // MAIN_DoOSEvent

/*------------------------------------------------------------------------------
//
//	MAIN_DoSuspendResumeEvent
//
*/

void MAIN_DoSuspendResumeEvent (void)

	{
	GrafPtr			  mySavePort;
	WindowPtr			  myFrontWindowPtr;

	myFrontWindowPtr = FrontWindow();

	if (gLastEvent.message & resumeFlag)
		{
		// it's a resume event
		gApplicationIsFront = true;
		MAIN_DoActivate(myFrontWindowPtr, true);
		}
	else
		{
		// it's a suspend event
		gApplicationIsFront = false;
		MAIN_DoActivate(myFrontWindowPtr, false);
		};

	GetPort(&mySavePort);
	SetPort(gUUCDWindowPtr);
	InvalRect(&gUUCDWindowPtr->portRect);
	SetPort(mySavePort);

/*
	if (gUUCDWindowBgRgnHandle != NULL)
		{
		GetPort(&mySavePort);
		SetPort(gUUCDWindowPtr);
		InvalRgn(gUUCDWindowBgRgnHandle);
		SetPort(mySavePort);
		};
*/

	} // MAIN_DoSuspendResumeEvent

/*------------------------------------------------------------------------------
//
//	MAIN_DoHighLevelEvent
//
*/

void MAIN_DoHighLevelEvent (void)

	{
	OSErr			  myOSErr;

	myOSErr = AEProcessAppleEvent(&gLastEvent);

	} /* MAIN_DoHighLevelEvent */

/*------------------------------------------------------------------------------
//
//	MAIN_DoCommand
//
*/

void MAIN_DoCommand (long theMenuResult)

	{
	short				  myMenuID;
	short				  myMenuItem;
	WindowPtr				  myFrontWindow;
	Str255				  myDAName;

	myMenuID = (HiWord(theMenuResult));
	myMenuItem = (LoWord(theMenuResult));
	myFrontWindow = FrontWindow();

	switch (myMenuID)
		{
	case MENUID_Apple:
		if (myMenuItem == MENU_APPLE_About)
			{
			ABOUT_ShowAboutBox();
			}
		else
			{
			GetMenuItemText(gApplicationMenus[MENU_Apple], myMenuItem, myDAName);
			OpenDeskAcc(myDAName);
			};
		break;
	case MENUID_File:
		switch (myMenuItem)
			{
		case MENU_FILE_UUDecode:
			UUCD_UUDecode(NULL);
			break;
		case MENU_FILE_UUEncode:
			UUCD_UUEncode(NULL);
			break;
		case MENU_FILE_Preferences:
			UUCD_EditPreferences();
			break;
		case MENU_FILE_Register:
			SysBeep(1);
			break;
		case MENU_FILE_Quit:
			gQuit = true;
			break;
			};
		break;
	case MENUID_Mark:
		switch (myMenuItem)
			{
		case MENU_MARK_ClearMarked:
			UUCD_RemoveMarkedSegments();
			break;
		case MENU_MARK_SaveUnmarked:
			UUCD_SaveAllLinkedSegments();
			break;
		case MENU_MARK_MarkAll:
			UUCD_MarkAllSegments(kUUCD_MARK_Check);
			UUCD_DrawSegments();
			break;
		case MENU_MARK_UnmarkAll:
			UUCD_MarkAllSegments(kUUCD_MARK_None);
			UUCD_DrawSegments();
			break;
			};
		break;
		};

	HiliteMenu(0);

	} /* MAIN_DoCommand */

/*------------------------------------------------------------------------------
//
//	InstallAEHandlers
//
*/

void InstallAEHandlers (void)

	{
	long			  myGestaltResult;

	if (Gestalt(gestaltAppleEventsAttr, &myGestaltResult) != noErr) return;

	AEInstallEventHandler(kCoreEventClass, kAEOpenApplication, NewAEEventHandlerProc(AEHandler_OAPP), 0, false);
	AEInstallEventHandler(kCoreEventClass, kAEOpenDocuments,   NewAEEventHandlerProc(AEHandler_ODOC), 0, false);
	AEInstallEventHandler(kCoreEventClass, kAEPrintDocuments,  NewAEEventHandlerProc(AEHandler_PDOC), 0, false);
	AEInstallEventHandler(kCoreEventClass, kAEQuitApplication, NewAEEventHandlerProc(AEHandler_QUIT), 0, false);

	} /* InstallAEHandlers */

/*------------------------------------------------------------------------------
//
//	AEGotRequiredParams
//
*/

OSErr AEGotRequiredParams (AppleEvent theAppleEvent)

	{
	OSErr			  myOSErr;
	DescType			  myReturnedType;
	Size				  myActualSize;

	myOSErr = AEGetAttributePtr(&theAppleEvent, keyMissedKeywordAttr,
		typeWildCard, &myReturnedType, NULL, 0, &myActualSize);

	/* noErr means that we found some unused parameters.              */
	/* errAEDescNotFound means that there are no more parameters.     */

	if (myOSErr == errAEDescNotFound) return noErr;
	if (myOSErr == noErr) return errAEEventNotHandled;

	return myOSErr;

	} /* AEGotRequiredParams */

/*------------------------------------------------------------------------------
//
//	SendAEOpenDoc
*/

OSErr SendAEOpenDoc (FSSpec *theFSSpecPtr)

	{
	OSErr				  myOSErr = noErr;
	AppleEvent			  myAppleEvent;
	AppleEvent			  myDefReply;
	AEDescList			  myDocList;
	ProcessSerialNumber		  myProcessSerialNumber;
	AEAddressDesc			  mySelfAddrDesc;

	mySelfAddrDesc.descriptorType = typeNull;
	mySelfAddrDesc.dataHandle = NULL;

	myAppleEvent.descriptorType = typeNull;
	myAppleEvent.dataHandle = NULL;

	myDefReply.descriptorType = typeNull;
	myDefReply.dataHandle = NULL;

	myDocList.descriptorType = typeNull;
	myDocList.dataHandle = NULL;

	/*---- Create an address descriptor for the current process ----*/

	myProcessSerialNumber.highLongOfPSN = 0;
	myProcessSerialNumber.lowLongOfPSN = kCurrentProcess;
	myOSErr = AECreateDesc(typeProcessSerialNumber, (Ptr)&myProcessSerialNumber,
		sizeof(ProcessSerialNumber), &mySelfAddrDesc);
	if (myOSErr != noErr) goto SendAEOpenDoc_EXIT;

	/*---- Create empty list ----*/

	myOSErr = AECreateList(NULL, 0, false, &myDocList);
	if (myOSErr != noErr) goto SendAEOpenDoc_EXIT;

	/*---- Add file spec to list ----*/

	myOSErr = AEPutPtr(&myDocList, 1,
		typeFSS, (Ptr)theFSSpecPtr, sizeof(FSSpec));
	if (myOSErr != noErr) goto SendAEOpenDoc_EXIT;

	/*---- Create event ----*/

	myOSErr = AECreateAppleEvent(kCoreEventClass,
		kAEOpenDocuments, &mySelfAddrDesc, kAutoGenerateReturnID,
		kAnyTransactionID, &myAppleEvent);
	if (myOSErr != noErr) goto SendAEOpenDoc_EXIT;

	/*---- Add list to event ----*/

	myOSErr = AEPutParamDesc(&myAppleEvent, keyDirectObject, &myDocList);
	if (myOSErr != noErr) goto SendAEOpenDoc_EXIT;

	/*---- Send event ----*/

	myOSErr = AESend(&myAppleEvent, &myDefReply,
		kAENoReply + kAEAlwaysInteract, kAENormalPriority,
		kAEDefaultTimeout, NULL, NULL);

	return myOSErr;

SendAEOpenDoc_EXIT:

	AEDisposeDesc(&mySelfAddrDesc);
	AEDisposeDesc(&myAppleEvent);
	AEDisposeDesc(&myDefReply);
	AEDisposeDesc(&myDocList);

	return myOSErr;

	} /* SendAEOpenDoc */

/*------------------------------------------------------------------------------
//
//	AEHandler_OAPP
//
*/

pascal OSErr AEHandler_OAPP (AEDescList *theAppleEvent, AEDescList *theReply, long theHandlerRefCon)

	{
#pragma unused (theAppleEvent, theReply, theHandlerRefCon)

	return noErr;

	} /* AEHandler_OAPP */

/*------------------------------------------------------------------------------
//
//	AEHandler_ODOC
//
*/

pascal OSErr AEHandler_ODOC (AEDescList *theAppleEvent, AEDescList *theReply, long theHandlerRefCon)

	{
#pragma unused (theReply, theHandlerRefCon)

	OSErr			  myOSErr = noErr;
	AEDesc			  myFileListDesc = {typeNull, NULL};
	long				  myFileListCount;
	long				  myFileNum;
	AEKeyword			  myActualKeyword;
	DescType			  myActualType;
	long				  myActualSize;
	FSSpec			  myFileFSSpec;
	CInfoPBRec		  myCInfoPBRec;
	OSType			  myDocType;

	myOSErr = AEGetKeyDesc(theAppleEvent, keyDirectObject, typeAEList, &myFileListDesc);
	if (myOSErr != noErr) goto AEHandler_ODOC_EXIT;
		
	myOSErr = AECountItems(&myFileListDesc, &myFileListCount);
	if (myOSErr != noErr) goto AEHandler_ODOC_EXIT;
				
	for (myFileNum = 1; myFileNum <= myFileListCount; myFileNum++)
		{
		myOSErr = AEGetNthPtr(&myFileListDesc, myFileNum, typeFSS, &myActualKeyword,
			&myActualType, (Ptr)&myFileFSSpec, sizeof(FSSpec), &myActualSize);
		if (myOSErr != noErr) goto AEHandler_ODOC_EXIT;
		
		((HFileInfo *)&myCInfoPBRec)->ioCompletion = NULL;
		((HFileInfo *)&myCInfoPBRec)->ioVRefNum = myFileFSSpec.vRefNum;
		((HFileInfo *)&myCInfoPBRec)->ioFVersNum = 0;
		((HFileInfo *)&myCInfoPBRec)->ioFDirIndex = 0;
		((HFileInfo *)&myCInfoPBRec)->ioNamePtr = myFileFSSpec.name;
		((HFileInfo *)&myCInfoPBRec)->ioDirID = myFileFSSpec.parID;

		if (PBGetCatInfoSync(&myCInfoPBRec) == noErr)
			{
			if (!(((HFileInfo *)&myCInfoPBRec)->ioFlAttrib & 16))
				{
				/* UUCD_BringApplicationToFront(); */

				myDocType = ((HFileInfo *)&myCInfoPBRec)->ioFlFndrInfo.fdType;

				if (myDocType == 'TEXT')
					{
					DOCLIST_Push(&myFileFSSpec);
					}
				else if (myDocType != kUUCDPrefsFileType) UUCD_UUEncode(&myFileFSSpec);
				};
			};
		};

AEHandler_ODOC_EXIT:

	AEDisposeDesc(&myFileListDesc);
	return myOSErr;

	} /* AEHandler_ODOC */

/*------------------------------------------------------------------------------
//
//	AEHandler_PDOC
//
*/

pascal OSErr AEHandler_PDOC (AEDescList *theAppleEvent, AEDescList *theReply, long theHandlerRefCon)

	{
#pragma unused (theAppleEvent, theReply, theHandlerRefCon)

	return noErr;

	} /* AEHandler_PDOC */

/*------------------------------------------------------------------------------
//
//	AEHandler_QUIT
//
*/

pascal OSErr AEHandler_QUIT (AEDescList *theAppleEvent, AEDescList *theReply, long theHandlerRefCon)

	{
#pragma unused (theAppleEvent, theReply, theHandlerRefCon)

	gQuit = true;

	return noErr;

	} /* AEHandler_QUIT */

/*------------------------------------------------------------------------------
//
//	AESendFinderOpenDoc
//
*/

OSErr AESendFinderOpenDoc (FSSpecPtr theFSSpecPtr)

	{
	OSErr			  myOSErr;
	ProcessSerialNumber	  myFinderPSN;
	AEDesc			  myFinderAddressDesc;
	ProcessInfoRec		  myProcessInfoRec;
	FSSpec			  myProcessFSSpec;
	Str31			  myProcessName;
	AppleEvent		  myAppleEvent;
	FSSpec			  myParentFolderFSSpec;
	AliasHandle		  myParentFolderAlias;
	AliasHandle		  myFileAlias;
	AEDesc			  myFileList;
	AEDesc			  myParentFolderDesc;
	AEDesc			  myFileDesc;

	/*---- Get the PSN of the Finder ----*/

	myFinderPSN.lowLongOfPSN = kNoProcess;
	myFinderPSN.highLongOfPSN = kNoProcess;

	myProcessInfoRec.processInfoLength = sizeof(ProcessInfoRec);
	myProcessInfoRec.processName = myProcessName;
	myProcessInfoRec.processAppSpec = &myProcessFSSpec;

	for (;;)
		{
		myOSErr = GetNextProcess(&myFinderPSN);
		if (myOSErr != noErr) break;
		GetProcessInformation(&myFinderPSN, &myProcessInfoRec);
		if (myProcessInfoRec.processSignature != 'MACS') continue;
		if (myProcessInfoRec.processType != 'FNDR') continue;
		break;
		};

	if (myOSErr != noErr) return procNotFound;

	/*---- Create the target address for the AE ----*/

	myOSErr = AECreateDesc(typeProcessSerialNumber,
		(Ptr)&myFinderPSN, sizeof(ProcessSerialNumber), &myFinderAddressDesc);
	if (myOSErr != noErr) return myOSErr;

	/*---- Create an empty AE ----*/

	myOSErr = AECreateAppleEvent('FNDR', 'sope',
		&myFinderAddressDesc, kAutoGenerateReturnID, kAnyTransactionID,
		&myAppleEvent);
	if (myOSErr != noErr) return myOSErr;

	/*---- Make an FSSpec and alias for the parentFolder ----*/
	/*----           and an alias for the file           ----*/

	FSMakeFSSpec(theFSSpecPtr->vRefNum, theFSSpecPtr->parID,
		NULL, &myParentFolderFSSpec);
	NewAlias(NULL, &myParentFolderFSSpec, &myParentFolderAlias);
	NewAlias(NULL, theFSSpecPtr, &myFileAlias);

	/*---- Create the file list ----*/

	myOSErr = AECreateList(NULL, 0, false, &myFileList);
	if (myOSErr != noErr) return myOSErr;

	/*---- folder descriptor ----*/

	HLock((Handle)myParentFolderAlias);
	AECreateDesc(typeAlias, (Ptr)*myParentFolderAlias,
		GetHandleSize((Handle)myParentFolderAlias),
		&myParentFolderDesc);
	HUnlock((Handle)myParentFolderAlias);
	DisposeHandle((Handle)myParentFolderAlias);

	myOSErr = AEPutParamDesc(&myAppleEvent, keyDirectObject, &myParentFolderDesc);
	if (myOSErr != noErr) return myOSErr;

	AEDisposeDesc(&myParentFolderDesc);

	/*---- file descriptor ----*/

	HLock((Handle)myFileAlias);
	AECreateDesc(typeAlias, (Ptr)*myFileAlias,
		GetHandleSize((Handle)myFileAlias),
		&myFileDesc);
	HUnlock((Handle)myFileAlias);
	DisposeHandle((Handle)myFileAlias);

	myOSErr = AEPutDesc(&myFileList, 0, &myFileDesc);
	if (myOSErr != noErr) return myOSErr;

	AEDisposeDesc(&myFileDesc);

	/*---- Collect and send ----*/

	myOSErr = AEPutParamDesc(&myAppleEvent, 'fsel', &myFileList);
	if (myOSErr != noErr) return myOSErr;

	AEDisposeDesc(&myFileList);

	myOSErr = AESend(&myAppleEvent, NULL,
		kAENoReply + kAEAlwaysInteract + kAECanSwitchLayer,
		kAENormalPriority, kAEDefaultTimeout, NULL, NULL);

	AEDisposeDesc(&myAppleEvent);

	return myOSErr;

	} /* AESendFinderOpenDoc */

/*------------------------------------------------------------------------------
//
//	DRAG_CanAcceptItems
//
*/

Boolean DRAG_CanAcceptItems (DragReference theDragReference)

	{
	OSErr			  myOSErr;
	Boolean			  myDragItemsAreAcceptable = false;
	ushort			  myNbOfDragItems;
	ushort			  myDragItemIndex;
	ItemReference		  myItemReference;
	Size				  myFlavorDataSize;
	HFSFlavor			  myHFSFlavor;

	myOSErr = CountDragItems(theDragReference, &myNbOfDragItems);
	if (myOSErr != noErr) return false;

	for (myDragItemIndex = 1;
		myDragItemIndex <= myNbOfDragItems;
		myDragItemIndex++)
		{
		GetDragItemReferenceNumber(theDragReference,
			myDragItemIndex, &myItemReference);

		myFlavorDataSize = sizeof(HFSFlavor);
		myOSErr = GetFlavorData(theDragReference,
			myItemReference, flavorTypeHFS,
			&myHFSFlavor, &myFlavorDataSize, 0);

		if ((myOSErr == noErr) && (myHFSFlavor.fileType == 'TEXT'))
			myDragItemsAreAcceptable = true;
		};

	return myDragItemsAreAcceptable;

	} /* DRAG_CanAcceptItems */

/*------------------------------------------------------------------------------
//
//	DRAG_MouseIsInContentRgn
//
*/

Boolean DRAG_MouseIsInContentRgn (DragReference theDragReference, WindowPtr theWindowPtr)

	{
	Point			  myMouseLoc;
	
	GetDragMouse(theDragReference, &myMouseLoc, NULL);

	return PtInRgn(myMouseLoc, ((WindowPeek)theWindowPtr)->contRgn);

	} /* DRAG_MouseIsInContentRgn */

/*------------------------------------------------------------------------------
//
//	DRAG_IsNotInSourceWindow
//
*/

Boolean DRAG_IsNotInSourceWindow (DragReference theDragReference)

	{
	DragAttributes			  myDragAttributes;

	GetDragAttributes(theDragReference, &myDragAttributes);

	return ((myDragAttributes & kDragInsideSenderWindow) == 0);

	} /* DRAG_IsNotInSourceWindow */

/*------------------------------------------------------------------------------
//
//	DRAG_HANDLER_Track
//
*/

Boolean			  gDrag_UUCDWindowCanAcceptDrag = false;
Boolean			  gDrag_UUCDListViewIsHilited = false;

pascal OSErr DRAG_HANDLER_Track (DragTrackingMessage theMessage,
	WindowPtr theWindowPtr, void *theHandlerRefCon, DragReference theDragReference)

	{
#pragma unused (theHandlerRefCon)

	OSErr				  myOSErr = noErr;
	Boolean				  myMouseInContent;

	switch (theMessage)
		{
	case kDragTrackingEnterHandler:

		gDrag_UUCDWindowCanAcceptDrag = DRAG_CanAcceptItems(theDragReference);
		gDrag_UUCDListViewIsHilited = false;

		if (!gDrag_UUCDWindowCanAcceptDrag) myOSErr = dragNotAcceptedErr;

		break;

	case kDragTrackingEnterWindow:
	case kDragTrackingInWindow:
	case kDragTrackingLeaveWindow:

		if (!gDrag_UUCDWindowCanAcceptDrag) break;
		if (!DRAG_IsNotInSourceWindow(theDragReference)) break;

		if (theMessage == kDragTrackingLeaveWindow)
			{
			myMouseInContent = false;
			}
		else myMouseInContent = DRAG_MouseIsInContentRgn(theDragReference, theWindowPtr);
			
		if (myMouseInContent && !gDrag_UUCDListViewIsHilited)
			{
			ClipRect(&theWindowPtr->portRect);

			if (gDragOutlineRgnHandle != NULL)
				{
				if (gDragOutlinePixPatHandle != NULL)
					{
					FillCRgn(gDragOutlineRgnHandle, gDragOutlinePixPatHandle);
					}
				else InvertRgn(gDragOutlineRgnHandle);
				};

			gDrag_UUCDListViewIsHilited = true;
			}
		else if (!myMouseInContent && gDrag_UUCDListViewIsHilited)
				{
				ClipRect(&theWindowPtr->portRect);

				if (gDragOutlineRgnHandle != NULL)
					{
					if (gDragOutlinePixPatHandle != NULL)
						{
						FillCRgn(gDragOutlineRgnHandle, gUUCDWindowBgPixPatHandle);
						}
					else InvertRgn(gDragOutlineRgnHandle);
					};

				gDrag_UUCDListViewIsHilited = true;
				};

		break;

	case kDragTrackingLeaveHandler:
		break;

	default:
		myOSErr = paramErr;
		break;
		};

	return myOSErr;

	} /* DRAG_HANDLER_Track */

/*------------------------------------------------------------------------------
//
//	DRAG_HANDLER_Receive
//
*/

pascal OSErr DRAG_HANDLER_Receive (WindowPtr theWindowPtr,
				void *theHandlerRefCon, DragReference theDragReference)

	{
#pragma unused (theHandlerRefCon)

	OSErr			  myOSErr;
	ushort			  myNbOfDragItems;
	ushort			  myDragItemIndex;
	ItemReference		  myItemReference;
	Size				  myFlavorDataSize;
	HFSFlavor			  myHFSFlavor;

	if (!DRAG_CanAcceptItems(theDragReference)) return dragNotAcceptedErr;
	if (!DRAG_MouseIsInContentRgn(theDragReference, theWindowPtr)) return dragNotAcceptedErr;
	if (!DRAG_IsNotInSourceWindow(theDragReference)) return dragNotAcceptedErr;

	myOSErr = CountDragItems(theDragReference, &myNbOfDragItems);
	if (myOSErr != noErr) return myOSErr;

	for (myDragItemIndex = 1;
		myDragItemIndex <= myNbOfDragItems;
		myDragItemIndex++)
		{
		if (GetDragItemReferenceNumber(theDragReference,
			myDragItemIndex, &myItemReference) != noErr) continue;

		myFlavorDataSize = sizeof(HFSFlavor);
		if (GetFlavorData(theDragReference, myItemReference, flavorTypeHFS,
			&myHFSFlavor, &myFlavorDataSize, 0L) != noErr) continue;

		if (myHFSFlavor.fileType == 'TEXT')
			{
			SendAEOpenDoc(&myHFSFlavor.fileSpec);
			};
		};

	return noErr;

	} /* DRAG_HANDLER_Receive */

/*------------------------------------------------------------------------------
//
//	DOCLIST_Push
//
*/

void DOCLIST_Push (FSSpecPtr theFSSpecPtr)

	{
	DOCLIST_ItemHdl	  myNewItemHdl;
	DOCLIST_ItemHdl	  myLastItemHdl;

	myNewItemHdl = (DOCLIST_ItemHdl)NewHandle((long)sizeof(DOCLIST_Item));
	if (myNewItemHdl == NULL) return;

	HLock((Handle)myNewItemHdl);
	(**myNewItemHdl).DOCLIST_ITEM_Next = NULL;
	(**myNewItemHdl).DOCLIST_ITEM_FSSpec = *theFSSpecPtr;
	HUnlock((Handle)myNewItemHdl);

	if (gDocList == NULL)
		{
		gDocList = myNewItemHdl;
		return;
		};

	myLastItemHdl = gDocList;
	while ((**myLastItemHdl).DOCLIST_ITEM_Next != NULL)
		myLastItemHdl = (DOCLIST_ItemHdl)(**myLastItemHdl).DOCLIST_ITEM_Next;

	HLock((Handle)myLastItemHdl);
	(**myLastItemHdl).DOCLIST_ITEM_Next = (Handle)myNewItemHdl;
	HUnlock((Handle)myLastItemHdl);

	} /* DOCLIST_Push */

Boolean DOCLIST_Pull (FSSpecPtr theFSSpecPtr)

	{
	DOCLIST_ItemHdl	  myNewDocList;

	if (gDocList == NULL) return false;

	HLock((Handle)gDocList);
	myNewDocList = (DOCLIST_ItemHdl)(**gDocList).DOCLIST_ITEM_Next;
	BlockMove(&(**gDocList).DOCLIST_ITEM_FSSpec, theFSSpecPtr, (long)sizeof(FSSpec));
	HUnlock((Handle)gDocList);

	DisposeHandle((Handle)gDocList);
	gDocList = myNewDocList;

	return true;

	} /* DOCLIST_Pull */

/*------------------------------------------------------------------------------
//
//	UUCD_CalcUUCDWindowRects
//
*/

void UUCD_CalcUUCDWindowRects (short theNbOfVisibleSegments,
		short theSegmentWidth, RectPtr theUUCDWindowBoundsPtr)

	{
	GrafPtr			  mySavePort;
	Point			  myWindowLoc;
	Rect				  myWindowBounds;
	Rect				  myWindowPortRect;
	RgnHandle			  myTempRgn;

	UUCD_SetNbOfVisibleSegments(theNbOfVisibleSegments);
	UUCD_SetSegmentWidth(theSegmentWidth);

	gUUCDCellMoveRect = *UUCD_GetSegListViewRect();
	gUUCDCellMoveRect.left -= kUUCDCellMoveRectWidth + 1;
	gUUCDCellMoveRect.right = gUUCDCellMoveRect.left + kUUCDCellMoveRectWidth;

	if (gUUCDWindowPtr != NULL)
		{
		GetPort(&mySavePort);
		SetPort(gUUCDWindowPtr);
		SetPt(&myWindowLoc, 0, 0);
		LocalToGlobal(&myWindowLoc);
		SetPort(mySavePort);
		}
	else PREF_GetUUCDWindowPos(&myWindowLoc);

	myWindowPortRect.top = 0;
	myWindowPortRect.left = 0;
	myWindowPortRect.right = kUUCDBorderSize + kUUCDCellMoveRectWidth + 1
		+ theSegmentWidth + 16 + kUUCDBorderSize;
	myWindowPortRect.bottom = (UUCD_GetSegListViewRect())->bottom
		+ kUUCDSpaceSize + kUUCDButtonHeight + kUUCDSpaceSize;

	myWindowBounds.top = myWindowLoc.v;
	myWindowBounds.left = myWindowLoc.h;
	myWindowBounds.right = myWindowBounds.left + myWindowPortRect.right;
	myWindowBounds.bottom = myWindowBounds.top + myWindowPortRect.bottom;
	if (theUUCDWindowBoundsPtr != NULL)
		*theUUCDWindowBoundsPtr = myWindowBounds;

	gUUCDGrowIconRect = myWindowPortRect;
	gUUCDGrowIconRect.left = gUUCDGrowIconRect.right - kUUCDWindowGrowIconSize;
	gUUCDGrowIconRect.top = gUUCDGrowIconRect.bottom - kUUCDWindowGrowIconSize;
	OffsetRect(&gUUCDGrowIconRect, 1, 1);

	gUUCDMemoryRect = myWindowPortRect;
	gUUCDMemoryRect.bottom = gUUCDMemoryRect.top + kUUCDMemoryHeight;

	gUUCDProgressRect = myWindowPortRect;
	InsetRect(&gUUCDProgressRect, -1, -1);
	gUUCDProgressRect.top = gUUCDMemoryRect.bottom;
	gUUCDProgressRect.bottom = gUUCDProgressRect.top + kUUCDProgressHeight;

	gUUCDRAZButtonRect.bottom = myWindowPortRect.bottom - kUUCDSpaceSize;
	gUUCDRAZButtonRect.left = myWindowPortRect.left + kUUCDBorderSize;
	gUUCDRAZButtonRect.top = gUUCDRAZButtonRect.bottom - kUUCDButtonHeight;
	gUUCDRAZButtonRect.right = gUUCDRAZButtonRect.left + kUUCDRAZButtonWidth;

	gUUCDSaveAllButtonRect.bottom = myWindowPortRect.bottom - kUUCDSpaceSize;
	gUUCDSaveAllButtonRect.left = gUUCDRAZButtonRect.right + kUUCDSpaceSize;
	gUUCDSaveAllButtonRect.top = gUUCDSaveAllButtonRect.bottom - kUUCDButtonHeight;
	gUUCDSaveAllButtonRect.right = gUUCDSaveAllButtonRect.left + kUUCDSaveAllButtonWidth;

	gUUCDSortButtonRect.bottom = myWindowPortRect.bottom - kUUCDSpaceSize;
	gUUCDSortButtonRect.left = gUUCDSaveAllButtonRect.right + kUUCDSpaceSize;
	gUUCDSortButtonRect.top = gUUCDSortButtonRect.bottom - kUUCDButtonHeight;
	gUUCDSortButtonRect.right = gUUCDSortButtonRect.left + kUUCDSortButtonWidth;

	gUUCDAboutButtonRect.bottom = myWindowPortRect.bottom - kUUCDSpaceSize;
	gUUCDAboutButtonRect.right = myWindowPortRect.right - kUUCDWindowGrowIconSize - kUUCDBorderSize;
	gUUCDAboutButtonRect.top = gUUCDAboutButtonRect.bottom - kUUCDButtonHeight;
	gUUCDAboutButtonRect.left = gUUCDAboutButtonRect.right - kUUCDAboutButtonWidth;

	gUUCDPrefsButtonRect.bottom = myWindowPortRect.bottom - kUUCDSpaceSize;
	gUUCDPrefsButtonRect.right = gUUCDAboutButtonRect.left - kUUCDSpaceSize;
	gUUCDPrefsButtonRect.top = gUUCDPrefsButtonRect.bottom - kUUCDButtonHeight;
	gUUCDPrefsButtonRect.left = gUUCDPrefsButtonRect.right - kUUCDPrefsButtonWidth;

	gUUCDSoundButtonRect.bottom = myWindowPortRect.bottom - kUUCDSpaceSize;
	gUUCDSoundButtonRect.right = gUUCDPrefsButtonRect.left - kUUCDSpaceSize;
	gUUCDSoundButtonRect.top = gUUCDSoundButtonRect.bottom - kUUCDButtonHeight;
	gUUCDSoundButtonRect.left = gUUCDSoundButtonRect.right - kUUCDSoundButtonWidth;

	gUUCDViewButtonRect.bottom = myWindowPortRect.bottom - kUUCDSpaceSize;
	gUUCDViewButtonRect.right = gUUCDSoundButtonRect.left - kUUCDSpaceSize;
	gUUCDViewButtonRect.top = gUUCDViewButtonRect.bottom - kUUCDButtonHeight;
	gUUCDViewButtonRect.left = gUUCDViewButtonRect.right - kUUCDViewButtonWidth;

	gUUCDSegListVSBarRect = *UUCD_GetSegListViewRect();
	gUUCDSegListVSBarRect.left = gUUCDSegListVSBarRect.right;
	gUUCDSegListVSBarRect.right += 16;
	gUUCDSegListVSBarRect.top--;
	gUUCDSegListVSBarRect.bottom++;

	if (gUUCDWindowBgRgnHandle == NULL)
		gUUCDWindowBgRgnHandle = NewRgn();

	if (gUUCDWindowBgRgnHandle != NULL)
		{
		RectRgn(gUUCDWindowBgRgnHandle, &myWindowPortRect);

		myTempRgn = NewRgn();

		RectRgn(myTempRgn, UUCD_GetSegListViewRect());
		DiffRgn(gUUCDWindowBgRgnHandle, myTempRgn, gUUCDWindowBgRgnHandle);

		RectRgn(myTempRgn, &gUUCDProgressRect);
		DiffRgn(gUUCDWindowBgRgnHandle, myTempRgn, gUUCDWindowBgRgnHandle);

		RectRgn(myTempRgn, &gUUCDMemoryRect);
		DiffRgn(gUUCDWindowBgRgnHandle, myTempRgn, gUUCDWindowBgRgnHandle);

		RectRgn(myTempRgn, &gUUCDCellMoveRect);
		DiffRgn(gUUCDWindowBgRgnHandle, myTempRgn, gUUCDWindowBgRgnHandle);

		DisposeRgn(myTempRgn);
		};

	} /* UUCD_CalcUUCDWindowRects */

/*------------------------------------------------------------------------------
//
//	UUCD_TrackCICNButton
//
*/

Boolean UUCD_TrackCICNButton (Rect *theRectPtr,
	CICNHandle theButtonUpCICNHandle, CICNHandle theButtonDownCICNHandle)

	{
	GrafPtr			  mySavePort;
	Boolean			  myButtonDown = false;
	Point			  myMouseLoc;

	GetPort(&mySavePort);
	SetPort(gUUCDWindowPtr);

	while (StillDown())
		{
		GetMouse(&myMouseLoc);

		if (PtInRect(myMouseLoc, theRectPtr))
			{
			if (!myButtonDown)
				{
				CICN_PlotIcon(theRectPtr, theButtonDownCICNHandle);
				myButtonDown = true;
				};
			}
		else
			{
			if (myButtonDown)
				{
				CICN_PlotIcon(theRectPtr, theButtonUpCICNHandle);
				myButtonDown = false;
				};
			};
		};

	if (myButtonDown) CICN_PlotIcon(theRectPtr, theButtonUpCICNHandle);

	SetPort(mySavePort);

	return myButtonDown;

	} /* UUCD_TrackCICNButton */

/*------------------------------------------------------------------------------
//
//	UUCD_GetCursor
//
*/

short UUCD_GetCursor (void)

	{

	return gUUCDCursor;

	} /* UUCD_GetCursor */

/*------------------------------------------------------------------------------
//
//	UUCD_SetCursor
//
*/

void UUCD_SetCursor (short theUUCDCursorNum)

	{
	CursHandle		  myCursHandle;

	if (theUUCDCursorNum == gUUCDCursor) return;

	switch (theUUCDCursorNum)
		{
	case UUCDCURS_Watch: myCursHandle = GetCursor(watchCursor); break;
	case UUCDCURS_Plus: myCursHandle = GetCursor(plusCursor); break;
	case UUCDCURS_CmdDot1: myCursHandle = GetCursor(kUUCDCmdDot1CursorID); break;
	case UUCDCURS_CmdDot2: myCursHandle = GetCursor(kUUCDCmdDot2CursorID); break;
	case UUCDCURS_MoveCell: myCursHandle = GetCursor(kUUCDMoveCellCursorID); break;
	case UUCDCURS_Loop1: myCursHandle = GetCursor(kUUCDLoop1CursorID); break;
	case UUCDCURS_Loop2: myCursHandle = GetCursor(kUUCDLoop2CursorID); break;
	case UUCDCURS_Loop3: myCursHandle = GetCursor(kUUCDLoop3CursorID); break;
	case UUCDCURS_Loop4: myCursHandle = GetCursor(kUUCDLoop4CursorID); break;
	default: myCursHandle = NULL; break;
		};

	if (myCursHandle != NULL)
		{
		HLock((Handle)myCursHandle);
		SetCursor(*myCursHandle);
		HUnlock((Handle)myCursHandle);
		}
	else InitCursor();

	gUUCDCursor = theUUCDCursorNum;

	} /* UUCD_SetCursor */

/*------------------------------------------------------------------------------
//
//	UUCD_SetLoopCursor
//
*/

void UUCD_SetLoopCursor (short theLoopCount)

	{

	switch (theLoopCount % 4)
		{
	case 0: UUCD_SetCursor(UUCDCURS_Loop1); break;
	case 1: UUCD_SetCursor(UUCDCURS_Loop2); break;
	case 2: UUCD_SetCursor(UUCDCURS_Loop3); break;
	case 3: UUCD_SetCursor(UUCDCURS_Loop4); break;
		};

	} /* UUCD_SetLoopCursor */

/*------------------------------------------------------------------------------
//
//	UUCD_GetString
//
*/

StringPtr UUCD_GetString (short theStringNum)

	{

	gUUCDString[0] = '\0';

	GetIndString(gUUCDString, kUUCDStringsListID, theStringNum + 1);

	return gUUCDString;

	} /* UUCD_GetString */

/*------------------------------------------------------------------------------
//
//	UUCD_OSErrAlert
//
*/

void UUCD_OSErrAlert (OSErr theOSErr)

	{
	Str31			  myOSErrStr31;

	if (theOSErr == noErr) return;

	NumToString((long)theOSErr, myOSErrStr31);
	ParamText(myOSErrStr31, "\p", "\p", "\p");

	UUCD_SetCursor(UUCDCURS_Arrow);
	Alert(kOSErrAlertID, NULL);

	} /* UUCD_OSErrAlert */

/*------------------------------------------------------------------------------
//
//	UUCD_CheckMemory
//
*/

Boolean UUCD_CheckMemory (long theMemoryWanted)

	{
	long			  myMemoryWanted = 20480L;

	if (theMemoryWanted == 0L)
		{
		if (FreeMem() >= myMemoryWanted) return true;
		goto UUCD_CheckMemory_MemFull;
		};

	myMemoryWanted += theMemoryWanted;
	if (FreeMem() < myMemoryWanted) goto UUCD_CheckMemory_MemFull;
	if (MaxBlock() < myMemoryWanted) goto UUCD_CheckMemory_MemFull;

	return true;

UUCD_CheckMemory_MemFull:

	UUCD_SetCursor(UUCDCURS_Arrow);
	Alert(kMemFullAlertID, NULL);

	return false;

	} /* UUCD_CheckMemory */

/*------------------------------------------------------------------------------
//
//	UUCD_GetFreeMemory
//
*/

long UUCD_GetFreeMemory (void)

	{

	return FreeMem();

	} /* UUCD_GetFreeMemory */

/*------------------------------------------------------------------------------
//
//	UUCD_UUDecode
//
*/

pascal Boolean UUCD_UUDecode_ModalFilter (DialogPtr theDialogPtr, EventRecord *theEventPtr, short *theItemHitPtr, void *theDataPtr);
pascal Boolean UUCD_UUDecode_ModalFilter (DialogPtr theDialogPtr, EventRecord *theEventPtr, short *theItemHitPtr, void *theDataPtr)

	{
#pragma unused (theDataPtr)

	Boolean			  myFilterResult = false;
	GrafPtr			  mySavePort;
	ModalFilterUPP		  myStandardModalFilterUPP;

	switch (theEventPtr->what)
		{
	case updateEvt:
		if ((WindowPtr)theEventPtr->message == (WindowPtr)theDialogPtr) break;
		return MAIN_DoUpdate((WindowPtr)theEventPtr->message);
		};

	if (GetStdFilterProc(&myStandardModalFilterUPP) == noErr)
		{
		GetPort(&mySavePort);
		SetPort(theDialogPtr);
		myFilterResult = CallModalFilterProc(myStandardModalFilterUPP, theDialogPtr, theEventPtr, theItemHitPtr);
		SetPort(mySavePort);
		};

	return myFilterResult;

	} /* UUCD_UUDecode_ModalFilter */

void UUCD_UUDecode (FSSpecPtr theFSSpecPtr)

	{
	OSErr			  myOSErr = noErr;
	StandardFileReply	  mySFReply;
	SFTypeList		  mySFTypeList;
	Point			  mySFLoc;

	if (theFSSpecPtr != NULL)
		{
		UUCD_Decode(theFSSpecPtr);
		UTIL_SetSFDir(theFSSpecPtr->vRefNum, theFSSpecPtr->parID);
		return;
		};

	SetPt(&mySFLoc, 100, 100);
	mySFTypeList[0] = 'TEXT';

	CustomGetFile ((FileFilterYDUPP)NULL,
				1, mySFTypeList, &mySFReply, 0, mySFLoc,
				(DlgHookYDUPP)NULL,
				(ModalFilterYDUPP)NewModalFilterYDProc(UUCD_UUDecode_ModalFilter),
				NULL,
				(ActivateYDUPP)NULL,
				(void *)NULL);

	if (!mySFReply.sfGood) return;

	UUCD_Decode(&mySFReply.sfFile);
	UTIL_SetSFDir(mySFReply.sfFile.vRefNum, mySFReply.sfFile.parID);

	} /* UUCD_UUDecode */

/*------------------------------------------------------------------------------
//
//	UUCD_UUEncode
//
*/

pascal Boolean UUCD_UUEncode_SFGetSourceFSSpec_FileFilter (CInfoPBPtr theCInfoPBPtr, void *theDataPtr);
pascal Boolean UUCD_UUEncode_SFGetSourceFSSpec_FileFilter (CInfoPBPtr theCInfoPBPtr, void *theDataPtr)

	{
#pragma unused (theDataPtr)

	if (((HFileInfo *)theCInfoPBPtr)->ioFlAttrib & ioDirMask) return false;

	if (((HFileInfo *)theCInfoPBPtr)->ioFlFndrInfo.fdType == 'TEXT') return true;

	return false;

	} /* UUCD_UUEncode_SFGetSourceFSSpec_FileFilter */

pascal Boolean UUCD_UUEncode_SFGetSourceFSSpec_ModalFilter (DialogPtr theDialogPtr, EventRecord *theEventPtr, short *theItemHitPtr, void *theDataPtr);
pascal Boolean UUCD_UUEncode_SFGetSourceFSSpec_ModalFilter (DialogPtr theDialogPtr, EventRecord *theEventPtr, short *theItemHitPtr, void *theDataPtr)

	{
#pragma unused (theDataPtr)

	Boolean			  myFilterResult = false;
	GrafPtr			  mySavePort;
	ModalFilterUPP		  myStandardModalFilterUPP;

	switch (theEventPtr->what)
		{
	case updateEvt:
		if ((WindowPtr)theEventPtr->message == (WindowPtr)theDialogPtr) break;
		return MAIN_DoUpdate((WindowPtr)theEventPtr->message);
		};

	if (GetStdFilterProc(&myStandardModalFilterUPP) == noErr)
		{
		GetPort(&mySavePort);
		SetPort(theDialogPtr);
		myFilterResult = CallModalFilterProc(myStandardModalFilterUPP, theDialogPtr, theEventPtr, theItemHitPtr);
		SetPort(mySavePort);
		};

	return myFilterResult;

	} /* UUCD_UUEncode_SFGetSourceFSSpec_ModalFilter */

Boolean UUCD_UUEncode_SFGetSourceFSSpec (FSSpecPtr theFSSpecPtr);
Boolean UUCD_UUEncode_SFGetSourceFSSpec (FSSpecPtr theFSSpecPtr)

	{
	StandardFileReply	  mySFReply;
	SFTypeList		  mySFTypeList;
	Point			  mySFLoc;

	SetPt(&mySFLoc, 100, 100);

	CustomGetFile( (FileFilterYDUPP)NewFileFilterYDProc(UUCD_UUEncode_SFGetSourceFSSpec_FileFilter),
				-1, mySFTypeList, &mySFReply, 0, mySFLoc,
				(DlgHookYDUPP)NULL,
				(ModalFilterYDUPP)NewModalFilterYDProc(UUCD_UUEncode_SFGetSourceFSSpec_ModalFilter),
				(ActivationOrderListPtr)NULL,
				(ActivateYDUPP)NULL,
				(void *)NULL);

	if (!mySFReply.sfGood) return false;

	*theFSSpecPtr = mySFReply.sfFile;
	return true;

	} /* UUCD_UUEncode_SFGetSourceFSSpec */

Boolean UUCD_UUEncode_SFPutDestFSSpec (FSSpecPtr theFSSpecPtr, ScriptCode *theScriptTagPtr, StringPtr theDefaultFileName);
Boolean UUCD_UUEncode_SFPutDestFSSpec (FSSpecPtr theFSSpecPtr, ScriptCode *theScriptTagPtr, StringPtr theDefaultFileName)

	{
	StandardFileReply	  mySFReply;
	Point			  mySFLoc;

	SetPt(&mySFLoc, 100, 100);

	CustomPutFile( UUCD_GetString(UUCDSTR_UUEncodeAs),
				theDefaultFileName,
				&mySFReply, 0, mySFLoc,
				(DlgHookYDUPP)NULL,
				(ModalFilterYDUPP)NewModalFilterYDProc(UUCD_UUEncode_SFGetSourceFSSpec_ModalFilter), /* la meme */
				(ActivationOrderListPtr)NULL,
				(ActivateYDUPP)NULL,
				(void *)NULL);

	if (!mySFReply.sfGood) return false;

	*theFSSpecPtr = mySFReply.sfFile;
	*theScriptTagPtr = mySFReply.sfScript;
	return true;

	} /* UUCD_UUEncode_SFPutDestFSSpec */

OSErr UUCD_UUEncode_WriteCutLine (short theDstFileRefNum, short theUUCDStrNum, short theSegmentNum, short theNbOfSegments);
OSErr UUCD_UUEncode_WriteCutLine (short theDstFileRefNum, short theUUCDStrNum, short theSegmentNum, short theNbOfSegments)

	{
	uchar			  myTempStr127[128];
	Str15			  myTempStr15;
	long				  myWriteCount;

	myTempStr127[0] = '\0';
	UTIL_PStrCat(myTempStr127, UUCD_GetString(theUUCDStrNum));

	NumToString((long)(theSegmentNum + 1), myTempStr15);
	UTIL_PStrCat(myTempStr127, myTempStr15);

	myTempStr15[0] = 0x01;
	myTempStr15[1] = '/';
	UTIL_PStrCat(myTempStr127, myTempStr15);

	NumToString((long)theNbOfSegments, myTempStr15);
	UTIL_PStrCat(myTempStr127, myTempStr15);

	switch (PREF_GetNewLineMode())
		{
	case kUUCDPrefsNewLineLF: myTempStr15[0] = 0x01; myTempStr15[1] = 0x0A; break;
	case kUUCDPrefsNewLineCRLF: myTempStr15[0] = 0x02; myTempStr15[1] = 0x0D; myTempStr15[2] = 0x0A; break;
	default: myTempStr15[0] = 0x01; myTempStr15[1] = 0x0D; break;
		};
	UTIL_PStrCat(myTempStr127, myTempStr15);

	myWriteCount = (long)myTempStr127[0];
	return FSWrite(theDstFileRefNum, &myWriteCount, &myTempStr127[1]);

	} /* UUCD_UUEncode_WriteCutLine */

void UUCD_UUEncode (FSSpecPtr theFSSpecPtr)

	{
	OSErr			  myOSErr = noErr;
	FSSpec			  mySourceFSSpec;
	FSSpec			  myDestFSSpec;
	FSSpec			  mySFDestFSSpec;
	ScriptCode		  myDstFileScriptTag;
	Str255			  myTempStr255;
	Str31			  myTempStr31;
	short			  mySrcFileRefNum = -1;
	long				  mySrcFileSize = 0L;
	short			  myDstFileRefNum = -1;
	OSType			  myDstFileCreator;
	OSType			  myDstFilePermissionMode;
	short			  myDstFileSegmentMaxLines;
	short			  myDstFileNbOfSegments = 1;
	short			  myDstFileSegmentNum;
	short			  mySegmentLineCount;
	short			  mySrcCharNum;
	short			  myDstCharNum;
	long				  myReadCount;
	char				  myReadBuffer[48];
	Boolean			  myEndOfSrcFileReached;
	char				  myDstLine[64];
	long				  myWriteCount;
	short			  myBlockNb;
	short			  myBlockNum;
	short			  myFirstSrcCharNum;
	short			  myCharNum;
	short			  myDstFileNewLineMode;

	myDstFileCreator = PREF_GetTextCreator();
	myDstFilePermissionMode = PREF_GetPermissionMode();
	myDstFileSegmentMaxLines = PREF_GetSegmentMaxLines();
	myDstFileNewLineMode = PREF_GetNewLineMode();

	/*------------------------------------------------------------------*/
	/*                   Ouverture du Fichier Source                    */
	/*------------------------------------------------------------------*/

	if (theFSSpecPtr != NULL)
		{
		mySourceFSSpec = *theFSSpecPtr;
		}
	else if (!UUCD_UUEncode_SFGetSourceFSSpec(&mySourceFSSpec)) goto UUCD_UUEncode_EXIT;

	myOSErr = FSpOpenDF(&mySourceFSSpec, fsRdPerm, &mySrcFileRefNum);
	if (myOSErr != noErr) goto UUCD_UUEncode_EXIT;

	myOSErr = SetFPos(mySrcFileRefNum, fsFromStart, 0L);
	if (myOSErr != noErr) goto UUCD_UUEncode_EXIT;

	myOSErr = GetEOF(mySrcFileRefNum, &mySrcFileSize);
	if (myOSErr != noErr) goto UUCD_UUEncode_EXIT;

	if (myDstFileSegmentMaxLines > 0)
		{
		long			  myFileNbOfLines;
		long			  myFileNbOfSegments;
		long			  myLastSegmentNbOfLines;

		myFileNbOfLines = mySrcFileSize / 45L;
		myFileNbOfSegments = myFileNbOfLines / (long)myDstFileSegmentMaxLines;
		myLastSegmentNbOfLines = myFileNbOfLines % (long)myDstFileSegmentMaxLines;
		if (myLastSegmentNbOfLines > 2) myFileNbOfSegments++;

		myDstFileNbOfSegments = (short)myFileNbOfSegments;
		}
	else myDstFileNbOfSegments = 1;

	/*------------------------------------------------------------------*/
	/*              Specification du Nom de Destination                 */
	/*------------------------------------------------------------------*/

	BlockMove(mySourceFSSpec.name, myTempStr255, 64L);
	UTIL_PStrCat(myTempStr255, UUCD_GetString(UUCDSTR_UUEncodeExtension));
	if (myTempStr255[0] > 63)
		{
		myTempStr255[0] = (uchar)63;
		myTempStr255[63] = '…';
		};

	if (!UUCD_UUEncode_SFPutDestFSSpec(&mySFDestFSSpec, &myDstFileScriptTag, myTempStr255)) goto UUCD_UUEncode_EXIT;

	/*------------------------------------------------------------------*/
	/*            Boucle de Lecture / Encodage / Ecriture               */
	/*------------------------------------------------------------------*/

	myEndOfSrcFileReached = false;
	myDstFileSegmentNum = 0;
	mySegmentLineCount = 0;

	while (!myEndOfSrcFileReached)
		{
		myReadCount = 45L;
		myOSErr = FSRead(mySrcFileRefNum, &myReadCount, myReadBuffer);
		if (myOSErr == eofErr)
			{
			myEndOfSrcFileReached = true;
			myOSErr = noErr;
			};
		if (myOSErr != noErr) goto UUCD_UUEncode_EXIT;

		if (myDstFileRefNum == -1)
			{
			/*---- Ouverture du Fichier de Destination ----*/
			
			BlockMove(mySFDestFSSpec.name, myTempStr255, 64L);
	
			if (myDstFileNbOfSegments > 1)
				{
				NumToString((long)(myDstFileSegmentNum + 1), myTempStr31);
				UTIL_PStrCat(myTempStr255, "\p.");
				UTIL_PStrCat(myTempStr255, myTempStr31);
				};
	
			myOSErr = FSMakeFSSpec(mySFDestFSSpec.vRefNum, mySFDestFSSpec.parID,
				myTempStr255, &myDestFSSpec);
	
			myOSErr = FSpDelete(&myDestFSSpec);
			if (myOSErr == fnfErr) myOSErr = noErr;
			if (myOSErr != noErr) goto UUCD_UUEncode_EXIT;
		
			myOSErr = FSpCreate(&myDestFSSpec, myDstFileCreator, 'TEXT', myDstFileScriptTag);
			if (myOSErr != noErr) goto UUCD_UUEncode_EXIT;
		
			myOSErr = FSpOpenDF(&myDestFSSpec, fsWrPerm, &myDstFileRefNum);
			if (myOSErr != noErr) goto UUCD_UUEncode_EXIT;
		
			myOSErr = SetFPos(myDstFileRefNum, fsFromStart, 0L);
			if (myOSErr != noErr) goto UUCD_UUEncode_EXIT;

			myOSErr = UUCD_UUEncode_WriteCutLine(myDstFileRefNum,
				UUCDSTR_UUEncodeBeginCut, myDstFileSegmentNum, myDstFileNbOfSegments);
			if (myOSErr != noErr) goto UUCD_UUEncode_EXIT;

			if (myDstFileSegmentNum == 0)
				{
				myDstCharNum = 0;
				myDstLine[myDstCharNum++] = 'b';
				myDstLine[myDstCharNum++] = 'e';
				myDstLine[myDstCharNum++] = 'g';
				myDstLine[myDstCharNum++] = 'i';
				myDstLine[myDstCharNum++] = 'n';
				myDstLine[myDstCharNum++] = ' ';
				myDstLine[myDstCharNum++] = ((char *)&myDstFilePermissionMode)[1];
				myDstLine[myDstCharNum++] = ((char *)&myDstFilePermissionMode)[2];
				myDstLine[myDstCharNum++] = ((char *)&myDstFilePermissionMode)[3];
				myDstLine[myDstCharNum++] = ' ';
	
				for (mySrcCharNum = 1; mySrcCharNum <= mySourceFSSpec.name[0]; mySrcCharNum++)
					{
					myDstLine[myDstCharNum++] = mySourceFSSpec.name[mySrcCharNum];
					};

				switch (myDstFileNewLineMode)
					{
				case kUUCDPrefsNewLineLF: myDstLine[myDstCharNum++] = 0x0A; break;
				case kUUCDPrefsNewLineCRLF: myDstLine[myDstCharNum++] = 0x0D; myDstLine[myDstCharNum++] = 0x0A; break;
				default: myDstLine[myDstCharNum++] = 0x0D; break;
					};
	
				myWriteCount = myDstCharNum;
				myOSErr = FSWrite(myDstFileRefNum, &myWriteCount, myDstLine);
				if (myOSErr != noErr) goto UUCD_UUEncode_EXIT;
				};
			};

		myDstCharNum = 0;
		myDstLine[myDstCharNum++] = (char)(' ' + myReadCount);

		/*   +------+ +------+ +------+ +------+   */
		/*   ..765432 ..107654 ..321076 ..543210   */
		/*   +------+ +------+ +------+ +------+   */

		myBlockNb = (short)((myReadCount + 2) / 3);

		for (myBlockNum = 0; myBlockNum < myBlockNb; myBlockNum++)
			{
			myFirstSrcCharNum = (short)(myBlockNum * 3);

			myDstLine[myDstCharNum++] = (char)(' '
				+ ((myReadBuffer[myFirstSrcCharNum] >> 2) & 0x3F));

			myDstLine[myDstCharNum++] = (char)(' '
				+ ((myReadBuffer[myFirstSrcCharNum] << 4) & 0x30)
				+ ((myReadBuffer[myFirstSrcCharNum + 1] >> 4) & 0x0F));

			myDstLine[myDstCharNum++] = (char)(' '
				+ ((myReadBuffer[myFirstSrcCharNum + 1] << 2) & 0x3C)
				+ ((myReadBuffer[myFirstSrcCharNum + 2] >> 6) & 0x03));

			myDstLine[myDstCharNum++] = (char)(' '
				+ (myReadBuffer[myFirstSrcCharNum + 2] & 0x3F));
			};

		switch (myDstFileNewLineMode)
			{
		case kUUCDPrefsNewLineLF: myDstLine[myDstCharNum++] = 0x0A; break;
		case kUUCDPrefsNewLineCRLF: myDstLine[myDstCharNum++] = 0x0D; myDstLine[myDstCharNum++] = 0x0A; break;
		default: myDstLine[myDstCharNum++] = 0x0D; break;
			};

		for (myCharNum = 0; myCharNum < myDstCharNum; myCharNum++)
			{
			if (myDstLine[myCharNum] == ' ') myDstLine[myCharNum] = 0x60; /* BackQuote */
			};

		UUCD_SetLoopCursor(mySegmentLineCount);

		myWriteCount = myDstCharNum;
		myOSErr = FSWrite(myDstFileRefNum, &myWriteCount, myDstLine);
		if (myOSErr != noErr) goto UUCD_UUEncode_EXIT;

		if (myDstFileNbOfSegments > 1)
			{
			if (myDstFileSegmentMaxLines > 0)
				{
				if (++mySegmentLineCount >= myDstFileSegmentMaxLines)
					{
					if (myDstFileSegmentNum < (myDstFileNbOfSegments - 1))
						{
						myOSErr = UUCD_UUEncode_WriteCutLine(myDstFileRefNum,
							UUCDSTR_UUEncodeEndCut, myDstFileSegmentNum, myDstFileNbOfSegments);
						if (myOSErr != noErr) goto UUCD_UUEncode_EXIT;

						mySegmentLineCount = 0;
						myDstFileSegmentNum++;
						FSClose(myDstFileRefNum);
						myDstFileRefNum = -1;
						};
					};
				};
			};
		};

	/*------------------------------------------------------------------*/
	/*                      Ecriture du Trailer                         */
	/*------------------------------------------------------------------*/

	if (myDstFileRefNum != -1)
		{
		myDstCharNum = 0;
		myDstLine[myDstCharNum++] = 0x60; /* BackQuote */

		switch (myDstFileNewLineMode)
			{
		case kUUCDPrefsNewLineLF: myDstLine[myDstCharNum++] = 0x0A; break;
		case kUUCDPrefsNewLineCRLF: myDstLine[myDstCharNum++] = 0x0D; myDstLine[myDstCharNum++] = 0x0A; break;
		default: myDstLine[myDstCharNum++] = 0x0D; break;
			};

		myDstLine[myDstCharNum++] = 'e';
		myDstLine[myDstCharNum++] = 'n';
		myDstLine[myDstCharNum++] = 'd';

		switch (myDstFileNewLineMode)
			{
		case kUUCDPrefsNewLineLF: myDstLine[myDstCharNum++] = 0x0A; break;
		case kUUCDPrefsNewLineCRLF: myDstLine[myDstCharNum++] = 0x0D; myDstLine[myDstCharNum++] = 0x0A; break;
		default: myDstLine[myDstCharNum++] = 0x0D; break;
			};

		myWriteCount = myDstCharNum;
		myOSErr = FSWrite(myDstFileRefNum, &myWriteCount, myDstLine);
		if (myOSErr != noErr) goto UUCD_UUEncode_EXIT;

		myOSErr = UUCD_UUEncode_WriteCutLine(myDstFileRefNum,
			UUCDSTR_UUEncodeEndCut, myDstFileSegmentNum, myDstFileNbOfSegments);
		if (myOSErr != noErr) goto UUCD_UUEncode_EXIT;
		};

	/*------------------------------------------------------------------*/
	/*              On se retrouve ici en cas d'annulation              */
	/*------------------------------------------------------------------*/

UUCD_UUEncode_EXIT:

	/*---- Fermeture du Fichier Destination ----*/

	if (myDstFileRefNum != -1) FSClose(myDstFileRefNum);

	/*---- Fermeture du Fichier Source ----*/

	if (mySrcFileRefNum != -1) FSClose(mySrcFileRefNum);

	/*---- OSErr Alert ----*/

	if (myOSErr != noErr) UUCD_OSErrAlert(myOSErr);

	} /* UUCD_UUEncode */

/*------------------------------------------------------------------------------
//
//	UUCD_DrawProgress
//
*/

void UUCD_DrawProgress (ulong theProgressMax, ulong theProgressVal)

	{
	GrafPtr		  mySavePort;
	Rect			  myProgressBarRect;
	short		  myProgressBarWidth;
	Rect			  myProgressBarColorRect;
	Rect			  myProgressBarWhiteRect;
	long			  myTempLong;

	while (theProgressMax > 65536L)
		{
		theProgressMax /= 256L;
		theProgressVal /= 256L;
		};

	myProgressBarRect = gUUCDProgressRect;
	InsetRect(&myProgressBarRect, 2, 2);

	if (theProgressMax != 0L)
		{
		myProgressBarWidth = myProgressBarRect.right - myProgressBarRect.left;
		myProgressBarColorRect = myProgressBarRect;
		myTempLong = theProgressVal;
		myTempLong *= (long)myProgressBarWidth;
		myTempLong /= theProgressMax;
		myProgressBarColorRect.right = myProgressBarColorRect.left + (short)myTempLong;
		myProgressBarWhiteRect = myProgressBarRect;
		myProgressBarWhiteRect.left = myProgressBarColorRect.right;
		}
	else
		{
		SetRect(&myProgressBarColorRect, 0, 0, 0, 0);
		myProgressBarWhiteRect = myProgressBarRect;
		};

	GetPort(&mySavePort);
	SetPort(gUUCDWindowPtr);
	if (gUUCDProgressPixPatHandle != NULL)
		{
		FillCRect(&myProgressBarColorRect, gUUCDProgressPixPatHandle);
		}
	else FillRect(&myProgressBarColorRect, (ConstPatternParam)&qd.black);
	FillRect(&myProgressBarWhiteRect, (ConstPatternParam)&qd.white);
	FrameRect(&gUUCDProgressRect);
	SetPort(mySavePort);

	} /* UUCD_DrawProgress */

/*------------------------------------------------------------------------------
//
//	UUCD_DrawMemory
//
*/

void UUCD_DrawMemory (void)

	{
	GrafPtr		  mySavePort;
	long			  myMemoryUsed;
	ulong		  myProgressBarMax;
	ulong		  myProgressBarVal;
	Rect			  myProgressBarRect;
	short		  myProgressBarWidth;
	Rect			  myProgressBarColorRect;
	Rect			  myProgressBarWhiteRect;
	long			  myTempLong;

	myMemoryUsed = gUUCDStartFreeMemory;
	myMemoryUsed -= UUCD_GetFreeMemory();
	if (myMemoryUsed < 0L) myMemoryUsed = 0L;

	myProgressBarMax = gUUCDStartFreeMemory;
	myProgressBarVal = (ulong)myMemoryUsed;

	while (myProgressBarMax > 65536L)
		{
		myProgressBarMax /= 256L;
		myProgressBarVal /= 256L;
		};

	myProgressBarRect = gUUCDMemoryRect;

	if (myProgressBarMax != 0L)
		{
		myProgressBarWidth = myProgressBarRect.right - myProgressBarRect.left;
		myProgressBarColorRect = myProgressBarRect;
		myTempLong = myProgressBarVal;
		myTempLong *= (long)myProgressBarWidth;
		myTempLong /= myProgressBarMax;
		myProgressBarColorRect.right = myProgressBarColorRect.left + (short)myTempLong;
		myProgressBarWhiteRect = myProgressBarRect;
		myProgressBarWhiteRect.left = myProgressBarColorRect.right;
		}
	else
		{
		SetRect(&myProgressBarColorRect, 0, 0, 0, 0);
		myProgressBarWhiteRect = myProgressBarRect;
		};

	GetPort(&mySavePort);
	SetPort(gUUCDWindowPtr);
	if (gUUCDMemoryPixPatHandle != NULL)
		{
		FillCRect(&myProgressBarColorRect, gUUCDMemoryPixPatHandle);
		}
	else FillRect(&myProgressBarColorRect, (ConstPatternParam)&qd.black);
	FillRect(&myProgressBarWhiteRect, (ConstPatternParam)&qd.white);
	SetPort(mySavePort);

	} /* UUCD_DrawMemory */

/*------------------------------------------------------------------------------
//
//	UUCD_SegListVSBarActionProc
//
*/

pascal void UUCD_SegListVSBarActionProc (ControlHandle theControlHandle, short theControlPart)

	{
	short			  myScrollAmount = 0;

	switch (theControlPart)
		{
	case kControlUpButtonPart:	myScrollAmount = -1 ; break;
	case kControlDownButtonPart:	myScrollAmount =  1 ; break;
	case kControlPageUpPart:		myScrollAmount = -5; break;
	case kControlPageDownPart:	myScrollAmount =  5; break;
	default: return;
		};

	SetControlValue(theControlHandle, GetControlValue(theControlHandle) + myScrollAmount);
	UUCD_SetTopSegment(GetControlValue(theControlHandle));

	} /* UUCD_SegListVSBarActionProc */

/*------------------------------------------------------------------------------
//
//	UUCD_FindSegment
//
*/

short UUCD_FindSegment (Point theLocalPoint)

	{
	short			  mySegmentNum;

	if (!PtInRect(theLocalPoint, UUCD_GetSegListViewRect())) return -1;

	mySegmentNum = theLocalPoint.v - (UUCD_GetSegListViewRect())->top;
	mySegmentNum /= kUUCD_SegmentHeight;
	mySegmentNum += UUCD_GetTopSegmentNum();

	if (mySegmentNum >= UUCD_GetNbOfSegments()) return -1;

	return mySegmentNum;

	} /* UUCD_FindSegment */

/*------------------------------------------------------------------------------
//
//	UUCD_SaveAllLinkedSegments
//
*/

void UUCD_SaveAllLinkedSegments (void)

	{
	short				  mySegmentNum;
	short				  myNbOfSegments = UUCD_GetNbOfSegments();

	UUCD_SetCursor(UUCDCURS_Watch);

	for (mySegmentNum = 0; mySegmentNum < myNbOfSegments; mySegmentNum++)
		{
		UUCD_DrawProgress((ulong)myNbOfSegments, (ulong)mySegmentNum);

		switch (UUCD_GetSegmentState(mySegmentNum))
			{
		case kUUCD_SEGSTATE_LinkedBegin:
		case kUUCD_SEGSTATE_Complete:

			if (UUCD_GetSegmentMark(mySegmentNum) == kUUCD_MARK_Check) break;

			if (!UUCD_SaveLinkedSegments(mySegmentNum))
				{
				UUCD_SetCursor(UUCDCURS_Arrow);
				return;
				};

			break;
			};
		};

	UUCD_DrawProgress(0L, 0L);
	UUCD_SetCursor(UUCDCURS_Arrow);

	} /* UUCD_SaveAllLinkedSegments */

/*------------------------------------------------------------------------------
//
//	UUCD_SaveLinkedSegments
//
*/

Boolean UUCD_SaveLinkedSegments (short theLinkedBeginSegmentNum)

	{
	OSErr				  myOSErr;
	BLOB_Err				  myBlobErr;
	Str63				  myDefaultDstFileName;
	FSSpec				  myDstFileFSSpec;
	unsigned short			  myDstFileIndex;
	short				  myDstFileRefNum;
	OSType				  myDstFileType = '????';
	OSType				  myDstFileCreator = '????';
	Boolean				  myTypeCreatorFound;
	short				  mySegmentNum;
	UUCD_Segment			  mySegment;
	Handle				  mySegmentDataHandle;
	long					  mySegmentDataSize;
	unsigned long			  mySegmentBlobID;
	long					  myWriteCount;
	Boolean				  myEndReached = false;

	if (!UUCD_GetSegment(theLinkedBeginSegmentNum, &mySegment)) return false;

	if (!UUCD_GetSegmentDstFileName(theLinkedBeginSegmentNum, myDefaultDstFileName))
		{
		UTIL_PStrCpy(myDefaultDstFileName, UUCD_GetString(UUCDSTR_Untitled));

		switch (mySegment.UUCD_SEGMENT_DataType)
			{
		case kUUCD_DATATYPE_gif:
			UTIL_PStrCat(myDefaultDstFileName, "\p.gif");
			break;
		case kUUCD_DATATYPE_jpg:
			UTIL_PStrCat(myDefaultDstFileName, "\p.jpg");
			break;
			};
		};

	for (myDstFileIndex = 1, myOSErr = noErr;
		myOSErr == noErr;
		myDstFileIndex++)
		{
		myOSErr = FSMakeFSSpec(mySegment.UUCD_SEGMENT_SrcFileFSSpec.vRefNum,
			mySegment.UUCD_SEGMENT_SrcFileFSSpec.parID,
			UTIL_BuildIndexedFileName(myDefaultDstFileName, myDstFileIndex),
			&myDstFileFSSpec);
		};
	if (myOSErr != fnfErr)
		{
		UUCD_OSErrAlert(myOSErr);
		return false;
		};

	myTypeCreatorFound = PREF_GetFileNameTypeCreator(myDstFileFSSpec.name, &myDstFileType, &myDstFileCreator);

	myOSErr = FSpCreate(&myDstFileFSSpec, myDstFileCreator, myDstFileType, smSystemScript);
	if (myOSErr != noErr)
		{
		UUCD_OSErrAlert(myOSErr);
		return false;
		};

	myOSErr = FSpOpenDF(&myDstFileFSSpec, fsWrPerm, &myDstFileRefNum);
	if (myOSErr != noErr)
		{
		UUCD_OSErrAlert(myOSErr);
		return false;
		};

	for (mySegmentNum = theLinkedBeginSegmentNum;
		mySegmentNum < UUCD_GetNbOfSegments();
		mySegmentNum++)
		{
		if (UUCD_GetSegment(mySegmentNum, &mySegment))
			{
			mySegmentDataSize = mySegment.UUCD_SEGMENT_DataSize;
			mySegmentBlobID = mySegment.UUCD_SEGMENT_BlobID;

			mySegmentDataHandle = NewHandle(mySegmentDataSize);
			if (mySegmentDataHandle != NULL)
				{
				HLock(mySegmentDataHandle);

				myBlobErr = BLOB_ReadFile(UUCD_GetBlobBasePtr(),
					mySegmentBlobID, *mySegmentDataHandle);

				if (myBlobErr == BLOB_ERR_NoErr)
					{
					myWriteCount = mySegmentDataSize;
					myOSErr = FSWrite(myDstFileRefNum, &myWriteCount, *mySegmentDataHandle);
					};

				HUnlock(mySegmentDataHandle);
				DisposeHandle(mySegmentDataHandle);
				}
			else SysBeep(1);

//			mySegmentDataHandle = mySegment.UUCD_SEGMENT_DataHdl;
//			HLock(mySegmentDataHandle);
//			myWriteCount = GetHandleSize(mySegmentDataHandle);
//			myOSErr = FSWrite(myDstFileRefNum, &myWriteCount, *mySegmentDataHandle);
//			HUnlock(mySegmentDataHandle);

			switch (mySegment.UUCD_SEGMENT_State)
				{
			case kUUCD_SEGSTATE_LinkedBegin:
			case kUUCD_SEGSTATE_LinkedBody:
				break;
			default:
				myEndReached = true;
				break;
				};
			};

		UUCD_SetSegmentMark(mySegmentNum, kUUCD_MARK_Check);
		UUCD_DrawSegment(mySegmentNum, NULL);

		if (myEndReached) break;
		};

	myOSErr = FSClose(myDstFileRefNum);

	if (PREF_IsAutoOpenEnabled())
	if (myTypeCreatorFound) UUCD_OSErrAlert(AESendFinderOpenDoc(&myDstFileFSSpec));

	return true;

	} /* UUCD_SaveLinkedSegments */

/*------------------------------------------------------------------------------
//
//	UUCD_RemoveMarkedSegments
//
*/

void UUCD_RemoveMarkedSegments (void)

	{
	short			  mySegmentNum;
	short			  myVSBarMax;
	short			  myVSBarValue;

	if (gUUCD_DECODE_State != kUUCD_DECODE_STATE_Idle)
		{
		SysBeep(1);
		return;
		};

	for (mySegmentNum = UUCD_GetNbOfSegments() - 1;
		mySegmentNum >= 0;
		mySegmentNum--)
		{
		if (UUCD_GetSegmentMark(mySegmentNum) == kUUCD_MARK_Check)
			UUCD_DelSegment(mySegmentNum);
		};

	myVSBarMax = UUCD_GetNbOfSegments() - UUCD_GetNbOfVisibleSegments();
	if (myVSBarMax < 0) myVSBarMax = 0;
	myVSBarValue = UUCD_GetTopSegmentNum();
	if (myVSBarValue > myVSBarMax) myVSBarValue = myVSBarMax;
	SetControlMaximum(gUUCDSegListVScrollBar, myVSBarMax);
	SetControlValue(gUUCDSegListVScrollBar, myVSBarValue);
	UUCD_SetTopSegment(myVSBarValue);
	UUCD_CheckSegmentStates();
	UUCD_DrawSegments();

	UUCD_DrawMemory();

	} /* UUCD_RemoveMarkedSegments */

/*------------------------------------------------------------------------------
//
//	UUCD_DECODE
//
*/

#define kUUCDParseLineSize 256  // 011008: 128 -> 256
#define kUUCDParseBufferSize 32768

/*------------------------------------------------------------------------------
*/

FSSpec			  gUUCD_DECODE_SrcFSSpec;
long				  gUUCD_DECODE_SrcFileSize;

Str255			  gUUCD_DECODE_LastSubject;
Str255			  gUUCD_DECODE_FileName;

short			  gUUCD_DECODE_SegmentNum;
short			  gUUCD_DECODE_FileSegNum;

void UUCD_DECODE_NewSegment (UUCD_SegmentType theSegmentType)

	{
	UUCD_SegmentState	  mySegmentState;
	short			  myVSBarMax;

	if (gUUCD_DECODE_SegmentNum != -1)
		{
		UUCD_BlobSegment(gUUCD_DECODE_SegmentNum);
		UUCD_DrawMemory();
		};

	gUUCD_DECODE_SegmentNum = UUCD_NewSegment();
	gUUCD_DECODE_FileSegNum++;

	UUCD_SetSegmentSrcFile(gUUCD_DECODE_SegmentNum,
		&gUUCD_DECODE_SrcFSSpec, gUUCD_DECODE_SrcFileSize);

	if (*gUUCD_DECODE_LastSubject == '\0')
		{
		Str255		  mySegmentName;

		*mySegmentName = '\0';
		UTIL_PStrCat(mySegmentName, "\p[");
		UTIL_PStrCat(mySegmentName, gUUCD_DECODE_SrcFSSpec.name);
		UTIL_PStrCat(mySegmentName, "\p]");

		if (gUUCD_DECODE_FileSegNum > 1)
			{
			Str31		  mySegmentNumStr31;
			NumToString((long)(gUUCD_DECODE_FileSegNum), mySegmentNumStr31);
			UTIL_PStrCat(mySegmentName, "\p .");
			UTIL_PStrCat(mySegmentName, mySegmentNumStr31);
			};

		UUCD_SetSegmentName(gUUCD_DECODE_SegmentNum, mySegmentName);

		if (gUUCD_DECODE_FileSegNum == 2)
			{
			UUCD_GetSegmentName(gUUCD_DECODE_SegmentNum - 1, mySegmentName);
			UTIL_PStrCat(mySegmentName, "\p .1");
			UUCD_SetSegmentName(gUUCD_DECODE_SegmentNum - 1, mySegmentName);
			};
		}
	else UUCD_SetSegmentName(gUUCD_DECODE_SegmentNum, gUUCD_DECODE_LastSubject);

	UUCD_SetSegmentType(gUUCD_DECODE_SegmentNum, theSegmentType);

	mySegmentState = kUUCD_SEGSTATE_Body;
	if (*gUUCD_DECODE_FileName != '\0')
		{
		mySegmentState = kUUCD_SEGSTATE_Begin;
		UUCD_SetSegmentDstFileName(gUUCD_DECODE_SegmentNum, gUUCD_DECODE_FileName);
		*gUUCD_DECODE_FileName = '\0';
		};
	UUCD_SetSegmentState(gUUCD_DECODE_SegmentNum, mySegmentState);

	UUCD_DrawSegment(gUUCD_DECODE_SegmentNum, NULL);
	myVSBarMax = UUCD_GetNbOfSegments() - UUCD_GetNbOfVisibleSegments();
	if (myVSBarMax < 0) myVSBarMax = 0;
	UUCD_SetTopSegment(myVSBarMax);
	SetControlMaximum(gUUCDSegListVScrollBar, myVSBarMax);
	SetControlValue(gUUCDSegListVScrollBar, myVSBarMax);

	} /* UUCD_DECODE_NewSegment */

/*------------------------------------------------------------------------------
*/

short				  gUUCD_DECODE_TextFileRefNum = -1;
char					 *gUUCD_DECODE_TextFileBuffer = NULL;
long					  gUUCD_DECODE_TextCharNum = 0L;
char					  gUUCD_DECODE_TextChar;
char					  gUUCD_DECODE_LastTextChar;
Str31				  gUUCD_DECODE_SubjectTag;
long					  gUUCD_DECODE_FileLineNum;
long					  gUUCD_DECODE_FileBlocksRead;
char					  gUUCD_DECODE_LastLastTextLineChars[kUUCDParseLineSize];
short				  gUUCD_DECODE_LastLastTextLineLen;
char					  gUUCD_DECODE_LastTextLineChars[kUUCDParseLineSize];
short				  gUUCD_DECODE_LastTextLineLen;
char					  gUUCD_DECODE_TextLineChars[kUUCDParseLineSize];
short				  gUUCD_DECODE_TextLineLen;
Boolean				  gUUCD_DECODE_B64HeadLineSeen;
Boolean				  gUUCD_DECODE_CmdDotCursorState;
LineType				  gUUCD_DECODE_LastLineType = LINETYPE_None;

/*------------------------------------------------------------------------------
//   UUCD_Decode(FSSPecPtr) lance le decodage d'un fichier
//   UUCD_Decode(NULL) fait progresser le decodage en cours
*/

OSErr UUCD_Decode (FSSpecPtr theFSSpecPtr)

	{
	OSErr			  myOSErr = noErr;

	if (theFSSpecPtr != NULL)
		{
		if (gUUCD_DECODE_State == kUUCD_DECODE_STATE_Idle)
			{
			myOSErr = UUCD_DECODE_Start(theFSSpecPtr);
			if (myOSErr != noErr) UUCD_DECODE_End(myOSErr);
			}
		else myOSErr = 1;

		return myOSErr;
		};

	switch (gUUCD_DECODE_State)
		{
	case kUUCD_DECODE_STATE_Running:
		myOSErr = UUCD_DECODE_Body();
		if (myOSErr != noErr) UUCD_DECODE_End(myOSErr);
		return myOSErr;

	case kUUCD_DECODE_STATE_Done:
		UUCD_DECODE_End(noErr);
		};

	return noErr;

	} /* UUCD_Decode */

/*------------------------------------------------------------------------------
*/

OSErr UUCD_DECODE_Start (FSSpecPtr theFSSpecPtr)

	{
	OSErr			  myOSErr = noErr;

	if (theFSSpecPtr == NULL) return 1;
	if (gUUCD_DECODE_State != kUUCD_DECODE_STATE_Idle) return 1;

	gUUCD_DECODE_TextFileRefNum = -1;
	gUUCD_DECODE_TextFileBuffer = NULL;
	gUUCD_DECODE_TextCharNum = 0L;
	gUUCD_DECODE_TextChar = '\0';
	gUUCD_DECODE_LastTextChar = '\0';

	BlockMove(theFSSpecPtr, &gUUCD_DECODE_SrcFSSpec, (long)sizeof(FSSpec));

	PREF_GetSubjectTag(gUUCD_DECODE_SubjectTag);

	DisableItem(gApplicationMenus[MENU_File], 0);
	DrawMenuBar();

	myOSErr = FSpOpenDF(&gUUCD_DECODE_SrcFSSpec, fsRdPerm, &gUUCD_DECODE_TextFileRefNum);
	if (myOSErr != noErr) return myOSErr;

	gUUCD_DECODE_SrcFileSize = 0L;
	myOSErr = GetEOF(gUUCD_DECODE_TextFileRefNum, &gUUCD_DECODE_SrcFileSize);
	if (myOSErr != noErr) return myOSErr;

	if (!UUCD_CheckMemory(kUUCDParseBufferSize)) return memFullErr;

	myOSErr = SetFPos(gUUCD_DECODE_TextFileRefNum, fsFromStart, 0L);
	if (myOSErr != noErr) return myOSErr;

	gUUCD_DECODE_TextFileBuffer = (char *)NewPtr(kUUCDParseBufferSize);
	myOSErr = MemError();
	if (myOSErr != noErr) return myOSErr;

	gUUCD_DECODE_TextLineLen = 0;
	gUUCD_DECODE_LastTextLineLen = 0;
	*gUUCD_DECODE_LastSubject = '\0';
	*gUUCD_DECODE_FileName = '\0';
	gUUCD_DECODE_SegmentNum = -1;
	gUUCD_DECODE_FileSegNum = 0;
	gUUCD_DECODE_FileLineNum = 0L;
	gUUCD_DECODE_FileBlocksRead = 0L;

	gUUCD_DECODE_B64HeadLineSeen = false;

	gUUCD_DECODE_State = kUUCD_DECODE_STATE_Running;
	gUUCD_DECODE_CmdDotCursorState = false;

	gUUCD_DECODE_LastLineType = LINETYPE_None;

	return noErr;

	} /* UUCD_DECODE_Start */

/*------------------------------------------------------------------------------
*/

OSErr UUCD_DECODE_Body (void)

	{
	OSErr			  myOSErr = noErr;
	long				  myReadCount;
	short			  mySrcCharNum;
	short			  myDstCharNum;
	uchar			  mySrcChar;
	UUCD_SegmentState	  mySegmentState;
	long				  myStartTicks;
	Boolean			  mySubjectLineFlag;
	Boolean			  myLineDone;
	Boolean			  myEOFReached = false;
	HQXLineType		  myHQXLineType;
	HQXLineType		  myLastHQXLineType;
	B64LineType		  myB64LineType;
	B64LineType		  myLastB64LineType;

	if (gUUCD_DECODE_State != kUUCD_DECODE_STATE_Running) return noErr;

	UUCD_SetCursor(gUUCD_DECODE_CmdDotCursorState ? UUCDCURS_CmdDot1 : UUCDCURS_CmdDot2);
	gUUCD_DECODE_CmdDotCursorState = !gUUCD_DECODE_CmdDotCursorState;

	for (myStartTicks = TickCount();
		gUUCD_DECODE_TextCharNum < gUUCD_DECODE_SrcFileSize;
		gUUCD_DECODE_TextCharNum++)
		{
		if ((gUUCD_DECODE_TextCharNum % kUUCDParseBufferSize) == 0)
			{
			myReadCount = (long)kUUCDParseBufferSize;
			myOSErr = FSRead(gUUCD_DECODE_TextFileRefNum, &myReadCount,
				(Ptr)gUUCD_DECODE_TextFileBuffer);
			if (myOSErr == eofErr)
				{
				long			  myReadChars;
				myReadChars = gUUCD_DECODE_FileBlocksRead;
				myReadChars *= (long)kUUCDParseBufferSize;
				myReadChars += myReadCount;
				if (myReadChars == gUUCD_DECODE_SrcFileSize)
					{
					myOSErr = noErr;
					myEOFReached = true;
					};
				};
			if (myOSErr != noErr) break;
			gUUCD_DECODE_FileBlocksRead++;
			};

		gUUCD_DECODE_LastTextChar = gUUCD_DECODE_TextChar;
		gUUCD_DECODE_TextChar = gUUCD_DECODE_TextFileBuffer[gUUCD_DECODE_TextCharNum % kUUCDParseBufferSize];

		if ((gUUCD_DECODE_TextChar == 0x0D)
			|| ((gUUCD_DECODE_TextChar == 0x0A) && (gUUCD_DECODE_LastTextChar != 0x0D)))
			{
			if ((TickCount() - myStartTicks) >= 60L) break;

			if ((++gUUCD_DECODE_FileLineNum % 10L) == 0L)
				{
				UUCD_DrawProgress(gUUCD_DECODE_SrcFileSize, gUUCD_DECODE_TextCharNum);
				UUCD_DrawMemory();
				};

			if (gUUCD_DECODE_TextLineLen == 0) continue;

			myLineDone = false;

			/*----------------------------------------------------------*/
			/*				UU Full line ?						*/
			/*----------------------------------------------------------*/

			if (!myLineDone)
			if (UUCD_IsUUFullLine(gUUCD_DECODE_TextLineChars, &gUUCD_DECODE_TextLineLen))
			if ((gUUCD_DECODE_LastLineType == LINETYPE_UUCode) || (gUUCD_DECODE_LastLineType == LINETYPE_None))
				{
				myLineDone = true;

				if (gUUCD_DECODE_SegmentNum >= 0)
					{
					if (!UUCD_IsUUFullLine(gUUCD_DECODE_LastTextLineChars,
						&gUUCD_DECODE_LastTextLineLen))
						{
						if (UUCD_GetSegmentDataSize(gUUCD_DECODE_SegmentNum) != 0L)
							{
							UUCD_DrawSegment(gUUCD_DECODE_SegmentNum, NULL);
							UUCD_DECODE_NewSegment(kUUCD_SEGTYPE_UUCode);
							};
						};
					if (UUCD_GetSegmentType(gUUCD_DECODE_SegmentNum) != kUUCD_SEGTYPE_UUCode)
						{
						UUCD_DrawSegment(gUUCD_DECODE_SegmentNum, NULL);
						UUCD_DECODE_NewSegment(kUUCD_SEGTYPE_UUCode);
						};
					}
				else UUCD_DECODE_NewSegment(kUUCD_SEGTYPE_UUCode);

				if (!UUCD_CheckMemory(0L)) return memFullErr;
				UUCD_AddUUToSegment(gUUCD_DECODE_SegmentNum,
					gUUCD_DECODE_TextLineChars,
					gUUCD_DECODE_TextLineLen,
					gUUCD_DECODE_FileLineNum);

				gUUCD_DECODE_LastLineType = LINETYPE_UUCode;
				};

			/*----------------------------------------------------------*/
			/*				'begin' line ?						*/
			/*----------------------------------------------------------*/

			if (!myLineDone)
				{
				if (gUUCD_DECODE_TextLineLen > 11)
				if (gUUCD_DECODE_TextLineChars[0] == 'b')
				if (gUUCD_DECODE_TextLineChars[1] == 'e')
				if (gUUCD_DECODE_TextLineChars[2] == 'g')
				if (gUUCD_DECODE_TextLineChars[3] == 'i')
				if (gUUCD_DECODE_TextLineChars[4] == 'n')
				if (gUUCD_DECODE_TextLineChars[5] == ' ')
					{
					myLineDone = true;

					mySrcCharNum = 6;

					/* Skip Space(s) */
					while ((mySrcCharNum < gUUCD_DECODE_TextLineLen)
						&& (gUUCD_DECODE_TextLineChars[mySrcCharNum] == ' ')) mySrcCharNum++;

					/* Skip Octal Mode */
					while ((mySrcCharNum < gUUCD_DECODE_TextLineLen)
						&& (gUUCD_DECODE_TextLineChars[mySrcCharNum] != ' ')) mySrcCharNum++;

					/* Skip Space(s) */
					while ((mySrcCharNum < gUUCD_DECODE_TextLineLen)
						&& (gUUCD_DECODE_TextLineChars[mySrcCharNum] == ' ')) mySrcCharNum++;

					/* Get File Name (&Remove Trailing Blanks) */
					for (myDstCharNum = 0;
						mySrcCharNum < gUUCD_DECODE_TextLineLen;
						mySrcCharNum++)
						{
						mySrcChar = gUUCD_DECODE_TextLineChars[mySrcCharNum];
						if (!UTIL_IsFileNameChar(mySrcChar)) continue;
						gUUCD_DECODE_FileName[++myDstCharNum] = mySrcChar;
						};
					while (gUUCD_DECODE_FileName[myDstCharNum] == ' ')
						{
						if (myDstCharNum == 0) break;
						myDstCharNum--;
						};
					gUUCD_DECODE_FileName[0] = (uchar)myDstCharNum;

					/*---- Nouveau Segment, vide ----*/

					UUCD_DECODE_NewSegment(kUUCD_SEGTYPE_UUCode);

					gUUCD_DECODE_LastLineType = LINETYPE_UUCode;
					};
				};

			/*----------------------------------------------------------*/
			/*				Subject line ?						*/
			/*----------------------------------------------------------*/

			if (!myLineDone)
				{
				if (gUUCD_DECODE_TextLineLen > (short)gUUCD_DECODE_SubjectTag[0])
					{
					mySubjectLineFlag = true;
					for (mySrcCharNum = 1;
						mySrcCharNum <= (short)gUUCD_DECODE_SubjectTag[0];
						mySrcCharNum++)
						{
						if (gUUCD_DECODE_TextLineChars[mySrcCharNum - 1]
							!= gUUCD_DECODE_SubjectTag[mySrcCharNum])
								mySubjectLineFlag = false;
						};
					}
				else mySubjectLineFlag = false;

				if (mySubjectLineFlag)
					{
					myLineDone = true;

					myDstCharNum = 0;
					for (mySrcCharNum = (short)gUUCD_DECODE_SubjectTag[0];
						mySrcCharNum < gUUCD_DECODE_TextLineLen;
						mySrcCharNum++)
						{
						gUUCD_DECODE_LastSubject[++myDstCharNum] =
							gUUCD_DECODE_TextLineChars[mySrcCharNum];
						};
					gUUCD_DECODE_LastSubject[0] = (uchar)myDstCharNum;

					gUUCD_DECODE_LastLineType = LINETYPE_None;
					};
				};

			/*----------------------------------------------------------*/
			/*				'end' line ?						*/
			/*----------------------------------------------------------*/

			if (!myLineDone)
				{
				if (gUUCD_DECODE_TextLineLen >= 3)
				if (gUUCD_DECODE_TextLineChars[0] == 'e')
				if (gUUCD_DECODE_TextLineChars[1] == 'n')
				if (gUUCD_DECODE_TextLineChars[2] == 'd')
					{
					myLineDone = true;

					if ((*gUUCD_DECODE_LastTextLineChars == ' ') || (*gUUCD_DECODE_LastTextLineChars == '`'))
						{
						short			  myByteCount;

						myByteCount = *gUUCD_DECODE_LastLastTextLineChars - ' ';
						if (myByteCount >= 64) myByteCount -= 64;
						*gUUCD_DECODE_LastLastTextLineChars = ' ' + myByteCount;

						UUCD_AddUUToSegment(gUUCD_DECODE_SegmentNum,
							gUUCD_DECODE_LastLastTextLineChars,
							gUUCD_DECODE_LastLastTextLineLen,
							gUUCD_DECODE_FileLineNum - 2);
						}
					else
						{
						short			  myByteCount;

						myByteCount = *gUUCD_DECODE_LastTextLineChars - ' ';
						if (myByteCount >= 64) myByteCount -= 64;
						*gUUCD_DECODE_LastTextLineChars = ' ' + myByteCount;

						UUCD_AddUUToSegment(gUUCD_DECODE_SegmentNum,
							gUUCD_DECODE_LastTextLineChars,
							gUUCD_DECODE_LastTextLineLen,
							gUUCD_DECODE_FileLineNum - 1);
						};

					mySegmentState = UUCD_GetSegmentState(gUUCD_DECODE_SegmentNum);

					switch (mySegmentState)
						{
					case kUUCD_SEGSTATE_Begin:
					case kUUCD_SEGSTATE_Complete:
						mySegmentState = kUUCD_SEGSTATE_Complete;
						break;
					default:
						mySegmentState = kUUCD_SEGSTATE_End;
						break;
						};

					UUCD_SetSegmentState(gUUCD_DECODE_SegmentNum, mySegmentState);
					UUCD_DrawSegment(gUUCD_DECODE_SegmentNum, NULL);

					gUUCD_DECODE_LastLineType = LINETYPE_UUCode;
					};
				};

			/*----------------------------------------------------------*/
			/*					HQX line ?					*/
			/*----------------------------------------------------------*/

			if (!myLineDone)
				{
				myHQXLineType = UUCD_IsHQXLine(gUUCD_DECODE_TextLineChars,
						&gUUCD_DECODE_TextLineLen);

				if (myHQXLineType != HQXLINETYPE_None)
					{
					myLastHQXLineType = UUCD_IsHQXLine(gUUCD_DECODE_LastTextLineChars,
							&gUUCD_DECODE_LastTextLineLen);
	
					if (myHQXLineType == HQXLINETYPE_Last)
						if (myLastHQXLineType == HQXLINETYPE_None) myHQXLineType = HQXLINETYPE_None;
					};

				if (myHQXLineType != HQXLINETYPE_None)
					{
					myLineDone = true;
	
					myLastHQXLineType = UUCD_IsHQXLine(gUUCD_DECODE_LastTextLineChars,
						&gUUCD_DECODE_LastTextLineLen);
	
					if (gUUCD_DECODE_SegmentNum >= 0)
						{
						if (myLastHQXLineType == HQXLINETYPE_None)
							{
							if (UUCD_GetSegmentDataSize(gUUCD_DECODE_SegmentNum) != 0L)
								{
								UUCD_DrawSegment(gUUCD_DECODE_SegmentNum, NULL);
								UUCD_DECODE_NewSegment(kUUCD_SEGTYPE_HQX);
								};
							};
						if (UUCD_GetSegmentType(gUUCD_DECODE_SegmentNum) != kUUCD_SEGTYPE_HQX)
							{
							UUCD_DrawSegment(gUUCD_DECODE_SegmentNum, NULL);
							UUCD_DECODE_NewSegment(kUUCD_SEGTYPE_HQX);
							};
						}
					else UUCD_DECODE_NewSegment(kUUCD_SEGTYPE_HQX);
	
					if (!UUCD_CheckMemory(0L)) return memFullErr;
					UUCD_AddHQXToSegment(gUUCD_DECODE_SegmentNum,
						gUUCD_DECODE_TextLineChars,
						gUUCD_DECODE_TextLineLen,
						gUUCD_DECODE_FileLineNum);
	
					if (myHQXLineType == HQXLINETYPE_First)
						{
						ushort		  mySrcCharValue;
						uchar		  myDstChars[64];
						short		  myFrstDstCharNum;
	
						UUCD_SetSegmentState(gUUCD_DECODE_SegmentNum, kUUCD_SEGSTATE_Begin);
						UUCD_DrawSegment(gUUCD_DECODE_SegmentNum, NULL);
	
						/*   +------+ +------+ +------+ +------+   */
						/*   ..765432 ..107654 ..321076 ..543210   */
						/*   +------+ +------+ +------+ +------+   */
	
						for (myDstCharNum = 0; myDstCharNum < 64; myDstCharNum++)
							myDstChars[myDstCharNum] = 0x00;
	
						for (mySrcCharNum = 0; mySrcCharNum < (gUUCD_DECODE_TextLineLen - 1); mySrcCharNum++)
							{
							mySrcChar = gUUCD_DECODE_TextLineChars[mySrcCharNum + 1];
	
							for (mySrcCharValue = 0; mySrcCharValue < 64; mySrcCharValue++)
								if (mySrcChar == gHQXCharSet[mySrcCharValue]) break;
							if (mySrcCharValue > 63) mySrcCharValue = 0;
	
							myFrstDstCharNum = (mySrcCharNum / 4) * 3;
	
							switch (mySrcCharNum % 4)
								{
							case 0:
								myDstChars[myFrstDstCharNum] |= (mySrcCharValue << 2) & 0xFC;
								break;
							case 1:
								myDstChars[myFrstDstCharNum] |= (mySrcCharValue >> 4) & 0x03;
								myDstChars[myFrstDstCharNum + 1] |= (mySrcCharValue << 4) & 0xF0;
								break;
							case 2:
								myDstChars[myFrstDstCharNum + 1] |= (mySrcCharValue >> 2) & 0x0F;
								myDstChars[myFrstDstCharNum + 2] |= (mySrcCharValue << 6) & 0xC0;
								break;
							case 3:
								myDstChars[myFrstDstCharNum + 2] |= mySrcCharValue & 0x3F;
								break;
								};
							};
	
						if (myDstChars[0] > (uchar)23) myDstChars[0] = (uchar)23;
						UTIL_PStrCat((StringPtr)myDstChars, "\p.hqx");
						UUCD_SetSegmentDstFileName(gUUCD_DECODE_SegmentNum, (StringPtr)myDstChars);
						};
	
					if (myHQXLineType == HQXLINETYPE_Last)
						{
						mySegmentState = UUCD_GetSegmentState(gUUCD_DECODE_SegmentNum);
						mySegmentState = (mySegmentState == kUUCD_SEGSTATE_Begin)
							? kUUCD_SEGSTATE_Complete : kUUCD_SEGSTATE_End;

						UUCD_SetSegmentState(gUUCD_DECODE_SegmentNum, mySegmentState);
						UUCD_DrawSegment(gUUCD_DECODE_SegmentNum, NULL);
						};

					gUUCD_DECODE_LastLineType = LINETYPE_HQX;
					};

				};

			/*----------------------------------------------------------*/
			/*					B64 HeadLine ?					*/
			/*----------------------------------------------------------*/

			if (!myLineDone)
				{
				ushort		  mySrcCharNum = 0;
				ushort		  myDstCharNum;
				Boolean		  myLineMatch = true;

				for (myDstCharNum = 0; gB64HeadMark[myDstCharNum] != '\0'; myDstCharNum++)
					{
					while (gUUCD_DECODE_TextLineChars[mySrcCharNum] == ' ') mySrcCharNum++;

					if (gUUCD_DECODE_TextLineChars[mySrcCharNum] == '\0')
						{
						myLineMatch = false;
						break;
						};

					if (UTIL_ToUpper(gUUCD_DECODE_TextLineChars[mySrcCharNum++]) != gB64HeadMark[myDstCharNum])
						{
						myLineMatch = false;
						break;
						};
					};

				if (myLineMatch)
					{
					gUUCD_DECODE_B64HeadLineSeen = true;
					myLineDone = true;
					gUUCD_DECODE_LastLineType = LINETYPE_Base64;
					};

				};

			/*----------------------------------------------------------*/
			/*					B64 NameLine ?					*/
			/*----------------------------------------------------------*/

			if (!myLineDone)
				{
				short		  mySrcCharNum;
				short		  myDstCharNum;
				char			  mySrcChar;
				short		  myMarkPos;
				short		  myFileNamePos;
				Boolean		  myNameLine = false;

				myMarkPos = UTIL_StrPos(gUUCD_DECODE_TextLineChars, gB64CFNAMMark, true, true);

				if (myMarkPos >= 0)
					{
					if (UTIL_StrPos(gUUCD_DECODE_TextLineChars, gB64CDISPMark, true, true) >= 0)
						myNameLine = true;

					if (!myNameLine)
					if (UTIL_StrPos(gUUCD_DECODE_LastTextLineChars, gB64CDISPMark, true, true) >= 0)
						myNameLine = true;

					if (myNameLine)
						{
						myFileNamePos = myMarkPos;
						while (gUUCD_DECODE_TextLineChars[myFileNamePos] != '"') myFileNamePos++;
						mySrcCharNum = ++myFileNamePos;
						myDstCharNum = 0;
						while ((mySrcChar = gUUCD_DECODE_TextLineChars[mySrcCharNum]) != '\0')
							{
							if (mySrcChar == '"') break;
							if (myDstCharNum > 27) break;
							if (UTIL_IsFileNameChar(mySrcChar))
								{
								gUUCD_DECODE_FileName[++myDstCharNum] = mySrcChar;
								};
							mySrcCharNum++;
							};
						gUUCD_DECODE_FileName[0] = (char)myDstCharNum;

						myLineDone = true;
						gUUCD_DECODE_LastLineType = LINETYPE_Base64;
						};
					};


/*
				short		  mySrcCharNum;
				short		  myDstCharNum;
				char			  mySrcChar;
				short		  myMarkLen;
				Boolean		  myLineMatch = true;
				short		  myMarkPos;
				short		  myFileNamePos;

				mySrcCharNum = 0;
				myMarkLen = UTIL_StrLen(gB64CDISPMark);
				for (myDstCharNum = 0; myDstCharNum < myMarkLen; myDstCharNum++)
					{
					while (UTIL_IsSpaceChar(mySrcChar = gUUCD_DECODE_TextLineChars[mySrcCharNum]))
						{
						mySrcCharNum++;
						};
					if (UTIL_ToUpper(mySrcChar) != gB64CDISPMark[myDstCharNum])
						{
						myLineMatch = false;
						break;
						};
					mySrcCharNum++;
					};

				if (myLineMatch)
					{
					myMarkPos = UTIL_StrPos(gUUCD_DECODE_TextLineChars,
						gB64CFNAMMark, true, true);

					if (myMarkPos >= 0)
						{
						myFileNamePos = myMarkPos + UTIL_StrLen(gB64CFNAMMark);
						while (gUUCD_DECODE_TextLineChars[myFileNamePos] != '"') myFileNamePos++;
						mySrcCharNum = ++myFileNamePos;
						myDstCharNum = 0;
						while ((mySrcChar = gUUCD_DECODE_TextLineChars[mySrcCharNum]) != '\0')
							{
							if (mySrcChar == '"') break;
							if (!UTIL_IsFileNameChar(mySrcChar)) break;
							gUUCD_DECODE_FileName[++myDstCharNum] = mySrcChar;
							mySrcCharNum++;
							};
						gUUCD_DECODE_FileName[0] = (char)myDstCharNum;

						myLineDone = true;
						gUUCD_DECODE_LastLineType = LINETYPE_Base64;
						};
					};
*/
				};

			/*----------------------------------------------------------*/
			/*					B64 line ?					*/
			/*----------------------------------------------------------*/

			if (!myLineDone)
				{
				short		  myB64LineLen = 0;
				short		  myLastB64LineLen = 0;

				myB64LineType = B64LINETYPE_None;
				myLastB64LineType = B64LINETYPE_None;

				if ((myB64LineLen = UUCD_IsB64Line(gUUCD_DECODE_TextLineChars,
					&gUUCD_DECODE_TextLineLen)) > 0)
					{
					myB64LineType = B64LINETYPE_Full;

					if ((myLastB64LineLen = UUCD_IsB64Line(gUUCD_DECODE_LastTextLineChars,
						&gUUCD_DECODE_LastTextLineLen)) > 0)
						{
						myLastB64LineType = B64LINETYPE_Full;

						if (myB64LineLen < myLastB64LineLen) myB64LineType = B64LINETYPE_Last;
						};

					if (myB64LineType == B64LINETYPE_Full)
						if (myB64LineLen < 60) myB64LineType = B64LINETYPE_None;
					};

				if (myB64LineType != B64LINETYPE_None)
					{
					myLineDone = true;

					if (gUUCD_DECODE_SegmentNum >= 0)
						{
						if (myLastB64LineType == B64LINETYPE_None)
							{
							if (UUCD_GetSegmentDataSize(gUUCD_DECODE_SegmentNum) != 0L)
								{
								UUCD_DrawSegment(gUUCD_DECODE_SegmentNum, NULL);
								UUCD_DECODE_NewSegment(kUUCD_SEGTYPE_B64);
								};
							};
						if (UUCD_GetSegmentType(gUUCD_DECODE_SegmentNum) != kUUCD_SEGTYPE_B64)
							{
							UUCD_DrawSegment(gUUCD_DECODE_SegmentNum, NULL);
							UUCD_DECODE_NewSegment(kUUCD_SEGTYPE_B64);
							};
						}
					else UUCD_DECODE_NewSegment(kUUCD_SEGTYPE_B64);
	
					if (!UUCD_CheckMemory(0L)) return memFullErr;
					UUCD_AddB64ToSegment(gUUCD_DECODE_SegmentNum,
						gUUCD_DECODE_TextLineChars,
						gUUCD_DECODE_TextLineLen,
						gUUCD_DECODE_FileLineNum);

					if (myB64LineType == B64LINETYPE_Full)
						{
						if (myLastB64LineType == B64LINETYPE_None)
							{
							if (gUUCD_DECODE_B64HeadLineSeen)
								{
								StringPtr		  myFileNamePtr = NULL;
								Str63		  myTempStr63;

								UUCD_SetSegmentState(gUUCD_DECODE_SegmentNum, kUUCD_SEGSTATE_Begin);
								UUCD_DrawSegment(gUUCD_DECODE_SegmentNum, NULL);

								if (!UUCD_GetSegmentDstFileName(gUUCD_DECODE_SegmentNum, myTempStr63))
									{
									myFileNamePtr = UTIL_ExtFileNameSearch(gUUCD_DECODE_LastSubject, "\p.gif");
									if (myFileNamePtr == NULL)
										myFileNamePtr = UTIL_ExtFileNameSearch(gUUCD_DECODE_LastSubject, "\p.jpg");
									if (myFileNamePtr != NULL)
										UUCD_SetSegmentDstFileName(gUUCD_DECODE_SegmentNum, myFileNamePtr);
									};

								gUUCD_DECODE_B64HeadLineSeen = false;
								};
							};
						};

					if (myB64LineType == B64LINETYPE_Last)
						{
						mySegmentState = UUCD_GetSegmentState(gUUCD_DECODE_SegmentNum);
						mySegmentState = (mySegmentState == kUUCD_SEGSTATE_Begin)
							? kUUCD_SEGSTATE_Complete : kUUCD_SEGSTATE_End;

						UUCD_SetSegmentState(gUUCD_DECODE_SegmentNum, mySegmentState);
						UUCD_DrawSegment(gUUCD_DECODE_SegmentNum, NULL);
						};

					gUUCD_DECODE_LastLineType = LINETYPE_Base64;
					};

				};

			/*----------------------------------------------------------*/
			/*					All lines						*/
			/*----------------------------------------------------------*/

			if (!myLineDone)
				{
				gUUCD_DECODE_LastLineType = LINETYPE_None;
				};

			BlockMove(gUUCD_DECODE_LastTextLineChars,
				gUUCD_DECODE_LastLastTextLineChars, kUUCDParseLineSize);
			gUUCD_DECODE_LastLastTextLineLen = gUUCD_DECODE_LastTextLineLen;
			BlockMove(gUUCD_DECODE_TextLineChars,
				gUUCD_DECODE_LastTextLineChars, kUUCDParseLineSize);
			gUUCD_DECODE_LastTextLineLen = gUUCD_DECODE_TextLineLen;
			gUUCD_DECODE_TextLineLen = 0;
			continue;
			};

		if (gUUCD_DECODE_TextChar < ' ') continue;
		if (gUUCD_DECODE_TextChar >= (char)0x7F) continue;
		if (gUUCD_DECODE_TextLineLen >= 250) continue; // 011008: 80 -> 250
		gUUCD_DECODE_TextLineChars[gUUCD_DECODE_TextLineLen++] = gUUCD_DECODE_TextChar;
		};

	if (gUUCD_DECODE_TextCharNum >= gUUCD_DECODE_SrcFileSize)
		gUUCD_DECODE_State = kUUCD_DECODE_STATE_Done;

	return myOSErr;

	} /* UUCD_DECODE_Body */

/*------------------------------------------------------------------------------
*/

void UUCD_DECODE_End (OSErr theOSErr)

	{

	if (gUUCD_DECODE_SegmentNum != -1) UUCD_BlobSegment(gUUCD_DECODE_SegmentNum);

	if (gUUCD_DECODE_TextFileBuffer != NULL)
		{
		DisposePtr((Ptr)gUUCD_DECODE_TextFileBuffer);
		gUUCD_DECODE_TextFileBuffer = NULL;
		};
	if (gUUCD_DECODE_TextFileRefNum != -1)
		{
		FSClose(gUUCD_DECODE_TextFileRefNum);
		gUUCD_DECODE_TextFileRefNum = -1;
		};

	UUCD_DrawProgress(0L, 0L);
	UUCD_DrawMemory();

	UUCD_CheckSegmentStates();
	UUCD_DrawSegments();

	if (theOSErr != noErr)
	if (theOSErr != memFullErr) UUCD_OSErrAlert(theOSErr);

	EnableItem(gApplicationMenus[MENU_File], 0);
	DrawMenuBar();

	gUUCD_DECODE_State = kUUCD_DECODE_STATE_Idle;

	UUCD_PlayReceivedSound();

	UTIL_InstallNotification();

	} /* UUCD_DECODE_End */

/*------------------------------------------------------------------------------
//
//	UUCD_IsB64Line
//
*/

short UUCD_IsB64Line (char *theLineCharsPtr, short *theLineLenPtr)

	{
	short			  myLineCharNum;
	uchar			  myLineChar;
	short			  myCutLineLen = *theLineLenPtr;

	while ((myCutLineLen > 0) && (theLineCharsPtr[myCutLineLen - 1] == ' ')) myCutLineLen--;
	while ((myCutLineLen > 0) && (theLineCharsPtr[myCutLineLen - 1] == '=')) myCutLineLen--;

	for (myLineCharNum = 0; myLineCharNum < myCutLineLen; myLineCharNum++)
		{
		myLineChar = theLineCharsPtr[myLineCharNum];
		if (!gIsB64Char[myLineChar]) return 0;
		};

	return myCutLineLen;


/*

	short			  myLineCharNum;
	uchar			  myLineChar;
	short			  myCutLineLen;

	for (myCutLineLen = *theLineLenPtr; myCutLineLen > 0; myCutLineLen--)
		{
		if (theLineCharsPtr[myCutLineLen - 1] != ' ') break;
		};

	if (myCutLineLen < 2) return B64LINETYPE_None;

	---- B64 Last Line ? ----

	if (myCutLineLen < 60)
		{
		short		  myNbOfEndingEquals = 0;

		if (theLineCharsPtr[myCutLineLen - 1] == '=')
			{
			myNbOfEndingEquals++;

			if (theLineCharsPtr[myCutLineLen - 2] == '=')
				{
				myNbOfEndingEquals++;
				};
			};

		for (myLineCharNum = 0; myLineCharNum < (myCutLineLen - myNbOfEndingEquals); myLineCharNum++)
			{
			myLineChar = theLineCharsPtr[myLineCharNum];
			if (!gIsB64Char[myLineChar]) return B64LINETYPE_None;
			};

		return B64LINETYPE_Last;
		};

	---- B64 Full Line ? ----

	if (myCutLineLen != 60) return B64LINETYPE_None;

	for (myLineCharNum = 0; myLineCharNum < myCutLineLen; myLineCharNum++)
		{
		myLineChar = theLineCharsPtr[myLineCharNum];
		if (!gIsB64Char[myLineChar]) return B64LINETYPE_None;
		};

	return B64LINETYPE_Full;

*/

	} /* UUCD_IsB64Line */

/*------------------------------------------------------------------------------
//
//	UUCD_IsHQXLine
//
*/

HQXLineType UUCD_IsHQXLine (char *theLineCharsPtr, short *theLineLenPtr)

	{
	short			  myLineCharNum;
	uchar			  myLineChar;
	short			  myCutLineLen;

	for (myCutLineLen = *theLineLenPtr; myCutLineLen > 0; myCutLineLen--)
		{
		if (theLineCharsPtr[myCutLineLen - 1] != ' ') break;
		};

	if (myCutLineLen < 2) return HQXLINETYPE_None;

	/*---- HQX Last Line ? ----*/

	if (theLineCharsPtr[myCutLineLen - 1] == ':')
		{
		if (myCutLineLen > 65) return HQXLINETYPE_None;
		if (myCutLineLen < 2) return HQXLINETYPE_None;

		for (myLineCharNum = 0; myLineCharNum < (myCutLineLen - 1); myLineCharNum++)
			{
			myLineChar = theLineCharsPtr[myLineCharNum];
			if (!gIsHQXChar[myLineChar]) return HQXLINETYPE_None;
			};

		return HQXLINETYPE_Last;
		};

	/*---- HQX First Line ? ----*/

	if (theLineCharsPtr[0] == ':')
		{
		if (myCutLineLen != 64) return HQXLINETYPE_None;

		for (myLineCharNum = 1; myLineCharNum < myCutLineLen; myLineCharNum++)
			{
			myLineChar = theLineCharsPtr[myLineCharNum];
			if (!gIsHQXChar[myLineChar]) return HQXLINETYPE_None;
			};

		return HQXLINETYPE_First;
		};

	/*---- HQX Full Line ? ----*/

	if (myCutLineLen != 64) return HQXLINETYPE_None;

	for (myLineCharNum = 0; myLineCharNum < myCutLineLen; myLineCharNum++)
		{
		myLineChar = theLineCharsPtr[myLineCharNum];
		if (!gIsHQXChar[myLineChar]) return HQXLINETYPE_None;
		};

	return HQXLINETYPE_Full;

	} /* UUCD_IsHQXFullLine */

/*------------------------------------------------------------------------------
//
//	UUCD_IsUUFullLine
//
*/

Boolean UUCD_IsUUFullLine (char *theLineCharsPtr, short *theLineLenPtr)

	{
	short			  myCharNum;
	uchar			  myLineChar;
	uchar			  myLineCharMin = (uchar)0xFF;
	uchar			  myLineCharMax = (uchar)0x00;

	if (*theLineCharsPtr != 'M') return false;
	if (*theLineLenPtr < 2) return false;

	if (*theLineLenPtr > 61)
		{
		for (myCharNum = *theLineLenPtr - 1; myCharNum >= 61; myCharNum--)
			{
			if (theLineCharsPtr[myCharNum] != ' ')
				if (myCharNum != 61) /* we only allow one extra char */
					return false;
			};
		*theLineLenPtr = 61;
		};

	for (myCharNum = 1; myCharNum < *theLineLenPtr; myCharNum++)
		{
		myLineChar = theLineCharsPtr[myCharNum];
		if (myLineChar == ' ') myLineChar = '`'; // NEW 961114
		if (myLineChar < myLineCharMin) myLineCharMin = myLineChar;
		if (myLineChar > myLineCharMax) myLineCharMax = myLineChar;
		};

	if (*theLineLenPtr < 61)
		{
		if (myLineCharMax < (uchar)96) // '`'
			{
			for (myCharNum = *theLineLenPtr; myCharNum < 61; myCharNum++)
				{
				theLineCharsPtr[myCharNum] = ' ';
				};
			*theLineLenPtr = 61;
			};
		};

	if ((myLineCharMax - myLineCharMin) < (uchar)64) return true;

	/*---- No Sir, it's not an UUFullLine ----*/

	return false;

	} /* UUCD_IsUUFullLine */

/*------------------------------------------------------------------------------
//
//	UUCD_PlayReceivedSound
//
*/

void UUCD_PlayReceivedSound (void)

	{

	if (!PREF_IsSoundEnabled()) return;

	if (gUUCDReceivedSoundHandle == NULL) return;
	SndPlay(NULL, (SndListHandle)gUUCDReceivedSoundHandle, false);

	/* UTIL_PlaySndFile("\preceived"); */

	} /* UUCD_PlayReceivedSound */

/*------------------------------------------------------------------------------
//
//	UUCD_DbgWrite
//
*/

void UUCD_DbgWrite (StringPtr theStringPtr)

	{
	OSErr			  myOSErr;
	FSSpec			  myDbgFileFSSpec;
	short			  myDbgFileRefNum;
	long				  myWriteCount;

	myOSErr = FSMakeFSSpec(gApplicationVRefNum, gApplicationDirID, "\puucd.dbg", &myDbgFileFSSpec);

	if (myOSErr == fnfErr) myOSErr = FSpCreate(&myDbgFileFSSpec, 'MPS ', 'TEXT', 0);
	if (myOSErr != noErr) return;

	myOSErr = FSpOpenDF(&myDbgFileFSSpec, fsWrPerm, &myDbgFileRefNum);
	if (myOSErr == noErr)
		{
		if (SetFPos(myDbgFileRefNum, fsFromLEOF, 0L) == noErr)
			{
			myWriteCount = (long)theStringPtr[0];
			FSWrite(myDbgFileRefNum, &myWriteCount, &theStringPtr[1]);
			};
		FSClose(myDbgFileRefNum);
		};

	} /* UUCD_DbgWrite */

/*------------------------------------------------------------------------------
//
//	UUCD_GetUUCDWindowPtr
//
*/

WindowPtr UUCD_GetUUCDWindowPtr (void)

	{

	return gUUCDWindowPtr;

	} /* UUCD_GetUUCDWindowPtr */

/*------------------------------------------------------------------------------
//
//	That's All, Folks !...
//
*/
