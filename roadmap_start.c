/* roadmap_start.c - The main function of the RoadMap application.
 *
 * LICENSE:
 *
 *   (c) Copyright 2002, 2003 Pascal F. Martin
 *   (c) Copyright 2008 Ehud Shabtai
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
 *   void roadmap_start (int argc, char **argv);
 */

#include <stdlib.h>
#include <string.h>

#ifdef ROADMAP_DEBUG_HEAP
#include <mcheck.h>
#endif

#include "roadmap.h"
#include "roadmap_copyright.h"
#include "roadmap_dbread.h"
#include "roadmap_math.h"
#include "roadmap_string.h"
#include "roadmap_config.h"
#include "roadmap_history.h"

#include "roadmap_spawn.h"
#include "roadmap_path.h"
#include "roadmap_net.h"
#include "roadmap_io.h"
#include "roadmap_state.h"
#include "roadmap_object.h"
#include "roadmap_voice.h"
#include "roadmap_gps.h"
#include "roadmap_car.h"
#include "roadmap_canvas.h"

#include "roadmap_preferences.h"
#include "roadmap_coord.h" 
#include "roadmap_crossing.h"
#include "roadmap_sprite.h"
#include "roadmap_screen_obj.h"
#include "roadmap_trip.h"
#include "roadmap_adjust.h"
#include "roadmap_screen.h"
#include "roadmap_view.h"
#include "roadmap_fuzzy.h"
#include "roadmap_navigate.h"
#include "roadmap_label.h"
#include "roadmap_display.h" 
#include "roadmap_locator.h"
#include "roadmap_copy.h"
#include "roadmap_httpcopy.h"
#include "roadmap_download.h"
#include "roadmap_driver.h"
#include "roadmap_factory.h"
#include "roadmap_res.h"
#include "roadmap_main.h"
#include "roadmap_messagebox.h"
#include "roadmap_help.h"
#include "roadmap_pointer.h"
#include "roadmap_sound.h"
#include "roadmap_lang.h"
#include "roadmap_skin.h"
#include "roadmap_start.h"
#include "roadmap_state.h"
#include "roadmap_trip.h"
#include "roadmap_search.h"
#include "roadmap_alerter.h"
#include "roadmap_bar.h"
#include "roadmap_device.h"
#include "roadmap_softkeys.h"
#include "roadmap_border.h"
#include "roadmap_general_settings.h"

#include "Realtime/RealtimeAlerts.h"
#include "Realtime/RealtimeAlertsList.h"
#include "Realtime/RealtimeAlertCommentsList.h"
#include "Realtime/RealtimeTrafficInfoPlugin.h"
#include "Realtime/RealtimePrivacy.h"

#include "roadmap_device_events.h"

#ifdef SSD
#include "ssd/ssd_widget.h"
#include "ssd/ssd_menu.h"
#include "ssd/ssd_confirm_dialog.h"
#include "ssd/ssd_login_details.h"
#include "ssd/ssd_tabcontrol.h"
#include "ssd/ssd_contextmenu.h"

#ifdef _WIN32
   #ifdef   TOUCH_SCREEN
      #pragma message("    Target device type:    TOUCH-SCREEN")
   #else
      #pragma message("    Target device type:    MENU ONLY (NO TOUCH-SCREEN)")
   #endif   // TOUCH_SCREEN
#endif   // _WIN32

ssd_contextmenu_ptr s_main_menu = NULL;

#ifdef  TOUCH_SCREEN
const char*   grid_menu_labels[256];
#endif  //  TOUCH_SCREEN

#endif


#include "navigate/navigate_main.h"
#include "editor/editor_main.h"
#include "editor/editor_screen.h"
#include "editor/db/editor_db.h"
#include "editor/static/update_range.h"
#include "editor/static/add_alert.h"
#include "editor/static/edit_marker.h"
#include "editor/static/notes.h"
#include "editor/export/editor_report.h"
#include "editor/export/editor_download.h"
#include "editor/track/editor_track_main.h"
#include "editor/export/editor_sync.h"
#include "Realtime/Realtime.h"
#include "Realtime/RealtimeTrafficInfo.h"
#include "roadmap_keyboard.h"
#include "roadmap_phone_keyboard.h"
#include "roadmap_input_type.h"

#ifdef IPHONE
#include "iphone/roadmap_location.h"
#endif //IPHONE

static const char *RoadMapMainTitle = "RoadMap";

#define MAX_ACTIONS 120

static int RoadMapStartFrozen = 0;

static RoadMapDynamicString RoadMapStartGpsID;

static RoadMapConfigDescriptor RoadMapConfigGeneralUnit =
                        ROADMAP_CONFIG_ITEM("General", "Unit");

static RoadMapConfigDescriptor RoadMapConfigGeneralKeyboard =
                        ROADMAP_CONFIG_ITEM("General", "Keyboard");

static RoadMapConfigDescriptor RoadMapConfigGeometryMain =
                        ROADMAP_CONFIG_ITEM("Geometry", "Main");

RoadMapConfigDescriptor RoadMapConfigMapPath =
                        ROADMAP_CONFIG_ITEM("Map", "Path");

static RoadMapMenu LongClickMenu;
#ifndef SSD
static RoadMapMenu QuickMenu;
#endif

static RoadMapStartSubscriber  RoadMapStartSubscribers = NULL;
static RoadMapScreenSubscriber roadmap_start_prev_after_refresh = NULL;

/* The menu and toolbar callbacks: --------------------------------------- */

static void roadmap_start_periodic (void);
static BOOL on_key_pressed( const char* utf8char, uint32_t flags);
void roadmap_confirmed_exit(void);
void start_zoom_side_menu(void);
void start_rotate_side_menu(void);
void start_map_options_side_menu(void);
void start_alerts_menu(void);
void start_map_updates_menu(void);
void display_shortcuts(void);

static void roadmap_start_console (void) {

   const char *url = roadmap_gps_source();

   if (url == NULL) {
      roadmap_spawn ("roadgps", "");
   } else {
      char arguments[1024];
      snprintf (arguments, sizeof(arguments), "--gps=%s", url);
      roadmap_spawn ("roadgps", arguments);
   }
}

static void roadmap_start_purge (void) {
   roadmap_history_purge (10);
}

static void roadmap_start_show_destination (void) {
    roadmap_trip_set_focus ("Destination");
    roadmap_screen_refresh ();
}

static void roadmap_start_show_location (void) {
    roadmap_trip_set_focus ("Address");
    roadmap_screen_refresh ();
}

static void roadmap_start_show_gps (void) {
    roadmap_screen_hold ();
    roadmap_trip_set_focus ("GPS");
    roadmap_screen_refresh ();
    roadmap_state_refresh ();//Added by AviR
    
}

static void roadmap_start_hold_map (void) {
   roadmap_start_periodic (); /* To make sure the map is current. */
   roadmap_screen_hold ();
}

static void roadmap_start_rotate (void) {
    roadmap_screen_rotate (10);
}

static void roadmap_start_counter_rotate (void) {
    roadmap_screen_rotate (-10);
}

static void roadmap_start_about (void) {

   char about[500];

   snprintf (about, sizeof(about),
                       "Freemap-Waze 0.14.2.0-mk2\n"
                       "(c)LinQmap \n"
                       "www.waze.co.il \n");

   roadmap_messagebox ("About", about);
}

static void roadmap_start_export_data (void) {

}

static void roadmap_start_export_reset (void) {

   //editor_export_reset_dirty ();
}

static void roadmap_start_download_map (void) {

   //editor_download_update_map (NULL);
}

static void roadmap_start_create_trip (void) {
    
    roadmap_trip_new ();
}

static void roadmap_start_open_trip (void) {
    
    roadmap_trip_load (NULL, 0);
}

static void roadmap_start_save_trip (void) {
    
    roadmap_trip_save (roadmap_trip_current());
}

static void roadmap_start_save_trip_as (void) {
    
    roadmap_trip_save (NULL);
}

static void roadmap_start_trip (void) {
    
    roadmap_trip_start ();
}

static void roadmap_start_trip_resume (void) {
    
    roadmap_trip_resume ();
}

static void roadmap_start_trip_reverse (void) {
    
    roadmap_trip_reverse ();
}

static void roadmap_start_navigate (void) {
    
    navigate_main_calc_route ();
}

static void roadmap_start_set_destination (void) {

    roadmap_trip_set_selection_as ("Destination");
    ssd_dialog_hide_current(dec_close);
    roadmap_screen_refresh();
    navigate_main_calc_route ();
}

static void roadmap_start_set_departure (void) {

    roadmap_trip_set_selection_as ("Departure");
    ssd_dialog_hide_current(dec_close);
    roadmap_screen_refresh();
}

static void roadmap_start_set_waypoint (void) {

    const char *id = roadmap_display_get_id ("Selected Street");

    if (id != NULL) {
       roadmap_trip_set_selection_as (id);
       roadmap_screen_refresh();
    }
}

static void roadmap_start_delete_waypoint (void) {
    
    roadmap_trip_remove_point (NULL);
}

static int roadmap_start_no_download (int fips) {

   if (! roadmap_download_blocked (fips)) {
      roadmap_log (ROADMAP_WARNING, "cannot open map database usc%05d", fips);
      roadmap_download_block (fips);
   }
   return 0;
}

static void roadmap_start_toggle_download (void) {

   if (roadmap_download_enabled()) {

      roadmap_download_subscribe_when_done (NULL);
      roadmap_locator_declare (&roadmap_start_no_download);

   } else {

      static int ProtocolInitialized = 0;

      if (! ProtocolInitialized) {

         /* PLUGINS NOT SUPPORTED YET.
          * roadmap_plugin_load_all
          *      ("download", roadmap_download_subscribe_protocol);
          */

         roadmap_copy_init (roadmap_download_subscribe_protocol);
         roadmap_httpcopy_init (roadmap_download_subscribe_protocol);

         ProtocolInitialized = 1;
      }

      roadmap_download_subscribe_when_done (roadmap_screen_redraw);
      //roadmap_locator_declare (roadmap_download_get_county);
      roadmap_download_unblock_all ();
   }

   roadmap_screen_redraw ();
}


static void roadmap_start_detect_receiver (void) {
    
    roadmap_gps_detect_receiver ();
}

static void roadmap_confirmed_start_sync_callback(int exit_code, void *context){
    if (exit_code != dec_yes)
         return;

    editor_report_markers ();
    export_sync ();
}

static void roadmap_start_sync_data (void) {
    ssd_confirm_dialog ("Warning", "Sync requires large amount of data, continue?", TRUE, roadmap_confirmed_start_sync_callback , NULL);
}


static void roadmap_start_quick_menu (void);
static void start_settings_quick_menu(void);

void search_menu_search_history  (void);
void search_menu_search_favorites(void);
void search_menu_search_address  (void);

#ifdef   TESTING_MENU
void do_nothing(void)
{}
#endif   // TESTING_MENU


/* The RoadMap menu and toolbar items: ----------------------------------- */

/* This table lists all the RoadMap actions that can be initiated
 * fom the user interface (a sort of symbol table).
 * Any other part of the user interface (menu, toolbar, etc..)
 * will reference an action.
 */
RoadMapAction RoadMapStartActions[MAX_ACTIONS + 1] = {

   {"preferences", "Preferences", "Preferences", "P",
      "Open the preferences editor", roadmap_preferences_edit},

   {"gpsconsole", "GPS Console", "Console", "C",
      "Start the GPS console application", roadmap_start_console},

   {"mutevoice", "Mute Voice", "Mute", NULL,
      "Mute all voice annoucements", roadmap_voice_mute},

   {"enablevoice", "Enable Voice", "Mute Off", NULL,
      "Enable all voice annoucements", roadmap_voice_enable},

   {"nonavigation", "Disable Navigation", "Nav Off", NULL,
      "Disable all navigation functions", roadmap_navigate_disable},

   {"navigation", "Enable Navigation", "Nav On", NULL,
      "Enable all GPS-based navigation functions", roadmap_navigate_enable},

   {"logtofile", "Log to File", "Log", NULL,
      "Save future log messages to the postmortem file",
      roadmap_log_save_all},

   {"nolog", "Disable Log", "No Log", NULL,
      "Do not save future log messages to the postmortem file",
      roadmap_log_save_all},

   {"purgelogfile", "Purge Log File", "Purge", NULL,
      "Delete the current postmortem log file", roadmap_log_purge},

   {"purgehistory", "Purge History", "Forget", NULL,
      "Remove all but the 10 most recent addresses", roadmap_start_purge},

   {"quit", "Quit", NULL, NULL,
      "Quit RoadMap", roadmap_confirmed_exit}, 

   {"zoomin", "Zoom In", "+", NULL,
      "Enlarge the central part of the map", roadmap_screen_zoom_in},

   {"zoomout", "Zoom Out", "-", NULL,
      "Show a larger area", roadmap_screen_zoom_out},

   {"zoom1", "Normal Size", ":1", NULL,
      "Set the map back to the default zoom level", roadmap_screen_zoom_reset},

   {"up", "Up", "N", NULL,
      "Move the map view upward", roadmap_screen_move_up},

   {"left", "Left", "W", NULL,
      "Move the map view to the left", roadmap_screen_move_left},

   {"right", "Right", "E", NULL,
      "Move the map view to the right", roadmap_screen_move_right},

   {"down", "Down", "S", NULL,
      "Move the map view downward", roadmap_screen_move_down},

   {"toggleview", "Toggle view mode", "M", NULL,
      "Toggle view mode 2D / 3D", roadmap_screen_toggle_view_mode},

   {"toggleskin", "Toggle skin", "", NULL,
      "Toggle skin (day / night)", roadmap_skin_toggle},

   {"toggleorientation", "Toggle orientation mode", "", NULL,
      "Toggle orientation mode dynamic / fixed",
      roadmap_screen_toggle_orientation_mode},

   {"IncHorizon", "Increase Horizon", "I", NULL,
      "Increase the 3D horizon", roadmap_screen_increase_horizon},

   {"DecHorizon", "Decrease Horizon", "DI", NULL,
      "Decrease the 3D horizon", roadmap_screen_decrease_horizon},

   {"clockwise", "Rotate Clockwise", "R+", NULL,
      "Rotate the map view clockwise", roadmap_start_rotate},

   {"counterclockwise", "Rotate Counter-Clockwise", "R-", NULL,
      "Rotate the map view counter-clockwise", roadmap_start_counter_rotate},

   {"hold", "Hold Map", "Hold", "H",
      "Hold the map view in its current position", roadmap_start_hold_map},

#ifndef SSD
   {"address", "Address...", "Addr", "A",
      "Show a specified address", roadmap_address_location_by_city},
#else      
   {"address", "Address...", "Addr", "A",
      "Show a specified address", roadmap_address_history},
#endif      

   {"intersection", "Intersection...", "X", NULL,
      "Show a specified street intersection", roadmap_crossing_dialog},

   {"position", "Position...", "P", NULL,
      "Show a position at the specified coordinates", roadmap_coord_dialog},

   {"destination", "Destination", "D", NULL,
      "Show the current destination point", roadmap_start_show_destination},

   {"gps", "GPS Position", "GPS", "G",
      "Center the map on the current GPS position", roadmap_start_show_gps},

   {"location", "Location", "L", NULL,
      "Center the map on the last selected location",
      roadmap_start_show_location},

   {"mapdownload", "Map Download", "Download", NULL,
      "Enable/Disable the map download mode", roadmap_start_toggle_download},

   {"mapdiskspace", "Map Disk Space", "Disk", NULL,
      "Show the amount of disk space occupied by the maps",
      roadmap_download_show_space},

   {"deletemaps", "Delete Maps...", "Delete", "Del",
      "Delete maps that are currently visible", roadmap_download_delete},

   {"newtrip", "New Trip", "New", NULL,
      "Create a new trip", roadmap_start_create_trip},

   {"opentrip", "Open Trip", "Open", "O",
      "Open an existing trip", roadmap_start_open_trip},

   {"savetrip", "Save Trip", "Save", "S",
      "Save the current trip", roadmap_start_save_trip},

   {"savescreenshot", "Make a screenshot of the map", "Screenshot", "Y",
      "Make a screenshot of the current map under the trip name",
      roadmap_trip_save_screenshot},

   {"savetripas", "Save Trip As...", "Save As", "As",
      "Save the current trip under a different name",
      roadmap_start_save_trip_as},

   {"starttrip", "Start Trip", "Start", NULL,
      "Start tracking the current trip", roadmap_start_trip},

   {"stoptrip", "Stop Trip", "Stop", NULL,
      "Stop tracking the current trip", roadmap_trip_stop},

   {"resumetrip", "Resume Trip", "Resume", NULL,
      "Resume the trip (keep the existing departure point)",
      roadmap_start_trip_resume},

   {"returntrip", "Return Trip", "Return", NULL,
      "Start the trip back to the departure point",
      roadmap_start_trip_reverse},

   {"setasdeparture", "Set as Departure", NULL, NULL,
      "Set the selected street block as the trip's departure",
      roadmap_start_set_departure},

   {"setasdestination", "Set as Destination", NULL, NULL,
      "Set the selected street block as the trip's destination",
      roadmap_start_set_destination},

   {"navigate", "Navigate", NULL, NULL,
      "Calculate route",
      roadmap_start_navigate},

   {"addaswaypoint", "Add as Waypoint", "Waypoint", "W",
      "Set the selected street block as waypoint", roadmap_start_set_waypoint},

   {"deletewaypoints", "Delete Waypoints...", "Delete...", NULL,
      "Delete selected waypoints", roadmap_start_delete_waypoint},

   {"full", "Full Screen", "Full", "F",
      "Toggle the window full screen mode (depends on the window manager)",
      roadmap_main_toggle_full_screen},

   {"about", "About", NULL, NULL,
      "Show information about RoadMap", roadmap_start_about},

   {"exportdata", "Export Data", NULL, NULL,
      "Export editor data", roadmap_start_export_data},

   {"resetexport", "Reset export data", NULL, NULL,
      "Reset export data", roadmap_start_export_reset},

   {"updatemap", "Update map", NULL, NULL,
      "Export editor data", roadmap_start_download_map},

   {"detectreceiver", "Detect GPS receiver", NULL, NULL,
      "Auto-detect GPS receiver", roadmap_start_detect_receiver},

   {"sync", "Sync", NULL, NULL,
      "Sync map and data", roadmap_start_sync_data},

   {"quickmenu", "Open quick menu", NULL, NULL,
      "Open quick menu", roadmap_start_quick_menu},

   {"alertsmenu", "alerts menu", NULL, NULL,
     "alerts menu", start_alerts_menu},

   {"updaterange", "Update street range", NULL, NULL,
      "Update street range", update_range_dialog},

   {"viewmarkers", "View markers", NULL, NULL,
      "View / Edit markers", edit_markers_dialog},

   {"addquicknote", "Add a quick note", NULL, NULL,
      "Add a quick note", editor_notes_add_quick},

   {"addeditnote", "Add a note", NULL, NULL,
      "Add a note and open edit dialog", editor_notes_add_edit},

   {"addvoicenote", "Add a voice note", NULL, NULL,
      "Add a voice note", editor_notes_add_voice},
#ifdef IPHONE
   {"toggle_location", "Toggle location mode", NULL, NULL,
      "Enable/Disable location services", roadmap_location_toggle},
      
   {"minimize", "Minimize application", NULL, NULL,
      "Close app window and keep in background", roadmap_main_minimize},
#endif

   {"speedcam", "Add a speed camera", NULL,  NULL,
      "Add a speed camera", add_speed_cam_alert},

   {"redlight", "Add a red light camera", NULL,  NULL,
         "Add a red light camera", add_red_light_cam_alert},

   {"reportpolice", "Report a police", NULL,  NULL,
      "Report a police", add_real_time_alert_for_police},
      
   {"reportaccident", "Report an accident", NULL,  NULL,
      "Report an accident", add_real_time_alert_for_accident},
   
   {"reporttrafficjam", "Report a traffic jam", NULL,  NULL,
      "Report a traffic jam", add_real_time_alert_for_traffic_jam},

   {"reporthazard", "Report a hazard condition", NULL,  NULL,
      "Report a hazard condition", add_real_time_alert_for_hazard},

   {"reportother", "Report other", NULL,  NULL,
      "Report other", add_real_time_alert_for_other},

   {"road_construction", "Report road construction", NULL,  NULL,
      "Report road construction", add_real_time_alert_for_construction},


   {"reporttrafficjam", "Report a traffic jam", NULL,  NULL,
      "Report a traffic jam", add_real_time_alert_for_traffic_jam},

   {"real_time_alerts_list", "Real Time Alerts", NULL,  NULL,
      "Real Time Alerts",RealtimeAlertsList},
      
    {"next_real_time_alerts", "Next real time alert", NULL,  NULL,
         "Next real time alert",RTAlerts_Scroll_Next},

   {"prev_real_time_alerts", "Previous real time alert", NULL,  NULL,
         "Previous real time alert",RTAlerts_Scroll_Prev},
         
   {"cancel_real_time_scroll", "Cancel real time scroll", NULL,  NULL,
         "Cancel real time scroll", RTAlerts_Cancel_Scrolling},  

   {"reportincident", "Report an incident", NULL,  NULL,
      "Report an incident", add_real_time_chit_chat},
          
   {"toggle_navigation_guidance", "Toggle navigation guidance", NULL,  NULL,
         "Toggle navigation guidance", toggle_navigation_guidance},
         
   {"settingsmenu", "Settings menu", NULL, NULL,
            "Settings menu", start_settings_quick_menu},
            
   {"select_car", "Select your car", NULL,  NULL,
               "Select your car", roadmap_car}, 
 
   {"send_comment", "Send a comment on alert", NULL,  NULL,
                           "Send a comment on alert", real_time_post_alert_comment}, 
            
   {"left_softkey", "left soft key", NULL,  NULL,
                           "left soft key", roadmap_softkeys_left_softkey_callback}, 

   {"right_softkey", "right soft key", NULL,  NULL,
                           "right soft key", roadmap_softkeys_right_softkey_callback}, 

   {"zoom", "zoom", NULL,  NULL,
                     "zoom", start_zoom_side_menu}, 

   {"rotate", "rotate", NULL,  NULL,
                     "rotate", start_rotate_side_menu}, 
                     
   {"toggle_traffic", "Toggle traffic info", NULL,  NULL,
                           "Toggle traffic info", RealtimeRoadToggleShowTraffic}, 

   {"search_menu", "Search menu", NULL,  NULL,
                           "Search menu", roadmap_search_menu},
 
   {"view", "View menu", NULL,  NULL,
                           "View menu", roadmap_view_menu},
 
   {"commute_view", "Commute view", "Commute view", NULL,
      "Commute view", roadmap_view_commute},
   
   {"navigation_view", "Navigation view", "Navigation view", NULL,
      "Navigation view", roadmap_view_navigation},
   
   {"privacy_settings", "Privacy settings", NULL,  NULL,
                        "Privacy settings", RealtimePrivacySettings}, 
                     
   {"general_settings", "General settings", NULL,  NULL,
                        "General settings", roadmap_general_settings_show}, 

   {"stop_navigate", "Stop navigation", NULL,  NULL,
                        "Stop navigation", navigate_main_stop_navigation_menu}, 

#ifdef SSD
   {"login_details", "Login details", NULL, NULL, 
                  "Login details", ssd_login_details_dialog_show},
#endif

   {"search_history", "Search history", "Search history", NULL,
      "Search history", search_menu_search_history},

   {"search_favorites", "Search favorites", "Search favorites", NULL,
      "Search favorites", search_menu_search_favorites},
   
   {"search_address", "Search address", "Search address", NULL,
      "Search address", search_menu_search_address},
      
   {"show_me", "Me on map", "Me on map", NULL,
   	"Me on map", show_me_on_map},

   {"show_shortcuts", "Shortcuts", NULL,  NULL,
         "Shortcuts", display_shortcuts}, 

   {"recommend_waze", "Recommend Waze to a friend", NULL, NULL,
   		"Recommend Waze to a friend", RecommentToFriend},

   {"help_menu", "Help menu", NULL, NULL,
   		"Help", roadmap_help_menu},
         
   {"help", "Help", NULL, NULL,
   		"Help",  roadmap_open_help},
   		
   {"navigation_list", "Navigation list", NULL, NULL,
   		"Navigation list", navigate_main_list},

  {"map_updates_menu", "Open map updates menu", NULL, NULL,
      "Open map updates menu", start_map_updates_menu},

  {"nav_list_next", "Show next instruction", NULL, NULL,
      "Show next instruction", navigate_main_list_display_next},
  
  {"nav_list_prev", "Show previous instruction", NULL, NULL,
      "how previous instruction", navigate_main_list_display_previous},


#ifdef   TESTING_MENU
   {"dummy_entry", "Dummy popup", "", NULL,
      "Dummy popup...", do_nothing},
   //
   {"dummy_entry1", "DE1", "", NULL,
      "Dummy entry...", do_nothing},
   {"dummy_entry2", "DE2", "", NULL,
      "Dummy entry...", do_nothing},
   {"dummy_entry3", "DE3", "", NULL,
      "Dummy entry...", do_nothing},
   {"dummy_entry4", "DE4", "", NULL,
      "Dummy entry...", do_nothing},
   {"dummy_entry5", "DE5", "", NULL,
      "Dummy entry...", do_nothing},
   {"dummy_entry6", "DE6", "", NULL,
      "Dummy entry...", do_nothing},
   {"dummy_entry7", "DE7", "", NULL,
      "Dummy entry...", do_nothing},
   {"dummy_entry8", "DE8", "", NULL,
      "Dummy entry...", do_nothing},
   {"dummy_entry9", "DE9", "", NULL,
      "Dummy entry...", do_nothing},
   {"dummy_entry10", "DE10", "", NULL,
      "Dummy entry...", do_nothing},
   {"dummy_entry11", "DE11", "", NULL,
      "Dummy entry...", do_nothing},
   {"dummy_entry12", "DE12", "", NULL,
      "Dummy entry...", do_nothing},
#endif   // TESTING_MENU

   {NULL, NULL, NULL, NULL, NULL, NULL}
};


#ifdef UNDER_CE
static const char *RoadMapStartCfgActions[] = {

   "preferences",
   "mutevoice",
   "enablevoice",
   "quit",
   "zoomin",
   "zoomout",
   "zoom1",
   "up",
   "left",
   "right",
   "down",
   "toggleview",
   "toggleorientation",
   "IncHorizon",
   "DecHorizon",
   "clockwise",
   "counterclockwise",
   "hold",
   "address",
   "destination",
   "gps",
   "location",
   "full",
   "sync",
   "quickmenu",
   "updaterange",
   "viewmarkers",
   "addquicknote",
   "addeditnote",
   "addvoicenote",
#ifdef IPHONE
   "toggle_location",
#endif
   NULL
};
#endif


#ifdef J2ME
static const char *RoadMapStartMenu[] = {
   "about",
   "address",
   "gps",
   "hold",
   "toggleview",
   "toggleorientation",
   "traffic",
   "togglegpsrecord",
   "uploadj2merecord",
   "detectreceiver",
   "destination",
   "preferences",
   "quit",
   NULL
};
#else

static const char *RoadMapStartMenu[] = {

   ROADMAP_MENU "File",

   "preferences",
   "gpsconsole",

   RoadMapFactorySeparator,

   "sync",
   "exportdata",
   "updatemap",

   RoadMapFactorySeparator,
   "mutevoice",
   "enablevoice",
   "nonavigation",
   "navigation",

   RoadMapFactorySeparator,

   "logtofile",
   "nolog",
   "purgehistory",

   RoadMapFactorySeparator,

   "quit",


   ROADMAP_MENU "View",

   "zoomin",
   "zoomout",
   "zoom1",

   RoadMapFactorySeparator,

   "up",
   "left",
   "right",
   "down",
   "toggleorientation",
   "IncHorizon",
   "DecHorizon",
   "toggleview",
   "full",

   RoadMapFactorySeparator,

   "clockwise",
   "counterclockwise",

   RoadMapFactorySeparator,

   "hold",


   ROADMAP_MENU "Find",

   "address",
   "intersection",
   "position",

   RoadMapFactorySeparator,

   "destination",
   "gps",

   RoadMapFactorySeparator,

   "mapdownload",
   "mapdiskspace",
   "deletemaps",


   ROADMAP_MENU "Trip",

   "newtrip",
   "opentrip",
   "savetrip",
   "savetripas",
   "savescreenshot",

   RoadMapFactorySeparator,

   "starttrip",
   "stoptrip",
   "resumetrip",
   "resumetripnorthup",
   "returntrip",
   "navigate",

   RoadMapFactorySeparator,

   "setasdestination",
   "setasdeparture",
   "addaswaypoint",
   "deletewaypoints",

   ROADMAP_MENU "Tools",

   "detectreceiver",

   ROADMAP_MENU "Help",

   RoadMapFactoryHelpTopics,

   RoadMapFactorySeparator,

   "about",

   NULL
};
#endif


static char const *RoadMapStartToolbar[] = {

   "destination",
   "location",
   "gps",
   "hold",

   RoadMapFactorySeparator,

   "counterclockwise",
   "clockwise",

   "zoomin",
   "zoomout",
   "zoom1",

   RoadMapFactorySeparator,

   "up",
   "left",
   "right",
   "down",

   RoadMapFactorySeparator,

   "full",
   "quit",

   NULL,
};


static char const *RoadMapStartLongClickMenu[] = {

   "setasdeparture",
   "setasdestination",

   NULL,
};


#ifndef SSD
static char const *RoadMapStartQuickMenu[] = {

   "address",
   RoadMapFactorySeparator,
   "detectreceiver",
   "preferences",
   "about",
   "quit",

   NULL,
};
#endif

static char const *RoadMapAlertMenu[] = {
   NULL,
};

#ifndef SSD
static char const *RoadMapSettingsMenu[] = {
   "login_details",
   "traffic",
   "select_car",
   NULL,
};
#endif

void display_shortcuts(){
	
#ifdef TOUCH_SCREEN
	return;
#endif 	
	ssd_dialog_hide_all(dec_close);
	
	if (roadmap_display_is_sign_active("Shortcuts")){
		roadmap_display_hide("Shortcuts");
		roadmap_screen_redraw();
		return;
	}
	if (is_screen_wide())
		roadmap_activate_image_sign("Shortcuts", "shortcuts_wide");
	else
		roadmap_activate_image_sign("Shortcuts", "shortcuts");
}	
	
#ifndef UNDER_CE
static char const *RoadMapStartKeyBinding[] = {

   "Button-Left"     ROADMAP_MAPPED_TO "left",
   "Button-Right"    ROADMAP_MAPPED_TO "right",
   "Button-Up"       ROADMAP_MAPPED_TO "up",
   "Button-Down"     ROADMAP_MAPPED_TO "down",

   /* These binding are for the iPAQ buttons: */
   "Button-Menu"     ROADMAP_MAPPED_TO "zoom1",
   "Button-Contact"  ROADMAP_MAPPED_TO "zoomin",
   "Button-Calendar" ROADMAP_MAPPED_TO "zoomout",
   "Button-Start"    ROADMAP_MAPPED_TO "quit",

   /* These binding are for regular keyboards (case unsensitive !): */
   "+"               ROADMAP_MAPPED_TO "zoomin",
   "-"               ROADMAP_MAPPED_TO "zoomout",
   "A"               ROADMAP_MAPPED_TO "address",
   "B"               ROADMAP_MAPPED_TO "returntrip",
   /* C Unused. */
   "D"               ROADMAP_MAPPED_TO "destination",
   "E"               ROADMAP_MAPPED_TO "deletemaps",
   "F"               ROADMAP_MAPPED_TO "full",
   "G"               ROADMAP_MAPPED_TO "gps",
   "H"               ROADMAP_MAPPED_TO "hold",
   "I"               ROADMAP_MAPPED_TO "intersection",
   "J"               ROADMAP_MAPPED_TO "counterclockwise",
   "K"               ROADMAP_MAPPED_TO "clockwise",
   "L"               ROADMAP_MAPPED_TO "location",
   "M"               ROADMAP_MAPPED_TO "mapdownload",
   "N"               ROADMAP_MAPPED_TO "newtrip",
   "O"               ROADMAP_MAPPED_TO "opentrip",
   "P"               ROADMAP_MAPPED_TO "stoptrip",
   "Q"               ROADMAP_MAPPED_TO "quit",
   "R"               ROADMAP_MAPPED_TO "zoom1",
   "S"               ROADMAP_MAPPED_TO "starttrip",
   "T"             ROADMAP_MAPPED_TO "next_real_time_alerts",
   "U"               ROADMAP_MAPPED_TO "gpsnorthup",
   "V"             ROADMAP_MAPPED_TO "prev_real_time_alerts",
   "W"               ROADMAP_MAPPED_TO "addaswaypoint",
   "X"               ROADMAP_MAPPED_TO "intersection",
   "Y"               ROADMAP_MAPPED_TO "savesscreenshot",
   /* Z Unused. */
   NULL
};

#else
static char const *RoadMapStartKeyBinding[] = {

   "Button-Left"     ROADMAP_MAPPED_TO "counterclockwise",
   "Button-Right"    ROADMAP_MAPPED_TO "clockwise",
   "Button-Up"       ROADMAP_MAPPED_TO "zoomin",
   "Button-Down"     ROADMAP_MAPPED_TO "zoomout",

   "Button-App1"     ROADMAP_MAPPED_TO "",
   "Button-App2"     ROADMAP_MAPPED_TO "",
   "Button-App3"     ROADMAP_MAPPED_TO "",
   "Button-App4"     ROADMAP_MAPPED_TO "",

   NULL
};
#endif


BOOL get_menu_item_names(  const char*          menu_name,
                           ssd_contextmenu_ptr  parent, 
                           const char*          items[],
                           int*                 count)
{
   int                  i;
   ssd_contextmenu_ptr  menu  = NULL;
   RoadMapAction*       action= NULL;
   
   if( count)
      (*count) = 0;
  
   if( !parent)
   {
      assert(0);
      return FALSE;
   }
   
   if( menu_name && (*menu_name))
   {
      for( i=0; i<parent->item_count; i++)
      {
         if( CONTEXT_MENU_FLAG_POPUP & parent->item[i].flags)
         {
            action = (RoadMapAction*)parent->item[i].action;
            if( action && !strcmp( menu_name, action->name))
            {
               menu = parent->item[i].popup->menu;
               break;
            }
            
            if( get_menu_item_names( menu_name, parent->item[i].popup->menu, items, count))
               return TRUE;
         }
      }
   }
   else
      menu = parent;

   if( !menu)
      return FALSE;      

   for( i=0; i<menu->item_count; i++)
   {
      action = (RoadMapAction*)menu->item[i].action;
      assert( action);
      
      items[i] = action->name;
   }
   items[i] = NULL;
   
   if( count)
      (*count) = menu->item_count;
   
   return TRUE;
}

#ifndef  TOUCH_SCREEN
void  on_contextmenu_closed(  BOOL              made_selection,
                              ssd_cm_item_ptr   item,
                              void*             context)
{
   if( made_selection)
   {
      RoadMapAction* action = (RoadMapAction*)item->action;
      action->callback();
   }
}
#endif   // !TOUCH_SCREEN

static void roadmap_start_quick_menu (void) {

#ifdef  TOUCH_SCREEN
   int   count;
#else   
   int   menu_x;
#endif   // !TOUCH_SCREEN   

   if( !s_main_menu)
   {
      s_main_menu = roadmap_factory_load_menu("quick.menu", RoadMapStartActions);
      
      if( !s_main_menu)
      {
         assert(0);
         return;
      }
   }
   
#ifndef  TOUCH_SCREEN
   if  (ssd_widget_rtl (NULL))
	   menu_x = SSD_X_SCREEN_RIGHT;
	else
		menu_x = SSD_X_SCREEN_LEFT;

   ssd_context_menu_show(  menu_x,              // X
                           SSD_Y_SCREEN_BOTTOM, // Y
                           s_main_menu,
                           on_contextmenu_closed,
                           NULL,
                           dir_default); 

#else
   // TOUCH SCREEN:

#ifdef SSD
   if( !get_menu_item_names( NULL, s_main_menu, grid_menu_labels, &count))
   {
      assert(0);
      return;
   }      

   ssd_menu_activate("Main Menu", 
                     NULL, 
                     grid_menu_labels, 
                     NULL,
                     RoadMapStartActions,
                     SSD_CONTAINER_BORDER|SSD_CONTAINER_TITLE|SSD_DIALOG_TRANSPARENT|SSD_ROUNDED_CORNERS|SSD_POINTER_MENU|SSD_HEADER_GRAY);
#else
   #error Need to port next menu to use 'ssd_contextmenu_ptr'
   // NOT SSD
   if (QuickMenu == NULL) {

       QuickMenu = roadmap_factory_menu ("quick",
                                     RoadMapStartQuickMenu,
                                     RoadMapStartActions);
   }

   if (QuickMenu != NULL) {
      const RoadMapGuiPoint *point = roadmap_pointer_position ();
      roadmap_main_popup_menu (QuickMenu, point->x, point->y);
   }
#endif   // SSD

#endif   // !TOUCH_SCREEN
}

void on_closed_alerts_quick_menu (int exit_code, void* context){
	if ((exit_code == dec_cancel) ||( exit_code == dec_close)){
		roadmap_trip_restore_focus();   
	}
}

void start_map_updates_menu(void){
		ssd_list_menu_activate ("map_updates", "map_updates", RoadMapAlertMenu, NULL,
   		  				        RoadMapStartActions,
    							SSD_DIALOG_FLOAT|SSD_DIALOG_TRANSPARENT|SSD_DIALOG_VERTICAL|SSD_BG_IMAGE);
}

void start_alerts_menu(void){

    PluginLine line;
    int direction;
    RoadMapGpsPosition 			  *CurrentGpsPoint;
    const RoadMapPosition 		*GpsPosition;
    const char *menu[2] ={"alerts_offline", "alerts"};
    
    int menu_file;
    BOOL has_position = FALSE;

	
	if (ssd_dialog_is_currently_active()) {
		if (ssd_dialog_is_currently_vertical()){ 
			on_closed_alerts_quick_menu(dec_close, NULL);
			ssd_dialog_hide_current(dec_close);
			roadmap_screen_refresh ();
			return; 
		}
	}

	CurrentGpsPoint = malloc(sizeof(*CurrentGpsPoint));
	roadmap_check_allocated(CurrentGpsPoint);

	if (roadmap_navigate_get_current
        (CurrentGpsPoint, &line, &direction) == -1) {
		BOOL has_reception = (roadmap_gps_reception_state() != GPS_RECEPTION_NONE) && (roadmap_gps_reception_state() != GPS_RECEPTION_NA);
		GpsPosition = roadmap_trip_get_position ("GPS");
	    if ((GpsPosition != NULL) && (has_reception)){
                     CurrentGpsPoint->latitude = GpsPosition->latitude;
                     CurrentGpsPoint->longitude = GpsPosition->longitude;
                     CurrentGpsPoint->speed = 0;
                     CurrentGpsPoint->steering = 0;
                     has_position = TRUE;
	     } else{
                     free(CurrentGpsPoint);
         }
    }
    else
    	has_position = TRUE;
    
    if (has_position){
    	roadmap_trip_set_gps_position ("AlertSelection", "Selection", "new_alert_marker", CurrentGpsPoint);
    	roadmap_trip_set_focus("AlertSelection"); 
		roadmap_screen_refresh ();
    }
    
    menu_file = RealTimeLoginState();     
#ifdef IPHONE
    if (is_screen_wide() && (menu_file == 1)){
    	char name[255];
    	sprintf(name,"%s_wide",menu[menu_file]);
		ssd_menu_activate (name, menu[menu_file], RoadMapAlertMenu, on_closed_alerts_quick_menu,
   		  	   	           RoadMapStartActions,SSD_CONTAINER_BORDER|SSD_CONTAINER_TITLE|SSD_DIALOG_TRANSPARENT|SSD_ROUNDED_CORNERS|SSD_POINTER_MENU|SSD_HEADER_GRAY);
    }   		  	   	          
    else
#endif    
		ssd_list_menu_activate (menu[menu_file], menu[menu_file], RoadMapAlertMenu, on_closed_alerts_quick_menu,
   		  				        RoadMapStartActions,
    							SSD_DIALOG_FLOAT|SSD_DIALOG_TRANSPARENT|SSD_DIALOG_VERTICAL|SSD_BG_IMAGE);
   												 
}

static void start_settings_quick_menu (void) {

#ifdef TOUCH_SCREEN
#ifdef SSD
   int count;

   if( !s_main_menu)
   {
      assert(0);
      return;
   }

   if( !get_menu_item_names( "settingsmenu", s_main_menu, grid_menu_labels, &count))
   {
      assert(0);
      return;
   }      

   ssd_menu_activate("Settings menu", 
                     NULL, 
                     grid_menu_labels, 
                     NULL,
                     RoadMapStartActions,
                     SSD_CONTAINER_BORDER|SSD_CONTAINER_TITLE|SSD_DIALOG_TRANSPARENT|SSD_ROUNDED_CORNERS|SSD_POINTER_MENU|SSD_HEADER_GRAY);
#else
   if (QuickMenu == NULL) {

       QuickMenu = roadmap_factory_menu ("settings",
                                  RoadMapSettingsMenu,
                                         RoadMapStartActions);
   }

   if (QuickMenu != NULL) {
      const RoadMapGuiPoint *point = roadmap_pointer_position ();
      roadmap_main_popup_menu (QuickMenu, point->x, point->y);
   }
#endif   
#endif   // TOUCH_SCREEN
}


void start_side_menu (const char           			 *name,
				      const char           			 *items[],
            		  const RoadMapAction  *actions,
            		  int                   flags)
{
   ssd_menu_activate (name, NULL, items, NULL,
          actions,
    SSD_DIALOG_FLOAT|SSD_DIALOG_TRANSPARENT|SSD_DIALOG_VERTICAL|SSD_ALIGN_VCENTER|SSD_ROUNDED_CORNERS|SSD_POINTER_MENU|SSD_HEADER_GRAY|flags);
}

#ifndef TOUCH_SCREEN	
static char const *MapOptionsMenu[] = {
   "zoom",
   RoadMapFactorySeparator,
   "rotate",
   RoadMapFactorySeparator,
   "toggleview",
   RoadMapFactorySeparator,
   "toggleskin",
   NULL,
};
#endif

void start_map_options_side_menu(){
#ifndef TOUCH_SCREEN	
	start_side_menu("Map Options Menu", MapOptionsMenu, RoadMapStartActions,0);
#endif
}

static char const *ZoomMenu[] = {
   "zoomin",
   RoadMapFactorySeparator,
   "zoomout",
   NULL,
};

void start_zoom_side_menu(){
	start_side_menu("Zoom Menu", ZoomMenu, RoadMapStartActions,SSD_BUTTON_NO_TEXT);
}

static char const *RotateMenu[] = {
   "clockwise",
   RoadMapFactorySeparator,
   "counterclockwise",
   NULL,
};

void start_rotate_side_menu(){
	start_side_menu("Rotate Menu", RotateMenu, RoadMapStartActions,SSD_BUTTON_NO_TEXT);
}

#ifdef UNDER_CE
static void roadmap_start_init_key_cfg (void) {

   const char **keys = RoadMapStartKeyBinding;
   RoadMapConfigDescriptor config = ROADMAP_CONFIG_ITEM("KeyBinding", "");

   while (*keys) {
      char *text;
      char *separator;
      const RoadMapAction *this_action;
      const char **cfg_actions = RoadMapStartCfgActions;
      RoadMapConfigItem *item;

      text = strdup (*keys);
      roadmap_check_allocated(text);

      separator = strstr (text, ROADMAP_MAPPED_TO);
      if (separator != NULL) {

         const char *new_config = NULL;
         char *p;
         for (p = separator; *p <= ' '; --p) *p = 0;

         p = separator + strlen(ROADMAP_MAPPED_TO);
         while (*p && (*p <= ' ')) ++p;

         this_action = roadmap_start_find_action (p);

         config.name = text;
         config.reference = NULL;

         if (this_action != NULL) {

            item = roadmap_config_declare_enumeration
                   ("preferences", &config, NULL,
                    roadmap_lang_get (this_action->label_long), NULL);

            if (strcmp(roadmap_lang_get (this_action->label_long),
                     roadmap_config_get (&config))) {

               new_config = roadmap_config_get (&config);
            }

            roadmap_config_add_enumeration_value (item, "");
         } else {

            item = roadmap_config_declare_enumeration
                   ("preferences", &config, NULL, "", NULL);

            if (strlen(roadmap_config_get (&config))) {
               new_config = roadmap_config_get (&config);
            }
         }

         while (*cfg_actions) {
            const RoadMapAction *cfg_action =
                        roadmap_start_find_action (*cfg_actions);
            if (new_config &&
                  !strcmp(new_config,
                          roadmap_lang_get (cfg_action->label_long))) {
               new_config = cfg_action->name;
            }

            roadmap_config_add_enumeration_value
                     (item, roadmap_lang_get (cfg_action->label_long));
            cfg_actions++;
         }

         if (new_config != NULL) {
            char str[100];
            snprintf(str, sizeof(str), "%s %s %s", text, ROADMAP_MAPPED_TO,
                                                   new_config);
            *keys = strdup(str);
         }
      }

      keys++;
   }
}
#endif


static void roadmap_start_set_unit (void) {

   const char *unit = roadmap_config_get (&RoadMapConfigGeneralUnit);

   if (strcmp (unit, "imperial") == 0) {

      roadmap_math_use_imperial();

   } else if (strcmp (unit, "metric") == 0) {

      roadmap_math_use_metric();

   } else {
      roadmap_log (ROADMAP_ERROR, "%s is not a supported unit", unit);
      roadmap_math_use_imperial();
   }
}


static int RoadMapStartGpsRefresh = 0;

static void roadmap_gps_update
               (time_t gps_time,
                const RoadMapGpsPrecision *dilution,
                const RoadMapGpsPosition *gps_position) {

   static int RoadMapSynchronous = -1;

   if (RoadMapStartFrozen) {

      RoadMapStartGpsRefresh = 0;

   } else {

/*
      roadmap_object_move (RoadMapStartGpsID, gps_position);

      roadmap_trip_set_mobile ("GPS", gps_position);
      */

      roadmap_log_reset_stack ();

      roadmap_navigate_locate (gps_position);
      roadmap_navigate_check_alerts ();
      roadmap_log_reset_stack ();

      if (RoadMapSynchronous) {

         if (RoadMapSynchronous < 0) {
            RoadMapSynchronous = roadmap_option_is_synchronous ();
         }

         RoadMapStartGpsRefresh = 0;

         roadmap_screen_refresh();
         roadmap_log_reset_stack ();

      } else {

         RoadMapStartGpsRefresh = 1;
      }
   }
}


static void roadmap_start_periodic (void) {

   roadmap_spawn_check ();

   if (RoadMapStartGpsRefresh) {

      RoadMapStartGpsRefresh = 0;

      roadmap_screen_refresh();
      roadmap_log_reset_stack ();
   }
}


static void roadmap_start_add_gps (RoadMapIO *io) {

   roadmap_main_set_input (io, roadmap_gps_input);
}

static void roadmap_start_remove_gps (RoadMapIO *io) {

   roadmap_main_remove_input(io);
}


static void roadmap_start_add_driver (RoadMapIO *io) {

   roadmap_main_set_input (io, roadmap_driver_input);
}

static void roadmap_start_remove_driver (RoadMapIO *io) {

   roadmap_main_remove_input(io);
}


static void roadmap_start_add_driver_server (RoadMapIO *io) {

   roadmap_main_set_input (io, roadmap_driver_accept);
}

static void roadmap_start_remove_driver_server (RoadMapIO *io) {

   roadmap_main_remove_input(io);
}


static void roadmap_start_set_timeout (RoadMapCallback callback) {

   roadmap_main_set_periodic (3000, callback);
}


static int roadmap_start_long_click (RoadMapGuiPoint *point) {
   
   RoadMapPosition position;

   roadmap_math_to_position (point, &position, 1);
   roadmap_trip_set_point ("Selection", &position);
   
   if (LongClickMenu != NULL) {
      roadmap_main_popup_menu (LongClickMenu, point->x, point->y);
   }

   return 1;
}
 

static void roadmap_start_splash (void) {

   int height, width;
   RoadMapImage image;
   RoadMapGuiPoint pos;

   height = roadmap_canvas_height ();
   width = roadmap_canvas_width ();
   
   if (height > width)
   		image = roadmap_canvas_load_image (roadmap_path_user(),
        				                "skins\\default\\welcome.png");
   else
   		image = roadmap_canvas_load_image (roadmap_path_user(),
        				                "skins\\default\\welcome_wide.png");

   if( !image)
      return;

   pos.x = (width - roadmap_canvas_image_width(image)) / 2;
   pos.y = (height - roadmap_canvas_image_height(image)) / 2;
   roadmap_canvas_draw_image (image, &pos, 0, IMAGE_NORMAL);
   roadmap_canvas_free_image (image);
   roadmap_canvas_refresh ();
/*
   Sleep (500);

   const char *message = roadmap_lang_get ("Loading please wait...");
   RoadMapPen pen = roadmap_canvas_create_pen ("splash");
   roadmap_canvas_set_foreground ("red");

   int width;
   int ascent;
   int descent;

   int current_x;

   if (roadmap_lang_rtl ()) {
      current_x = roadmap_canvas_width () - pos.x - 10;
   } else {
      current_x = pos.x + 10;
   }

   for (unsigned int i=0; i<strlen(message); i++) {
      char str[3] = {0};
      RoadMapGuiPoint position = {current_x, 300};

      for (int j=0; j<3; j++) {
         str[j] = message[i];
         if (str[j] != -41) break;
         i++;
      }

      roadmap_canvas_get_text_extents
         (str, -1, &width, &ascent, &descent, NULL);

      if (roadmap_lang_rtl ()) {
         current_x -= width;
         position.x = current_x;
      } else {
         current_x += width;
      }

      roadmap_canvas_draw_string_angle (&position, &position, 0, -1, str);
      roadmap_canvas_refresh ();

      Sleep (30);
   }

   Sleep (1000);
*/
}

static void roadmap_start_realtime (void) {
   roadmap_main_remove_periodic(roadmap_start_realtime);
   if(!Realtime_Initialize())
      roadmap_log( ROADMAP_ERROR, "roadmap_start() - 'Realtime_Initialize()' had failed");
}

void load_resources_on_init(){
		ssd_menu_load_images("alerts", RoadMapStartActions);
		roadmap_border_load_images();
}

static void roadmap_start_window (void) {

   roadmap_main_new (RoadMapMainTitle,
                     roadmap_option_width("Main"),
                     roadmap_option_height("Main"));

#ifndef J2ME
   roadmap_factory ("roadmap",
                    RoadMapStartActions,
                    RoadMapStartMenu,
                    RoadMapStartToolbar);

   LongClickMenu = roadmap_factory_menu ("long_click",
                                         RoadMapStartLongClickMenu,
                                         RoadMapStartActions);

   roadmap_pointer_register_long_click
      (roadmap_start_long_click, POINTER_DEFAULT);

#endif

   roadmap_main_add_canvas ();

   roadmap_main_show ();
   roadmap_start_splash ();
#if 0
   load_resources_on_init();
#endif
   roadmap_gps_register_link_control
      (roadmap_start_add_gps, roadmap_start_remove_gps);

   roadmap_gps_register_periodic_control
      (roadmap_start_set_timeout, roadmap_main_remove_periodic);

   roadmap_driver_register_link_control
      (roadmap_start_add_driver, roadmap_start_remove_driver);

   roadmap_driver_register_server_control
      (roadmap_start_add_driver_server, roadmap_start_remove_driver_server);
}


const char *roadmap_start_get_title (const char *name) {

   static char *RoadMapMainTitleBuffer = NULL;

   int length;


   if (name == NULL) {
      return RoadMapMainTitle;
   }

   length = strlen(RoadMapMainTitle) + strlen(name) + 4;

   if (RoadMapMainTitleBuffer != NULL) {
         free(RoadMapMainTitleBuffer);
   }
   RoadMapMainTitleBuffer = malloc (length);

   if (RoadMapMainTitleBuffer != NULL) {

      strcpy (RoadMapMainTitleBuffer, RoadMapMainTitle);
      strcat (RoadMapMainTitleBuffer, ": ");
      strcat (RoadMapMainTitleBuffer, name);
      return RoadMapMainTitleBuffer;
   }

   return name;
}


static void roadmap_start_after_refresh (void) {

   if (roadmap_download_enabled()) {

      RoadMapGuiPoint download_point = {0, 20};

      download_point.x = roadmap_canvas_width() - 20;
      if (download_point.x < 0) {
         download_point.x = 0;
      }
      roadmap_sprite_draw
         ("Download", &download_point, 0 - roadmap_math_get_orientation());
   }

   if (roadmap_start_prev_after_refresh) {
      (*roadmap_start_prev_after_refresh) ();
   }
}


static void roadmap_start_usage (const char *section) {

   roadmap_factory_keymap (RoadMapStartActions, RoadMapStartKeyBinding);
   roadmap_factory_usage (section, RoadMapStartActions);
}


void roadmap_start_freeze (void) {

   RoadMapStartFrozen = 1;
   RoadMapStartGpsRefresh = 0;

   roadmap_screen_freeze ();
}

void roadmap_start_unfreeze (void) {

   RoadMapStartFrozen = 0;
   roadmap_screen_unfreeze ();
}

void roadmap_start_screen_refresh (int refresh) {

   if (refresh) {
      roadmap_screen_unfreeze ();
   } else {
      roadmap_screen_freeze ();
   }
}

int roadmap_start_is_frozen (void) {

   return RoadMapStartFrozen;
}

static void roadmap_start_set_left_softkey(const char *name, const char*str, RoadMapCallback callback){
	static Softkey s;
	strcpy(s.text, str);
	s.callback = callback;
	roadmap_softkeys_set_left_soft_key(name, &s);
	
}

static void roadmap_start_set_right_softkey(const char *name, const char *str, RoadMapCallback callback){
	static Softkey s;
	strcpy(s.text, str);
	s.callback = callback;
	roadmap_softkeys_set_right_soft_key(name, &s);
	
}

void roadmap_start (int argc, char **argv) {

#ifdef ROADMAP_DEBUG_HEAP
   /* Do not forget to set the trace file using the env. variable MALLOC_TRACE,
    * then use the mtrace tool to analyze the output.
    */
   mtrace();
#endif
   roadmap_config_initialize   ();
   roadmap_config_declare_enumeration
      ("preferences", &RoadMapConfigGeneralUnit, NULL, "imperial", "metric", NULL);
   roadmap_config_declare_enumeration
      ("preferences", &RoadMapConfigGeneralKeyboard, NULL, "yes", "no", NULL);

   roadmap_config_declare
      ("preferences", &RoadMapConfigGeometryMain, "800x600", NULL);

   roadmap_config_declare
      ("preferences", &RoadMapConfigMapPath, "", NULL);

///[BOOKMARK]:[WARNING]:[PAZ] - Skim build with minimal GUI (for testings)
#ifdef TESTING_BUILD
   #pragma message("    WARNING: TESTING BUILD!")
   roadmap_trip_initialize    ();
   roadmap_pointer_initialize ();
   roadmap_screen_initialize  ();
   roadmap_label_initialize   ();
   roadmap_display_initialize ();
   roadmap_history_initialize  ();
   roadmap_sound_initialize   ();
   roadmap_start_window       ();
   roadmap_sprite_initialize  ();
   roadmap_screen_set_initial_position ();
   roadmap_screen_obj_initialize ();
   roadmap_history_load ();
   roadmap_lang_initialize     ();

#else
   roadmap_net_initialize      ();
   roadmap_device_events_init  ();
   roadmap_log_register_msgbox (roadmap_messagebox);
   roadmap_option_initialize   ();
   roadmap_alerter_initialize  ();
   roadmap_math_initialize     ();
   roadmap_trip_initialize     ();
   roadmap_pointer_initialize  ();
   roadmap_screen_initialize   ();
   roadmap_fuzzy_initialize    ();
   roadmap_navigate_initialize ();
   roadmap_label_initialize    ();
   roadmap_display_initialize  ();
   roadmap_voice_initialize    ();
   roadmap_gps_initialize      ();
   roadmap_history_initialize  ();
   roadmap_download_initialize ();
   roadmap_adjust_initialize   ();
   roadmap_driver_initialize   ();
   roadmap_lang_initialize     ();
   roadmap_sound_initialize    ();
   roadmap_device_initialize    ();
   roadmap_phone_keyboard_init ();
#ifdef IPHONE
   roadmap_location_initialize ();
#endif //IPHONE

   roadmap_start_set_title (roadmap_lang_get ("RoadMap"));
   roadmap_gps_register_listener (&roadmap_gps_update);

   RoadMapStartGpsID = roadmap_string_new("GPS");

   roadmap_object_add (roadmap_string_new("RoadMap"),
                       RoadMapStartGpsID,
                       NULL,
                       NULL,
                       NULL);

   roadmap_path_set("maps", roadmap_config_get(&RoadMapConfigMapPath));

#if UNDER_CE
   roadmap_start_init_key_cfg ();
#endif   

   roadmap_option (argc, argv, roadmap_start_usage);

   roadmap_start_set_unit ();
   
   roadmap_math_restore_zoom ();
   roadmap_start_window      ();
   roadmap_factory_keymap (RoadMapStartActions, RoadMapStartKeyBinding);
   roadmap_label_activate    ();
   roadmap_sprite_initialize ();

   roadmap_screen_set_initial_position ();

   roadmap_history_load ();
   
   roadmap_spawn_initialize (argv != NULL ? argv[0] : NULL);

   roadmap_driver_activate ();

   roadmap_help_initialize ();

   roadmap_locator_declare (&roadmap_start_no_download);

   roadmap_start_prev_after_refresh =
      roadmap_screen_subscribe_after_refresh (roadmap_start_after_refresh);

   if (RoadMapStartSubscribers) RoadMapStartSubscribers (ROADMAP_START_INIT);

   /* due to the automatic sync on WinCE, the editor plugin must register
    * first
    */
   editor_main_initialize ();
   
   

   roadmap_state_add ("navigation_guidance_state", navigation_guidance_state);
   roadmap_state_add ("real_time_state", RealTimeLoginState);
   roadmap_state_add ("alerts_state", RTAlerts_State);
   roadmap_state_add ("traffic_info_state", RealtimeTrafficInfoState);
   roadmap_state_add ("skin_state", roadmap_skin_state);
   roadmap_state_add ("scroll_state", RTAlerts_ScrollState);
   roadmap_state_add ("privacy_state", RealtimePrivacytState);
   roadmap_state_add ("softkeys_state", roadmap_softkeys_orientation);
   roadmap_state_add ("wide_screen", is_screen_wide);
   roadmap_state_add ("navigation_list_state", navigate_main_list_state);
   
   roadmap_start_set_left_softkey("Menu","Menu", roadmap_start_quick_menu );
   
   
   roadmap_bar_initialize();
   roadmap_screen_obj_initialize ();

   roadmap_trip_restore_focus ();
   
   
   roadmap_start_set_right_softkey("Report", "Report", start_alerts_menu);
   if (strcmp(roadmap_trip_get_focus_name(), "GPS"))
   		roadmap_screen_add_focus_on_me_softkey();   
      
   roadmap_gps_open ();

   // Set the input mode for the starting screen to the numeric
   roadmap_input_type_set_mode( inputtype_numeric );
   
   if (! roadmap_trip_load (roadmap_trip_current(), 1)) {
      roadmap_start_create_trip ();
   }

   RTTrafficInfo_Init();
   navigate_main_initialize ();

   roadmap_screen_redraw ();
   
   // Start 'realtime':
   roadmap_main_set_periodic(4000, roadmap_start_realtime);
#ifdef J2ME
   roadmap_factory ("roadmap",
                    RoadMapStartActions,
                    RoadMapStartMenu,
                    RoadMapStartToolbar);
#endif

   // Register for keyboard callback:
   roadmap_keyboard_register_to_event__key_pressed( on_key_pressed);

   roadmap_main_set_periodic (200, roadmap_start_periodic);
#endif   // TESTING_BUILD   
}


void roadmap_start_exit (void) {
    
   // Terminate 'realtime' engine:
   Realtime_Terminate();

   // Un-register from keyboard callback:
   roadmap_keyboard_unregister_from_event__key_pressed( on_key_pressed);
   
    navigate_main_shutdown ();
    roadmap_main_set_cursor (ROADMAP_CURSOR_WAIT);
    roadmap_plugin_shutdown ();
    roadmap_driver_shutdown ();
    roadmap_sound_shutdown ();
    roadmap_net_shutdown ();
    roadmap_history_save ();
    roadmap_screen_shutdown ();
#ifndef __SYMBIAN32__    
    roadmap_start_save_trip ();
#endif
    roadmap_config_save (0);
    editor_main_shutdown ();
    roadmap_db_end ();
    roadmap_gps_shutdown ();
    roadmap_res_shutdown ();
    roadmap_device_events_term();
    roadmap_phone_keyboard_term();
#ifndef J2ME
    roadmap_main_set_cursor (ROADMAP_CURSOR_NORMAL);
#endif
}


const RoadMapAction *roadmap_start_find_action (const char *name) {

   const RoadMapAction *actions = RoadMapStartActions;

   while (actions->name != NULL) {
      if (strcmp (actions->name, name) == 0) return actions;
      ++actions;
   }

   return NULL;
}


void roadmap_start_set_title (const char *title) {
   RoadMapMainTitle = title;
}


RoadMapStartSubscriber roadmap_start_subscribe
                                 (RoadMapStartSubscriber handler) {

   RoadMapStartSubscriber previous = RoadMapStartSubscribers;

   RoadMapStartSubscribers = handler;

   return previous;
}


int roadmap_start_add_action (const char *name, const char *label_long,
                              const char *label_short, const char *label_terse,
                              const char *tip, RoadMapCallback callback) {
                    
   int i;
   RoadMapAction action;
   
   action.name = name;
   action.label_long = label_long;
   action.label_short = label_short;
   action.label_terse = label_terse;
   action.tip = tip;
   action.callback = callback;

   for (i=0; i<MAX_ACTIONS; i++) {
      if (RoadMapStartActions[i].name == NULL) break;
   }

   if (i == MAX_ACTIONS) {
      roadmap_log (ROADMAP_ERROR, "Too many actions.");
      return -1;
   }

   RoadMapStartActions[i] = action;
   RoadMapStartActions[i+1].name = NULL;

   return 0;
}
 

void roadmap_start_context_menu (const RoadMapGuiPoint *point) {
   
#ifdef TOUCH_SCREEN
#ifdef SSD
   int count;

   if( !s_main_menu)
   {
      assert(0);
      return;
   }

   if( !get_menu_item_names( "Context", s_main_menu, grid_menu_labels, &count))
   {
      assert(0);
      return;
   }      

   ssd_menu_activate("Context", 
                     NULL, 
                     grid_menu_labels, 
                     NULL,
                     RoadMapStartActions,
                     SSD_CONTAINER_BORDER|SSD_CONTAINER_TITLE|SSD_DIALOG_FLOAT|SSD_ROUNDED_CORNERS|SSD_POINTER_MENU|SSD_HEADER_GRAY);
#else   
   if (LongClickMenu == NULL) return;

   roadmap_main_popup_menu (LongClickMenu, point->x, point->y);
#endif   
#endif   
}
 

#ifdef SSD
void roadmap_start_popup_menu (const char *name,
                               const char *items[],
                               PFN_ON_DIALOG_CLOSED on_dialog_closed,
                               const RoadMapGuiPoint *point) {
   
   int height = roadmap_canvas_height();
   int flags = 0;

   if (point->y > height / 2) {
      flags = SSD_ALIGN_BOTTOM;
   }

   ssd_menu_activate (name, "", items, on_dialog_closed, RoadMapStartActions,
                      SSD_DIALOG_FLOAT|SSD_CONTAINER_BORDER|SSD_ROUNDED_CORNERS|flags);
}
#endif   


#ifdef SSD
void roadmap_start_hide_menu (const char *name) {
   ssd_menu_hide (name);
}
#endif

 
void roadmap_start_redraw (void) {
   roadmap_screen_redraw ();
}


static void roadmap_confirmed_exit_callback(int exit_code, void *context){
    if (exit_code != dec_yes)
         return;

    roadmap_main_exit();
}

void roadmap_confirmed_exit(){
   ssd_confirm_dialog ("", "Exit?", TRUE, roadmap_confirmed_exit_callback , NULL);
}

static BOOL on_key_pressed( const char* utf8char, uint32_t flags)
{
   BOOL  key_handled = TRUE;

   if( ssd_dialog_is_currently_active())
      return FALSE;
   
   if( flags & KEYBOARD_VIRTUAL_KEY)
   {
      switch( *utf8char)
      {
         case VK_Back:
            break;   // return TRUE so application will not be closed...
      
         case VK_Arrow_up:
            roadmap_screen_hold ();
            roadmap_screen_move_up();
            break;
      
         case VK_Arrow_down:
            roadmap_screen_hold ();
            roadmap_screen_move_down();
            break;
      
         case VK_Arrow_left:            
            roadmap_screen_hold ();           
            roadmap_screen_move_left();
            break;
      
         case VK_Arrow_right:
            roadmap_screen_hold ();             
            roadmap_screen_move_right();
            break;

         case VK_Softkey_left:
            roadmap_softkeys_left_softkey_callback();
            break;

         case VK_Softkey_right:
            roadmap_softkeys_right_softkey_callback();
            break;
            
         default:
            key_handled = FALSE;
            
      }
   }
   else
   {
      if( flags & KEYBOARD_ASCII)
      {
         switch( *utf8char)
         {
            case ENTER_KEY:
               if (RTAlerts_ScrollState() == 0){
                  RealtimeAlertCommentsList(RTAlerts_Get_Current_Alert_Id());
               }
               else
                  display_shortcuts();
               break;

            case SPACE_KEY:
            case '0':
               search_menu_search_favorites();
               break;

            case 'd':
            case 'D':
            case '5':
               roadmap_screen_toggle_view_mode();
               break;
               
            case 's':
            case 'S':
               roadmap_skin_toggle();
               break;
               
            case 'n':
            case 'N':
               roadmap_address_history();
               break;            
            
            case 'h':
            case 'H':
               //show_keyboard_hotkeys_map();
               break;
               
            case 'q':
            case 'Q':
            case '9':
               roadmap_confirmed_exit();
               return TRUE;
               break;

            case 'm':
            case 'M':
                roadmap_start_quick_menu();
               break;

            case 'v':
            case 'V':
            case '4':
               RTAlerts_Scroll_Prev();
               break;
               
            case 't':
            case 'T':
            case '6':
               RTAlerts_Scroll_Next();
               break;
               

		    case '7':
		       start_map_updates_menu();
		       break;
		       
            case 'y':
            case 'Y':
            case '8':
               RTAlerts_Cancel_Scrolling();
               break;

            case 'z':
            case 'Z':
               roadmap_softkeys_right_softkey_callback();
               break;
            
            case '+':
            case '*':
               roadmap_screen_zoom_in();
               break;

            case '-':
            case '#':
               roadmap_screen_zoom_out();
               break;

            case '1':
               roadmap_screen_hold ();  
               roadmap_screen_toggle_orientation_mode();
               break;

            case '2':
               roadmap_skin_toggle();
               break;

			case '3':
			   toggle_navigation_guidance();
			   break;

            default:
               key_handled = FALSE;
         }
      }
      else
         key_handled = FALSE;
   }
   
   if( key_handled)
      roadmap_screen_redraw();
   
   return key_handled;
}

const char* roadmap_start_version() {
   static char version[64];
   static int done = 0;
   
   if( done)
      return version;
      
   sprintf( version, 
            "%d.%d.%d.%d", 
            CLIENT_VERSION_MAJOR,
            CLIENT_VERSION_MINOR,
            CLIENT_VERSION_SUB,
            CLIENT_VERSION_CFG);
            
   done = TRUE;
   return version;
}


