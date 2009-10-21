/* roadmap_skin.c - manage skins
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
 * DESCRIPTION:
 *
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "roadmap.h"
#include "roadmap_main.h"
#include "roadmap_path.h"
#include "roadmap_config.h"
#include "roadmap_screen.h"
#include "roadmap_plugin.h"
#include "roadmap_gps.h"
#include "roadmap_navigate.h"
#include "roadmap_skin.h"
#include "roadmap_sunrise.h"
#include "roadmap_trip.h"
#include "roadmap_config.h"

#define MAX_LISTENERS 16
static RoadMapCallback RoadMapSkinListeners[MAX_LISTENERS] = {NULL};
static const char *CurrentSkin = "default";
static const char *CurrentSubSkin = "day";

RoadMapConfigDescriptor RoadMapConfigAutoNightMode =
                        ROADMAP_CONFIG_ITEM("Display", "Auto night mode");


static void toggle_skin_timer(void);

static void notify_listeners (void) {
   int i;

   for (i = 0; i < MAX_LISTENERS; ++i) {

      if (RoadMapSkinListeners[i] == NULL) break;

      (RoadMapSkinListeners[i]) ();
   }

}


void roadmap_skin_register (RoadMapCallback listener) {

   int i;

   for (i = 0; i < MAX_LISTENERS; ++i) {
      if (RoadMapSkinListeners[i] == NULL) {
         RoadMapSkinListeners[i] = listener;
         break;
      }
   }
}


void roadmap_skin_set_subskin (const char *sub_skin) {
   const char *base_path = roadmap_path_preferred ("skin");
   char path[255];
   char *skin_path;
   char *subskin_path;

   CurrentSubSkin = sub_skin;

   skin_path = roadmap_path_join (base_path, CurrentSkin);
   subskin_path = roadmap_path_join (skin_path, CurrentSubSkin);

   snprintf (path, sizeof(path), "%s,%s", subskin_path, skin_path);

   roadmap_path_set ("skin", path);

   roadmap_path_free (subskin_path);
   roadmap_path_free (skin_path);

   roadmap_config_reload ("schema");
   notify_listeners ();

   roadmap_screen_redraw ();
}


void roadmap_skin_toggle (void) {
   if (!strcmp (CurrentSubSkin, "day")) {
      roadmap_skin_set_subskin ("night");
   } else {
      roadmap_skin_set_subskin ("day");
   }
}


int roadmap_skin_state_screen_touched(void){

  if (roadmap_screen_touched_state() == -1)
  		return -1;

   if (!strcmp (CurrentSubSkin, "day")) {
	   return 0;
   } else {
      return 1;
   }
}


int roadmap_skin_state(void){

   if (!strcmp (CurrentSubSkin, "day")) {
	   return 0;
   } else {
      return 1;
   }
}


void roadmap_skin_auto_night_mode_kill_timer(void){
   roadmap_main_remove_periodic(toggle_skin_timer);
}

static void toggle_skin_timer(void){
   roadmap_skin_toggle();
   roadmap_skin_auto_night_mode_kill_timer();
}

static int auto_night_mode_cfg_on (void) {
   return (roadmap_config_match(&RoadMapConfigAutoNightMode, "yes"));
}

static void roadmap_skin_gps_listener
               (time_t gps_time,
                const RoadMapGpsPrecision *dilution,
                const RoadMapGpsPosition *position){
   time_t rawtime_sunset, rawtime_sunrise;
   struct tm *realtime;
   struct tm *realtime2;
   static int has_run = 0;
   time_t now ;
   int timer_t;
   //time_t now = time(NULL);

#ifdef _WIN32
   struct tm curtime_gmt;
#endif

   if ((gps_time + (3600 * 24)) < time(NULL))
      now = time(NULL);
   else
      now = gps_time;

#ifdef _WIN32
   curtime_gmt = *(gmtime(&now));
   now  = mktime(&curtime_gmt);
#endif
   realtime = localtime (&now);
//   printf ("gpstime %s", asctime (realtime) );

   roadmap_gps_unregister_listener(roadmap_skin_gps_listener);

   if (has_run)
      return;


   has_run = TRUE;

   rawtime_sunrise = roadmap_sunrise (position, now);
   realtime = localtime (&rawtime_sunrise);

//   printf ("sunrise: %d:%d\n", realtime->tm_hour, realtime->tm_min);
//   printf ("sunrise %s", asctime (realtime) );

   rawtime_sunset = roadmap_sunset (position, now);
   realtime2 = localtime (&rawtime_sunset);

//   printf ("sunset:  %d:%d\n", realtime->tm_hour, realtime->tm_min);
//   printf ("sunset %s", asctime (realtime) );

   if (rawtime_sunset > rawtime_sunrise){
      roadmap_skin_set_subskin ("night");
      timer_t = rawtime_sunrise - now;
      if ((timer_t > 0) && (timer_t < 1800))
         roadmap_main_set_periodic (timer_t*1000, toggle_skin_timer);
   }
   else{
      timer_t = rawtime_sunset - now;
      if ((timer_t > 0) && (timer_t < 1800))
         roadmap_main_set_periodic (timer_t*1000, toggle_skin_timer);
   }

}

void roadmap_skin_auto_night_mode(){

   roadmap_config_declare_enumeration
      ("user", &RoadMapConfigAutoNightMode, NULL, "yes", "no", NULL);

   if (!auto_night_mode_cfg_on())
      return;

   roadmap_gps_register_listener (roadmap_skin_gps_listener);

}

