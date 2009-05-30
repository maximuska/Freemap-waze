/*  roadmap_general_settings.c
 *
 * LICENSE:
 *
 *   Copyright 2008 Ehud Shabtai
 *
 *   RoadMap is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License V2 as published by
 *   the Free Software Foundation.
 *
 *   RoadMap is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with RoadMap; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */


#include <assert.h>
#include <string.h>
#include "ssd/ssd_dialog.h"
#include "ssd/ssd_button.h"
#include "ssd/ssd_container.h"
#include "ssd/ssd_text.h"
#include "ssd/ssd_choice.h"
#include "roadmap_keyboard.h"
#include "roadmap_config.h"
#include "roadmap_lang.h"
#include "Realtime/Realtime.h"
#include "Realtime/RealtimeDefs.h"
#include "ssd/ssd_checkbox.h"
#include "ssd/ssd_login_details.h"
#include "roadmap_device.h"
#include "roadmap_sound.h"
#include "roadmap_car.h"
#include "roadmap_path.h"

static const char*   title = "General settings";
static int DialogShowsShown = 0;
static const char *yesno_label[2];
static const char *yesno[2];
static RoadMapConfigDescriptor RoadMapConfigConnectionAuto =
                        ROADMAP_CONFIG_ITEM("Connection", "AutoConnect");
static RoadMapConfigDescriptor RoadMapConfigBackLight =
                        ROADMAP_CONFIG_ITEM("Display", "BackLight");
static RoadMapConfigDescriptor RoadMapConfigVolControl =
                        ROADMAP_CONFIG_ITEM( "Voice", "Volume Control" );
                
extern RoadMapConfigDescriptor NavigateConfigAutoZoom;
extern RoadMapConfigDescriptor NavigateConfigNavigationGuidance;
                        
static int on_ok( SsdWidget this, const char *new_value) {

  
#ifdef __SYMBIAN32__
   roadmap_config_set(&RoadMapConfigConnectionAuto, ( const char* ) ssd_dialog_get_data("AutoConnect"));

   roadmap_device_set_backlight( !( strcasecmp( ( const char* ) ssd_dialog_get_data("BackLight"), yesno[0] ) ) );

   roadmap_sound_set_volume( ( int ) ssd_dialog_get_data( "Volume Control" ) );
#endif

   roadmap_config_set (&NavigateConfigAutoZoom,
                           (const char *)ssd_dialog_get_data ("autozoom"));
   roadmap_config_set (&NavigateConfigNavigationGuidance,
                           (const char *)ssd_dialog_get_data ("navigationguidance"));

   ssd_dialog_hide_all(dec_cancel);

   DialogShowsShown = 0;
   return 0;
}

#ifndef TOUCH_SCREEN
static int on_ok_softkey(SsdWidget this, const char *new_value, void *context){
	return on_ok(this, new_value);
}
#endif


void roadmap_general_settings_show(void) {

   static int initialized = 0;
   const char *pVal;
   
   if (!initialized) {
      initialized = 1;

      roadmap_config_declare
         ("user", &RoadMapConfigConnectionAuto, "yes", NULL);
      roadmap_config_declare
         ("user", &RoadMapConfigBackLight, "yes", NULL);
      roadmap_config_declare
         ("user", &RoadMapConfigVolControl, SND_DEFAULT_VOLUME_LVL, NULL);
      
      // Define the labels and values
	 yesno_label[0] = roadmap_lang_get ("Yes");
	 yesno_label[1] = roadmap_lang_get ("No");
	 yesno[0] = "Yes";
	 yesno[1] = "No";
   }

   if (!ssd_dialog_activate (title, NULL)) {

      SsdWidget dialog;
      SsdWidget box;
      
      dialog = ssd_dialog_new (title, roadmap_lang_get(title), NULL,
                               SSD_CONTAINER_BORDER|SSD_CONTAINER_TITLE|SSD_ROUNDED_CORNERS);


#ifdef __SYMBIAN32__
      //////////// Automatic connection selection box /////////////
      // TODO :: Move to another settings directory
      box = ssd_container_new ("AutoConnect Group", NULL, SSD_MAX_SIZE, SSD_MIN_SIZE,
                               SSD_WIDGET_SPACE|SSD_END_ROW);
      ssd_widget_set_color (box, "#000000", "#ffffff");

      ssd_widget_add (box,
         ssd_text_new ( "AutoConnectLabel",
                        roadmap_lang_get ("Automatic Connection"),
                        -1, SSD_TEXT_LABEL|SSD_ALIGN_VCENTER|SSD_WIDGET_SPACE ) );

		
       ssd_widget_add (box,
            ssd_checkbox_new ("AutoConnect", TRUE, SSD_ALIGN_RIGHT, NULL,CHECKBOX_STYLE_DEFAULT));

      ssd_widget_add (dialog, box);
      //////////////////////////////////////////////////////////
      
      ////////////  Backlight control  /////////////
      // TODO :: Move to another settings directory
      box = ssd_container_new ("BackLight Group", NULL, SSD_MAX_SIZE, SSD_MIN_SIZE,
                               SSD_WIDGET_SPACE|SSD_END_ROW);
      ssd_widget_set_color (box, "#000000", "#ffffff");

      ssd_widget_add (box,
         ssd_text_new ( "BackLightLabel",
                        roadmap_lang_get ("Back Light On"),
                        -1, SSD_TEXT_LABEL|SSD_ALIGN_VCENTER|SSD_WIDGET_SPACE ) );
		
      ssd_widget_add (box,
            ssd_checkbox_new ( "BackLight", TRUE, SSD_ALIGN_RIGHT, NULL,CHECKBOX_STYLE_DEFAULT ) );

      ssd_widget_add (dialog, box);

      //////////////////////////////////////////////////////////
      
      ////////////  Volume control  /////////////
      // TODO :: Move to another settings directory
      box = ssd_container_new ("Volume Control Group", NULL, SSD_MAX_SIZE, SSD_MIN_SIZE,
                               SSD_WIDGET_SPACE|SSD_END_ROW);
      ssd_widget_set_color (box, "#000000", NULL);
      ssd_widget_set_color (box, "#000000", "#ffffff");

      ssd_widget_add (box,
         ssd_text_new ( "VolumeCtrlLabel",
                        roadmap_lang_get ("Volume Control"),
                        -1, SSD_TEXT_LABEL|SSD_ALIGN_VCENTER|SSD_WIDGET_SPACE ) );
      
      ssd_widget_add (box,
         ssd_choice_new ( "Volume Control", SND_VOLUME_LVLS_COUNT,
                                 SND_VOLUME_LVLS_LABELS,
                                 ( const void** ) SND_VOLUME_LVLS,
                                 SSD_ALIGN_RIGHT|SSD_ALIGN_VCENTER, NULL) );
      
      ssd_widget_add (dialog, box);
      
      /////////////////////////////////////////////////////////
#endif

   box = ssd_container_new ("autozoom group", NULL, SSD_MAX_SIZE, SSD_MIN_SIZE,
                            SSD_WIDGET_SPACE|SSD_END_ROW);
   ssd_widget_set_color (box, "#000000", "#ffffff");
   
   ssd_widget_add (box,
      ssd_text_new ("autozoom_label",
                     roadmap_lang_get ("Auto zoom"),
                    -1, SSD_TEXT_LABEL|SSD_ALIGN_VCENTER|SSD_WIDGET_SPACE));

   ssd_widget_add (box,
         ssd_checkbox_new ("autozoom", TRUE,  SSD_ALIGN_RIGHT, NULL,CHECKBOX_STYLE_DEFAULT));
                         
   ssd_widget_add (dialog, box);
   
   box = ssd_container_new ("navigationguidance group", NULL, SSD_MAX_SIZE, SSD_MIN_SIZE,
                            SSD_WIDGET_SPACE|SSD_END_ROW);
   ssd_widget_set_color (box, "#000000", "#ffffff");
   
   ssd_widget_add (box,
      ssd_text_new ("navigationguidance_label",
                     roadmap_lang_get ("Navigation Guidance"),
                    -1, SSD_TEXT_LABEL|SSD_ALIGN_VCENTER|SSD_WIDGET_SPACE));

    ssd_widget_add (box,
         ssd_checkbox_new ("navigationguidance", TRUE,  SSD_ALIGN_RIGHT, NULL,CHECKBOX_STYLE_DEFAULT));                         
        
   ssd_widget_add (dialog, box);

#ifdef TOUCH_SCREEN      
      ssd_widget_add (dialog,
         ssd_button_label ("confirm", roadmap_lang_get ("Ok"),
                        SSD_ALIGN_CENTER|SSD_START_NEW_ROW|SSD_WS_TABSTOP|SSD_ALIGN_BOTTOM,
                        on_ok));
#else
   ssd_widget_set_left_softkey_text       ( dialog, roadmap_lang_get("Ok"));
   ssd_widget_set_left_softkey_callback   ( dialog, on_ok_softkey);
#endif
      ssd_dialog_activate (title, NULL);
   }

   if (!DialogShowsShown) {
   	  // Case insensitive comparison
#ifdef __SYMBIAN32__
   	  pVal = roadmap_config_match( &RoadMapConfigConnectionAuto, yesno[0] ) ? yesno[0] : yesno[1];
   	  ssd_dialog_set_data("AutoConnect", pVal );
   	  pVal = roadmap_config_match( &RoadMapConfigBackLight, yesno[0] ) ? yesno[0] : yesno[1];
      ssd_dialog_set_data("BackLight", pVal );
      ssd_dialog_set_data("Volume Control", ( void* ) roadmap_config_get_integer( &RoadMapConfigVolControl ) );
#endif

	  if (roadmap_config_match(&NavigateConfigAutoZoom, "yes")) pVal = yesno[0];
   	  else pVal = yesno[1];
   	  ssd_dialog_set_data ("autozoom", (void *) pVal);

   	  if (roadmap_config_match(&NavigateConfigNavigationGuidance, "yes")) pVal = yesno[0];
   	  else pVal = yesno[1];
   	  ssd_dialog_set_data ("navigationguidance", (void *) pVal);

   }

   DialogShowsShown = 1;
   ssd_dialog_draw ();
}
