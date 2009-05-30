/* navigate_main.h - main plugin file
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
 */

#ifndef INCLUDE__NAVIGATE_MAIN__H
#define INCLUDE__NAVIGATE_MAIN__H

#include "roadmap_canvas.h"
#include "roadmap_plugin.h"
#include "roadmap_screen.h"
#include "roadmap_line.h"

enum NavigateInstr {
   TURN_LEFT = 0,
   TURN_RIGHT,
   KEEP_LEFT,
   KEEP_RIGHT,
   CONTINUE,
   ROUNDABOUT_ENTER,
   ROUNDABOUT_EXIT,
   ROUNDABOUT_LEFT,
   ROUNDABOUT_EXIT_LEFT,
   ROUNDABOUT_STRAIGHT,
   ROUNDABOUT_EXIT_STRAIGHT,
   ROUNDABOUT_RIGHT,
   ROUNDABOUT_EXIT_RIGHT,
   ROUNDABOUT_U,
   ROUNDABOUT_EXIT_U,
   APPROACHING_DESTINATION,
   LAST_DIRECTION
};

typedef struct {
   PluginLine           line;
   int                  line_direction;
   PluginStreet         street;
   RoadMapPosition      from_pos;
   RoadMapPosition      to_pos;
   RoadMapPosition      shape_initial_pos;
   int                  first_shape;
   int                  last_shape;
   RoadMapShapeItr      shape_itr;
   SegmentContext			context;
   int						exit_no;
   enum NavigateInstr   instruction;
   int                  group_id;
   int                  distance;
   int                  cross_time;
} NavigateSegment;
 
int navigate_is_enabled (void);
int navigate_track_enabled(void);
int navigate_is_line_on_route(int square_id, int line_id, int from_line, int to_line);
void navigate_main_shutdown (void);
void navigate_main_initialize (void);
int navigation_guidance_state(void);
void toggle_navigation_guidance(void);
int  navigate_main_reload_data (void);
void navigate_main_set (int status);
int  navigate_main_calc_route (void);

void navigate_main_screen_repaint (int max_pen);

int navigate_main_override_pen (int line,
                                int cfcc,
                                int fips,
                                int pen_type,
                                RoadMapPen *override_pen);

void navigate_main_adjust_layer (int layer, int thickness, int pen_count);

void navigate_main_stop_navigation(void);
void navigate_main_stop_navigation_menu(void);


void navigate_auto_zoom_suspend(void);
void navigate_get_waypoint (int distance, RoadMapPosition *way_point);
int navigate_is_auto_zoom (void);

void navigate_main_list(void);
int  navigate_main_is_list_displaying(void);
int  navigate_main_list_state(void);
void navigate_main_list_display_previous(void);
void navigate_main_list_display_next(void);

#endif /* INCLUDE__NAVIGATE_MAIN__H */

