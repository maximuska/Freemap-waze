/*
 * LICENSE:
 *
 *   Copyright 2008 PazO
 *
 *   RoadMap is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License V2 as published by
 *   the Free Software Foundation.
 *
 *   RoadMap is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with RoadMap; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */


#ifndef	__FREEMAP_REALTIMEUSERS_H__
#define	__FREEMAP_REALTIMEUSERS_H__
//////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////
#define  RT_USERNM_MAXSIZE             (63)
#define  RT_USERPW_MAXSIZE             (63)
#define  RT_USERNK_MAXSIZE             (63)
#define  RT_USERID_MAXSIZE             (63)
#define  RL_MAXIMUM_USERS_COUNT        (50)
//////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////
typedef struct tagRTUserLocation
{
   int         iID;                          // User ID (within the server)
   char        sName [RT_USERNM_MAXSIZE+1];  // User name (nickname?)
   char        sGUIID[RT_USERID_MAXSIZE+1];  // User ID (within the GUI)
   double      fLongitude;                   // User location:   Length Cordinate
   double      fLatitude;                    // User location:   Width Cordinate
   int         iAzimuth;                     // User Azimuth
   double      fSpeed;                       // User Speed
   long long   i64LastAccessTime;            // Last access time
   int		   iMood;
   BOOL        bWasUpdated;                  // New user, OR user location was changed

}  RTUserLocation, *LPRTUserLocation;

void   RTUserLocation_Init       ( LPRTUserLocation this);
void   RTUserLocation_CreateGUIID( LPRTUserLocation this);   //   Create ID for the GUI
//////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////
typedef void(*PFN_ONUSER)(LPRTUserLocation pUI);

typedef struct tagRTUsers
{
   RTUserLocation Users[RL_MAXIMUM_USERS_COUNT];
   int            iCount;

}  RTUsers, *LPRTUsers;

void  RTUsers_Init         ( LPRTUsers this, PFN_ONUSER pfnOnAddUser, PFN_ONUSER pfnOnMoveUser, PFN_ONUSER pfnOnRemoveUser);
void  RTUsers_Reset        ( LPRTUsers this);
void  RTUsers_Term         ( LPRTUsers this);
int   RTUsers_Count        ( LPRTUsers this);
BOOL  RTUsers_IsEmpty      ( LPRTUsers this);
BOOL  RTUsers_Add          ( LPRTUsers this, LPRTUserLocation pUser);
BOOL  RTUsers_Update       ( LPRTUsers this, LPRTUserLocation pUser);   /* change position */
BOOL  RTUsers_UpdateOrAdd  ( LPRTUsers this, LPRTUserLocation pUser);
BOOL  RTUsers_RemoveByID   ( LPRTUsers this, int              iUserID);
BOOL  RTUsers_RemoveByIndex( LPRTUsers this, int              iIndex);
BOOL  RTUsers_Exists       ( LPRTUsers this, int              iUserID);
void  RTUsers_ClearAll     ( LPRTUsers this);

void  RTUsers_ResetUpdateFlag       ( LPRTUsers this);
void  RTUsers_RedoUpdateFlag         ( LPRTUsers this);
void  RTUsers_RemoveUnupdatedUsers  ( LPRTUsers this, int* pUpdatedCount, int* pRemovedCount);

LPRTUserLocation  RTUsers_User      ( LPRTUsers this, int  iIndex);
LPRTUserLocation  RTUsers_UserByID  ( LPRTUsers this, int  iUserID);
//////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////
#endif	//	__FREEMAP_REALTIMEUSERS_H__
