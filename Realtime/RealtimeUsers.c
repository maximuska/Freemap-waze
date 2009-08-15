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


#include <string.h>
#include <stdio.h>   //   _snprintf

#include "RealtimeDefs.h"
//////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////
static PFN_ONUSER gs_pfnOnAddUser   = NULL;
static PFN_ONUSER gs_pfnOnMoveUser  = NULL;
static PFN_ONUSER gs_pfnOnRemoveUser= NULL;
//////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////
void RTUserLocation_Init( LPRTUserLocation this)
{
   this->iID              = RT_INVALID_LOGINID_VALUE;
   this->fLongitude       = 0.F;
   this->fLatitude        = 0.F;
   this->iAzimuth         = 0;
   this->fSpeed           = 0.F;
   this->i64LastAccessTime= 0;
   this->bWasUpdated      = FALSE;
   memset( this->sName, 0, sizeof(this->sName));
   memset( this->sGUIID,0, sizeof(this->sGUIID));
}

void RTUserLocation_CreateGUIID( LPRTUserLocation this)
{ snprintf( this->sGUIID, RT_USERID_MAXSIZE, "Friend_%d", this->iID);}
//////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////
void RTUsers_Init(LPRTUsers   this,
                  PFN_ONUSER  pfnOnAddUser,
                  PFN_ONUSER  pfnOnMoveUser,
                  PFN_ONUSER  pfnOnRemoveUser)
{
   int i;

   for( i=0; i<RL_MAXIMUM_USERS_COUNT; i++)
      RTUserLocation_Init( &(this->Users[i]));

   this->iCount      = 0;
   gs_pfnOnAddUser   = pfnOnAddUser;
   gs_pfnOnMoveUser  = pfnOnMoveUser;
   gs_pfnOnRemoveUser= pfnOnRemoveUser;
}

void RTUsers_Reset(LPRTUsers   this)
{
   int i;

   for( i=0; i<RL_MAXIMUM_USERS_COUNT; i++)
      RTUserLocation_Init( &(this->Users[i]));

   this->iCount = 0;
}

void RTUsers_Term( LPRTUsers this)
{
   RTUsers_ClearAll( this);
   gs_pfnOnAddUser     = NULL;
   gs_pfnOnMoveUser    = NULL;
   gs_pfnOnRemoveUser  = NULL;
}

int RTUsers_Count( LPRTUsers this)
{ return this->iCount;}

BOOL RTUsers_IsEmpty( LPRTUsers this)
{ return (0 == this->iCount);}

BOOL RTUsers_Add( LPRTUsers this, LPRTUserLocation pUser)
{
   assert(gs_pfnOnAddUser);

   //   Full?
   if( RL_MAXIMUM_USERS_COUNT == this->iCount)
      return FALSE;

   //   Already exists?
   if( RTUsers_Exists( this, pUser->iID))
      return FALSE;

   this->Users[this->iCount]            = (*pUser);
   this->Users[this->iCount].bWasUpdated= TRUE;
   this->iCount++;
   gs_pfnOnAddUser( pUser);

   return TRUE;
}

BOOL RTUsers_Update( LPRTUsers this, LPRTUserLocation pUser)
{
   LPRTUserLocation pUI = RTUsers_UserByID( this, pUser->iID);

   assert(gs_pfnOnMoveUser);

   if( !pUI)
      return FALSE;

   (*pUI) = (*pUser);
   gs_pfnOnMoveUser( pUser);
   pUI->bWasUpdated = TRUE;
   return TRUE;
}

BOOL RTUsers_UpdateOrAdd( LPRTUsers this, LPRTUserLocation pUser)
{
   if( !RTUsers_Update( this, pUser) && !RTUsers_Add( this, pUser))
      return FALSE;

   pUser->bWasUpdated = TRUE;
   return TRUE;
}

BOOL  RTUsers_RemoveByIndex( LPRTUsers this, int iIndex)
{
   int i;

   assert(gs_pfnOnRemoveUser);

   //   Are we empty?
   if( (iIndex < 0) || (this->iCount <= iIndex))
      return FALSE;

   gs_pfnOnRemoveUser( &(this->Users[iIndex]));

   for( i=iIndex; i<(this->iCount-1); i++)
      this->Users[i] = this->Users[i+1];

   this->iCount--;
   RTUserLocation_Init( &(this->Users[this->iCount]));

   return TRUE;
}

BOOL RTUsers_RemoveByID( LPRTUsers this, int iUserID)
{
   int   i;

   //   Are we empty?
   if( 0 == this->iCount)
      return FALSE;

   for( i=0; i<this->iCount; i++)
      if( this->Users[i].iID == iUserID)
      {
         RTUsers_RemoveByIndex( this, i);
         return TRUE;
      }

   return FALSE;
}

BOOL RTUsers_Exists( LPRTUsers this, int iUserID)
{
   if( NULL == RTUsers_UserByID( this, iUserID))
      return FALSE;

   return TRUE;
}

void RTUsers_ClearAll( LPRTUsers this)
{
   int i;

   assert(gs_pfnOnRemoveUser);

   //   Find user:
   for( i=0; i<this->iCount; i++)
   {
      LPRTUserLocation pUI = &(this->Users[i]);

      gs_pfnOnRemoveUser( pUI);
      RTUserLocation_Init( pUI);
   }

   this->iCount = 0;

}

void RTUsers_ResetUpdateFlag( LPRTUsers this)
{
   int i;

   //   Find user:
   for( i=0; i<this->iCount; i++)
      this->Users[i].bWasUpdated = FALSE;
}

void RTUsers_RedoUpdateFlag( LPRTUsers this)
{
   int i;

   //   Find user:
   for( i=0; i<this->iCount; i++)
      this->Users[i].bWasUpdated = TRUE;
}

void RTUsers_RemoveUnupdatedUsers( LPRTUsers this, int* pUpdatedCount, int* pRemovedCount)
{
   int i;

   (*pUpdatedCount) = 0;
   (*pRemovedCount) = 0;

   for( i=0; i<this->iCount; i++)
      if( this->Users[i].bWasUpdated)
         (*pUpdatedCount)++;
      else
      {
         RTUsers_RemoveByIndex( this, i);
         (*pRemovedCount)++;
         i--;
      }
}

LPRTUserLocation RTUsers_User( LPRTUsers this, int iIndex)
{
   if( (0 <= iIndex) && (iIndex < this->iCount))
      return &(this->Users[iIndex]);
   //   Else
   return NULL;
}

LPRTUserLocation RTUsers_UserByID( LPRTUsers this, int iUserID)
{
   int i;

   //   Find user:
   for( i=0; i<this->iCount; i++)
      if( this->Users[i].iID == iUserID)
         return &(this->Users[i]);

   return NULL;
}
//////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////
