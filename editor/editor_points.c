/* editor_points.c - New roads points 
 *
 * LICENSE:
 *
 *   Copyright 2008 Ehud Shabtai
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
 *   See editor_points.h 
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "roadmap.h"
#include "roadmap_main.h"
#include "roadmap_message.h"
#include "roadmap_screen.h"
#include "roadmap_ticker.h"
#include "roadmap_config.h"
#include "roadmap_sound.h"
#include "roadmap_res.h"
#include "db/editor_override.h"

static int gTotalNewPoints;
static int gOldPoints;

static RoadMapConfigDescriptor NewPointsSessionCfg =
                        ROADMAP_CONFIG_ITEM("Points", "New Points");

static RoadMapConfigDescriptor OldPointsSessionCfg =
                        ROADMAP_CONFIG_ITEM("Points", "Old Points");

static RoadMapCallback TimerCallback = NULL;

static void editor_points_set_saved_new_points(int new_points){
   roadmap_config_set_integer(&NewPointsSessionCfg, new_points);
}

static void editor_points_set_saved_old_points(int old_points){
   roadmap_config_set_integer(&OldPointsSessionCfg, old_points);
   roadmap_config_save(0);
}

static int editor_points_get_saved_new_points(){
   return roadmap_config_get_integer(&NewPointsSessionCfg);
}

static int editor_points_get_saved_old_points(){
   return roadmap_config_get_integer(&OldPointsSessionCfg);
}

//////////////////////////////////////////////////////////////////////
void editor_points_initialize (void) {
	
  roadmap_config_declare ("session", &NewPointsSessionCfg, "0", NULL);

  roadmap_config_declare ("session", &OldPointsSessionCfg, "0", NULL);
  gOldPoints = editor_points_get_saved_old_points();
  
  gTotalNewPoints = editor_points_get_saved_new_points();
}

//////////////////////////////////////////////////////////////////////
void editor_points_hide (void) {
  roadmap_message_unset('*');

}

//////////////////////////////////////////////////////////////////////
void editor_points_display(int road_length){
   
   int points = road_length/10; 
   if (points > 0)
      roadmap_message_set('*', "%d", points);
   
}

//////////////////////////////////////////////////////////////////////
void editor_points_display_new_points(int points){
   roadmap_message_set('*', "%d", points);
}

//////////////////////////////////////////////////////////////////////
static void timer_cb(void){
   roadmap_message_unset('X');
   roadmap_main_remove_periodic (timer_cb);
   TimerCallback = NULL;
   if (!roadmap_screen_refresh())
      roadmap_screen_redraw();
   roadmap_ticker_set_last_event(default_event);
}

//////////////////////////////////////////////////////////////////////
void editor_points_display_new_points_timed(int points, int seconds, int event){
   roadmap_message_set('X', "%d", points);
   if (TimerCallback){
      roadmap_main_remove_periodic (timer_cb);
      TimerCallback = NULL;
   }
   roadmap_ticker_set_last_event(event);
   if (!roadmap_screen_refresh())
     roadmap_screen_redraw();
   roadmap_main_set_periodic (seconds * 1000, timer_cb);
   TimerCallback = timer_cb;
}

//////////////////////////////////////////////////////////////////////
void editor_points_add_new_points(int points){
   gTotalNewPoints += points;
   editor_points_set_saved_new_points(gTotalNewPoints);
}

//////////////////////////////////////////////////////////////////////
void editor_points_add(int road_length){
   int points = road_length/10; 
   if (points > 0){
      gTotalNewPoints += points;
      editor_points_set_saved_new_points(gTotalNewPoints);
   }
}

//////////////////////////////////////////////////////////////////////
int editor_points_get_new_points(void){
	return gTotalNewPoints;
}

//////////////////////////////////////////////////////////////////////
void editor_points_set_old_points(int points){
   gOldPoints = points;
	if (editor_points_get_saved_old_points() != points){
	   editor_points_set_saved_new_points(0);
	   gTotalNewPoints = 0;
	}

	editor_points_set_saved_old_points(points);
}

//////////////////////////////////////////////////////////////////////
int editor_points_get_old_points(void){
	return gOldPoints;
}


int editor_points_get_total_points(void){
   return gOldPoints + gTotalNewPoints; 
}
