#ifndef _blobs_h_
#define _blobs_h_

/*------------------------------------------------------------------------------
//
//	blobs.h
//
*/

#include "Files.h"

/*------------------------------------------------------------------------------
//
//	Documentation
//
*/

//   Une base est constituee de trois parties :
//
//   +------------------+
//   |   Header         |
//   +------------------+
//   |   Blocks         |
//   +------------------+
//   |   Map            |
//   +------------------+
//
//   Les blocks sont numerotes de 1 a NbBlocks.
//
//

/*------------------------------------------------------------------------------
//
//	Definitions
//
*/

typedef enum
	{
	BLOB_ERR_NoErr = 0,

	BLOB_ERR_BaseAlreadyExists,
	BLOB_ERR_BadBlockID,
	BLOB_ERR_NoBlockMap,
	BLOB_ERR_ZeroDataSize,

	BLOB_ERR_MemErr,
	BLOB_ERR_OSErr

	} BLOB_Err;

#define kBLOB_BlockSize 512

typedef struct
	{
	char				  BLOB_BASE_HEADER_Magic[8];
	long				  BLOB_BASE_HEADER_NbOfBlocks;
	unsigned long		  BLOB_BASE_HEADER_FirstFreeBlock;

	char				  BLOB_BASE_HEADER_Fill[512 - 16];

	} BLOB_BaseHeader, *BLOB_BaseHeaderPtr;

typedef struct
	{
	short			  BLOB_BASE_FileRefNum;
	FSSpec			  BLOB_BASE_FSSpec;
	BLOB_BaseHeader	  BLOB_BASE_Header;
	Handle			  BLOB_BASE_BlockInfos;

	} BLOB_Base, *BLOB_BasePtr;

/*------------------------------------------------------------------------------
//
//	Prototypes
//
*/

void BLOB_InitCheck (void);

BLOB_Err BLOB_CreateBase (char *theBaseName);
BLOB_Err BLOB_DeleteBase (char *theBaseName);
BLOB_Err BLOB_OpenBase (BLOB_BasePtr *theBasePtrPtr, char *theBaseName);
BLOB_Err BLOB_CloseBase (BLOB_BasePtr theBasePtr);
unsigned long BLOB_GetBaseNbOfBlocks (BLOB_BasePtr theBasePtr);

BLOB_Err BLOB_ReadFile (BLOB_BasePtr theBasePtr, unsigned long theFileID,
	void *theBufferPtr);
BLOB_Err BLOB_WriteFile (BLOB_BasePtr theBasePtr, unsigned long *theFileIDPtr,
	void *theDataPtr, unsigned long theDataSize);
BLOB_Err BLOB_FreeFile (BLOB_BasePtr theBasePtr, unsigned long theFileID);
BLOB_Err BLOB_GetFileSize (BLOB_BasePtr theBasePtr, unsigned long theFileID,
	unsigned long *theSizePtr);

BLOB_Err BLOB_ReadBaseHeader (BLOB_BasePtr theBasePtr);
BLOB_Err BLOB_WriteBaseHeader (BLOB_BasePtr theBasePtr);

BLOB_Err BLOB_ReadBaseBlockInfos (BLOB_BasePtr theBasePtr);
BLOB_Err BLOB_WriteBaseBlockInfos (BLOB_BasePtr theBasePtr);

BLOB_Err BLOB_ReadBlock (BLOB_BasePtr theBasePtr,
	unsigned long theBlockID, void *theBufferPtr);
BLOB_Err BLOB_WriteBlock (BLOB_BasePtr theBasePtr,
	unsigned long theBlockID, void *theBufferPtr);
BLOB_Err BLOB_GetLastFreeBlockID (BLOB_BasePtr theBasePtr,
	unsigned long *theBlockIDPtr);
BLOB_Err BLOB_CreateFreeBlocks (BLOB_BasePtr theBasePtr,
	unsigned long theNbOfBlocks);

BLOB_Err BLOB_GetBlockInfo (BLOB_BasePtr theBasePtr, unsigned long theBlockID,
	unsigned long *theNextBlockIDPtr, unsigned short *theBlockSizePtr);
BLOB_Err BLOB_SetBlockInfo (BLOB_BasePtr theBasePtr, unsigned long theBlockID,
		unsigned long theNextBlockID, unsigned short theBlockSize);

OSErr BLOB_MAC_GetFileFSSpec (FSSpecPtr theFSSpecPtr, char *theFileName);

void BLOB_MemCopy (void *theDstPtr, void *theSrcPtr, unsigned long theSize);
void BLOB_MemClear (void *theDstPtr, unsigned long theSize);

#if kUsesTranscript
void BLOB_DumpBlockMap (BLOB_BasePtr theBasePtr);
#endif

/*------------------------------------------------------------------------------
*/

#endif

/*------------------------------------------------------------------------------
//
//	That's All, Folks !...
//
*/

