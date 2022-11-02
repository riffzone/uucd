/*------------------------------------------------------------------------------
**
**	prefs.h
**
*/

#ifndef _prefs_h_
#define _prefs_h_

/*------------------------------------------------------------------------------
**
**	Definitions
**
*/

#define kUUCDPrefsFileType 'uupf'

#define kUUCDPrefsNewLineCR 0
#define kUUCDPrefsNewLineLF 1
#define kUUCDPrefsNewLineCRLF 2

typedef struct
	{
	Str31		  UUP1_Extension;
	OSType		  UUP1_FileType;
	OSType		  UUP1_FileCreator;

	} UUP1, *UUP1Ptr, **UUP1Hdl;

enum {
	kUUP2FLAG_Sound = 0,
	kUUP2FLAG_AutoOpen,
	kUUP2FLAG_AutoDelete
	};

typedef struct
	{
	OSType		  UUP2_TextCreator;
	OSType		  UUP2_PermissionMode;
	ushort		  UUP2_SegmentMaxLines;
	Str31		  UUP2_SubjectTag;
	ushort		  UUP2_NewLineMode;
	ushort		  UUP2_Flags;

	} UUP2, *UUP2Ptr, **UUP2Hdl;

#define kUUCDDefaultWindowPosH 48
#define kUUCDDefaultWindowPosV 10
#define kUUCDDefaultNbOfVisibleSegments 15
#define kUUCDDefaultSegmentWidth 450

/*------------------------------------------------------------------------------
**
**	Prototypes
**
*/

UUP1Hdl PREF_GetUUP1Hdl (void);
UUP2Hdl PREF_GetUUP2Hdl (void);

OSType PREF_GetTextCreator (void);
OSType PREF_GetPermissionMode (void);
ushort PREF_GetSegmentMaxLines (void);
void PREF_GetSubjectTag (StringPtr theStringPtr);
ushort PREF_GetNewLineMode (void);

Boolean PREF_TestFlag (ushort theFlagNum);
void PREF_SetFlag (ushort theFlagNum, Boolean theValue);

Boolean PREF_IsSoundEnabled (void);
void PREF_SetSoundEnable (Boolean theEnable);

Boolean PREF_IsAutoOpenEnabled (void);
void PREF_SetAutoOpenEnable (Boolean theEnable);

Boolean PREF_IsAutoDeleteEnabled (void);
void PREF_SetAutoDeleteEnable (Boolean theEnable);

Boolean PREF_GetFileNameTypeCreator (StringPtr theFileName,
	OSType *theTypePtr, OSType *theCreatorPtr);

void PREF_GetUUCDWindowPos (Point *theUUCDWindowPos);
void PREF_SetUUCDWindowPos (Point theUUCDWindowPos);
void PREF_GetUUCDWindowSiz (Point *theUUCDWindowSiz);
void PREF_SetUUCDWindowSiz (Point theUUCDWindowPos);

void PREF_LoadPreferences (void);
void PREF_SavePreferences (void);
void PREF_RAZPreferences (void);

void UUCD_EditPreferences (void);

/*------------------------------------------------------------------------------
*/

#endif

/*------------------------------------------------------------------------------
**
**	That's All, Folks !...
**
*/
