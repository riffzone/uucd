/*------------------------------------------------------------------------------
**
**	info.h
**
*/

#ifndef _info_h_
#define _info_h_

#include "segments.h"

/*------------------------------------------------------------------------------
**
**	Prototypes
**
*/

short INFO_GetInfoSegmentNum (void);

void INFO_ShowInfoWindow (short theSegmentNum);
void INFO_HideInfoWindow (short theButtonClicked);
WindowPtr INFO_GetInfoWindowPtr (void);

void INFO_DrawInfoWindow (void);
void INFO_DrawSegmentName (void);
void INFO_GetSegmentStateRect (UUCD_SegmentState theSegmentState, Rect *theRectPtr);

Boolean INFO_SetSegmentState (UUCD_SegmentState theSegmentState);
void INFO_DoMouseDown_InContent (Point theMouseLoc);
void INFO_DoKeyDown (EventRecord *theEventPtr);

/*------------------------------------------------------------------------------
*/

#endif

/*------------------------------------------------------------------------------
**
**	That's All, Folks !...
**
*/
