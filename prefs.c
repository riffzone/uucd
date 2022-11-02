/*------------------------------------------------------------------------------
//
//	prefs.c
//
*/

#include "Errors.h"
#include "Files.h"
#include "Folders.h"
#include "Fonts.h"
#include "Lists.h"
#include "LowMem.h"
#include "Memory.h"
#include "OSUtils.h"
#include "QuickDraw.h"
#include "Resources.h"
#include "Sound.h"
#include "TextUtils.h"
#include "ToolUtils.h"

#include "uucd.h"
#include "util.h"

#include "prefs.h"

extern Boolean MAIN_DoUpdate (WindowPtr theWindowPtr);

/* Internet Config API */
#include "ICTypes.h"
#include "ICKeys.h"
#include "ICAPI.h"

/*------------------------------------------------------------------------------
//
//	Definitions
//
*/

#define kUUCDPrefsDialogID 300
#define kUUCDPrefsDlgDefSettingsItem 3
#define kUUCDPrefsDlgTESubjectTagItem 4
#define kUUCDPrefsDlgTETextCreatorItem 5
#define kUUCDPrefsDlgTEPermissionModeItem 6
#define kUUCDPrefsDlgTESegmentMaxLinesItem 7
#define kUUCDPrefsDlgUUDecodeTitleItem 8
#define kUUCDPrefsDlgUUEncodeTitleItem 9
#define kUUCDPrefsDlgSeparator1Item 10
#define kUUCDPrefsDlgSeparator2Item 11
#define kUUCDPrefsDlgNewLineCRItem 12
#define kUUCDPrefsDlgNewLineLFItem 13
#define kUUCDPrefsDlgNewLineCRLFItem 14
#define kUUCDPrefsUUP1RsrcType 'UUP1'
#define kUUCDPrefsUUP1RsrcID 300
#define kUUCDPrefsUUP2RsrcType 'UUP2'
#define kUUCDPrefsUUP2RsrcID 300
#define kUUCDPrefsUUCDWPosRsrcType 'WPOS'
#define kUUCDPrefsUUCDWPosRsrcID 300
#define kUUCDPrefsUUCDWSizRsrcType 'WSIZ'
#define kUUCDPrefsUUCDWSizRsrcID 300

#define kUUPFStringsListID 300
enum {
	UUPFSTR_PrefsFileName = 1,
	UUPFSTR_UUDecodePrefs,
	UUPFSTR_UUEncodePrefs,
	UUPFSTR_AutoTypePrefs
	};

/*------------------------------------------------------------------------------
//
//	Globales
//
*/

UUP1Hdl			  gUUCDUUP1Hdl = NULL;
UUP2Hdl			  gUUCDUUP2Hdl = NULL;
Point			  gUUCDWindowPos = {kUUCDDefaultWindowPosH, kUUCDDefaultWindowPosV};
Point			  gUUCDWindowSiz = {kUUCDDefaultNbOfVisibleSegments, kUUCDDefaultSegmentWidth};

/*------------------------------------------------------------------------------
//
//	PREF_GetUUP1Hdl / PREF_GetUUP2Hdl
//
*/

UUP1Hdl PREF_GetUUP1Hdl (void) { return gUUCDUUP1Hdl; }
UUP2Hdl PREF_GetUUP2Hdl (void) { return gUUCDUUP2Hdl; }

/*------------------------------------------------------------------------------
//
//	PREF_GetTextCreator
//
*/

OSType PREF_GetTextCreator (void)

	{
	UUP2Ptr			  myUUP2Ptr;
	OSType			  myTextCreator;

	if (gUUCDUUP2Hdl == NULL) return 'ttxt';

	HLock((Handle)gUUCDUUP2Hdl);
	myUUP2Ptr = *gUUCDUUP2Hdl;
	myTextCreator = myUUP2Ptr->UUP2_TextCreator;
	HUnlock((Handle)gUUCDUUP2Hdl);

	return myTextCreator;

	} /* PREF_GetTextCreator */

/*------------------------------------------------------------------------------
//
//	PREF_GetPermissionMode
//
*/

OSType PREF_GetPermissionMode (void)

	{
	UUP2Ptr			  myUUP2Ptr;
	OSType			  myPermissionMode;

	if (gUUCDUUP2Hdl == NULL) return '0700';

	HLock((Handle)gUUCDUUP2Hdl);
	myUUP2Ptr = *gUUCDUUP2Hdl;
	myPermissionMode = myUUP2Ptr->UUP2_PermissionMode;
	HUnlock((Handle)gUUCDUUP2Hdl);

	return myPermissionMode;

	} /* PREF_GetPermissionMode */

/*------------------------------------------------------------------------------
//
//	PREF_GetSegmentMaxLines
//
*/

ushort PREF_GetSegmentMaxLines (void)

	{
	UUP2Ptr			  myUUP2Ptr;
	ushort			  mySegmentMaxLines;

	if (gUUCDUUP2Hdl == NULL) return 0;

	HLock((Handle)gUUCDUUP2Hdl);
	myUUP2Ptr = *gUUCDUUP2Hdl;
	mySegmentMaxLines = myUUP2Ptr->UUP2_SegmentMaxLines;
	HUnlock((Handle)gUUCDUUP2Hdl);

	return mySegmentMaxLines;

	} /* PREF_GetSegmentMaxLines */

/*------------------------------------------------------------------------------
//
//	PREF_GetSubjectTag
//
*/

void PREF_GetSubjectTag (StringPtr theStringPtr)

	{
	UUP2Ptr			  myUUP2Ptr;
	Str31			  mySubjectTag = "\pSubject:";

	if (gUUCDUUP2Hdl == NULL) goto UUP2_GetSubjectTag_EXIT;

	HLock((Handle)gUUCDUUP2Hdl);
	myUUP2Ptr = *gUUCDUUP2Hdl;
	BlockMove(myUUP2Ptr->UUP2_SubjectTag, mySubjectTag, (long)sizeof(Str31));
	HUnlock((Handle)gUUCDUUP2Hdl);

UUP2_GetSubjectTag_EXIT:

	BlockMove(mySubjectTag, theStringPtr, (long)sizeof(Str31));

	} /* PREF_GetSubjectTag */

/*------------------------------------------------------------------------------
//
//	PREF_GetNewLineMode
//
*/

ushort PREF_GetNewLineMode (void)

	{
	UUP2Ptr			  myUUP2Ptr;
	ushort			  myNewLineMode;

	if (gUUCDUUP2Hdl == NULL) return 0;

	HLock((Handle)gUUCDUUP2Hdl);
	myUUP2Ptr = *gUUCDUUP2Hdl;
	myNewLineMode = myUUP2Ptr->UUP2_NewLineMode;
	HUnlock((Handle)gUUCDUUP2Hdl);

	return myNewLineMode;

	} /* PREF_GetNewLineMode */

/*------------------------------------------------------------------------------
//
//	PREF_TestFlag
//
*/

Boolean PREF_TestFlag (ushort theFlagNum)

	{
	UUP2Ptr			  myUUP2Ptr;
	Boolean			  myEnableFlag;

	if (gUUCDUUP2Hdl == NULL) return false;

	HLock((Handle)gUUCDUUP2Hdl);
	myUUP2Ptr = *gUUCDUUP2Hdl;
	myEnableFlag = BitTst(&myUUP2Ptr->UUP2_Flags, theFlagNum);
	HUnlock((Handle)gUUCDUUP2Hdl);

	return myEnableFlag;

	} /* PREF_TestFlag */

/*------------------------------------------------------------------------------
//
//	PREF_SetFlag
//
*/

void PREF_SetFlag (ushort theFlagNum, Boolean theValue)

	{
	UUP2Ptr			  myUUP2Ptr;

	if (gUUCDUUP2Hdl == NULL) return;

	HLock((Handle)gUUCDUUP2Hdl);
	myUUP2Ptr = *gUUCDUUP2Hdl;
	if (theValue)
		{
		BitSet(&myUUP2Ptr->UUP2_Flags, theFlagNum);
		}
	else BitClr(&myUUP2Ptr->UUP2_Flags, theFlagNum);
	HUnlock((Handle)gUUCDUUP2Hdl);

	} /* PREF_SetFlag */

/*------------------------------------------------------------------------------
//
//	PREF_IsSoundEnabled / PREF_SetSoundEnable
//
*/

Boolean PREF_IsSoundEnabled (void)

	{
	return PREF_TestFlag(kUUP2FLAG_Sound);
	}

void PREF_SetSoundEnable (Boolean theEnable)

	{
	PREF_SetFlag(kUUP2FLAG_Sound, theEnable);
	}

/*------------------------------------------------------------------------------
//
//	PREF_IsAutoOpenEnabled / PREF_SetAutoOpenEnable
//
*/

Boolean PREF_IsAutoOpenEnabled (void)

	{
	return PREF_TestFlag(kUUP2FLAG_AutoOpen);
	}

void PREF_SetAutoOpenEnable (Boolean theEnable)

	{
	PREF_SetFlag(kUUP2FLAG_AutoOpen, theEnable);
	}

/*------------------------------------------------------------------------------
//
//	PREF_IsAutoDeleteEnabled / PREF_SetAutoDeleteEnable
//
*/

Boolean PREF_IsAutoDeleteEnabled (void)

	{
	return PREF_TestFlag(kUUP2FLAG_AutoDelete);
	}

void PREF_SetAutoDeleteEnable (Boolean theEnable)

	{
	PREF_SetFlag(kUUP2FLAG_AutoDelete, theEnable);
	}

/*------------------------------------------------------------------------------
//
//	PREF_GetFileNameTypeCreator
//
*/

Boolean PREF_GetFileNameTypeCreator (StringPtr theFileName,
					OSType *theTypePtr, OSType *theCreatorPtr)

	{
	Boolean			  myTypeCreatorFound = false;
	ICInstance		  myICInstance;
	ICMapEntry		  myICMapEntry;
	UUP1Hdl			  myUUP1Hdl;

	*theTypePtr = '????';
	*theCreatorPtr = '????';

	if (ICStart(&myICInstance, kUUCDCreator) == noErr)
		{
		if (ICFindConfigFile(myICInstance, 0, NULL) == noErr)
			{
			if (ICMapFilename(myICInstance, theFileName, &myICMapEntry) == noErr)
				{
				*theTypePtr = myICMapEntry.file_type;
				*theCreatorPtr = myICMapEntry.file_creator;
				myTypeCreatorFound = true;
				};
			};
		ICStop(myICInstance);
		};

	if (!myTypeCreatorFound)
	if ((myUUP1Hdl = PREF_GetUUP1Hdl()) != NULL)
		{
		short				  myUUP1Count;
		short				  myUUP1Num;
		UUP1Ptr				  myUUP1Ptr;
		Str63				  myUpperFileName;

		BlockMove(theFileName, myUpperFileName, (long)sizeof(Str63));
		StripUpperText((Ptr)&myUpperFileName[1], myUpperFileName[0]);
		myUUP1Count = (short)(GetHandleSize((Handle)myUUP1Hdl) / (long)sizeof(UUP1));
		HLock((Handle)myUUP1Hdl);
		myUUP1Ptr = *myUUP1Hdl;
		for (myUUP1Num = 0; myUUP1Num < myUUP1Count; myUUP1Num++, myUUP1Ptr++)
			{
			StripUpperText((Ptr)&myUUP1Ptr->UUP1_Extension[1], myUUP1Ptr->UUP1_Extension[0]);
			if (UTIL_PStringEndsWith(myUpperFileName, myUUP1Ptr->UUP1_Extension))
				{
				*theTypePtr = myUUP1Ptr->UUP1_FileType;
				*theCreatorPtr = myUUP1Ptr->UUP1_FileCreator;
				myTypeCreatorFound = true;
				break;
				};
			};
		HUnlock((Handle)myUUP1Hdl);
		};

	return myTypeCreatorFound;

	} /* PREF_GetFileNameTypeCreator */

/*------------------------------------------------------------------------------
//
//	PREF_GetUUCDWindowPos
//
*/

void PREF_GetUUCDWindowPos (Point *theUUCDWindowPos)

	{
	GrafPtr		  myDeskPort;
	Rect			  myDeskRect;
	Rect			  myPinRect;
	long			  myPinResult;

	GetWMgrPort(&myDeskPort);
	myDeskRect = myDeskPort->portRect;
	myPinRect = myDeskRect;
	myPinRect.top += LMGetMBarHeight();
	InsetRect(&myPinRect, 20, 20);
	myPinResult = PinRect(&myPinRect, gUUCDWindowPos);
	gUUCDWindowPos.v = HiWord(myPinResult);
	gUUCDWindowPos.h = LoWord(myPinResult);

	if (theUUCDWindowPos != NULL) *theUUCDWindowPos = gUUCDWindowPos;

	} /* PREF_GetUUCDWindowPos */

/*------------------------------------------------------------------------------
//
//	PREF_SetUUCDWindowPos
//
*/

void PREF_SetUUCDWindowPos (Point theUUCDWindowPos)

	{

	gUUCDWindowPos = theUUCDWindowPos;

	} /* PREF_SetUUCDWindowPos */

/*------------------------------------------------------------------------------
//
//	PREF_GetUUCDWindowSiz
//
*/

void PREF_GetUUCDWindowSiz (Point *theUUCDWindowSiz)

	{

	if (theUUCDWindowSiz != NULL) *theUUCDWindowSiz = gUUCDWindowSiz;

	} /* PREF_GetUUCDWindowPos */

/*------------------------------------------------------------------------------
//
//	PREF_SetUUCDWindowSiz
//
*/

void PREF_SetUUCDWindowSiz (Point theUUCDWindowSiz)

	{

	gUUCDWindowSiz = theUUCDWindowSiz;

	} /* PREF_SetUUCDWindowSiz */

/*------------------------------------------------------------------------------
//
//	PREF_LoadPreferences
//
*/

void PREF_LoadPreferences (void)

	{
	short			  myPrefsVRefNum;
	long				  myPrefsDirID;
	FSSpec			  myPrefsFSSpec;
	short			  myPrefsRefNum = -1;
	Handle			  myTempHandle;

	if (FindFolder(kOnSystemDisk, kPreferencesFolderType, kCreateFolder,
		&myPrefsVRefNum, &myPrefsDirID) == noErr)
		{
		Str255		  myPrefsFileName;

		GetIndString(myPrefsFileName, kUUPFStringsListID, UUPFSTR_PrefsFileName);

		if (FSMakeFSSpec(myPrefsVRefNum, myPrefsDirID,
			myPrefsFileName, &myPrefsFSSpec) == noErr)
			{
			myPrefsRefNum = FSpOpenResFile(&myPrefsFSSpec, fsCurPerm);
			};
		};

	if (gUUCDUUP1Hdl != NULL)
		{
		DisposeHandle((Handle)gUUCDUUP1Hdl);
		gUUCDUUP1Hdl = NULL;
		};

	gUUCDUUP1Hdl = (UUP1Hdl)GetResource(kUUCDPrefsUUP1RsrcType, kUUCDPrefsUUP1RsrcID);
	if (gUUCDUUP1Hdl != NULL) DetachResource((Handle)gUUCDUUP1Hdl);

	if (gUUCDUUP2Hdl != NULL)
		{
		DisposeHandle((Handle)gUUCDUUP2Hdl);
		gUUCDUUP2Hdl = NULL;
		};

	gUUCDUUP2Hdl = (UUP2Hdl)GetResource(kUUCDPrefsUUP2RsrcType, kUUCDPrefsUUP2RsrcID);
	if (gUUCDUUP2Hdl != NULL)
		{
		long			  myUUP2HandleSize;
		long			  myRefHandleSize = (long)sizeof(UUP2);

		DetachResource((Handle)gUUCDUUP2Hdl);

		if ((myUUP2HandleSize = GetHandleSize((Handle)gUUCDUUP2Hdl)) < myRefHandleSize)
			{
			SetHandleSize((Handle)gUUCDUUP2Hdl, (long)sizeof(UUP2));

			HLock((Handle)gUUCDUUP2Hdl);

			myRefHandleSize -= (long)sizeof(ushort);
			if (myUUP2HandleSize <= myRefHandleSize) (*gUUCDUUP2Hdl)->UUP2_Flags = 0xE000;

			myRefHandleSize -= (long)sizeof(ushort);
			if (myUUP2HandleSize <= myRefHandleSize) (*gUUCDUUP2Hdl)->UUP2_NewLineMode = 0;

			HUnlock((Handle)gUUCDUUP2Hdl);
			};
		};

	myTempHandle = GetResource(kUUCDPrefsUUCDWPosRsrcType, kUUCDPrefsUUCDWPosRsrcID);
	if (myTempHandle != NULL)
		{
		HLock(myTempHandle);
		gUUCDWindowPos = **((Point **)myTempHandle);
		HUnlock(myTempHandle);

		ReleaseResource(myTempHandle);
		};

	myTempHandle = GetResource(kUUCDPrefsUUCDWSizRsrcType, kUUCDPrefsUUCDWSizRsrcID);
	if (myTempHandle != NULL)
		{
		HLock(myTempHandle);
		gUUCDWindowSiz = **((Point **)myTempHandle);
		HUnlock(myTempHandle);

		ReleaseResource(myTempHandle);
		};

	if (myPrefsRefNum != -1) CloseResFile(myPrefsRefNum);

	} /* PREF_LoadPreferences */

/*------------------------------------------------------------------------------
//
//	PREF_SavePreferences
//
*/

void PREF_SavePreferences (void)

	{
	OSErr			  myOSErr;
	short			  myPrefsVRefNum;
	long				  myPrefsDirID;
	FSSpec			  myPrefsFSSpec;
	short			  myPrefsRefNum = -1;
	Handle			  myUUP1ResourceHandle = NULL;
	Handle			  myUUP2ResourceHandle = NULL;
	Handle			  myUUCDWPosResourceHandle = NULL;
	Handle			  myUUCDWSizResourceHandle = NULL;

	if (FindFolder(kOnSystemDisk, kPreferencesFolderType, kCreateFolder,
		&myPrefsVRefNum, &myPrefsDirID) == noErr)
		{
		Str255		  myPrefsFileName;

		GetIndString(myPrefsFileName, kUUPFStringsListID, UUPFSTR_PrefsFileName);

		myOSErr = FSMakeFSSpec(myPrefsVRefNum, myPrefsDirID,
			myPrefsFileName, &myPrefsFSSpec);

		if (myOSErr == noErr) FSpDelete(&myPrefsFSSpec);
		}
	else
		{
		SysBeep(1);
		return;
		};

	FSpCreateResFile(&myPrefsFSSpec, kUUCDCreator, kUUCDPrefsFileType, 0);

	myPrefsRefNum = FSpOpenResFile(&myPrefsFSSpec, fsCurPerm);
	if (myPrefsRefNum == -1) goto UUCD_SavePreferences_ABORT;

	myUUP1ResourceHandle = (Handle)gUUCDUUP1Hdl;
	myOSErr = HandToHand(&myUUP1ResourceHandle);
	if (myOSErr == noErr)
		AddResource(myUUP1ResourceHandle, kUUCDPrefsUUP1RsrcType,
			kUUCDPrefsUUP1RsrcID, "\p");

	myUUP2ResourceHandle = (Handle)gUUCDUUP2Hdl;
	myOSErr = HandToHand(&myUUP2ResourceHandle);
	if (myOSErr == noErr)
		AddResource(myUUP2ResourceHandle, kUUCDPrefsUUP2RsrcType,
			kUUCDPrefsUUP2RsrcID, "\p");

	myUUCDWPosResourceHandle = NewHandle(sizeof(Point));
	if (myUUCDWPosResourceHandle != NULL)
		{
		HLock(myUUCDWPosResourceHandle);
		**((Point **)myUUCDWPosResourceHandle) = gUUCDWindowPos;
		HUnlock(myUUCDWPosResourceHandle);
		AddResource(myUUCDWPosResourceHandle, kUUCDPrefsUUCDWPosRsrcType,
			kUUCDPrefsUUCDWPosRsrcID, "\p");
		};

	myUUCDWSizResourceHandle = NewHandle(sizeof(Point));
	if (myUUCDWSizResourceHandle != NULL)
		{
		HLock(myUUCDWSizResourceHandle);
		**((Point **)myUUCDWSizResourceHandle) = gUUCDWindowSiz;
		HUnlock(myUUCDWSizResourceHandle);
		AddResource(myUUCDWSizResourceHandle, kUUCDPrefsUUCDWSizRsrcType,
			kUUCDPrefsUUCDWSizRsrcID, "\p");
		};

	CloseResFile(myPrefsRefNum);

	return;

UUCD_SavePreferences_ABORT:

	if (myPrefsRefNum != -1) CloseResFile(myPrefsRefNum);

	FSpDelete(&myPrefsFSSpec);

	SysBeep(1);
	SysBeep(1);

	} /* PREF_SavePreferences */

/*------------------------------------------------------------------------------
//
//	PREF_RAZPreferences
//
*/

void PREF_RAZPreferences (void)

	{
	short			  myPrefsVRefNum;
	long				  myPrefsDirID;
	FSSpec			  myPrefsFSSpec;

	if (FindFolder(kOnSystemDisk, kPreferencesFolderType, kCreateFolder,
		&myPrefsVRefNum, &myPrefsDirID) == noErr)
		{
		Str255		  myPrefsFileName;

		GetIndString(myPrefsFileName, kUUPFStringsListID, UUPFSTR_PrefsFileName);

		if (FSMakeFSSpec(myPrefsVRefNum, myPrefsDirID,
			myPrefsFileName, &myPrefsFSSpec) == noErr)
			{
			FSpDelete(&myPrefsFSSpec);
			};
		};

	PREF_LoadPreferences();

	} /* PREF_RAZPreferences */

/*------------------------------------------------------------------------------
//
//	UUCD_EditPreferences
//
*/

DialogPtr				  gUUCDPREFS_DialogPtr = NULL;

pascal void UUCD_EditPreferences_UserProc (WindowPtr theWindowPtr, short theItemNum);
pascal void UUCD_EditPreferences_UserProc (WindowPtr theWindowPtr, short theItemNum)

	{
	GrafPtr			  mySavePort;
	Rect				  myTempRect;
	short			  myItemType;
	Handle			  myItemHandle;
	Rect				  myItemRect;
	Str255			  myItemText;
	short			  myUUPFSTRNum = 0;
	UInt8			  myHiliteMode;

	GetPort(&mySavePort);
	SetPort(theWindowPtr);

	switch (theItemNum)
		{
	case kUUCDPrefsDlgUUDecodeTitleItem:
		if (myUUPFSTRNum == 0) myUUPFSTRNum = UUPFSTR_UUDecodePrefs;
	case kUUCDPrefsDlgUUEncodeTitleItem:
		if (myUUPFSTRNum == 0) myUUPFSTRNum = UUPFSTR_UUEncodePrefs;

		GetDialogItem((DialogPtr)theWindowPtr, theItemNum, &myItemType, &myItemHandle, &myItemRect);
		GetIndString(myItemText, kUUPFStringsListID, myUUPFSTRNum);
		TETextBox((Ptr)&myItemText[1], (long)myItemText[0], &myItemRect, teJustLeft);
		myTempRect = myItemRect;
		myTempRect.right = myTempRect.left + StringWidth(myItemText);
		myHiliteMode = LMGetHiliteMode();
		BitClr((Ptr)&myHiliteMode, pHiliteBit);
		LMSetHiliteMode(myHiliteMode);
		InvertRect(&myTempRect);
		MoveTo(myItemRect.left, myItemRect.bottom - 1);
		LineTo(myItemRect.right - 1, myItemRect.bottom - 1);
		break;

	case kUUCDPrefsDlgSeparator1Item:
	case kUUCDPrefsDlgSeparator2Item:
		GetDialogItem((DialogPtr)theWindowPtr, theItemNum, &myItemType, &myItemHandle, &myItemRect);
		myHiliteMode = LMGetHiliteMode();
		BitClr((Ptr)&myHiliteMode, pHiliteBit);
		LMSetHiliteMode(myHiliteMode);
		InvertRect(&myItemRect);
		FrameRect(&myItemRect);
		break;
		};

	SetPort(mySavePort);

	} /* UUCD_EditPreferences_UserProc */

pascal Boolean UUCD_EditPreferences_FilterProc (DialogPtr theDialogPtr, EventRecord *theEventPtr, short *theItemHitPtr);
pascal Boolean UUCD_EditPreferences_FilterProc (DialogPtr theDialogPtr, EventRecord *theEventPtr, short *theItemHitPtr)

	{
	Boolean			  myFilterResult = false;
	ModalFilterUPP		  myStandardModalFilterUPP;
	GrafPtr			  mySavePort;

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

	} /* UUCD_EditPreferences_FilterProc */

void UUCD_EditPreferences (void)

	{
	Boolean			  myDialogDone = false;
	Boolean			  myPrefsLoaded = false;
	short			  myItemHit = 0;
	short			  myItemType;
	Handle			  myItemHandle;
	Rect				  myItemRect;
	UUP2Ptr			  myUUP2Ptr;
	Str255			  myTempStr255;
	long				  myTempLong;
	short			  myTempShort;
	short			  myNewLineModeItemSelected;

	gUUCDPREFS_DialogPtr = NULL;

	/*---- Ouverture du Dialogue ----*/

	gUUCDPREFS_DialogPtr = GetNewDialog(kUUCDPrefsDialogID, NULL, (WindowPtr)-1L);
	if (gUUCDPREFS_DialogPtr == NULL) goto UUCD_EditPreferences_EXIT;

/*
	GrafPtr			  mySavePort;
	FontInfo			  myFontInfo;

	GetPort(&mySavePort);
	SetPort(gUUCDPREFS_DialogPtr);
	TextFont(geneva);
	TextSize(9);
	GetFontInfo(&myFontInfo);
	(**(((DialogPeek)gUUCDPREFS_DialogPtr)->textH)).txFont = geneva;
	(**(((DialogPeek)gUUCDPREFS_DialogPtr)->textH)).txSize = 9;
	(**(((DialogPeek)gUUCDPREFS_DialogPtr)->textH)).lineHeight = myFontInfo.ascent + myFontInfo.descent + myFontInfo.leading;
	(**(((DialogPeek)gUUCDPREFS_DialogPtr)->textH)).fontAscent = myFontInfo.ascent;
	SetPort(mySavePort);
*/

	SetDialogDefaultItem(gUUCDPREFS_DialogPtr, ok);
	SetDialogCancelItem(gUUCDPREFS_DialogPtr, cancel);
	SetDialogTracksCursor(gUUCDPREFS_DialogPtr, true);

	GetDialogItem(gUUCDPREFS_DialogPtr, kUUCDPrefsDlgUUDecodeTitleItem, &myItemType, &myItemHandle, &myItemRect);
	SetDialogItem(gUUCDPREFS_DialogPtr, kUUCDPrefsDlgUUDecodeTitleItem, myItemType,
		(Handle)NewUserItemProc(UUCD_EditPreferences_UserProc), &myItemRect);

	GetDialogItem(gUUCDPREFS_DialogPtr, kUUCDPrefsDlgUUEncodeTitleItem, &myItemType, &myItemHandle, &myItemRect);
	SetDialogItem(gUUCDPREFS_DialogPtr, kUUCDPrefsDlgUUEncodeTitleItem, myItemType,
		(Handle)NewUserItemProc(UUCD_EditPreferences_UserProc), &myItemRect);

	GetDialogItem(gUUCDPREFS_DialogPtr, kUUCDPrefsDlgSeparator1Item, &myItemType, &myItemHandle, &myItemRect);
	SetDialogItem(gUUCDPREFS_DialogPtr, kUUCDPrefsDlgSeparator1Item, myItemType,
		(Handle)NewUserItemProc(UUCD_EditPreferences_UserProc), &myItemRect);

	GetDialogItem(gUUCDPREFS_DialogPtr, kUUCDPrefsDlgSeparator2Item, &myItemType, &myItemHandle, &myItemRect);
	SetDialogItem(gUUCDPREFS_DialogPtr, kUUCDPrefsDlgSeparator2Item, myItemType,
		(Handle)NewUserItemProc(UUCD_EditPreferences_UserProc), &myItemRect);

	while (!myDialogDone)
		{
		if (!myPrefsLoaded)
			{
			/*---- UUP2 ----*/

			HLock((Handle)gUUCDUUP2Hdl);
			myUUP2Ptr = *gUUCDUUP2Hdl;

			myTempStr255[0] = (uchar)4;
			BlockMove(&myUUP2Ptr->UUP2_TextCreator, &myTempStr255[1], 4L);
			GetDialogItem(gUUCDPREFS_DialogPtr, kUUCDPrefsDlgTETextCreatorItem, &myItemType, &myItemHandle, &myItemRect);
			SetDialogItemText(myItemHandle, myTempStr255);

			BlockMove(&myUUP2Ptr->UUP2_PermissionMode, myTempStr255, 4L);
			myTempStr255[0] = (uchar)3;
			GetDialogItem(gUUCDPREFS_DialogPtr, kUUCDPrefsDlgTEPermissionModeItem, &myItemType, &myItemHandle, &myItemRect);
			SetDialogItemText(myItemHandle, myTempStr255);

			NumToString((long)myUUP2Ptr->UUP2_SegmentMaxLines, myTempStr255);
			GetDialogItem(gUUCDPREFS_DialogPtr, kUUCDPrefsDlgTESegmentMaxLinesItem, &myItemType, &myItemHandle, &myItemRect);
			SetDialogItemText(myItemHandle, myTempStr255);

			BlockMove(myUUP2Ptr->UUP2_SubjectTag, myTempStr255, 32L);
			GetDialogItem(gUUCDPREFS_DialogPtr, kUUCDPrefsDlgTESubjectTagItem, &myItemType, &myItemHandle, &myItemRect);
			SetDialogItemText(myItemHandle, myTempStr255);

			GetDialogItem(gUUCDPREFS_DialogPtr, kUUCDPrefsDlgNewLineCRItem, &myItemType, &myItemHandle, &myItemRect);
			SetControlValue((ControlHandle)myItemHandle, 0);
			GetDialogItem(gUUCDPREFS_DialogPtr, kUUCDPrefsDlgNewLineLFItem, &myItemType, &myItemHandle, &myItemRect);
			SetControlValue((ControlHandle)myItemHandle, 0);
			GetDialogItem(gUUCDPREFS_DialogPtr, kUUCDPrefsDlgNewLineCRLFItem, &myItemType, &myItemHandle, &myItemRect);
			SetControlValue((ControlHandle)myItemHandle, 0);
			switch (myUUP2Ptr->UUP2_NewLineMode)
				{
			case kUUCDPrefsNewLineCR: myNewLineModeItemSelected = kUUCDPrefsDlgNewLineCRItem; break;
			case kUUCDPrefsNewLineLF: myNewLineModeItemSelected = kUUCDPrefsDlgNewLineLFItem; break;
			case kUUCDPrefsNewLineCRLF: myNewLineModeItemSelected = kUUCDPrefsDlgNewLineCRLFItem; break;
			default: myNewLineModeItemSelected = 0; break;
				};
			GetDialogItem(gUUCDPREFS_DialogPtr, myNewLineModeItemSelected, &myItemType, &myItemHandle, &myItemRect);
			SetControlValue((ControlHandle)myItemHandle, 1);

			HUnlock((Handle)gUUCDUUP2Hdl);

			SelectDialogItemText(gUUCDPREFS_DialogPtr, kUUCDPrefsDlgTETextCreatorItem, 0, 32000);

			/*---- done ----*/

			myPrefsLoaded = true;
			};

		ModalDialog(NewModalFilterProc(UUCD_EditPreferences_FilterProc), &myItemHit);

		switch (myItemHit)
			{
		case ok:

			/*---- UUP2 ----*/

			HLock((Handle)gUUCDUUP2Hdl);
			myUUP2Ptr = *gUUCDUUP2Hdl;

			GetDialogItem(gUUCDPREFS_DialogPtr, kUUCDPrefsDlgTETextCreatorItem, &myItemType, &myItemHandle, &myItemRect);
			GetDialogItemText(myItemHandle, myTempStr255);
			((char *)&myUUP2Ptr->UUP2_TextCreator)[0] = myTempStr255[0] >= 1 ? myTempStr255[1] : ' ';
			((char *)&myUUP2Ptr->UUP2_TextCreator)[1] = myTempStr255[0] >= 2 ? myTempStr255[2] : ' ';
			((char *)&myUUP2Ptr->UUP2_TextCreator)[2] = myTempStr255[0] >= 3 ? myTempStr255[3] : ' ';
			((char *)&myUUP2Ptr->UUP2_TextCreator)[3] = myTempStr255[0] >= 4 ? myTempStr255[4] : ' ';

			GetDialogItem(gUUCDPREFS_DialogPtr, kUUCDPrefsDlgTEPermissionModeItem, &myItemType, &myItemHandle, &myItemRect);
			GetDialogItemText(myItemHandle, myTempStr255);
			((char *)&myUUP2Ptr->UUP2_PermissionMode)[0] = '0';
			((char *)&myUUP2Ptr->UUP2_PermissionMode)[1] = myTempStr255[0] >= 1 ? myTempStr255[1] : '0';
			((char *)&myUUP2Ptr->UUP2_PermissionMode)[2] = myTempStr255[0] >= 2 ? myTempStr255[2] : '0';
			((char *)&myUUP2Ptr->UUP2_PermissionMode)[3] = myTempStr255[0] >= 3 ? myTempStr255[3] : '0';

			GetDialogItem(gUUCDPREFS_DialogPtr, kUUCDPrefsDlgTESegmentMaxLinesItem, &myItemType, &myItemHandle, &myItemRect);
			GetDialogItemText(myItemHandle, myTempStr255);
			StringToNum(myTempStr255, &myTempLong);
			if (myTempLong < 0L) myTempLong = 0L;
			if (myTempLong > 32767L) myTempLong = 32767L;
			myUUP2Ptr->UUP2_SegmentMaxLines = (short)myTempLong;

			GetDialogItem(gUUCDPREFS_DialogPtr, kUUCDPrefsDlgTESubjectTagItem, &myItemType, &myItemHandle, &myItemRect);
			GetDialogItemText(myItemHandle, myTempStr255);
			if ((uchar)myTempStr255[0] > 31) myTempStr255[0] = (char)31;
			for (myTempShort = 0; myTempShort < 32; myTempShort++) myUUP2Ptr->UUP2_SubjectTag[myTempShort] = '\0';
			BlockMove(myTempStr255, myUUP2Ptr->UUP2_SubjectTag, (long)myTempStr255[0] + 1L);

			switch (myNewLineModeItemSelected)
				{
			case kUUCDPrefsDlgNewLineCRItem: myUUP2Ptr->UUP2_NewLineMode = kUUCDPrefsNewLineCR; break;
			case kUUCDPrefsDlgNewLineLFItem: myUUP2Ptr->UUP2_NewLineMode = kUUCDPrefsNewLineLF; break;
			case kUUCDPrefsDlgNewLineCRLFItem: myUUP2Ptr->UUP2_NewLineMode = kUUCDPrefsNewLineCRLF; break;
			default: myUUP2Ptr->UUP2_NewLineMode = kUUCDPrefsNewLineCR; break;
				};

			HUnlock((Handle)gUUCDUUP2Hdl);

			/*---- Enregistrement ----*/

			PREF_SavePreferences();

			/* UUCD_SetCursor(UUCDCURS_Arrow); */

		case cancel:
			myDialogDone = true;
			break;

		case kUUCDPrefsDlgDefSettingsItem:

			PREF_RAZPreferences();

			myPrefsLoaded = false;

			break;

		case kUUCDPrefsDlgNewLineCRItem:
		case kUUCDPrefsDlgNewLineLFItem:
		case kUUCDPrefsDlgNewLineCRLFItem:

			if (myNewLineModeItemSelected != 0)
				{
				GetDialogItem(gUUCDPREFS_DialogPtr, myNewLineModeItemSelected, &myItemType, &myItemHandle, &myItemRect);
				SetControlValue((ControlHandle)myItemHandle, 0);
				};

			myNewLineModeItemSelected = myItemHit;

			GetDialogItem(gUUCDPREFS_DialogPtr, myNewLineModeItemSelected, &myItemType, &myItemHandle, &myItemRect);
			SetControlValue((ControlHandle)myItemHandle, 1);

			break;
			};
		};

	/*---- CleanUp ----*/

UUCD_EditPreferences_EXIT:

	if (gUUCDPREFS_DialogPtr != NULL)
		{
		DisposeDialog(gUUCDPREFS_DialogPtr);
		gUUCDPREFS_DialogPtr = NULL;
		};

	} /* UUCD_EditPreferences */

/*------------------------------------------------------------------------------
//
//	That's All, Folks !...
//
*/
