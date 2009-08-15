/* ssd_button.c - Bitmap button widget
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
 *   See ssd_button.h
 */

#include <string.h>
#include <stdlib.h>

#include "roadmap.h"
#include "roadmap_res.h"
#include "roadmap_canvas.h"
#include "roadmap_sound.h"
#include "roadmap_screen.h"
#include "roadmap_main.h"

#include "roadmap_keyboard.h"

#include "ssd_widget.h"
#include "ssd_text.h"
#include "ssd_button.h"

/* Buttons states */
#define BUTTON_STATE_NORMAL   0
#define BUTTON_STATE_SELECTED 1
#define BUTTON_STATE_DISABLED 2
#define MAX_STATES            3

struct ssd_button_data {
   int state;
   const char *bitmaps[MAX_STATES];
};


void get_state (SsdWidget widget, int *state, RoadMapImage *image, int *image_state) {

   struct ssd_button_data *data = (struct ssd_button_data *) widget->data;
   int i;

   if ((widget->in_focus ))
      *state = IMAGE_SELECTED;
   else
      *state = data->state;

   for (i=*state; i>=0; i--) {
      if (data->bitmaps[i] &&
         (*image = roadmap_res_get (RES_BITMAP, RES_SKIN, data->bitmaps[i]))) {
         break;
      }
   }
   *image_state = i;
}


static void draw (SsdWidget widget, RoadMapGuiRect *rect, int flags) {

   struct ssd_button_data *data = (struct ssd_button_data *) widget->data;
   RoadMapImage image = NULL;
   RoadMapGuiPoint point;
   int image_state;
   int state;

   point.x = rect->minx;
   point.y = rect->miny;

   get_state(widget, &state, &image, &image_state);

   if ((flags & SSD_GET_SIZE)) {
    if (data->bitmaps[state] != NULL)
            image = roadmap_res_get (RES_BITMAP, RES_SKIN, data->bitmaps[state]);
      else
        image = roadmap_res_get (RES_BITMAP, RES_SKIN, data->bitmaps[0]);
      if (!image) {
         widget->size.height = widget->size.width = 0;
         return;
      }

      rect->maxx = rect->minx + roadmap_canvas_image_width(image);
      rect->maxy = rect->minx + roadmap_canvas_image_height(image);

    return;
   }


   if (!image) {
      roadmap_log (ROADMAP_ERROR, "SSD - Can't get image for button widget: %s",
      widget->name);
      return;
   }

   switch (state) {
   case BUTTON_STATE_NORMAL:
      roadmap_canvas_draw_image (image, &point, 0, IMAGE_NORMAL);
      break;
   case BUTTON_STATE_SELECTED:
     if (image_state == state) {
         roadmap_canvas_draw_image (image, &point, 0, IMAGE_NORMAL);
      } else {
         roadmap_canvas_draw_image (image, &point, 0, IMAGE_SELECTED);
      }
      break;
   }
}


static int ssd_button_pointer_down (SsdWidget widget,
                                    const RoadMapGuiPoint *point) {
   struct ssd_button_data *data = (struct ssd_button_data *) widget->data;

   data->state = BUTTON_STATE_SELECTED;

   return 1;
}

static SsdWidget delayed_widget;

static void button_callbak (void) {

    struct ssd_button_data *data = (struct ssd_button_data *) delayed_widget->data;

    roadmap_main_remove_periodic (button_callbak);

   if (delayed_widget->callback) {
      (*delayed_widget->callback) (delayed_widget, SSD_BUTTON_SHORT_CLICK);
   }

    data->state = BUTTON_STATE_NORMAL;
    roadmap_screen_redraw ();
}

static int ssd_button_short_click (SsdWidget widget,
                                   const RoadMapGuiPoint *point) {
   struct ssd_button_data *data = (struct ssd_button_data *) widget->data;

   static RoadMapSoundList list;

   if (!list) {
      list = roadmap_sound_list_create (SOUND_LIST_NO_FREE);
      roadmap_sound_list_add (list, "click");
      roadmap_res_get (RES_SOUND, 0, "click");
   }

   roadmap_sound_play_list (list);

#ifdef IPHONE
    if (widget->callback == NULL){
        data->state = BUTTON_STATE_NORMAL;
        roadmap_screen_redraw ();
        return 0;
    }
    else{
        delayed_widget = widget;
        roadmap_main_set_periodic (300, button_callbak);
    }
#else
   if (widget->callback) {
      (*widget->callback) (widget, SSD_BUTTON_SHORT_CLICK);
   }

    data->state = BUTTON_STATE_NORMAL;
    roadmap_screen_redraw ();
#endif

   return 1;
}


static int ssd_button_long_click (SsdWidget widget,
                                  const RoadMapGuiPoint *point) {
   struct ssd_button_data *data = (struct ssd_button_data *) widget->data;

   static RoadMapSoundList list;
   if (!list) {
      list = roadmap_sound_list_create (SOUND_LIST_NO_FREE);
      roadmap_sound_list_add (list, "click_long");
      roadmap_res_get (RES_SOUND, 0, "click_long");
   }

   roadmap_sound_play_list (list);

   if (widget->callback) {
      (*widget->callback) (widget, SSD_BUTTON_LONG_CLICK);
   }

   data->state = BUTTON_STATE_NORMAL;

   return 1;
}


static int set_value (SsdWidget widget, const char *value) {
   struct ssd_button_data *data = (struct ssd_button_data *) widget->data;
   RoadMapImage bmp;
    int max_width = 0;
    int max_height = 0;
    int is_different = 0;
   int i;

   if (widget->value && *widget->value) free (widget->value);

   if (*value) widget->value = strdup(value);
   else widget->value = "";

   if (widget->flags & SSD_VAR_SIZE) {
    widget->size.height = -1;
    widget->size.width  = -1;
      return 0;
   }

    for (i=0; i<MAX_STATES; i++) {
      if (!data->bitmaps[i]) continue;

      bmp = roadmap_res_get (RES_BITMAP, RES_SKIN, data->bitmaps[i]);
      if (!bmp) continue;

      if (!max_width || !max_height) {
         max_width  = roadmap_canvas_image_width(bmp);
         max_height = roadmap_canvas_image_height(bmp);
      } else {
         int val = roadmap_canvas_image_width(bmp);
         if (max_width != val) {
            is_different = 1;
            if (val > max_width) max_width = val;
         }

         val = roadmap_canvas_image_height(bmp);
         if (max_height != val) {
            is_different = 1;
            if (val > max_height) max_height = val;
         }
      }
   }

    widget->size.height = max_height;
    widget->size.width  = max_width;

   return 0;
}

static BOOL ssd_button_on_key_pressed (SsdWidget button, const char* utf8char, uint32_t flags)
{
   if( KEY_IS_ENTER)
   {
      button->callback(button, SSD_BUTTON_SHORT_CLICK);
      return TRUE;
   }

   return FALSE;
}


SsdWidget ssd_button_new (const char *name, const char *value,
                          const char **bitmaps, int num_bitmaps,
                          int flags, SsdCallback callback) {

   SsdWidget w;
   struct ssd_button_data *data =
      (struct ssd_button_data *)calloc (1, sizeof(*data));
   int i;

   w = ssd_widget_new (name, ssd_button_on_key_pressed, flags);

   w->_typeid = "Button";

   w->draw  = draw;
   w->flags = flags;

   data->state  = BUTTON_STATE_NORMAL;
   for (i=0; i<num_bitmaps; i++) data->bitmaps[i] = bitmaps[i];

   w->data = data;
   w->callback = callback;

   set_value (w, value);

   w->pointer_down = ssd_button_pointer_down;
   w->short_click  = ssd_button_short_click;
   w->long_click   = ssd_button_long_click;
   w->set_value    = set_value;

   return w;
}


int ssd_button_change_icon( SsdWidget widget, const char **bitmaps, int num_bitmaps){
    int i;
    struct ssd_button_data *data = (struct ssd_button_data *) widget->data;
    RoadMapImage bmp;

    for (i=0; i<num_bitmaps; i++) data->bitmaps[i] = bitmaps[i];


    bmp = roadmap_res_get (RES_BITMAP, RES_SKIN, data->bitmaps[0]);
    if (!bmp) {
      widget->size.height = widget->size.width = 0;
      return -1;
   }

    widget->size.height = roadmap_canvas_image_height(bmp);
    widget->size.width  = roadmap_canvas_image_width(bmp);

    return 0;
}


SsdWidget ssd_button_label (const char *name, const char *label,
                            int flags, SsdCallback callback) {

   const char *button_icon[]   = {"button_up", "button_down"};
   SsdWidget text;
   SsdWidget button = ssd_button_new (name, "", button_icon, 2,
                                      flags, callback);

   text = ssd_text_new ("label", label, -1, SSD_ALIGN_VCENTER| SSD_ALIGN_CENTER) ;
#ifdef IPHONE
   ssd_widget_set_color(text, "#ffffff", "#ffffff");
#else
   ssd_widget_set_color(text, "#000000", "#000000");
#endif
   ssd_widget_add (button,text);

   return button;
}

