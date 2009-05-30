/* ssd_menu.c - Icons menu
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
 *   See ssd_menu.h.
 */

#include <string.h>
#include <stdlib.h> 

#include "roadmap.h"
#include "roadmap_lang.h"
#include "roadmap_path.h"
#include "roadmap_res.h"
#include "roadmap_screen.h"
#include "roadmap_keyboard.h"
#include "roadmap_factory.h"
#include "roadmap_bar.h"
#include "ssd_dialog.h"
#include "ssd_container.h"
#include "ssd_button.h"
#include "ssd_text.h"

#include "ssd_menu.h"

#ifdef   USE_CONTEXTMENU_INSTEAD
#endif   // USE_CONTEXTMENU_INSTEAD

typedef struct list_menu_data{
   SsdWidget  MenuWidgets[20];
   int        CurrentIndex;
   int        num_rows;
   SsdWidget  container ;
   int        num_items ;
   
}  menu_list_data;


static int button_callback (SsdWidget widget, const char *new_value) {

   RoadMapCallback callback = (RoadMapCallback) widget->context;

   (*callback)();
   
#ifdef TOUCH_SCREEN   
   if (widget->parent)
   	ssd_widget_loose_focus(widget->parent);
#endif

   /* Hide dialog */
   while (widget->parent) widget = widget->parent;

   return 0;
}

#ifndef TOUCH_SCREEN
static void move_to_start(){
	  menu_list_data *data;
	  int i;
	  int num_items;
	  
	  data = (menu_list_data *)	ssd_dialog_get_current_data();
	  	
	  for (i=0; i<data->num_items;i++)
         ssd_widget_hide(data->MenuWidgets[i]);
     
     
     if (data->num_items < data->num_rows)
     	num_items = data->num_items;
     else
     	num_items = data->num_rows;
     for (i=0; i<num_items;i++)
         		ssd_widget_show(data->MenuWidgets[i]);
	  ssd_dialog_resort_tab_order();
	  ssd_dialog_set_focus(ssd_widget_get(data->MenuWidgets[0],"button"));
	  data->CurrentIndex = 0;
}
#endif

static void move_down(){
   menu_list_data *data;
   int i;
   div_t n;
   int relative_index;

   data = (menu_list_data *)   ssd_dialog_get_current_data();
   
   n = div (data->CurrentIndex, data->num_rows);
   relative_index =  n.rem;
   
      if (((data->CurrentIndex == data->num_rows -1) || (relative_index == data->num_rows -1) || (data->CurrentIndex == data->num_items -1)) && (data->num_items > data->num_rows)){
          int index;
          
         for (i=0; i<data->num_items;i++)
               ssd_widget_hide(data->MenuWidgets[i]);

         if (data->CurrentIndex == data->num_items -1)
            index = 0;
         else
            index = data->CurrentIndex +1;
                  
         for (i=index; i<index+data->num_rows;i++)
            if (i < data->num_items)
               ssd_widget_show(data->MenuWidgets[i]);
      }
             
      if (data->CurrentIndex < data->num_items-1)
         data->CurrentIndex++;
      else
         data->CurrentIndex = 0;
             
      ssd_dialog_resort_tab_order();
      ssd_dialog_set_focus(ssd_widget_get(data->MenuWidgets[data->CurrentIndex],"button"));
}

static void move_up(){
      menu_list_data *data;
      int i;
      div_t n;
   	  int relative_index;
   
      data = (menu_list_data *)   ssd_dialog_get_current_data();
      n = div (data->CurrentIndex, data->num_rows);
      relative_index =  n.rem;
      
      if (((data->CurrentIndex == 0) || (relative_index == 0) || (data->CurrentIndex == data->num_rows )) && (data->num_items > data->num_rows)){
          int index;
                
         for (i=0; i<data->num_items;i++)
            ssd_widget_hide(data->MenuWidgets[i]);

         if (relative_index == 0)
         	if (n.quot == 0){
         		div_t n2;
         		n2 = div (data->num_items, data->num_rows);
         		if (n2.rem == 0)
         			index = (n2.quot -1) * data->num_rows;
         		else
         			index = n2.quot * data->num_rows ;
         	}
         	else
         		index = (n.quot -1) * data->num_rows;
         else
            index =0;
                  
         for (i=index; i<index+data->num_rows;i++)
            if (i < data->num_items)
               ssd_widget_show(data->MenuWidgets[i]);
      }

                
      if (data->CurrentIndex > 0)
         data->CurrentIndex--;
      else
         data->CurrentIndex = data->num_items -1;
             
      ssd_dialog_resort_tab_order();
      ssd_dialog_set_focus(ssd_widget_get(data->MenuWidgets[data->CurrentIndex],"button"));
   
}

#ifdef TOUCH_SCREEN
static void scroll_page_down(){
	  menu_list_data *data;
   	     	  
      data = (menu_list_data *)   ssd_dialog_get_current_data();
      if( (data->CurrentIndex + data->num_rows) >= data->num_items)
      	return;
	
	  data->CurrentIndex += data->num_rows-1;
	  
	  move_down();
	  
}

static void scroll_page_up(){
	  menu_list_data *data;
   	     	  
      data = (menu_list_data *)   ssd_dialog_get_current_data();
      if( (data->CurrentIndex - data->num_rows) < 0)
      	return;
	  move_up();
	  data->CurrentIndex -= data->num_rows -2;
	  move_up();
}


static int scroll_buttons_callback (SsdWidget widget, const char *new_value) {

   if (!strcmp(widget->name, "scroll_up")) {
      scroll_page_up();
      return 0;
   }

   if (!strcmp( widget->name, "scroll_down")) {
      scroll_page_down();
      return 0;
   }

   return 1;
}
#endif

static BOOL OnKeyPressed (SsdWidget widget, const char* utf8char, uint32_t flags){
   BOOL        key_handled = TRUE;
   
   if( KEY_IS_ENTER)
   {
      widget->callback(widget, SSD_BUTTON_SHORT_CLICK);
      return TRUE;
   }
   
    if( KEYBOARD_VIRTUAL_KEY & flags)
   {
      switch( *utf8char) {
        
         case VK_Arrow_up:
               move_up();
            break;

         case VK_Arrow_down:
            move_down();
            break;

         default:       
            key_handled = FALSE;
      }

   }
   else
   {
      assert(utf8char);
      assert(*utf8char);
      
      // Other special keys:
      if( KEYBOARD_ASCII & flags)
      {      
         switch(*utf8char)
         {
            case TAB_KEY:
                move_down();
               break;

            default:       
               key_handled = FALSE;
         }
      }
   }
   
   if( key_handled)
      roadmap_screen_redraw ();

   return key_handled;
}

static void draw_bg( SsdWidget this, RoadMapGuiRect *rect, int flags){

   static RoadMapImage TopBgImage;
   static RoadMapImage BottomBgImage;
   static RoadMapImage MiddleBgImage;
   RoadMapGuiPoint point;
   int i;
   static int top_height, top_width;
   static int bottom_height, bottom_width;
   
   int height = roadmap_canvas_height() -  roadmap_bar_bottom_height();
   int width = roadmap_canvas_width();
   
   if (!TopBgImage)
      TopBgImage = (RoadMapImage) roadmap_res_get(RES_BITMAP, RES_SKIN, "menu_bg_top");
   if (TopBgImage == NULL){
      return;
   }
      
   if (!BottomBgImage)
      BottomBgImage = (RoadMapImage) roadmap_res_get(RES_BITMAP, RES_SKIN, "menu_bg_bottom");
   if (BottomBgImage == NULL){
      return;
   }
      
   if (!MiddleBgImage)
      MiddleBgImage = (RoadMapImage) roadmap_res_get(RES_BITMAP, RES_SKIN, "menu_bg_middle");
   if (MiddleBgImage == NULL){
         return;
   } 

   top_height = roadmap_canvas_image_height(TopBgImage);
   bottom_height = roadmap_canvas_image_height(BottomBgImage);
   

    if ((flags & SSD_GET_SIZE)){
    	rect->miny += top_height + 4;
    	rect->maxy = height - bottom_height ;
    	return;
    }
    
   top_width  = roadmap_canvas_image_width(TopBgImage);
    bottom_width  = roadmap_canvas_image_width(BottomBgImage);
    

	if (!(flags & SSD_GET_SIZE)){
		rect->miny += top_height + 4;
    	        
                rect->maxy = height - bottom_height -roadmap_bar_bottom_height() ;
    	        rect->maxx -= 7;
		
		point.y = 0;
		point.x = width - top_width;
	
		roadmap_canvas_draw_image (TopBgImage, &point, 0,IMAGE_NORMAL);
		
		point.y = height - bottom_height+4;
		point.x = width - bottom_width;
		
		for (i = top_height; i <  point.y; i++){
			RoadMapGuiPoint pointMiddle;
			pointMiddle.y = i;
			pointMiddle.x = width - top_width;
			roadmap_canvas_draw_image (MiddleBgImage, &pointMiddle, 0,IMAGE_NORMAL);
		}  
		
		roadmap_canvas_draw_image (BottomBgImage, &point, 0,IMAGE_NORMAL);
	}
}

static const RoadMapAction *find_action_by_label
                              (const RoadMapAction *actions, const char *item) {

   while (actions->label_long != NULL) {
      if (strcmp (actions->label_long, item) == 0) return actions;
      ++actions;
   }

   return NULL;
}

static const RoadMapAction *find_action
                              (const RoadMapAction *actions, const char *item) {

   const RoadMapAction* first = actions;

   while (actions->name != NULL) {
      if (strcmp (actions->name, item) == 0) return actions;
      ++actions;
   }

   return find_action_by_label( first, item) ;
}


static int long_click (SsdWidget widget, const RoadMapGuiPoint *point) {

   if (ssd_widget_long_click (widget->children, point)) {
      ssd_dialog_hide (widget->name, dec_ok);
   }

   return 1;
}


static SsdWidget ssd_menu_new (const char           *name,
                               const char           *items_file,
                               const char           *items[],
                               const RoadMapAction  *actions,
                               int                   flags) {

   int i;
   int next_item_flags = 0;
   int width = SSD_MAX_SIZE;

   const char **menu_items =
      roadmap_factory_user_config (items_file, "menu", actions);

   SsdWidget dialog = ssd_dialog_new (name, roadmap_lang_get (name), NULL,
                      flags|SSD_DIALOG_GUI_TAB_ORDER);

   SsdWidget container;

   if (flags & SSD_DIALOG_FLOAT) {

      if (flags & SSD_DIALOG_VERTICAL){
         width = SSD_MIN_SIZE;
      }
         
      container = ssd_container_new (name, NULL, width, SSD_MIN_SIZE, 0);
      ssd_widget_set_size (dialog, SSD_MIN_SIZE, SSD_MIN_SIZE);
      ssd_widget_set_color (dialog, "#000000", "#ff0000000");
      ssd_widget_set_color (container, "#000000", "#ff0000000");

   } else {
      container = ssd_container_new (name, NULL, SSD_MAX_SIZE, SSD_MAX_SIZE,
                                  SSD_ALIGN_GRID);
      ssd_widget_set_color (dialog, "#000000", "#ff0000000");
      ssd_widget_set_color (container, "#000000", "#ff0000000");
   }

   /* Override long click */
   container->long_click = long_click;
   
   if (!menu_items) menu_items = items;

   for (i = 0; menu_items[i] != NULL; ++i) {

      const char *item = menu_items[i];

      if (item == RoadMapFactorySeparator) {
         next_item_flags = SSD_START_NEW_ROW;

      } else {
      	 if (flags & SSD_DIALOG_FLOAT){
	      	 	         SsdWidget text_box;
	         SsdSize size;
	         const RoadMapAction *this_action = find_action (actions, item);
	         const char *button_icon[2];
	         SsdWidget w = ssd_container_new (item, NULL,
	                           SSD_MIN_SIZE, SSD_MIN_SIZE,
	                           SSD_WS_TABSTOP|SSD_ALIGN_CENTER|next_item_flags);
	         SsdWidget button;
	         SsdWidget text;
	         
	         button_icon[0] = item;
	         button_icon[1] = NULL;
	         
	         ssd_widget_set_color (w, "#000000", NULL);
	
	         button = ssd_button_new
	                    (item, item, button_icon, 1, SSD_ALIGN_CENTER|SSD_END_ROW,
	                     button_callback);
	
	         ssd_widget_get_size (button ,&size, NULL);
	
	         ssd_widget_set_context (button, this_action->callback);
	         ssd_widget_add (w, button);
	
	         if (!(flags & SSD_BUTTON_NO_TEXT)){
	           text_box = ssd_container_new ("text_box", NULL,
	                                         size.width + size.width / 4,
	                                         SSD_MIN_SIZE,
	                                         SSD_ALIGN_CENTER|SSD_END_ROW);
	
	           ssd_widget_set_color (text_box, "#000000", NULL);
	
	           text = ssd_text_new (item,
	                                roadmap_lang_get (this_action->label_long),
	                                10, /* 60,*/ SSD_ALIGN_CENTER|SSD_END_ROW);
	           ssd_widget_add (text_box, text);
	           ssd_widget_add (w, text_box);
	         }
	         ssd_widget_add (container, w);
      	 	
      	 }
      	 else{
		 const char *bg_button_icon[]   = {"menu_button", "menu_button_selected"};
		 
         SsdWidget text_box;
         SsdSize size;
         const RoadMapAction *this_action = find_action (actions, item);
         const char *button_icon[2];
         SsdWidget button;
         SsdWidget text;

         SsdWidget w = ssd_container_new (item, NULL,
                           SSD_MIN_SIZE, SSD_MIN_SIZE,
                           SSD_ALIGN_CENTER|next_item_flags);
                           
         SsdWidget bg_button = ssd_button_new ("menu_bg_button", "", bg_button_icon, 2,
                                      SSD_ALIGN_CENTER|SSD_WS_TABSTOP, button_callback);

         button_icon[0] = item;
         button_icon[1] = NULL;
         
         ssd_widget_set_color (w, "#000000", NULL);

		 
		 
         button = ssd_button_new
                    (item, item, button_icon, 1, SSD_ALIGN_CENTER|SSD_END_ROW,
                     button_callback);

         ssd_widget_get_size (bg_button ,&size, NULL);

         ssd_widget_set_context (bg_button, this_action->callback);
         ssd_widget_add (bg_button, button);

		 
		 
         if (!(flags & SSD_BUTTON_NO_TEXT)){
           text_box = ssd_container_new ("text_box", NULL,
                                         size.width + size.width / 4,
                                         SSD_MIN_SIZE,
                                         SSD_ALIGN_CENTER|SSD_END_ROW);

           ssd_widget_set_color (text_box, "#000000", NULL);

           text = ssd_text_new (item,
                                roadmap_lang_get (this_action->label_long),
                                10, /* 60,*/ SSD_ALIGN_CENTER|SSD_END_ROW);
           ssd_widget_add (text_box, text);
           ssd_widget_add (bg_button, text_box);
         }
         
         ssd_widget_add (w, bg_button);
         ssd_widget_add (container, w);
      	 }
      	 
         next_item_flags = 0;
      }
   }


   ssd_widget_add (dialog, container);

   return dialog;
}

static SsdWidget  ssd_menu_list_new(const char           *name,
                               const char           *items_file,
                               const char           *items[],
                               const RoadMapAction  *actions,
                               int                         flags) {
   int i;
   int next_item_flags = 0;
   int width = SSD_MAX_SIZE;
   SsdSize button_size;
   SsdSize size;
   SsdWidget dialog;
   const char **menu_items = NULL;
   const char *button_icon[3];
   
#ifdef TOUCH_SCREEN   
   SsdWidget scroll_up, scroll_down;
   SsdWidget container;
#endif   
   menu_list_data *data =  ( menu_list_data *)calloc (1, sizeof(menu_list_data));
        
        
   dialog = ssd_dialog_new (name, roadmap_lang_get (name), NULL,
                      flags|SSD_DIALOG_GUI_TAB_ORDER);

     data->num_items = 0;
     data->CurrentIndex = 0;
     
   if( items_file)
      menu_items = roadmap_factory_user_config (items_file, "menu", actions);

   if (flags & SSD_DIALOG_FLOAT) {

      if (flags & SSD_DIALOG_VERTICAL){
         width = SSD_MIN_SIZE;
            
         
      }
   
    if (flags & SSD_BG_IMAGE)
          dialog->draw = draw_bg;      
                                          
      data->container = ssd_container_new (name, NULL, width, SSD_MIN_SIZE, 0);
      ssd_widget_set_size (dialog, SSD_MIN_SIZE, SSD_MIN_SIZE);
      ssd_widget_set_color (dialog, NULL, NULL);
      ssd_widget_set_color (data->container, NULL, NULL);
     
   } else {
      data->container = ssd_container_new (name, NULL, SSD_MAX_SIZE, SSD_MAX_SIZE,
                                  SSD_ALIGN_GRID);
      ssd_widget_set_color (dialog, "#000000", "#ffffffee");
      ssd_widget_set_color (data->container, "#000000", NULL);
   }

   /* Override long click */
   data->container->long_click = long_click;
   
   if (!menu_items) menu_items = items;

#ifdef TOUCH_SCREEN
#ifndef IPHONE
   container = ssd_container_new ("scroll_up_con", NULL,
                           70, SSD_MIN_SIZE,
                           SSD_END_ROW);
   ssd_widget_set_color (container, "#000000", NULL);                           
   button_icon[0] = "menu_page_up";
   scroll_up = ssd_button_new
                    ("scroll_up", "scroll_up", button_icon, 1,
                     SSD_WS_TABSTOP|SSD_END_ROW|SSD_VAR_SIZE|SSD_ALIGN_CENTER,
                     scroll_buttons_callback);
   ssd_widget_add (container, scroll_up);
   ssd_widget_add (dialog, container);
#endif   
#endif

   for (i = 0; menu_items[i] != NULL; ++i) {

      const char *item = menu_items[i];

      if (item == RoadMapFactorySeparator) {
         next_item_flags = SSD_START_NEW_ROW;

      } else {
		 int rtl_flag;
         char focus_name[250];
         SsdWidget button;
		 SsdWidget w;
         const RoadMapAction *this_action = find_action (actions, item);
         
         if (ssd_widget_rtl (NULL))
			rtl_flag = SSD_END_ROW;
		 else
			rtl_flag = SSD_ALIGN_RIGHT;
         
         w = ssd_container_new (item, NULL,
                           SSD_MIN_SIZE, SSD_MIN_SIZE,
                           next_item_flags|rtl_flag);

         button_icon[0] = item;
         strcpy(focus_name, item);
         strcat(focus_name,"_focus");
       
         button_icon[1] = strdup(focus_name);
         button_icon[2] = NULL;
 
         ssd_widget_set_color (w, "#000000", NULL);

		
        button = ssd_button_new
                    ("button", item, button_icon, 2,
                     SSD_WS_TABSTOP|SSD_VAR_SIZE,
                     button_callback);
         ssd_widget_get_size(button, &button_size, NULL );
         
         ssd_widget_set_context (button, this_action->callback);
         ssd_widget_add (w, button);
         ssd_widget_hide(w);
         data->MenuWidgets[data->num_items] = w;
         data->num_items++;                  
         ssd_widget_add (data->container, w);
           
         next_item_flags = 0;
      }
   }


   ssd_widget_container_size (dialog, &size);
   if (data->num_items > 0){
   	ssd_widget_container_size (dialog, &size);
   	data->num_rows = size.height / button_size.height;	
   	if  (data->num_items <= data->num_rows){
	   	int i;
   		for (i = 0 ;i < data->num_items; i++){
   			ssd_widget_show (data->MenuWidgets[i]);
   			data->MenuWidgets[i]->children->key_pressed = OnKeyPressed;
   		}
   	}
   	else{
   		for (i = 0 ;i < data->num_rows; i++){
   			ssd_widget_show (data->MenuWidgets[i]);
   		}
   	
   		for (i=0; i<data->num_items;i++){
   			data->MenuWidgets[i]->children->key_pressed = OnKeyPressed;
   		}
   	}
   }
   ssd_widget_add (dialog, data->container);
   
#if defined (TOUCH_SCREEN) && !defined (IPHONE)
   container = ssd_container_new ("scroll_down_con", NULL,
                           70, SSD_MIN_SIZE,
                           SSD_END_ROW);
   ssd_widget_set_color (container, "#000000", NULL);                           
   button_icon[0] = "menu_page_down";
   scroll_down = ssd_button_new
                    ("scroll_down", "scroll_down", button_icon, 1,
                     SSD_WS_TABSTOP|SSD_END_ROW|SSD_VAR_SIZE|SSD_ALIGN_CENTER,
                     scroll_buttons_callback);
   ssd_widget_add (container, scroll_down);
   ssd_widget_add (dialog, container);
#endif
   
   dialog->data = (void *)data;
   return dialog;
}                                  



void ssd_menu_activate (const char           *name,
                        const char           *items_file,
                        const char           *items[],
                        PFN_ON_DIALOG_CLOSED on_dialog_closed,
                        const RoadMapAction  *actions,
                        int                   flags) {
   SsdWidget dialog = ssd_dialog_activate (name, NULL);

   
   
   if (dialog) {
      ssd_dialog_set_callback (on_dialog_closed);
      ssd_widget_set_flags (dialog, flags);
      ssd_dialog_draw ();
      return;
   }

   dialog = ssd_menu_new (name, items_file, items, actions, flags);
   
   ssd_dialog_activate (name, NULL);
   ssd_dialog_set_callback (on_dialog_closed);
   ssd_dialog_draw ();
}

void ssd_list_menu_activate (const char           *name,
                        const char           *items_file,
                        const char           *items[],
                        PFN_ON_DIALOG_CLOSED on_dialog_closed,
                        const RoadMapAction  *actions,
                        int                   flags) {

   menu_list_data *data;
   int align = 0;
   SsdWidget dialog = ssd_dialog_activate (name, NULL);
   
   if (!ssd_widget_rtl (NULL))
			align = SSD_ALIGN_RIGHT;

   if (dialog) {
   	  ssd_dialog_set_callback (on_dialog_closed);
      ssd_widget_set_flags (dialog, flags|align);
      data = (menu_list_data *)dialog->data;
      
#ifndef TOUCH_SCREEN   	
      dialog->in_focus = dialog->default_widget;
      move_to_start();
#endif      
      ssd_dialog_draw ();
      return;
   }
   
   
   dialog = ssd_menu_list_new (name, items_file, items, actions, flags|align);
   data = (menu_list_data *)dialog->data;
   data->CurrentIndex = 0;
   
   ssd_dialog_activate (name, NULL);
   ssd_dialog_set_callback (on_dialog_closed);
   ssd_dialog_draw ();
}

void ssd_menu_hide (const char *name) {
         
   ssd_dialog_hide (name, dec_close);
}


void ssd_menu_load_images(const char   *items_file, const RoadMapAction  *actions){

#ifdef TOUCH_SCREEN
     const char **menu_items = roadmap_factory_user_config (items_file, "menu", actions);
     int i;
      
     for (i = 0; menu_items[i] != NULL; ++i) {
         const char *item = menu_items[i];
         if (item != RoadMapFactorySeparator) {
               roadmap_res_get(RES_BITMAP, 
                                 RES_SKIN|RES_LOCK, 
                                item);
         }
   }
#endif
}
