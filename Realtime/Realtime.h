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

#ifndef   __FREEMAP_REALTIME_H__
#define   __FREEMAP_REALTIME_H__
//////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////
#include "Realtime/LMap_Base.h"
#include "roadmap_gps.h"
#include "roadmap_address.h"
#include "roadmap.h"
//////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////
typedef void(*PFN_LOGINTESTRES)( BOOL bDetailsVerified, int iErrorCode);

// Module initialization/termination   - Called once, when the process starts/terminates
BOOL  Realtime_Initialize();
void  Realtime_Terminate();

//   Module start/stop                 - Can be called many times during the process lifetime
BOOL  Realtime_Start();
void  Realtime_Stop( BOOL bEnableLogout);

void  Realtime_AbortTransaction( RoadMapCallback pfnOnTransAborted);
void  Realtime_NotifyOnIdle    ( RoadMapCallback pfnOnTransIdle);

BOOL  Realtime_ServiceIsActive();
BOOL  Realtime_IsInTransaction();

BOOL  Realtime_IsEnabled();

BOOL  Realtime_SendCurrentViewDimentions();
BOOL  Realtime_VerifyLoginDetails( PFN_LOGINTESTRES pfn);

BOOL  Realtime_ReportOnNavigation( const RoadMapPosition* cordinates, address_info_ptr ai);

BOOL  Realtime_Report_Alert(int iAlertType, const char * szDescription, int iDirection);
BOOL  Realtime_Alert_ReportAtLocation(int iAlertType, const char * szDescription, int iDirection, RoadMapGpsPosition   MyLocation);
BOOL  Realtime_Remove_Alert(int iAlertId);
BOOL  Realtime_Post_Alert_Comment(int iAlertId, const char * szCommentText);
BOOL  Realtime_StartSendingTrafficInfo(void);
BOOL  Realtime_SendTrafficInfo(int mode);
BOOL  Realtime_SendSMS(const char * szPhoneNumber);

BOOL  Realtime_Editor_ExportMarkers(PFN_LOGINTESTRES editor_cb);
BOOL  Realtime_Editor_ExportSegments(PFN_LOGINTESTRES editor_cb);

void OnSettingsChanged_VisabilityGroup(void);

void    RealTime_Auth (void);
void RecommentToFriend(void);

int         RealTimeLoginState(void);
const char* RealTime_Get_UserName();
//////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////
#endif   //   __FREEMAP_REALTIME_H__

