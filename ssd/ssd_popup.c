/* ssd_popup.h - PopUp widget
 *
 * LICENSE:
 *
 *   Copyright 2008 Avi Ben-Shoshan
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
 *   See ssd_popup.h.
 */

#include <string.h>
#include <stdlib.h>
#include "roadmap.h"
#include "ssd_dialog.h"
#include "ssd_container.h"
#include "ssd_button.h"
#include "ssd_text.h"
#include "ssd_popup.h"

#define POPUP_CLOSE_CLICK_OFFSETS_DEFAULT	{-20, -20, 20, 20 };

static SsdClickOffsets sgPopUpCloseOffsets = POPUP_CLOSE_CLICK_OFFSETS_DEFAULT;

struct ssd_popup_data {
   SsdCallback callback;
   void (*draw) (SsdWidget widget, RoadMapGuiRect *rect, int flags);
   RoadMapPosition position;
};

static void draw (SsdWidget widget, RoadMapGuiRect *rect, int flags){
	struct ssd_popup_data *data = (struct ssd_popup_data *)widget->data;
	widget->parent->context = (void *)&data->position;
	data->draw(widget,rect,flags);
}

#ifdef TOUCH_SCREEN
static int close_button_callback (SsdWidget widget, const char *new_value) {

   ssd_dialog_hide_current (dec_close);
   return 0;
}
#endif

SsdWidget ssd_popup_new (const char *name,
								 const char *title,
								 PFN_ON_DIALOG_CLOSED on_popup_closed,
								 int width,
								 int height,
								 const RoadMapPosition *position,
                         int flags) {

   SsdWidget dialog, popup, container, header, text, btn_close;
   int text_size = 20;
   int header_size = 30;

#ifdef TOUCH_SCREEN
   const char *close_icons[] = {"rm_quit","rm_quit_pressed"};
#endif
   struct ssd_popup_data *data =
      (struct ssd_popup_data *)calloc (1, sizeof(*data));

   dialog = ssd_dialog_new(name,
            title,
            on_popup_closed,
            SSD_DIALOG_FLOAT|SSD_ALIGN_CENTER|SSD_CONTAINER_BORDER|SSD_ROUNDED_CORNERS|flags);
    popup =
      ssd_container_new (name, title, width, height, flags);

   if (position != NULL){
   	data->position.latitude = position->latitude;
   	data->position.longitude = position->longitude;
   }
   data->draw = popup->draw;
   popup->draw = draw;
   popup->data = data;
   popup->bg_color = NULL;

#if (defined(__SYMBIAN32__) && defined(TOUCH_SCREEN))
    header_size = 45;
#endif
    header = ssd_container_new ("header_conatiner", "", SSD_MIN_SIZE, header_size, SSD_END_ROW);
    ssd_widget_set_color(header, NULL, NULL);
    ssd_widget_set_click_offsets( header, &sgPopUpCloseOffsets );

#ifdef IPHONE
    text_size = 18;
#endif
	text = ssd_text_new("popuup_text", title, text_size, SSD_END_ROW|SSD_ALIGN_VCENTER);
	ssd_widget_set_color(text,"#ffffff", NULL);

#ifdef TOUCH_SCREEN
   container =
      ssd_container_new ("icon", "", SSD_MIN_SIZE, SSD_MIN_SIZE, SSD_ALIGN_RIGHT|SSD_WIDGET_SPACE);
   ssd_widget_set_color(container, NULL, NULL);
   btn_close = ssd_button_new ("close", "", close_icons, 2,
           SSD_ALIGN_CENTER|SSD_ALIGN_VCENTER|SSD_ALIGN_RIGHT, close_button_callback);
   ssd_widget_set_click_offsets( btn_close, &sgPopUpCloseOffsets );
   ssd_widget_add (header, btn_close );

   ssd_widget_add(header, text);
   ssd_widget_add(header, container);
#else
   ssd_widget_add(header, text);
#endif


   ssd_widget_add(popup, header);
   ssd_widget_add(dialog, popup);
   return popup;
}


void ssd_popup_update_location(SsdWidget popup, const RoadMapPosition *position){
   struct ssd_popup_data *data =
      (struct ssd_popup_data *)popup->data;
   data->position.latitude = position->latitude;
   data->position.longitude = position->longitude;
}
