#ifndef _util_h_
#define _util_h_

/*------------------------------------------------------------------------------
**
**	util.h
**
*/

#include "Dialogs.h"

/*------------------------------------------------------------------------------
**
**	Switches
**
*/

#define kUsesTranscript 1

/*------------------------------------------------------------------------------
**
**	Definitions
**
*/

typedef unsigned long ulong;
typedef unsigned short ushort;
typedef unsigned char uchar;

#define mColorQDPresent (CICN_IsCQDPresent())

typedef Handle CICNHandle;

typedef enum
	{
	kIconDown,
	kIconUp

	} IconState;

#define mCheckKeyMap(theKeyMap, theKeyCode) \
	((((unsigned char *)theKeyMap)[(unsigned short)theKeyCode >> 3] \
		>> ((unsigned short)theKeyCode & 7)) & 1)

/*------------------------------------------------------------------------------
**
**	Prototypes
**
*/

Boolean CICN_IsCQDPresent (void);
CICNHandle CICN_GetIcon (short theIconID);
void CICN_DisposIcon (CICNHandle theCICNHandle);
void CICN_PlotIcon (Rect *theRectPtr, CICNHandle theCICNHandle);

short UTIL_GetAppVRefNum (void);
long UTIL_GetAppDirID (void);

Boolean UTIL_IsFrontApplication (void);
void UTIL_InstallNotification (void);
void UTIL_RemoveNotification (void);
void UTIL_BringApplicationToFront (void);
void UTIL_SetSFDir (short theVRefNum, long theDirID);
Boolean UTIL_EqualFSSpec (FSSpecPtr theFSSpecPtr1, FSSpecPtr theFSSpecPtr2);

void UTIL_PlaySndFile (StringPtr theFileName);

StringPtr UTIL_BuildIndexedFileName (StringPtr theSrcFileName, unsigned short theIndex);
StringPtr UTIL_ExtFileNameSearch (StringPtr theStringPtr, StringPtr theExtension);
Boolean UTIL_IsFileNameChar (char theChar);
Boolean UTIL_IsSpaceChar (char theChar);
Boolean UTIL_IsEmptyLine (char *theLineCharsPtr, short *theLineLenPtr);
Boolean UTIL_PStringEndsWith (StringPtr theStringPtr, StringPtr theEndPtr);
void UTIL_PStrCpy (StringPtr theTargetStringPtr, StringPtr theArrowStringPtr);
void UTIL_PStrCat (StringPtr theTargetStringPtr, StringPtr theArrowStringPtr);
Boolean UTIL_IsOptionKeyDown (void);
char UTIL_ToUpper (char theChar);
unsigned short UTIL_StrLen (char *theStr);
short UTIL_StrNCmp (char *theStr1, char *theStr2, unsigned short theScope);
short UTIL_StrPos (char *theStrToLookIn, char *theStrToLookFor,
		Boolean theIgnoreCaseFlag, Boolean theIgnoreSpacesFlag);
char *UTIL_P2LocalCStr (StringPtr theStringPtr);

#if kUsesTranscript
void UTIL_DeleteTranscript (void);
void UTIL_TransWrite (const char *theFormatStr, ...);
#endif

/*------------------------------------------------------------------------------
*/

#endif

/*------------------------------------------------------------------------------
**
**	That's All, Folks !...
**
*/

