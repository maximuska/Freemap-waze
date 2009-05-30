/* ssd_messagebox.c - ssd messagebox.
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
 *   See roadmap_messagebox.h
 */

#include <stdlib.h>
#include "roadmap.h"
#include "ssd_dialog.h"
#include "ssd_text.h"
#include "ssd_container.h"
#include "ssd_button.h"
#include "ssd_bitmap.h"

#include "roadmap_lang.h"
#include "roadmap_main.h"
#include "roadmap_res.h"

#include "roadmap_messagebox.h"

static RoadMapCallback MessageBoxCallback = NULL;
struct ssd_messagebox_data {
   const char *bitmaps;
};

void roadmap_messagebox_cb(const char *title, const char *message,
         messagebox_closed on_messagebox_closed)
{
   roadmap_messagebox( title, message);
}         

static void kill_messagebox_timer (void) {
	
	if (MessageBoxCallback) {
		roadmap_main_remove_periodic (MessageBoxCallback);
		MessageBoxCallback = NULL;
	}	
}


static void close_messagebox (void) {

	kill_messagebox_timer ();
	ssd_dialog_hide ("message_box", dec_ok);
}


static int button_callback (SsdWidget widget, const char *new_value) {

	close_messagebox ();
   return 0;
}

static void create_messagebox (void) {

   SsdWidget dialog;
   
   dialog = ssd_dialog_new ("message_box", "", NULL,
         SSD_CONTAINER_BORDER|SSD_CONTAINER_TITLE|SSD_DIALOG_FLOAT|
         SSD_ALIGN_CENTER|SSD_ALIGN_VCENTER|SSD_ROUNDED_CORNERS);

   ssd_widget_set_color (dialog, "#000000", "#ff0000000");
   ssd_widget_add (dialog,
      ssd_container_new ("spacer1", NULL, 0, 10, SSD_END_ROW));

   ssd_widget_add (dialog,
      ssd_text_new ("text", "", 13, SSD_END_ROW|SSD_WIDGET_SPACE));

   /* Spacer */
   ssd_widget_add (dialog,
      ssd_container_new ("spacer2", NULL, 0, 20, SSD_END_ROW));

   ssd_widget_add (dialog,
      ssd_button_label ("confirm", roadmap_lang_get ("Ok"),
                        SSD_ALIGN_CENTER|SSD_START_NEW_ROW|SSD_WS_DEFWIDGET|
                        SSD_WS_TABSTOP, 
                        button_callback));
   
}


void roadmap_messagebox (const char *title, const char *text) {

   SsdWidget dialog = ssd_dialog_activate ("message_box", NULL);
   title = roadmap_lang_get (title);
   text  = roadmap_lang_get (text);

   if (!dialog) {
      create_messagebox ();
      dialog = ssd_dialog_activate ("message_box", NULL);
   }

   dialog->set_value (dialog, title);
   ssd_widget_set_value (dialog, "text", text);

	kill_messagebox_timer ();
   ssd_dialog_draw ();
}


void roadmap_messagebox_timeout (const char *title, const char *text, int seconds) {

	roadmap_messagebox (title, text);	
	MessageBoxCallback = close_messagebox;
	roadmap_main_set_periodic (seconds * 1000, close_messagebox);
}



