/* ssd_container.c - Container widget
 * (requires agg)
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
 *   See ssd_widget.h
 */

#include <string.h>
#include <stdlib.h>

#include "roadmap.h"
#include "roadmap_canvas.h"
#include "roadmap_sound.h"
#include "roadmap_lang.h"
#include "roadmap_res.h"
#include "roadmap_border.h"

#include "ssd_widget.h"
#include "ssd_text.h"
#include "ssd_button.h"
#include "ssd_dialog.h"


#include "roadmap_keyboard.h"

#include "ssd_container.h"

static int initialized;
static RoadMapPen bg;
static RoadMapPen border;
static const char *default_fg = "#000000";
static const char *default_bg = "#f3f3f5";

static void init_containers (void) {
   bg = roadmap_canvas_create_pen ("container_bg");
   roadmap_canvas_set_foreground (default_bg);

   border = roadmap_canvas_create_pen ("container_border");
   roadmap_canvas_set_foreground (default_fg);
   roadmap_canvas_set_thickness (2);

   initialized = 1;
}


static void draw (SsdWidget widget, RoadMapGuiRect *rect, int flags) {

   
   RoadMapGuiPoint points[5];
   int count = 5;
   const char* background = widget->bg_color;
   
   if (!(flags & SSD_GET_SIZE)) {
      

      if (widget->in_focus)
         background = "#4791f3";
      else if (widget->background_focus)
         background = "#4791f3";
      	   
      if (background) {
      	if (widget->flags & SSD_ROUNDED_CORNERS){
      		RoadMapGuiRect erase_rect;
			erase_rect.minx = rect->minx + SSD_ROUNDED_CORNER_WIDTH;
      		erase_rect.miny = rect->miny + SSD_ROUNDED_CORNER_HEIGHT;
      		erase_rect.maxx = rect->maxx - SSD_ROUNDED_CORNER_WIDTH;
      		erase_rect.maxy = rect->maxy - SSD_ROUNDED_CORNER_HEIGHT;
      		
      		roadmap_canvas_select_pen (bg);
         	roadmap_canvas_set_foreground (background);
         
	        roadmap_canvas_erase_area (&erase_rect);
      		
  	  	 }
  	  	 else{
         	roadmap_canvas_select_pen (bg);
         	roadmap_canvas_set_foreground (background);
         
         	roadmap_canvas_erase_area (rect);
  	  	 }
      }
   }

	if (widget->flags & SSD_CONTAINER_BORDER) {

      points[0].x = rect->minx + 1;
      points[0].y = rect->miny + 1;
      points[1].x = rect->maxx - 0;
      points[1].y = rect->miny + 1;
      points[2].x = rect->maxx - 0;
      points[2].y = rect->maxy - 0;
      points[3].x = rect->minx + 1;
      points[3].y = rect->maxy - 0;
      points[4].x = rect->minx + 1;
      points[4].y = rect->miny + 1;

      if (!(flags & SSD_GET_SIZE)) {
         const char *color = default_fg;

         roadmap_canvas_select_pen (border);
         if (widget->fg_color) color = widget->fg_color;
         roadmap_canvas_set_foreground (color);
		 if (widget->flags & SSD_ROUNDED_CORNERS){
		 	int pointer_type = POINTER_NONE;
		 	int header_type = HEADER_NONE;
		 	
		 	if (widget->flags & SSD_CONTAINER_TITLE)
		 		header_type= HEADER_BLACK;
		 	if (widget->flags & SSD_HEADER_GREEN)
		 		header_type = HEADER_GREEN;
			if (widget->flags & SSD_HEADER_GRAY)
				header_type= HEADER_GRAY;
		 	
		 	
		 	if (widget->flags & SSD_POINTER_MENU){
		 		pointer_type = POINTER_MENU;
		 		points[2].y -= 10;
		 	}
		 	
		 	else if (widget->flags & SSD_POINTER_COMMENT){
		 		pointer_type = POINTER_COMMNET;
		 		points[2].y -= 7;
		 	}
		 	else if (widget->flags & SSD_POINTER_NONE){
		 		pointer_type = POINTER_NONE;
		 		points[2].y -= 10;
		 	}
		 	
		 	if (widget->in_focus)
         		background = "#4791f3";
         	else if ((widget->flags & SSD_POINTER_COMMENT) | (widget->flags & SSD_POINTER_MENU) |  (widget->flags & SSD_POINTER_NONE))
         		background = "#e4f1f9";
         	else  
         		background = "#f3f3f5"; 
         	
         	roadmap_display_border(STYLE_NORMAL, header_type, pointer_type, &points[2], &points[0], background, NULL);
         		

		 }
		 else
	        roadmap_canvas_draw_multiple_lines (1, &count, points, 0);
      }

	  if (widget->flags & SSD_ROUNDED_CORNERS){
		rect->minx += 10;
		if ((widget->flags & SSD_CONTAINER_TITLE) || (widget->flags & SSD_POINTER_MENU) ||  (widget->flags & SSD_POINTER_NONE))
      		rect->miny += 10;
      	else
      		rect->miny += 4;
      	rect->maxx -= 10;
      	rect->maxy -= 10;
  	  }
	  else{
      	rect->minx += 2;
      	rect->miny += 2;
      	rect->maxx -= 2;
      	rect->maxy -= 2;
	  }

   }


   if (widget->flags & SSD_CONTAINER_TITLE){
   		ssd_widget_set_left_softkey_text(widget, widget->left_softkey);
   		ssd_widget_set_right_softkey_text(widget, widget->right_softkey);
   }
	   
   if ((flags & SSD_GET_CONTAINER_SIZE) &&
       (widget->flags & SSD_CONTAINER_TITLE)) {
      SsdWidget title;

      title = ssd_widget_get (widget, "title_bar");

      if (title) {
         SsdSize title_size; 
         SsdSize max_size;
         max_size.width = rect->maxx - rect->minx + 1;
         max_size.height = rect->maxy - rect->miny + 1;
         
         ssd_widget_get_size (title, &title_size, &max_size);
         rect->miny += title_size.height;
      }
     
   }
}


static int short_click (SsdWidget widget, const RoadMapGuiPoint *point) {

   if (widget->callback) {
      RoadMapSoundList list = roadmap_sound_list_create (0);
      if (roadmap_sound_list_add (list, "click") != -1) {
         roadmap_sound_play_list (list);
      }

      (*widget->callback) (widget, SSD_BUTTON_SHORT_CLICK);
      return 1;
   }

   return 0;
}

static void add_title (SsdWidget w, int flags) {
   SsdWidget title, text;
	int rt_soft_key_flag;
	int lf_soft_key_flag;
#ifdef TOUCH_SCREEN2
		const char *close_icon[] = {"rm_quit"};
#endif
	
	if (ssd_widget_rtl (NULL)) {
		rt_soft_key_flag = SSD_END_ROW;
		lf_soft_key_flag = SSD_ALIGN_RIGHT;
	}
	else{
		rt_soft_key_flag = SSD_ALIGN_RIGHT;
		lf_soft_key_flag = SSD_END_ROW;
	}
	
   title = ssd_container_new ("title_bar", NULL, SSD_MAX_SIZE, 22 ,
                              SSD_END_ROW);
  
   ssd_widget_set_color (title, "#000000", "#ff0000000");

   text = ssd_text_new ("title_text", "" , 16, SSD_WIDGET_SPACE|SSD_ALIGN_VCENTER|SSD_ALIGN_CENTER);
   
   if ((w->flags & SSD_ROUNDED_CORNERS) && (!(w->flags & SSD_POINTER_MENU)))
   	ssd_widget_set_color (text, "#ffffff", "#ff0000000");
   ssd_widget_add (title,text);
   

   ssd_widget_add (w, title);
}


static int set_value (SsdWidget widget, const char *value) {

   if (!(widget->flags & SSD_CONTAINER_TITLE)) return -1;

   return ssd_widget_set_value (widget, "title_text", value);
}


static const char *get_value (SsdWidget widget) {

   if (!(widget->flags & SSD_CONTAINER_TITLE)) return "";

   return ssd_widget_get_value (widget, "title_text");
}

static BOOL ssd_container_on_key_pressed( SsdWidget   widget,
                                          const char* utf8char, 
                                          uint32_t      flags)
{
   if( KEY_IS_ENTER && widget->callback)
   {
      widget->callback( widget, SSD_BUTTON_SHORT_CLICK);
      return TRUE;
   }

   return FALSE;
}

int on_pointer_down( SsdWidget this, const RoadMapGuiPoint *point)
{
   if( !this->tab_stop)
      return 0;

   if( !this->in_focus)
      ssd_dialog_set_focus( this);
   
   return 1;
}

SsdWidget ssd_container_new (const char *name, const char *title,
                             int width, int height, int flags) {

   SsdWidget w;

   if (!initialized) {
      init_containers();
   }
   
   w = ssd_widget_new (name, ssd_container_on_key_pressed, flags);

   w->_typeid = "Container"; 

   w->flags = flags;

   w->draw = draw;

   w->size.width = width;
   w->size.height = height;

   w->get_value = get_value;
   w->set_value = set_value;
   w->short_click = short_click;

   w->fg_color = default_fg;
   w->bg_color = default_bg;
   
   w->pointer_down = on_pointer_down;

   if (flags & SSD_CONTAINER_TITLE) {
       add_title (w, flags);
   }
   
   if ((flags & SSD_CONTAINER_TITLE) || flags & (flags &SSD_DIALOG_FLOAT)){
   	   ssd_widget_set_right_softkey_text(w, roadmap_lang_get("Back_key"));
   	   ssd_widget_set_left_softkey_text(w, roadmap_lang_get("Exit_key"));
   }

   set_value (w, title);

   return w;
}

void ssd_container_get_zero_offset(
                     SsdWidget         this, 
                     int*              zero_offset_x,
                     int*              zero_offset_y)
{
   RoadMapGuiRect org_rect;
   RoadMapGuiRect dra_rect;

   assert(this);
   assert(zero_offset_x);
   assert(zero_offset_y);
   
   org_rect.minx = this->position.x;
   org_rect.miny = this->position.y;
   org_rect.maxx = org_rect.minx + this->size.width;
   org_rect.maxy = org_rect.miny + this->size.height;
   dra_rect      = org_rect;

   draw( this, &dra_rect, SSD_GET_SIZE);
   
   (*zero_offset_x) = dra_rect.minx - org_rect.minx;
   (*zero_offset_y) = dra_rect.miny - org_rect.miny;
}
                     
void ssd_container_get_visible_dimentions(
                     SsdWidget         this, 
                     RoadMapGuiPoint*  position,
                     SsdSize*          size)
{
   assert(this);
   assert(position);
   assert(size);

   (*position) = this->position;
   (*size)     = this->size;
   
   if( SSD_ROUNDED_CORNERS & this->flags)
   {
      position->x += 3;
      position->y += 4;

      size->width -= (3 + 3);
      size->height-= (4 + 3);
      
     if( (SSD_POINTER_MENU|SSD_POINTER_NONE) & this->flags)
         size->height -= 11;
   }
}
