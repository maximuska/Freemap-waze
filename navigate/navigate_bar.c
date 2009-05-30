 /* navigate_bar.c - implement navigation bar
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
 *   See navigate_bar.h
 */

#include <stdlib.h>
#include <string.h>
#include "roadmap.h"
#include "roadmap_canvas.h"
#include "roadmap_screen_obj.h"
#include "roadmap_file.h"
#include "roadmap_res.h"
#include "roadmap_math.h"
#include "roadmap_lang.h"
#include "roadmap_message.h"
#include "roadmap_navigate.h"


#include "navigate_main.h"
#include "navigate_bar.h"

typedef struct {
   const char     *image_file;
   int                 min_screen_width;
   int 						start_y_pos;
   RoadMapGuiPoint instruction_pos;
   RoadMapGuiPoint exit_pos;
   RoadMapGuiRect  distance_rect;
   RoadMapGuiPoint distance_value_pos;
   RoadMapGuiPoint distance_unit_pos;
   int				   street_line1_pos;
   int				   street_line2_pos;
   int             street_start;
   int             street_width;
   
   RoadMapGuiRect  speed_rect;
   RoadMapGuiPoint speed_pos;
   RoadMapGuiPoint speed_unit_pos;
      
   RoadMapGuiRect  distance_to_destination_rect;
   RoadMapGuiPoint distance_to_destination_pos;
	RoadMapGuiPoint distance_to_destination_unit_pos;

   RoadMapGuiRect  time_to_destination_rect;
   RoadMapGuiPoint time_to_destination_pos;
	
   
} NavigateBarPanel;

#define  NAV_BAR_TEXT_COLOR                   ("#d7ff00")
#define  NAV_BAR_TEXT_COLOR_GRAY              ("#d7ff00")

#define NAV_BAR_TIME_TO_DESTINATION_COLOR     ("#FFFFFF")
#define NAV_BAR_DIST_TO_DESTINATION_COLOR     ("#FFFFFF")
#define NAV_BAR_SPEED_COLOR                   ("#000000")

static NavigateBarPanel NavigateBarDefaultPanels[] = {
#ifndef IPHONE 
//   {"nav_panel_wide400", 400,0, {0, 19}, {25, 31}, {43, 20, 95, 55}, {52, 25}, {48, 46}, 39,-1, 90, 300, {330, 0, 400, 37}, {356, 8}, {357, 32}, {600, 0, 600, 30}, {600, 600}, {600, 600}, {600, 0, 60, 22}, {600,600} },

   {"nav_panel_wide", 320,0, {0, 19}, {25, 31}, {43, 20, 95, 55}, {52, 25}, {48, 46}, 39,-1, 72, 245, {270, 0, 320, 37}, {276, 8}, {277, 32}, {400, 0, 400, 30}, {400, 400}, {400, 400}, {400, 0, 400, 22}, {400,400} },
   	
   {"nav_panel", 240, 21, {6, 23}, {31, 35}, {0, 54, 78, 89}, {32, 60}, {10, 70}, 48, -1, 74, 165, {190, 8, 240, 41}, {196, 12}, {195, 36}, {129, 66, 200, 83}, {188, 72}, {140, 72}, {87, 66, 130, 83}, {88, 70} },
#else
   {"nav_panel_wide", 400,0, {0, 19}, {25, 31}, {63, 20, 120, 55}, {72, 25}, {68, 46}, 39,-1, 72, 405, {430, 0, 480, 37}, {436, 6}, {437, 30}, {700, 0, 700, 29}, {700, 700}, {700, 700}, {700, 0, 700, 22}, {700,700} },

   {"nav_panel", 240, 40, {6, 28}, {31, 40}, {0, 59, 78, 94}, {32, 65}, {10, 75}, 50, -1, 74, 240, {270, 8, 325, 41}, {271, 13}, {279, 38}, {129, 71, 210, 88}, {207, 75}, {140, 75}, {87, 71, 130, 88}, {88, 75} },  

   {"nav_panel", 240, 21, {6, 33}, {31, 40}, {0, 64, 78, 99}, {32, 70}, {10, 80}, 48, -1, 74, 225, {250, 8, 300, 41}, {256, 12}, {255, 36}, {129, 66, 200, 83}, {188, 70}, {140, 70}, {87, 66, 130, 83}, {88, 70} },
 #endif
   {"nav_panel_small", 176, 0, {11, 16}, {29, 32}, {0, 50, 78, 83}, {33, 59}, {10, 67}, 87, 103, 0, 165, {195, 60, 240, 110}, {202, 68}, {205, 90}, {200, 2, 234, 31}, {230, 10}, {210, 26}, {200, 36, 240, 56}, {204, 44} }
};

static NavigateBarPanel *NavigatePanel = NULL;
static RoadMapScreenSubscriber navigate_prev_after_refresh = NULL;


const char NAVIGATE_DIR_IMG[][40] = {
   "nav_turn_left",
   "nav_turn_right",
   "nav_keep_left",
   "nav_keep_right",
   "nav_continue",
   "nav_roundabout_e",
   "nav_roundabout_e",
   "nav_roundabout_l",
   "nav_roundabout_l",
   "nav_roundabout_s",
   "nav_roundabout_s",
   "nav_roundabout_r",
   "nav_roundabout_r",
   "nav_roundabout_u",
   "nav_roundabout_u",
   "nav_approaching"
};

static RoadMapImage NavigateBarImage;
static RoadMapImage NavigateBarBG;
RoadMapImage NavigateDirections[LAST_DIRECTION];
static int NavigateBarInitialized = 0;
static RoadMapGuiPoint NavigateBarLocation;

static enum NavigateInstr NavigateBarCurrentInstr = LAST_DIRECTION;
static char *NavigateBarCurrentStreet = NULL;
static int  NavigateBarCurrentDistance = -1;
static int  NavigateBarEnabled = 0;

static int height;

static int navigate_bar_align_text (char *text, char **line1, char **line2,
                                    int size) {

   int width, ascent, descent;

   roadmap_canvas_get_text_extents
      (text, size, &width, &ascent, &descent, NULL);

   if (width >= 2 * NavigatePanel->street_width) return -1;

   if (width < NavigatePanel->street_width) {

      *line1 = text;
      return 1;

   } else {

      /* Check if we can place the text in two lines */

      char *text_line = text;
      char *text_end = text_line + strlen(text_line);
      char *p1 = text_line + (strlen(text_line) / 2);
      char *p2 = p1;

      while (p1 > text_line) {
         if (*p1 == ' ') {
            break;
         }
         p1 -= 1;
      }
      while (p2 < text_end) {
         if (*p2 == ' ') {
            break;
         }
         p2 += 1;
      }
      if (text_end - p1 > p2 - text_line) {
         p1 = p2;
      }
      if (p1 > text_line) {

         char saved = *p1;
         *p1 = 0;

         roadmap_canvas_get_text_extents
            (text_line, size, &width, &ascent, &descent, NULL);

         if (width < NavigatePanel->street_width) {

            roadmap_canvas_get_text_extents
               (text_line, size, &width, &ascent, &descent, NULL);

            if (width < NavigatePanel->street_width) {

               *line1 = text_line;
               *line2 = p1 + 1;
               return 2;
            }
         }

         *p1 = saved;
      }
   }

   return -1;
}

void navigate_bar_resize(void){
   int i;
   int width;
   
   width = roadmap_canvas_width ();
	height = roadmap_canvas_height();
	
   for (i=0;
       (unsigned)i<
        sizeof(NavigateBarDefaultPanels)/sizeof(NavigateBarDefaultPanels[0]);
       i++) {

      if (width >= NavigateBarDefaultPanels[i].min_screen_width) {
         NavigatePanel = NavigateBarDefaultPanels + i;
         break;
      }
   }

   if (!NavigatePanel) {
      roadmap_log (ROADMAP_ERROR, "Can't find nav panel for screen width: %d",
            width);
      NavigateBarInitialized = -1;
      return;
   }

   NavigateBarBG =
      (RoadMapImage) roadmap_res_get
         (RES_BITMAP, RES_SKIN|RES_NOCACHE, NavigatePanel->image_file);

   NavigateBarImage =
      (RoadMapImage) roadmap_res_get
         (RES_BITMAP, RES_SKIN, NavigatePanel->image_file);

   if (!NavigateBarBG || !NavigateBarImage) return;

   roadmap_canvas_image_set_mutable (NavigateBarImage);
   
   NavigateBarLocation.x = 0;
   NavigateBarLocation.y = height - roadmap_canvas_image_height(NavigateBarBG) - NavigatePanel->start_y_pos ;
   
}

static void navigate_bar_after_refresh (void) {

	int new_height;
	
	
   if (NavigateBarEnabled) {
		new_height = roadmap_canvas_height();
		if (new_height != height){
   			navigate_bar_resize();
   			navigate_bar_set_instruction(NavigateBarCurrentInstr);
   			navigate_bar_set_street(NavigateBarCurrentStreet);
   			navigate_bar_set_distance(NavigateBarCurrentDistance);
   			roadmap_screen_refresh();
		}
		navigate_bar_set_speed();
	    navigate_bar_set_distance_to_destination();
	    navigate_bar_set_time_to_destination();
		
      navigate_bar_draw ();
   }

   if (navigate_prev_after_refresh) {
      (*navigate_prev_after_refresh) ();
   }
}



void navigate_bar_initialize (void) {

   int i;
   int width;
   

   if (NavigateBarInitialized) return;
      
   width = roadmap_canvas_width ();
	height = roadmap_canvas_height();
	
   for (i=0;
       (unsigned)i<
        sizeof(NavigateBarDefaultPanels)/sizeof(NavigateBarDefaultPanels[0]);
       i++) {

      if (width >= NavigateBarDefaultPanels[i].min_screen_width) {
         NavigatePanel = NavigateBarDefaultPanels + i;
         break;
      }
   }

   if (!NavigatePanel) {
      roadmap_log (ROADMAP_ERROR, "Can't find nav panel for screen width: %d",
            width);
      NavigateBarInitialized = -1;
      return;
   }

   NavigateBarBG =
      (RoadMapImage) roadmap_res_get
         (RES_BITMAP, RES_SKIN|RES_NOCACHE, NavigatePanel->image_file);

   NavigateBarImage =
      (RoadMapImage) roadmap_res_get
         (RES_BITMAP, RES_SKIN, NavigatePanel->image_file);

   if (!NavigateBarBG || !NavigateBarImage) goto error;

   roadmap_canvas_image_set_mutable (NavigateBarImage);

   for (i=0; i<LAST_DIRECTION; i++) {
      NavigateDirections[i] =
         (RoadMapImage) roadmap_res_get
               (RES_BITMAP, RES_SKIN, NAVIGATE_DIR_IMG[i]);

      if (!NavigateDirections[i]) goto error;
   }
      

   NavigateBarLocation.x = 0;
   NavigateBarLocation.y = height - roadmap_canvas_image_height(NavigateBarBG) - NavigatePanel->start_y_pos;

   NavigateBarInitialized = 1;
   navigate_prev_after_refresh = 
      roadmap_screen_subscribe_after_refresh (navigate_bar_after_refresh);

   return;

error:
   NavigateBarInitialized = -1;
}
 

void navigate_bar_set_instruction (enum NavigateInstr instr) {

 
   RoadMapGuiPoint pos0 = {0,0};
   RoadMapGuiPoint pos = NavigatePanel->instruction_pos;

   if (NavigateBarInitialized != 1) return;
   
   roadmap_canvas_copy_image (NavigateBarImage, &pos0, NULL, NavigateBarBG,
                              CANVAS_COPY_NORMAL);

   roadmap_canvas_copy_image (NavigateBarImage, &pos, NULL,
                              NavigateDirections[(int)instr],
                              CANVAS_COPY_BLEND);

   NavigateBarCurrentInstr = instr;

}


void navigate_bar_set_exit (int exit) {

	char text[3];
	
	if (exit < 0 || exit > 9) return; 
	
	sprintf (text, "%d", exit);
	
	roadmap_canvas_create_pen ("nav_bar_pen");
  	roadmap_canvas_set_foreground(NAV_BAR_TEXT_COLOR);
	
   roadmap_canvas_draw_image_text
      (NavigateBarImage, &NavigatePanel->exit_pos, 20, text);
}


void navigate_bar_set_distance (int distance) {

   char str[100];
   char unit_str[20];
   int  distance_far;
   RoadMapGuiPoint position = {0, 0};
   RoadMapPen nav_bar_pen;
   int font_size = 22;
   
   #ifdef IPHONE
   	font_size = 16;
   #endif
   
   if (NavigateBarInitialized != 1) return;

   NavigateBarCurrentDistance = distance;
   /* erase the old distance */
   roadmap_canvas_copy_image (NavigateBarImage, &position,
                              &NavigatePanel->distance_rect,
                              NavigateBarBG,
                              CANVAS_COPY_NORMAL);

   distance_far =
      roadmap_math_to_trip_distance(distance);

   if (distance_far > 0) {

      int tenths = roadmap_math_to_trip_distance_tenths(distance);
      if (distance_far < 100)
      	snprintf (str, sizeof(str), "%d.%d", distance_far, tenths % 10);
      else
      	snprintf (str, sizeof(str), "%d", distance_far);	
      snprintf (unit_str, sizeof(unit_str), "%s", roadmap_lang_get(roadmap_math_trip_unit()));
   } else {

      snprintf (str, sizeof(str), "%d", distance);
      snprintf (unit_str, sizeof(unit_str), "%s",
                roadmap_lang_get(roadmap_math_distance_unit()));
   };

	nav_bar_pen = roadmap_canvas_create_pen ("nav_bar_pen");
  	roadmap_canvas_set_foreground(NAV_BAR_TEXT_COLOR);
	
   position = NavigatePanel->distance_value_pos;
   roadmap_canvas_draw_image_text
      (NavigateBarImage, &NavigatePanel->distance_value_pos, font_size, str);


   position = NavigatePanel->distance_unit_pos;
   roadmap_canvas_draw_image_text
      (NavigateBarImage, &NavigatePanel->distance_unit_pos, 12, unit_str);
}


void navigate_bar_set_street (const char *street) {

#define NORMAL_SIZE 20
#define SMALL_SIZE  18

   int width, ascent, descent;
   int size;
   char *line1;
   char *line2;
   char *text;
   int num_lines;
   int i;
   RoadMapPen nav_bar_pen;

   //street = "רחוב המלך הגדולי מלקט פרחים 18";
   if (NavigateBarInitialized != 1) return;

   if (street == NULL)
   	return;
   	
   NavigateBarCurrentStreet = strdup(street);
   
   text = strdup(street);

   size = NORMAL_SIZE;

   num_lines = navigate_bar_align_text (text, &line1, &line2, size);

   if ((num_lines < 0)  || ((num_lines>1) && (NavigatePanel->street_line2_pos == -1))){
      /* Try again with a smaller font size */
	  text = strdup(street);
      size = SMALL_SIZE;
      num_lines = navigate_bar_align_text (text, &line1, &line2, size);
   }

   
   /* Cut some text until it fits */
   while ((num_lines < 0) || ((num_lines > 1) && (NavigatePanel->street_line2_pos == -1))) {

      char *end = text + strlen(text) - 1;

      while ((end > text) && (*end != ' ')) end--;
      if (end == text) {

         roadmap_log (ROADMAP_ERROR, "Can't align street in nav bar: %s",
                      street);

         free(text);
         return;
      }

      *end = '\0';
      num_lines = navigate_bar_align_text (text, &line1, &line2, size);
   }

	if (NavigatePanel->street_line2_pos == -1){
   	/* Cut some text until it fits */
   	while (num_lines > 1) {

      	char *end = text + strlen(text) - 1;

      	while ((end > text) && (*end != ' ')) end--;

      	if (end == text) {

         		roadmap_log (ROADMAP_ERROR, "Can't align street in nav bar: %s",
                      	street);

		         free(text);
      		   return;
      	}

      	*end = '\0';
      	num_lines = navigate_bar_align_text (text, &line1, &line2, size);
   	}
	}

   for (i=0; i < num_lines; i++) {

      char *line;
      RoadMapGuiPoint position;
      
      position.x = NavigatePanel->street_start;
      position.y = NavigatePanel->street_line1_pos;
       
		if ((num_lines == 1) && (NavigatePanel->street_line2_pos != -1)){
			position.y = (NavigatePanel->street_line1_pos + NavigatePanel->street_line2_pos) / 2;
		}

      if (i ==0 ) {
         line = line1;
      } else {
         line = line2;
#ifndef IPHONE //TODO:  (y) should not be hard coded
         position.y = NavigatePanel->street_line2_pos;
#else
         position.y = 122;
#endif
      }

      	roadmap_canvas_get_text_extents
         	(line, size, &width, &ascent, &descent, NULL);
      
      	position.x = NavigatePanel->street_width - width + NavigatePanel->street_start;
		
   		nav_bar_pen = roadmap_canvas_create_pen ("nav_bar_pen");
  	   	roadmap_canvas_set_foreground(NAV_BAR_TEXT_COLOR);
      	roadmap_canvas_draw_image_text (NavigateBarImage, &position, size, line);
   }
}

void navigate_bar_set_mode (int mode) {
//   int x_offset;
//   int y_offset;

   if (NavigateBarEnabled == mode) return;

//   x_offset = 0;
//   y_offset = roadmap_canvas_image_height (NavigateBarBG);

//   roadmap_screen_move_center (20);
//   if (mode) {
//      roadmap_screen_obj_offset (x_offset, y_offset);
//      roadmap_screen_move_center (-y_offset / 2);
//   } else {
//      roadmap_screen_obj_offset (-x_offset, -y_offset);
//      roadmap_screen_move_center (y_offset / 2);
//   }
   
   NavigateBarEnabled = mode;
}


void navigate_bar_draw (void) {

   if (NavigateBarInitialized != 1) return;

   roadmap_canvas_draw_image (NavigateBarImage, &NavigateBarLocation, 0,
         IMAGE_NORMAL);
}

void navigate_bar_set_time_to_destination () {

   char text[256];
   RoadMapGuiPoint position = {0, 0};
   RoadMapPen nav_bar_pen;

   if (NavigateBarInitialized != 1) return;
  
  	if (!roadmap_message_format (text, sizeof(text), "%A|%T"))
  		return;
  	
  	nav_bar_pen = roadmap_canvas_create_pen ("nav_bar_pen");

  	roadmap_canvas_set_foreground(NAV_BAR_TIME_TO_DESTINATION_COLOR);
  	
   /* erase the old distance */
   roadmap_canvas_copy_image (NavigateBarImage, &position,
                              &NavigatePanel->time_to_destination_rect,
                              NavigateBarBG,
                              CANVAS_COPY_NORMAL);

	

   roadmap_canvas_draw_image_text
      (NavigateBarImage, &NavigatePanel->time_to_destination_pos, 14, text);


}


void navigate_bar_set_distance_to_destination () {

   char text[256];
   char distance[100];
   RoadMapGuiPoint position = {0, 0};
   RoadMapPen nav_bar_pen;
   char * pch;
   int width, ascent, descent;
   
   if (NavigateBarInitialized != 1) return;

	if (!roadmap_message_format (distance, sizeof(distance), "%D (%W)|%D"))
		return;
	
	pch = strtok (distance," ,.");
   sprintf(text, "%s", pch);
	roadmap_canvas_get_text_extents
         (text, 18, &width, &ascent, &descent, NULL);
  
   /* erase the old distance */
   roadmap_canvas_copy_image (NavigateBarImage, &position,
                              &NavigatePanel->distance_to_destination_rect,
                              NavigateBarBG,
                              CANVAS_COPY_NORMAL);


   nav_bar_pen = roadmap_canvas_create_pen ("nav_bar_pen");

  	roadmap_canvas_set_foreground(NAV_BAR_DIST_TO_DESTINATION_COLOR);

   position.x = NavigatePanel->distance_to_destination_pos.x - width;
	position.y = NavigatePanel->distance_to_destination_pos.y;
   roadmap_canvas_draw_image_text
      (NavigateBarImage, &position, 14, text);

  pch = strtok (NULL," ,.");
  sprintf(text, "%s",pch);
	
   
  roadmap_canvas_draw_image_text
      (NavigateBarImage, &NavigatePanel->distance_to_destination_unit_pos, 14, text);

}

void navigate_bar_set_speed () {

   char str[100];
   char unit_str[100];
   RoadMapGuiPoint position = {0, 0};
   RoadMapGpsPosition pos;
   RoadMapPen nav_bar_pen;
   int speed;
   int font_size = 24;
   
   #ifdef IPHONE
   	font_size = 20;
   #endif

   if (NavigateBarInitialized != 1) return;
  
  roadmap_navigate_get_current (&pos, NULL, NULL);
  speed = pos.speed;
  
  if (speed == -1) return;
  
   /* erase the old distance */
   roadmap_canvas_copy_image (NavigateBarImage, &position,
                              &NavigatePanel->speed_rect,
                              NavigateBarBG,
                              CANVAS_COPY_NORMAL);

   snprintf (str, sizeof(str), "%3d", roadmap_math_to_speed_unit(speed));

   nav_bar_pen = roadmap_canvas_create_pen ("nav_bar_pen");
  	roadmap_canvas_set_foreground(NAV_BAR_SPEED_COLOR);

   roadmap_canvas_draw_image_text
      (NavigateBarImage, &NavigatePanel->speed_pos, font_size, str);

   snprintf (unit_str, sizeof(unit_str), "%s",  roadmap_lang_get(roadmap_math_speed_unit()));

   roadmap_canvas_draw_image_text
      (NavigateBarImage, &NavigatePanel->speed_unit_pos, 10, unit_str);

}
