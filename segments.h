/*------------------------------------------------------------------------------
**
**	segments.h
**
*/

#ifndef _segments_h_
#define _segments_h_

#include "Files.h"

#include "blobs.h"

/*------------------------------------------------------------------------------
**
**	Definitions
**
*/

typedef enum
	{
	kUUCD_SEGTYPE_None = 0,

	kUUCD_SEGTYPE_UUCode,
	kUUCD_SEGTYPE_HQX,
	kUUCD_SEGTYPE_B64

	} UUCD_SegmentType;

typedef char UUCD_SegmentMark;
#define kUUCD_MARK_None '\0'
#define kUUCD_MARK_Check '√'

typedef enum
	{
	kUUCD_SEGSTATE_None = 0,

	kUUCD_SEGSTATE_Begin,
	kUUCD_SEGSTATE_Body,
	kUUCD_SEGSTATE_End,

	kUUCD_SEGSTATE_LinkedBegin,
	kUUCD_SEGSTATE_LinkedBody,
	kUUCD_SEGSTATE_LinkedEnd,
	kUUCD_SEGSTATE_Complete

	} UUCD_SegmentState;

typedef enum
	{
	kUUCD_DATATYPE_Unknown = 0,

	kUUCD_DATATYPE_gif,
	kUUCD_DATATYPE_jpg

	} UUCD_DataType;

typedef struct
	{
	StringHandle			  UUCD_SEGMENT_NameHdl;
	Handle				  UUCD_SEGMENT_DataHdl;
	unsigned long			  UUCD_SEGMENT_DataSize;
	UUCD_DataType			  UUCD_SEGMENT_DataType;
	unsigned long			  UUCD_SEGMENT_BlobID;
	UUCD_SegmentType		  UUCD_SEGMENT_Type;
	UUCD_SegmentState		  UUCD_SEGMENT_State;
	StringHandle			  UUCD_SEGMENT_DstFileNameHdl; /* if Begin, otherwise NULL */

	UUCD_SegmentMark		  UUCD_SEGMENT_Mark;
	FSSpec				  UUCD_SEGMENT_SrcFileFSSpec;
	long					  UUCD_SEGMENT_SrcFileSize;
	long					  UUCD_SEGMENT_FrstSrcLineNum;
	long					  UUCD_SEGMENT_LastSrcLineNum;

	} UUCD_Segment, *UUCD_SegmentPtr, **UUCD_SegmentHdl;

#define kUUCD_SegmentHeight 16

#define kUUCD_MarkIcon_Height kUUCD_SegmentHeight
#define kUUCD_MarkIcon_Width 8
#define kUUCD_MarkIcon_HOffset 4

#define kUUCD_TypeIcon_Height kUUCD_SegmentHeight
#define kUUCD_TypeIcon_Width 8
#define kUUCD_TypeIcon_HOffset 47

#define kUUCD_StateIcon_Height kUUCD_SegmentHeight
#define kUUCD_StateIcon_Width 32
#define kUUCD_StateIcon_HOffset 55

#define kUUCD_SegmentFile_HOffset (kUUCD_StateIcon_HOffset + 40)
#define kUUCD_SegmentFile_Width 100

#define kUUCD_SegmentName_HOffset (kUUCD_SegmentFile_HOffset + kUUCD_SegmentFile_Width)

#define kUUCD_InfoIcon_Width 8

#define kUUCD_CICONID_Begin 1024
#define kUUCD_CICONID_Body 1025
#define kUUCD_CICONID_End 1026
#define kUUCD_CICONID_LinkedBegin 1124
#define kUUCD_CICONID_LinkedBody 1125
#define kUUCD_CICONID_LinkedEnd 1126
#define kUUCD_CICONID_Complete 1127

#define kUUCD_CICONID_HQX 1228
#define kUUCD_CICONID_B64 1229

#define kUUCD_CICONID_MarkNone 1221
#define kUUCD_CICONID_MarkCheck 1222

#define kUUCDSaveIconUpCIconID 1224
#define kUUCDSaveIconDownCIconID 1225
#define kUUCDViewIconUpCIconID 1234
#define kUUCDViewIconDownCIconID 1235
#define kUUCDInfoIconUpCIconID 1226
#define kUUCDInfoIconDownCIconID 1227

typedef enum
	{
	kUUCD_SEGBG_Standard = 0,

	kUUCD_SEGBG_Info,

	kUUCD_SEGBG_Source,
	kUUCD_SEGBG_Target

	} UUCD_SegmentBg;

/*------------------------------------------------------------------------------
**
**	Prototypes
**
*/

short UUCD_NewSegment (void);
void UUCD_DelSegment (short theSegmentNum);
UUCD_SegmentHdl UUCD_GetSegmentHdl (short theSegmentNum);
short UUCD_GetNbOfSegments (void);

short UUCD_GetNbOfVisibleSegments (void);
void UUCD_SetNbOfVisibleSegments (short theNbOfSegments);
short UUCD_GetSegmentWidth (void);
void UUCD_SetSegmentWidth (short theSegmentWidth);
RectPtr UUCD_GetSegListViewRect (void);

short UUCD_GetTopSegmentNum (void);
void UUCD_SetTopSegment (short theSegmentNum);

Boolean UUCD_GetSegment (short theSegmentNum, UUCD_SegmentPtr theSegmentPtr);
Boolean UUCD_SetSegment (short theSegmentNum, UUCD_SegmentPtr theSegmentPtr);

long UUCD_GetSegmentDataSize (short theSegmentNum);

void UUCD_GetSegmentName (short theSegmentNum, StringPtr theNamePtr);
void UUCD_SetSegmentName (short theSegmentNum, StringPtr theName);

void UUCD_SetSegmentType (short theSegmentNum, UUCD_SegmentType theSegmentType);
UUCD_SegmentType UUCD_GetSegmentType (short theSegmentNum);

void UUCD_SetSegmentMark (short theSegmentNum, UUCD_SegmentMark theSegmentMark);
UUCD_SegmentMark UUCD_GetSegmentMark (short theSegmentNum);

void UUCD_MarkAllSegments (UUCD_SegmentMark theSegmentMark);

void UUCD_SetSegmentState (short theSegmentNum, UUCD_SegmentState theSegmentState);
UUCD_SegmentState UUCD_GetSegmentState (short theSegmentNum);
void UUCD_CheckSegmentStates (void);

long UUCD_GetSegmentSrcFile (short theSegmentNum, FSSpecPtr theFSSpecPtr);
void UUCD_SetSegmentSrcFile (short theSegmentNum, FSSpecPtr theFSSpecPtr, long theFileSize);

Boolean UUCD_GetSegmentDstFileName (short theSegmentNum, StringPtr theDstFileNamePtr);
void UUCD_SetSegmentDstFileName (short theSegmentNum, StringPtr theFileName);

void UUCD_MoveSegment (short theSrcSegmentNum, short theDstSegmentNum);
void UUCD_SortSegments (void);

void UUCD_AddUUToSegment (short theSegmentNum, char *theUULinePtr, short theUULineLen, long theFileLineNum);
void UUCD_AddHQXToSegment (short theSegmentNum, char *theLinePtr, short theLineLen, long theFileLineNum);
void UUCD_AddB64ToSegment (short theSegmentNum, char *theLinePtr, short theLineLen, long theFileLineNum);

void UUCD_DrawSegments (void);
Boolean UUCD_DrawSegment (short theSegmentNum, PixPatHandle theBgPixPat);
Boolean UUCD_GetSegmentRect (short theSegmentNum, Rect *theRectPtr);
void UUCD_GetSegmentStateIconRect (Rect *theSegmentRectPtr, Rect *theIconRectPtr);
void UUCD_GetSegmentSaveIconRect (Rect *theSegmentRectPtr, Rect *theIconRectPtr);
void UUCD_GetSegmentInfoIconRect (Rect *theSegmentRectPtr, Rect *theIconRectPtr);
void UUCD_GetSegmentMarkIconRect (Rect *theSegmentRectPtr, Rect *theIconRectPtr);
void UUCD_GetSegmentTypeIconRect (Rect *theSegmentRectPtr, Rect *theIconRectPtr);
Boolean UUCD_SegmentHasSaveIcon (short theSegmentNum);
CICNHandle UUCD_GetSegStateIcon (UUCD_SegmentState theSegmentState);
CICNHandle UUCD_GetSegTypeIcon (UUCD_SegmentType theSegmentType);
PixPatHandle UUCD_GetSegmentBgPixPat (short theSegmentNum, UUCD_SegmentBg theSegmentBg);
CICNHandle UUCD_GetMarkIcon (UUCD_SegmentMark theSegmentMark);
CICNHandle UUCD_GetSaveIcon (IconState theIconState);
CICNHandle UUCD_GetViewIcon (IconState theIconState);
CICNHandle UUCD_GetInfoIcon (IconState theIconState);

void UUCD_InitBlobs (void);
void UUCD_ExitBlobs (void);
BLOB_BasePtr UUCD_GetBlobBasePtr (void);
void UUCD_BlobSegment (short theSegmentNum);

/*------------------------------------------------------------------------------
*/

#endif

/*------------------------------------------------------------------------------
**
**	That's All, Folks !...
**
*/
