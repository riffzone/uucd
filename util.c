/*------------------------------------------------------------------------------
//
//	util.c
//
*/

#include "ctype.h"
#include "stdio.h"
#include "stdarg.h"

#include "Icons.h"
#include "LowMem.h"
#include "Memory.h"
#include "QuickDraw.h"
#include "Resources.h"
#include "Sound.h"
#include "TextUtils.h"
#include "Types.h"

#include "util.h"

/*------------------------------------------------------------------------------
//
//	CICN_IsCQDPresent
//
*/

Boolean CICN_IsCQDPresent (void)

	{
	SysEnvRec			  mySysEnvRec;

	SysEnvirons(1, &mySysEnvRec);

	return (mySysEnvRec.hasColorQD != 0);

	} /* CICN_IsCQDPresent */

/*------------------------------------------------------------------------------
//
//	CICN_GetIcon
//
*/

CICNHandle CICN_GetIcon (short theIconID)

	{
	CICNHandle		  myCICNHandle = NULL;

	if (CICN_IsCQDPresent())
		{
		myCICNHandle = (CICNHandle)GetCIcon(theIconID);
		}
	else
		{
		myCICNHandle = (CICNHandle)GetResource('cicn', theIconID);
		if (myCICNHandle != NULL) DetachResource((Handle)myCICNHandle);
		};

	return myCICNHandle;

	} /* CICN_GetIcon */

/*------------------------------------------------------------------------------
//
//	CICN_DisposIcon
//
*/

void CICN_DisposIcon (CICNHandle theCICNHandle)

	{

	if (CICN_IsCQDPresent())
		{
		DisposeCIcon((CIconHandle)theCICNHandle);
		}
	else DisposeHandle((Handle)theCICNHandle);

	} /* CICN_DisposIcon */

/*------------------------------------------------------------------------------
//
//	CICN_PlotIcon
//
*/

void CICN_PlotIcon (Rect *theRectPtr, CICNHandle theCICNHandle)

	{

	if (theCICNHandle == NULL) return;

	if (CICN_IsCQDPresent())
		{
		PlotCIcon(theRectPtr, (CIconHandle)theCICNHandle);
		}
	else
		{
		GrafPtr			  myCurPort;
		BitMap			  myMaskBitMap;
		BitMap			  myIconBitMap;
		Ptr				  myTempPtr;
		long				  myTempSize;
	
		HLock((Handle)theCICNHandle);
	
		myTempPtr = (Ptr)(*theCICNHandle);
		myTempPtr += 0x32; /* Offset for MaskBMap */
		BlockMove(myTempPtr, (Ptr)&myMaskBitMap, (long)sizeof(BitMap));
	
		myTempPtr = (Ptr)(*theCICNHandle);
		myTempPtr += 0x40; /* Offset for IconBMap */
		BlockMove(myTempPtr, (Ptr)&myIconBitMap, (long)sizeof(BitMap));
	
		myTempSize = (long)(myMaskBitMap.bounds.bottom - myMaskBitMap.bounds.top);
		myTempSize *= (long)myMaskBitMap.rowBytes;
		myTempPtr = (Ptr)(*theCICNHandle);
		myTempPtr += 0x52; /* Offset for MaskData */
		myTempPtr += myTempSize;
		myIconBitMap.baseAddr = myTempPtr;
	
		GetPort(&myCurPort);
		CopyBits (&myIconBitMap, &myCurPort->portBits,
				&myIconBitMap.bounds, theRectPtr,
				srcCopy, NULL);
	
		HUnlock((Handle)theCICNHandle);
		};

	} /* CICN_PlotIcon */

/*------------------------------------------------------------------------------
//
//	UTIL_GetAppVRefNum
//
*/

short UTIL_GetAppVRefNum (void)

	{
	OSErr			  myOSErr;
	short			  myAppRefNum;
	short			  myAppVRefNum;

	myAppRefNum = LMGetCurApRefNum();
	myOSErr = GetVRefNum(myAppRefNum, &myAppVRefNum);
	if (myOSErr != noErr) myAppVRefNum = 0;

	return myAppVRefNum;

	} /* UTIL_GetAppVRefNum */

/*------------------------------------------------------------------------------
//
//	UTIL_GetAppDirID
//
*/

long UTIL_GetAppDirID (void)

	{
	OSErr			  myOSErr;
	FCBPBRec			  myFCBPBRec;
	Str255			  myTempStr255;
	long				  myAppDirID = 0L;

	myFCBPBRec.ioCompletion = NULL;
	myFCBPBRec.ioNamePtr = myTempStr255;
	myFCBPBRec.ioFCBIndx = 0;
	myFCBPBRec.ioVRefNum = 0;
	myFCBPBRec.ioRefNum = LMGetCurApRefNum();
	myOSErr = PBGetFCBInfoSync(&myFCBPBRec);
	if (myOSErr == noErr) myAppDirID = myFCBPBRec.ioFCBParID;

	return myAppDirID;

	} /* UTIL_GetAppDirID */

/*------------------------------------------------------------------------------
//
//	UTIL_IsFrontApplication
//
*/

Boolean UTIL_IsFrontApplication (void)

	{
	ProcessSerialNumber		  myFrontPSN;
	ProcessSerialNumber		  myCurrentPSN;
	Boolean				  myProcessIsFront;

	if (GetFrontProcess(&myFrontPSN) != noErr) return false;

	myCurrentPSN.highLongOfPSN = 0;
	myCurrentPSN.lowLongOfPSN = kCurrentProcess;
	if (SameProcess(&myFrontPSN, &myCurrentPSN, &myProcessIsFront) != noErr) return false;

	return myProcessIsFront;

	} /* UTIL_IsFrontApplication */

/*------------------------------------------------------------------------------
//
//	UTIL_InstallNotification/UTIL_RemoveNotification
//
*/

Boolean			  gNotificationInstalled = false;
NMRec			  gNotificationRecord;

void UTIL_InstallNotification (void)

	{

	if (gNotificationInstalled) return;

	if (UTIL_IsFrontApplication()) return;

	if (GetIconSuite(&gNotificationRecord.nmIcon, 128, svAllSmallData) != noErr)
		gNotificationRecord.nmIcon = NULL;

	gNotificationRecord.qType = nmType;
	gNotificationRecord.nmMark = 1;
	gNotificationRecord.nmSound = NULL;
	gNotificationRecord.nmStr = NULL;
	gNotificationRecord.nmResp = NULL;
	gNotificationRecord.nmRefCon = 0L;
	
	if (NMInstall(&gNotificationRecord) != noErr) return;

	gNotificationInstalled = true;

	} /* UTIL_InstallNotification */

void UTIL_RemoveNotification (void)

	{

	if (!gNotificationInstalled) return;

	if (!UTIL_IsFrontApplication()) return;

	if (gNotificationRecord.nmIcon != NULL)
		{
		DisposeIconSuite(gNotificationRecord.nmIcon, true);
		gNotificationRecord.nmIcon = NULL;
		};

	NMRemove(&gNotificationRecord);
	gNotificationInstalled = false;

	} /* UTIL_RemoveNotification */

/*------------------------------------------------------------------------------
//
//	UTIL_BringApplicationToFront
//
*/

void UTIL_BringApplicationToFront (void)

	{
	ProcessSerialNumber		  myPSN;

	if (GetCurrentProcess(&myPSN) != noErr) return;

	SetFrontProcess(&myPSN);

	} /* UTIL_BringApplicationToFront */

/*------------------------------------------------------------------------------
//
//	UTIL_SetSFDir
//
*/

void UTIL_SetSFDir (short theVRefNum, long theDirID)

	{

	/*---- SFSaveDisk & CurDirStore / cf TN80 ----*/

	*((short *)0x0214) = (short)(- theVRefNum);
	*((long *)0x0398) = theDirID;

	} /* UTIL_SetSFDir */

/*------------------------------------------------------------------------------
//
//	UTIL_EqualFSSpec
//
*/

Boolean UTIL_EqualFSSpec (FSSpecPtr theFSSpecPtr1, FSSpecPtr theFSSpecPtr2)

	{

	if (theFSSpecPtr1->vRefNum != theFSSpecPtr2->vRefNum) return false;
	if (theFSSpecPtr1->parID != theFSSpecPtr2->parID) return false;
	return EqualString(theFSSpecPtr1->name, theFSSpecPtr2->name, true, true);

	} /* UTIL_EqualFSSpec */

/*------------------------------------------------------------------------------
//
//	UTIL_PlaySndFile
//
*/

void UTIL_PlaySndFile (StringPtr theFileName)

	{
	FSSpec			  mySndFileFSSpec;
	short			  mySndFileRefNum;
	Handle			  mySndResource;

	if (FSMakeFSSpec(UTIL_GetAppVRefNum(), UTIL_GetAppDirID(),
		theFileName, &mySndFileFSSpec) == noErr)
		{
		if ((mySndFileRefNum = FSpOpenResFile(&mySndFileFSSpec, fsRdPerm)) != -1)
			{
			if ((mySndResource = GetIndResource('snd ', 1)) != NULL)
				{
				DetachResource(mySndResource);
				SndPlay(NULL, (SndListHandle)mySndResource, false);
				DisposeHandle(mySndResource);
				};
			CloseResFile(mySndFileRefNum);
			};
		};

	} /* UTIL_PlaySndFile */

/*------------------------------------------------------------------------------
//
//	UTIL_BuildIndexedFileName
//
*/

Str63		  gUTIL_BuildIndexedFileName_Result;

StringPtr UTIL_BuildIndexedFileName (StringPtr theSrcFileName, unsigned short theIndex)

	{
	short		  mySrcFileNameLen;
	short		  myExtensionDotPos;
	short		  myExtensionLen;
	short		  mySrcCharNum;
	short		  myDstCharNum;
	Str31		  myIndexStr;
	short		  myIndexStrLen;
	short		  myMaxFileNameBodyLen;

	if (theIndex <= 1) return theSrcFileName;

	mySrcFileNameLen = theSrcFileName[0];

	NumToString((long)theIndex, myIndexStr);
	myIndexStrLen = myIndexStr[0];

	myExtensionLen = 0;
	myExtensionDotPos = 0;
	for (mySrcCharNum = mySrcFileNameLen; mySrcCharNum > 1; mySrcCharNum--)
		{
		myExtensionLen++;
		if (theSrcFileName[mySrcCharNum] == '.')
			{
			myExtensionDotPos = mySrcCharNum;
			break;
			};
		};

	myMaxFileNameBodyLen = 63;
	myMaxFileNameBodyLen -= myExtensionLen;
	myMaxFileNameBodyLen -= 1 + myIndexStrLen;

	if (myExtensionDotPos != 0)
		{
		for (mySrcCharNum = 1; mySrcCharNum < myExtensionDotPos; mySrcCharNum++)
			{
			gUTIL_BuildIndexedFileName_Result[mySrcCharNum] =
				theSrcFileName[mySrcCharNum];
			};
		gUTIL_BuildIndexedFileName_Result[0] = (char)(myExtensionDotPos - 1);
		}
	else UTIL_PStrCpy(gUTIL_BuildIndexedFileName_Result, theSrcFileName);

	if (gUTIL_BuildIndexedFileName_Result[0] > myMaxFileNameBodyLen)
		{
		gUTIL_BuildIndexedFileName_Result[0] = (char)myMaxFileNameBodyLen;
		};

	UTIL_PStrCat(gUTIL_BuildIndexedFileName_Result, "\p.");
	UTIL_PStrCat(gUTIL_BuildIndexedFileName_Result, myIndexStr);

	if (myExtensionDotPos != 0)
		{
		for (mySrcCharNum = myExtensionDotPos;
			mySrcCharNum <= mySrcFileNameLen;
			mySrcCharNum++)
			{
			myDstCharNum = gUTIL_BuildIndexedFileName_Result[0];
			gUTIL_BuildIndexedFileName_Result[++myDstCharNum] = 
				theSrcFileName[mySrcCharNum];
			gUTIL_BuildIndexedFileName_Result[0] = (char)myDstCharNum;
			};
		};

	return gUTIL_BuildIndexedFileName_Result;

	} // UTIL_BuildIndexedFileName

/*------------------------------------------------------------------------------
//
//	UTIL_ExtFileNameSearch
//
*/

Str31		  gUTIL_ExtFileNameSearch_FileName;

StringPtr UTIL_ExtFileNameSearch (StringPtr theStringPtr, StringPtr theExtension)

	{
	ushort		  mySrcStrLen;
	ushort		  mySrcCharNum;
	ushort		  myExtStrLen;
	ushort		  myExtCharNum;
	Boolean		  myExtFound = false;
	ushort		  myExtStartCharNum;
	ushort		  myExtEndCharNum;
	ushort		  myFileNameStart;
	ushort		  myTempSrcCharNum;
	ushort		  myFileNameCharNum;

	if ((mySrcStrLen = (ushort)theStringPtr[0]) == 0) return NULL;
	if ((myExtStrLen = (ushort)theExtension[0]) == 0) return NULL;

	for (mySrcCharNum = 1; mySrcCharNum <= mySrcStrLen; mySrcCharNum++)
		{
		myExtFound = true;
		myExtStartCharNum = mySrcCharNum;
		for (myExtCharNum = 1; myExtCharNum <= myExtStrLen; myExtCharNum++)
			{
			myExtEndCharNum = mySrcCharNum + myExtCharNum - 1;
			if (myExtEndCharNum > mySrcStrLen)
				{
				myExtFound = false;
				break;
				};
			if (theStringPtr[myExtEndCharNum] != theExtension[myExtCharNum])
				{
				myExtFound = false;
				break;
				};
			};
		if (!myExtFound) continue;

		for (myFileNameStart = myExtStartCharNum;
			myFileNameStart > 0;
			myFileNameStart--)
			{
			if (!UTIL_IsFileNameChar(theStringPtr[myFileNameStart]))
				{
				myFileNameStart++;
				break;
				};
			};
		if (myFileNameStart == myExtStartCharNum)
			{
			myExtFound = false;
			continue;
			};

		myFileNameCharNum = 0;
		for (myTempSrcCharNum = myFileNameStart;
			myTempSrcCharNum <= myExtEndCharNum;
			myTempSrcCharNum++)
			{
			gUTIL_ExtFileNameSearch_FileName[++myFileNameCharNum] =
				theStringPtr[myTempSrcCharNum];
			};
		gUTIL_ExtFileNameSearch_FileName[0] = (uchar)myFileNameCharNum;

		break;
		};

	if (myExtFound) return gUTIL_ExtFileNameSearch_FileName;

	return NULL;

	} // UTIL_ExtFileNameSearch

/*------------------------------------------------------------------------------
//
//	UTIL_IsFileNameChar
//
*/

Boolean UTIL_IsFileNameChar (char theChar)

	{

	if ((theChar >= 'A') && (theChar <= 'Z')) return true;
	if ((theChar >= 'a') && (theChar <= 'z')) return true;
	if ((theChar >= '0') && (theChar <= '9')) return true;

	if (theChar == '.') return true;
	if (theChar == '-') return true;
	if (theChar == '_') return true;
	if (theChar == '#') return true;
	if (theChar == '&') return true;

	return false;

	} /* UTIL_IsFileNameChar */

/*------------------------------------------------------------------------------
//
//	UTIL_IsSpaceChar
//
*/

Boolean UTIL_IsSpaceChar (char theChar)

	{

	if (theChar == '\0') return false;

	if (theChar <= ' ') return true;

	return false;

	} // UTIL_IsSpaceChar

/*------------------------------------------------------------------------------
//
//	UTIL_IsEmptyLine
//
*/

Boolean UTIL_IsEmptyLine (char *theLineCharsPtr, short *theLineLenPtr)

	{
	short			  myCharNum;

	for (myCharNum = 0; myCharNum < *theLineLenPtr; myCharNum++)
		{
		if (theLineCharsPtr[myCharNum] != ' ') return false;
		};

	return true;

	} /* UTIL_IsEmptyLine */

/*------------------------------------------------------------------------------
//
//	UTIL_PStringEndsWith
//
*/

Boolean UTIL_PStringEndsWith (StringPtr theStringPtr, StringPtr theEndPtr)

	{
	short			  myStringLen = (short)theStringPtr[0];
	short			  myEndLen = (short)theEndPtr[0];
	short			  myLensDiff = myStringLen - myEndLen;
	short			  myEndCharNum;

	if (myLensDiff < 0) return false;

	for (myEndCharNum = 1; myEndCharNum <= myEndLen; myEndCharNum++)
		{
		if (theStringPtr[myLensDiff + myEndCharNum] != theEndPtr[myEndCharNum])
			{
			return false;
			};
		};

	return true;

	} /* UTIL_PStringEndsWith */

/*------------------------------------------------------------------------------
//
//	UTIL_PStrCpy
//
*/

void UTIL_PStrCpy (StringPtr theTargetStringPtr, StringPtr theArrowStringPtr)

	{
	ushort			  myStrLen;
	ushort			  myCharNum;

	myStrLen = (ushort)theArrowStringPtr[0];

	for (myCharNum = 0; myCharNum <= myStrLen; myCharNum++)
		{
		theTargetStringPtr[myCharNum] = theArrowStringPtr[myCharNum];
		};

	} /* UTIL_PStrCpy */

/*------------------------------------------------------------------------------
//
//	UTIL_PStrCat
//
*/

void UTIL_PStrCat (StringPtr theTargetStringPtr, StringPtr theArrowStringPtr)

	{
	ushort			  myTargetStrLen;
	ushort			  myArrowStrLen;
	ushort			  myTargetCharNum;
	ushort			  myArrowCharNum;

	myTargetStrLen = (short)theTargetStringPtr[0];
	myArrowStrLen = (short)theArrowStringPtr[0];

	for (myArrowCharNum = 1; myArrowCharNum <= myArrowStrLen; myArrowCharNum++)
		{
		myTargetCharNum = myTargetStrLen + myArrowCharNum;
		if (myTargetCharNum > 255) break;
		theTargetStringPtr[myTargetCharNum] = theArrowStringPtr[myArrowCharNum];
		};

	myTargetStrLen += myArrowStrLen;
	if (myTargetStrLen > 255) myTargetStrLen = 255;
	theTargetStringPtr[0] = (uchar)myTargetStrLen;

	} /* UTIL_PStrCat */

/*------------------------------------------------------------------------------
//
//	UTIL_IsOptionKeyDown
//
*/

Boolean UTIL_IsOptionKeyDown (void)

	{
	KeyMap		  myKeyMap;

	GetKeys(myKeyMap);

	return mCheckKeyMap(myKeyMap, 58);

	} /* UTIL_IsOptionKeyDown */

/*------------------------------------------------------------------------------
//
//	UTIL_ToUpper
//
*/

char UTIL_ToUpper (char theChar)

	{

	if ((theChar >= 'a') && (theChar <= 'z')) return (char)(theChar - 32);

	return theChar;

	} /* UTIL_ToUpper */


/*------------------------------------------------------------------------------
//
//	UTIL_StrLen
//
*/

unsigned short UTIL_StrLen (char *theStr)

	{
	unsigned short		  myCharNum;

	for (myCharNum = 0; theStr[myCharNum] != '\0'; myCharNum++) {};

	return myCharNum;

	} // UTIL_StrLen

/*------------------------------------------------------------------------------
//
//	UTIL_StrNCmp
//
*/

short UTIL_StrNCmp (char *theStr1, char *theStr2, unsigned short theScope)

	{
	unsigned short		  myCharNum;

	for (myCharNum = 0; myCharNum < theScope; myCharNum++)
		{
		if (theStr1[myCharNum] == '\0')
			{
			if (theStr2[myCharNum] == '\0')
				{
				return 0;
				}
			else return -1;
			};
		if (theStr2[myCharNum] == '\0') return 1;
		if (theStr1[myCharNum] < theStr2[myCharNum]) return -1;
		if (theStr1[myCharNum] > theStr2[myCharNum]) return 1;
		};

	return 0;

	} // UTIL_StrNCmp

/*------------------------------------------------------------------------------
//
//	UTIL_StrPos
//
*/

short UTIL_StrPos (char *theStrToLookIn, char *theStrToLookFor,
		Boolean theIgnoreCaseFlag, Boolean theIgnoreSpacesFlag)

	{
	short		  myLookInStartScope;
	short		  myLookInStartCharNum;
	short		  myLookInCharNum;
	short		  myLookForCharNum;
	char			  myLookInChar;
	char			  myLookForChar;
	Boolean		  myMatchOK = false;

	myLookInStartScope = UTIL_StrLen(theStrToLookIn);
	myLookInStartScope -= UTIL_StrLen(theStrToLookFor);
	if (myLookInStartScope < 0) return -1;

	for (myLookInStartCharNum = 0;
		myLookInStartCharNum <= myLookInStartScope;
		myLookInStartCharNum++)
		{
		myMatchOK = false;

		for (myLookInCharNum = myLookInStartCharNum, myLookForCharNum = 0;
			theStrToLookIn[myLookInCharNum] != '\0';
			myLookInCharNum++)
			{
			myLookInChar = theStrToLookIn[myLookInCharNum];

			if (theIgnoreSpacesFlag)
				if (UTIL_IsSpaceChar(myLookInChar)) continue;

			myLookInChar = (theIgnoreCaseFlag)
				? UTIL_ToUpper(myLookInChar)
				: myLookInChar;

			if (theIgnoreSpacesFlag)
				{
				while (UTIL_IsSpaceChar(theStrToLookFor[myLookForCharNum]))
					{
					myLookForCharNum++;
					};
				};

			myLookForChar = theStrToLookFor[myLookForCharNum];
			if (myLookForChar == '\0')
				{
				myMatchOK = true;
				break;
				};

			myLookForChar = (theIgnoreCaseFlag)
				? UTIL_ToUpper(myLookForChar)
				: myLookForChar;

			if (myLookInChar != myLookForChar) break;

			myLookForCharNum++;
			};

		if (myMatchOK) break;
		};

	if (myMatchOK)
		{
		return myLookInStartCharNum;
		}
	else return -1;


/*
	short		  myLookScope;
	short		  myCharNum;

	myLookScope = UTIL_StrLen(theStrToLookIn);
	myLookScope -= UTIL_StrLen(theStrToLookFor);
	if (myLookScope < 0) return -1;

	for (myCharNum = 0; myCharNum <= myLookScope; myCharNum++)
		{
		if (UTIL_StrNCmp(theStrToLookIn, theStrToLookFor,
			UTIL_StrLen(theStrToLookFor)) == 0)
			{
			return myCharNum;
			};
		};

	return -1;
*/

	} // UTIL_StrPos

/*------------------------------------------------------------------------------
//
//	UTIL_P2LocalCStr
//
*/

char		  gUTIL_P2LocalCStr[256];

char *UTIL_P2LocalCStr (StringPtr theStringPtr)

	{
	short		  myStrLen = (short)theStringPtr[0];
	short		  myCharNum;

	for (myCharNum = 0; myCharNum < myStrLen; myCharNum++)
		{
		gUTIL_P2LocalCStr[myCharNum] = theStringPtr[myCharNum + 1];
		};
	gUTIL_P2LocalCStr[myStrLen] = '\0';

	return gUTIL_P2LocalCStr;

	} // UTIL_P2LocalCStr

/*------------------------------------------------------------------------------
//
//	UTIL_DeleteTranscript
//
*/

#if kUsesTranscript

void UTIL_DeleteTranscript (void)

	{
	FSSpec			  myTranscriptFSSpec;

	if (FSMakeFSSpec(UTIL_GetAppVRefNum(), UTIL_GetAppDirID(), "\pTranscript",
		&myTranscriptFSSpec) == noErr)
		{
		FSpDelete(&myTranscriptFSSpec);
		};

	} /* UTIL_DeleteTranscript */

#endif

/*------------------------------------------------------------------------------
//
//	UTIL_TransWrite
//
*/

#if kUsesTranscript

void UTIL_TransWrite (const char *theFormatStr, ...)

	{
	OSErr			  myOSErr;
	va_list			  myVAList;
	char				  myWriteStr[256];
	long				  myWriteCount = 0L;
	char				 *myCharPtr = myWriteStr;
	FSSpec			  myTranscriptFSSpec;
	short			  myTranscriptRefNum;
	IOParam			  myFlushIOParam;

	va_start(myVAList, theFormatStr);
	vsprintf(myWriteStr, theFormatStr, myVAList);
	va_end(myVAList);

	myOSErr = FSMakeFSSpec(UTIL_GetAppVRefNum(), UTIL_GetAppDirID(), "\pTranscript", &myTranscriptFSSpec);
	if (myOSErr == fnfErr) myOSErr = FSpCreate(&myTranscriptFSSpec, 'R*ch', 'TEXT', 0);
	if (myOSErr != noErr) return;
	myOSErr = FSpOpenDF(&myTranscriptFSSpec, fsCurPerm, &myTranscriptRefNum);
	if (myOSErr != noErr) return;

	if (SetFPos(myTranscriptRefNum, fsFromLEOF, 0L) == noErr)
		{
		while (*(myCharPtr++)) myWriteCount++;
		myOSErr = FSWrite(myTranscriptRefNum, &myWriteCount, myWriteStr);
		};

	myFlushIOParam.ioCompletion = NULL;
	myFlushIOParam.ioRefNum = myTranscriptRefNum;
	PBFlushFileSync((ParmBlkPtr)&myFlushIOParam);
	FSClose(myTranscriptRefNum);

	} /* UTIL_TransWrite */

#endif

/*------------------------------------------------------------------------------
//
//	That's All, Folks !...
//
*/
