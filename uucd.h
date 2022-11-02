/*------------------------------------------------------------------------------
**
**	uucd.h
**
*/

#ifndef _uucd_h_
#define _uucd_h_

/*------------------------------------------------------------------------------
**
**	Definitions
**
*/

#define kUUCDCreator 'UUCD'

#define kUUCDBorderSize 10
#define kUUCDSpaceSize 4
#define kUUCDProgressHeight 6
#define kUUCDMemoryHeight 1
#define kUUCDCellMoveRectWidth 16
#define kUUCDProgressPixPatID 200
#define kUUCDMemoryPixPatID 201
#define kUUCDWindowBgPatID 128
#define kUUCDSegListVSBarRefCon ((long)'SLst')

/*------------------------------------------------------------------------------
**
**	Prototypes
**
*/

WindowPtr UUCD_GetUUCDWindowPtr (void);

/*------------------------------------------------------------------------------
*/

#endif

/*------------------------------------------------------------------------------
**
**	That's All, Folks !...
**
*/

