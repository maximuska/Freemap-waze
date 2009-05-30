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


#ifndef   __FREEMAP_REALTIMENET_H__
#define   __FREEMAP_REALTIMENET_H__
//////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////
#include "RealtimeNetDefs.h"
#include "../editor/track/editor_track_report.h"
#include "roadmap_gps.h"
#include "roadmap_address.h"
//////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL  RTNet_Init();
void  RTNet_Term();

BOOL  RTNet_LoadParams ();
int   RTNet_Send       ( RoadMapSocket socket, const char* szText);
BOOL  RTNet_TransactionQueue_ProcessSingleItem( BOOL* pTransactionStarted);
void  RTNet_TransactionQueue_Clear();
//////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL  RTNet_Login(      LPRTConnectionInfo   pCI,
                        const char*          szUserName,
                        const char*          szUserPW,
                        const char*          szUserNickname,
                        PFN_ONASYNCCOMPLETED pfnOnCompleted);
                        
BOOL  RTNet_GuestLogin( LPRTConnectionInfo   pCI,
                        PFN_ONASYNCCOMPLETED pfnOnCompleted);
                        
BOOL  RTNet_Register(   LPRTConnectionInfo   pCI,
                        PFN_ONASYNCCOMPLETED pfnOnCompleted);
                        
BOOL  RTNet_Logout(     LPRTConnectionInfo   pCI,
                        PFN_ONASYNCCOMPLETED pfnOnCompleted);
//////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////
void  RTNet_Watchdog(LPRTConnectionInfo  pCI);

BOOL  RTNet_SetMyVisability(  LPRTConnectionInfo   pCI, 
                              ERTVisabilityGroup   eVisability, 
                              ERTVisabilityReport  eVisabilityReport,
                              PFN_ONASYNCCOMPLETED pfnOnCompleted,
                              char*                packet_only);

BOOL  RTNet_At            (LPRTConnectionInfo   pCI,
                           const
                           RoadMapGpsPosition*  pGPSPosition,
                           int                  from_node,
                           int                  to_node,
                           PFN_ONASYNCCOMPLETED pfnOnCompleted,
                           char*                packet_only);

BOOL  RTNet_NavigateTo(    LPRTConnectionInfo   pCI,
                           const
                           RoadMapPosition*     cordinates,
                           address_info_ptr     ai,
                           PFN_ONASYNCCOMPLETED pfnOnCompleted);

BOOL  RTNet_MapDisplyed ( LPRTConnectionInfo   pCI,
                           const RoadMapArea*   pRoadMapArea,
                           unsigned int         scale,
                           PFN_ONASYNCCOMPLETED pfnOnCompleted,
                           char*                packet_only);
                           
BOOL  RTNet_CreateNewRoads ( 
                           LPRTConnectionInfo   pCI,
                           int                  nToggles,
                           const time_t*        toggle_time,
                           BOOL                 bStatusFirst,
                           PFN_ONASYNCCOMPLETED pfnOnCompleted,
                           char*                packet_only);
                           
BOOL  RTNet_StartFollowUsers(
                           LPRTConnectionInfo   pCI, 
                           PFN_ONASYNCCOMPLETED pfnOnCompleted,
                           char*                packet_only);
                           
BOOL  RTNet_StopFollowUsers(LPRTConnectionInfo pCI, 
                           PFN_ONASYNCCOMPLETED pfnOnCompleted,
                           char*                packet_only);

BOOL  RTNet_GPSPath(      LPRTConnectionInfo   pCI,
                           time_t               period_begin,
                           LPGPSPointInTime     points,
                           int                  count,
                           PFN_ONASYNCCOMPLETED pfnOnCompleted,
                           char*                packet_only);

BOOL  RTNet_NodePath(     LPRTConnectionInfo   pCI,
                           time_t               period_begin,
                           LPNodeInTime         nodes,
                           int                  count,
                           PFN_ONASYNCCOMPLETED pfnOnCompleted,
                           char*                packet_only);

BOOL  RTNet_ReportAlert(   LPRTConnectionInfo   pCI,
                           int                  iType,
                           const char*          szDescription,
                           int                  iDirection,
                           PFN_ONASYNCCOMPLETED pfnOnCompleted);

BOOL  RTNet_ReportAlertAtPosition( 
                        LPRTConnectionInfo         pCI,
                        int                        iType,
                        const char*                szDescription,
                        int                        iDirection,
                        const RoadMapGpsPosition*  MyLocation,
                        PFN_ONASYNCCOMPLETED       pfnOnCompleted);
                        
BOOL RTNet_SendSMS ( 
                LPRTConnectionInfo   pCI,
	            const char*          szPhoneNumber,
	            PFN_ONASYNCCOMPLETED pfnOnCompleted);

BOOL  RTNet_PostAlertComment( 
                        LPRTConnectionInfo   pCI,
                        int                  iAlertId,
                        const char*          szDescription,
                        PFN_ONASYNCCOMPLETED pfnOnCompleted);

BOOL  RTNet_RemoveAlert(LPRTConnectionInfo   pCI,
                        int                  iAlertId,
                        PFN_ONASYNCCOMPLETED pfnOnCompleted);
                        
BOOL  RTNet_ReportMarker(  LPRTConnectionInfo   pCI,
                           const char*          szType,
                           int                  iLongitude,
                           int                  iLatitude,
                           int                  iAzimuth,
                           const char*          szDescription,
                           int                  nParams,
                           const char*          szParamKey[],
                           const char*          szParamVal[],
                           PFN_ONASYNCCOMPLETED pfnOnCompleted,
                           char*                packet_only);

BOOL  RTNet_ReportOneSegment_Encode (char *packet, int iSegment);

int   RTNet_ReportOneSegment_MaxLength (int iSegment);

BOOL  RTNet_SendTrafficInfo ( LPRTConnectionInfo   pCI,
                             int mode,
                             PFN_ONASYNCCOMPLETED pfnOnCompleted);
                                                                                                                                                      
BOOL  RTNet_GeneralPacket( LPRTConnectionInfo   pCI, 
                           const char*          Packet,
                           PFN_ONASYNCCOMPLETED pfnOnCompleted);
                           
void	RTNet_Auth_BuildCommand (	char*				Command,
											int				ServerId,
											const char*		ServerCookie,
											const char*		UserName,
											int				DeviceId,
											const char*		Version);
											                           
//////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////
#include "realtime_roadmap_net.h"
//////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////
#endif   //   __FREEMAP_REALTIMENET_H__
