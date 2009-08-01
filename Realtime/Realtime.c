/*
 * LICENSE:
 *
 *   Copyright 2009 Maxim Kalaev
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

#include <stdlib.h>
#include <string.h>

#include "../roadmap_main.h"
#include "../roadmap_config.h"
#include "../roadmap_navigate.h"
#include "../roadmap_math.h"
#include "../roadmap_object.h"
#include "../roadmap_line.h"
#include "../roadmap_point.h"
#include "../roadmap_messagebox.h"
#include "../roadmap_line_route.h"
#include "../roadmap_net_mon.h"
#include "../roadmap_screen.h"
#include "../roadmap_internet.h"
#include "../roadmap_square.h"
#include "../roadmap_start.h"
#include "../roadmap_lang.h"
#include "../ssd/ssd_login_details.h"
#include "../roadmap_device_events.h"
#include "../roadmap_state.h"
#include "../ssd/ssd_dialog.h"
#include "../ssd/ssd_keyboard.h"
#include "../ssd/ssd_confirm_dialog.h"
#include "../editor/db/editor_marker.h"
#include "../editor/db/editor_line.h"
#include "../editor/export/editor_report.h"
#include "../editor/export/editor_sync.h"

#include "RealtimeNet.h"
#include "RealtimeAlerts.h"
#include "RealtimeTrafficInfo.h"

#include "RealtimePrivacy.h"
#include "RealtimeOffline.h"

#include "RealtimeString.h"
#include "Realtime.h"

#ifdef _WIN32
   #ifdef   TOUCH_SCREEN
      #pragma message("    Target device type:    TOUCH-SCREEN")
   #else
      #pragma message("    Target device type:    MENU ONLY (NO TOUCH-SCREEN)")
   #endif   // TOUCH_SCREEN
#endif   // _WIN32
//////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////
static   BOOL                 gs_bInitialized               = FALSE;
static   BOOL                 gs_bRunning                   = FALSE;
static   BOOL                 gs_bShouldSendMyVisability    = TRUE;
static   BOOL                 gs_bHadAtleastOneGoodSession  = FALSE;
static   BOOL                 gs_bWasStoppedAutoamatically  = FALSE;
static   BOOL                 gs_bQuiteErrorMode            = FALSE;
static   RTConnectionInfo     gs_CI;
         VersionUpgradeInfo   gs_VU;
static   StatusStatistics     gs_ST;
static   EConnectionStatus    gs_eConnectionStatus          = CS_Unknown;
static   PFN_LOGINTESTRES     gs_pfnOnLoginTestResult       = NULL;
static   RoadMapCallback      gs_pfnOnSystemIsIdle          = NULL;
static   PFN_LOGINTESTRES     gs_pfnOnExportMarkersResult   = NULL;
static   PFN_LOGINTESTRES     gs_pfnOnExportSegmentsResult  = NULL;
static   PFN_ONASYNCCOMPLETED gs_pfnOnLoginAfterRegister    = NULL;
static   RTPathInfo*          gs_pPI;
static  BOOL                        gs_bWritingOffline          = FALSE;

static void Realtime_FullReset( BOOL bRedraw)
{
   RTConnectionInfo_FullReset    ( &gs_CI);
   VersionUpgradeInfo_Init       ( &gs_VU);
   StatusStatistics_Reset        ( &gs_ST);
   RTNet_TransactionQueue_Clear  ();
   RTSystemMessageQueue_Empty    ();

   gs_pfnOnLoginTestResult       = NULL;
   gs_bHadAtleastOneGoodSession  = FALSE;
   gs_bQuiteErrorMode            = FALSE;

   if(bRedraw)
      roadmap_screen_redraw();
}

static void Realtime_ResetTransactionState( BOOL bRedraw)
{
   RTConnectionInfo_ResetTransaction( &gs_CI);
   VersionUpgradeInfo_Init          ( &gs_VU);
   RTSystemMessageQueue_Empty       ();

   if(bRedraw)
      roadmap_screen_redraw();
}

static void Realtime_ResetLoginState( BOOL bRedraw)
{
   RTConnectionInfo_ResetLogin   ( &gs_CI);
   VersionUpgradeInfo_Init       ( &gs_VU);
   StatusStatistics_Reset        ( &gs_ST);
   RTNet_TransactionQueue_Clear  ();
   RTSystemMessageQueue_Empty    ();

   if(bRedraw)
      roadmap_screen_redraw();
}
//////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////
void     OnTimer_Realtime                 (void);
void     OnSettingsChanged_EnableDisable  (void);
void     OnSettingsChanged_VisabilityGroup(void);

void     OnAddUser      (LPRTUserLocation pUI);
void     OnMoveUser     (LPRTUserLocation pUI);
void     OnRemoveUser   (LPRTUserLocation pUI);
void     OnMapMoved     (void);

void     OnDeviceEvent  ( device_event event, void* context);
BOOL     TestLoginDetails();

BOOL     Login( PFN_ONASYNCCOMPLETED callback, BOOL bAutoRegister);

void        Realtime_DumpOffline (void);
//////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////
//   User name
RoadMapConfigDescriptor RT_CFG_PRM_NAME_Var =
                           ROADMAP_CONFIG_ITEM(
                                    RT_CFG_TAB,
                                    RT_CFG_PRM_NAME_Name);

//   Nickname
RoadMapConfigDescriptor RT_CFG_PRM_NKNM_Var =
                           ROADMAP_CONFIG_ITEM(
                                    RT_CFG_TAB,
                                    RT_CFG_PRM_NKNM_Name);

//   Web-service address
RoadMapConfigDescriptor RT_CFG_PRM_PASSWORD_Var =
                           ROADMAP_CONFIG_ITEM(
                                    RT_CFG_TAB,
                                    RT_CFG_PRM_PASSWORD_Name);

//   Enable / Disable service
static RoadMapConfigDescriptor RT_CFG_PRM_STATUS_Var =
                           ROADMAP_CONFIG_ITEM(
                                    RT_CFG_TAB,
                                    RT_CFG_PRM_STATUS_Name);

//   Refresh rate
static RoadMapConfigDescriptor RT_CFG_PRM_REFRAT_Var =
                           ROADMAP_CONFIG_ITEM(
                                    RT_CFG_TAB,
                                    RT_CFG_PRM_REFRAT_Name);

//   Web-service address
static RoadMapConfigDescriptor RT_CFG_PRM_WEBSRV_Var =
                           ROADMAP_CONFIG_ITEM(
                                    RT_CFG_TAB,
                                    RT_CFG_PRM_WEBSRV_Name);

/*   My visability status
static RoadMapConfigDescriptor RT_CFG_PRM_MYVSBL_Var =
                           ROADMAP_CONFIG_ITEM(
                                    RT_CFG_TAB,
                                    RT_CFG_PRM_MYVSBL_Name);

//   My ROI - What people do I want to see
static RoadMapConfigDescriptor RT_CFG_PRM_INTRST_Var =
                           ROADMAP_CONFIG_ITEM(
                                    RT_CFG_TAB,
                                    RT_CFG_PRM_INTRST_Name);*/

//   Visability group:
RoadMapConfigDescriptor RT_CFG_PRM_VISGRP_Var =
                                   ROADMAP_CONFIG_ITEM(
                                    RT_CFG_TAB,
                                    RT_CFG_PRM_VISGRP_Name);

//   Visability Report:
RoadMapConfigDescriptor RT_CFG_PRM_VISREP_Var =
                                   ROADMAP_CONFIG_ITEM(
                                    RT_CFG_TAB,
                                    RT_CFG_PRM_VISREP_Name);

BOOL GetCurrentDirectionPoints(  RoadMapGpsPosition*  GPS_position,
                                 int*                 from_node,
                                 int*                 to_node,
                                 int*                 direction);
//////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////
//   Expose configuration property - web-service address - to external modules (C files)
const char* RT_GetWebServiceAddress()
{
    return roadmap_config_get( &RT_CFG_PRM_WEBSRV_Var );
}

void RT_SwitchToOtherWebService( void )
{
    extern BOOL gs_WebServiceParamsLoaded;

    roadmap_net_mon_error( "Switching web-service.." );
    gs_WebServiceParamsLoaded = FALSE;
    gs_CI.bLoggedIn           = FALSE;

    // Currently we are only can switch to the defalt server
    roadmap_config_set( &RT_CFG_PRM_WEBSRV_Var, RT_CFG_PRM_WEBSRV_Default );
}

//////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////
int Realtime_GetRefreshRateinMilliseconds()
{
   const char* szRefreshRate  = NULL;
   float       fRefreshRate   = 0.F;

   //   Calculate refresh rate:
   szRefreshRate = roadmap_config_get( &RT_CFG_PRM_REFRAT_Var);
   if( szRefreshRate && (*szRefreshRate))
      fRefreshRate = (float)atof(szRefreshRate);

   //   Fix refresh rate:
   if( !fRefreshRate)
      fRefreshRate = RT_CFG_PRM_REFRAT_iDef;
   else
   {
      if( fRefreshRate < RT_CFG_PRM_REFRAT_iMin)
         fRefreshRate = RT_CFG_PRM_REFRAT_iMin;
      else
         if( RT_CFG_PRM_REFRAT_iMax < fRefreshRate)
            fRefreshRate = RT_CFG_PRM_REFRAT_iMax;
   }

   //   Refresh-rate in milliseconds
   return (int)(fRefreshRate * 60 * 1000.F);
}

//   Enabled / Disabled:
BOOL GetEnableDisableState()
{
   if( 0 == strcmp( roadmap_config_get( &RT_CFG_PRM_STATUS_Var), RT_CFG_PRM_STATUS_Enabled))
      return TRUE;
   return FALSE;
}

BOOL Realtime_IsEnabled()
{ return GetEnableDisableState();}

//   Module initialization/termination - Called once, when the process starts/terminates
BOOL Realtime_Initialize()
{
   if( gs_bInitialized)
      return TRUE;

   // Nickname:
   roadmap_config_declare( RT_USER_TYPE,
                           &RT_CFG_PRM_NKNM_Var,
                           RT_CFG_PRM_NKNM_Default,
                           NULL);


   // Password:
   roadmap_config_declare_password (
                           RT_USER_TYPE,
                           &RT_CFG_PRM_PASSWORD_Var,
                           RT_CFG_PRM_PASSWORD_Default);

   // Name:
   roadmap_config_declare( RT_USER_TYPE,
                           &RT_CFG_PRM_NAME_Var,
                           RT_CFG_PRM_NAME_Default,
                           NULL);

   //   Enable / Disable service
   roadmap_config_declare_enumeration( RT_CFG_TYPE,
                                       &RT_CFG_PRM_STATUS_Var,
                                       OnSettingsChanged_EnableDisable,
                                       RT_CFG_PRM_STATUS_Enabled,
                                       RT_CFG_PRM_STATUS_Disabled,
                                       NULL);

   //   Refresh rate
   roadmap_config_declare( RT_CFG_TYPE,
                           &RT_CFG_PRM_REFRAT_Var,
                           RT_CFG_PRM_REFRAT_Default,
                           NULL);

   //   Web-service address
   roadmap_config_declare( RT_CFG_TYPE,
                           &RT_CFG_PRM_WEBSRV_Var,
                           RT_CFG_PRM_WEBSRV_Default,
                           NULL);

   // Visability group:
   roadmap_config_declare_enumeration( RT_USER_TYPE,
                                       &RT_CFG_PRM_VISGRP_Var,
                                       OnSettingsChanged_VisabilityGroup,
                                       RT_CFG_PRM_VISGRP_Anonymous,
                                       RT_CFG_PRM_VISGRP_Invisible,
                                       RT_CFG_PRM_VISGRP_Nickname,
                                       NULL);


   // Visability report:
   roadmap_config_declare_enumeration( RT_USER_TYPE,
                                       &RT_CFG_PRM_VISREP_Var,
                                       OnSettingsChanged_VisabilityGroup,
                                       RT_CFG_PRM_VISREP_Nickname,
                                       RT_CFG_PRM_VISREP_Anonymous,
                                       NULL);

   RealtimePrivacyInit();
   RTConnectionInfo_Init   ( &gs_CI, OnAddUser, OnMoveUser, OnRemoveUser);
   VersionUpgradeInfo_Init ( &gs_VU);
   StatusStatistics_Init   ( &gs_ST);
   RTAlerts_Init();
   RTNet_Init();

   // This will disable the auto-detection of large time gap between good sessions:
   gs_ST.timeLastGoodSession = time(NULL);

   roadmap_device_events_register( OnDeviceEvent, NULL);

   gs_bInitialized = TRUE;

    // dump data from previous run to offline file
    Realtime_DumpOffline ();

   //   If needed - start service:
   OnSettingsChanged_EnableDisable();

   return TRUE;
}

void Realtime_Terminate()
{
   if( !gs_bInitialized)
      return;

   roadmap_device_events_unregister( OnDeviceEvent);

    // dump pending data to offline file
    Realtime_DumpOffline ();

   Realtime_Stop( FALSE /* Enable Logout? */);
   RTNet_Term();
   RTAlerts_Term();
   RTTrafficInfo_Term();

   gs_bInitialized = FALSE;
}
//////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////
//   Module start/stop - Can be called many times during the process lifetime
BOOL Realtime_Start()
{
   if( !gs_bInitialized)
      return FALSE;

   if( gs_bRunning)
      return FALSE;

   //   Initialize all:
   Realtime_FullReset( FALSE /* Redraw */);
   RTAlerts_Term();  // 'Term' is not called from 'Stop', thus here we do ('Termp' + 'Init').
                     // 'Init' was called from 'Realtime_Initialize()', thus it is always
                     // called before 'Term'...
   RTAlerts_Init();

   gs_pfnOnSystemIsIdle = NULL;

   gs_bRunning = TRUE;
   roadmap_main_set_periodic( Realtime_GetRefreshRateinMilliseconds(), OnTimer_Realtime);

   roadmap_screen_subscribe_after_flow_control_refresh( OnMapMoved);

   OnTimer_Realtime();

   return gs_bRunning;
}

void OnTransactionCompleted_LogoutAndStop( BOOL bTransactionSucceeded)
{
   if( !bTransactionSucceeded)
      roadmap_log( ROADMAP_WARNING, "OnTransactionCompleted_LogoutAndStop() - 'Logout' failed");

   gs_CI.eTransactionStatus = TS_Idle;
   Realtime_FullReset( TRUE /* Redraw */);
}

void Realtime_Stop(BOOL bEnableLogout)
{
   BOOL bLogoutInProcess = FALSE;

   if( !gs_bRunning)
      return;

   roadmap_screen_subscribe_after_flow_control_refresh( NULL);

   roadmap_main_remove_periodic( OnTimer_Realtime);

   if( gs_CI.bLoggedIn)
   {
      VersionUpgradeInfo_Init       ( &gs_VU);
      RTNet_TransactionQueue_Clear  ();
      RTSystemMessageQueue_Empty    ();

      if( bEnableLogout)
      {
         if( RTNet_Logout( &gs_CI, OnTransactionCompleted_LogoutAndStop))
            bLogoutInProcess = TRUE;
         else
            roadmap_log( ROADMAP_ERROR, "Realtime_Stop() - 'RTNet_Logout()' had failed");
      }
   }

   if( bLogoutInProcess)
      return;

   if( TS_Idle == gs_CI.eTransactionStatus)
      Realtime_FullReset(TRUE /* Redraw */);
   else
      gs_CI.eTransactionStatus = TS_Stopping;

   gs_bRunning = FALSE;
}

BOOL Realtime_ServiceIsActive()
{ return gs_bInitialized && gs_bRunning;}

BOOL Realtime_IsInTransaction()
{ return (Realtime_ServiceIsActive() && (TS_Idle != gs_CI.eTransactionStatus));}

void  Realtime_NotifyOnIdle( RoadMapCallback pfnOnSystemIsIdle)
{
   assert(    pfnOnSystemIsIdle);
   assert(!gs_pfnOnSystemIsIdle);

   gs_pfnOnSystemIsIdle = NULL;

   if( TS_Idle == gs_CI.eTransactionStatus)
   {
      pfnOnSystemIsIdle();
      return;
   }

   gs_pfnOnSystemIsIdle = pfnOnSystemIsIdle;
}

void  Realtime_AbortTransaction( RoadMapCallback pfnOnSystemIsIdle)
{
   assert(    pfnOnSystemIsIdle);
   assert(!gs_pfnOnSystemIsIdle);

   gs_pfnOnSystemIsIdle = NULL;

   if( TS_Idle == gs_CI.eTransactionStatus)
   {
      pfnOnSystemIsIdle();
      return;
   }

   gs_pfnOnSystemIsIdle    = pfnOnSystemIsIdle;
   gs_CI.eTransactionStatus= TS_Stopping;
}
//////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////
void OnMsgboxClosed_ShowSystemMessage( int exit_code )
{
   RTSystemMessage Message;

   if( RTSystemMessageQueue_IsEmpty())
      return;

   if( RTSystemMessageQueue_Pop( &Message))
   {
      roadmap_messagebox_cb( Message.Title, Message.Text, OnMsgboxClosed_ShowSystemMessage);
      RTSystemMessage_Free( &Message);
   }
}

void ShowSystemMessages()
{ OnMsgboxClosed_ShowSystemMessage( -1);}

void PerformVersionUpgrade( int exit_code, void* context )
{
   assert( gs_VU.NewVersion[0]);
   assert( gs_VU.URL[0]);

#if defined (_WIN32) || defined (__SYMBIAN32__)
   if( dec_yes == exit_code)
      roadmap_internet_open_browser( gs_VU.URL);
#endif

   VersionUpgradeInfo_Init ( &gs_VU);
}

#ifndef IPHONE
void UpgradeVersion()
{
   const char* szTitle  = "Software Update";
   const char* szText   = "";

   switch( gs_VU.eSeverity)
   {
      case VUS_NA:
         return;

      case VUS_Low:
         szText = "A newer version of Waze is available. Would you like to install the new version?";
         break;

      case VUS_Medium:
         szText = "A newer version of Waze is available. Would you like to install the new version?";
         break;

      case VUS_Hi:
         szText = "A newer version of Waze is available. Would you like to install the new version?";
         break;

      default:
      {
         assert(0);
         return;
      }
   }

   ssd_confirm_dialog      (  szTitle,
                              szText,
                              TRUE,
                              PerformVersionUpgrade,
                              NULL);
}
#else

void UpgradeVersion()
{
   const char* szTitle  = "Software Update";
   const char* szText   = "A newer version of Waze is available. Please go to Cydia to perform an upgrade.";

   roadmap_messagebox(szTitle, szText);
}
#endif

BOOL ThereAreTooManyNetworkErrors(/*?*/)
{
   int iSecondsPassedFromLastGoodSession;

   if( (gs_ST.iNetworkErrorsSuccessive < RT_THRESHOLD_TO_DISABLE_SERVICE__MAX_NETWORK_ERRORS_SUCCESSIVE)
         &&
       (gs_ST.iNetworkErrors           < RT_THRESHOLD_TO_DISABLE_SERVICE__MAX_NETWORK_ERRORS))
      return FALSE;


   iSecondsPassedFromLastGoodSession = (int)(time(NULL) - gs_ST.timeLastGoodSession);
   if( iSecondsPassedFromLastGoodSession < RT_THRESHOLD_TO_DISABLE_SERVICE__MAX_SECONDS_FROM_LAST_SESSION)
      return FALSE;

   roadmap_log(ROADMAP_WARNING,
               "There Are Too Many Network Errors(!) - %d network errors occurred. %d of them successive. %d seconds passed from last good session!",
               gs_ST.iNetworkErrors,
               gs_ST.iNetworkErrorsSuccessive,
               iSecondsPassedFromLastGoodSession);

   return TRUE;
}

static void HandleNetworkErrors()
{
   if( (ERR_NetworkError == gs_CI.LastError) || (ERR_SessionTimedout == gs_CI.LastError))
   {
      gs_ST.iNetworkErrors++;
      gs_ST.iNetworkErrorsSuccessive++;

      if( ThereAreTooManyNetworkErrors())
      {
         // TURNNING OFF REALTIME
         roadmap_log( ROADMAP_WARNING, "HandleNetworkErrors() - STOPPING SERVICE - Too many network errors occurred; Disabling service for a while");
         Realtime_Stop( FALSE /* Enable Logout? */);
      }
      else
      {
         if(!gs_bQuiteErrorMode &&
            (RT_THRESHOLD_TO_ENTER_SILENT_MODE__MAX_NETWORK_ERRORS_SUCCESSIVE <
               gs_ST.iNetworkErrorsSuccessive))
         {
            roadmap_log( ROADMAP_INFO, "HandleNetworkErrors() - Entering the 'quite error mode' state");
            gs_bQuiteErrorMode = TRUE;
         }
      }
   }
   else
   {
      gs_ST.iNetworkErrorsSuccessive= 0;

      if( gs_bQuiteErrorMode)
      {
         gs_bQuiteErrorMode = FALSE;
         roadmap_log( ROADMAP_INFO, "HandleNetworkErrors() - Exiting the 'quite error mode' state");
      }
   }
}

void OnTransactionCompleted( BOOL bTransactionSucceeded)
{
    BOOL bNewTransactionStarted = FALSE;
    static int consequentFailedTransactions = 0;

    // Were we asked to abort?
    if (TS_Stopping == gs_CI.eTransactionStatus)
    {
        Realtime_ResetTransactionState(TRUE /* Redraw */);
        RTNet_TransactionQueue_Clear();

        if (gs_pfnOnSystemIsIdle)
        {
            gs_pfnOnSystemIsIdle();
            gs_pfnOnSystemIsIdle = NULL;
        }

        return;
    }

    if (!RTUsers_IsEmpty(&gs_CI.Users))
    {
        int iUpdatedUsersCount;
        int iRemovedUsersCount;

        RTUsers_RemoveUnupdatedUsers(&gs_CI.Users, &iUpdatedUsersCount,
                                     &iRemovedUsersCount);

        if (iUpdatedUsersCount || iRemovedUsersCount)
            roadmap_screen_redraw();
    }

    if (RTUsers_IsEmpty(&gs_CI.Users))
        roadmap_log(ROADMAP_DEBUG,"OnTransactionCompleted() - No users where found" );
    else
        roadmap_log( ROADMAP_DEBUG, "OnTransactionCompleted() - Have %d users",RTUsers_Count( &gs_CI.Users));

    // Count errors. If too many consequent errors encountered - try to switch server.
    if( bTransactionSucceeded == FALSE ) {
        if( ++consequentFailedTransactions > 1 ) {
            // 2 consequent errors, switching server, logging in again
            RT_SwitchToOtherWebService();
        }
    }else{
        consequentFailedTransactions = 0;
    }

    if( gs_CI.LastError != ERR_Success )
        roadmap_log( ROADMAP_WARNING, "OnTransactionCompleted() - Last operation ended with error '%s'", GetErrorString(gs_CI.LastError));

    switch (gs_CI.LastError) {
    case ERR_Success:
        if( bTransactionSucceeded ) {
            gs_ST.timeLastGoodSession = time(NULL);
            gs_CI.AdditionalErrorInfo = 0;

            if( !gs_bHadAtleastOneGoodSession)
                gs_bHadAtleastOneGoodSession = TRUE;
        }
        break;

    case ERR_UnknownLoginID:
        roadmap_net_mon_error(GetErrorString(gs_CI.LastError));
        Realtime_ResetLoginState( TRUE);
        break;

    case ERR_LoginFailed:
    case ERR_WrongNameOrPassword:
        ssd_login_details_dialog_show ();
        break;

    case ERR_GeneralError:
    case ERR_SessionTimedout:
    case ERR_ParsingError:
    case ERR_WrongNetworkSettings:
        roadmap_net_mon_error(GetErrorString(gs_CI.LastError));
        break;

    case ERR_NetworkError:
        if( !gs_bQuiteErrorMode &&
            gs_bHadAtleastOneGoodSession &&
            (gs_CI.AdditionalErrorInfo != neterr_request_pending) &&
            (gs_CI.AdditionalErrorInfo != neterr_no_path_to_destination))
            roadmap_net_mon_error(GetErrorString(gs_CI.LastError));
        else
            roadmap_log( ROADMAP_DEBUG,
                         "OnTransactionCompleted() - Not presenting network error (Virgin: %d; Err-info: %d; Quite err mode: %d)",
                         !gs_bHadAtleastOneGoodSession,
                         gs_CI.AdditionalErrorInfo,
                         gs_bQuiteErrorMode );
        break;

    default:
        assert(0);
        break;
    }

   // Special case: If 'no internet' - stop service:
   if((ERR_NetworkError             == gs_CI.LastError) &&
      (neterr_no_path_to_destination== gs_CI.AdditionalErrorInfo))
   {
      roadmap_log( ROADMAP_INFO, "OnTransactionCompleted() - !!! NO INTERNET !!! REALTIME SERVICE IS STOPPING AUTOMATICALLY !!!");
      Realtime_Stop( FALSE);
      gs_bWasStoppedAutoamatically = TRUE;
   }

   // Check for NO-NETWORK case:
   HandleNetworkErrors();

   if( RTSystemMessageQueue_Size())
      ShowSystemMessages();

   if( VUS_NA != gs_VU.eSeverity)
      UpgradeVersion();

   if( gs_pfnOnLoginTestResult)
      TestLoginDetails();
   else
   {
      if(!gs_CI.bLoggedIn                                                     ||
         !RTNet_TransactionQueue_ProcessSingleItem( &bNewTransactionStarted)  ||
         !bNewTransactionStarted)
         gs_CI.eTransactionStatus = TS_Idle;
   }

   if( (TS_Idle == gs_CI.eTransactionStatus) && gs_pfnOnSystemIsIdle)
   {
      gs_pfnOnSystemIsIdle();
      gs_pfnOnSystemIsIdle = NULL;
   }
}

void OnAsyncOperationCompleted_AllTogether( BOOL bResult)
{
   if( !bResult)
      roadmap_log( ROADMAP_ERROR, "OnAsyncOperationCompleted_AllTogether(POST) - The 'AllTogether' packet-send had failed");

   editor_track_report_conclude_export (bResult != FALSE);

   OnTransactionCompleted( bResult);
}

void OnAsyncOperationCompleted_NodePath( BOOL bResult)
{
   if( bResult)
   {
      roadmap_log( ROADMAP_DEBUG, "OnAsyncOperationCompleted_NodePath() - 'NodePath' succeeded (if there where points to send - they were sent)");
      roadmap_log( ROADMAP_DEBUG, "OnAsyncOperationCompleted_NodePath() - TRANSACTION FULLY COMPLETED");
   }
   else
      roadmap_log( ROADMAP_ERROR, "OnAsyncOperationCompleted_NodePath(POST) - 'NodePath' had failed");

   OnTransactionCompleted( bResult);
}

BOOL HaveNodePathToSend()
{ return (1 <= gs_pPI->num_nodes);}

BOOL SendMessage_NodePath( char* packet_only)
{
   BOOL  bRes;

   bRes = RTNet_NodePath( &gs_CI,
                           gs_pPI->nodes[0].GPS_time,
                           gs_pPI->nodes,
                           gs_pPI->num_nodes,
                           OnAsyncOperationCompleted_NodePath,
                           packet_only);

   return bRes;
}

void OnAsyncOperationCompleted_GPSPath( BOOL bResult)
{
   if( !bResult)
   {
      roadmap_log( ROADMAP_ERROR, "OnAsyncOperationCompleted_GPSPath(POST) - 'GPSPath' had failed");
      OnTransactionCompleted( bResult);
      return;
   }

   roadmap_log( ROADMAP_DEBUG, "OnAsyncOperationCompleted_GPSPath() - 'GPSPath' succeeded (if there where points to send - they were sent)");

   if( HaveNodePathToSend())
   {
      if( SendMessage_NodePath(NULL))
         roadmap_log( ROADMAP_DEBUG, "OnAsyncOperationCompleted_GPSPath() - Sending 'NodePath'...");
      else
      {
         roadmap_log( ROADMAP_ERROR, "OnAsyncOperationCompleted_GPSPath(PRE) - Failed to send 'NodePath'");
         OnTransactionCompleted( bResult);
      }
   }
   else
      OnAsyncOperationCompleted_NodePath(TRUE);
}


BOOL HaveGPSPointsToSend()
{
   return (1 < gs_pPI->num_points);
}

BOOL SendMessage_GPSPath( char* packet_only)
{
   BOOL  bRes;

   bRes = RTNet_GPSPath(&gs_CI,
                        gs_pPI->points[0].GPS_time,
                        gs_pPI->points,
                        gs_pPI->num_points,
                        OnAsyncOperationCompleted_GPSPath,
                        packet_only);

   return bRes;
}


void OnAsyncOperationCompleted_CreateNewRoads( BOOL bResult)
{
   if( !bResult)
   {
      roadmap_log( ROADMAP_ERROR, "OnAsyncOperationCompleted_CreateNewRoads(POST) - 'CreateNewRoads' had failed");
      OnTransactionCompleted( FALSE);
      return;
   }

   roadmap_log( ROADMAP_DEBUG, "OnAsyncOperationCompleted_CreateNewRoads() - 'CreateNewRoads' was sent!");

   if( HaveGPSPointsToSend())
   {
      if( SendMessage_GPSPath(NULL))
         roadmap_log( ROADMAP_DEBUG, "OnAsyncOperationCompleted_CreateNewRoads() - Sending 'GPSPath'...");
      else
      {
         roadmap_log( ROADMAP_ERROR, "OnAsyncOperationCompleted_CreateNewRoads(PRE) - Failed to send 'GPSPath'");
         OnTransactionCompleted( FALSE);
      }
   }
   else
      OnAsyncOperationCompleted_GPSPath(TRUE);  // Move on to next handler...
}


void OnAsyncOperationCompleted_ReportAlert( BOOL bResult)
{
   if (!bResult)
   {
        roadmap_messagebox ("Error", "Sending report failed");
   }
   else{
        roadmap_messagebox_timeout ("Thank you!!!", "Your report was sent to the community", 3);
   }

   OnTransactionCompleted( bResult);
}

void OnAsyncOperationCompleted_SendSMS( BOOL bResult)
{
   if (!bResult)
   {
        roadmap_messagebox ("Error", "Sending message failed");
   }
   else{
        roadmap_messagebox_timeout ("Thank you!!!", "Message sent", 3);
   }

   OnTransactionCompleted( bResult);
}
void OnAsyncOperationCompleted_PostComment( BOOL bResult)
{
   if (!bResult)
   {
        roadmap_messagebox ("Error", "Sending comment failed");
   }

   OnTransactionCompleted( bResult);
}

BOOL HaveCreateNewRoadsToSend()
{
   return (0 < gs_pPI->num_update_toggles);
}

BOOL SendMessage_CreateNewRoads( char* packet_only)
{
   BOOL bStatus = FALSE;
   RoadMapStateFn fnState;

   fnState = roadmap_state_find ("new_roads");

   if (fnState == NULL)
   {
    roadmap_log (ROADMAP_ERROR, "Failed to retrieve new_roads state");
    return TRUE;
   }

    bStatus = (fnState() != 0);

   return RTNet_CreateNewRoads(
                                &gs_CI,
                                gs_pPI->num_update_toggles,
                                gs_pPI->update_toggle_times,
                           gs_pPI->first_update_toggle_state,
                           OnAsyncOperationCompleted_CreateNewRoads,
                           packet_only);
}

void OnAsyncOperationCompleted_MapDisplayed( BOOL bResult)
{
   if( !bResult)
   {
      roadmap_log( ROADMAP_ERROR, "OnAsyncOperationCompleted_MapDisplayed(POST) - 'MapDisplayed' had failed");
      OnTransactionCompleted( FALSE);
      return;
   }

   roadmap_log( ROADMAP_DEBUG, "OnAsyncOperationCompleted_MapDisplayed() - 'MapDisplayed' was sent!");

    if (HaveCreateNewRoadsToSend ())
    {
        if (SendMessage_CreateNewRoads (NULL)) {
         roadmap_log( ROADMAP_DEBUG, "OnAsyncOperationCompleted_MapDisplayed() - Sending 'CreateNewRoads'...");
        }
        else {
         roadmap_log( ROADMAP_ERROR, "OnAsyncOperationCompleted_MapDisplayed(PRE) - Failed to send 'CreateNewRoads'");
         OnTransactionCompleted( FALSE);
        }
    }
   else
      OnAsyncOperationCompleted_CreateNewRoads(TRUE);  // Move on to next handler...
}


BOOL SendMessage_MapDisplyed( char* packet_only)
{
   RoadMapArea MapPosition;

   roadmap_math_screen_edges( &MapPosition);
   if((gs_CI.LastMapPosSent.west  == MapPosition.west ) &&
      (gs_CI.LastMapPosSent.south == MapPosition.south) &&
      (gs_CI.LastMapPosSent.east  == MapPosition.east ) &&
      (gs_CI.LastMapPosSent.north == MapPosition.north))
   {
      roadmap_log( ROADMAP_DEBUG, "SendMessage_MapDisplyed() - Skipping operation; Current coordinates where already sent...");

      if( !packet_only)
         OnAsyncOperationCompleted_MapDisplayed( TRUE);

      return TRUE;
   }

   if( RTNet_MapDisplyed(  &gs_CI,
                           &MapPosition,
               roadmap_math_get_scale(0),
                           OnAsyncOperationCompleted_MapDisplayed,
                           packet_only))
   {
      gs_CI.LastMapPosSent = MapPosition;
      return TRUE;
   }

   return FALSE;
}

void OnAsyncOperationCompleted_At( BOOL bResult)
{
   if( bResult)
      roadmap_log( ROADMAP_DEBUG, "OnAsyncOperationCompleted_At() - My position is set!");
   else
      roadmap_log( ROADMAP_ERROR, "OnAsyncOperationCompleted_At(POST) - Failed to set my position; Ignoring and continueing...");

   if( SendMessage_MapDisplyed( NULL))
      roadmap_log( ROADMAP_DEBUG, "OnAsyncOperationCompleted_At() - Sending 'MapDisplayed'...");
   else
   {
      roadmap_log( ROADMAP_ERROR, "OnAsyncOperationCompleted_At(PRE) - Failed to send 'MapDisplayed'");
      OnTransactionCompleted( FALSE);
   }
}

BOOL SendMessage_At( char* packet_only)
{
   RoadMapGpsPosition   MyLocation;
   int                  from;
   int                  to;
   int                  direction;

   if( !editor_track_report_get_current_position( &MyLocation, &from, &to, &direction))
   {
      roadmap_log( ROADMAP_DEBUG, "SendMessage_At() - 'GetCurrentDirectionPoints()' failed");
      return FALSE;
   }

   return RTNet_At(  &gs_CI,
        &MyLocation,
        from,
        to,
        OnAsyncOperationCompleted_At,
        packet_only);
}

void OnAsyncOperationCompleted_SetVisability( BOOL bResult)
{
   if( !bResult)
   {
      roadmap_log( ROADMAP_ERROR, "OnAsyncOperationCompleted_SetVisability(POST) - Failed to set visability");
      OnTransactionCompleted( FALSE);
      return;
   }

   roadmap_log( ROADMAP_DEBUG, "OnAsyncOperationCompleted_SetVisability() - Visability set!");

   //   Send current location:
   if( SendMessage_At( NULL))
      roadmap_log( ROADMAP_DEBUG, "OnAsyncOperationCompleted_SetVisability() - Setting my position...");
   else
   {
      roadmap_log( ROADMAP_WARNING, "OnAsyncOperationCompleted_SetVisability(PRE) - Failed to set my position ('At')");
      OnAsyncOperationCompleted_At(FALSE);
   }
}

BOOL SendMessage_SetMyVisability( char* packet_only)
{
   ERTVisabilityGroup eVisability;
   ERTVisabilityReport eVisabilityReport;

   if( !gs_bShouldSendMyVisability)
   {
      if( packet_only)
         (*packet_only) = '\0';

      return TRUE;
   }

   eVisability = ERTVisabilityGroup_from_string( roadmap_config_get( &RT_CFG_PRM_VISGRP_Var));

   eVisabilityReport = ERTVisabilityReport_from_string(roadmap_config_get( &RT_CFG_PRM_VISREP_Var));

   if( RTNet_SetMyVisability( &gs_CI,
                              eVisability,
                              eVisabilityReport,
                              OnAsyncOperationCompleted_SetVisability,
                              packet_only))
   {
      gs_bShouldSendMyVisability = FALSE;
      return TRUE;
   }

   return FALSE;
}

void OnAsyncOperationCompleted_MapDisplayed__only( BOOL bResult)
{
   if( bResult)
      roadmap_log( ROADMAP_DEBUG, "OnAsyncOperationCompleted_MapDisplayed__only() - 'MapDisplayed' was sent successfully");
   else
      roadmap_log( ROADMAP_ERROR, "OnAsyncOperationCompleted_MapDisplayed__only(POST) - 'MapDisplayed' had failed");

   OnTransactionCompleted( bResult);
}

BOOL Realtime_SendCurrentViewDimentions()
{
   RoadMapArea MapPosition;
   BOOL        bRes;

   if( !gs_bRunning)
   {
      roadmap_log( ROADMAP_ERROR, "Realtime_SendCurrentViewDimentions() - Realtime service is currently disabled; Exiting method");
      return FALSE;
   }

   roadmap_math_screen_edges( &MapPosition);
   if((gs_CI.LastMapPosSent.west  == MapPosition.west ) &&
      (gs_CI.LastMapPosSent.south == MapPosition.south) &&
      (gs_CI.LastMapPosSent.east  == MapPosition.east ) &&
      (gs_CI.LastMapPosSent.north == MapPosition.north))
   {
      roadmap_log( ROADMAP_DEBUG, "Realtime_SendCurrentViewDimentions() - Skipping operation; Current coordinates where already sent...");
      return TRUE;
   }

   //   Remove all users:
   RTUsers_ResetUpdateFlag( &gs_CI.Users);

   bRes = RTNet_MapDisplyed(  &gs_CI,
                              &MapPosition,
                  roadmap_math_get_scale(0),
                              OnAsyncOperationCompleted_MapDisplayed__only,
                              NULL);
   if( bRes)
   {
      gs_CI.LastMapPosSent = MapPosition;
      roadmap_log( ROADMAP_DEBUG, "Realtime_SendCurrentViewDimentions() - Sending 'MapDisplayed'...");
   }
   else
      roadmap_log( ROADMAP_ERROR, "Realtime_SendCurrentViewDimentions(PRE) - 'RTNet_MapDisplyed()' had failed");

   return bRes;
}

BOOL SendAllMessagesTogether_BuildPacket( char* Packet)
{
   char* p = Packet;

   // See me
   if( !SendMessage_SetMyVisability( p))
   {
      roadmap_log( ROADMAP_ERROR, "SendAllMessagesTogether(PRE) - 'SendMessage_SetMyVisability()' had failed");
      return FALSE;
   }
   p = Packet + strlen(Packet);

   // At (my location)
   if( SendMessage_At( p))
      p = Packet + strlen(Packet);
   else
      roadmap_log( ROADMAP_DEBUG, "SendAllMessagesTogether(PRE) - 'SendMessage_At()' had failed; Ignoring and continueing");

   // Map displayed
   if( !SendMessage_MapDisplyed( p))
   {
      roadmap_log( ROADMAP_ERROR, "SendAllMessagesTogether(PRE) - 'SendMessage_MapDisplyed()' had failed");
      return FALSE;
   }
   p = Packet + strlen(Packet);

    // Allow new roads toggles
    if (HaveCreateNewRoadsToSend ())
    {
        if (!SendMessage_CreateNewRoads (p)) {
         roadmap_log( ROADMAP_ERROR, "SendAllMessagesTogether(PRE) - 'SendMessage_CreateNewRoads()' had failed");
         return FALSE;
        }
      p = Packet + strlen(Packet);
    }

   // GPS points path
   if( HaveGPSPointsToSend())
   {
      if( !SendMessage_GPSPath( p))
      {
         roadmap_log( ROADMAP_ERROR, "SendAllMessagesTogether(PRE) - 'SendMessage_GPSPath()' had failed");
         return FALSE;
      }
      p = Packet + strlen(Packet);
   }

   // Nodes path
   if( HaveNodePathToSend())
   {
      if( !SendMessage_NodePath( p))
      {
         roadmap_log( ROADMAP_ERROR, "SendAllMessagesTogether(PRE) - 'SendMessage_NodePath()' had failed");
         return FALSE;
      }
      p = Packet + strlen(Packet);
   }

   if( 0 < strlen( Packet))
      return TRUE;

   gs_CI.LastError = ERR_NoDataToSend;
   return FALSE;
}

BOOL SendAllMessagesTogether( BOOL bCalledAfterLogin)
{
   RTPacket Packet;
   int      iGPSPointsCount;
   int      iNodePointsCount;
   int      iAllowNewRoadsCount;
   char*    Buffer;
   BOOL     bTransactionStarted = FALSE;

   RTPacket_Init( &Packet);

   gs_pPI = editor_track_report_begin_export (0);
   iGPSPointsCount      = gs_pPI->num_points;
   iNodePointsCount     = gs_pPI->num_nodes;
   iAllowNewRoadsCount  = gs_pPI->num_update_toggles;
   Buffer               = RTPacket_Alloc( &Packet,
                            MESSAGE_MAX_SIZE__AllTogether__dynamic(iGPSPointsCount,
                                                                                iNodePointsCount,
                                                                               iAllowNewRoadsCount));

   if( SendAllMessagesTogether_BuildPacket( Buffer))
      bTransactionStarted = RTNet_GeneralPacket(&gs_CI,
                                                Buffer,
                                                OnAsyncOperationCompleted_AllTogether);
   else
      roadmap_log( ROADMAP_DEBUG, "SendAllMessagesTogether() - NOT SENDING (maybe no data to send?)");


   if( !bTransactionStarted && bCalledAfterLogin)
      OnTransactionCompleted( TRUE /* Called on behalf of the 'login', which DID succeed... */);

   if (!bTransactionStarted)
    editor_track_report_conclude_export (0);

   RTPacket_Free( &Packet);

   return bTransactionStarted;
}

void OnAsyncOperationCompleted_Login( BOOL bResult)
{
   if( !bResult)
   {
      roadmap_log( ROADMAP_ERROR, "OnAsyncOperationCompleted_Login(POST) - Failed to log in");
      OnTransactionCompleted( FALSE);
      return;
   }

   roadmap_log( ROADMAP_DEBUG, "OnAsyncOperationCompleted_Login() - User logged in!");

   SendAllMessagesTogether( TRUE /* Called After Login */);
}

void OnAsyncOperationCompleted_Register( BOOL bResult)
{
   PFN_ONASYNCCOMPLETED pfnLoginAfterRegister = gs_pfnOnLoginAfterRegister;

   gs_pfnOnLoginAfterRegister = NULL;

   assert( pfnLoginAfterRegister);

   if( bResult)
   {
      roadmap_config_set( &RT_CFG_PRM_NAME_Var,    gs_CI.UserNm);
      roadmap_config_set( &RT_CFG_PRM_PASSWORD_Var,gs_CI.UserPW);
      roadmap_config_set( &RT_CFG_PRM_NKNM_Var,    gs_CI.UserNk);

      roadmap_log(ROADMAP_DEBUG,
                  "OnAsyncOperationCompleted_Register(POST) - The 'Register' operation had succeeded! (Name: '%s'; PW: '%s')",
                  gs_CI.UserNm, gs_CI.UserPW);
   }
   else
      roadmap_log( ROADMAP_ERROR, "OnAsyncOperationCompleted_Register(POST) - The 'Register' operation had failed");

   if( !bResult || !Login( pfnLoginAfterRegister, FALSE /* Auto register? */))
      pfnLoginAfterRegister( FALSE);
}

BOOL Login( PFN_ONASYNCCOMPLETED callback, BOOL bAutoRegister)
{
   const char* szName         = roadmap_config_get( &RT_CFG_PRM_NAME_Var);
   const char* szPassword     = roadmap_config_get( &RT_CFG_PRM_PASSWORD_Var);
   const char* szNickname     = roadmap_config_get( &RT_CFG_PRM_NKNM_Var);
   gs_bShouldSendMyVisability = TRUE;

   if( szName && (*szName) && szPassword && (*szPassword))
   {
      roadmap_log( ROADMAP_DEBUG, "Login() - Trying to login user '%s'", szName);
      return RTNet_Login( &gs_CI, szName, szPassword, szNickname, callback);
   }

   // Else:

   if( !bAutoRegister)
   {
      roadmap_log( ROADMAP_DEBUG, "Login() - Do not have 'name&password' AND (bAutoRegister == FALSE) - returning FALSE...");
      return FALSE;
   }

   gs_pfnOnLoginAfterRegister = callback;
   roadmap_log( ROADMAP_DEBUG, "Login() - Do not have 'name&password', doing 'Register()'");
   return RTNet_Register( &gs_CI, OnAsyncOperationCompleted_Register);
}

BOOL TransactionStarted()
{
   if( gs_bRunning && !gs_CI.bLoggedIn)
      return Login( OnAsyncOperationCompleted_Login, TRUE /* Auto register? */);

   return SendAllMessagesTogether( FALSE /* Called After Login */);
}

BOOL NameAndPasswordAlreadyFailedAuthentication()
{
   const char* szName      = roadmap_config_get( &RT_CFG_PRM_NAME_Var);
   const char* szPassword  = roadmap_config_get( &RT_CFG_PRM_PASSWORD_Var);

   if( ERR_WrongNameOrPassword != gs_CI.LastError)
      return FALSE;

   return (!strcmp(gs_CI.UserNm, szName) && !strcmp(gs_CI.UserPW, szPassword));
}

BOOL StartTransaction()
{
   if( gs_bQuiteErrorMode)
   {
      static BOOL s_skip_this_time = FALSE;

      s_skip_this_time = !s_skip_this_time;

      if( s_skip_this_time)
      {
         roadmap_log( ROADMAP_WARNING, "StartTransaction() - QUITE ERROR MODE - Skipping this iteration (one on one off)");
         return TRUE;
      }
   }

   // Do we want to start session?
   if( ERR_Success != gs_CI.LastError)
   {
      if( NameAndPasswordAlreadyFailedAuthentication())
      {
         roadmap_log( ROADMAP_WARNING, "StartTransaction() - NOT ATTEMPTING A NEW SESSION - Last login attempt failed with 'wrong name/pw'; Login details were not modified");

         return FALSE;
      }

      gs_CI.LastError            = ERR_Success;
   }

   // Loose previous additional error info:
   gs_CI.AdditionalErrorInfo = 0;

   // START TRANSACTION:
   RTUsers_ResetUpdateFlag( &gs_CI.Users);   // User[i].updated = FALSE
   gs_CI.eTransactionStatus = TS_Active;     // We are in transaction now
   if( TransactionStarted())                 // Start...
      return TRUE;

   // TRANSACTION FAILED TO START
   //    ...rollback...

   gs_CI.eTransactionStatus = TS_Idle;       // We are idle
   RTUsers_RedoUpdateFlag( &gs_CI.Users);    // User[i].updated = TRUE  (undo above)

   if( ERR_Success == gs_CI.LastError)
      gs_CI.LastError = ERR_GeneralError;    // Last error - General error

   if( ERR_NoDataToSend == gs_CI.LastError)
      gs_CI.LastError = ERR_Success;

   return FALSE;
}

void OnTimer_Realtime(void)
{
   if( !gs_bRunning)
   {
      assert(0);
      return;
   }

   switch(gs_CI.eTransactionStatus)
   {
      case TS_Idle:
         StartTransaction();
         break;

      case TS_Active:
         RTNet_Watchdog( &gs_CI);
         break;

      default:
         return;
   }
}
//////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////
void OnSettingsChanged_EnableDisable(void)
{
   BOOL bEnabled = GetEnableDisableState();

   if( bEnabled)
   {
      if( !gs_bRunning)
         Realtime_Start();
   }
   else
   {
      if( gs_bRunning)
         Realtime_Stop( TRUE /* Enable Logout? */);
   }
}

void OnSettingsChanged_VisabilityGroup(void)
{
   gs_bShouldSendMyVisability = TRUE;
}

//////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////
void OnAddUser(LPRTUserLocation pUI)
{
   RoadMapGpsPosition   Pos;
   RoadMapDynamicString Group = roadmap_string_new( "Friends");
   RoadMapDynamicString GUI_ID= roadmap_string_new( pUI->sGUIID);
   RoadMapDynamicString Name  = roadmap_string_new( pUI->sName);
   RoadMapDynamicString Sprite= roadmap_string_new( "Friend");
   RoadMapDynamicString Image = roadmap_string_new( "Friend");

   Pos.longitude  = (int)(1000000.F * pUI->fLongitude);
   Pos.latitude   = (int)(1000000.F * pUI->fLatitude);
   Pos.altitude   = 0;   //   Hieght
   Pos.speed      = (int)pUI->fSpeed;
   Pos.steering   = pUI->iAzimuth;

   roadmap_object_add( Group, GUI_ID, Name, Sprite, Image);
   roadmap_object_move( GUI_ID, &Pos);

   roadmap_string_release( Group);
   roadmap_string_release( GUI_ID);
   roadmap_string_release( Name);
   roadmap_string_release( Sprite);
   roadmap_string_release( Image);
}

void OnMoveUser(LPRTUserLocation pUI)
{
   RoadMapGpsPosition   Pos;
   RoadMapDynamicString GUI_ID   = roadmap_string_new( pUI->sGUIID);

   Pos.longitude  = (int)(1000000.F * pUI->fLongitude);
   Pos.latitude   = (int)(1000000.F * pUI->fLatitude);
   Pos.altitude   = 0;   //   Hieght
   Pos.speed      = (int)pUI->fSpeed;
   Pos.steering   = pUI->iAzimuth;

   roadmap_object_move( GUI_ID, &Pos);

   roadmap_string_release( GUI_ID);
}

void OnRemoveUser(LPRTUserLocation pUI)
{
   RoadMapDynamicString   GUI_ID   = roadmap_string_new( pUI->sGUIID);
   roadmap_object_remove ( GUI_ID);
   roadmap_string_release( GUI_ID);
}

//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL Realtime_Report_Alert(int iAlertType, const char * szDescription, int iDirection)
{
   BOOL success;

   success = RTNet_ReportAlert(&gs_CI, iAlertType, szDescription, iDirection, OnAsyncOperationCompleted_ReportAlert);
   if( !success)
   {
      roadmap_messagebox ("Error", "Can't find current street.");
      return FALSE;
   }

   return success;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL Realtime_SendSMS(const char * szPhoneNumber)
{
   return RTNet_SendSMS(&gs_CI, szPhoneNumber, OnAsyncOperationCompleted_SendSMS);
}
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL Realtime_Post_Alert_Comment(int iAlertId, const char * szCommentText)
{
   BOOL success;
   success = RTNet_PostAlertComment(&gs_CI, iAlertId, szCommentText, OnAsyncOperationCompleted_PostComment);

   if( !success)
   {
      roadmap_messagebox ("Error", "Sending comment failed");
      return FALSE;
   }

   return success;
}


//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL Realtime_Alert_ReportAtLocation(int iAlertType, const char * szDescription, int iDirection, RoadMapGpsPosition   MyLocation){
   BOOL success = RTNet_ReportAlertAtPosition(&gs_CI, iAlertType, szDescription, iDirection, &MyLocation, OnAsyncOperationCompleted_MapDisplayed);

   if (!success) {
      roadmap_messagebox ("Error", "Sending report failed");
   }

   return success;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL Realtime_Remove_Alert(int iAlertId){
   BOOL success;
   success = RTNet_RemoveAlert(&gs_CI, iAlertId, OnTransactionCompleted);

   return success;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
void OnTransactionCompleted_ReportMarkers( BOOL bTransactionSucceeded)
{
   if( !bTransactionSucceeded)
      roadmap_log( ROADMAP_WARNING, "OnTransactionCompleted_ReportMarkers() - failed");

    if (gs_pfnOnExportMarkersResult) gs_pfnOnExportMarkersResult(bTransactionSucceeded, gs_CI.LastError);
   OnTransactionCompleted( bTransactionSucceeded);
}


//////////////////////////////////////////////////////////////////////////////////////////////////
void OnTransactionCompleted_ReportSegments( BOOL bTransactionSucceeded)
{
   if( !bTransactionSucceeded)
      roadmap_log( ROADMAP_WARNING, "OnTransactionCompleted_ReportSegments() - failed");

    if (gs_pfnOnExportSegmentsResult) gs_pfnOnExportSegmentsResult(bTransactionSucceeded, gs_CI.LastError);
   OnTransactionCompleted( bTransactionSucceeded);
}

//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL    ReportOneMarker(char* packet, int iMarker)
{
    const char*         szName;
    const char*         szType;
    const char*         szDescription;
    const char*         szKeys[ED_MARKER_MAX_ATTRS];
    char*                   szValues[ED_MARKER_MAX_ATTRS];
    int                 iKeyCount;
    RoadMapPosition position;
    int                 steering;

    editor_marker_position(iMarker, &position, &steering);
    editor_marker_export(iMarker, &szName, &szDescription,
                                szKeys, szValues, &iKeyCount);
    szType = editor_marker_type(iMarker);

    return RTNet_ReportMarker(&gs_CI, szType,
                                      position.longitude,
                                      position.latitude,
                                      steering,
                                      szDescription,
                                      iKeyCount,
                                      szKeys,
                                      (const char**)szValues,
                                      OnTransactionCompleted_ReportMarkers,
                                      packet);
}

//////////////////////////////////////////////////////////////////////////////////////////////////
char*   Realtime_Report_Markers(void)
{
   BOOL success = FALSE;
    int nMarkers;
    int iMarker;
    char*   packet;
    char*   p;

    nMarkers = editor_marker_count ();

    if( !nMarkers)
       return NULL;

    packet = malloc(nMarkers * 1024); //TBD

    p = packet;
    *p = '\0';
    for (iMarker = 0; iMarker < nMarkers; iMarker++)
    {
        if (editor_marker_committed (iMarker)) continue;
        success = ReportOneMarker(p, iMarker);
        if (!success)
        {
        roadmap_messagebox ("Error", "Sending markers failed");
        free(packet);
        return NULL;
        }

        p += strlen(p);
    }

    return packet;
}


//////////////////////////////////////////////////////////////////////////////////////////////////
char* Realtime_Report_Segments(void)
{
   BOOL success = FALSE;
    char*   packet;
    char*   p;
    int required_size = 0;
    int count;
    int i;

    count = editor_line_get_count ();
    for (i = 0; i < count; i++)
    {
        required_size += RTNet_ReportOneSegment_MaxLength (i);
    }

    if (!required_size) return NULL;

    packet = malloc (required_size);

    p = packet;
    *p = '\0';
    for (i = 0; i < count; i++)
    {
        if (editor_line_committed (i)) continue;
        success = RTNet_ReportOneSegment_Encode (p, i);
        if (!success)
        {
        roadmap_messagebox ("Error", "Sending update failed");
        free(packet);
        return NULL;
        }

        p += strlen(p);
    }


    return packet;
}


//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL    Realtime_Editor_ExportMarkers(PFN_LOGINTESTRES editor_cb)
{
   BOOL success = FALSE;
    char*   packet;

    packet = Realtime_Report_Markers ();

    if (packet) {
        if (*packet) {
            if (gs_bWritingOffline) {
                Realtime_OfflineWrite (packet);
                editor_cb (TRUE, 0);
                success = TRUE;
            } else {
                gs_pfnOnExportMarkersResult = editor_cb;
                success = RTNet_GeneralPacket(&gs_CI, packet, OnTransactionCompleted_ReportMarkers);
            }
        }
        free(packet);
    }

    return success;
}



//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL    Realtime_Editor_ExportSegments(PFN_LOGINTESTRES editor_cb)
{
   BOOL success = FALSE;
    char*   packet;

    packet = Realtime_Report_Segments ();

    if (packet) {
        if (*packet) {
            if (gs_bWritingOffline) {
                Realtime_OfflineWrite (packet);
                editor_cb (TRUE, 0);
                success = TRUE;
            } else {
                gs_pfnOnExportSegmentsResult = editor_cb;
                success = RTNet_GeneralPacket(&gs_CI, packet, OnTransactionCompleted_ReportSegments);
            }
        }
        free(packet);
    }

    return success;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BOOL Realtime_SendTrafficInfo(int mode){

    BOOL success;
    //TEMP_AVI
    return TRUE;

    success = RTNet_SendTrafficInfo(&gs_CI, mode, OnTransactionCompleted);
    return success;
}


//////////////////////////////////////////////////////////////////////////////////////////////////
#define  MAP_DRAG_EVENT__PEEK_INTERVAL             (500) // Milliseconds
#define  MAP_DRAG_EVENT__TERMINATION_THRESHOLD     (2)   // Seconds

static   BOOL     gs_bMapMoveTimerWasSet     = FALSE;
static   time_t   gs_TimeOfLastMapMoveEvent  = 0;

void OnTimePassedFromLastMapDragEvent(void)
{
   time_t   now                        = time(NULL);
   int      iSecondsPassedFromLastEvent= (int)(now - gs_TimeOfLastMapMoveEvent);

   if( iSecondsPassedFromLastEvent < MAP_DRAG_EVENT__TERMINATION_THRESHOLD)
      return;

   roadmap_main_remove_periodic( OnTimePassedFromLastMapDragEvent);
   gs_bMapMoveTimerWasSet = FALSE;

   if( gs_bRunning && gs_CI.bLoggedIn)
      Realtime_SendCurrentViewDimentions();
}

void OnMapMoved(void)
{
   if( !gs_bRunning)
   {
      assert(0);
      return;
   }

   if( !gs_CI.bLoggedIn)
      return;

   if( !gs_bMapMoveTimerWasSet)
   {
      roadmap_main_set_periodic( MAP_DRAG_EVENT__PEEK_INTERVAL, OnTimePassedFromLastMapDragEvent);
      gs_bMapMoveTimerWasSet = TRUE;
   }

   gs_TimeOfLastMapMoveEvent = time(NULL);
}

void OnDeviceEvent( device_event event, void* context )
{
   EConnectionStatus eNewStatus = gs_eConnectionStatus;
   int               iSecondsPassedFromLastGoodSession;

   roadmap_log( ROADMAP_DEBUG, "OnDeviceEvent() - Event: %d (%s)", event, get_device_event_name(event));

   switch(event)
   {
      case device_event_network_connected:
      case device_event_RS232_connection_established:
         eNewStatus = CS_Connected;
         break;

      case device_event_network_disconnected:
         eNewStatus = CS_Disconnected;
         break;

      default:
         break;
   }

   // Anything changed?
   if( eNewStatus == gs_eConnectionStatus)
      return;

   // Update current status
   gs_eConnectionStatus = eNewStatus;

   // Connected?
   if( CS_Connected != gs_eConnectionStatus)
   {
      roadmap_log( ROADMAP_DEBUG,"OnDeviceEvent() - New state: Disconnected");

      if( !gs_bWasStoppedAutoamatically)
      {
         roadmap_log( ROADMAP_INFO, "OnDeviceEvent() - !!! GOT DISCONNECTED !!! REALTIME SERVICE IS STOPPING AUTOMATICALLY !!!");
         Realtime_Stop( FALSE);
         gs_bWasStoppedAutoamatically = TRUE;
      }

      return;
   }

   roadmap_log( ROADMAP_DEBUG, "OnDeviceEvent() - New state: Connected");

   if( gs_bWasStoppedAutoamatically)
   {
      roadmap_log( ROADMAP_INFO, "OnDeviceEvent() - !!! REALTIME SERVICE AUTO-RESTART !!!");
      Realtime_Start();
      gs_bWasStoppedAutoamatically = FALSE;
      return;
   }

   // Verify we are not currently in the transaction:
   if( TS_Idle != gs_CI.eTransactionStatus)
      return;

   // Check time threshold:
   iSecondsPassedFromLastGoodSession = (int)(time(NULL) - gs_ST.timeLastGoodSession);
   if( RT_MAX_SECONDS_BETWEEN_GOOD_SESSIONS < iSecondsPassedFromLastGoodSession)
   {
      roadmap_log( ROADMAP_DEBUG, "OnDeviceEvent() - %d seconds passed from last-good-session; INITIATING A NEW SESSION!", iSecondsPassedFromLastGoodSession);
      OnTimer_Realtime();
   }
}
//////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////
int RealTimeLoginState(void){
    if (gs_CI.bLoggedIn && (ERR_Success == gs_CI.LastError))
        return 1;
    else
        return 0;

}

const char *RealTime_Get_UserName(){
    return roadmap_config_get( &RT_CFG_PRM_NAME_Var);
}
//////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////
void    RealTime_Auth (void) {

   char Command [MESSAGE_MAX_SIZE__Auth];

   if (gs_CI.bLoggedIn) {

    RTNet_Auth_BuildCommand (Command,
                                     gs_CI.iServerID,
                                     gs_CI.ServerCookie,
                                     gs_CI.UserNm,
                                     RT_DEVICE_ID,
                                     roadmap_start_version ());
   } else {

    char  Name    [(RT_USERNM_MAXSIZE * 2) + 1];
    const char* szName      = roadmap_config_get( &RT_CFG_PRM_NAME_Var);
       if( HAVE_STRING(szName))
          PackNetworkString( szName, Name, RT_USERNM_MAXSIZE);
       else
          Name[0] = '\0';

    RTNet_Auth_BuildCommand (Command,
                                     -1,
                                     "",
                                     Name,
                                     RT_DEVICE_ID,
                                     roadmap_start_version ());
   }

   Realtime_OfflineWrite (Command);
}

//////////////////////////////////////////////////////////////////////////////////////////////////
static BOOL LoginDetailsChanged()
{
   char  Name    [(RT_USERNM_MAXSIZE * 2) + 1];
   char  Password[(RT_USERPW_MAXSIZE * 2) + 1];
   char  Nickname[(RT_USERNK_MAXSIZE * 2) + 1];

   // Current settings:
   const char* szName      = roadmap_config_get( &RT_CFG_PRM_NAME_Var);
   const char* szPassword  = roadmap_config_get( &RT_CFG_PRM_PASSWORD_Var);
   const char* szNickname  = roadmap_config_get( &RT_CFG_PRM_NKNM_Var);

   // If we are not logged-in - we do not have any details cached:
   if( !gs_CI.bLoggedIn)
      return FALSE;  // Question is not applicable

   // Convert all strings to 'network strings'     (',' == "\,")
   if( HAVE_STRING(szName))
      PackNetworkString( szName, Name, RT_USERNM_MAXSIZE);
   else
      Name[0] = '\0';
   if( HAVE_STRING(szPassword))
      PackNetworkString( szPassword, Password, RT_USERPW_MAXSIZE);
   else
      Password[0] = '\0';
   if( HAVE_STRING(szNickname))
      PackNetworkString( szNickname, Nickname, RT_USERNK_MAXSIZE);
   else
      Nickname[0] = '\0';

   // Compare
   if(strcmp( Name,     gs_CI.UserNm)  ||
      strcmp( Password, gs_CI.UserPW)  ||
      strcmp( Nickname, gs_CI.UserNk))
      return TRUE;

   return FALSE;
}

void TestLoginDetailsCompleted( BOOL bRes)
{
   PFN_LOGINTESTRES  pfn   = gs_pfnOnLoginTestResult;
   gs_pfnOnLoginTestResult = NULL;
   gs_CI.eTransactionStatus= TS_Idle;

   pfn( bRes, gs_CI.LastError);
}

void OnTransactionCompleted_TestLoginDetails_Login( BOOL bTransactionSucceeded)
{
   if( !bTransactionSucceeded)
      roadmap_log( ROADMAP_WARNING, "OnTransactionCompleted_TestLoginDetails_Login() - 'Login' failed");

   TestLoginDetailsCompleted( bTransactionSucceeded);
}

BOOL TestLoginDetails_WhileNotLoggedIn()
{
   if( Login( OnTransactionCompleted_TestLoginDetails_Login, FALSE /* Auto register? */))
      return TRUE;

   TestLoginDetailsCompleted( FALSE);
   return FALSE;
}

void OnTransactionCompleted_TestLoginDetails_Logout( BOOL bTransactionSucceeded)
{
   if( !bTransactionSucceeded)
      roadmap_log( ROADMAP_WARNING, "OnTransactionCompleted_VerifyLoginDetails() - 'Logout' failed");

   Realtime_ResetLoginState(TRUE /* Redraw */);
   TestLoginDetails_WhileNotLoggedIn();
}

BOOL TestLoginDetails()
{
   BOOL  bLogoutInProcess  = FALSE;

   // Catch the process flag:
   gs_CI.eTransactionStatus= TS_Active;

   if( gs_CI.bLoggedIn)
   {
      if( !LoginDetailsChanged())
      {
         // We are currently logged-in, and login-details did not change
         TestLoginDetailsCompleted( TRUE);
         return TRUE;
      }

      roadmap_log( ROADMAP_DEBUG, "TestLoginDetails() - Login details changed and we are logged in; LOGGING OUT");

      if( RTNet_Logout( &gs_CI, OnTransactionCompleted_TestLoginDetails_Logout))
         bLogoutInProcess  = TRUE;
      else
         roadmap_log( ROADMAP_ERROR, "TestLoginDetails() - 'RTNet_Logout()' had failed");
   }
   else
   {
      if( NameAndPasswordAlreadyFailedAuthentication())
      {
         TestLoginDetailsCompleted( FALSE);
         return TRUE;
      }
   }

   if( bLogoutInProcess)
      return TRUE;

   Realtime_ResetLoginState(TRUE /* Redraw */);
   return TestLoginDetails_WhileNotLoggedIn();
}

BOOL Realtime_VerifyLoginDetails( PFN_LOGINTESTRES pfn)
{
   if( !gs_bInitialized || !pfn || gs_pfnOnLoginTestResult)
   {
      assert(0);
      return FALSE;
   }

   gs_pfnOnLoginTestResult = pfn;

   if( gs_CI.eTransactionStatus != TS_Idle)
      return TRUE;

   return TestLoginDetails();
}

void OnAsyncOperationCompleted_ReportOnNavigation( BOOL bResult)
{
   if( bResult)
      roadmap_log( ROADMAP_DEBUG, "OnAsyncOperationCompleted_ReportOnNavigation() - 'MapDisplayed' was sent successfully");
   else
      roadmap_log( ROADMAP_ERROR, "OnAsyncOperationCompleted_ReportOnNavigation(POST) - 'MapDisplayed' had failed");

   OnTransactionCompleted( bResult);
}

BOOL Realtime_ReportOnNavigation( const RoadMapPosition* cordinates, address_info_ptr aiptr)
{
   address_info   ai = (*aiptr);
   BOOL           bRes;

   NULLSTRING_TO_EMPTYSTRING(ai.state)
   NULLSTRING_TO_EMPTYSTRING(ai.country)
   NULLSTRING_TO_EMPTYSTRING(ai.city)
   NULLSTRING_TO_EMPTYSTRING(ai.street)
   NULLSTRING_TO_EMPTYSTRING(ai.house)

   if( !gs_bInitialized)
   {
//      assert(0);
      return FALSE;
   }

   if( !gs_bRunning)
   {
      roadmap_log( ROADMAP_ERROR, "Realtime_ReportOnNavigation() - Realtime service is currently disabled; Exiting method");
      return FALSE;
   }

   bRes = RTNet_NavigateTo(   &gs_CI,
                              cordinates,
                              &ai,
                              OnAsyncOperationCompleted_ReportOnNavigation);
   if( bRes)
      roadmap_log( ROADMAP_DEBUG, "Realtime_ReportOnNavigation()");
   else
      roadmap_log( ROADMAP_ERROR, "Realtime_ReportOnNavigation(PRE) - 'RTNet_NavigateTo()' had failed");

   return bRes;
}

static int keyboard_callback(int type, const char *new_value, void* context )
{
    BOOL success;

    if (type != SSD_KEYBOARD_OK)
        return 1;

    if (new_value[0] == 0)
        return 1;

    success = Realtime_SendSMS(new_value);
    if (success){
        ssd_keyboard_hide_all();
    }
    else
        ssd_keyboard_hide();

    return 1;

}

///////////////////////////////////////////////////////////////////////////////////////////
static void recommend_waze_dialog_callbak(int exit_code, void* context ){

    if (exit_code != dec_yes)
         return;

#ifdef __SYMBIAN32__
    ShowEditbox(roadmap_lang_get("Phone number"), "",
            keyboard_callback, NULL, EEditBoxEmptyForbidden );
#else
    ssd_keyboard_show (SSD_KEYBOARD_DIGITS,
            roadmap_lang_get("Phone number"), "", NULL, keyboard_callback, NULL);
#endif

}

void RecommentToFriend(){

    ssd_confirm_dialog("Recommend Waze to a friend", "To recommend to a friend, please enter their phone number. Send?", FALSE, recommend_waze_dialog_callbak,  NULL);
}


void Realtime_DumpOffline (void)
{
   RTPacket     Packet;
   int          iPoint;
   int          iNode;
   int          iToggle;
   char*        Buffer;
   RTPathInfo   pi;
   RTPathInfo   *pOrigPI;

    Realtime_OfflineOpen (editor_sync_get_export_path (),
                                 editor_sync_get_export_name ());

   RTPacket_Init( &Packet);

   pOrigPI = editor_track_report_begin_export (1);

   if (pOrigPI && pOrigPI->num_nodes + pOrigPI->num_points + pOrigPI->num_update_toggles > 0)
   {
       Buffer               = RTPacket_Alloc( &Packet,
                                MESSAGE_MAX_SIZE__AllTogether__dynamic(RTTRK_GPSPATH_MAX_POINTS,
                                                                                    RTTRK_NODEPATH_MAX_POINTS,
                                                                                   RTTRK_CREATENEWROADS_MAX_TOGGLES));

       pi = *pOrigPI;

       for (iToggle = 0;
          iToggle < pOrigPI->num_update_toggles;
          iToggle += RTTRK_CREATENEWROADS_MAX_TOGGLES)
        {
            pi.num_update_toggles = pOrigPI->num_update_toggles - iToggle;
            if (pi.num_update_toggles > RTTRK_CREATENEWROADS_MAX_TOGGLES)
                pi.num_update_toggles = RTTRK_CREATENEWROADS_MAX_TOGGLES;
            pi.update_toggle_times = pOrigPI->update_toggle_times + iToggle;

            gs_pPI = &pi;
            if (SendMessage_CreateNewRoads (Buffer))
            {
                Realtime_OfflineWrite (Buffer);
            }
        }

       for (iPoint = 0;
          iPoint < pOrigPI->num_points;
          iPoint += RTTRK_GPSPATH_MAX_POINTS)
        {
            pi.num_points = pOrigPI->num_points - iPoint;
            if (pi.num_points > RTTRK_GPSPATH_MAX_POINTS)
                pi.num_points = RTTRK_GPSPATH_MAX_POINTS;
            pi.points = pOrigPI->points + iPoint;

            gs_pPI = &pi;
            if (SendMessage_GPSPath (Buffer))
            {
                Realtime_OfflineWrite (Buffer);
            }
        }
       for (iNode = 0;
          iNode < pOrigPI->num_nodes;
          iNode += RTTRK_NODEPATH_MAX_POINTS)
        {
            pi.num_nodes = pOrigPI->num_nodes - iNode;
            if (pi.num_nodes > RTTRK_NODEPATH_MAX_POINTS)
                pi.num_nodes = RTTRK_NODEPATH_MAX_POINTS;
            pi.nodes = pOrigPI->nodes + iNode;

            gs_pPI = &pi;
            if (SendMessage_NodePath (Buffer))
            {
                Realtime_OfflineWrite (Buffer);
            }
        }
        RTPacket_Free (&Packet);
   }
   editor_track_report_conclude_export (1);

   gs_bWritingOffline = TRUE;
   editor_report_markers ();
   editor_report_segments ();
   gs_bWritingOffline = FALSE;

    Realtime_OfflineClose ();
}
