/* navigate_main.c - main navigate plugin file
 *
 * LICENSE:
 *
 *   Copyright 2006 Ehud Shabtai
 *
 *   This file is part of RoadMap.
 *
 *   RoadMap is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
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
 *   See navigate_main.h
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "roadmap.h"
#include "roadmap_pointer.h"
#include "roadmap_plugin.h"
#include "roadmap_line.h"
#include "roadmap_display.h"
#include "roadmap_message.h"
#include "roadmap_voice.h"
#include "roadmap_messagebox.h"
#include "roadmap_canvas.h"
#include "roadmap_street.h"
#include "roadmap_trip.h"
#include "roadmap_navigate.h"
#include "roadmap_screen.h"
#include "roadmap_line_route.h"
#include "roadmap_math.h"
#include "roadmap_point.h"
#include "roadmap_layer.h"
#include "roadmap_adjust.h"
#include "roadmap_lang.h"
#include "roadmap_address.h"
#include "roadmap_sound.h"
#include "roadmap_locator.h"
#include "roadmap_config.h"
#include "roadmap_skin.h"
#include "roadmap_main.h"
#include "roadmap_square.h"
#include "roadmap_search.h"
#include "roadmap_view.h"
#include "roadmap_softkeys.h"

#ifdef SSD
#include "ssd/ssd_dialog.h"
#include "ssd/ssd_text.h"
#include "ssd/ssd_button.h"
#include "ssd/ssd_container.h"
#include "ssd/ssd_bitmap.h"
#include "ssd/ssd_generic_list_dialog.h"
#else
#include "roadmap_dialog.h"
#endif

//FIXME remove when navigation will support plugin lines
#include "editor/editor_plugin.h"

#include "Realtime/Realtime.h"

#include "navigate_plugin.h"
#include "navigate_bar.h"
#include "navigate_instr.h"
#include "navigate_traffic.h"
#include "navigate_cost.h"
#include "navigate_route.h"
#include "navigate_zoom.h"
#include "navigate_main.h"

#define ROUTE_PEN_WIDTH 5
//#define TEST_ROUTE_CALC 1

extern const char NAVIGATE_DIR_IMG[][40];

static RoadMapConfigDescriptor NavigateConfigRouteColor =
                    ROADMAP_CONFIG_ITEM("Navigation", "RouteColor");

static RoadMapConfigDescriptor NavigateConfigPossibleRouteColor =
                    ROADMAP_CONFIG_ITEM("Navigation", "PossibleRouteColor");

RoadMapConfigDescriptor NavigateConfigAutoZoom =
                  ROADMAP_CONFIG_ITEM("Routing", "Auto zoom");

RoadMapConfigDescriptor NavigateConfigNavigationGuidance =
                  ROADMAP_CONFIG_ITEM("Navigation", "Navigation Guidance");

RoadMapConfigDescriptor NavigateConfigLastPos =
                  ROADMAP_CONFIG_ITEM("Navigation", "Last position");

RoadMapConfigDescriptor NavigateConfigNavigating =
                  ROADMAP_CONFIG_ITEM("Navigation", "Is navigating");

int NavigateEnabled = 0;
int NavigatePluginID = -1;
static int NavigateTrackEnabled = 0;
static int NavigateTrackFollowGPS = 0;
static RoadMapPen NavigatePen[2];
static RoadMapPen NavigatePenEst[2];
static void navigate_update (RoadMapPosition *position, PluginLine *current);
static void navigate_get_next_line
          (PluginLine *current, int direction, PluginLine *next);

static int navigate_line_in_route (PluginLine *current, int direction);

static RoadMapCallback NextMessageUpdate;

static int NavigateDistanceToDest;
static int NavigateETA;
static int NavigateDistanceToTurn;
static int NavigateDistanceToNext;
static int NavigateETAToTurn;
static int NavigateFlags;
static int NavigateETADiff;
static time_t NavigateETATime;
static time_t NavigateCalcTime;

RoadMapNavigateRouteCB NavigateCallbacks = {
   &navigate_update,
   &navigate_get_next_line,
   &navigate_line_in_route
};


#define MAX_NAV_SEGEMENTS 2500

static NavigateSegment NavigateSegments[MAX_NAV_SEGEMENTS];
static int NavigateNumSegments = 0;
static int NavigateCurrentSegment = 0;
static PluginLine NavigateDestination = PLUGIN_LINE_NULL;
static int NavigateDestPoint;
static RoadMapPosition NavigateDestPos;
static RoadMapPosition NavigateSrcPos;
static int NavigateNextAnnounce;

static const char *ExitName[] = {
    "First", "Second", "Third", "Fourth", "Fifth", "Sixth", "Seventh"
};
#define MaxExitName ((int)(sizeof (ExitName) / sizeof (ExitName[0])))

static int navigate_find_track_points (PluginLine *from_line, int *from_point,
                                        PluginLine *to_line, int *to_point,
                                        int *from_direction, int recalc_route) {

   const RoadMapPosition *position = NULL;
   RoadMapPosition from_position;
   RoadMapPosition to_position;
   PluginLine line;
   int distance;
   int from_tmp;
   int to_tmp;
   int direction = ROUTE_DIRECTION_NONE;

   *from_point = -1;


   if (NavigateTrackFollowGPS || recalc_route) {

      RoadMapGpsPosition pos;

#ifndef J2ME
      //FIXME remove when navigation will support plugin lines
      editor_plugin_set_override (0);
#endif

      if ((roadmap_navigate_get_current (&pos, &line, &direction) != -1) &&
          (roadmap_plugin_get_id(&line) == ROADMAP_PLUGIN_ID)) {

         roadmap_adjust_position (&pos, &NavigateSrcPos);

            roadmap_square_set_current (line.square);
         roadmap_line_points (line.line_id, &from_tmp, &to_tmp);

         if (direction == ROUTE_DIRECTION_WITH_LINE) {

            *from_point = to_tmp;
         } else {

            *from_point = from_tmp;
         }

      } else {
         position = roadmap_trip_get_position ("GPS");
         if (position) NavigateSrcPos = *position;
         direction = ROUTE_DIRECTION_NONE;
      }

#ifndef J2ME
      //FIXME remove when navigation will support plugin lines
      editor_plugin_set_override (1);
#endif

   } else {

      position = roadmap_trip_get_position ("Departure");
      NavigateSrcPos = *position;
   }

   if (*from_point == -1) {

      if (!position)
      {
         roadmap_messagebox("Error", "Current position is unknown");
         return -1;
      }

#ifndef J2ME
      //FIXME remove when navigation will support plugin lines
      editor_plugin_set_override (0);
#endif

      if ((roadmap_navigate_retrieve_line
               (position, 0, 200, &line, &distance, LAYER_ALL_ROADS) == -1) ||
            (roadmap_plugin_get_id (&line) != ROADMAP_PLUGIN_ID)) {

#ifndef J2ME
         //FIXME remove when navigation will support plugin lines
         editor_plugin_set_override (1);
#endif

         roadmap_messagebox
            ("Error", "Can't find a road near departure point.");

         return -1;
      }

#ifndef J2ME
      //FIXME remove when navigation will support plugin lines
      editor_plugin_set_override (1);
#endif

   }

   *from_line = line;

   if (direction == ROUTE_DIRECTION_NONE) {

        roadmap_square_set_current (line.square);
      switch (roadmap_plugin_get_direction (from_line, ROUTE_CAR_ALLOWED)) {
         case ROUTE_DIRECTION_ANY:
         case ROUTE_DIRECTION_NONE:
            roadmap_line_points (from_line->line_id, &from_tmp, &to_tmp);
            roadmap_point_position (from_tmp, &from_position);
            roadmap_point_position (to_tmp, &to_position);

            if (roadmap_math_distance (position, &from_position) <
                  roadmap_math_distance (position, &to_position)) {
               *from_point = from_tmp;
               direction = ROUTE_DIRECTION_AGAINST_LINE;
            } else {
               *from_point = to_tmp;
               direction = ROUTE_DIRECTION_WITH_LINE;
            }
            break;
         case ROUTE_DIRECTION_WITH_LINE:
            roadmap_line_points (from_line->line_id, &from_tmp, from_point);
            direction = ROUTE_DIRECTION_WITH_LINE;
            break;
         case ROUTE_DIRECTION_AGAINST_LINE:
            roadmap_line_points (from_line->line_id, from_point, &from_tmp);
            direction = ROUTE_DIRECTION_AGAINST_LINE;
            break;
         default:
            roadmap_line_points (from_line->line_id, &from_tmp, from_point);
            direction = ROUTE_DIRECTION_WITH_LINE;
      }
   }
   if (from_direction) *from_direction = direction;


   if (to_line->plugin_id != INVALID_PLUGIN_ID) {
      /* we already calculated the destination point */
      return 0;
   }

   position = roadmap_trip_get_position ("Destination");
   if (!position) return -1;

   NavigateDestPos = *position;

#ifndef J2ME
   //FIXME remove when navigation will support plugin lines
   editor_plugin_set_override (0);
#endif

   if ((roadmap_navigate_retrieve_line
            (position, 0, 50, &line, &distance, LAYER_ALL_ROADS) == -1) ||
         (roadmap_plugin_get_id (&line) != ROADMAP_PLUGIN_ID)) {

      roadmap_messagebox ("Error", "Can't find a road near destination point.");
#ifndef J2ME
      //FIXME remove when navigation will support plugin lines
      editor_plugin_set_override (1);
#endif

      return -1;
   }

#ifndef J2ME
   //FIXME remove when navigation will support plugin lines
   editor_plugin_set_override (1);
#endif
   *to_line = line;

   switch (roadmap_plugin_get_direction (to_line, ROUTE_CAR_ALLOWED)) {
      case ROUTE_DIRECTION_ANY:
      case ROUTE_DIRECTION_NONE:
         roadmap_line_points (to_line->line_id, &from_tmp, &to_tmp);
         roadmap_point_position (from_tmp, &from_position);
         roadmap_point_position (to_tmp, &to_position);

         if (roadmap_math_distance (position, &from_position) <
             roadmap_math_distance (position, &to_position)) {
            *to_point = from_tmp;
         } else {
            *to_point = to_tmp;
         }
         break;
      case ROUTE_DIRECTION_WITH_LINE:
         roadmap_line_points (to_line->line_id, to_point, &to_tmp);
         break;
      case ROUTE_DIRECTION_AGAINST_LINE:
         roadmap_line_points (to_line->line_id, &to_tmp, to_point);
         break;
      default:
         roadmap_line_points (to_line->line_id, &to_tmp, to_point);
   }

   return 0;
}


static int navigate_find_track_points_in_scale (PluginLine *from_line, int *from_point,
                                                PluginLine *to_line, int *to_point,
                                                int *from_direction, int recalc_route, int scale) {

    int prev_scale = roadmap_square_get_screen_scale ();
    int rc;

    roadmap_square_set_screen_scale (scale);
    rc = navigate_find_track_points (from_line, from_point, to_line, to_point, from_direction, recalc_route);
    roadmap_square_set_screen_scale (prev_scale);

    return rc;
}


static void navigate_main_suspend_navigation()
{
   if( !NavigateTrackEnabled)
      return;

   NavigateTrackEnabled = 0;
   navigate_bar_set_mode( NavigateTrackEnabled);
   roadmap_navigate_end_route();
}


static int navigate_main_recalc_route () {

   int track_time;
   PluginLine from_line;
   int from_point;
   int flags;
   time_t timeNow = time(NULL);

   navigate_main_suspend_navigation ();

   NavigateNumSegments = MAX_NAV_SEGEMENTS;

   if (navigate_route_load_data () < 0) {
      return -1;
   }

   if (navigate_find_track_points_in_scale
         (&from_line, &from_point,
          &NavigateDestination, &NavigateDestPoint, NULL, 1, 0) < 0) {

      return -1;
   }

   roadmap_main_set_cursor (ROADMAP_CURSOR_WAIT);

   flags = RECALC_ROUTE;
   if (timeNow > NavigateCalcTime &&
       timeNow < NavigateCalcTime + 60) {
       flags = flags | USE_LAST_RESULTS;
   }

   navigate_cost_reset ();
   track_time =
      navigate_route_get_segments
            (&from_line, from_point, &NavigateDestination, &NavigateDestPoint,
             NavigateSegments, &NavigateNumSegments,
             &flags);

   roadmap_main_set_cursor (ROADMAP_CURSOR_NORMAL);
   roadmap_navigate_resume_route ();

   if (track_time <= 0) {
      return -1;
   }

   NavigateCalcTime = time(NULL);
   navigate_bar_initialize ();

   NavigateFlags = flags;

   navigate_instr_prepare_segments (NavigateSegments, NavigateNumSegments,
                                   &NavigateSrcPos, &NavigateDestPos);
   NavigateTrackEnabled = 1;
   navigate_bar_set_mode (NavigateTrackEnabled);
   NavigateCurrentSegment = 0;

   return 0;
}

static void navigate_main_format_messages (void) {

   int distance_to_destination;
   int distance_to_destination_far;
   int ETA;
   char str[100];
   RoadMapGpsPosition pos;

   (*NextMessageUpdate) ();

   if (!NavigateTrackEnabled) return;

   distance_to_destination = NavigateDistanceToDest + NavigateDistanceToTurn;
   ETA = NavigateETA + NavigateETAToTurn + 60;

   distance_to_destination_far =
      roadmap_math_to_trip_distance(distance_to_destination);

   if (distance_to_destination_far > 0) {
      roadmap_message_set ('D', "%d %s",
            distance_to_destination_far,
            roadmap_lang_get(roadmap_math_trip_unit()));
   } else {
      roadmap_message_set ('D', "%d %s",
            distance_to_destination,
            roadmap_lang_get(roadmap_math_distance_unit()));
   };

   sprintf (str, "%d:%02d", ETA / 3600, (ETA % 3600) / 60);
   roadmap_message_set ('T', str);

   roadmap_navigate_get_current (&pos, NULL, NULL);
   roadmap_message_set ('S', "%3d %s",
         roadmap_math_to_speed_unit(pos.speed),
         roadmap_lang_get(roadmap_math_speed_unit()));

}


static int navigate_address_cb (const RoadMapPosition *point,
                                const PluginLine      *line,
                                int                    direction,
                                address_info_ptr       ai) {

   roadmap_trip_set_point ("Destination", point);

   if( -1 == navigate_main_calc_route ())
      return -1;

   // Navigation started, send realtime message
   Realtime_ReportOnNavigation(point, ai);

   return 0;
}


/****** Route calculation progress dialog ******/
#ifdef SSD

static void show_progress_dialog (void) {

   SsdWidget dialog = ssd_dialog_activate ("Route calc", NULL);

   if (!dialog) {
      SsdWidget group;

      dialog = ssd_dialog_new ("Route calc",
            roadmap_lang_get("Route calculation"), NULL,
            SSD_CONTAINER_BORDER|SSD_CONTAINER_TITLE|SSD_DIALOG_FLOAT|
            SSD_ALIGN_CENTER|SSD_ALIGN_VCENTER|SSD_ROUNDED_CORNERS);

      group = ssd_container_new ("Progress group", NULL,
                  SSD_MIN_SIZE, SSD_MIN_SIZE, SSD_WIDGET_SPACE|SSD_END_ROW);
      ssd_widget_set_color (group, NULL, NULL);
      ssd_widget_add (group,
         ssd_text_new ("Label",
            roadmap_lang_get("Calculating route, please wait..."), -1,
                        SSD_END_ROW));
      ssd_widget_add (group,
         ssd_text_new ("Label", "%", -1, 0));
      ssd_widget_add (group, ssd_text_new ("Progress", "", -1, 0));
      ssd_widget_add (dialog, group);

      ssd_dialog_activate ("Route calc", NULL);
      ssd_dialog_draw ();
   }

   ssd_widget_set_value (dialog, "Progress", "0");

   ssd_dialog_draw ();
}

#else

static void cancel_calc (const char *name, void *data) {
}

static void show_progress_dialog (void) {

   if (roadmap_dialog_activate ("Route calc", NULL, 1)) {

      roadmap_dialog_new_label ("Calculating", "Calculating route, please wait...");
      roadmap_dialog_new_progress ("Calculating", "Progress");

      roadmap_dialog_add_button ("Cancel", cancel_calc);

      roadmap_dialog_complete (0);
   }

   roadmap_dialog_set_progress ("Calculating", "Progress", 0);

   roadmap_main_flush ();
}
#endif


static void refresh_eta (BOOL display_change) {

    /* recalculate ETA according to possibly changing traffic info */
   NavigateSegment *segment = NavigateSegments + NavigateCurrentSegment;
   int group_id = segment->group_id;
   int prev_eta = NavigateETA + NavigateETAToTurn;

    if (NavigateETA == 0) {
        prev_eta = 0;
        NavigateETADiff = 0;
    }

    navigate_instr_calc_cross_time (segment,
                                              NavigateNumSegments - NavigateCurrentSegment);

    /* ETA to end of current segment */
   NavigateETAToTurn = (int) (1.0 * segment->cross_time * NavigateDistanceToNext /
                             (segment->distance + 1));

    /* ETA to next turn */
   while (++segment < NavigateSegments + NavigateNumSegments) {
      if (segment->group_id != group_id) break;
      NavigateETAToTurn += segment->cross_time;
   }

    /* ETA from next turn to destination */
   NavigateETA = 0;
   while (segment < NavigateSegments + NavigateNumSegments) {

      NavigateETA            += segment->cross_time;
      segment++;
   }

    if (prev_eta) {
        NavigateETADiff += NavigateETA + NavigateETAToTurn - prev_eta;
    }


    if ((NavigateETADiff < -180 ||
         NavigateETADiff > 180)  && display_change){

        char msg[1000];

        if (NavigateETADiff > 0) {
            snprintf (msg, sizeof (msg), "%s %d %s.",
                         roadmap_lang_get ("Due to new traffic information, ETA is longer by"),
                         (NavigateETADiff + 30) / 60,
                         roadmap_lang_get ("minutes"));
        } else {
            snprintf (msg, sizeof (msg), "%s %d %s.",
                         roadmap_lang_get ("Due to new traffic information, ETA is shorter by"),
                         (-NavigateETADiff + 30) / 60,
                         roadmap_lang_get ("minutes"));
        }
      roadmap_messagebox_timeout ("Route information", msg, 7);
        roadmap_log (ROADMAP_DEBUG, "Major ETA change!! (%+d seconds)", NavigateETADiff);
        NavigateETADiff = 0;
    }

    NavigateETATime = time(NULL);
}


int navigate_line_in_route
          (PluginLine *line, int direction) {
   const NavigateSegment *segment = NavigateSegments + NavigateCurrentSegment;
   int count = 5;

   while (count &&
         (segment < (NavigateSegments + NavigateNumSegments - 1))) {

      if ((direction == segment->line_direction) &&
            roadmap_plugin_same_line(&segment->line, line))
         return 1;
      segment++;
      count--;
   }

   return 0;
}

void navigate_update (RoadMapPosition *position, PluginLine *current) {

   int announce = 0;
   const NavigateSegment *segment = NavigateSegments + NavigateCurrentSegment;
   const NavigateSegment *next_turn_segment;
   const NavigateSegment *prev_segment;
   int group_id = segment->group_id;
   const char *inst_text = "";
   const char *inst_voice = NULL;
   const char *inst_roundabout = NULL;
   int roundabout_exit = 0;
   RoadMapSoundList sound_list;
   const int ANNOUNCES[] = { 800, 200, 40 };
#ifdef __SYMBIAN32__
   const int ANNOUNCE_PREPARE_FACTORS[] = { 400, 400, 150 };
#else
   const int ANNOUNCE_PREPARE_FACTORS[] = { 200, 200, 100 };
#endif
   int announce_distance = 0;
   int distance_to_prev;
   int distance_to_next;
   RoadMapGpsPosition pos;


#ifdef J2ME
#define NAVIGATE_COMPENSATE 20
#else
#define NAVIGATE_COMPENSATE 10
#endif


#ifdef TEST_ROUTE_CALC
   static int test_counter;

   if ((++test_counter % 300) == 0) {
      navigate_main_test(1);
      return;
   }
#endif

   if (!NavigateTrackEnabled) return;

   if (segment->line_direction == ROUTE_DIRECTION_WITH_LINE) {

      NavigateDistanceToNext =
         navigate_instr_calc_length (position, segment, LINE_END);
   } else {

      NavigateDistanceToNext =
         navigate_instr_calc_length (position, segment, LINE_START);
   }

   distance_to_prev = segment->distance - NavigateDistanceToNext;
   for (prev_segment = segment - 1;
      prev_segment >= NavigateSegments && prev_segment->group_id == segment->group_id;
      prev_segment--) {

    distance_to_prev += prev_segment->distance;
   }

   NavigateETAToTurn = (int) (1.0 * segment->cross_time * NavigateDistanceToNext /
                             (segment->distance + 1));

    NavigateDistanceToTurn = NavigateDistanceToNext;
   while (segment < (NavigateSegments + NavigateNumSegments - 1)) {
      if ((segment+1)->group_id != group_id) break;
      segment++;
      NavigateDistanceToTurn += segment->distance;
      NavigateETAToTurn += segment->cross_time;
   }
   if (NavigateETATime + 60 <= time(NULL)) {
    refresh_eta (TRUE);
   }


   distance_to_next = 0;

   if (segment <  (NavigateSegments + NavigateNumSegments - 1)) {
      next_turn_segment = segment + 1;
      group_id = next_turn_segment->group_id;
      distance_to_next = next_turn_segment->distance;
      while (next_turn_segment < (NavigateSegments + NavigateNumSegments - 1)) {
         if ((next_turn_segment+1)->group_id != group_id) break;
         next_turn_segment++;
         distance_to_next += next_turn_segment->distance;
      }
   }

   if (roadmap_config_match(&NavigateConfigAutoZoom, "yes")) {
      const char *focus = roadmap_trip_get_focus_name ();

      if (focus && !strcmp (focus, "GPS")) {

         navigate_zoom_update (NavigateDistanceToTurn,
                               distance_to_prev,
                               distance_to_next);
      }
   }

   navigate_bar_set_distance (NavigateDistanceToTurn);

   switch (segment->instruction) {

      case TURN_LEFT:
         inst_text = "Turn left";
         inst_voice = "TurnLeft";
         break;
      case ROUNDABOUT_LEFT:
         inst_text = "At the roundabout, turn left";
         inst_roundabout = "Roundabout";
         inst_voice = "TurnLeft";
         break;
      case KEEP_LEFT:
         inst_text = "Keep left";
         inst_voice = "KeepLeft";
         break;
      case TURN_RIGHT:
         inst_text = "Turn right";
         inst_voice = "TurnRight";
         break;
      case ROUNDABOUT_RIGHT:
         inst_text = "At the roundabout, turn right";
         inst_roundabout = "Roundabout";
         inst_voice = "TurnRight";
         break;
      case KEEP_RIGHT:
         inst_text = "Keep right";
         inst_voice = "KeepRight";
         break;
      case APPROACHING_DESTINATION:
         inst_text = "Approaching destination";
         break;
      case CONTINUE:
         inst_text = "continue straight";
         inst_voice = "Straight";
         break;
      case ROUNDABOUT_STRAIGHT:
         inst_text = "At the roundabout, continue straight";
         inst_roundabout = "Roundabout";
         inst_voice = "Straight";
         break;
      case ROUNDABOUT_ENTER:
         inst_text = "At the roundabout, exit";
         inst_roundabout = "Roundabout";
         inst_voice = "Exit";
         roundabout_exit = segment->exit_no;
         break;
      default:
         break;
   }

   roadmap_navigate_get_current (&pos, NULL, NULL);

   if ((segment->instruction == APPROACHING_DESTINATION) &&
        NavigateDistanceToTurn <= 20 + pos.speed * 1) {

      sound_list = roadmap_sound_list_create (0);
      roadmap_sound_list_add (sound_list, "Arrive");
      if (roadmap_config_match(&NavigateConfigNavigationGuidance, "yes")) {
        roadmap_sound_play_list (sound_list);
      }

      if (roadmap_config_match(&NavigateConfigAutoZoom, "yes")) {
         const char *focus = roadmap_trip_get_focus_name ();
         /* We used auto zoom, so now we need to reset it */

         if (focus && !strcmp (focus, "GPS")) {

            roadmap_screen_zoom_reset ();
         }
      }

      navigate_main_stop_navigation ();
      return;
   }

   roadmap_message_set ('I', inst_text);

   if (NavigateNextAnnounce == -1) {

      unsigned int i;

      for (i=0; i<sizeof(ANNOUNCES)/sizeof(ANNOUNCES[0]) - 1; i++) {

         if (NavigateDistanceToTurn > ANNOUNCES[i]) {
            NavigateNextAnnounce = i + 1;
            break;
         }
      }

      if (NavigateNextAnnounce == -1) {
         NavigateNextAnnounce = sizeof(ANNOUNCES)/sizeof(ANNOUNCES[0]);
      }
   }

   if (NavigateNextAnnounce > 0 &&
      (NavigateDistanceToTurn <=
        (ANNOUNCES[NavigateNextAnnounce - 1] + pos.speed * ANNOUNCE_PREPARE_FACTORS[NavigateNextAnnounce - 1] / 100))) {
      unsigned int i;

      announce_distance = ANNOUNCES[NavigateNextAnnounce - 1];
      NavigateNextAnnounce = 0;

      if (inst_voice) {
         announce = 1;
      }

      for (i=0; i<sizeof(ANNOUNCES)/sizeof(ANNOUNCES[0]); i++) {
         if ((ANNOUNCES[i] < announce_distance) &&
             (NavigateDistanceToTurn > ANNOUNCES[i])) {
            NavigateNextAnnounce = i + 1;
            break;
         }
      }
   }

   if (announce) {
      PluginStreetProperties properties;

      if (segment < (NavigateSegments + NavigateNumSegments - 1)) {
         segment++;
      }

      roadmap_plugin_get_street_properties (&segment->line, &properties, 0);

      roadmap_message_set ('#', properties.address);
      roadmap_message_set ('N', properties.street);
      //roadmap_message_set ('T', properties.street_t2s);
      roadmap_message_set ('C', properties.city);

      if (roadmap_config_match(&NavigateConfigNavigationGuidance, "yes")) {
        sound_list = roadmap_sound_list_create (0);
        if (!NavigateNextAnnounce) {
            roadmap_message_unset ('w');
        } else {

                char distance_str[100];
            int distance_far =
                roadmap_math_to_trip_distance(announce_distance);

            roadmap_sound_list_add (sound_list, "within");

             if (distance_far > 0) {
                    roadmap_message_set ('w', "%d %s",
                      distance_far, roadmap_math_trip_unit());

                sprintf(distance_str, "%d", distance_far);
                roadmap_sound_list_add (sound_list, distance_str);
                    roadmap_sound_list_add (sound_list, roadmap_math_trip_unit());
                } else {
                roadmap_message_set ('w', "%d %s",
                      announce_distance, roadmap_math_distance_unit());

                sprintf(distance_str, "%d", announce_distance);
                roadmap_sound_list_add (sound_list, distance_str);
                roadmap_sound_list_add (sound_list, roadmap_math_distance_unit());
                };
        }

            if (inst_roundabout) {
            roadmap_sound_list_add (sound_list, inst_roundabout);
            }
        roadmap_sound_list_add (sound_list, inst_voice);
        if (inst_roundabout) {
            if (roundabout_exit > 0 && roundabout_exit <= MaxExitName) {
                roadmap_sound_list_add (sound_list, ExitName[roundabout_exit - 1]);
            } else  if (roundabout_exit == -1) {
                roadmap_sound_list_add (sound_list, "Marked");
            }
        }
        //roadmap_voice_announce ("Driving Instruction");

        roadmap_sound_play_list (sound_list);
      }
   }

}

void navigate_main_stop_navigation(void)
{
   if( !NavigateTrackEnabled)
      return;

   navigate_main_suspend_navigation ();

   roadmap_config_set_integer (&NavigateConfigNavigating, 0);
   roadmap_config_save(1);
}

void navigate_main_stop_navigation_menu(void)
{
    navigate_main_stop_navigation();
    ssd_dialog_hide_all(dec_close);
}
void navigate_get_next_line
          (PluginLine *current, int direction, PluginLine *next) {

   int new_instruction = 0;

   if (!NavigateTrackEnabled) {

      if (navigate_main_recalc_route () != -1) {

         roadmap_trip_stop ();
      }

      return;
   }

   /* Ugly hack as we don't support navigation through editor lines */
   if (roadmap_plugin_get_id (current) != ROADMAP_PLUGIN_ID) {
      *next = NavigateSegments[NavigateCurrentSegment+1].line;
      return;
   }

   if (!roadmap_plugin_same_line
         (current, &NavigateSegments[NavigateCurrentSegment].line)) {

      int i;
      for (i=NavigateCurrentSegment+1; i < NavigateNumSegments; i++) {

         if (roadmap_plugin_same_line
            (current, &NavigateSegments[i].line)) {

            if (NavigateSegments[NavigateCurrentSegment].group_id !=
                  NavigateSegments[i].group_id) {

               new_instruction = 1;
            }

            NavigateCurrentSegment = i;
            NavigateFlags &= ~CHANGED_DEPARTURE;
            break;
         }
      }
   }

   if ((NavigateCurrentSegment < NavigateNumSegments) &&
       !roadmap_plugin_same_line
         (current, &NavigateSegments[NavigateCurrentSegment].line) &&
       !(NavigateFlags & CHANGED_DEPARTURE)) {

      NavigateNextAnnounce = -1;

      if (navigate_main_recalc_route () == -1) {

         roadmap_trip_start ();
         return;
      }
   }

   if ((NavigateCurrentSegment+1) >= NavigateNumSegments) {

      next->plugin_id = INVALID_PLUGIN_ID;
   } else {

      *next = NavigateSegments[NavigateCurrentSegment+1].line;
   }

   if (new_instruction || !NavigateCurrentSegment) {
      int group_id;

      /* new driving instruction */

      PluginStreetProperties properties;
      const NavigateSegment *segment =
               NavigateSegments + NavigateCurrentSegment;

      while (segment < NavigateSegments + NavigateNumSegments - 1) {
         if ((segment + 1)->group_id != segment->group_id) break;
         segment++;
      }

      navigate_bar_set_instruction (segment->instruction);
      if (segment->instruction == ROUNDABOUT_ENTER ||
         segment->instruction == ROUNDABOUT_EXIT) {
        navigate_bar_set_exit (segment->exit_no);
      }

      group_id = segment->group_id;
      if (segment < NavigateSegments + NavigateNumSegments - 1) {
         /* we need the name of the next street */
         segment++;
      }
      while (segment < (NavigateSegments + NavigateNumSegments - 1) &&
             segment->context == SEG_ROUNDABOUT) {
        /* skip roundabout segments for street name */
        segment++;
      }
      roadmap_plugin_get_street_properties (&segment->line, &properties, 0);
      navigate_bar_set_street (properties.street);

      NavigateNextAnnounce = -1;

      NavigateDistanceToDest = 0;
      NavigateETA = 0;
      NavigateETADiff = 0;

      if (segment->group_id != group_id) {

         /* Update distance to destination and ETA
          * excluding current group (computed in navigate_update)
          */

         while (segment < NavigateSegments + NavigateNumSegments) {

            NavigateDistanceToDest += segment->distance;
            NavigateETA            += segment->cross_time;
            segment++;
         }
         NavigateETATime = time(NULL);
      }
   }

   return;
}


int navigate_is_enabled (void) {
   return NavigateEnabled;
}

int navigate_track_enabled(void){
    return NavigateTrackEnabled;
}


int navigate_is_line_on_route(int square_id, int line_id, int from_line, int to_line){

   int i;
   int line_from_point;
   int line_to_point;

   if (!NavigateTrackEnabled)
      return 0;

   for (i=NavigateCurrentSegment+1; i < NavigateNumSegments; i++) {
      if (NavigateSegments[i].line.square == square_id &&
            NavigateSegments[i].line.line_id == line_id) {

         if (from_line == -1 && to_line == -1)
            return 1;

         roadmap_square_set_current (square_id);
         if (NavigateSegments[i].line_direction == ROUTE_DIRECTION_WITH_LINE)
            roadmap_line_points (line_id, &line_from_point, &line_to_point);
         else
            roadmap_line_points (line_id, &line_to_point, &line_from_point);
         if ((line_from_point == from_line) && (line_to_point ==to_line))
            return 1;
      }
   }

   return 0;

}

void navigate_get_waypoint (int distance, RoadMapPosition *way_point) {
   NavigateSegment *segment = NavigateSegments + NavigateCurrentSegment;

   assert(NavigateTrackEnabled);

   if (distance == -1) {
      *way_point = NavigateDestPos;
      return;
   }

   distance -= NavigateDistanceToNext;

   while ((distance > 0) &&
      (++segment < NavigateSegments + NavigateNumSegments)) {
      distance -= segment->distance;
   }

   if (distance > 0) segment--;

   if (segment->line_direction == ROUTE_DIRECTION_WITH_LINE) {
      *way_point = segment->to_pos;
   } else {
      *way_point = segment->from_pos;
   }
}

static void navigate_main_init_pens (void) {

   RoadMapPen pen;

   pen = roadmap_canvas_create_pen ("NavigatePen1");
   roadmap_canvas_set_foreground
      (roadmap_config_get (&NavigateConfigRouteColor));
   roadmap_canvas_set_thickness (ROUTE_PEN_WIDTH);
   NavigatePen[0] = pen;

   pen = roadmap_canvas_create_pen ("NavigatePen2");
   roadmap_canvas_set_foreground
      (roadmap_config_get (&NavigateConfigRouteColor));
   roadmap_canvas_set_thickness (ROUTE_PEN_WIDTH);
   NavigatePen[1] = pen;

   pen = roadmap_canvas_create_pen ("NavigatePenEst1");
   roadmap_canvas_set_foreground
      (roadmap_config_get (&NavigateConfigPossibleRouteColor));
   roadmap_canvas_set_thickness (ROUTE_PEN_WIDTH);
   NavigatePenEst[0] = pen;

   pen = roadmap_canvas_create_pen ("NavigatePenEst2");
   roadmap_canvas_set_foreground
      (roadmap_config_get (&NavigateConfigPossibleRouteColor));
   roadmap_canvas_set_thickness (ROUTE_PEN_WIDTH);
   NavigatePenEst[1] = pen;
}

void navigate_main_shutdown (void) {
#ifdef IPHONE
   if (!roadmap_main_is_in_background()) //should we keep the navigation state?
#endif //IPHONE
   roadmap_config_set_integer (&NavigateConfigNavigating, 0);
}


void toggle_navigation_guidance(){
    if (roadmap_config_match(&NavigateConfigNavigationGuidance, "yes")){
        ssd_bitmap_splash("splash_sound_off", 1);
        roadmap_config_set(&NavigateConfigNavigationGuidance, "no");
    }else{
        ssd_bitmap_splash("splash_sound_on", 1);
        roadmap_config_set(&NavigateConfigNavigationGuidance, "yes");
    }
}

int navigation_guidance_state(){
        if (roadmap_config_match(&NavigateConfigNavigationGuidance, "yes"))
            return 1;
        else
            return 0;
}


void navigate_main_initialize (void) {

   roadmap_config_declare
      ("schema", &NavigateConfigRouteColor,  "#b3a1f6a0", NULL);
   roadmap_config_declare
      ("schema", &NavigateConfigPossibleRouteColor,  "#ff0000a0", NULL);
   roadmap_config_declare_enumeration
      ("user", &NavigateConfigAutoZoom, NULL, "yes", "no", NULL);
   roadmap_config_declare_enumeration
      ("user", &NavigateConfigNavigationGuidance, NULL, "yes", "no", NULL);

   roadmap_config_declare
      ("session",  &NavigateConfigLastPos, "0, 0", NULL);
   roadmap_config_declare
      ("session",  &NavigateConfigNavigating, "0", NULL);

   navigate_main_init_pens ();

   navigate_cost_initialize ();

   NavigatePluginID = navigate_plugin_register ();
   navigate_traffic_initialize ();

   navigate_main_set (1);

   NextMessageUpdate =
      roadmap_message_register (navigate_main_format_messages);

   roadmap_address_register_nav (navigate_address_cb);
   roadmap_search_register_nav(navigate_address_cb);
   roadmap_skin_register (navigate_main_init_pens);

   if (roadmap_config_get_integer (&NavigateConfigNavigating)) {
      RoadMapPosition pos;
      roadmap_config_get_position (&NavigateConfigLastPos, &pos);
      roadmap_trip_set_focus ("GPS");
      roadmap_trip_set_point ("Destination", &pos);

      navigate_main_calc_route ();
   }
}


void navigate_main_set (int status) {

   if (status && NavigateEnabled) {
      return;
   } else if (!status && !NavigateEnabled) {
      return;
   }

   NavigateEnabled = status;
}


#ifdef TEST_ROUTE_CALC
#include "roadmap_shape.h"
int navigate_main_test (int test_count) {

   int track_time;
   PluginLine from_line;
   int from_point;
   int line;
   int lines_count = roadmap_line_count();
   RoadMapPosition pos;
   int flags;
   static int itr = 0;
   const char *focus = roadmap_trip_get_focus_name ();

   if (!itr) {
      srand(0);
   }

   NavigateTrackFollowGPS = focus && !strcmp (focus, "GPS");

   if (navigate_route_load_data () < 0) {

      roadmap_messagebox("Error", "Error loading navigation data.");
      return -1;
   }

   if (test_count) test_count++;

   while (1) {
      int first_shape, last_shape;

      printf ("Iteration: %d\n", itr++);
         if (test_count) {
            test_count--;
            if (!test_count) break;
         }

      line = (int) (lines_count * (rand() / (RAND_MAX + 1.0)));
      roadmap_line_from (line, &pos);
      roadmap_line_shapes (line, -1, &first_shape, &last_shape);
      if (first_shape != -1) {
         last_shape = (first_shape + last_shape) / 2;
         roadmap_shape_get_position (first_shape, &pos);
         while (first_shape != last_shape) {
            roadmap_shape_get_position (++first_shape, &pos);
         }
      }

      if (!NavigateTrackFollowGPS) {
         roadmap_trip_set_point ("Departure", &pos);
      }

      line = (int) (lines_count * (rand() / (RAND_MAX + 1.0)));
      roadmap_line_from (line, &pos);
      roadmap_line_shapes (line, -1, &first_shape, &last_shape);
      if (first_shape != -1) {
         last_shape = (first_shape + last_shape) / 2;
         roadmap_shape_get_position (first_shape, &pos);
         while (first_shape != last_shape) {
            roadmap_shape_get_position (++first_shape, &pos);
         }
      }
      roadmap_trip_set_point ("Destination", &pos);

      NavigateDestination.plugin_id = INVALID_PLUGIN_ID;
      navigate_main_suspend_navigation ();

      NavigateNumSegments = MAX_NAV_SEGEMENTS;

      if (navigate_find_track_points_in_scale
            (&from_line, &from_point, &NavigateDestination, &NavigateDestPoint, 1, 0)) {

         printf ("Error finding navigate points.\n");
         continue;
      }

      flags = NEW_ROUTE|RECALC_ROUTE;
      navigate_cost_reset ();
      track_time =
         navigate_route_get_segments
         (&from_line, from_point, &NavigateDestination, NavigateDestPoint,
          NavigateSegments, &NavigateNumSegments,
          &flags);

      if (track_time <= 0) {
         navigate_main_suspend_navigation ();
         if (track_time < 0) {
            printf("Error calculating route.\n");
         } else {
            printf("Error - Can't find a route.\n");
         }

         continue;
      } else {
         char msg[200] = {0};
         int i;
         int length = 0;

         NavigateCalcTime = time(NULL);

         navigate_instr_prepare_segments (NavigateSegments, NavigateNumSegments,
               &NavigateSrcPos, &NavigateDestPos);

         track_time = 0;
         for (i=0; i<NavigateNumSegments; i++) {
            length += NavigateSegments[i].distance;
            track_time += NavigateSegments[i].cross_time;
         }

         NavigateFlags = flags;

         if (flags & GRAPH_IGNORE_TURNS) {
            snprintf(msg, sizeof(msg), "%s\n",
                  roadmap_lang_get ("The calculated route may have incorrect turn instructions."));
         }

         if (flags & CHANGED_DESTINATION) {
            snprintf(msg + strlen(msg), sizeof(msg) - strlen(msg), "%s\n",
                  roadmap_lang_get ("Showing route to nearest reachable destination."));
         }

         if (flags & CHANGED_DEPARTURE) {
            snprintf(msg + strlen(msg), sizeof(msg) - strlen(msg), "%s\n",
                  roadmap_lang_get ("Showing route using alternative departure point."));
         }

         snprintf(msg + strlen(msg), sizeof(msg) - strlen(msg),
               "%s: %.1f %s\n%s: %.1f %s",
               roadmap_lang_get ("Length"),
               length/1000.0,
               roadmap_lang_get ("Km"),
               roadmap_lang_get ("Time"),
               track_time/60.0,
               roadmap_lang_get ("minutes"));

         NavigateTrackEnabled = 1;
           NavigateAnnounceSegment = -1;
         navigate_bar_set_mode (NavigateTrackEnabled);

         roadmap_screen_redraw ();
         printf ("Route found!\n%s\n\n", msg);

      }
   }

   return 0;
}
#endif

int navigate_main_calc_route () {

   int track_time;
   PluginLine from_line;
   PluginLine next_line;
   int from_point;
   int flags;
   int from_direction;
   int gps_state;
   BOOL gps_active;

   const char *focus = roadmap_trip_get_focus_name ();

#ifdef TEST_ROUTE_CALC_STRESS
   navigate_bar_initialize ();
   navigate_main_test(0);
#endif

   NavigateTrackFollowGPS = focus && !strcmp (focus, "GPS");

   if (NavigateTrackFollowGPS) {
      if (roadmap_trip_get_position ("Departure")) {
         roadmap_trip_remove_point ("Departure");
      }
   }

   if (!NavigateTrackFollowGPS && !roadmap_trip_get_position ("Departure")) {
      NavigateTrackFollowGPS = 1;
   }

   NavigateDestination.plugin_id = INVALID_PLUGIN_ID;
   navigate_main_suspend_navigation ();

   NavigateNumSegments = MAX_NAV_SEGEMENTS;

   if (navigate_route_load_data () < 0) {

      roadmap_messagebox("Error", "Error loading navigation data.");
      return -1;
   }

   if (navigate_find_track_points_in_scale
         (&from_line, &from_point, &NavigateDestination, &NavigateDestPoint, &from_direction, 0, 0)) {

      return -1;
   }

   flags = NEW_ROUTE | ALLOW_DESTINATION_CHANGE | ALLOW_ALTERNATE_SOURCE;

   show_progress_dialog ();

   navigate_cost_reset ();
   track_time =
      navigate_route_get_segments
            (&from_line, from_point, &NavigateDestination, &NavigateDestPoint,
             NavigateSegments, &NavigateNumSegments,
             &flags);

   if (track_time <= 0) {
#ifdef SSD
      ssd_dialog_hide ("Route calc", dec_close);
#else
      roadmap_dialog_hide ("Route calc");
#endif
      if (track_time < 0) {
         roadmap_messagebox("Error", "Error calculating route.");
      } else {
         roadmap_messagebox("Error", "Can't find a route.");
      }

      return -1;
   } else {
      char msg[200] = {0};
      int i;
      int length = 0;
      int trip_distance;

      NavigateCalcTime = time(NULL);
      navigate_bar_initialize ();
      navigate_instr_prepare_segments (NavigateSegments, NavigateNumSegments,
                                      &NavigateSrcPos, &NavigateDestPos);

      track_time = 0;
      for (i=0; i<NavigateNumSegments; i++) {
         length += NavigateSegments[i].distance;
         track_time += NavigateSegments[i].cross_time;
      }

#ifdef SSD
      ssd_dialog_hide ("Route calc", dec_close);
#else
      roadmap_dialog_hide ("Route calc");
#endif
      NavigateFlags = flags;

      if (flags & GRAPH_IGNORE_TURNS) {
         snprintf(msg, sizeof(msg), "%s\n",
            roadmap_lang_get ("The calculated route may have incorrect turn instructions."));
      }
      if (flags & CHANGED_DESTINATION) {
         snprintf(msg + strlen(msg), sizeof(msg) - strlen(msg), "%s\n",
               roadmap_lang_get ("Showing route to nearest reachable destination."));
      }
      if (flags & CHANGED_DEPARTURE) {
         snprintf(msg + strlen(msg), sizeof(msg) - strlen(msg), "%s\n",
               roadmap_lang_get ("Showing route using alternative departure point."));
      }

      trip_distance = roadmap_math_to_trip_distance_tenths(length);
      snprintf(msg + strlen(msg), sizeof(msg) - strlen(msg),
            "%s: %.1f %s,\n%s: %.1f %s",
            roadmap_lang_get ("Length"),
            trip_distance/10.0,
            roadmap_lang_get(roadmap_math_trip_unit()),
            roadmap_lang_get ("Time"),
            track_time/60.0,
            roadmap_lang_get ("minutes"));

      NavigateTrackEnabled = 1;
      navigate_bar_set_mode (NavigateTrackEnabled);

        navigate_get_next_line (&from_line, from_direction, &next_line);
        navigate_update (&NavigateSrcPos, &from_line);
      if (NavigateTrackFollowGPS) {
         NavigateCurrentSegment = 0;

         roadmap_trip_stop ();
         roadmap_navigate_route (NavigateCallbacks);
      }

      gps_state = roadmap_gps_reception_state();
      gps_active = (gps_state != GPS_RECEPTION_NA) && (gps_state != GPS_RECEPTION_NONE);

      if (!gps_active) {
         NavigateDistanceToDest = length;
      } else {
         NavigateDistanceToDest = length - NavigateDistanceToTurn ;
      }
      NavigateDistanceToTurn = 0;
      NavigateDistanceToNext = 0;
      NavigateETAToTurn = 0;
      refresh_eta(FALSE);
      navigate_main_format_messages();
      //navigate_bar_set_time_to_destination();
      roadmap_screen_redraw ();
      roadmap_config_set_position (&NavigateConfigLastPos, &NavigateDestPos);
      roadmap_config_set_integer (&NavigateConfigNavigating, 1);
      roadmap_config_save (0);

      roadmap_messagebox_timeout ("Route found", msg, 5);
      focus_on_me();
   }

   return 0;
}


void navigate_main_screen_repaint (int max_pen) {
   int i;
   int current_width = -1;
   int last_cfcc = -1;
   RoadMapPen *pens;
   int current_pen = 0;
   int pen_used = 0;

   if (!NavigateTrackEnabled) return;

   if ((NavigateFlags & GRAPH_IGNORE_TURNS) ||
     (NavigateFlags & CHANGED_DESTINATION)) {
      pens = NavigatePenEst;
   } else {
      pens = NavigatePen;
   }

   if (!NavigateTrackFollowGPS && roadmap_trip_get_focus_name () &&
         !strcmp (roadmap_trip_get_focus_name (), "GPS")) {

      NavigateTrackFollowGPS = 1;

      roadmap_trip_stop ();

      if (roadmap_trip_get_position ("Departure")) {
         roadmap_trip_remove_point ("Departure");
      }
      roadmap_navigate_route (NavigateCallbacks);
   }

   for (i=0; i<NavigateNumSegments; i++) {

      NavigateSegment *segment = NavigateSegments + i;

      if (segment->line.cfcc != last_cfcc) {
         RoadMapPen layer_pen =
               roadmap_layer_get_pen (segment->line.cfcc, 0, 0);
         int width;

         if (layer_pen) width = roadmap_canvas_get_thickness (layer_pen) - 2;
         else width = ROUTE_PEN_WIDTH;

         if (width < ROUTE_PEN_WIDTH) {
            width = ROUTE_PEN_WIDTH;
         }

         if (width != current_width) {

            RoadMapPen previous_pen;
            if (pen_used) {
                current_pen = (current_pen+1)%2;
            }
            pen_used = 0;
            previous_pen = roadmap_canvas_select_pen (pens[current_pen]);
            roadmap_canvas_set_thickness (width);
            current_width = width;

            if (previous_pen) {
               roadmap_canvas_select_pen (previous_pen);
            }
         }

         last_cfcc = segment->line.cfcc;
      }

      roadmap_square_set_current (segment->line.square);
      pen_used |=
           roadmap_screen_draw_one_line (&segment->from_pos,
                                         &segment->to_pos,
                                         0,
                                         &segment->shape_initial_pos,
                                         segment->first_shape,
                                         segment->last_shape,
                                         segment->shape_itr,
                                         pens + current_pen,
                                         1,
                                         -1,
                                         NULL,
                                         NULL,
                                         NULL);
   }
}


int navigate_main_reload_data (void) {

   navigate_traffic_refresh ();
   return navigate_route_reload_data ();
}

int navigate_is_auto_zoom (void) {
   return (roadmap_config_match(&NavigateConfigAutoZoom, "yes"));
}

///////////////////////////////////////////////////
// Navigation List
//////////////////////////////////////////////////
#define MAX_NAV_LIST_ENTRIES    100
#define NAVIGATION_POP_UP_NAME  "Navigation list Pop Up"

typedef struct navigate_list_value_st{
    const char      *str;
    const char      *icon;
    int             inst_num;
    RoadMapPosition position;
}navigate_list_value;

static char *navigate_list_labels[MAX_NAV_LIST_ENTRIES] ;
static navigate_list_value *navigate_list_values[MAX_NAV_LIST_ENTRIES] ;
static const char *navigate_list_icons[MAX_NAV_LIST_ENTRIES];
static navigate_list_value *current_displayed_value;
static int navigate_list_count;


///////////////////////////////////////////////////
//
//////////////////////////////////////////////////
int navigate_main_list_state(void){
   int sign_active = roadmap_display_is_sign_active(NAVIGATION_POP_UP_NAME);

   if (sign_active)
        return 0;
   else
       return -1;
}

///////////////////////////////////////////////////
//
//////////////////////////////////////////////////
int navigate_main_is_list_displaying(void){
    if (navigate_main_list_state() == 0)
        return TRUE;
    else
        return FALSE;
}

///////////////////////////////////////////////////
//
//////////////////////////////////////////////////
static void navigate_main_list_hide_directions_pop_up(void){
    roadmap_display_hide(NAVIGATION_POP_UP_NAME);
    roadmap_softkeys_remove_right_soft_key("Hide_Directions");
}

///////////////////////////////////////////////////
//
//////////////////////////////////////////////////
static void navigate_main_list_set_right_softkey(void){
    static Softkey s;
    strcpy(s.text, "Hide");
    s.callback = navigate_main_list_hide_directions_pop_up;
    roadmap_softkeys_set_right_soft_key("Hide_Directions", &s);
}

///////////////////////////////////////////////////
//
//////////////////////////////////////////////////
static void display_pop_up(navigate_list_value *list_value){
    roadmap_screen_hold ();
    roadmap_display_pop_up(NAVIGATION_POP_UP_NAME,
            list_value->icon , &list_value->position, list_value->str);
    roadmap_trip_set_point("Hold", &list_value->position);
    roadmap_screen_update_center(&list_value->position);
    roadmap_math_set_scale(300, roadmap_screen_height() / 3);
    roadmap_layer_adjust();
    roadmap_view_auto_zoom_suspend();
    ssd_dialog_hide_all(dec_ok);
    navigate_main_list_set_right_softkey();
}

///////////////////////////////////////////////////
//
//////////////////////////////////////////////////
static int navigate_main_list_call_back (SsdWidget widget, const char *new_value, const void *value, void *context) {
    navigate_list_value *list_value = (navigate_list_value *)value;

    current_displayed_value = list_value;
    display_pop_up(list_value);
    return 0;
}

///////////////////////////////////////////////////
//
//////////////////////////////////////////////////
void navigate_main_list_display_next(void){
    navigate_list_value *list_value = current_displayed_value;
    if (list_value->inst_num == (navigate_list_count-1))
        return;

    current_displayed_value = navigate_list_values[list_value->inst_num+1];
    list_value = current_displayed_value;
    display_pop_up(list_value);
}

///////////////////////////////////////////////////
//
//////////////////////////////////////////////////
void navigate_main_list_display_previous(void){
    navigate_list_value *list_value = current_displayed_value;
    if (list_value->inst_num == 0)
        return;

    current_displayed_value = navigate_list_values[list_value->inst_num-1];
    list_value = current_displayed_value;
    display_pop_up(list_value);
}

///////////////////////////////////////////////////
//
//////////////////////////////////////////////////
void navigate_main_list(void){
    int i;
    int count = 0;
    NavigateSegment *segment = NavigateSegments  + NavigateCurrentSegment;
    const char *inst_text = "";
    int roundabout_exit;
    int distance_to_next =0;
    int group_id = 0;

    for (i=0; i<MAX_NAV_LIST_ENTRIES;i++){
        navigate_list_labels[i] = NULL;
        navigate_list_values[i] = NULL;
        navigate_list_icons[i] = NULL;
    }


    if (NavigateTrackEnabled){
        PluginStreetProperties properties;
        NavigateSegment *Nextsegment;
        int total_instructions = 0;
        while (segment < NavigateSegments + NavigateNumSegments) {
           if (segment->context == SEG_ROUNDABOUT){
                segment++;
                continue;
           }
           Nextsegment = segment;
           Nextsegment++;
           if (segment->group_id != Nextsegment->group_id)
                total_instructions++;
           segment++;
        }

        segment = NavigateSegments  + NavigateCurrentSegment;

        while (segment < NavigateSegments + NavigateNumSegments) {


           distance_to_next += segment->distance;

           if (segment->context == SEG_ROUNDABOUT){
                segment++;
                continue;
           }

           switch (segment->instruction) {
              case TURN_LEFT:
                 inst_text = "Turn left";
                 break;
              case ROUNDABOUT_LEFT:
                 inst_text = "At the roundabout, turn left";
                 break;
              case KEEP_LEFT:
                 inst_text = "Keep left";
                 break;
              case TURN_RIGHT:
                 inst_text = "Turn right";
                 break;
              case ROUNDABOUT_RIGHT:
                 inst_text = "At the roundabout, turn right";
                 break;
              case KEEP_RIGHT:
                 inst_text = "Keep right";
                 break;
              case APPROACHING_DESTINATION:
                 inst_text = "Approaching destination";
                 break;
              case CONTINUE:
                 inst_text = "continue straight";
                 break;
              case ROUNDABOUT_STRAIGHT:
                 inst_text = "At the roundabout, continue straight";
                 break;
              case ROUNDABOUT_ENTER:
                 inst_text = "At the roundabout, exit number";
                 roundabout_exit = segment->exit_no;
                 break;
              case ROUNDABOUT_EXIT:
                 inst_text = "At the roundabout, exit";
                 break;
              case ROUNDABOUT_U:
                 inst_text = "At the roundabout, make a u turn";
                 break;
              default:
                 break;
           }

           roadmap_plugin_get_street_properties (&segment->line, &properties, 0);
           Nextsegment = segment;
           Nextsegment++;
           if (segment->group_id != Nextsegment->group_id){
            char str[100];
            char dist_str[100];
            char unit_str[20];
            int distance_far;
            int instr;
            if (count == 0 && (NavigateDistanceToNext != 0))
                distance_to_next = NavigateDistanceToTurn;
                distance_far = roadmap_math_to_trip_distance(distance_to_next);
                if (distance_far > 0)
                {
                    int tenths = roadmap_math_to_trip_distance_tenths(distance_to_next);
                    snprintf(dist_str, sizeof(str), "%d.%d", distance_far, tenths % 10);
                    snprintf(unit_str, sizeof(unit_str), "%s", roadmap_lang_get(roadmap_math_trip_unit()));
                }
                else
                {
                    snprintf(dist_str, sizeof(str), "%d", distance_to_next);
                    snprintf(unit_str, sizeof(unit_str), "%s", roadmap_lang_get(roadmap_math_distance_unit()));
                }

                if (Nextsegment < NavigateSegments + NavigateNumSegments){

                    instr = segment->instruction;
                    if ((segment->instruction >= ROUNDABOUT_ENTER) && (segment->instruction <= ROUNDABOUT_U))
                        Nextsegment++;

                    roadmap_plugin_get_street_properties (&Nextsegment->line, &properties, 0);

                    if (instr == ROUNDABOUT_ENTER )
                        sprintf(str, " (%d/%d) %s %s %s%d",count+1,total_instructions, dist_str, unit_str, roadmap_lang_get(inst_text), roundabout_exit);
                    else if (instr == ROUNDABOUT_U )
                        sprintf(str, " (%d/%d) %s %s %s",count+1, total_instructions, dist_str, unit_str, roadmap_lang_get(inst_text));

                    else
                        if (properties.street[0] != 0)
                            sprintf(str, " (%d/%d) %s %s %s %s%s",count+1, total_instructions, dist_str, unit_str, roadmap_lang_get(inst_text), roadmap_lang_get("to"), properties.street);
                        else
                            sprintf(str, " (%d/%d) %s %s %s",count+1, total_instructions, dist_str, unit_str, roadmap_lang_get(inst_text));

                    navigate_list_labels[count] = strdup(str);
                    navigate_list_values[count] = (navigate_list_value *) malloc(sizeof(navigate_list_value));
                    navigate_list_values[count]->str = strdup(str);
                    navigate_list_values[count]->inst_num = count;
                    navigate_list_values[count]->icon = NAVIGATE_DIR_IMG[instr];
                    if (Nextsegment->line_direction == ROUTE_DIRECTION_WITH_LINE){
                        navigate_list_values[count]->position.longitude = Nextsegment->from_pos.longitude;
                        navigate_list_values[count]->position.latitude = Nextsegment->from_pos.latitude;
                    }
                    else{
                        navigate_list_values[count]->position.longitude = Nextsegment->to_pos.longitude;
                        navigate_list_values[count]->position.latitude = Nextsegment->to_pos.latitude;
                    }
                    navigate_list_icons[count] = NAVIGATE_DIR_IMG[instr];
                    count++;
                }
                else{
                    instr = segment->instruction;
                    sprintf(str, " (%d/%d) %s %s %s %s", count+1, total_instructions, roadmap_lang_get("Continue"), dist_str, unit_str, roadmap_lang_get(inst_text));
                    navigate_list_labels[count] = strdup(str);
                    navigate_list_values[count] = (navigate_list_value *) malloc(sizeof(navigate_list_value));
                    navigate_list_values[count]->str = strdup(str);
                    navigate_list_values[count]->inst_num = count;
                    navigate_list_values[count]->icon = NAVIGATE_DIR_IMG[instr];
                    if (Nextsegment->line_direction == ROUTE_DIRECTION_WITH_LINE){
                        navigate_list_values[count]->position.longitude = segment->to_pos.longitude;
                        navigate_list_values[count]->position.latitude = segment->to_pos.latitude;
                    }
                    else{
                        navigate_list_values[count]->position.longitude = segment->from_pos.longitude;
                        navigate_list_values[count]->position.latitude = segment->from_pos.latitude;
                    }
                    navigate_list_icons[count] = NAVIGATE_DIR_IMG[instr];
                    count++;
                }

            }

            if (segment->group_id != Nextsegment->group_id){
                distance_to_next = 0;
            }

            group_id = segment->group_id;
            segment++;
        }
    }
    navigate_list_count = count;
    ssd_generic_icon_list_dialog_show (roadmap_lang_get ("Navigation list"),
                  count,
                  (const char **)navigate_list_labels,
                  (const void **)navigate_list_values,
                  (const char **)navigate_list_icons,
                  NULL,
                  navigate_main_list_call_back,
                  NULL,
                  NULL,
                  NULL,
                  NULL,
                  60,
                  0,
                  FALSE);


}
