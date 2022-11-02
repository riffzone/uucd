/*------------------------------------------------------------------------------
//
//	segments.c
//
*/

#include "Fonts.h"
#include "Lists.h"
#include "LowMem.h"
#include "Memory.h"
#include "QDOffscreen.h"
#include "QuickDraw.h"
#include "Resources.h"
#include "Script.h"
#include "Sound.h"
#include "TextUtils.h"
#include "ToolUtils.h"
#include "Types.h"

#include "util.h"
#include "prefs.h"
#include "info.h"
#include "uucd.h"
#include "blobs.h"

#include "segments.h"

/*------------------------------------------------------------------------------
//
//	Definitions
//
*/

#define kUUCDMaxNbOfSegments 10000

#define kBlobsFileName "uucd.blobs"

/*------------------------------------------------------------------------------
//
//	Globales
//
*/

short			  gUUCDNbOfSegments = 0;
UUCD_SegmentHdl	  gUUCDSegments[kUUCDMaxNbOfSegments];
short			  gUUCDTopSegmentNum = 0;
Rect				  gUUCDSegListViewRect = {0, 0, 0, 0};
short			  gUUCDNbOfVisibleSegments = 0;
short			  gUUCDSegmentWidth = 0;

BLOB_BasePtr		  gBlobBasePtr = NULL;

GWorldPtr			  gUUCDSegmentGWorldPtr = NULL;

uchar			  gHQXHeadLine[80] = "(This file must be converted with BinHex 4.0)";

CICNHandle		  gUUCD_CICON_Begin = NULL;
CICNHandle		  gUUCD_CICON_Body = NULL;
CICNHandle		  gUUCD_CICON_End = NULL;
CICNHandle		  gUUCD_CICON_LinkedBegin = NULL;
CICNHandle		  gUUCD_CICON_LinkedBody = NULL;
CICNHandle		  gUUCD_CICON_LinkedEnd = NULL;
CICNHandle		  gUUCD_CICON_Complete = NULL;

CICNHandle		  gUUCD_CICON_MarkNone = NULL;
CICNHandle		  gUUCD_CICON_MarkCheck = NULL;

CICNHandle		  gUUCD_CICON_HQX = NULL;
CICNHandle		  gUUCD_CICON_B64 = NULL;

CICNHandle		  gUUCDSaveIconUpCIconHandle =  NULL;
CICNHandle		  gUUCDSaveIconDownCIconHandle =  NULL;
CICNHandle		  gUUCDViewIconUpCIconHandle =  NULL;
CICNHandle		  gUUCDViewIconDownCIconHandle =  NULL;
CICNHandle		  gUUCDInfoIconUpCIconHandle =  NULL;
CICNHandle		  gUUCDInfoIconDownCIconHandle =  NULL;

PixPatHandle		  gUUCDBgEvenLinePixPatHandle = NULL;
PixPatHandle		  gUUCDBgOddLinePixPatHandle = NULL;
PixPatHandle		  gUUCDBgSourceLinePixPatHandle = NULL;
PixPatHandle		  gUUCDBgTargetLinePixPatHandle = NULL;
PixPatHandle		  gUUCDBgInfoLinePixPatHandle = NULL;

/*------------------------------------------------------------------------------
//
//	UUCD_NewSegment
//
*/

short UUCD_NewSegment (void)

	{
	short			  myNewSegmentNum;
	UUCD_Segment		  myNewSegment;
	Handle			  myNewSegmentHdl = NULL;
	Handle			  myDataHandle = NULL;

	if (gUUCDNbOfSegments >= kUUCDMaxNbOfSegments) return -1;
	myNewSegmentNum = gUUCDNbOfSegments;

	myDataHandle = NewHandle(0);
	if (myDataHandle == NULL) return -1;

	myNewSegment.UUCD_SEGMENT_NameHdl = NULL;
	myNewSegment.UUCD_SEGMENT_DataHdl = myDataHandle;
	myNewSegment.UUCD_SEGMENT_DataSize = 0L;
	myNewSegment.UUCD_SEGMENT_DataType = kUUCD_DATATYPE_Unknown;
	myNewSegment.UUCD_SEGMENT_BlobID = 0L;
	myNewSegment.UUCD_SEGMENT_Type = kUUCD_SEGTYPE_None;
	myNewSegment.UUCD_SEGMENT_State = kUUCD_SEGSTATE_None;
	myNewSegment.UUCD_SEGMENT_DstFileNameHdl = NULL;

	myNewSegment.UUCD_SEGMENT_Mark = kUUCD_MARK_None;
	myNewSegment.UUCD_SEGMENT_SrcFileFSSpec.vRefNum = 0;
	myNewSegment.UUCD_SEGMENT_SrcFileFSSpec.parID = 0L;
	myNewSegment.UUCD_SEGMENT_SrcFileFSSpec.name[0] = '\0';
	myNewSegment.UUCD_SEGMENT_SrcFileSize = 0L;
	myNewSegment.UUCD_SEGMENT_FrstSrcLineNum = 0L;
	myNewSegment.UUCD_SEGMENT_LastSrcLineNum = 0L;

	if (PtrToHand(&myNewSegment, &myNewSegmentHdl, (long)sizeof(UUCD_Segment)) != noErr)
		{
		DisposeHandle(myDataHandle);
		return -1;
		};

	gUUCDSegments[myNewSegmentNum] = (UUCD_SegmentHdl)myNewSegmentHdl;
	gUUCDNbOfSegments++;

	return myNewSegmentNum;

	} /* UUCD_NewSegment */

/*------------------------------------------------------------------------------
//
//	UUCD_DelSegment
//
*/

void UUCD_DelSegment (short theSegmentNum)

	{
	UUCD_SegmentHdl	  mySegmentHdl;
	UUCD_SegmentPtr	  mySegmentPtr;
	short			  myTempSegmentNum;

	mySegmentHdl = UUCD_GetSegmentHdl(theSegmentNum);
	if (mySegmentHdl == NULL) return;

	HLock((Handle)mySegmentHdl);
	mySegmentPtr = *mySegmentHdl;

	if (mySegmentPtr->UUCD_SEGMENT_NameHdl != NULL)
		DisposeHandle((Handle)mySegmentPtr->UUCD_SEGMENT_NameHdl);

	if (mySegmentPtr->UUCD_SEGMENT_DataHdl != NULL)
		DisposeHandle((Handle)mySegmentPtr->UUCD_SEGMENT_DataHdl);

	if (mySegmentPtr->UUCD_SEGMENT_DstFileNameHdl != NULL)
		DisposeHandle((Handle)mySegmentPtr->UUCD_SEGMENT_DstFileNameHdl);

	if (mySegmentPtr->UUCD_SEGMENT_BlobID != 0L)
		BLOB_FreeFile(gBlobBasePtr, mySegmentPtr->UUCD_SEGMENT_BlobID);

	HUnlock((Handle)mySegmentHdl);

	DisposeHandle((Handle)mySegmentHdl);
	gUUCDSegments[theSegmentNum] = NULL;

	for (myTempSegmentNum = theSegmentNum + 1;
		myTempSegmentNum < gUUCDNbOfSegments;
		myTempSegmentNum++)
		{
		gUUCDSegments[myTempSegmentNum - 1] = gUUCDSegments[myTempSegmentNum];
		};
	gUUCDNbOfSegments--;

	} /* UUCD_DelSegment */

/*------------------------------------------------------------------------------
//
//	UUCD_GetSegmentHdl
//
*/

UUCD_SegmentHdl UUCD_GetSegmentHdl (short theSegmentNum)

	{

	if (theSegmentNum < 0) return NULL;
	if (theSegmentNum >= gUUCDNbOfSegments) return NULL;

	return gUUCDSegments[theSegmentNum];

	} /* UUCD_GetSegmentHdl */

/*------------------------------------------------------------------------------
//
//	UUCD_GetNbOfSegments
//
*/

short UUCD_GetNbOfSegments (void)

	{

	return gUUCDNbOfSegments;

	} /* UUCD_GetNbOfSegments */

/*------------------------------------------------------------------------------
//
//	UUCD_Get/SetNbOfVisibleSegments
//
*/

short UUCD_GetNbOfVisibleSegments (void)

	{

	return gUUCDNbOfVisibleSegments;

	} /* UUCD_GetNbOfVisibleSegments */

void UUCD_SetNbOfVisibleSegments (short theNbOfSegments)

	{

	gUUCDNbOfVisibleSegments = theNbOfSegments;
	SetRect(&gUUCDSegListViewRect, 0, 0, 0, 0);

	} /* UUCD_SetNbOfVisibleSegments */

/*------------------------------------------------------------------------------
//
//	UUCD_Get/SetSegmentWidth
//
*/

short UUCD_GetSegmentWidth (void)

	{

	return gUUCDSegmentWidth;

	} /* UUCD_GetSegmentWidth */

void UUCD_SetSegmentWidth (short theSegmentWidth)

	{

	gUUCDSegmentWidth = theSegmentWidth;

	DisposeGWorld(gUUCDSegmentGWorldPtr);
	gUUCDSegmentGWorldPtr = NULL;

	} /* UUCD_SetSegmentWidth */

/*------------------------------------------------------------------------------
//
//	UUCD_GetSegListViewRect
//
*/

RectPtr UUCD_GetSegListViewRect (void)

	{

	if (!EmptyRect(&gUUCDSegListViewRect)) return &gUUCDSegListViewRect;

	gUUCDSegListViewRect.top = kUUCDMemoryHeight + kUUCDProgressHeight + kUUCDBorderSize;
	gUUCDSegListViewRect.left = kUUCDBorderSize + kUUCDCellMoveRectWidth + 1;
	gUUCDSegListViewRect.right = gUUCDSegListViewRect.left + gUUCDSegmentWidth;
	gUUCDSegListViewRect.bottom = gUUCDSegListViewRect.top + (UUCD_GetNbOfVisibleSegments() * kUUCD_SegmentHeight);

	return &gUUCDSegListViewRect;

	} /* UUCD_GetSegListViewRect */

/*------------------------------------------------------------------------------
//
//	UUCD_GetTopSegmentNum
//
*/

short UUCD_GetTopSegmentNum (void)

	{

	return gUUCDTopSegmentNum;

	} /* UUCD_GetTopSegmentNum */

/*------------------------------------------------------------------------------
//
//	UUCD_SetTopSegment
//
*/

void UUCD_SetTopSegment (short theSegmentNum)

	{
	GrafPtr			  mySavePort;
	short			  myOldTopSegmentNum;
	short			  myNewTopSegmentNum;
	short			  myNbOfSegments = UUCD_GetNbOfSegments();
	short			  myMaxTopSegmentNum;
	short			  myVScrollAmount;
	RgnHandle			  myUpdateRgn = NewRgn();
	short			  mySegmentNum;
	Rect				  mySegmentRect;

	if (myNbOfSegments == 0) return;
	if (theSegmentNum == gUUCDTopSegmentNum) return;

	myNewTopSegmentNum = theSegmentNum;
	if (myNewTopSegmentNum < 0) myNewTopSegmentNum = 0;
	myMaxTopSegmentNum = myNbOfSegments - gUUCDNbOfVisibleSegments;
	if (myMaxTopSegmentNum < 0) myMaxTopSegmentNum = 0;
	if (myNewTopSegmentNum > myMaxTopSegmentNum) myNewTopSegmentNum = myMaxTopSegmentNum;

	myOldTopSegmentNum = gUUCDTopSegmentNum;
	gUUCDTopSegmentNum = myNewTopSegmentNum;

	myVScrollAmount = myNewTopSegmentNum - myOldTopSegmentNum;
	myVScrollAmount *= kUUCD_SegmentHeight;

	GetPort(&mySavePort);
	SetPort(UUCD_GetUUCDWindowPtr());
	ScrollRect(&gUUCDSegListViewRect, 0, - myVScrollAmount, myUpdateRgn);
	SetPort(mySavePort);

	for (mySegmentNum = myNewTopSegmentNum;
		mySegmentNum < myNbOfSegments;
		mySegmentNum++)
		{
		if (!UUCD_GetSegmentRect(mySegmentNum, &mySegmentRect)) break;
		if (RectInRgn(&mySegmentRect, myUpdateRgn))
			UUCD_DrawSegment(mySegmentNum, NULL);
		};

	DisposeRgn(myUpdateRgn);

	} /* UUCD_SetTopSegment */

/*------------------------------------------------------------------------------
//
//	UUCD_GetSegment
//
*/

Boolean UUCD_GetSegment (short theSegmentNum, UUCD_SegmentPtr theSegmentPtr)

	{
	UUCD_SegmentHdl	  mySegmentHdl;

	mySegmentHdl = UUCD_GetSegmentHdl(theSegmentNum);
	if (mySegmentHdl == NULL) return false;

	HLock((Handle)mySegmentHdl);
	BlockMove(*mySegmentHdl, theSegmentPtr, (long)sizeof(UUCD_Segment));
	HUnlock((Handle)mySegmentHdl);

	return true;

	} /* UUCD_GetSegment */

/*------------------------------------------------------------------------------
//
//	UUCD_SetSegment
//
*/

Boolean UUCD_SetSegment (short theSegmentNum, UUCD_SegmentPtr theSegmentPtr)

	{
	UUCD_SegmentHdl	  mySegmentHdl;

	mySegmentHdl = UUCD_GetSegmentHdl(theSegmentNum);
	if (mySegmentHdl == NULL) return false;

	HLock((Handle)mySegmentHdl);
	BlockMove(theSegmentPtr, *mySegmentHdl, (long)sizeof(UUCD_Segment));
	HUnlock((Handle)mySegmentHdl);

	return true;

	} /* UUCD_SetSegment */

/*------------------------------------------------------------------------------
//
//	UUCD_GetSegmentDataSize
//
*/

long UUCD_GetSegmentDataSize (short theSegmentNum)

	{
	UUCD_SegmentHdl	  mySegmentHdl;
//	Handle			  mySegmentDataHdl;
	long				  mySegmentDataSize = 0L;

	mySegmentHdl = UUCD_GetSegmentHdl(theSegmentNum);
	if (mySegmentHdl == NULL) return 0L;

	HLock((Handle)mySegmentHdl);
	mySegmentDataSize = (**mySegmentHdl).UUCD_SEGMENT_DataSize;
//	mySegmentDataHdl = (**mySegmentHdl).UUCD_SEGMENT_DataHdl;
//	if (mySegmentDataHdl != NULL) mySegmentDataSize = GetHandleSize(mySegmentDataHdl);
	HUnlock((Handle)mySegmentHdl);

	return mySegmentDataSize;

	} /* UUCD_GetSegmentDataSize */

/*------------------------------------------------------------------------------
//
//	UUCD_GetSegmentDstFileName
//
*/

Boolean UUCD_GetSegmentDstFileName (short theSegmentNum, StringPtr theDstFileNamePtr)

	{
	UUCD_SegmentHdl	  mySegmentHdl;
	StringHandle		  myDstFileNameHdl;
	uchar			 *myDstFileNamePtr;
	ushort			  myDstFileNameLen;

	*theDstFileNamePtr = 0x00;

	mySegmentHdl = UUCD_GetSegmentHdl(theSegmentNum);
	if (mySegmentHdl == NULL) return false;

	HLock((Handle)mySegmentHdl);
	myDstFileNameHdl = (**mySegmentHdl).UUCD_SEGMENT_DstFileNameHdl;
	if (myDstFileNameHdl != NULL)
		{
		HLock((Handle)myDstFileNameHdl);
		myDstFileNamePtr = (uchar *)*myDstFileNameHdl;
		myDstFileNameLen = (short)*myDstFileNamePtr;
		if (myDstFileNameLen > 63) myDstFileNameLen = 63;
		BlockMove((Ptr)myDstFileNamePtr, theDstFileNamePtr, (long)(myDstFileNameLen + 1));
		theDstFileNamePtr[0] = (uchar)myDstFileNameLen;
		HUnlock((Handle)myDstFileNameHdl);
		};
	HUnlock((Handle)mySegmentHdl);

	return (*theDstFileNamePtr != 0x00);

	} /* UUCD_GetSegmentDstFileName */

/*------------------------------------------------------------------------------
//
//	UUCD_GetSegmentName
//
*/

void UUCD_GetSegmentName (short theSegmentNum, StringPtr theNamePtr)

	{
	UUCD_SegmentHdl	  mySegmentHdl;
	StringHandle		  mySegmentNameHdl;
	StringPtr			  myStringPtr;

	*theNamePtr = '\0';

	mySegmentHdl = UUCD_GetSegmentHdl(theSegmentNum);
	if (mySegmentHdl == NULL) return;

	HLock((Handle)mySegmentHdl);
	mySegmentNameHdl = (**mySegmentHdl).UUCD_SEGMENT_NameHdl;
	if (mySegmentNameHdl != NULL)
		{
		HLock((Handle)mySegmentNameHdl);
		myStringPtr = *mySegmentNameHdl;
		BlockMove(myStringPtr, theNamePtr, (long)(myStringPtr[0] + 1));
		HUnlock((Handle)mySegmentNameHdl);
		};
	HUnlock((Handle)mySegmentHdl);

	} /* UUCD_GetSegmentName */

/*------------------------------------------------------------------------------
//
//	UUCD_SetSegmentName
//
*/

void UUCD_SetSegmentName (short theSegmentNum, StringPtr theName)

	{
	UUCD_SegmentHdl	  mySegmentHdl;

	mySegmentHdl = UUCD_GetSegmentHdl(theSegmentNum);
	if (mySegmentHdl == NULL) return;

	HLock((Handle)mySegmentHdl);
	if ((**mySegmentHdl).UUCD_SEGMENT_NameHdl != NULL)
		{
		DisposeHandle((Handle)(**mySegmentHdl).UUCD_SEGMENT_NameHdl);
		(**mySegmentHdl).UUCD_SEGMENT_NameHdl = NULL;
		};
	(**mySegmentHdl).UUCD_SEGMENT_NameHdl = NewString(theName);
	HUnlock((Handle)mySegmentHdl);

	} /* UUCD_SetSegmentName */

/*------------------------------------------------------------------------------
//
//	UUCD_SetSegmentType
//
*/

void UUCD_SetSegmentType (short theSegmentNum, UUCD_SegmentType theSegmentType)

	{
	UUCD_SegmentHdl	  mySegmentHdl;

	mySegmentHdl = UUCD_GetSegmentHdl(theSegmentNum);
	if (mySegmentHdl == NULL) return;

	HLock((Handle)mySegmentHdl);
	(**mySegmentHdl).UUCD_SEGMENT_Type = theSegmentType;
	HUnlock((Handle)mySegmentHdl);

	} /* UUCD_SetSegmentType */

/*------------------------------------------------------------------------------
//
//	UUCD_GetSegmentType
//
*/

UUCD_SegmentType UUCD_GetSegmentType (short theSegmentNum)

	{
	UUCD_SegmentHdl	  mySegmentHdl;
	UUCD_SegmentType	  mySegmentType;

	mySegmentHdl = UUCD_GetSegmentHdl(theSegmentNum);
	if (mySegmentHdl == NULL) return kUUCD_SEGTYPE_None;

	HLock((Handle)mySegmentHdl);
	mySegmentType = (**mySegmentHdl).UUCD_SEGMENT_Type;
	HUnlock((Handle)mySegmentHdl);

	return mySegmentType;

	} /* UUCD_GetSegmentType */

/*------------------------------------------------------------------------------
//
//	UUCD_SetSegmentMark
//
*/

void UUCD_SetSegmentMark (short theSegmentNum, UUCD_SegmentMark theSegmentMark)

	{
	UUCD_SegmentHdl	  mySegmentHdl;

	mySegmentHdl = UUCD_GetSegmentHdl(theSegmentNum);
	if (mySegmentHdl == NULL) return;

	HLock((Handle)mySegmentHdl);
	(**mySegmentHdl).UUCD_SEGMENT_Mark = theSegmentMark;
	HUnlock((Handle)mySegmentHdl);

	} /* UUCD_SetSegmentMark */

/*------------------------------------------------------------------------------
//
//	UUCD_GetSegmentMark
//
*/

UUCD_SegmentMark UUCD_GetSegmentMark (short theSegmentNum)

	{
	UUCD_SegmentHdl	  mySegmentHdl;
	UUCD_SegmentMark	  mySegmentMark;

	mySegmentHdl = UUCD_GetSegmentHdl(theSegmentNum);
	if (mySegmentHdl == NULL) return kUUCD_MARK_None;

	HLock((Handle)mySegmentHdl);
	mySegmentMark = (**mySegmentHdl).UUCD_SEGMENT_Mark;
	HUnlock((Handle)mySegmentHdl);

	return mySegmentMark;

	} /* UUCD_GetSegmentMark */

/*------------------------------------------------------------------------------
//
//	UUCD_MarkAllSegments
//
*/

void UUCD_MarkAllSegments (UUCD_SegmentMark theSegmentMark)

	{
	short			  mySegmentNum;

	for (mySegmentNum = 0; mySegmentNum < UUCD_GetNbOfSegments(); mySegmentNum++)
		{
		UUCD_SetSegmentMark(mySegmentNum, theSegmentMark);
		};

	} /* UUCD_MarkAllSegments */

/*------------------------------------------------------------------------------
//
//	UUCD_SetSegmentState
//
*/

void UUCD_SetSegmentState (short theSegmentNum, UUCD_SegmentState theSegmentState)

	{
	UUCD_SegmentHdl	  mySegmentHdl;

	mySegmentHdl = UUCD_GetSegmentHdl(theSegmentNum);
	if (mySegmentHdl == NULL) return;

	HLock((Handle)mySegmentHdl);
	(**mySegmentHdl).UUCD_SEGMENT_State = theSegmentState;
	HUnlock((Handle)mySegmentHdl);

	} /* UUCD_SetSegmentState */

/*------------------------------------------------------------------------------
//
//	UUCD_GetSegmentState
//
*/

UUCD_SegmentState UUCD_GetSegmentState (short theSegmentNum)

	{
	UUCD_SegmentHdl	  mySegmentHdl;
	UUCD_SegmentState	  mySegmentState;

	mySegmentHdl = UUCD_GetSegmentHdl(theSegmentNum);
	if (mySegmentHdl == NULL) return kUUCD_SEGSTATE_None;

	HLock((Handle)mySegmentHdl);
	mySegmentState = (**mySegmentHdl).UUCD_SEGMENT_State;
	HUnlock((Handle)mySegmentHdl);

	return mySegmentState;

	} /* UUCD_GetSegmentState */

/*------------------------------------------------------------------------------
//
//	UUCD_CheckSegmentStates
//
*/

void UUCD_ReverseUnlinkSegmentStates (short theStartSegmentNum);

void UUCD_CheckSegmentStates (void)

	{
	short			  mySegmentNum;

	for (mySegmentNum = 0; mySegmentNum < gUUCDNbOfSegments; mySegmentNum++)
		{
		switch (UUCD_GetSegmentState(mySegmentNum))
			{
		case kUUCD_SEGSTATE_Begin:
		case kUUCD_SEGSTATE_LinkedBegin:
			if (mySegmentNum > 0)
				{
				if (UUCD_GetSegmentState(mySegmentNum - 1) != kUUCD_SEGSTATE_LinkedEnd)
					{
					UUCD_ReverseUnlinkSegmentStates(mySegmentNum - 1);
					};
				};
			if (mySegmentNum == (gUUCDNbOfSegments - 1))
				{
				UUCD_SetSegmentState(mySegmentNum, kUUCD_SEGSTATE_Begin);
				break;
				};
			UUCD_SetSegmentState(mySegmentNum, kUUCD_SEGSTATE_LinkedBegin);
			break;
		case kUUCD_SEGSTATE_Body:
		case kUUCD_SEGSTATE_LinkedBody:
			if (mySegmentNum == (gUUCDNbOfSegments - 1))
				{
				UUCD_SetSegmentState(mySegmentNum, kUUCD_SEGSTATE_Body);
				UUCD_ReverseUnlinkSegmentStates(mySegmentNum - 1);
				break;
				};
			if (mySegmentNum == 0)
				{
				UUCD_SetSegmentState(mySegmentNum, kUUCD_SEGSTATE_Body);
				break;
				};
			switch (UUCD_GetSegmentState(mySegmentNum - 1))
				{
			case kUUCD_SEGSTATE_LinkedBegin:
			case kUUCD_SEGSTATE_LinkedBody:
				UUCD_SetSegmentState(mySegmentNum, kUUCD_SEGSTATE_LinkedBody);
				break;
			default:
				UUCD_SetSegmentState(mySegmentNum, kUUCD_SEGSTATE_Body);
				UUCD_ReverseUnlinkSegmentStates(mySegmentNum - 1);
				break;
				};
			break;
		case kUUCD_SEGSTATE_End:
		case kUUCD_SEGSTATE_LinkedEnd:
			if (mySegmentNum == 0)
				{
				UUCD_SetSegmentState(mySegmentNum, kUUCD_SEGSTATE_End);
				break;
				};
			switch (UUCD_GetSegmentState(mySegmentNum - 1))
				{
			case kUUCD_SEGSTATE_LinkedBegin:
			case kUUCD_SEGSTATE_LinkedBody:
				UUCD_SetSegmentState(mySegmentNum, kUUCD_SEGSTATE_LinkedEnd);
				break;
			default:
				UUCD_SetSegmentState(mySegmentNum, kUUCD_SEGSTATE_End);
				break;
				};
			break;
		case kUUCD_SEGSTATE_Complete:
			if (mySegmentNum > 0)
				{
				if (UUCD_GetSegmentState(mySegmentNum - 1) != kUUCD_SEGSTATE_LinkedEnd)
					{
					UUCD_ReverseUnlinkSegmentStates(mySegmentNum - 1);
					};
				};
			break;
			};
		};

	} /* UUCD_CheckSegmentStates */

void UUCD_ReverseUnlinkSegmentStates (short theStartSegmentNum)

	{
	short			  mySegmentNum;

	for (mySegmentNum = theStartSegmentNum; mySegmentNum >= 0; mySegmentNum--)
		{
		switch (UUCD_GetSegmentState(mySegmentNum))
			{
		case kUUCD_SEGSTATE_LinkedBegin:
			UUCD_SetSegmentState(mySegmentNum, kUUCD_SEGSTATE_Begin);
			mySegmentNum = 0; /* exit */
			break;
		case kUUCD_SEGSTATE_LinkedBody:
			UUCD_SetSegmentState(mySegmentNum, kUUCD_SEGSTATE_Body);
			break;
		case kUUCD_SEGSTATE_LinkedEnd:
			mySegmentNum = 0; /* exit */
			break;
			};
		};

	} /* UUCD_ReverseUnlinkSegmentStates */

/*------------------------------------------------------------------------------
//
//	UUCD_GetSegmentSrcFile
//
*/

long UUCD_GetSegmentSrcFile (short theSegmentNum, FSSpecPtr theFSSpecPtr)

	{
	long				  myFileSize = 0L;
	UUCD_SegmentHdl	  mySegmentHdl;

	mySegmentHdl = UUCD_GetSegmentHdl(theSegmentNum);
	if (mySegmentHdl == NULL) return 0L;

	HLock((Handle)mySegmentHdl);
	if (theFSSpecPtr != NULL)
		BlockMove(&((**mySegmentHdl).UUCD_SEGMENT_SrcFileFSSpec),
			theFSSpecPtr, (long)sizeof(FSSpec));
	myFileSize = (**mySegmentHdl).UUCD_SEGMENT_SrcFileSize;
	HUnlock((Handle)mySegmentHdl);

	return myFileSize;

	} /* UUCD_GetSegmentSrcFile */

/*------------------------------------------------------------------------------
//
//	UUCD_SetSegmentSrcFile
//
*/

void UUCD_SetSegmentSrcFile (short theSegmentNum, FSSpecPtr theFSSpecPtr, long theFileSize)

	{
	UUCD_SegmentHdl	  mySegmentHdl;

	mySegmentHdl = UUCD_GetSegmentHdl(theSegmentNum);
	if (mySegmentHdl == NULL) return;

	HLock((Handle)mySegmentHdl);
	BlockMove(theFSSpecPtr, &((**mySegmentHdl).UUCD_SEGMENT_SrcFileFSSpec), (long)sizeof(FSSpec));
	(**mySegmentHdl).UUCD_SEGMENT_SrcFileSize = theFileSize;
	HUnlock((Handle)mySegmentHdl);

	} /* UUCD_SetSegmentSrcFile */

/*------------------------------------------------------------------------------
//
//	UUCD_SetSegmentDstFileName
//
*/

void UUCD_SetSegmentDstFileName (short theSegmentNum, StringPtr theFileName)

	{
	UUCD_SegmentHdl	  mySegmentHdl;
	Str31			  myFileName;
	short			  mySrcCharNum;
	short			  myDstCharNum;
	short			  myFileNameLen;

	mySegmentHdl = UUCD_GetSegmentHdl(theSegmentNum);
	if (mySegmentHdl == NULL) return;

	myFileNameLen = theFileName[0];
	if (myFileNameLen > 31) myFileNameLen = 31;
	myFileName[0] = myFileNameLen;
	mySrcCharNum = theFileName[0];
	for (myDstCharNum = myFileNameLen; myDstCharNum > 0; myDstCharNum--)
		{
		myFileName[myDstCharNum] = theFileName[mySrcCharNum--];
		};

	HLock((Handle)mySegmentHdl);
	if ((**mySegmentHdl).UUCD_SEGMENT_DstFileNameHdl != NULL)
		{
		DisposeHandle((Handle)(**mySegmentHdl).UUCD_SEGMENT_DstFileNameHdl);
		(**mySegmentHdl).UUCD_SEGMENT_DstFileNameHdl = NULL;
		};
	(**mySegmentHdl).UUCD_SEGMENT_DstFileNameHdl = NewString(myFileName);
	HUnlock((Handle)mySegmentHdl);

	} /* UUCD_SetSegmentDstFileName */

/*------------------------------------------------------------------------------
//
//	UUCD_MoveSegment
//
*/

void UUCD_MoveSegment (short theSrcSegmentNum, short theDstSegmentNum)

	{
	short			  myLastSegmentNum;
	UUCD_SegmentHdl	  myMoveSegmentHdl;

	if (theSrcSegmentNum < 0) return;
	if (theDstSegmentNum < 0) return;
	if (theSrcSegmentNum >= gUUCDNbOfSegments) return;
	if (theDstSegmentNum >= gUUCDNbOfSegments) return;
	if (theSrcSegmentNum == theDstSegmentNum) return;

	if (gUUCDNbOfSegments < 2) return;
	myLastSegmentNum = gUUCDNbOfSegments - 1;

	myMoveSegmentHdl = gUUCDSegments[theSrcSegmentNum];

	if (theSrcSegmentNum != myLastSegmentNum)
	BlockMove(&gUUCDSegments[theSrcSegmentNum + 1],
			&gUUCDSegments[theSrcSegmentNum],
			(long)(myLastSegmentNum - theSrcSegmentNum) * (long)sizeof(UUCD_SegmentHdl));

	if (theDstSegmentNum != myLastSegmentNum)
	BlockMove(&gUUCDSegments[theDstSegmentNum],
			&gUUCDSegments[theDstSegmentNum + 1],
			(long)(myLastSegmentNum - theDstSegmentNum) * (long)sizeof(UUCD_SegmentHdl));

	gUUCDSegments[theDstSegmentNum] = myMoveSegmentHdl;

	} /* UUCD_MoveSegment */

/*------------------------------------------------------------------------------
//
//	UUCD_SortSegments
//
*/

void UUCD_SortSegments_ParseName (StringPtr theName, ushort *theLeftStrLenPtr, ushort *thePartNumPtr);
void UUCD_SortSegments_ParseName (StringPtr theName, ushort *theLeftStrLenPtr, ushort *thePartNumPtr)

	{
	ushort			  myNameLen;
	ushort			  mySlashPos = 0;
	ushort			  myCharNum;
	uchar			  myTempChar;
	ushort			  myPartNumPos = 0;

	myNameLen = (ushort)((uchar *)theName)[0];
	*theLeftStrLenPtr = myNameLen;
	*thePartNumPtr = 0;
	if (myNameLen < 3) return;

	for (myCharNum = myNameLen - 1; myCharNum >= 2; myCharNum--)
		{
		if (theName[myCharNum] == '/')
			{
			myTempChar = theName[myCharNum + 1];
			if ((myTempChar < '0') || (myTempChar > '9')) continue;

			myTempChar = theName[myCharNum - 1];
			if ((myTempChar < '0') || (myTempChar > '9')) continue;

			mySlashPos = myCharNum;
			break;
			};
		};

	if (mySlashPos == 0) return;

	for (myCharNum = mySlashPos - 1; myCharNum >= 1; myCharNum--)
		{
		myTempChar = theName[myCharNum];
		if ((myTempChar < '0') || (myTempChar > '9')) break;
		myPartNumPos = myCharNum;
		};

	if (myPartNumPos == 0) return;

	for (myCharNum = myPartNumPos; myCharNum < mySlashPos; myCharNum++)
		{
		*thePartNumPtr *= 10;
		*thePartNumPtr += (theName[myCharNum] - '0') % 10;
		};

	} /* UUCD_SortSegments_ParseName */

void UUCD_SortSegments (void)

	{
	Boolean			  myListSorted;
	short			  mySegmentsOrder; /* 0 (A=B), -1 (A<B), 1 (A>B) */
	short			  mySegmentNum;
	Str255			  mySegNameA;
	Str255			  mySegNameB;
	ushort			  myLeftStrLenA;
	ushort			  myLeftStrLenB;
	ushort			  myLeftStrLenMax;
	ushort			  myPartNumA;
	ushort			  myPartNumB;
	ushort			  myCharNum;

	if (gUUCDNbOfSegments < 2) return;

	for (;;)
		{
		myListSorted = true;

		for (mySegmentNum = 0;
			mySegmentNum < (gUUCDNbOfSegments - 1);
			mySegmentNum++)
			{
			mySegmentsOrder = 0;

			UUCD_GetSegmentName(mySegmentNum, mySegNameA);
			UUCD_GetSegmentName(mySegmentNum + 1, mySegNameB);

			UUCD_SortSegments_ParseName(mySegNameA, &myLeftStrLenA, &myPartNumA);
			UUCD_SortSegments_ParseName(mySegNameB, &myLeftStrLenB, &myPartNumB);

			myLeftStrLenMax = (myLeftStrLenA > myLeftStrLenB) ? myLeftStrLenA : myLeftStrLenB;

			for (myCharNum = 1; myCharNum <= myLeftStrLenMax; myCharNum++)
				{
				if (myCharNum > myLeftStrLenA) mySegmentsOrder = -1;
				if (myCharNum > myLeftStrLenB) mySegmentsOrder = 1;
				if (mySegmentsOrder != 0) break;

				if (mySegNameA[myCharNum] < mySegNameB[myCharNum]) mySegmentsOrder = -1;
				if (mySegNameA[myCharNum] > mySegNameB[myCharNum]) mySegmentsOrder = 1;
				if (mySegmentsOrder != 0) break;
				};

			if (mySegmentsOrder == 0)
				{
				if (myPartNumA < myPartNumB) mySegmentsOrder = -1;
				if (myPartNumA > myPartNumB) mySegmentsOrder = 1;
				};

			if (mySegmentsOrder == 1)
				{
				UUCD_MoveSegment(mySegmentNum + 1, mySegmentNum);
				myListSorted = false;
				};
			};

		if (myListSorted) break;
		};

	} /* UUCD_SortSegments */

/*------------------------------------------------------------------------------
//
//	UUCD_AddUUToSegment
//
*/

void UUCD_AddUUToSegment (short theSegmentNum, char *theUULinePtr, short theUULineLen, long theFileLineNum)

	{
#pragma unused (theUULineLen)

	OSErr			  myOSErr;
	UUCD_SegmentHdl	  mySegmentHdl;
	Handle			  myDataHandle;
	short			  myDstBytesLen;
	char				  myDstBytes[64];
	short			  myDstByteNum;
	short			  myFrstSrcCharNum;

	mySegmentHdl = UUCD_GetSegmentHdl(theSegmentNum);
	if (mySegmentHdl == NULL) return;

	myDstBytesLen = (theUULinePtr[0] - ' ') & 0x3F;
	if (myDstBytesLen == 0) return;

	/*   +------+ +------+ +------+ +------+   */
	/*   ..765432 ..107654 ..321076 ..543210   */
	/*   +------+ +------+ +------+ +------+   */

	for (myDstByteNum = 0; myDstByteNum < myDstBytesLen; myDstByteNum++)
		{
		myFrstSrcCharNum = (short)(((myDstByteNum / 3) * 4) + 1);

		switch (myDstByteNum % 3)
			{
		case 0:
			myDstBytes[myDstByteNum] = (char)(((theUULinePtr[myFrstSrcCharNum] - ' ') << 2) & 0xFC);
			myDstBytes[myDstByteNum] |= (char)(((theUULinePtr[myFrstSrcCharNum + 1] - ' ') >> 4) & 0x03);
			break;
		case 1:
			myDstBytes[myDstByteNum] = (char)(((theUULinePtr[myFrstSrcCharNum + 1] - ' ') << 4) & 0xF0);
			myDstBytes[myDstByteNum] |= (char)(((theUULinePtr[myFrstSrcCharNum + 2] - ' ') >> 2) & 0x0F);
			break;
		case 2:
			myDstBytes[myDstByteNum] = (char)(((theUULinePtr[myFrstSrcCharNum + 2] - ' ') << 6) & 0xC0);
			myDstBytes[myDstByteNum] |= (char)((theUULinePtr[myFrstSrcCharNum + 3] - ' ') & 0x3F);
			break;
			};
		};

	HLock((Handle)mySegmentHdl);

	if ((**mySegmentHdl).UUCD_SEGMENT_DataSize == 0L)
		{
		if (*((OSType *)&myDstBytes[6]) == 'JFIF')
			(**mySegmentHdl).UUCD_SEGMENT_DataType = kUUCD_DATATYPE_jpg;
		if (*((OSType *)myDstBytes) == 'GIF8')
			(**mySegmentHdl).UUCD_SEGMENT_DataType = kUUCD_DATATYPE_gif;
		};

	myDataHandle = (**mySegmentHdl).UUCD_SEGMENT_DataHdl;
	myOSErr = PtrAndHand((Ptr)myDstBytes, myDataHandle, (long)myDstBytesLen);
	(**mySegmentHdl).UUCD_SEGMENT_DataSize += (long)myDstBytesLen;

	if ((**mySegmentHdl).UUCD_SEGMENT_FrstSrcLineNum == 0L)
		(**mySegmentHdl).UUCD_SEGMENT_FrstSrcLineNum = theFileLineNum;
	(**mySegmentHdl).UUCD_SEGMENT_LastSrcLineNum = theFileLineNum;

	HUnlock((Handle)mySegmentHdl);

	} /* UUCD_AddUUToSegment */

/*------------------------------------------------------------------------------
//
//	UUCD_AddHQXToSegment
//
*/

void UUCD_AddHQXToSegment (short theSegmentNum, char *theLinePtr, short theLineLen, long theFileLineNum)

	{
	OSErr			  myOSErr;
	UUCD_SegmentHdl	  mySegmentHdl;
	char				  myHQXChars[80];
	short			  myHQXCharsLen;
	short			  myHQXHeaderLen = 0;
	Handle			  myDataHandle;

	if (theLineLen == 0) return;

	mySegmentHdl = UUCD_GetSegmentHdl(theSegmentNum);
	if (mySegmentHdl == NULL) return;

	HLock((Handle)mySegmentHdl);
	myDataHandle = (**mySegmentHdl).UUCD_SEGMENT_DataHdl;
	HUnlock((Handle)mySegmentHdl);

	if (*theLinePtr == ':')
		{
		// First add the '(This file must be converted with BinHex 4.0)' line
		for (myHQXHeaderLen = 0; gHQXHeadLine[myHQXHeaderLen] != '\0'; myHQXHeaderLen++) {};
		BlockMove((Ptr)gHQXHeadLine, (Ptr)myHQXChars, (long)myHQXHeaderLen);
		myHQXChars[myHQXHeaderLen++] = 0x0D;
		myOSErr = PtrAndHand((Ptr)myHQXChars, myDataHandle, (long)myHQXHeaderLen);
		};

	myHQXCharsLen = theLineLen;
	if (myHQXCharsLen > 64) myHQXCharsLen = 64;
	BlockMove((Ptr)theLinePtr, (Ptr)myHQXChars, (long)myHQXCharsLen);
	myHQXChars[myHQXCharsLen++] = 0x0D;
	myOSErr = PtrAndHand((Ptr)myHQXChars, myDataHandle, (long)myHQXCharsLen);

	HLock((Handle)mySegmentHdl);
	(**mySegmentHdl).UUCD_SEGMENT_DataSize += (long)myHQXCharsLen + (long)myHQXHeaderLen;
	if ((**mySegmentHdl).UUCD_SEGMENT_FrstSrcLineNum == 0L)
		(**mySegmentHdl).UUCD_SEGMENT_FrstSrcLineNum = theFileLineNum;
	(**mySegmentHdl).UUCD_SEGMENT_LastSrcLineNum = theFileLineNum;
	HUnlock((Handle)mySegmentHdl);

	} /* UUCD_AddHQXToSegment */

/*------------------------------------------------------------------------------
//
//	UUCD_AddB64ToSegment
//
*/

short UUCD_AddB64ToSegment_FastGetSrcCharValue (char theSrcChar);
short UUCD_AddB64ToSegment_FastGetSrcCharValue (char theSrcChar)

	{

	if ((theSrcChar >= 'A') && (theSrcChar <= 'Z')) return (short)(theSrcChar - 'A');
	if ((theSrcChar >= 'a') && (theSrcChar <= 'z')) return 26 + (short)(theSrcChar - 'a');
	if ((theSrcChar >= '0') && (theSrcChar <= '9')) return 52 + (short)(theSrcChar - '0');
	if (theSrcChar == '+') return 62;
	if (theSrcChar == '/') return 63;
	return -1;

	} /* UUCD_AddB64ToSegment_FastGetSrcCharValue */

void UUCD_AddB64ToSegment (short theSegmentNum, char *theLinePtr, short theLineLen, long theFileLineNum)

	{
	OSErr			  myOSErr;
	UUCD_SegmentHdl	  mySegmentHdl;
	Handle			  myDataHandle;
	short			  mySrcCharNum;
	short			  myDstCharNum;
	uchar			  myDstChars[64];
	short			  mySrcCharValue;
	uchar			  mySrcChar;
	ulong			  myDstCharValue;
	ulong			  myDstAccu;
	ulong			  myDstShift;

	if (theLineLen == 0) return;

	mySegmentHdl = UUCD_GetSegmentHdl(theSegmentNum);
	if (mySegmentHdl == NULL) return;

	/*   +------+ +------+ +------+ +------+   */
	/*   ..765432 ..107654 ..321076 ..543210   */
	/*   +------+ +------+ +------+ +------+   */

	for (myDstCharNum = 0; myDstCharNum < sizeof(myDstChars); myDstCharNum++)
		myDstChars[myDstCharNum] = 0x00;

	myDstCharNum = 0;
	myDstAccu = 0L;
	myDstShift = 0L;

	for (mySrcCharNum = 0; mySrcCharNum < theLineLen; mySrcCharNum++)
		{
		mySrcChar = theLinePtr[mySrcCharNum];
		if (mySrcChar == '=') break;

		mySrcCharValue = UUCD_AddB64ToSegment_FastGetSrcCharValue(mySrcChar);
		if (mySrcCharValue < 0) continue;

		myDstAccu <<= 6;
		myDstShift += 6;
		myDstAccu |= (ulong)mySrcCharValue;
		if (myDstShift >= 8)
			{
			myDstShift -= 8;
			myDstCharValue = myDstAccu >> myDstShift;
			myDstChars[myDstCharNum++] = (unsigned char)myDstCharValue & 0xFFL;
			};

		if (myDstCharNum >= sizeof(myDstChars)) break;
		};

	HLock((Handle)mySegmentHdl);

	myDataHandle = (**mySegmentHdl).UUCD_SEGMENT_DataHdl;

	if (myDstCharNum > 0)
		{
		if ((**mySegmentHdl).UUCD_SEGMENT_DataSize == 0L)
			{
			if (*((OSType *)&myDstChars[6]) == 'JFIF')
				(**mySegmentHdl).UUCD_SEGMENT_DataType = kUUCD_DATATYPE_jpg;
			if (*((OSType *)myDstChars) == 'GIF8')
				(**mySegmentHdl).UUCD_SEGMENT_DataType = kUUCD_DATATYPE_gif;
			};

		myOSErr = PtrAndHand((Ptr)myDstChars, myDataHandle, (long)myDstCharNum);
		(**mySegmentHdl).UUCD_SEGMENT_DataSize += (long)myDstCharNum;
		};

	if ((**mySegmentHdl).UUCD_SEGMENT_FrstSrcLineNum == 0L)
		(**mySegmentHdl).UUCD_SEGMENT_FrstSrcLineNum = theFileLineNum;
	(**mySegmentHdl).UUCD_SEGMENT_LastSrcLineNum = theFileLineNum;

	HUnlock((Handle)mySegmentHdl);

	} /* UUCD_AddB64ToSegment */

/*------------------------------------------------------------------------------
//
//	UUCD_DrawSegments
//
*/

void UUCD_DrawSegments (void)

	{
	short			  myNbOfSegments = UUCD_GetNbOfSegments();
	short			  myLocalSegmentNum;
	short			  mySegmentNum;
	Rect				  myEraseRect;
	GrafPtr			  mySavePort;

	for (myLocalSegmentNum = 0;
		myLocalSegmentNum < UUCD_GetNbOfVisibleSegments();
		myLocalSegmentNum++)
		{
		mySegmentNum = UUCD_GetTopSegmentNum() + myLocalSegmentNum;
		if (!UUCD_DrawSegment(mySegmentNum, NULL)) break;
		};

	myEraseRect = *UUCD_GetSegListViewRect();
	myEraseRect.top += (myNbOfSegments - UUCD_GetTopSegmentNum()) * kUUCD_SegmentHeight;
	if (myEraseRect.top < (UUCD_GetSegListViewRect())->top) myEraseRect.top = (UUCD_GetSegListViewRect())->top;
	if (myEraseRect.top > (UUCD_GetSegListViewRect())->bottom) return;

	GetPort(&mySavePort);
	SetPort(UUCD_GetUUCDWindowPtr());
	EraseRect(&myEraseRect);
	SetPort(mySavePort);

	} /* UUCD_DrawSegments */

/*------------------------------------------------------------------------------
//
//	UUCD_DrawSegment
//
*/

Boolean UUCD_DrawSegment (short theSegmentNum, PixPatHandle theBgPixPat)

	{
	GrafPtr			  mySavePort;
	GWorldPtr			  mySaveGWorldPtr;
	GDHandle			  mySaveDevice;
	Rect				  mySegmentRect;
	Rect				  mySegmentDrawRect;
	PixPatHandle		  myPixPatHandle;
	CICNHandle		  myCICNHandle;
	Rect				  myCIconRect;
	Str255			  myTempStr255;
	long				  mySegmentDataSize;
	short			  myStringWidth;
	UUCD_SegmentState	  mySegmentState;

	if (!UUCD_GetSegmentRect(theSegmentNum, &mySegmentRect)) return false;

	mySegmentState = UUCD_GetSegmentState(theSegmentNum);

	/*---- Segment GWorldPtr ----*/

	mySegmentDrawRect = mySegmentRect;
	OffsetRect(&mySegmentDrawRect, - mySegmentRect.left, - mySegmentRect.top);

	if (gUUCDSegmentGWorldPtr == NULL)
		{
		QDErr			  myQDErr;

		GetGWorld(&mySaveGWorldPtr, &mySaveDevice);
		myQDErr = NewGWorld(&gUUCDSegmentGWorldPtr, 8, &mySegmentDrawRect,
			(CTabHandle)NULL, (GDHandle)NULL, 0);
		if (myQDErr == paramErr) SysBeep(1);
		SetGWorld(mySaveGWorldPtr, mySaveDevice);
		};

	/*---- Set Port ----*/

	if (gUUCDSegmentGWorldPtr != NULL)
		{
		LockPixels(gUUCDSegmentGWorldPtr->portPixMap);
		GetGWorld(&mySaveGWorldPtr, &mySaveDevice);
		SetGWorld(gUUCDSegmentGWorldPtr, (GDHandle)NULL);
		EraseRect(&gUUCDSegmentGWorldPtr->portRect);
		}
	else
		{
		GetPort(&mySavePort);
		SetPort(UUCD_GetUUCDWindowPtr());

		mySegmentDrawRect = mySegmentRect;
		};

	/*---- Background ----*/

	if (theBgPixPat == NULL)
		{
		myPixPatHandle = UUCD_GetSegmentBgPixPat(theSegmentNum,
			((theSegmentNum == INFO_GetInfoSegmentNum()) ? kUUCD_SEGBG_Info : kUUCD_SEGBG_Standard));
		}
	else myPixPatHandle = theBgPixPat;

	if (myPixPatHandle != NULL)
		{
		FillCRect(&mySegmentDrawRect, myPixPatHandle);
		}
	else EraseRect(&mySegmentDrawRect);

	/*---- Mark Icon ----*/

	if ((myCICNHandle = UUCD_GetMarkIcon(UUCD_GetSegmentMark(theSegmentNum))) != NULL)
		{
		UUCD_GetSegmentMarkIconRect(&mySegmentDrawRect, &myCIconRect);
		CICN_PlotIcon(&myCIconRect, myCICNHandle);
		};

	/*---- State Icon ----*/

	myCICNHandle = UUCD_GetSegStateIcon(mySegmentState);
	UUCD_GetSegmentStateIconRect(&mySegmentDrawRect, &myCIconRect);

	if (myCICNHandle != NULL)
		{
		CICN_PlotIcon(&myCIconRect, myCICNHandle);
		}
	else FrameRect(&myCIconRect);

	/*---- Save/View Icon ----*/

	switch (mySegmentState)
		{
	case kUUCD_SEGSTATE_LinkedBegin:
	case kUUCD_SEGSTATE_Complete:

		UUCD_GetSegmentSaveIconRect(&mySegmentDrawRect, &myCIconRect);

		if (PREF_IsAutoOpenEnabled())
			{
			CICN_PlotIcon(&myCIconRect, UUCD_GetViewIcon(kIconUp));
			}
		else CICN_PlotIcon(&myCIconRect, UUCD_GetSaveIcon(kIconUp));

		break;
		};

	/*---- Info Icon ----*/

	UUCD_GetSegmentInfoIconRect(&mySegmentDrawRect, &myCIconRect);

	if ((myCICNHandle = UUCD_GetInfoIcon(kIconUp)) != NULL)
		{
		CICN_PlotIcon(&myCIconRect, myCICNHandle);
		}
	else FrameRect(&myCIconRect);

	UUCD_GetSegmentTypeIconRect(&mySegmentDrawRect, &myCIconRect);
	myCICNHandle = UUCD_GetSegTypeIcon(UUCD_GetSegmentType(theSegmentNum));
	if (myCICNHandle != NULL) CICN_PlotIcon(&myCIconRect, myCICNHandle);

	/*---- Display the FileName, if any ----*/

	if (UUCD_GetSegmentDstFileName(theSegmentNum, myTempStr255))
		{
		TextFont(kFontIDGeneva);
		TextSize(9);
		TextFace(0);
		TruncString(kUUCD_SegmentFile_Width - 10, myTempStr255, smTruncMiddle);
		MoveTo(mySegmentDrawRect.left + kUUCD_SegmentFile_HOffset, mySegmentDrawRect.bottom - 4);
		DrawString(myTempStr255);
		};

	/*---- Display the Name of the Segment ----*/

	UUCD_GetSegmentName(theSegmentNum, myTempStr255);
	if (myTempStr255[0] != 0x00)
		{
		short			  myMaxWidth;
		TextFont(kFontIDGeneva);
		TextSize(9);
		TextFace(0);
		myMaxWidth = mySegmentDrawRect.right - mySegmentDrawRect.left;
		myMaxWidth -= kUUCD_SegmentName_HOffset;
		myMaxWidth -= kUUCD_InfoIcon_Width;
		TruncString(myMaxWidth - 10, myTempStr255, smTruncMiddle);
		MoveTo(mySegmentDrawRect.left + kUUCD_SegmentName_HOffset, mySegmentDrawRect.bottom - 4);
		DrawString(myTempStr255);
		};

	/*---- Display the Size of the Data ----*/

	mySegmentDataSize = UUCD_GetSegmentDataSize(theSegmentNum);
	if (mySegmentDataSize != 0L)
		{
		TextFont(kFontIDGeneva);
		TextSize(9);
		TextFace(0);
		if (mySegmentDataSize >= 1024L)
			{
			TextFace(bold);
			mySegmentDataSize /= 1024L;
			NumToString(mySegmentDataSize, myTempStr255);
			myTempStr255[++myTempStr255[0]] = 'K';
			}
		else NumToString(mySegmentDataSize, myTempStr255);
		myStringWidth = StringWidth(myTempStr255);
		MoveTo(mySegmentDrawRect.left + kUUCD_TypeIcon_HOffset - myStringWidth - 1,
			mySegmentDrawRect.bottom - 4);
		DrawString(myTempStr255);
		};

	/*---- Restore Port ----*/

	if (gUUCDSegmentGWorldPtr != NULL)
		{
		SetGWorld(mySaveGWorldPtr, mySaveDevice);
		SetPort(UUCD_GetUUCDWindowPtr());
		ForeColor(blackColor);
		BackColor(whiteColor);
		CopyBits( (BitMap *)*(gUUCDSegmentGWorldPtr->portPixMap),
				&((GrafPtr)UUCD_GetUUCDWindowPtr())->portBits,
				&mySegmentDrawRect, &mySegmentRect, srcCopy, NULL);
		UnlockPixels(gUUCDSegmentGWorldPtr->portPixMap);
		}
	else SetPort(mySavePort);

	return true;

	} /* UUCD_DrawSegment */

/*------------------------------------------------------------------------------
//
//	UUCD_GetSegmentRect
//
*/

Boolean UUCD_GetSegmentRect (short theSegmentNum, Rect *theRectPtr)

	{
	Rect				  mySegmentRect;
	short			  myNbOfSegments = UUCD_GetNbOfSegments();
	short			  myLocalSegmentNum;

	SetRect(theRectPtr, 0, 0, 0, 0);

	if (theSegmentNum < gUUCDTopSegmentNum) return false;
	if (theSegmentNum >= myNbOfSegments) return false;

	myLocalSegmentNum = theSegmentNum - gUUCDTopSegmentNum;
	if (myLocalSegmentNum >= gUUCDNbOfVisibleSegments) return false;

	mySegmentRect = gUUCDSegListViewRect;
	mySegmentRect.top += myLocalSegmentNum * kUUCD_SegmentHeight;
	mySegmentRect.bottom = mySegmentRect.top + kUUCD_SegmentHeight;
	*theRectPtr = mySegmentRect;
	return true;

	} /* UUCD_GetSegmentRect */

/*------------------------------------------------------------------------------
//
//	UUCD_GetSegmentStateIconRect
//
*/

void UUCD_GetSegmentStateIconRect (Rect *theSegmentRectPtr, Rect *theIconRectPtr)

	{

	*theIconRectPtr = *theSegmentRectPtr;
	theIconRectPtr->left += kUUCD_StateIcon_HOffset;
	theIconRectPtr->right = theIconRectPtr->left + kUUCD_StateIcon_Width;

	} /* UUCD_GetSegmentStateIconRect */

/*------------------------------------------------------------------------------
//
//	UUCD_GetSegmentSaveIconRect
//
*/

void UUCD_GetSegmentSaveIconRect (Rect *theSegmentRectPtr, Rect *theIconRectPtr)

	{

	theIconRectPtr->top = theSegmentRectPtr->top + 4;
	theIconRectPtr->left = theSegmentRectPtr->left + kUUCD_StateIcon_HOffset + 3;
	theIconRectPtr->bottom = theIconRectPtr->top + 8;
	theIconRectPtr->right = theIconRectPtr->left + 8;

	} /* UUCD_GetSegmentSaveIconRect */

/*------------------------------------------------------------------------------
//
//	UUCD_GetSegmentInfoIconRect
//
*/

void UUCD_GetSegmentInfoIconRect (Rect *theSegmentRectPtr, Rect *theIconRectPtr)

	{

	*theIconRectPtr = *theSegmentRectPtr;
	theIconRectPtr->left = theSegmentRectPtr->right - kUUCD_InfoIcon_Width;

	} /* UUCD_GetSegmentInfoIconRect */

/*------------------------------------------------------------------------------
//
//	UUCD_GetSegmentMarkIconRect
//
*/

void UUCD_GetSegmentMarkIconRect (Rect *theSegmentRectPtr, Rect *theIconRectPtr)

	{

	*theIconRectPtr = *theSegmentRectPtr;
	theIconRectPtr->left += kUUCD_MarkIcon_HOffset;
	theIconRectPtr->right = theIconRectPtr->left + kUUCD_MarkIcon_Width;

	} /* UUCD_GetSegmentMarkIconRect */

/*------------------------------------------------------------------------------
//
//	UUCD_GetSegmentTypeIconRect
//
*/

void UUCD_GetSegmentTypeIconRect (Rect *theSegmentRectPtr, Rect *theIconRectPtr)

	{

	*theIconRectPtr = *theSegmentRectPtr;
	theIconRectPtr->left += kUUCD_TypeIcon_HOffset;
	theIconRectPtr->right = theIconRectPtr->left + kUUCD_TypeIcon_Width;

	} /* UUCD_GetSegmentTypeIconRect */

/*------------------------------------------------------------------------------
//
//	UUCD_SegmentHasSaveIcon
//
*/

Boolean UUCD_SegmentHasSaveIcon (short theSegmentNum)

	{

	switch (UUCD_GetSegmentState(theSegmentNum))
		{
	case kUUCD_SEGSTATE_LinkedBegin:
	case kUUCD_SEGSTATE_Complete:
		return true;
		};

	return false;

	} /* UUCD_SegmentHasSaveIcon */

/*------------------------------------------------------------------------------
//
//	UUCD_GetSegStateIcon
//
*/

CICNHandle UUCD_GetSegStateIcon (UUCD_SegmentState theSegmentState)

	{

	switch (theSegmentState)
		{
	case kUUCD_SEGSTATE_Begin:
		if (gUUCD_CICON_Begin == NULL)
			gUUCD_CICON_Begin = CICN_GetIcon(kUUCD_CICONID_Begin);
		return gUUCD_CICON_Begin;

	case kUUCD_SEGSTATE_Body:
		if (gUUCD_CICON_Body == NULL)
			gUUCD_CICON_Body = CICN_GetIcon(kUUCD_CICONID_Body);
		return gUUCD_CICON_Body;

	case kUUCD_SEGSTATE_End:
		if (gUUCD_CICON_End == NULL)
			gUUCD_CICON_End = CICN_GetIcon(kUUCD_CICONID_End);
		return gUUCD_CICON_End;

	case kUUCD_SEGSTATE_LinkedBegin:
		if (gUUCD_CICON_LinkedBegin == NULL)
			gUUCD_CICON_LinkedBegin = CICN_GetIcon(kUUCD_CICONID_LinkedBegin);
		return gUUCD_CICON_LinkedBegin;

	case kUUCD_SEGSTATE_LinkedBody:
		if (gUUCD_CICON_LinkedBody == NULL)
			gUUCD_CICON_LinkedBody = CICN_GetIcon(kUUCD_CICONID_LinkedBody);
		return gUUCD_CICON_LinkedBody;

	case kUUCD_SEGSTATE_LinkedEnd:
		if (gUUCD_CICON_LinkedEnd == NULL)
			gUUCD_CICON_LinkedEnd = CICN_GetIcon(kUUCD_CICONID_LinkedEnd);
		return gUUCD_CICON_LinkedEnd;

	case kUUCD_SEGSTATE_Complete:
		if (gUUCD_CICON_Complete == NULL)
			gUUCD_CICON_Complete = CICN_GetIcon(kUUCD_CICONID_Complete);
		return gUUCD_CICON_Complete;
		};

	return NULL;

	} /* UUCD_GetSegStateIcon */

/*------------------------------------------------------------------------------
//
//	UUCD_GetSegTypeIcon
//
*/

CICNHandle UUCD_GetSegTypeIcon (UUCD_SegmentType theSegmentType)

	{

	switch (theSegmentType)
		{
	case kUUCD_SEGTYPE_HQX:
		if (gUUCD_CICON_HQX == NULL)
			gUUCD_CICON_HQX = CICN_GetIcon(kUUCD_CICONID_HQX);
		return gUUCD_CICON_HQX;

	case kUUCD_SEGTYPE_B64:
		if (gUUCD_CICON_B64 == NULL)
			gUUCD_CICON_B64 = CICN_GetIcon(kUUCD_CICONID_B64);
		return gUUCD_CICON_B64;
		};

	return NULL;

	} /* UUCD_GetSegTypeIcon */

/*------------------------------------------------------------------------------
//
//	UUCD_GetSegmentBgPixPat
//
*/

PixPatHandle UUCD_GetSegmentBgPixPat (short theSegmentNum, UUCD_SegmentBg theSegmentBg)

	{

	switch (theSegmentBg)
		{
	case kUUCD_SEGBG_Info:
		if (gUUCDBgInfoLinePixPatHandle == NULL)
			gUUCDBgInfoLinePixPatHandle = GetPixPat(1028);
		return gUUCDBgInfoLinePixPatHandle;

	case kUUCD_SEGBG_Source:
		if (gUUCDBgSourceLinePixPatHandle == NULL)
			gUUCDBgSourceLinePixPatHandle = GetPixPat(1026);
		return gUUCDBgSourceLinePixPatHandle;

	case kUUCD_SEGBG_Target:
		if (gUUCDBgTargetLinePixPatHandle == NULL)
			gUUCDBgTargetLinePixPatHandle = GetPixPat(1027);
		return gUUCDBgTargetLinePixPatHandle;

	default:
		if ((theSegmentNum % 2) == 0)
			{
			if (gUUCDBgEvenLinePixPatHandle == NULL)
				gUUCDBgEvenLinePixPatHandle = GetPixPat(1024);
			return gUUCDBgEvenLinePixPatHandle;
			};
		if (gUUCDBgOddLinePixPatHandle == NULL)
			gUUCDBgOddLinePixPatHandle = GetPixPat(1025);
		return gUUCDBgOddLinePixPatHandle;
		};

	} /* UUCD_GetSegmentBgPixPat */

/*------------------------------------------------------------------------------
//
//	UUCD_GetMarkIcon
//
*/

CICNHandle UUCD_GetMarkIcon (UUCD_SegmentMark theSegmentMark)

	{

	if (theSegmentMark == kUUCD_MARK_Check)
		{
		if (gUUCD_CICON_MarkCheck == NULL)
			gUUCD_CICON_MarkCheck = CICN_GetIcon(kUUCD_CICONID_MarkCheck);
		return gUUCD_CICON_MarkCheck;
		};

	if (gUUCD_CICON_MarkNone == NULL)
		gUUCD_CICON_MarkNone = CICN_GetIcon(kUUCD_CICONID_MarkNone);
	return gUUCD_CICON_MarkNone;

	} /* UUCD_GetMarkIcon */

/*------------------------------------------------------------------------------
//
//	UUCD_GetSaveIcon
//
*/

CICNHandle UUCD_GetSaveIcon (IconState theIconState)

	{

	if (theIconState == kIconUp)
		{
		if (gUUCDSaveIconUpCIconHandle == NULL)
			gUUCDSaveIconUpCIconHandle = CICN_GetIcon(kUUCDSaveIconUpCIconID);
		return gUUCDSaveIconUpCIconHandle;
		};

	if (gUUCDSaveIconDownCIconHandle == NULL)
		gUUCDSaveIconDownCIconHandle = CICN_GetIcon(kUUCDSaveIconDownCIconID);
	return gUUCDSaveIconDownCIconHandle;

	} /* UUCD_GetSaveIcon */

/*------------------------------------------------------------------------------
//
//	UUCD_GetViewIcon
//
*/

CICNHandle UUCD_GetViewIcon (IconState theIconState)

	{

	if (theIconState == kIconUp)
		{
		if (gUUCDViewIconUpCIconHandle == NULL)
			gUUCDViewIconUpCIconHandle = CICN_GetIcon(kUUCDViewIconUpCIconID);
		return gUUCDViewIconUpCIconHandle;
		};

	if (gUUCDViewIconDownCIconHandle == NULL)
		gUUCDViewIconDownCIconHandle = CICN_GetIcon(kUUCDViewIconDownCIconID);
	return gUUCDViewIconDownCIconHandle;

	} /* UUCD_GetViewIcon */

/*------------------------------------------------------------------------------
//
//	UUCD_GetInfoIcon
//
*/

CICNHandle UUCD_GetInfoIcon (IconState theIconState)

	{

	if (theIconState == kIconUp)
		{
		if (gUUCDInfoIconUpCIconHandle == NULL)
			gUUCDInfoIconUpCIconHandle = CICN_GetIcon(kUUCDInfoIconUpCIconID);
		return gUUCDInfoIconUpCIconHandle;
		};

	if (gUUCDInfoIconDownCIconHandle == NULL)
		gUUCDInfoIconDownCIconHandle = CICN_GetIcon(kUUCDInfoIconDownCIconID);
	return gUUCDInfoIconDownCIconHandle;

	} /* UUCD_GetInfoIcon */

/*------------------------------------------------------------------------------
//
//	UUCD_Init/ExitBlobs
//
*/

void UUCD_InitBlobs (void)

	{
	BLOB_Err			  myBlobErr;

	myBlobErr = BLOB_CreateBase(kBlobsFileName);
	myBlobErr = BLOB_OpenBase(&gBlobBasePtr, kBlobsFileName);

	} // UUCD_InitBlobs

void UUCD_ExitBlobs (void)

	{

	BLOB_CloseBase(gBlobBasePtr);
	gBlobBasePtr = NULL;
	BLOB_DeleteBase(kBlobsFileName);

	} // UUCD_ExitBlobs

/*------------------------------------------------------------------------------
//
//	UUCD_GetBlobBasePtr
//
*/

BLOB_BasePtr UUCD_GetBlobBasePtr (void)

	{

	return gBlobBasePtr;

	} // UUCD_GetBlobBasePtr

/*------------------------------------------------------------------------------
//
//	UUCD_BlobSegment
//
*/

void UUCD_BlobSegment (short theSegmentNum)

	{
	BLOB_Err			  myBlobErr;
	UUCD_SegmentHdl	  mySegmentHdl;
	Handle			  mySegmentDataHdl;
	long				  mySegmentDataSize = 0L;
	unsigned long		  mySegmentBlobID = 0L;

	mySegmentHdl = UUCD_GetSegmentHdl(theSegmentNum);
	if (mySegmentHdl == NULL) return;

	HLock((Handle)mySegmentHdl);
	mySegmentDataHdl = (**mySegmentHdl).UUCD_SEGMENT_DataHdl;
	if (mySegmentDataHdl != NULL)
		{
		mySegmentDataSize = GetHandleSize(mySegmentDataHdl);
		HLock(mySegmentDataHdl);
		myBlobErr = BLOB_WriteFile(gBlobBasePtr, &mySegmentBlobID,
			*mySegmentDataHdl, mySegmentDataSize);
		if (myBlobErr == noErr)
			{
			(**mySegmentHdl).UUCD_SEGMENT_BlobID = mySegmentBlobID;
			(**mySegmentHdl).UUCD_SEGMENT_DataHdl = NULL;
			};
		HUnlock(mySegmentDataHdl);
		if ((**mySegmentHdl).UUCD_SEGMENT_DataHdl == NULL)
			DisposeHandle(mySegmentDataHdl);
		};
	HUnlock((Handle)mySegmentHdl);

	} // UUCD_BlobSegment

/*------------------------------------------------------------------------------
//
//	That's All, Folks !...
//
*/
