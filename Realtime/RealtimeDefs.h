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


#ifndef   __FREEMAP_REALTIMEDEFS_H__
#define   __FREEMAP_REALTIMEDEFS_H__
//////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////
#include <time.h>
#include "Realtime/LMap_Base.h"

#define  RT_INVALID_LOGINID_VALUE                        (-1)
#define  RT_MAX_SECONDS_BETWEEN_GOOD_SESSIONS            (3600)   /* One hour */

#define  RT_THRESHOLD_TO_DISABLE_SERVICE__MAX_NETWORK_ERRORS                  (1000)
#define  RT_THRESHOLD_TO_DISABLE_SERVICE__MAX_NETWORK_ERRORS_SUCCESSIVE       (100)
#define  RT_THRESHOLD_TO_DISABLE_SERVICE__MAX_SECONDS_FROM_LAST_SESSION       (20*60)
#define  RT_THRESHOLD_TO_ENTER_SILENT_MODE__MAX_NETWORK_ERRORS_SUCCESSIVE     (3)
//////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////
#define  RT_CFG_TYPE                   ("preferences")
#define  RT_USER_TYPE				   ("user")
#define  RT_CFG_TAB                    ("Realtime")

//   User name
#define  RT_CFG_PRM_NAME_Var           RTPrm_Name
#define  RT_CFG_PRM_NAME_Name          ("Name")
#define  RT_CFG_PRM_NAME_Default       ("")

//   User password
#define  RT_CFG_PRM_PASSWORD_Var       RTPrm_Password
#define  RT_CFG_PRM_PASSWORD_Name      ("Password")
#define  RT_CFG_PRM_PASSWORD_Default   ("")

//   User nickname
#define  RT_CFG_PRM_NKNM_Var           RTPrm_Nickname
#define  RT_CFG_PRM_NKNM_Name          ("Nickname")
#define  RT_CFG_PRM_NKNM_Default       ("")

//   Enable / Disable:
#define  RT_CFG_PRM_STATUS_Var         RTPrm_Status
#define  RT_CFG_PRM_STATUS_Name        ("Status")
#define  RT_CFG_PRM_STATUS_Enabled     ("Enabled")
#define  RT_CFG_PRM_STATUS_Disabled    ("Disabled")

//   Refresh rate
#define  RT_CFG_PRM_REFRAT_Var         RTPrm_RefreshRate
#define  RT_CFG_PRM_REFRAT_Name        ("Refresh rate (minutes)")
#define  RT_CFG_PRM_REFRAT_Default     ("0.5")
#define  RT_CFG_PRM_REFRAT_iMin        (0.1F)
#define  RT_CFG_PRM_REFRAT_iMax        (30.F)
#define  RT_CFG_PRM_REFRAT_iDef        (4.F)
#define  RT_CFG_PRM_REFRAT_iWD         (15.F)   //   Watchdog

//   Remote web-service address
#define  RT_CFG_PRM_WEBSRV_Var         RTPrm_WebServiceAddress
#define  RT_CFG_PRM_WEBSRV_Name        ("Web-Service Address")
#define  RT_CFG_PRM_WEBSRV_Default     ("http://212.150.51.90:80/rtserver")
//#define  RT_CFG_PRM_WEBSRV_Default     ("http://62.219.147.126:80/rtserver")

const char*  RT_CFG_GetWebServiceAddress();

//   Visability group:
#define  RT_CFG_PRM_VISGRP_Var         RTPrm_VisabilityGroup
#define  RT_CFG_PRM_VISGRP_Name        ("Visability Group")
#define  RT_CFG_PRM_VISGRP_Nickname    ("Nickname")
#define  RT_CFG_PRM_VISGRP_Anonymous   ("Anonymous")
#define  RT_CFG_PRM_VISGRP_Invisible   ("Invisible")


//   Visability group:
#define  RT_CFG_PRM_VISREP_Var         RTPrm_VisabilityReport
#define  RT_CFG_PRM_VISREP_Name        ("Visability Driving")
#define  RT_CFG_PRM_VISREP_Nickname    ("ReportNickname")
#define  RT_CFG_PRM_VISREP_Anonymous   ("ReportAnonymous")


/*   My visability:
#define  RT_CFG_PRM_MYVSBL_Var         RTPrm_MyVisability
#define  RT_CFG_PRM_MYVSBL_Name        ("My visability")
#define  RT_CFG_PRM_MYVSBL_None        ("Invisible")
#define  RT_CFG_PRM_MYVSBL_Frnd        ("Only my friends can see me")
#define  RT_CFG_PRM_MYVSBL_All         ("Visible to everyone")

//   My ROI
#define  RT_CFG_PRM_INTRST_Var         RTPrm_OtherVisability
#define  RT_CFG_PRM_INTRST_Name        ("I want to see")
#define  RT_CFG_PRM_INTRST_None        ("No one")
#define  RT_CFG_PRM_INTRST_Frnd        ("My friends")
#define  RT_CFG_PRM_INTRST_All         ("Everyone")   */
//////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////
#include "Realtime/RealtimeUsers.h"
#include "Realtime/RealtimeSystemMessage.h"
#include "../roadmap_internet.h"
//////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////
// Visability group:
typedef enum tagERTVisabilityGroup
{
   VisGrp_Invisible = 1,
   VisGrp_NickName,
   VisGrp_Anonymous,

   VisGrp__count,
   VisGrp__invalid
   
}  ERTVisabilityGroup;


//////////////////////////////////////////////////////////////////////////////////////////////////
// Visability Report group:
typedef enum tagERTVisabilityReport
{
   
   VisRep_Anonymous = 1,
   VisRep_NickName ,
   
   VisRep__count,
   VisRep__invalid
   
}  ERTVisabilityReport;

const char*          ERTVisabilityGroup_to_string  ( ERTVisabilityGroup e);
ERTVisabilityGroup   ERTVisabilityGroup_from_string( const char* szE);

ERTVisabilityReport  ERTVisabilityReport_from_string( const char* szE);
const char*          ERTVisabilityReport_to_string( ERTVisabilityReport e);

//////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////
#define  COORDINATE_VALUE_STRING_MAXSIZE           (18)  /* "350.0123456789"  */
#define  VERSION_STRING_MAXSIZE                    (32)

typedef enum tagEVersionUpgradeSeverity
{
   VUS_NA,
   
   VUS_Low,
   VUS_Medium,
   VUS_Hi

}  EVersionUpgradeSeverity;

typedef struct tagVersionUpgradeInfo
{
   EVersionUpgradeSeverity eSeverity;
   char                    NewVersion[VERSION_STRING_MAXSIZE+1];
   char                    URL[GENERAL_URL_MAXSIZE+1];

}  VersionUpgradeInfo, *LPVersionUpgradeInfo;

void  VersionUpgradeInfo_Init( LPVersionUpgradeInfo this);
//////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////
typedef enum tagEConnectionStatus
{
   CS_Unknown,
   CS_Connected,
   CS_Disconnected

}  EConnectionStatus;

typedef struct tagStatusStatistics
{
   time_t   timeLastGoodSession;
   int      iNetworkErrors;
   int      iNetworkErrorsSuccessive;

}  StatusStatistics, *LPStatusStatistics;

void  StatusStatistics_Init   ( LPStatusStatistics this);
void  StatusStatistics_Reset  ( LPStatusStatistics this);

typedef enum tagETransactionStatus
{
   TS_Idle,
   TS_Active,
   TS_Stopping,
   
   TS__count,
   TS__invalid

}  ETransactionStatus;
//////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////
#endif   //   __FREEMAP_REALTIMEDEFS_H__
