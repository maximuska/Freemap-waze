/* RealtimeAlerts.c - Manage real time alerts
 *
 * LICENSE:
 *
 *   Copyright 2008 Avi B.S
 *   Copyright 2008 Ehud Shabtai
 *
 *   This file is part of RoadMap.
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
 *
 *
 */

#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "../ssd/ssd_keyboard.h"
#include "../roadmap_main.h"
#include "../roadmap_factory.h"
#include "../roadmap_config.h"
#include "../roadmap_navigate.h"
#include "../roadmap_math.h"
#include "../roadmap_object.h"
#include "../roadmap_types.h"
#include "../roadmap_alerter.h"
#include "../roadmap_db_alert.h"
#include "../roadmap_display.h"
#include "../roadmap_time.h"
#include "../roadmap_messagebox.h"
#include "../roadmap_lang.h"
#include "../roadmap_trip.h"
#include "../roadmap_res.h"
#include "../roadmap_layer.h"
#include "../roadmap_square.h"
#include "../roadmap_locator.h"
#include "../roadmap_line_route.h"
#include "../roadmap_line.h"
#include "../roadmap_gps.h"
#include "../roadmap_softkeys.h"
#include "../editor/db/editor_street.h"

#include "../navigate/navigate_main.h"

#include "RealtimeAlerts.h"
#include "RealtimeAlertsList.h"
#include "RealtimeNet.h"
#include "Realtime.h"

#include "ssd/ssd_dialog.h"
#include "ssd/ssd_container.h"
#include "ssd/ssd_text.h"
#include "ssd/ssd_choice.h"
#include "ssd/ssd_button.h"
#include "ssd/ssd_menu.h"

static RTAlerts gAlertsTable;
static int gIterator;
static int gScrolling;
static int gIdleCount;
static int gIdleScrolling;
static int gCurrentAlertId;
static BOOL gTimerActive;
static RTAlertCommentsEntry *gCurrentComment;
static BOOL gCentered;
static int gState =STATE_NONE;
static int gSavedZoom = -1;
static RoadMapPosition gSavedPosition;

static int gPopUpType = -1;

#define POP_UP_COMMENT 1
#define POP_UP_ALERT   2

static void RTAlerts_Timer(void);
static void OnAlertAdd(RTAlert *pAlert);
static void OnAlertRemove(void);
static void create_ssd_dialog(RTAlert *pAlert);
static void RTAlerts_Comment_PopUp(RTAlertComment *Comment, RTAlert *Alert);
static void RTAlerts_Comment_PopUp_Hide(void);
#define CENTER_NONE             -1
#define CENTER_ON_ALERT  1
#define CENTER_ON_ME           2

roadmap_alert_providor RoadmapRealTimeAlertProvidor =
{ "RealTimeAlerts", RTAlerts_Count, RTAlerts_Get_Id, RTAlerts_Get_Position,
        RTAlerts_Get_Speed, RTAlerts_Get_Map_Icon, RTAlerts_Get_Alert_Icon,
        RTAlerts_Get_Warn_Icon, RTAlerts_Get_Distance, RTAlerts_Get_Sound,
        RTAlerts_Is_Alertable, RTAlerts_Get_String, RTAlerts_Is_Cancelable, Rtalerts_Delete };

/**
 * calculate the delta between two azymuths
 * @param az1, az2 - the azymuths
 * @return the delta
 */
static int azymuth_delta(int az1, int az2)
{

    int delta;

    delta = az1 - az2;

    while (delta > 180)
        delta -= 360;
    while (delta < -180)
        delta += 360;

    return delta;
}

/**
 * Initialize the an alert structure
 * @param pAlert - pointer to the alert
 * @return None
 */
void RTAlerts_Alert_Init(RTAlert *pAlert)
{
    pAlert->iID = -1;
    pAlert->iLongitude = 0;
    pAlert->iLatitude = 0;
    pAlert->iAzymuth = 0;
    pAlert->iType = 0;
    pAlert->iSpeed = 0;
    pAlert->i64ReportTime = 0L;
    pAlert->iNode1 = 0;
    pAlert->iNode2 = 0;
    pAlert->iDistance = 0;
    pAlert->bAlertByMe = FALSE;
    memset(pAlert->sReportedBy, 0, RT_ALERT_USERNM_MAXSIZE);
    memset(pAlert->sDescription, 0, RT_ALERT_DESCRIPTION_MAXSIZE);
    memset(pAlert->sLocationStr, 0, RT_ALERT_LOCATION_MAX_SIZE);
    pAlert->iNumComments = 0;
    pAlert->Comment = NULL;
}

/**
 * Initialize the Realtime alerts
 * @param None
 * @return None
 */
void RTAlerts_Init()
{
    int i;

    for (i=0; i<RT_MAXIMUM_ALERT_COUNT; i++)
        gAlertsTable.alert[i] = NULL;

    gAlertsTable.iCount = 0;
    roadmap_alerter_register(&RoadmapRealTimeAlertProvidor);
    gScrolling = FALSE;
    gIdleScrolling = FALSE;
    gIterator = 0;
    gCurrentAlertId = -1;
    gIdleScrolling = FALSE;
    gTimerActive = FALSE;

    gSavedZoom = -1;
    gSavedPosition.latitude = -1;
    gSavedPosition.longitude = -1;
}

/**
 * Terminate the Realtime alerts
 * @param None
 * @return None
 */
void RTAlerts_Term()
{
    RTAlerts_Clear_All();
}

/**
 * The number of alerts currently in the table
 * @param None
 * @return the number of alerts
 */
int RTAlerts_Count(void)
{
    return gAlertsTable.iCount;
}


/**
 * The STRING of alerts currently in the table
 * @param None
 * @return string with number of alerts
 */
const char * RTAlerts_Count_Str(void)
{
    static char text[20];
    sprintf(text,"%d",gAlertsTable.iCount);
    return &text[0];
}


/**
 * Checks whether the alerts table is empty
 * @param None
 * @return TRUE if there are no alerts, FALSE otherwise
 */
BOOL RTAlerts_Is_Empty()
{
    return (0 == gAlertsTable.iCount);
}

/**
 * Retrieve an alert from table by record number
 * @param record - the record number in the table
 * @return alert - pointer to the alert
 */
RTAlert * RTAlerts_Get(int record)
{
    return gAlertsTable.alert[record];
}

/**
 * Retrieve an alert from table by alert ID
 * @param iID - The id of the alert to retrieve
 * @return alert - pointer to the alert, NULL if not found
 */
RTAlert *RTAlerts_Get_By_ID(int iID)
{
    int i;

    //   Find alert:
    for (i=0; i< gAlertsTable.iCount; i++)
        if (gAlertsTable.alert[i]->iID == iID)
        {
            return (gAlertsTable.alert[i]);
        }

    return NULL;
}

/**
 * Clear all alerts from alerts table
 * @param None
 * @return None
 */
void RTAlerts_Clear_All()
{
    int i;
    RTAlert * pAlert;

    for (i=0; i<gAlertsTable.iCount; i++)
    {
        pAlert = RTAlerts_Get(i);
        RTAlerts_Delete_All_Comments(pAlert);
        free(pAlert);
        gAlertsTable.alert[i] = NULL;
    }

    OnAlertRemove();

    gAlertsTable.iCount = 0;

}

/**
 * Checks whether an alerts ID exists
 * @param iID - the id to check
 * @return TRUE the alert exist, FALSE otherwise
 */
BOOL RTAlerts_Exists(int iID)
{
    if ( NULL == RTAlerts_Get_By_ID(iID))
        return FALSE;

    return TRUE;
}


void RTAlerts_hide_softkeys(){
    roadmap_softkeys_remove_left_soft_key("Comment");
    roadmap_softkeys_remove_right_soft_key("Hide");
    roadmap_softkeys_remove_right_soft_key("Me on map");
}

/**
 * Returns the state of the Realtime Alerts
 * @param None
 * @return STATE_OLD, STATE_NEW, STATE_SCROLLING, STATE_NEW_COMMENT
 */
int RTAlerts_State()
{
    if (RTAlerts_Is_Empty())
        return STATE_NONE;
    else
        return gState;
}

/**
 * Returns the state of the Realtime Alerts
 * @param None
 * @return STATE_OLD, STATE_NEW, STATE_SCROLLING, STATE_NEW_COMMENT
 */
int RTAlerts_ScrollState()
{
    int sign_active = roadmap_display_is_sign_active("Real Time Alert Pop Up");

   if (gScrolling & sign_active)
        return 0;
   else
       return -1;
}
/**
 * Add an alert to the list of the alerts
 * @param pAlert - pointer to the alert
 * @return TRUE operation was successful
 */
BOOL RTAlerts_Add(RTAlert *pAlert)
{
    int iDirection;
    RoadMapPosition position;
    RoadMapPosition lineFrom, lineTo;
    const char *street;
    const char *city;
    int iLineId;
    int line_from_point;
    int line_to_point;
    int line_azymuth;
    int delta;
    int         iSquare;


    // Full?
    if ( RT_MAXIMUM_ALERT_COUNT == gAlertsTable.iCount)
        return FALSE;

    // Already exists?
    if (RTAlerts_Exists(pAlert->iID))
    {
        roadmap_log( ROADMAP_INFO, "RTAlerts_Add - cannot add Alert  (%d) alert already exist", pAlert->iID);
        return TRUE;
    }

    if (pAlert->iType > RT_ALERTS_LAST_KNOWN_STATE)
    {
        roadmap_log( ROADMAP_WARNING, "RTAlerts_Add - add Alert(%d) unkown type (type=%d)", pAlert->iID, pAlert->iType);
        return TRUE;
    }

    if ((gState != STATE_NEW_COMMENT) && (!pAlert->bAlertByMe))
        gState = STATE_NEW;

     // in case the only alert is mine. set the state to old.
     if ((RTAlerts_Is_Empty())&& (pAlert->bAlertByMe))
        gState = STATE_OLD;

    gAlertsTable.alert[gAlertsTable.iCount] = calloc(1, sizeof(RTAlert));
    if (gAlertsTable.alert[gAlertsTable.iCount] == NULL)
    {
        roadmap_log( ROADMAP_ERROR, "RTAlerts_Add - cannot add Alert  (%d) calloc failed", pAlert->iID);
        return FALSE;
    }

    RTAlerts_Alert_Init(gAlertsTable.alert[gAlertsTable.iCount]);
    gAlertsTable.alert[gAlertsTable.iCount]->iLatitude = pAlert->iLatitude;
    gAlertsTable.alert[gAlertsTable.iCount]->iLongitude = pAlert->iLongitude;
    gAlertsTable.alert[gAlertsTable.iCount]->iID = pAlert->iID;
    gAlertsTable.alert[gAlertsTable.iCount]->iType = pAlert->iType;
    gAlertsTable.alert[gAlertsTable.iCount]->iSpeed = pAlert->iSpeed;
    gAlertsTable.alert[gAlertsTable.iCount]->i64ReportTime
            = pAlert->i64ReportTime;
    gAlertsTable.alert[gAlertsTable.iCount]->bAlertByMe = pAlert->bAlertByMe;
    strncpy(gAlertsTable.alert[gAlertsTable.iCount]->sDescription,
            pAlert->sDescription, RT_ALERT_DESCRIPTION_MAXSIZE);
    strncpy(gAlertsTable.alert[gAlertsTable.iCount]->sReportedBy,
            roadmap_lang_get(pAlert->sReportedBy), RT_ALERT_USERNM_MAXSIZE);

    position.longitude = pAlert->iLongitude;
    position.latitude = pAlert->iLatitude;


    // Save Location Descrition string
    RTAlerts_Get_City_Street(position, &city, &street, &iSquare, &iLineId, pAlert->iDirection);

    if (pAlert->sLocationStr[0] == 0){
         if (!((city == NULL) && (street == NULL)))
         {
          if ((city != NULL) && (strlen(city) == 0))
               snprintf(
                        gAlertsTable.alert[gAlertsTable.iCount]->sLocationStr
                             + strlen(gAlertsTable.alert[gAlertsTable.iCount]->sLocationStr),
                      sizeof(gAlertsTable.alert[gAlertsTable.iCount]->sLocationStr)
                               - strlen(gAlertsTable.alert[gAlertsTable.iCount]->sLocationStr),
                            "%s", street);
                else if ((street != NULL) && (strlen(street) == 0))
                snprintf(
                     gAlertsTable.alert[gAlertsTable.iCount]->sLocationStr
                              + strlen(gAlertsTable.alert[gAlertsTable.iCount]->sLocationStr),
                        sizeof(gAlertsTable.alert[gAlertsTable.iCount]->sLocationStr)
                             - strlen(gAlertsTable.alert[gAlertsTable.iCount]->sLocationStr),
                      "%s", city);
                else
                snprintf(
                     gAlertsTable.alert[gAlertsTable.iCount]->sLocationStr
                              + strlen(gAlertsTable.alert[gAlertsTable.iCount]->sLocationStr),
                        sizeof(gAlertsTable.alert[gAlertsTable.iCount]->sLocationStr)
                             - strlen(gAlertsTable.alert[gAlertsTable.iCount]->sLocationStr),
                      "%s, %s", street, city);
      }
    }
    else{
        strcpy(gAlertsTable.alert[gAlertsTable.iCount]->sLocationStr, pAlert->sLocationStr);
    }
    gAlertsTable.alert[gAlertsTable.iCount]->iSquare = iSquare;
    gAlertsTable.alert[gAlertsTable.iCount]->iLineId = iLineId;
    gAlertsTable.alert[gAlertsTable.iCount]->iDirection = pAlert->iDirection;

    if (pAlert->iDirection == RT_ALERT_OPPSOITE_DIRECTION)
    {
        iDirection = pAlert->iAzymuth + 180;
        while (iDirection > 360)
            iDirection -= 360;
        gAlertsTable.alert[gAlertsTable.iCount]->iAzymuth = iDirection;
    }
    else
    {
        gAlertsTable.alert[gAlertsTable.iCount]->iAzymuth = pAlert->iAzymuth;
    }

    if (iLineId != -1){
        roadmap_line_points(iLineId, &line_from_point, &line_to_point);
        roadmap_line_from(iLineId, &lineFrom);
        roadmap_line_to(iLineId, &lineTo);
        line_azymuth = roadmap_math_azymuth(&lineFrom, &lineTo);
        delta = azymuth_delta(gAlertsTable.alert[gAlertsTable.iCount]->iAzymuth,
                line_azymuth);
        if ((delta > 90) || (delta < -90))
        {
            gAlertsTable.alert[gAlertsTable.iCount]->iNode1 = line_to_point;
            gAlertsTable.alert[gAlertsTable.iCount]->iNode2 = line_from_point;
        }
        else
        {
            gAlertsTable.alert[gAlertsTable.iCount]->iNode1 = line_from_point;
            gAlertsTable.alert[gAlertsTable.iCount]->iNode2 = line_to_point;
        }
    }
    gAlertsTable.iCount++;
    OnAlertAdd(gAlertsTable.alert[gAlertsTable.iCount-1]);
    return TRUE;
}

/**
 * Initialize a comment
 * @param comment - pointer to the comment
 * @return None
 */
void RTAlerts_Comment_Init(RTAlertComment *comment)
{
    comment->iID = -1;
    comment->iAlertId = -1;
    comment->i64ReportTime = 0L;
    comment->bCommentByMe = FALSE;
    memset(comment->sPostedBy, 0, RT_ALERT_USERNM_MAXSIZE);
    memset(comment->sDescription, 0, RT_ALERT_DESCRIPTION_MAXSIZE);
}

/**
 * Checks whether a comment ID exists for that alert
 * @param alert - pointer to the alert, iCommentId the id to check
 * @return TRUE the comment exist, FALSE otherwise
 */
BOOL RTAlerts_Comment_Exist(RTAlert *pAlert, int iCommentId)
{

    RTAlertCommentsEntry * current = pAlert->Comment;

    while (current != NULL)
    {
        if (current->comment.iID == iCommentId)
            return TRUE;
        current = current->next;
    }

    return FALSE;
}

/**
 * Add a comment to the comments list of the alert
 * @param comment - pointer to the comment
 * @return TRUE operation was successful
 */

BOOL RTAlerts_Comment_Add(RTAlertComment *Comment)
{
    BOOL bCommentedByMe= FALSE;
    RTAlert *pAlert;
    RTAlertCommentsEntry * current;
    RTAlertCommentsEntry *CommentEntry;

    pAlert = RTAlerts_Get_By_ID(Comment->iAlertId);
    if (pAlert == NULL)
        return FALSE;

    if (RTAlerts_Comment_Exist(pAlert, Comment->iID))
        return TRUE;

    if (Comment->sDescription[0] == 0)
        return TRUE;

    CommentEntry = calloc(1, sizeof(RTAlertCommentsEntry));
    CommentEntry->comment.iID = Comment->iID;
    CommentEntry->comment.iAlertId = Comment->iAlertId;
    CommentEntry->comment.i64ReportTime = Comment->i64ReportTime;
    CommentEntry->comment.bCommentByMe = Comment->bCommentByMe;
    strncpy(CommentEntry->comment.sDescription, Comment->sDescription,
                RT_ALERT_DESCRIPTION_MAXSIZE);
    strncpy(CommentEntry->comment.sPostedBy, roadmap_lang_get(Comment->sPostedBy),
                RT_ALERT_USERNM_MAXSIZE);

    CommentEntry->previous = NULL;
    CommentEntry->next = NULL;

    current= pAlert->Comment;
    if (current != NULL)
    {
        while (current != NULL)
        {
            if (current->comment.bCommentByMe)
                bCommentedByMe = TRUE;
            if (current->next == NULL)
            {
                current->next = CommentEntry;
                CommentEntry->previous = current;
                current = NULL;
            }
            else
            {
                current = current->next;
            }
        }
    }
    else
    { /* the list is empty. = */
        pAlert->Comment = CommentEntry;
    }

    pAlert->iNumComments++;


    if (!Comment->bCommentByMe){
        if (pAlert->bAlertByMe || bCommentedByMe){
            gState = STATE_NEW_COMMENT;
            RTAlerts_Comment_PopUp(Comment,pAlert);
        }
        else
            gState = STATE_NEW;
    }



    return TRUE;
}

/**
 * Remove all comments of an alert
 * @param pAlert - pointer to the alert
 * @return None
 */
void RTAlerts_Delete_All_Comments(RTAlert *pAlert)
{
    if (pAlert->Comment != NULL)
    {
        RTAlertCommentsEntry * current =
                pAlert->Comment;
        while (current != NULL)
        {
            RTAlertCommentsEntry * new_current = current->next;
            free(current);
            current = new_current;
        }
    }

}

/**
 * Remove an alert from table
 * @param iID - Id of the alert
 * @return TRUE - the delete was successfull, FALSE the delete failed
 */
BOOL RTAlerts_Remove(int iID)
{
    BOOL bFound= FALSE;

    //   Are we empty?
    if ( 0 == gAlertsTable.iCount)
        return FALSE;

    if (gAlertsTable.alert[gAlertsTable.iCount-1]->iID == iID)
    {
        RTAlerts_Delete_All_Comments(gAlertsTable.alert[gAlertsTable.iCount-1]);
        free(gAlertsTable.alert[gAlertsTable.iCount-1]);
        bFound = TRUE;
    }
    else
    {
        int i;

        for (i=0; i<(gAlertsTable.iCount-1); i++)
        {
            if (bFound)
                gAlertsTable.alert[i] = gAlertsTable.alert[i+1];
            else
            {
                if (gAlertsTable.alert[i]->iID == iID)
                {
                    RTAlerts_Delete_All_Comments(gAlertsTable.alert[i]);
                    free(gAlertsTable.alert[i]);
                    gAlertsTable.alert[i] = gAlertsTable.alert[i+1];
                    bFound = TRUE;
                }
            }
        }
    }

    if (bFound)
    {
        gAlertsTable.iCount--;

        gAlertsTable.alert[gAlertsTable.iCount] = NULL;

        OnAlertRemove();
    }

    return bFound;
}

/**
 * Get the number of comments for a specific alert
 * @param iAlertId - Id of the alert
 * @return The number of alerts
 */
int RTAlerts_Get_Number_of_Comments(int iAlertId)
{
    RTAlert *pAlert = RTAlerts_Get_By_ID(iAlertId);
    if (pAlert == NULL)
        return 0;
    else
        return pAlert->iNumComments;
}

/**
 * Get the number of position for a specific alert
 * @param record - The alert record
 * @param position - pointer to the RoadMapPosition of the alert
 * @param steering - The steering of the alert
 * @None
 */
void RTAlerts_Get_Position(int record, RoadMapPosition *position, int *steering)
{

    RTAlert *pAlert = RTAlerts_Get(record);
    if (pAlert == NULL)
        return;

    position->longitude = pAlert->iLongitude;
    position->latitude = pAlert->iLatitude;

    *steering = pAlert->iAzymuth;
}

/**
 * Get the type of an Alert
 * @param record - The Record number in the table
 * @return The type of Alert
 */
int RTAlerts_Get_Type(int record)
{

    RTAlert *pAlert = RTAlerts_Get(record);
    assert(pAlert != NULL);
    if (pAlert != NULL)
        return pAlert->iType;
    else
        return 0;
}

/**
 * Get the ID of an Alert
 * @param record - The Record number in the table
 * @return The id of Alert, -1 the record does not exist
 */
int RTAlerts_Get_Id(int record)
{
    RTAlert *pAlert = RTAlerts_Get(record);
    assert(pAlert != NULL);

    if (pAlert != NULL)
        return pAlert->iID;
    else
        return -1;

}

/**
 * Get the Location string of an Alert
 * @param record - The Record number in the table
 * @return The location string of the Alert, NULL if the record does not exist
 */
char * RTAlerts_Get_LocationStr(int record)
{
    RTAlert *pAlert = RTAlerts_Get(record);
    assert(pAlert != NULL);

    if (pAlert != NULL)
        return pAlert->sLocationStr;
    else
        return NULL;

}

/**
 * Get the speed of an Alert
 * @param record - The Record number in the table
 * @return The speed of the Alert, -1 if the record does not exist
 */
unsigned int RTAlerts_Get_Speed(int record)
{
    RTAlert *pAlert = RTAlerts_Get(record);
    assert(pAlert != NULL);

    if (pAlert != NULL)
        return pAlert->iSpeed;
    else
        return -1;
}

/**
 * Get the an alert Alert
 * @param record - The Record number in the table
 * @return a new pointer to the alert
 */
RoadMapAlert * RTAlerts_Get_Alert(int record)
{
    RoadMapAlert *pRoadMapAlert;
    RTAlert *pAlert;

    pAlert = RTAlerts_Get(record);
    assert(pAlert != NULL);

    pRoadMapAlert = (RoadMapAlert *) malloc(sizeof(pRoadMapAlert));

    pRoadMapAlert->pos.latitude = pAlert->iLatitude;
    pRoadMapAlert->pos.longitude = pAlert->iLongitude;
    pRoadMapAlert->steering = pAlert->iAzymuth;
    pRoadMapAlert->speed = RTAlerts_Get_Speed(record);
    pRoadMapAlert->id = pAlert->iID;
    pRoadMapAlert->category = pAlert->iType;

    return pRoadMapAlert;

}

/**
 * Get the icon to display of an alert
 * @param alertId - The ID of the alert
 * @return the icon name, NULL if the alert is not found or type unkown
 */
const char * RTAlerts_Get_Icon(int alertId)
{

    RTAlert *pAlert;

    pAlert = RTAlerts_Get_By_ID(alertId);
    if (pAlert == NULL)
        return NULL;

    switch (pAlert->iType)
    {

   case RT_ALERT_TYPE_POLICE:
        if (pAlert->iNumComments == 0)
            return "real_time_police_icon";
        else
            return "real_time_police_with_comments";
        break;

    case RT_ALERT_TYPE_ACCIDENT:
        if (pAlert->iNumComments == 0)
            return "real_time_accident_icon";
        else
            return "real_time_accident_with_comments";
        break;

    case RT_ALERT_TYPE_CHIT_CHAT:
        if (pAlert->iNumComments == 0)
            return "real_time_chit_chat_icon";
        else
            return "real_time_chit_chat_with_comments";
        break;

    case RT_ALERT_TYPE_TRAFFIC_JAM:
        if (pAlert->iNumComments == 0)
            return "real_time_traffic_jam_icon";
        else
            return "real_time_trafficjam_with_comments";
        break;

    case RT_ALERT_TYPE_TRAFFIC_INFO:
        if (pAlert->iNumComments == 0)
            return "real_time_traffic_info_icon";
        else
            return "real_time_traffic_info_icon";
        break;

    case RT_ALERT_TYPE_HAZARD:
        if (pAlert->iNumComments == 0)
            return "real_time_hazard_icon";
        else
            return "real_time_hazard_icon_with_comments";
        break;

    case RT_ALERT_TYPE_OTHER:
        if (pAlert->iNumComments == 0)
            return "real_time_other_icon";
        else
            return "real_time_other_icon_with_comments";
        break;

    case RT_ALERT_TYPE_CONSTRUCTION:
        if (pAlert->iNumComments == 0)
            return "road_construction";
        else
            return "road_construction_with_comments";
        break;

    default:
        return RTAlerts_Get_Map_Icon(alertId);
    }
}
/**
 * Get the icon to display of an alert to diaplay on map
 * @param alertId - The ID of the alert
 * @return the icon name, NULL if the alert is not found or type unkown
 */
const char * RTAlerts_Get_Map_Icon(int alertId)
{

    RTAlert *pAlert;

    pAlert = RTAlerts_Get_By_ID(alertId);
    if (pAlert == NULL)
        return NULL;

    switch (pAlert->iType)
    {
    case RT_ALERT_TYPE_POLICE:
        return "police_on_map";
        break;

    case RT_ALERT_TYPE_ACCIDENT:
        return "accident_on_map";
        break;

    case RT_ALERT_TYPE_CHIT_CHAT:
        return "chit_chat_on_map";
        break;

    case RT_ALERT_TYPE_TRAFFIC_JAM:
            return "loads_on_map";
        break;

    case RT_ALERT_TYPE_HAZARD:
            return "hazard_on_map";
        break;

    case RT_ALERT_TYPE_OTHER:
            return "other_on_map";
        break;

    case RT_ALERT_TYPE_CONSTRUCTION:
            return "road_construction_on_map";
        break;

    case RT_ALERT_TYPE_TRAFFIC_INFO:
            return NULL;
        break;

    default:
        return NULL;
    }
}

/**
 * Get the icon to display of an in case of approaching the alert
 * @param alertId - The ID of the alert
 * @return the icon name, NULL if the alert is not found or type unkown
 */
const char * RTAlerts_Get_Alert_Icon(int alertId)
{
    RTAlert *pAlert;

    pAlert = RTAlerts_Get_By_ID(alertId);
    if (pAlert == NULL)
        return NULL;

    switch (pAlert->iType)
    {
    case RT_ALERT_TYPE_POLICE:
        return "real_time_police_alert";
    case RT_ALERT_TYPE_ACCIDENT:
        return "real_time_accident_alert";
    default:
        return NULL;
    }
}

/**
 * Get the icon to warn of an in case of approaching the alert
 * @param alertId - The ID of the alert
 * @return the icon name, NULL if the alert is not found or type unkown
 */
const char * RTAlerts_Get_Warn_Icon(int alertId)
{
    RTAlert *pAlert;

    pAlert = RTAlerts_Get_By_ID(alertId);
    if (pAlert == NULL)
        return NULL;

    switch (pAlert->iType)
    {
    case RT_ALERT_TYPE_POLICE:
        return "real_time_police_alert";
    case RT_ALERT_TYPE_ACCIDENT:
        return "real_time_accident_alert";
    default:
        return NULL;
    }
}

/**
 * Get the name of the alert
 * @param alertId - The ID of the alert
 * @return the name, NULL if the alert is not found or type unkown
 */
const char * RTAlerts_Get_String(int alertId)
{
    RTAlert *pAlert;

    pAlert = RTAlerts_Get_By_ID(alertId);
    if (pAlert == NULL)
        return NULL;

    switch (pAlert->iType)
    {
    case RT_ALERT_TYPE_POLICE:
        return "Police";
    case RT_ALERT_TYPE_ACCIDENT:
        return "Accident";
    default:
        return NULL;
    }
}

/**
 * Get the distance to alert in case of approaching the alert
 * @param record - record number in the table
 * @return the distance in meters to alerts before the alert, default 0
 */
int RTAlerts_Get_Distance(int record)
{
    RTAlert *pAlert;
    pAlert = RTAlerts_Get(record);
    if (pAlert == NULL)
        return 0;

    switch (pAlert->iType)
    {
    case RT_ALERT_TYPE_POLICE:
        return 600;
    case RT_ALERT_TYPE_ACCIDENT:
        return 2000;
    default:
        return 0;
    }
}

/**
 * Get the sound file name to play in case of approaching the alert
 * @param alertId - The ID of the alert
 * @return the sound name, empty sound list if the alert is not found
 */
RoadMapSoundList RTAlerts_Get_Sound(int alertId)
{

    RoadMapSoundList sound_list;
    RTAlert *pAlert;

    sound_list = roadmap_sound_list_create(0);

    pAlert = RTAlerts_Get_By_ID(alertId);
    if (pAlert == NULL)
        return sound_list;

    switch (pAlert->iType)
    {
    case RT_ALERT_TYPE_POLICE:
        roadmap_sound_list_add(sound_list, "Police");
        break;
    case RT_ALERT_TYPE_ACCIDENT:
        roadmap_sound_list_add(sound_list, "Accident");
        break;

    default:
        break;
    }

    return sound_list;
}

/**
 * Check wheter an alert is alertable
 * @param record - The record number of the alert
 * @return TRUE if the alert is alertable, FALSE otherwise
 */
int RTAlerts_Is_Alertable(int record)
{
    RTAlert *pAlert;
    pAlert = RTAlerts_Get(record);
    if (pAlert == NULL)
        return FALSE;

    switch (pAlert->iType)
    {
    case RT_ALERT_TYPE_POLICE:
        return TRUE;
    case RT_ALERT_TYPE_ACCIDENT:
        return FALSE;
    case RT_ALERT_TYPE_CHIT_CHAT:
        return FALSE;
    case RT_ALERT_TYPE_TRAFFIC_JAM:
        return FALSE;
    case RT_ALERT_TYPE_TRAFFIC_INFO:
        return FALSE;
    case RT_ALERT_TYPE_HAZARD:
        return FALSE;
    case RT_ALERT_TYPE_OTHER:
        return FALSE;
    case RT_ALERT_TYPE_CONSTRUCTION:
        return FALSE;
    default:
        return FALSE;
    }
}

/**
 * Check wheter an alert is reroutable (ask the user to calculate for a new route)
 * @param pAlert - pointer to the alert
 * @return TRUE if the alert is reroutable, FALSE otherwise
 */
int RTAlerts_Is_Reroutable(RTAlert *pAlert)
{

    if (pAlert == NULL)
        return FALSE;

    switch (pAlert->iType)
    {
    case RT_ALERT_TYPE_POLICE:
        return FALSE;
    case RT_ALERT_TYPE_ACCIDENT:
        return TRUE;
    case RT_ALERT_TYPE_CHIT_CHAT:
        return FALSE;
    case RT_ALERT_TYPE_TRAFFIC_JAM:
        return TRUE;
    case RT_ALERT_TYPE_TRAFFIC_INFO:
        return FALSE;
    case RT_ALERT_TYPE_HAZARD:
        return FALSE;
    case RT_ALERT_TYPE_OTHER:
        return FALSE;
    case RT_ALERT_TYPE_CONSTRUCTION:
        return FALSE;

    default:
        return FALSE;
    }
}

/**
 * Display a popup warning that an alert is on the rout
 * @param pAlert - pointer to the alert
 * @return None
 */
static void OnAlertReRoute(RTAlert *pAlert)
{
    RoadMapSoundList sound_list;

    sound_list = roadmap_sound_list_create(0);
    roadmap_sound_list_add(sound_list, "alert_1");
    roadmap_sound_play_list(sound_list);
    RTAlerts_Popup_By_Id(pAlert->iID);
    if (!ssd_dialog_activate("rt_alert_on_my_route", NULL))
    {
        create_ssd_dialog(pAlert);
        ssd_dialog_activate("rt_alert_on_my_route", NULL);
    }
}

/**
 * Action to be taken after an alert was added
 * @param pAlert - pointer to the alert
 * @return None
 */
static void OnAlertAdd(RTAlert *pAlert)
{

    if ((gAlertsTable.iCount == 1) && (gTimerActive == FALSE))
    {
        gIdleCount = 0;
        roadmap_main_set_periodic(1000, RTAlerts_Timer);
        gTimerActive = TRUE;
    }

    if (RTAlerts_Is_Reroutable(pAlert) && (!pAlert->bAlertByMe))
    {
        if (navigate_track_enabled())
        {
            if (navigate_is_line_on_route(pAlert->iSquare, pAlert->iLineId, pAlert->iNode1,
                    pAlert->iNode2))
            {
                int azymuth, delta;
                PluginLine line;
                RoadMapPosition alert_pos;
                RoadMapGpsPosition gps_map_pos;
                int steering;
                int distance;

                // check that the alert is within alert distance
                alert_pos.latitude = pAlert->iLatitude;
                alert_pos.longitude = pAlert->iLongitude;

                if (roadmap_navigate_get_current(&gps_map_pos, &line, &steering)
                        != -1)
                {
                    RoadMapPosition gps_pos;
                    gps_pos.latitude = gps_map_pos.altitude;
                    gps_pos.longitude = gps_map_pos.longitude;

                    //
                    distance = roadmap_math_distance(&gps_pos, &alert_pos);
                    if (distance > REROUTE_MIN_DISTANCE) {
                        return;
                    }

                    //check that we didnt pass the new alert on the route.
                    azymuth = roadmap_math_azymuth(&gps_pos, &alert_pos);
                    delta = azymuth_delta(azymuth, steering);
                    if (delta < 90 && delta >(-90))
                    {
                        OnAlertReRoute(pAlert);
                    }
                }
                else
                {
                    OnAlertReRoute(pAlert);
                }
            }
        }
    }
}

/**
 * Action to be taken after an alert was removed
 * @param None
 * @return None
 */
static void OnAlertRemove(void)
{
    if ((gAlertsTable.iCount == 0) && gTimerActive)
    {
        roadmap_main_remove_periodic(RTAlerts_Timer) ;
        gTimerActive = FALSE;
    }
}

/**
 * RealTime alerts timer. Checks whether we are standing for few seconds and display popup
 * @param None
 * @return None
 */
static void RTAlerts_Timer(void)
{

    RoadMapGpsPosition pos;
    const char *focus = roadmap_trip_get_focus_name();
    BOOL gps_active;
    int gps_state;


    gps_state = roadmap_gps_reception_state();
    gps_active = (gps_state != GPS_RECEPTION_NA) && (gps_state != GPS_RECEPTION_NONE);


    roadmap_navigate_get_current(&pos, NULL, NULL);

    if (gIdleScrolling && pos.speed > 5)
    {
        RTAlerts_Stop_Scrolling();
        return;
    }

    if (ssd_dialog_is_currently_active()){
        if (gIdleScrolling)
            RTAlerts_Stop_Scrolling();
        return;
    }

    if ((pos.speed < 2) && (gps_active))
        gIdleCount++;
    else
    {
        gIdleCount = 0;
        if (gIdleScrolling)
        {
            RTAlerts_Stop_Scrolling();
        }
    }

    if (focus && !strcmp(focus, "Hold"))
    {
        return;
    }

    //focus && !strcmp (focus, "GPS") &&
    if ((gIdleCount == 30) && !gScrolling)
    {
        roadmap_math_get_context(&gSavedPosition, &gSavedZoom);
        RTAlerts_Scroll_All();
        gIdleScrolling = TRUE;
        gScrolling = TRUE;
    }

    if (gIdleCount > 30)
        gIdleCount = 0;

}

/**
 * Zoomin map to the alert.
 * @param AlertPosition - The position of the alert
 * @param iCenterAround - Whether to center around the alert or the GPS position
 * @return None
 */
static void RTAlerts_zoom(RoadMapPosition AlertPosition, int iCenterAround)
{
    int distance;
    RoadMapGpsPosition CurrentPosition;
    RoadMapPosition pos;
    PluginLine line;
    int Direction;
    int scale;

    if (iCenterAround == CENTER_ON_ALERT)
    {
        roadmap_trip_set_point("Hold", &AlertPosition);
        roadmap_screen_update_center(&AlertPosition);
        scale = 1000;
        roadmap_math_set_scale(scale, roadmap_screen_height() / 3);
        roadmap_layer_adjust();
        gCentered = TRUE;
        return;
    }

    gCentered = FALSE;
    if (roadmap_navigate_get_current(&CurrentPosition, &line, &Direction) != -1)
    {

        pos.latitude = CurrentPosition.latitude;
        pos.longitude = CurrentPosition.longitude;
        distance = roadmap_math_distance(&pos, &AlertPosition);

        roadmap_screen_update_center(&pos);

        if (distance < 1000)
            scale = 1000;
        else if (distance < 10000)
            scale = 10000;
        else if (distance < 30000)
            scale = 30000;
        else
            scale = 130000;

        roadmap_math_set_scale(scale, roadmap_screen_height() / 3);

        roadmap_layer_adjust();
    }
    else
        roadmap_screen_update_center(&AlertPosition);

}

/**
 * Find the name of the street and city near an alert
 * @param AlertPosition - the position of the alert
 * @param city_name [OUT] - the name of the city, NULL if not found
 * @param street_name [OUT] - the name of the street, NULL if not found
 * @param line_id [OUT] - the line_id near the alert, -1 if not found
 * @param direction - the direction of the alert
 * @return None
 */
void RTAlerts_Get_City_Street(RoadMapPosition AlertPosition,
        const char **city_name, const char **street_name, int *square, int *line_id,
        int direction)
{

    RoadMapNeighbour neighbours[16];
    int count = 0;
    int layers[128];
    int layers_count;
    RoadMapStreetProperties properties;
    RoadMapPosition context_save_pos;
    int context_save_zoom;
    int line_directions;
    const char *street_name2;

    roadmap_math_get_context(&context_save_pos, &context_save_zoom);
    roadmap_math_set_context((RoadMapPosition *)&AlertPosition, 20);
    layers_count = roadmap_layer_all_roads(layers, 128);
    count = roadmap_street_get_closest(&AlertPosition, 0, layers, layers_count,
            &neighbours[0], 1);

    if (count != 0)
    {
          roadmap_square_set_current (neighbours[0].line.square);

        if (roadmap_locator_activate(neighbours[0].line.fips) < 0)
        {
            *city_name = NULL;
            *street_name = NULL;
            *line_id = -1;
            *square = -1;
            roadmap_math_set_context(&context_save_pos, context_save_zoom);
            return;
        }
        roadmap_street_get_properties(neighbours[0].line.line_id, &properties);

        line_directions =roadmap_line_route_get_direction(
                neighbours[0].line.line_id, ROUTE_CAR_ALLOWED);
        *street_name = roadmap_street_get_street_name(&properties);

        if ((line_directions == ROUTE_DIRECTION_ANY) || (direction
                == RT_ALERT_MY_DIRECTION))
        {

            *city_name = roadmap_street_get_street_city(&properties,
            ROADMAP_STREET_LEFT_SIDE);
            *line_id = neighbours[0].line.line_id;
            *square = neighbours[0].line.square;
            roadmap_math_set_context(&context_save_pos, context_save_zoom);
        }
        else
        {
            int i;

            // The alert is on the opposite side but the current line is one-way
            roadmap_square_set_current (neighbours[0].line.square);
            street_name2 = strdup(*street_name);
            *city_name = roadmap_street_get_street_city(&properties,
            ROADMAP_STREET_LEFT_SIDE);
            *line_id = neighbours[0].line.line_id;
               *square = neighbours[0].line.square;

            layers[0] = neighbours[0].line.cfcc;
            count = roadmap_street_get_closest(&AlertPosition, 0, layers, 1,
                    &neighbours[0], 10);
            for (i=0; i<count; i++)
            {
                 roadmap_square_set_current (neighbours[i].line.square);
                roadmap_street_get_properties(neighbours[i].line.line_id,
                        &properties);
                *street_name = roadmap_street_get_street_name(&properties);
                if (strcmp(street_name2, *street_name) && !strncmp(
                        street_name2, *street_name, 2))
                {
                    *city_name = roadmap_street_get_street_city(&properties,
                    ROADMAP_STREET_LEFT_SIDE);
                    *square = neighbours[i].line.square;
                    *line_id = neighbours[i].line.line_id;
                    roadmap_math_set_context(&context_save_pos, context_save_zoom);
                    return;
                }
            }
            *street_name = street_name2;
        }

    }
    else
    {
        *city_name = NULL;
        *street_name = NULL;
        *line_id = -1;
        *square = -1;
        roadmap_math_set_context(&context_save_pos, context_save_zoom);
        return;
    }
    roadmap_math_set_context(&context_save_pos, context_save_zoom);
}

/**
 * Compare method for qsort. sorts according to distance to alert
 * @param r1 - alert element
 * @param r2 - alert element
 * @return 0 if they are equal, -1 if distance to r1 is less than r2, 1 if distance to r2 is more than r1
 */
static int compare_proximity(const void *r1, const void *r2)
{

    RTAlert * const *elem1 = (RTAlert * const *)r1;
    RTAlert * const *elem2 = (RTAlert * const *)r2;

        if ((*elem1)->iDistance < (*elem2)->iDistance)
            return -1;

        else if ((*elem1)->iDistance > (*elem2)->iDistance)
            return 1;

        else
         return 0;
}

/**
 * Compare method for qsort. sorts according to distance to time
 * @param r1 - alert element
 * @param r2 - alert element
 * @return 0 if they are equal, -1 if distance to r1 is less than r2, 1 if distance to r2 is more than r1
 */
static int compare_recency(const void *r1, const void *r2)
{

    RTAlert * const *elem1 = (RTAlert * const *)r1;
    RTAlert * const *elem2 = (RTAlert * const *)r2;

    if ((*elem1)->i64ReportTime > (*elem2)->i64ReportTime)
            return -1;
     else if ((*elem1)->i64ReportTime < (*elem2)->i64ReportTime)
            return 1;
     else
         return 0;
}

/**
 * Sort list of alerts according to distance from my current location
 * @param None
  * @return None
 */

void RTAlerts_Sort_List(alert_sort_method sort_method)
{
    RoadMapGpsPosition CurrentPosition;
    RoadMapPosition position, current_pos;
    const RoadMapPosition *GpsPosition;
    PluginLine line;
    int Direction, distance;
    int i;
    gState = STATE_OLD;
    for (i=0; i < gAlertsTable.iCount; i++)
    {
        position.longitude = gAlertsTable.alert[i]->iLongitude;
        position.latitude = gAlertsTable.alert[i]->iLatitude;

        if (roadmap_navigate_get_current(&CurrentPosition, &line, &Direction)
                != -1)
        {

            current_pos.latitude = CurrentPosition.latitude;
            current_pos.longitude = CurrentPosition.longitude;

            distance = roadmap_math_distance(&current_pos, &position);
            gAlertsTable.alert[i]->iDistance = distance;
        }
        else
        {
            GpsPosition = roadmap_trip_get_position("GPS");
            if (GpsPosition != NULL)
            {
                current_pos.latitude = GpsPosition->latitude;
                current_pos.longitude = GpsPosition->longitude;
                distance = roadmap_math_distance(&current_pos, &position);
                gAlertsTable.alert[i]->iDistance = distance;
            }
            else
            {
                distance = 0;
            }
        }
    }

    if (sort_method == sort_proximity)
        qsort((void *) &gAlertsTable.alert[0], gAlertsTable.iCount, sizeof(void *), compare_proximity);
    else
        qsort((void *) &gAlertsTable.alert[0], gAlertsTable.iCount, sizeof(void *), compare_recency);

}
static void set_left_softkey(){
    static Softkey s;
    strcpy(s.text, "Comment");
    s.callback = real_time_post_alert_comment;
    roadmap_softkeys_set_left_soft_key("Comment", &s);

}

static void set_right_softkey(){
    static Softkey s;
    strcpy(s.text, "Hide");
    s.callback = RTAlerts_Cancel_Scrolling;
    roadmap_softkeys_set_right_soft_key("Hide", &s);

}

/**
 * Display the pop up of an alert
 * @param alertId - the ID of the alert to display
 * @param iCenterAround - whether to center around the alert or the GPS position
 * @return None
 */
static void RTAlerts_popup_alert(int alertId, int iCenterAround)
{
    RTAlert *pAlert;
    RoadMapPosition position, current_pos;
    char AlertStr[700];
    int timeDiff;
    time_t now;
    const RoadMapPosition *GpsPosition;
    int distance;
    RoadMapGpsPosition CurrentPosition;
    PluginLine line;
    int Direction;
    int distance_far;
    char str[100];
    char unit_str[20];
    const char *focus;
    AlertStr[0] = 0;

    focus = roadmap_trip_get_focus_name();

    if (gAlertsTable.iCount == 0)
        return;

    pAlert = RTAlerts_Get_By_ID(alertId);
    if (pAlert == NULL)
        return;

    position.longitude = pAlert->iLongitude;
    position.latitude = pAlert->iLatitude;
    AlertStr[0] =0;

     if (iCenterAround != CENTER_NONE)
        RTAlerts_zoom(position, iCenterAround);


    // Display alert String
    if (pAlert->iType == RT_ALERT_TYPE_ACCIDENT)
        snprintf(AlertStr + strlen(AlertStr), sizeof(AlertStr)
                - strlen(AlertStr), "<h1>%s", roadmap_lang_get("Accident"));
    else if (pAlert->iType == RT_ALERT_TYPE_POLICE)
        snprintf(AlertStr + strlen(AlertStr), sizeof(AlertStr)
                - strlen(AlertStr), "<h1>%s", roadmap_lang_get("Police"));
    else if (pAlert->iType == RT_ALERT_TYPE_TRAFFIC_JAM)
        snprintf(AlertStr + strlen(AlertStr), sizeof(AlertStr)
                - strlen(AlertStr), "<h1>%s", roadmap_lang_get("Traffic jam"));
    else if (pAlert->iType == RT_ALERT_TYPE_TRAFFIC_INFO)
        snprintf(AlertStr + strlen(AlertStr), sizeof(AlertStr)
                - strlen(AlertStr), "<h1>%s", roadmap_lang_get("Road info"));
    else if (pAlert->iType == RT_ALERT_TYPE_HAZARD)
        snprintf(AlertStr + strlen(AlertStr), sizeof(AlertStr)
                - strlen(AlertStr), "<h1>%s", roadmap_lang_get("Hazard"));
    else if (pAlert->iType == RT_ALERT_TYPE_OTHER)
        snprintf(AlertStr + strlen(AlertStr), sizeof(AlertStr)
                - strlen(AlertStr), "<h1>%s", roadmap_lang_get("Other"));
    else if (pAlert->iType == RT_ALERT_TYPE_CONSTRUCTION)
        snprintf(AlertStr + strlen(AlertStr), sizeof(AlertStr)
                - strlen(AlertStr), "<h1>%s", roadmap_lang_get("Road construction"));
    else
        snprintf(AlertStr + strlen(AlertStr), sizeof(AlertStr)
                - strlen(AlertStr), "<h1>%s",
                roadmap_lang_get("Chit Chat"));

    // Display if there are comment
    if (pAlert->iNumComments == 0)
        snprintf(AlertStr + strlen(AlertStr), sizeof(AlertStr)
                - strlen(AlertStr), "%s", NEW_LINE);
    else
        snprintf(AlertStr + strlen(AlertStr), sizeof(AlertStr)
                - strlen(AlertStr), " (%d %s)%s", pAlert->iNumComments,
                roadmap_lang_get("Comments"), NEW_LINE);

    // Display when the alert street name and city
    snprintf(AlertStr + strlen(AlertStr), sizeof(AlertStr) - strlen(AlertStr),
            "%s%s", pAlert->sLocationStr, NEW_LINE);

    // check the distance to the alert
    if (roadmap_navigate_get_current(&CurrentPosition, &line, &Direction) != -1)
    {

        current_pos.latitude = CurrentPosition.latitude;
        current_pos.longitude = CurrentPosition.longitude;
        distance = roadmap_math_distance(&current_pos, &position);

        distance_far = roadmap_math_to_trip_distance(distance);
        if (distance_far > 0)
        {
            int tenths = roadmap_math_to_trip_distance_tenths(distance);
            snprintf(str, sizeof(str), "%d.%d", distance_far, tenths % 10);
            snprintf(unit_str, sizeof(unit_str), "%s", roadmap_math_trip_unit());
        }
        else
        {
            snprintf(str, sizeof(str), "%d", distance);
            snprintf(unit_str, sizeof(unit_str), "%s",
                    roadmap_math_distance_unit());
        }
        snprintf(AlertStr + strlen(AlertStr), sizeof(AlertStr)
                - strlen(AlertStr), roadmap_lang_get("%s %s Away"), str,
                roadmap_lang_get(unit_str), NEW_LINE);
        snprintf(AlertStr + strlen(AlertStr), sizeof(AlertStr)
                - strlen(AlertStr), "%s", NEW_LINE);

    }
    else
    {
        GpsPosition = roadmap_trip_get_position("GPS");
        if (GpsPosition != NULL)
        {
            current_pos.latitude = GpsPosition->latitude;
            current_pos.longitude = GpsPosition->longitude;
            distance = roadmap_math_distance(&current_pos, &position);
            distance_far = roadmap_math_to_trip_distance(distance);

            if (distance_far > 0)
            {
                int tenths = roadmap_math_to_trip_distance_tenths(distance);
                snprintf(str, sizeof(str), "%d.%d", distance_far, tenths % 10);
                snprintf(unit_str, sizeof(unit_str), "%s",
                        roadmap_math_trip_unit());
            }
            else
            {
                snprintf(str, sizeof(str), "%d", distance);
                snprintf(unit_str, sizeof(unit_str), "%s",
                        roadmap_math_distance_unit());
            }
            snprintf(AlertStr + strlen(AlertStr), sizeof(AlertStr)
                    - strlen(AlertStr), roadmap_lang_get("%s %s Away"), str,
                    roadmap_lang_get(unit_str), NEW_LINE);
            snprintf(AlertStr + strlen(AlertStr), sizeof(AlertStr)
                    - strlen(AlertStr), "%s", NEW_LINE);

        }
    }

    // Display when the alert was generated
    now = time(NULL);
    timeDiff = (int)difftime(now, (time_t)pAlert->i64ReportTime);
    if (timeDiff <0)
        timeDiff = 0;

    snprintf(AlertStr + strlen(AlertStr), sizeof(AlertStr) - strlen(AlertStr),
            "%s", "<h3>");

     if (pAlert->iType == RT_ALERT_TYPE_TRAFFIC_INFO)
            snprintf(AlertStr + strlen(AlertStr), sizeof(AlertStr)
                   - strlen(AlertStr),
                    roadmap_lang_get("Updated "));
    else
            snprintf(AlertStr + strlen(AlertStr), sizeof(AlertStr)
                   - strlen(AlertStr),
                    roadmap_lang_get("Reported "));


    if (timeDiff < 60)
        snprintf(AlertStr + strlen(AlertStr), sizeof(AlertStr)
                - strlen(AlertStr),
                roadmap_lang_get("%d seconds ago"), timeDiff);
    else if ((timeDiff > 60) && (timeDiff < 3600))
        snprintf(AlertStr + strlen(AlertStr), sizeof(AlertStr)
                - strlen(AlertStr),
                roadmap_lang_get("%d minutes ago"), timeDiff/60);
    else
        snprintf(AlertStr + strlen(AlertStr), sizeof(AlertStr)
                - strlen(AlertStr),
                roadmap_lang_get("%2.1f hours ago"), (float)timeDiff
                        /3600);

    // Display who reported the alert
    if (pAlert->sReportedBy[0] != 0){
        snprintf(AlertStr + strlen(AlertStr), sizeof(AlertStr) - strlen(AlertStr),
              " %s %s", roadmap_lang_get("by"), pAlert->sReportedBy);
    }

    if (pAlert->sDescription[0] != 0){
    //Display the alert description
    snprintf(AlertStr + strlen(AlertStr), sizeof(AlertStr) - strlen(AlertStr),
            "%s<h3>%s", NEW_LINE, pAlert->sDescription);
    }

    gCurrentAlertId = alertId;
    gCurrentComment = pAlert->Comment;
    //roadmap_display_text("RTAlerts", roadmap_lang_get("Real Time Alert"));
    roadmap_display_pop_up("Real Time Alert Pop Up",
            RTAlerts_Get_Icon(pAlert->iID) , &position, AlertStr);
    set_left_softkey();
    set_right_softkey();
    roadmap_screen_redraw();
}

/**
 * Display the pop up of an alert according to the iterator
 * @param None
 * @return None
 */
static void RTAlerts_Popup(void)
{
    RTAlert *pAlert;

    if (gIterator >= gAlertsTable.iCount)
    {
        gIterator = -1;
    }

    gIterator++;
    pAlert = RTAlerts_Get(gIterator);
    if (pAlert == NULL)
        return;
    RTAlerts_popup_alert(pAlert->iID, CENTER_ON_ME);

}

/**
 * Display the pop up of a specific alert
 * @param IId - the ID of the alert to popup
 * @return None
 */
void RTAlerts_Popup_By_Id(int iID)
{
    if (!gScrolling)
    {
        roadmap_screen_hold();
        gScrolling = TRUE;
    }
    RTAlerts_popup_alert(iID, CENTER_ON_ALERT);
}

/**
 * Display the pop up of a specific alert without centering
 * @param IId - the ID of the alert to popup
 * @return None
 */
void RTAlerts_Popup_By_Id_No_Center(int iID)
{
    if (!gScrolling)
    {
        roadmap_screen_hold();
        gScrolling = TRUE;
        roadmap_math_get_context(&gSavedPosition, &gSavedZoom);
    }
    RTAlerts_popup_alert(iID, CENTER_NONE);
}
/**
 * Scrolls through all the alerts by popups
 * @param None
 * @return None
 */
void RTAlerts_Scroll_All()
{

    if (gScrolling == FALSE)
    {
        RTAlerts_Sort_List(sort_proximity);
        roadmap_screen_hold();
        gIterator = -1;
        gCurrentAlertId = -1;
        RTAlerts_Popup();
        roadmap_main_set_periodic(6000, RTAlerts_Popup);
        gScrolling = TRUE;
    }
}

/**
 * Remove the pop up of the alert and stop scrolling
 * @param None
 * @return None
 */
void RTAlerts_Stop_Scrolling()
{

    if (gScrolling)
    {
          if (gSavedZoom == -1){
           roadmap_trip_set_focus("GPS");
           roadmap_math_zoom_reset();
          }else{
              roadmap_math_set_context(&gSavedPosition, gSavedZoom);
              roadmap_trip_set_focus("GPS");
              gSavedPosition.latitude = -1;
              gSavedPosition.longitude = -1;
              gSavedZoom = -1;
          }

        RTAlerts_hide_softkeys();
        roadmap_main_remove_periodic(RTAlerts_Popup) ;
        roadmap_screen_unfreeze() ;
        roadmap_display_hide("Real Time Alert Pop Up");
//        roadmap_display_hide("RTAlerts");
        roadmap_layer_adjust();
        roadmap_screen_redraw();
    }

    gIterator = 0;
    gScrolling = FALSE;
    gIdleScrolling = FALSE;
}

/**
 * Remove the alert and stop the Alerts timer.
 * @param None
 * @return None
 */
void RTAlerts_Cancel_Scrolling()
{
    RTAlerts_Stop_Scrolling();
    roadmap_main_remove_periodic(RTAlerts_Timer);
    gTimerActive = FALSE;
}

/**
 * Zoomin to the alert
 * @param None
 * @return False, if we are already zoomed on the alert, TRUE otherwise
 */
BOOL RTAlerts_Zoomin_To_Aler()
{
    RTAlert *pAlert;

    if (gCentered)
        return FALSE;

    if (gIterator >= gAlertsTable.iCount)
        gIterator = 0;
    else if (gIterator < 0)
        gIterator = 0;

    if (gTimerActive)
        roadmap_main_remove_periodic(RTAlerts_Timer);

    pAlert = RTAlerts_Get(gIterator);
    RTAlerts_popup_alert(pAlert->iID, CENTER_ON_ALERT);
    return TRUE;
}

/**
 * Popup the next alert
 * @param None
 * @return None
 */
void RTAlerts_Scroll_Next()
{
    RTAlert *pAlert;
    gState = STATE_OLD;

    if (RTAlerts_Is_Empty())
        return;

    if (gScrolling == FALSE)
    {
        RTAlerts_Sort_List(sort_proximity);
        gIterator = -1;
    }

    if (gIterator < gAlertsTable.iCount-1)
        gIterator ++;
    else
        gIterator = 0;

    pAlert = RTAlerts_Get(gIterator);
    if (pAlert == NULL)
        return;
    gScrolling = TRUE;
    roadmap_screen_hold();
    RTAlerts_popup_alert(pAlert->iID, CENTER_ON_ME);

}

/**
 * Popup the previous alert
 * @param None
 * @return None
 */
void RTAlerts_Scroll_Prev()
{
    RTAlert *pAlert;
    gState = STATE_OLD;
    if (RTAlerts_Is_Empty())
        return;

    if (gScrolling == FALSE)
    {
        RTAlerts_Sort_List(sort_proximity);
        gIterator = 0;
    }

    if (gIterator >0)
        gIterator --;
    else
        gIterator=gAlertsTable.iCount-1;

    pAlert = RTAlerts_Get(gIterator);
    if (pAlert == NULL)
        return;
    gScrolling = TRUE;
    roadmap_screen_hold();
    RTAlerts_popup_alert(pAlert->iID, CENTER_ON_ME);
}

/**
 * Returns the currently displayed alert ID
 * @param None
 * @return the id of the alert currently being displayed
 */
int RTAlerts_Get_Current_Alert_Id()
{
    return gCurrentAlertId;
}

typedef struct
{
    int iType;
    int iDirection;
    const char * szDescription;
} RTAlertContext;

/**
 * Report Alert keyboard callback
 * @param type - the button that was pressed, new_value - text, context - hold a pointer to the alert context
 * @return int 1
 */
static int keyboard_callback(int type, const char *new_value, void *context)
{
    BOOL success;
    const char *desc;
    RTAlertContext *AlertContext = (RTAlertContext *)context;

    if (type != SSD_KEYBOARD_OK)
        return 1;

    if ((AlertContext->iType == RT_ALERT_TYPE_CHIT_CHAT) &&  (new_value[0] == 0)){
        free(AlertContext);
        ssd_keyboard_hide();
        roadmap_trip_restore_focus();
        return 1;
    }

    if (new_value[0] == 0)
        desc = AlertContext->szDescription;
    else
        desc = new_value;

    success = Realtime_Report_Alert(AlertContext->iType, desc,
            AlertContext->iDirection);
    if (success){
        ssd_keyboard_hide_all();
    }
    else
        ssd_keyboard_hide();


    free(AlertContext);
    return 1;

}

/**
 * Show add additi -alert Type, szDescription - alert description, iDirection - direction of the alert
 * @return void
 */
void report_alert(int iAlertType, const char * szDescription, int iDirection)
{

    RTAlertContext *AlertConext = calloc(1, sizeof(RTAlertContext));
    AlertConext->iType = iAlertType;
    AlertConext->szDescription = szDescription;
    AlertConext->iDirection = iDirection;
#ifdef __SYMBIAN32__
    ShowEditbox(roadmap_lang_get("Additional Details"), "",
            keyboard_callback, (void *)AlertConext, EEditBoxStandard );
#else
    ssd_keyboard_show (SSD_KEYBOARD_LETTERS,
            roadmap_lang_get("Additional Details"), "", NULL, keyboard_callback, (void *)AlertConext);
#endif
}

static char const *DirectionQuickMenu[] = {

   "mydirection",
   RoadMapFactorySeparator,
   "oppositedirection",
   NULL,
};


/**
 * Starts the Direction menu
 * @param actions - list of actions
 * @return void
 */
void alerts_direction_menu(const char *name, const RoadMapAction  *actions){

    const RoadMapGpsPosition   *TripLocation;

    if (!roadmap_gps_have_reception()) {
        roadmap_messagebox("Error", "No GPS reception!");
        return;
    }

    TripLocation = roadmap_trip_get_gps_position("AlertSelection");
    if ((TripLocation == NULL) || (TripLocation->latitude <= 0) || (TripLocation->longitude <= 0)) {
        roadmap_messagebox ("Error", "Can't find current street.");
        return;
    }


    ssd_list_menu_activate (name, NULL, DirectionQuickMenu, NULL, actions,
                SSD_DIALOG_FLOAT|SSD_DIALOG_TRANSPARENT|SSD_DIALOG_VERTICAL|SSD_ALIGN_VCENTER|SSD_BG_IMAGE);

}

/**
 * Report an alert for police
 * @param None
 * @return void
 */
void add_real_time_alert_for_police()
{

    RoadMapAction RoadMapAlertActions[2] = {

   {"mydirection", "My direction", "My direction", NULL,
      "My direction", add_real_time_alert_for_police_my_direction},

   {"oppositedirection", "Opposite direction", "Opposite direction", NULL,
      "Opposite direction", add_real_time_alert_for_police_opposite_direction}
    };

   alerts_direction_menu("Police Direction Menu", RoadMapAlertActions);

}

/**
 * Report an alert for accident
 * @param None
 * @return void
 */
void add_real_time_alert_for_accident()
{

    RoadMapAction RoadMapAlertActions[2] = {

   {"mydirection", "My direction", "My direction", NULL,
      "My direction", add_real_time_alert_for_accident_my_direction},

   {"oppositedirection", "Opposite direction", "Opposite direction", NULL,
      "Opposite direction", add_real_time_alert_for_accident_opposite_direction}
    };

 alerts_direction_menu("Accident Direction Menu", RoadMapAlertActions);

}

/**
 * Report an alert for traffic jam
 * @param None
 * @return void
 */
void add_real_time_alert_for_traffic_jam()
{

    RoadMapAction RoadMapAlertActions[2] = {

   {"mydirection", "My direction", "My direction", NULL,
      "My direction", add_real_time_alert_for_traffic_jam_my_direction},

   {"oppositedirection", "Opposite direction", "Opposite direction", NULL,
      "Opposite direction", add_real_time_alert_for_traffic_jam_opposite_direction}
    };

    alerts_direction_menu("Jam Direction Menu" ,RoadMapAlertActions);
}


/**
 * Report an alert for traffic jam
 * @param None
 * @return void
 */
void add_real_time_alert_for_hazard()
{

    RoadMapAction RoadMapAlertActions[2] = {

   {"mydirection", "My direction", "My direction", NULL,
      "My direction", add_real_time_alert_for_hazard_my_direction},

   {"oppositedirection", "Opposite direction", "Opposite direction", NULL,
      "Opposite direction", add_real_time_alert_for_hazard_opposite_direction}
    };

    alerts_direction_menu("Hazard Direction Menu" ,RoadMapAlertActions);
}


/**
 * Report an alert for other
 * @param None
 * @return void
 */
void add_real_time_alert_for_other()
{

    RoadMapAction RoadMapAlertActions[2] = {

   {"mydirection", "My direction", "My direction", NULL,
      "My direction", add_real_time_alert_for_other_my_direction},

   {"oppositedirection", "Opposite direction", "Opposite direction", NULL,
      "Opposite direction", add_real_time_alert_for_other_opposite_direction}
    };

    alerts_direction_menu("Other Direction Menu" ,RoadMapAlertActions);
}


/**
 * Report an alert for construction
 * @param None
 * @return void
 */
void add_real_time_alert_for_construction()
{

    RoadMapAction RoadMapAlertActions[2] = {

   {"mydirection", "My direction", "My direction", NULL,
      "My direction", add_real_time_alert_for_construction_my_direction},

   {"oppositedirection", "Opposite direction", "Opposite direction", NULL,
      "Opposite direction", add_real_time_alert_for_construction_opposite_direction}
    };

    alerts_direction_menu("Construction Direction Menu" ,RoadMapAlertActions);
}
/**
 * Report an alert for police my direction
 * @param None
 * @return void
 */
void add_real_time_alert_for_police_my_direction()
{

    report_alert(RT_ALERT_TYPE_POLICE, "" , RT_ALERT_MY_DIRECTION);
}


/**
 * Report an alert for Accident my direction
 * @param None
 * @return void
 */
void add_real_time_alert_for_accident_my_direction()
{

    report_alert(RT_ALERT_TYPE_ACCIDENT, "",RT_ALERT_MY_DIRECTION);
}

/**
 * Report an alert for Traffic Jam my direction
 * @param None
 * @return void
 */
void add_real_time_alert_for_traffic_jam_my_direction()
{

    report_alert(RT_ALERT_TYPE_TRAFFIC_JAM, "",RT_ALERT_MY_DIRECTION);
}

/**
 * Report an alert for Hazard Jam my direction
 * @param None
 * @return void
 */
void add_real_time_alert_for_hazard_my_direction()
{

    report_alert(RT_ALERT_TYPE_HAZARD, "",RT_ALERT_MY_DIRECTION);
}

/**
 * Report an alert for Other my direction
 * @param None
 * @return void
 */
void add_real_time_alert_for_other_my_direction()
{

    report_alert(RT_ALERT_TYPE_OTHER, "",RT_ALERT_MY_DIRECTION);
}

/**
 * Report an alert for constrcution my direction
 * @param None
 * @return void
 */
void add_real_time_alert_for_construction_my_direction()
{

    report_alert(RT_ALERT_TYPE_CONSTRUCTION, "",RT_ALERT_MY_DIRECTION);
}

/**
 * Report an alert for police on the opposite direction
 * @param None
 * @return void
 */
void add_real_time_alert_for_police_opposite_direction()
{

    report_alert(RT_ALERT_TYPE_POLICE, "" , RT_ALERT_OPPSOITE_DIRECTION);
}

/**
 * Report an alert for accident on the opposite direction
 * @param None
 * @return void
 */
void add_real_time_alert_for_accident_opposite_direction()
{

    report_alert(RT_ALERT_TYPE_ACCIDENT, "" , RT_ALERT_OPPSOITE_DIRECTION);
}

/**
 * Report an alert for Traffic Jam on the opposite direction
 * @param None
 * @return void
 */
void add_real_time_alert_for_traffic_jam_opposite_direction()
{

    report_alert(RT_ALERT_TYPE_TRAFFIC_JAM, "", RT_ALERT_OPPSOITE_DIRECTION);
}


/**
 * Report an alert for Hazard on the opposite direction
 * @param None
 * @return void
 */
void add_real_time_alert_for_hazard_opposite_direction()
{

    report_alert(RT_ALERT_TYPE_HAZARD, "", RT_ALERT_OPPSOITE_DIRECTION);
}

/**
 * Report an alert for Other on the opposite direction
 * @param None
 * @return void
 */
void add_real_time_alert_for_other_opposite_direction()
{

    report_alert(RT_ALERT_TYPE_OTHER, "", RT_ALERT_OPPSOITE_DIRECTION);
}

/**
 * Report an alert for Construction on the opposite direction
 * @param None
 * @return void
 */
void add_real_time_alert_for_construction_opposite_direction()
{

    report_alert(RT_ALERT_TYPE_CONSTRUCTION, "", RT_ALERT_OPPSOITE_DIRECTION);
}

///////////////////////////////////////////////////////////////////////////////////////////
void add_real_time_chit_chat()
{
    const RoadMapGpsPosition   *TripLocation;
    RTAlertContext *AlertConext;

    if (!roadmap_gps_have_reception()) {
        roadmap_messagebox("Error", "No GPS reception!");
        return;
    }

    TripLocation = roadmap_trip_get_gps_position("AlertSelection");
    if ((TripLocation == NULL) || (TripLocation->latitude <= 0) || (TripLocation->longitude <= 0)) {
        roadmap_messagebox ("Error", "Can't find current street.");
        return;
    }

    AlertConext = calloc(1, sizeof(RTAlertContext));

    AlertConext->iType = RT_ALERT_TYPE_CHIT_CHAT;
    AlertConext->szDescription = "";
    AlertConext->iDirection = RT_ALERT_MY_DIRECTION;

#ifdef __SYMBIAN32__
    ShowEditbox(roadmap_lang_get("Chat"), "", keyboard_callback,
            AlertConext, EEditBoxEmptyForbidden );
#else
    ssd_keyboard_show (SSD_KEYBOARD_LETTERS,
            roadmap_lang_get("Chat"), "", NULL, keyboard_callback, AlertConext);
#endif
}

/**
 * Keyboard callback for posting a comment
 * @param type - the button that was pressed, new_value - text, context - hold a pointer to the alert
 * @return int
 */
static int post_comment_keyboard_callback(int type, const char *new_value,
        void *context)
{
    BOOL success;
    RTAlert *pAlert = (RTAlert *)context;
    if (type != SSD_KEYBOARD_OK)
        return 1;

    if (new_value[0] == 0)
        return 0;

    success = Realtime_Post_Alert_Comment(pAlert->iID, new_value);
    if (success){
        ssd_keyboard_hide_all();
        if (gPopUpType == POP_UP_ALERT){
            RTAlerts_Stop_Scrolling();
            gPopUpType = -1;
        }else{
            RTAlerts_Comment_PopUp_Hide();
            gPopUpType = -1;
        }
    }
    else
        ssd_keyboard_hide();


    return 1;

}

/**
 * Post a comment to the a specific alert ID
 * @param iAlertId - the alert ID to post the comment to
 * @return void
 */
void real_time_post_alert_comment_by_id(int iAlertid)
{

    RTAlert *pAlert = RTAlerts_Get_By_ID(iAlertid);
    if (pAlert == NULL)
        return;

#ifdef __SYMBIAN32__
    ShowEditbox(roadmap_lang_get("Comment"), "", post_comment_keyboard_callback,
            pAlert, EEditBoxEmptyForbidden );
#else
    ssd_keyboard_show (SSD_KEYBOARD_LETTERS,
            roadmap_lang_get("Comment"), "", NULL, post_comment_keyboard_callback, (void *)pAlert);
#endif

}

/**
 * Post a comment to the currently active alert
 * @param None
 * @return void
 */
void real_time_post_alert_comment()
{

    gPopUpType = POP_UP_ALERT;
    real_time_post_alert_comment_by_id(gCurrentAlertId);
}

/**
 * Remove an alert
 * @param iAlertId - the id of the alert to delete
 * @return void
 */
BOOL real_time_remove_alert(int iAlertId)
{
    return Realtime_Remove_Alert(iAlertId);
}

/**
 * Checks the penality of an alert if it is on the line.
 * @param line_id- the line ID to check, against_dir -the direction of the travel
 * @param the penalty of the alert, 0 if no alert is on that line.
 * @return void
 */
int RTAlerts_Pernalty(int line_id, int against_dir)
{
    int i;
    int line_from_point;
    int line_to_point;
    int square = roadmap_square_active ();

    if (gAlertsTable.iCount == 0)
        return FALSE;
    for (i=0; i<gAlertsTable.iCount; i++)
    {
        if (RTAlerts_Is_Reroutable(gAlertsTable.alert[i]))
        {
            if (gAlertsTable.alert[i]->iLineId == line_id &&
                     gAlertsTable.alert[i]->iSquare == square)
            {
                roadmap_line_points(line_id, &line_from_point, &line_to_point);
                if (((line_from_point == gAlertsTable.alert[i]->iNode1)
                        && (!against_dir)) || ((line_to_point
                        == gAlertsTable.alert[i]->iNode1) && (against_dir)))
                {
                    if (gAlertsTable.alert[i]->iType == RT_ALERT_TYPE_ACCIDENT)
                        return 3600;
                    else
                        return 0;
                }
            }
        }
    }
    return 0;
}

int RTAlerts_Alert_near_position( RoadMapPosition position, int distance)
{
     RoadMapPosition context_save_pos;
    int context_save_zoom;
    RoadMapPosition alert_position;
    int new_distance, saved_distance = 5000;
    int alert_id = -1;
    int i;

    if (RTAlerts_Is_Empty())
        return -1;

    roadmap_math_get_context(&context_save_pos, &context_save_zoom);

    for (i=0; i < gAlertsTable.iCount; i++)
    {
        alert_position.longitude = gAlertsTable.alert[i]->iLongitude;
        alert_position.latitude = gAlertsTable.alert[i]->iLatitude;

        new_distance = roadmap_math_distance(&alert_position, &position);
        if (new_distance > distance)
                continue;

        if (new_distance < saved_distance){
                saved_distance = new_distance;
                alert_id = gAlertsTable.alert[i]->iID;
        }
    }


    roadmap_math_set_context(&context_save_pos, context_save_zoom);
    return alert_id;
}


/**
 * Button callbak for the messagebox indicating an incident on the route.
 * @param widget - the widget that was pressed
 * @param new_value - the value of the widget
 * @return void
 */
static int button_callback(SsdWidget widget, const char *new_value)
{

    if (!strcmp(widget->name, "OK") || !strcmp(widget->name, "Recalculate"))
    {

        ssd_dialog_hide_current(dec_ok);

        if (!strcmp(widget->name, "Recalculate"))
        {
            navigate_main_calc_route();
        }
        RTAlerts_Stop_Scrolling();
        return 1;

    }


    ssd_dialog_hide_current(dec_close);
    return 1;
}

/**
 * Creates and display a messagebox indicating an incident on the route.
 * @param pAlert - pointer to the alert that is on the route
 * @return void
 */
static void create_ssd_dialog(RTAlert *pAlert)
{
    const char *description;
    SsdWidget dialog = ssd_dialog_new("rt_alert_on_my_route",
            roadmap_lang_get("New Alert"),
            NULL,
            SSD_CONTAINER_BORDER|SSD_CONTAINER_TITLE|SSD_DIALOG_FLOAT|
            SSD_ALIGN_CENTER|SSD_ALIGN_VCENTER|SSD_ROUNDED_CORNERS);

    if (pAlert->iType== RT_ALERT_TYPE_ACCIDENT)
        description = roadmap_lang_get("New accident on your route");
    else if (pAlert->iType == RT_ALERT_TYPE_TRAFFIC_JAM)
        description = roadmap_lang_get("New traffic jam on your route");
    else
        description = roadmap_lang_get("New incident on your route");

    ssd_widget_add(dialog, ssd_text_new("text", description, 13,
    SSD_END_ROW|SSD_WIDGET_SPACE));


    /* Spacer */
    ssd_widget_add(dialog, ssd_container_new("spacer1", NULL, 0, 20,
    SSD_END_ROW));

    ssd_widget_add(dialog, ssd_button_label("Recalculate",
            roadmap_lang_get("Recalculate"),
            SSD_WS_TABSTOP, button_callback));

    ssd_widget_add(dialog, ssd_button_label("Cancel",
            roadmap_lang_get("Cancel"),
            SSD_WS_TABSTOP, button_callback));
}


/**
 *
 *
 * @return void
 */
static void RTAlerts_Comment_Softkeys_hide(){
    roadmap_softkeys_remove_left_soft_key("Comment_Reply");
    roadmap_softkeys_remove_right_soft_key("Comment_Ignore");
}


/**
 *
 *
 * @return void
 */
static void RTAlerts_Comment_PopUp_Hide(void)
{
    roadmap_display_hide("Comment Pop Up");
    RTAlerts_Comment_Softkeys_hide();
}

/**
 *
 *
 * @return void
 */
static void RTAlerts_Comment_reply(void){
    gPopUpType = POP_UP_COMMENT;
    real_time_post_alert_comment_by_id(gCurrentAlertId);
}

/**
 *
 *
 * @return void
 */
static void RTAlerts_Comment_ignore(void){
    RTAlerts_Comment_PopUp_Hide();
}

/**
 *
 *
 * @return void
 */
static void RTAlerts_Comment_set_softkeys(){
    static Softkey r_sk, l_sk;
    strcpy(r_sk.text, "Reply");
    r_sk.callback = RTAlerts_Comment_reply;
    roadmap_softkeys_set_left_soft_key("Comment_Reply", &r_sk);

    strcpy(l_sk.text, "Ignore");
    l_sk.callback = RTAlerts_Comment_ignore;
    roadmap_softkeys_set_right_soft_key("Comment_Ignore", &l_sk);

}
/**
 *
 *
 * @return void
 */
static void RTAlerts_Comment_PopUp(RTAlertComment *Comment, RTAlert *Alert)
{
    char CommentStr[700];
    CommentStr[0] = 0;

     snprintf(CommentStr + strlen(CommentStr), sizeof(CommentStr)
                - strlen(CommentStr), "<h1>%s%s",roadmap_lang_get("Responding to you"), NEW_LINE);

    if (Alert->iType == RT_ALERT_TYPE_ACCIDENT)
        snprintf(CommentStr + strlen(CommentStr), sizeof(CommentStr)
                - strlen(CommentStr), "%s,", roadmap_lang_get("Accident"));
    else if (Alert->iType == RT_ALERT_TYPE_POLICE)
        snprintf(CommentStr + strlen(CommentStr), sizeof(CommentStr)
                - strlen(CommentStr), "%s,", roadmap_lang_get("Police"));
    else if (Alert->iType == RT_ALERT_TYPE_TRAFFIC_JAM)
        snprintf(CommentStr + strlen(CommentStr), sizeof(CommentStr)
                - strlen(CommentStr), "%s,", roadmap_lang_get("Traffic jam"));
    else if (Alert->iType == RT_ALERT_TYPE_HAZARD)
        snprintf(CommentStr + strlen(CommentStr), sizeof(CommentStr)
                - strlen(CommentStr), "%s,", roadmap_lang_get("Hazard"));
    else if (Alert->iType == RT_ALERT_TYPE_OTHER)
        snprintf(CommentStr + strlen(CommentStr), sizeof(CommentStr)
                - strlen(CommentStr), "%s,", roadmap_lang_get("Other"));
    gCurrentAlertId = Alert->iID;
    snprintf(CommentStr + strlen(CommentStr), sizeof(CommentStr)
                - strlen(CommentStr), "%s%s", Alert->sLocationStr, NEW_LINE);

     snprintf(CommentStr + strlen(CommentStr), sizeof(CommentStr)
                - strlen(CommentStr), "%s%s", Comment->sDescription, NEW_LINE);

     snprintf(CommentStr + strlen(CommentStr), sizeof(CommentStr)
                - strlen(CommentStr), "(%s)%s", roadmap_lang_get(Comment->sPostedBy), NEW_LINE);

    roadmap_display_pop_up("Comment Pop Up",
            NULL, NULL, CommentStr);
    RTAlerts_Comment_set_softkeys();

}

int RTAlerts_Is_Cancelable(int alertId){
    return TRUE;
}

int Rtalerts_Delete(int alertId){
    return real_time_remove_alert(alertId);
}

