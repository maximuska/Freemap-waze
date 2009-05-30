/* ssd_confirm_dialog.c - ssd confirmation dialog (yes/no).
 *
 * LICENSE:
 *
 *   Copyright 2008 Avi B.S
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
 *   See ssd_confirm_dialog.h.h
 */
#include <string.h>
#include <stdlib.h>
#include "ssd_dialog.h"
#include "ssd_text.h"
#include "ssd_container.h"
#include "ssd_button.h"

#include "roadmap_lang.h"
#include "roadmap_softkeys.h"

#include "ssd_confirm_dialog.h"


typedef struct {
	void *context;
	ConfirmDialogCallback callback;	
} confirm_dialog_context;

static int yes_button_callback (SsdWidget widget, const char *new_value) {

	SsdWidget dialog;
	ConfirmDialogCallback callback;
	confirm_dialog_context *data;
	
	dialog = widget->parent;
	data = (confirm_dialog_context  *)dialog->context;
	
	callback = (ConfirmDialogCallback)data->callback;

	ssd_dialog_hide ("confirm_dialog", dec_yes);

    (*callback)(dec_yes, data->context);
	
	return 0;
}

static int no_button_callback (SsdWidget widget, const char *new_value) {


    SsdWidget dialog;
    ConfirmDialogCallback callback;
	 confirm_dialog_context *data;
    
    dialog = widget->parent;
    data = (confirm_dialog_context  *)dialog->context;
    
    callback = (ConfirmDialogCallback)data->callback;

    ssd_dialog_hide ("confirm_dialog", dec_no);
    
    (*callback)(dec_no, data->context);

    return 0;
}

static int no_softkey_callback (SsdWidget widget, const char *new_value, void *context){
	return no_button_callback(widget->children, new_value);
}

static int yes_softkey_callback (SsdWidget widget, const char *new_value, void *context){
	return yes_button_callback(widget->children, new_value);
}

static void set_soft_keys(SsdWidget dialog){
	
	ssd_widget_set_right_softkey_text(dialog, "No");
	ssd_widget_set_right_softkey_callback(dialog,no_softkey_callback);
	ssd_widget_set_left_softkey_text(dialog, "Yes");	
	ssd_widget_set_left_softkey_callback(dialog,yes_softkey_callback);
}


static void create_confirm_dialog (BOOL default_yes) {

   SsdWidget dialog;
   SsdWidget image_container, text_con;

   const char *question_icon[] = {"question"};
   int yes_flags = 0;
   int no_flags = 0;
   
#ifndef TOUCH_SCREEN   
   if(default_yes)
      yes_flags|= SSD_WS_DEFWIDGET;
   else                                                                     
      no_flags |= SSD_WS_DEFWIDGET;
#endif
   
   dialog = ssd_dialog_new ("confirm_dialog", "", NULL,
         SSD_CONTAINER_BORDER|SSD_DIALOG_FLOAT|
         SSD_ALIGN_CENTER|SSD_CONTAINER_TITLE|SSD_ALIGN_VCENTER|SSD_ROUNDED_CORNERS|SSD_POINTER_NONE);
   ssd_widget_set_color (dialog, "#000000", "#ff0000000");

   
   /* Spacer */
   ssd_widget_add (dialog,
      ssd_container_new ("spacer1", NULL, 0, 15, SSD_END_ROW));

   image_container = ssd_container_new ("image_container", NULL,
                                  SSD_MIN_SIZE,
                                  SSD_MIN_SIZE,
                                  SSD_ALIGN_RIGHT);

   ssd_widget_set_color (image_container, "#000000", "#ff0000000");
   ssd_widget_add (image_container,
        ssd_button_new ("question", "question", question_icon , 1, 
                           SSD_ALIGN_CENTER|SSD_ALIGN_VCENTER , 
                           NULL));
   // Image container
   ssd_widget_add (dialog,image_container);
   
   text_con = ssd_container_new ("text_container", NULL,
                                  SSD_MIN_SIZE,
                                  SSD_MIN_SIZE,
                                  SSD_END_ROW);
   ssd_widget_set_color (text_con, "#000000", "#ff0000000");
                                    
   // Text box
   ssd_widget_add (text_con,
      ssd_text_new ("text", "", 13, SSD_END_ROW|SSD_WIDGET_SPACE));

  ssd_widget_add(dialog, text_con);

#ifdef TOUCH_SCREEN
   ssd_widget_add (dialog,
      ssd_container_new ("spacer2", NULL, 0, 10, SSD_START_NEW_ROW|SSD_WIDGET_SPACE));
      
    ssd_widget_add (dialog,
     ssd_button_label (roadmap_lang_get ("Yes"), roadmap_lang_get ("Yes"), 
                        SSD_ALIGN_CENTER| SSD_WS_TABSTOP, 
                        yes_button_callback));

   ssd_widget_add (dialog,
		   ssd_button_label (roadmap_lang_get ("No"), roadmap_lang_get ("No"),  
                        SSD_ALIGN_CENTER| SSD_WS_TABSTOP, 
                        no_button_callback));

#else
	set_soft_keys(dialog); 
	             
#endif                       
    ssd_widget_add (dialog,
      ssd_container_new ("spacer2", NULL, 0, 10, SSD_START_NEW_ROW|SSD_WIDGET_SPACE));
}


void ssd_confirm_dialog (const char *title, const char *text, BOOL default_yes, ConfirmDialogCallback callback, void *context) {
	
	
  confirm_dialog_context *data =
    (confirm_dialog_context  *)calloc (1, sizeof(*data));
      
   SsdWidget dialog = ssd_dialog_activate ("confirm_dialog", NULL); 
   title = roadmap_lang_get (title);
   text  = roadmap_lang_get (text);

   if (!dialog) {
      create_confirm_dialog (default_yes);
      dialog = ssd_dialog_activate ("confirm_dialog", NULL);
   } 

   data->callback = callback;
   data->context = context;
 
   dialog->set_value (dialog, title);
   ssd_widget_set_value (dialog, "text", text);
   dialog->context = data;
   ssd_dialog_draw ();
}

