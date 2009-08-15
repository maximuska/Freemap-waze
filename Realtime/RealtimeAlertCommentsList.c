/* RealtimeAlertsCommentsList.c - manage the Real Time Alerts Commemnts list display
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
 * SYNOPSYS:
 *
 *   See RealtimeAlertsCommentsList.h
 */

#include <string.h>
#include <stdlib.h>

#include "roadmap.h"
#include "roadmap_types.h"
#include "roadmap_history.h"
#include "roadmap_locator.h"
#include "roadmap_street.h"
#include "roadmap_lang.h"
#include "roadmap_messagebox.h"
#include "roadmap_geocode.h"
#include "roadmap_config.h"
#include "roadmap_trip.h"
#include "roadmap_county.h"
#include "roadmap_display.h"
#include "roadmap_math.h"
#include "roadmap_navigate.h"
#include "ssd/ssd_keyboard.h"
#include "ssd/ssd_generic_list_dialog.h"
#include "ssd/ssd_contextmenu.h"

#include "RealtimeAlerts.h"
#include "Realtime.h"
#include "RealtimeAlertsList.h"
#include "RealtimeAlertCommentsList.h"

#define MAX_COMMENTS_ENTRIES 100
static   BOOL                   g_context_menu_is_active= FALSE;

// Context menu:
static ssd_cm_item      context_menu_items[] =
{
   SSD_CM_INIT_ITEM  ( "Show on map",  rtcl_cm_show),
   SSD_CM_INIT_ITEM  ( "Post Comment", rtcl_cm_add_comments),
   SSD_CM_INIT_ITEM  ( "Exit_key",     rtcl_cm_exit)
};
static ssd_contextmenu  context_menu = SSD_CM_INIT_MENU( context_menu_items);

typedef struct
{

    const char *title;
    int alert_id;

} RoadMapRealTimeAlertListCommentsDialog;

typedef struct
{

    char *value;
    char *label;
} RoadMapRealTimeAlertCommentsList;

static int g_Alert_Id;

///////////////////////////////////////////////////////////////////////////////////////////
static void on_option_show(){
     ssd_generic_list_dialog_hide_all();
    RTAlerts_Popup_By_Id(g_Alert_Id);
}

///////////////////////////////////////////////////////////////////////////////////////////
static void on_option_add_comment(){
     real_time_post_alert_comment_by_id(g_Alert_Id);
}

///////////////////////////////////////////////////////////////////////////////////////////
static void on_option_selected(  BOOL              made_selection,
                                 ssd_cm_item_ptr   item,
                                 void*             context)
{
   real_time_list_context_menu_items   selection;
   int                                 exit_code = dec_ok;

   g_context_menu_is_active = FALSE;

   if( !made_selection)
      return;

   selection = (real_time_list_context_menu_items)item->id;

   switch( selection)
   {
      case rtcl_cm_show:
        on_option_show();
         break;

      case rtcl_cm_add_comments:
         on_option_add_comment();
         break;


      case rtcl_cm_exit:
         exit_code = dec_cancel;
        ssd_dialog_hide_all( exit_code);
        roadmap_screen_refresh ();
         break;

      default:
         break;
   }
}

///////////////////////////////////////////////////////////////////////////////////////////

static int on_options(SsdWidget widget, const char *new_value, void *context)
{
    int   menu_x;

   if(g_context_menu_is_active)
   {
      ssd_dialog_hide_current(dec_cancel);
      g_context_menu_is_active = FALSE;
   }

   if  (ssd_widget_rtl (NULL))
       menu_x = SSD_X_SCREEN_RIGHT;
    else
        menu_x = SSD_X_SCREEN_LEFT;

   ssd_context_menu_show(  menu_x,   // X
                           SSD_Y_SCREEN_BOTTOM, // Y
                           &context_menu,
                           on_option_selected,
                           NULL,
                           dir_default);

   g_context_menu_is_active = TRUE;

   return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////
static int RealtimeAlertCommetsListCallBack(SsdWidget widget,
        const char *new_value, const void *value, void *context)
{

    int alertId;

    if (!value) return 0;

    alertId = atoi((const char*)value);
    ssd_generic_list_dialog_hide_all();
    RTAlerts_Popup_By_Id(alertId);

    return 1;
}


///////////////////////////////////////////////////////////////////////////////////////////
int RealtimeAlertCommentsList(int iAlertId)
{

    static char *labels[MAX_COMMENTS_ENTRIES];
    static void *values[MAX_COMMENTS_ENTRIES];
    static void *icons[MAX_COMMENTS_ENTRIES];
    static int  flags[MAX_COMMENTS_ENTRIES];

    int iNumberOfComments = -1;
    int iCount = 0;
    RTAlert *alert;
    char AlertStr[300];
    char CommentStr[300];
    char str[100];
    int distance;
    RoadMapGpsPosition CurrentPosition;
    RoadMapPosition position, current_pos;
    const RoadMapPosition *gps_pos;
    PluginLine line;
    int Direction;
    int distance_far;
    char dist_str[100];
    char unit_str[20];
    int i;
    const char *myUserName;
    int height;

    RTAlertCommentsEntry *CommentEntry;

    static RoadMapRealTimeAlertListCommentsDialog context =
    { "Real Time Alert Comments", -1 };
    AlertStr[0] = 0;
    str[0] = 0;


    iNumberOfComments = RTAlerts_Get_Number_of_Comments(iAlertId);
    if (iNumberOfComments == 0)
    {
        return -1;
    }


    for (i=0; i<MAX_COMMENTS_ENTRIES; i++)
    {
        icons[i] = NULL;
    }

    AlertStr[0] = 0;

    alert = RTAlerts_Get_By_ID(iAlertId);
    position.longitude = alert->iLongitude;
    position.latitude = alert->iLatitude;
    context.alert_id = iAlertId;
    g_Alert_Id = iAlertId;
    roadmap_math_set_context((RoadMapPosition *)&position, 20);

    if (alert->iType == RT_ALERT_TYPE_ACCIDENT)
        snprintf(AlertStr + strlen(AlertStr), sizeof(AlertStr)
                - strlen(AlertStr), "%s", roadmap_lang_get("Accident"));
    else if (alert->iType == RT_ALERT_TYPE_POLICE)
        snprintf(AlertStr + strlen(AlertStr), sizeof(AlertStr)
                - strlen(AlertStr), "%s", roadmap_lang_get("Police"));
    else if (alert->iType == RT_ALERT_TYPE_TRAFFIC_JAM)
        snprintf(AlertStr + strlen(AlertStr), sizeof(AlertStr)
                - strlen(AlertStr), "%s", roadmap_lang_get("Traffic jam"));
    else if (alert->iType == RT_ALERT_TYPE_HAZARD)
        snprintf(AlertStr + strlen(AlertStr), sizeof(AlertStr)
                - strlen(AlertStr), "%s", roadmap_lang_get("Hazard"));
    else if (alert->iType == RT_ALERT_TYPE_TRAFFIC_JAM)
        snprintf(AlertStr + strlen(AlertStr), sizeof(AlertStr)
                - strlen(AlertStr), "%s", roadmap_lang_get("Other"));
    else
        snprintf(AlertStr + strlen(AlertStr), sizeof(AlertStr)
                - strlen(AlertStr), "%s", roadmap_lang_get("Chit Chat"));

    if (roadmap_navigate_get_current(&CurrentPosition, &line, &Direction) == -1)
    {
        // check the distance to the alert
        gps_pos = roadmap_trip_get_position("GPS");
        if (gps_pos != NULL)
        {
            current_pos.latitude = gps_pos->latitude;
            current_pos.longitude = gps_pos->longitude;
        }
        else
        {
            current_pos.latitude = -1;
            current_pos.longitude = -1;
        }
    }
    else
    {
        current_pos.latitude = CurrentPosition.latitude;
        current_pos.longitude = CurrentPosition.longitude;
    }

    if ((current_pos.latitude != -1) && (current_pos.longitude != -1))
    {
        distance = roadmap_math_distance(&current_pos, &position);
        distance_far = roadmap_math_to_trip_distance(distance);

        if (distance_far > 0)
        {
            int tenths = roadmap_math_to_trip_distance_tenths(distance);
            snprintf(dist_str, sizeof(str), "%d.%d", distance_far, tenths % 10);
            snprintf(unit_str, sizeof(unit_str), "%s",
                    roadmap_lang_get(roadmap_math_trip_unit()));
        }
        else
        {
            snprintf(dist_str, sizeof(str), "%d", distance);
            snprintf(unit_str, sizeof(unit_str), "%s",
                    roadmap_lang_get(roadmap_math_distance_unit()));
        }
        snprintf(AlertStr + strlen(AlertStr), sizeof(AlertStr)
                - strlen(AlertStr), " (%s %s)", dist_str, unit_str);
    }

    snprintf(AlertStr + strlen(AlertStr), sizeof(AlertStr) - strlen(AlertStr),
            "\n%s", alert->sLocationStr);

    snprintf(AlertStr + strlen(AlertStr), sizeof(AlertStr) - strlen(AlertStr),
            "\n%s", alert->sDescription);

    snprintf(str, sizeof(str), "%d", alert->iID);

    labels[iCount] = strdup(AlertStr);

    values[iCount] = strdup(str);
    icons[iCount] = strdup(RTAlerts_Get_Map_Icon(alert->iID));
    flags[iCount] = 0;

    CommentEntry = alert->Comment;

    iCount++;

    myUserName = RealTime_Get_UserName();

    while (iCount < MAX_COMMENTS_ENTRIES && (CommentEntry != NULL))
    {
        time_t now;
        int timeDiff;
        CommentStr[0] = 0;
        // Display when the alert was generated


        now = time(NULL);
        timeDiff = (int)difftime(now, (time_t)CommentEntry->comment.i64ReportTime);
        if (timeDiff <0)   timeDiff = 0;

        snprintf(CommentStr + strlen(CommentStr), sizeof(CommentStr)
                - strlen(CommentStr), " %d.", iCount);
        if (timeDiff < 60)
            snprintf(CommentStr + strlen(CommentStr), sizeof(CommentStr)
                    - strlen(CommentStr),
                    roadmap_lang_get("Commented %d seconds ago"), timeDiff);
        else if ((timeDiff > 60) && (timeDiff < 3600))
            snprintf(CommentStr + strlen(CommentStr), sizeof(CommentStr)
                    - strlen(CommentStr),
                    roadmap_lang_get("Commented %d minutes ago"), timeDiff/60);
        else
            snprintf(CommentStr + strlen(CommentStr), sizeof(CommentStr)
                    - strlen(CommentStr),
                    roadmap_lang_get("Commented %2.1f hours ago"),
                    (float)timeDiff/3600);

        snprintf(CommentStr + strlen(CommentStr), sizeof(CommentStr)
                - strlen(CommentStr), " %s %s%s", roadmap_lang_get("by"),
                CommentEntry->comment.sPostedBy, NEW_LINE);
        snprintf(CommentStr + strlen(CommentStr), sizeof(CommentStr)
                - strlen(CommentStr), "  %s%s ",
                CommentEntry->comment.sDescription, NEW_LINE);

        labels[iCount] = strdup(CommentStr);
        values[iCount] = strdup(str);
        flags[iCount] = SSD_POINTER_COMMENT;
        if (CommentEntry->comment.bCommentByMe)
            icons[iCount] = "Comment_Sent";
        else
            icons[iCount] = "Comment_Received";
        iCount++;
        CommentEntry = CommentEntry->next;
    }

#ifdef IPHONE
    height = 90;
#else
    height = 65;
#endif

    ssd_generic_icon_list_dialog_show(roadmap_lang_get("Real Time Alerts Comments"),
            iCount, (const char **)labels, (const void **)values,
            (const char **)icons, (const int*)&flags,RealtimeAlertCommetsListCallBack, NULL,  &context,
            roadmap_lang_get("Options"), on_options, height, SSD_HEADER_GREEN, FALSE);

    return 0;
}

