/*------------------------------------------------------------------------------
//
//	blobs.c
//
*/

#include "LowMem.h"
#include "Script.h"

#include "util.h"

#include "blobs.h"

/*------------------------------------------------------------------------------
//
//	BLOB_CreateBase
//
*/

BLOB_Err BLOB_CreateBase (char *theBaseName)

	{
	OSErr			  myOSErr;
	FSSpec			  myBaseFSSpec;

	myOSErr = BLOB_MAC_GetFileFSSpec(&myBaseFSSpec, theBaseName);
	if (myOSErr == noErr) return BLOB_ERR_BaseAlreadyExists;
	if (myOSErr == fnfErr) myOSErr = noErr;
	if (myOSErr != noErr) return BLOB_ERR_OSErr;

	myOSErr = FSpCreate(&myBaseFSSpec, 'UUCD', 'BLOB', smSystemScript);
	if (myOSErr != noErr) return BLOB_ERR_OSErr;

	return BLOB_ERR_NoErr;

	} // BLOB_CreateBase

/*------------------------------------------------------------------------------
//
//	BLOB_DeleteBase
//
*/

BLOB_Err BLOB_DeleteBase (char *theBaseName)

	{
	OSErr			  myOSErr;
	FSSpec			  myBaseFSSpec;

	myOSErr = BLOB_MAC_GetFileFSSpec(&myBaseFSSpec, theBaseName);
	if (myOSErr != noErr) return BLOB_ERR_OSErr;

	myOSErr = FSpDelete(&myBaseFSSpec);
	if (myOSErr != noErr) return BLOB_ERR_OSErr;

	return BLOB_ERR_NoErr;

	} // BLOB_DeleteBase

/*------------------------------------------------------------------------------
//
//	BLOB_OpenBase
//
*/

BLOB_Err BLOB_OpenBase (BLOB_BasePtr *theBasePtrPtr, char *theBaseName)

	{
	OSErr			  myOSErr;
	BLOB_Err			  myBlobErr;
	BLOB_BasePtr		  myBasePtr;
	short			  myBaseRefNum;
	FSSpec			  myBaseFSSpec;
	long				  myBaseEOF;
	long				  myWriteCount;

	*theBasePtrPtr = NULL;

	myBasePtr = (BLOB_BasePtr)NewPtr((long)sizeof(BLOB_Base));
	if (myBasePtr == NULL) return BLOB_ERR_MemErr;

	myBasePtr->BLOB_BASE_FileRefNum = -1;
	myBasePtr->BLOB_BASE_BlockInfos = NULL;

	myOSErr = BLOB_MAC_GetFileFSSpec(&myBaseFSSpec, theBaseName);
	if (myOSErr != noErr) return (BLOB_Err)myOSErr;

	myBasePtr->BLOB_BASE_FSSpec = myBaseFSSpec;

	myOSErr = FSpOpenDF(&myBaseFSSpec, fsRdWrPerm, &myBaseRefNum);
	if (myOSErr != noErr) return (BLOB_Err)myOSErr;

	myBasePtr->BLOB_BASE_FileRefNum = myBaseRefNum;

	myOSErr = GetEOF(myBaseRefNum, &myBaseEOF);
	if (myOSErr != noErr) return (BLOB_Err)myOSErr;

	if (myBaseEOF < (long)sizeof(BLOB_BaseHeader))
		{
		BLOB_MemClear(&myBasePtr->BLOB_BASE_Header, (long)sizeof(BLOB_BaseHeader));

		myBasePtr->BLOB_BASE_Header.BLOB_BASE_HEADER_Magic[0] = 'U';
		myBasePtr->BLOB_BASE_Header.BLOB_BASE_HEADER_Magic[1] = 'U';
		myBasePtr->BLOB_BASE_Header.BLOB_BASE_HEADER_Magic[2] = 'C';
		myBasePtr->BLOB_BASE_Header.BLOB_BASE_HEADER_Magic[3] = 'D';
		myBasePtr->BLOB_BASE_Header.BLOB_BASE_HEADER_Magic[4] = 'B';
		myBasePtr->BLOB_BASE_Header.BLOB_BASE_HEADER_Magic[5] = 'L';
		myBasePtr->BLOB_BASE_Header.BLOB_BASE_HEADER_Magic[6] = 'O';
		myBasePtr->BLOB_BASE_Header.BLOB_BASE_HEADER_Magic[7] = 'B';
		myBasePtr->BLOB_BASE_Header.BLOB_BASE_HEADER_NbOfBlocks = 0L;
		myBasePtr->BLOB_BASE_Header.BLOB_BASE_HEADER_FirstFreeBlock = 0L;

		myOSErr = SetFPos(myBaseRefNum, fsFromStart, 0L);
		if (myOSErr != noErr) return (BLOB_Err)myOSErr;
		myWriteCount = (long)sizeof(BLOB_BaseHeader);
		myOSErr = FSWrite(myBaseRefNum, &myWriteCount, &myBasePtr->BLOB_BASE_Header);
		if (myOSErr != noErr) return (BLOB_Err)myOSErr;
		};

	myBlobErr = BLOB_ReadBaseHeader(myBasePtr);
	if (myBlobErr != BLOB_ERR_NoErr) return myBlobErr;

	myBlobErr = BLOB_ReadBaseBlockInfos(myBasePtr);
	if (myBlobErr != BLOB_ERR_NoErr) return myBlobErr;

	*theBasePtrPtr = myBasePtr;

	return BLOB_ReadBaseHeader(myBasePtr);

	} // BLOB_OpenBase

/*------------------------------------------------------------------------------
//
//	BLOB_CloseBase
//
*/

BLOB_Err BLOB_CloseBase (BLOB_BasePtr theBasePtr)

	{
	BLOB_Err			  myBlobErr;

	myBlobErr = BLOB_WriteBaseBlockInfos(theBasePtr);
	if (myBlobErr != BLOB_ERR_NoErr) return myBlobErr;

	myBlobErr = BLOB_WriteBaseHeader(theBasePtr);
	if (myBlobErr != BLOB_ERR_NoErr) return myBlobErr;

	FSClose(theBasePtr->BLOB_BASE_FileRefNum);

	DisposePtr((Ptr)theBasePtr);

	return BLOB_ERR_NoErr;

	} // BLOB_CloseBase

/*------------------------------------------------------------------------------
//
//	BLOB_GetBaseNbOfBlocks
//
*/

unsigned long BLOB_GetBaseNbOfBlocks (BLOB_BasePtr theBasePtr)

	{

	return theBasePtr->BLOB_BASE_Header.BLOB_BASE_HEADER_NbOfBlocks;

	} // BLOB_GetBaseNbOfBlocks

/*------------------------------------------------------------------------------
//
//	BLOB_ReadFile
//
*/

BLOB_Err BLOB_ReadFile (BLOB_BasePtr theBasePtr, unsigned long theFileID,
	void *theBufferPtr)

	{
	BLOB_Err			  myBlobErr;
	unsigned long		  myBlockID = theFileID;
	unsigned long		  myNextBlockID;
	unsigned short		  myBlockSize;
	char				  myTempBlock[kBLOB_BlockSize];
	char				 *myBlockDataPtr;

	myBlockDataPtr = (char *)theBufferPtr;

	while (myBlockID != 0L)
		{
		myBlobErr = BLOB_GetBlockInfo(theBasePtr,
			myBlockID, &myNextBlockID, &myBlockSize);
		if (myBlobErr != BLOB_ERR_NoErr) return myBlobErr;
		if (myBlockSize == 0) myBlockSize = kBLOB_BlockSize;

		myBlobErr = BLOB_ReadBlock(theBasePtr, myBlockID, myTempBlock);
		if (myBlobErr != BLOB_ERR_NoErr) return myBlobErr;

		BlockMove(myTempBlock, myBlockDataPtr, myBlockSize);

		myBlockDataPtr += myBlockSize;
		myBlockID = myNextBlockID;
		};

	return BLOB_ERR_NoErr;

	} // BLOB_ReadFile

/*------------------------------------------------------------------------------
//
//	BLOB_WriteFile
//
*/

BLOB_Err BLOB_WriteFile (BLOB_BasePtr theBasePtr, unsigned long *theFileIDPtr,
	void *theDataPtr, unsigned long theDataSize)

	{
	BLOB_Err			  myBlobErr;
	unsigned long		  myNbOfBlocksNeeded;
	unsigned long		  myFreeBlocksNeeded;
	unsigned long		  myBlockIndex;
	unsigned long		  myBlockID = 0L;
	unsigned long		  myPrevBlockID;
	unsigned long		  myNextBlockID;
	char				 *myBlockDataPtr;
	unsigned short		  myLastBlockSize;
	unsigned short		  myBlockSize;

	if (theDataSize == 0L) return BLOB_ERR_ZeroDataSize;

	myNbOfBlocksNeeded = (theDataSize / (long)kBLOB_BlockSize);
	myLastBlockSize = (unsigned short)(theDataSize % (long)kBLOB_BlockSize);
	if (myLastBlockSize != 0) myNbOfBlocksNeeded++;

	for (myBlockIndex = 0L; myBlockIndex < myNbOfBlocksNeeded; myBlockIndex++)
		{
		myPrevBlockID = myBlockID;
		myBlockID = theBasePtr->BLOB_BASE_Header.BLOB_BASE_HEADER_FirstFreeBlock;
		if (myBlockID == 0L)
			{
			myFreeBlocksNeeded = myNbOfBlocksNeeded - myBlockIndex;
			myBlobErr = BLOB_CreateFreeBlocks(theBasePtr, myFreeBlocksNeeded);
			if (myBlobErr != BLOB_ERR_NoErr) return myBlobErr;
			myBlockID = theBasePtr->BLOB_BASE_Header.BLOB_BASE_HEADER_FirstFreeBlock;
			};

		myBlockDataPtr = (char *)theDataPtr;
		myBlockDataPtr += myBlockIndex * (long)kBLOB_BlockSize;
		myBlobErr = BLOB_WriteBlock(theBasePtr, myBlockID, myBlockDataPtr);
		if (myBlobErr != BLOB_ERR_NoErr) return myBlobErr;

		if (myBlockIndex == 0L)
			{
			*theFileIDPtr = myBlockID;
			}
		else
			{
			myBlobErr = BLOB_SetBlockInfo(theBasePtr, myPrevBlockID, myBlockID, 0);
			if (myBlobErr != BLOB_ERR_NoErr) return myBlobErr;
			};

		BLOB_GetBlockInfo(theBasePtr, myBlockID, &myNextBlockID, &myBlockSize);
		theBasePtr->BLOB_BASE_Header.BLOB_BASE_HEADER_FirstFreeBlock = myNextBlockID;
		};
	myBlobErr = BLOB_SetBlockInfo(theBasePtr, myBlockID, 0L, myLastBlockSize);
	if (myBlobErr != BLOB_ERR_NoErr) return myBlobErr;

	return BLOB_ERR_NoErr;

	} // BLOB_WriteFile

/*------------------------------------------------------------------------------
//
//	BLOB_FreeFile
//
*/

BLOB_Err BLOB_FreeFile (BLOB_BasePtr theBasePtr, unsigned long theFileID)

	{
	BLOB_Err			  myBlobErr;
	unsigned long		  myBlockID = theFileID;
	unsigned long		  myNextBlockID;
	unsigned short		  myBlockSize;
	unsigned long		  myLastFreeBlockID;

	while (myBlockID != 0L)
		{
		myBlobErr = BLOB_GetBlockInfo(theBasePtr,
			myBlockID, &myNextBlockID, &myBlockSize);
		if (myBlobErr != BLOB_ERR_NoErr) return myBlobErr;

		myBlobErr = BLOB_SetBlockInfo(theBasePtr, myBlockID, myNextBlockID, 0);
		if (myBlobErr != BLOB_ERR_NoErr) break;

		myBlockID = myNextBlockID;
		};
	if (myBlobErr != BLOB_ERR_NoErr) return myBlobErr;

	myBlobErr = BLOB_GetLastFreeBlockID(theBasePtr, &myLastFreeBlockID);
	if (myBlobErr != BLOB_ERR_NoErr) return myBlobErr;

	if (myLastFreeBlockID != 0L)
		{
		myBlobErr = BLOB_SetBlockInfo(theBasePtr, myLastFreeBlockID, theFileID, 0);
		}
	else theBasePtr->BLOB_BASE_Header.BLOB_BASE_HEADER_FirstFreeBlock = theFileID;

	return myBlobErr;

	} // BLOB_FreeFile

/*------------------------------------------------------------------------------
//
//	BLOB_GetFileSize
//
*/

BLOB_Err BLOB_GetFileSize (BLOB_BasePtr theBasePtr, unsigned long theFileID,
	unsigned long *theSizePtr)

	{
	BLOB_Err			  myBlobErr;
	unsigned long		  myBlockID = theFileID;
	unsigned short		  myBlockSize;
	unsigned long		  myFileSize = 0L;

	while (myBlockID != 0L)
		{
		myBlobErr = BLOB_GetBlockInfo(theBasePtr,
			myBlockID, &myBlockID, &myBlockSize);
		if (myBlobErr != BLOB_ERR_NoErr) return myBlobErr;
		myFileSize += (unsigned long)myBlockSize;
		};

	*theSizePtr = myFileSize;

	return BLOB_ERR_NoErr;

	} // BLOB_GetFileSize

/*------------------------------------------------------------------------------
//
//	BLOB_ReadBaseHeader
//
*/

BLOB_Err BLOB_ReadBaseHeader (BLOB_BasePtr theBasePtr)

	{
	OSErr			  myOSErr;
	long				  myReadCount;

	myOSErr = SetFPos(theBasePtr->BLOB_BASE_FileRefNum, fsFromStart, 0L);
	if (myOSErr != noErr) return (BLOB_Err)myOSErr;

	myReadCount = (long)sizeof(BLOB_BaseHeader);
	myOSErr = FSRead(theBasePtr->BLOB_BASE_FileRefNum,
		&myReadCount, &theBasePtr->BLOB_BASE_Header);
	if (myOSErr != noErr) return (BLOB_Err)myOSErr;

	return BLOB_ERR_NoErr;

	} // BLOB_ReadBaseHeader

/*------------------------------------------------------------------------------
//
//	BLOB_WriteBaseHeader
//
*/

BLOB_Err BLOB_WriteBaseHeader (BLOB_BasePtr theBasePtr)

	{
	OSErr			  myOSErr;
	long				  myWriteCount;

	myOSErr = SetFPos(theBasePtr->BLOB_BASE_FileRefNum, fsFromStart, 0L);
	if (myOSErr != noErr) return BLOB_ERR_OSErr;

	myWriteCount = (long)sizeof(BLOB_BaseHeader);
	myOSErr = FSWrite(theBasePtr->BLOB_BASE_FileRefNum,
		&myWriteCount, &theBasePtr->BLOB_BASE_Header);
	if (myOSErr != noErr) return BLOB_ERR_OSErr;

	return BLOB_ERR_NoErr;

	} // BLOB_WriteBaseHeader

/*------------------------------------------------------------------------------
//
//	BLOB_ReadBaseBlockInfos
//
*/

BLOB_Err BLOB_ReadBaseBlockInfos (BLOB_BasePtr theBasePtr)

	{
	BLOB_Err			  myBlobErr = BLOB_ERR_NoErr;
	Handle			  myBaseBlockMap;
	long				  myBaseBlockMapSize;
	long				  myBaseBlockMapOffset;
	long				  myReadCount;

	myBaseBlockMapOffset = theBasePtr->BLOB_BASE_Header.BLOB_BASE_HEADER_NbOfBlocks;
	myBaseBlockMapOffset *= (long)kBLOB_BlockSize;
	myBaseBlockMapOffset += (long)sizeof(BLOB_BaseHeader);

	myBaseBlockMapSize = theBasePtr->BLOB_BASE_Header.BLOB_BASE_HEADER_NbOfBlocks;
	myBaseBlockMapSize *= (long)sizeof(unsigned long);

	if (theBasePtr->BLOB_BASE_BlockInfos == NULL)
		{
		myBaseBlockMap = NewHandle(myBaseBlockMapSize);
		theBasePtr->BLOB_BASE_BlockInfos = myBaseBlockMap;
		}
	else
		{
		myBaseBlockMap = theBasePtr->BLOB_BASE_BlockInfos;
		SetHandleSize(myBaseBlockMap, myBaseBlockMapSize);
		};

	if (myBaseBlockMapSize != 0L)
		{
		HLock(myBaseBlockMap);
		if (SetFPos(theBasePtr->BLOB_BASE_FileRefNum,
			fsFromStart, myBaseBlockMapOffset) == noErr)
			{
			myReadCount = myBaseBlockMapSize;
			myBlobErr = (BLOB_Err)FSRead(theBasePtr->BLOB_BASE_FileRefNum,
				&myReadCount, *myBaseBlockMap);
			};
		HUnlock(myBaseBlockMap);
		};

	if (myBlobErr != BLOB_ERR_NoErr)
		{
		DisposeHandle(myBaseBlockMap);
		theBasePtr->BLOB_BASE_BlockInfos = NULL;
		};

	return myBlobErr;

	} // BLOB_ReadBaseBlockInfos

/*------------------------------------------------------------------------------
//
//	BLOB_WriteBaseBlockInfos
//
*/

BLOB_Err BLOB_WriteBaseBlockInfos (BLOB_BasePtr theBasePtr)

	{
	BLOB_Err			  myBlobErr = BLOB_ERR_NoErr;
	Handle			  myBaseBlockMap;
	long				  myBaseBlockMapSize;
	long				  myBaseBlockMapOffset;
	long				  myWriteCount;

	myBaseBlockMapOffset = theBasePtr->BLOB_BASE_Header.BLOB_BASE_HEADER_NbOfBlocks;
	myBaseBlockMapOffset *= (long)kBLOB_BlockSize;
	myBaseBlockMapOffset += (long)sizeof(BLOB_BaseHeader);

	myBaseBlockMapSize = theBasePtr->BLOB_BASE_Header.BLOB_BASE_HEADER_NbOfBlocks;
	myBaseBlockMapSize *= (long)sizeof(unsigned long);

	myBaseBlockMap = theBasePtr->BLOB_BASE_BlockInfos;

	if (myBaseBlockMapSize != 0L)
		{
		HLock(myBaseBlockMap);
		myBlobErr = (BLOB_Err)SetFPos(theBasePtr->BLOB_BASE_FileRefNum,
			fsFromStart, myBaseBlockMapOffset);
		if (myBlobErr == noErr)
			{
			myWriteCount = myBaseBlockMapSize;
			myBlobErr = (BLOB_Err)FSWrite(theBasePtr->BLOB_BASE_FileRefNum,
				&myWriteCount, *myBaseBlockMap);
			};
		HUnlock(myBaseBlockMap);
		};

	return myBlobErr;

	} // BLOB_WriteBaseBlockInfos

/*------------------------------------------------------------------------------
//
//	BLOB_ReadBlock
//
*/

BLOB_Err BLOB_ReadBlock (BLOB_BasePtr theBasePtr,
	unsigned long theBlockID, void *theBufferPtr)

	{
	OSErr			  myOSErr;
	short			  myBlobFileRefNum;
	unsigned long		  myBlockOffset;
	long				  myReadCount;

	if (theBlockID == 0L) return BLOB_ERR_BadBlockID;
	if (theBlockID > BLOB_GetBaseNbOfBlocks(theBasePtr)) return BLOB_ERR_BadBlockID;

	myBlobFileRefNum = theBasePtr->BLOB_BASE_FileRefNum;
	myBlockOffset = (theBlockID - 1L) * (unsigned long)kBLOB_BlockSize;
	myBlockOffset += (long)sizeof(BLOB_BaseHeader);

	myOSErr = SetFPos(myBlobFileRefNum, fsFromStart, myBlockOffset);
	if (myOSErr != noErr) return (BLOB_Err)myOSErr;

	myReadCount = (long)kBLOB_BlockSize;
	myOSErr = FSRead(myBlobFileRefNum, &myReadCount, theBufferPtr);
	if (myOSErr != noErr) return (BLOB_Err)myOSErr;

	return BLOB_ERR_NoErr;

	} // BLOB_ReadBlock

/*------------------------------------------------------------------------------
//
//	BLOB_WriteBlock
//
*/

BLOB_Err BLOB_WriteBlock (BLOB_BasePtr theBasePtr,
	unsigned long theBlockID, void *theBufferPtr)

	{
	OSErr			  myOSErr;
	short			  myBlobFileRefNum;
	unsigned long		  myBlockOffset;
	long				  myWriteCount;

	if (theBlockID == 0L) return BLOB_ERR_BadBlockID;
	if (theBlockID > BLOB_GetBaseNbOfBlocks(theBasePtr)) return BLOB_ERR_BadBlockID;

	myBlobFileRefNum = theBasePtr->BLOB_BASE_FileRefNum;
	myBlockOffset = (theBlockID - 1L) * (unsigned long)kBLOB_BlockSize;
	myBlockOffset += (long)sizeof(BLOB_BaseHeader);

	myOSErr = SetFPos(myBlobFileRefNum, fsFromStart, myBlockOffset);
	if (myOSErr != noErr) return (BLOB_Err)myOSErr;

	myWriteCount = (long)kBLOB_BlockSize;
	myOSErr = FSWrite(myBlobFileRefNum, &myWriteCount, theBufferPtr);
	if (myOSErr != noErr) return (BLOB_Err)myOSErr;

	return BLOB_ERR_NoErr;

	} // BLOB_WriteBlock

/*------------------------------------------------------------------------------
//
//	BLOB_GetLastFreeBlockID
//
*/

BLOB_Err BLOB_GetLastFreeBlockID (BLOB_BasePtr theBasePtr,
	unsigned long *theBlockIDPtr)

	{
	unsigned long		  myBlockID;
	unsigned long		  myNextBlockID;

	myBlockID = theBasePtr->BLOB_BASE_Header.BLOB_BASE_HEADER_FirstFreeBlock;

	while (myBlockID != 0L)
		{
		BLOB_GetBlockInfo(theBasePtr, myBlockID, &myNextBlockID, NULL);
		if (myNextBlockID == 0L) break;
		myBlockID = myNextBlockID;
		};

	*theBlockIDPtr = myBlockID;

	return BLOB_ERR_NoErr;

	} // BLOB_GetLastFreeBlockID

/*------------------------------------------------------------------------------
//
//	BLOB_CreateFreeBlocks
//
*/

BLOB_Err BLOB_CreateFreeBlocks (BLOB_BasePtr theBasePtr,
	unsigned long theNbOfBlocks)

	{
	BLOB_Err			  myBlobErr;
	OSErr			  myOSErr;
	short			  myBlobFileRefNum;
	unsigned long		  myOldNbOfBlocks;
	unsigned long		  myNewNbOfBlocks;
	long				  myNewEOF;
	unsigned long		  myLastFreeBlockID;
	Handle			  myBlockInfos;
	unsigned long		  myBlockID;
	unsigned long		  myNextBlockID;

	myBlockInfos = theBasePtr->BLOB_BASE_BlockInfos;
	if (myBlockInfos == NULL) return BLOB_ERR_NoBlockMap;

	myBlobFileRefNum = theBasePtr->BLOB_BASE_FileRefNum;
	myOldNbOfBlocks = theBasePtr->BLOB_BASE_Header.BLOB_BASE_HEADER_NbOfBlocks;
	myNewNbOfBlocks = myOldNbOfBlocks + theNbOfBlocks;
	myNewEOF = myNewNbOfBlocks * (long)kBLOB_BlockSize;
	myNewEOF += (long)sizeof(BLOB_BaseHeader);
	myOSErr = SetEOF(myBlobFileRefNum, myNewEOF);
	if (myOSErr != noErr) return (BLOB_Err)myOSErr;

	SetHandleSize(myBlockInfos, myNewNbOfBlocks * (long)sizeof(unsigned long));
	theBasePtr->BLOB_BASE_Header.BLOB_BASE_HEADER_NbOfBlocks = myNewNbOfBlocks;

	for (myBlockID = myOldNbOfBlocks + 1L; myBlockID <= myNewNbOfBlocks; myBlockID++)
		{
		myNextBlockID = (myBlockID == myNewNbOfBlocks) ? 0L : (myBlockID + 1L);
		myBlobErr = BLOB_SetBlockInfo(theBasePtr, myBlockID, myNextBlockID, 0);
		if (myBlobErr != BLOB_ERR_NoErr) break;
		};
	if (myBlobErr != BLOB_ERR_NoErr) return myBlobErr;

	myBlobErr = BLOB_GetLastFreeBlockID(theBasePtr, &myLastFreeBlockID);
	if (myBlobErr != BLOB_ERR_NoErr) return myBlobErr;

	myBlockID = myOldNbOfBlocks + 1L;
	if (myLastFreeBlockID != 0L)
		{
		myBlobErr = BLOB_SetBlockInfo(theBasePtr, myLastFreeBlockID, myBlockID, 0);
		}
	else theBasePtr->BLOB_BASE_Header.BLOB_BASE_HEADER_FirstFreeBlock = myBlockID;

	return BLOB_ERR_NoErr;

	} // BLOB_CreateFreeBlocks

/*------------------------------------------------------------------------------
//
//	BLOB_GetBlockInfo
//
*/

BLOB_Err BLOB_GetBlockInfo (BLOB_BasePtr theBasePtr, unsigned long theBlockID,
	unsigned long *theNextBlockIDPtr, unsigned short *theBlockSizePtr)

	{
	Handle			  myBlockInfos;
	long				  myBlockInfo;

	if (theNextBlockIDPtr != NULL) *theNextBlockIDPtr = 0L;
	if (theBlockSizePtr != NULL) *theBlockSizePtr = 0;

	myBlockInfos = theBasePtr->BLOB_BASE_BlockInfos;
	if (myBlockInfos == NULL) return BLOB_ERR_NoBlockMap;

	if (theBlockID == 0L) return BLOB_ERR_BadBlockID;
	if (theBlockID > theBasePtr->BLOB_BASE_Header.BLOB_BASE_HEADER_NbOfBlocks)
		return BLOB_ERR_BadBlockID;

	HLock(myBlockInfos);
	myBlockInfo = ((unsigned long *)*myBlockInfos)[theBlockID - 1L];
	HUnlock(myBlockInfos);

	if (theNextBlockIDPtr != NULL)
		*theNextBlockIDPtr = myBlockInfo >> 9;

	if (theBlockSizePtr != NULL)
		*theBlockSizePtr = (unsigned short)(myBlockInfo & 0x1FFL);

	return BLOB_ERR_NoErr;

	} // BLOB_GetBlockInfo

/*------------------------------------------------------------------------------
//
//	BLOB_SetBlockInfo
//
*/

BLOB_Err BLOB_SetBlockInfo (BLOB_BasePtr theBasePtr, unsigned long theBlockID,
		unsigned long theNextBlockID, unsigned short theBlockSize)

	{
	Handle			  myBlockInfos;
	unsigned long		  myBlockInfo;

	myBlockInfos = theBasePtr->BLOB_BASE_BlockInfos;
	if (myBlockInfos == NULL) return BLOB_ERR_NoBlockMap;

	if (theBlockID == 0L) return BLOB_ERR_BadBlockID;
	if (theBlockID > theBasePtr->BLOB_BASE_Header.BLOB_BASE_HEADER_NbOfBlocks)
		return BLOB_ERR_BadBlockID;

	myBlockInfo = theNextBlockID << 9;
	myBlockInfo += theBlockSize;

	HLock(myBlockInfos);
	((unsigned long *)*myBlockInfos)[theBlockID - 1L] = myBlockInfo;
	HUnlock(myBlockInfos);

	return BLOB_ERR_NoErr;

	} // BLOB_SetBlockInfo

/*------------------------------------------------------------------------------
//
//	BLOB_MAC_GetFileFSSpec
//
*/

OSErr BLOB_MAC_GetFileFSSpec (FSSpecPtr theFSSpecPtr, char *theFileName)

	{
	OSErr			  myOSErr;
	short			  myAppRefNum;
	short			  myAppVRefNum;
	long				  myAppDirID = 0L;
	FCBPBRec			  myFCBPBRec;
	Str63			  myTempStr63;

	myAppRefNum = LMGetCurApRefNum();
	myOSErr = GetVRefNum(myAppRefNum, &myAppVRefNum);
	if (myOSErr != noErr) return myOSErr;

	myFCBPBRec.ioCompletion = NULL;
	myFCBPBRec.ioNamePtr = myTempStr63;
	myFCBPBRec.ioFCBIndx = 0;
	myFCBPBRec.ioVRefNum = 0;
	myFCBPBRec.ioRefNum = LMGetCurApRefNum();
	myOSErr = PBGetFCBInfoSync(&myFCBPBRec);
	if (myOSErr != noErr) return myOSErr;
	myAppDirID = myFCBPBRec.ioFCBParID;

	for (myTempStr63[0] = 0;
		theFileName[myTempStr63[0]] != '\0';
		myTempStr63[0]++)
		{
		if (myTempStr63[0] > 63) break;
		myTempStr63[myTempStr63[0] + 1] = theFileName[myTempStr63[0]];
		};
	myOSErr = FSMakeFSSpec(myAppVRefNum, myAppDirID, myTempStr63, theFSSpecPtr);
	return myOSErr;

	} // BLOB_MAC_GetFileFSSpec

/*------------------------------------------------------------------------------
//
//	BLOB_MemCopy/Clear
//
*/

void BLOB_MemCopy (void *theDstPtr, void *theSrcPtr, unsigned long theSize)

	{

	BlockMove((Ptr)theSrcPtr, (Ptr)theDstPtr, theSize);

	} // BLOB_MemCopy

void BLOB_MemClear (void *theDstPtr, unsigned long theSize)

	{
	unsigned long		  myCharNum;

	for (myCharNum = 0L; myCharNum < theSize; myCharNum++)
		{
		((char *)theDstPtr)[myCharNum] = '\0';
		};

	} // BLOB_MemClear

/*------------------------------------------------------------------------------
//
//	BLOB_DumpBlockMap
//
*/

#if kUsesTranscript
void BLOB_DumpBlockMap (BLOB_BasePtr theBasePtr)

	{
	Handle			  myBlockInfos;
	unsigned long		  myNbOfBlocks;
	unsigned long		  myBlockID;
	unsigned long		  myBlockInfo;
	unsigned long		  myNextBlockID;
	unsigned long		  myBlockSize;

	myBlockInfos = theBasePtr->BLOB_BASE_BlockInfos;

	UTIL_TransWrite("#### DUMP BlockMap - BEGIN\n");

	if (myBlockInfos != NULL)
		{
		myNbOfBlocks = GetHandleSize(myBlockInfos) / (long)sizeof(unsigned long);
		HLock(myBlockInfos);
		for (myBlockID = 1L; myBlockID <= myNbOfBlocks; myBlockID++)
			{
			myBlockInfo = ((unsigned long *)*myBlockInfos)[myBlockID - 1L];
			myNextBlockID = myBlockInfo >> 9;
			myBlockSize = myBlockInfo & 0x1FFL;
			UTIL_TransWrite("[%04ld] Next: %04ld Size: %03ld (0x%09lX)\n",
				myBlockID, myNextBlockID, myBlockSize, myBlockInfo);
			};
		HUnlock(myBlockInfos);
		}
	else UTIL_TransWrite("[NULL]\n");

	UTIL_TransWrite("#### DUMP BlockMap - END (%ld nodes)\n", myNbOfBlocks);

	} // BLOB_DumpBlockMap
#endif

/*------------------------------------------------------------------------------
//
//	That's All, Folks !...
//
*/
