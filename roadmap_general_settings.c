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
#include "roadmap_login.h"
#include "ssd/ssd_bitmap.h"
#include "ssd/ssd_separator.h"
#include "roadmap_device.h"
#include "roadmap_math.h"
#include "roadmap_sound.h"
#include "roadmap_skin.h"
#include "roadmap_car.h"
#include "roadmap_path.h"
#include "roadmap_skin.h"
#include "roadmap_mood.h"
#include "editor/editor_screen.h"
#include "navigate/navigate_main.h"

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
static RoadMapConfigDescriptor RoadMapConfigGeneralUnit =
                            ROADMAP_CONFIG_ITEM("General", "Unit");
static RoadMapConfigDescriptor RoadMapConfigShowTicker =
                        ROADMAP_CONFIG_ITEM("User", "Show points ticker");
static RoadMapConfigDescriptor RoadMapConfigShowScreenIconsOnTap =
                        ROADMAP_CONFIG_ITEM("Map Icons", "Show on screen on tap");

extern RoadMapConfigDescriptor RoadMapConfigAutoNightMode;
extern RoadMapConfigDescriptor NavigateConfigAutoZoom;
extern RoadMapConfigDescriptor NavigateConfigNavigationGuidance;

static int on_ok( SsdWidget this, const char *new_value) {


#ifdef __SYMBIAN32__
   roadmap_config_set(&RoadMapConfigConnectionAuto, ( const char* ) ssd_dialog_get_data("AutoConnect"));
#endif

#if (defined(__SYMBIAN32__) || defined(ANDROID))
   roadmap_device_set_backlight( !( strcasecmp( ( const char* ) ssd_dialog_get_data("BackLight"), yesno[0] ) ) );

   roadmap_sound_set_volume( ( int ) ssd_dialog_get_data( "Volume Control" ) );
#endif // Symbian or android

   roadmap_config_set (&NavigateConfigAutoZoom,
                           (const char *)ssd_dialog_get_data ("autozoom"));
#ifndef TOUCH_SCREEN
   roadmap_config_set (&NavigateConfigNavigationGuidance,
                           (const char *)ssd_dialog_get_data ("navigationguidance"));

#else
   roadmap_config_set (&RoadMapConfigShowScreenIconsOnTap,
                           (const char *)ssd_dialog_get_data ("show_icons"));
   
#endif
   roadmap_config_set (&RoadMapConfigShowTicker,
                              (const char *)ssd_dialog_get_data ("show_ticker"));

   if(!strcasecmp( ( const char* ) ssd_dialog_get_data("use_metric"), yesno[0] )){
   	  roadmap_config_set (&RoadMapConfigGeneralUnit,"metric");
   	  roadmap_math_use_metric();
   }
   else{
	  roadmap_config_set (&RoadMapConfigGeneralUnit,"imperial");
	  roadmap_math_use_imperial();
   }

   roadmap_config_set (&RoadMapConfigAutoNightMode,
                              (const char *)ssd_dialog_get_data ("auto_night_mode"));
   if(!strcasecmp( ( const char* ) ssd_dialog_get_data("auto_night_mode"), yesno[0] )){
      roadmap_skin_auto_night_mode();
   }
   else{
      roadmap_skin_auto_night_mode_kill_timer();
   }

   roadmap_config_save(TRUE);
   DialogShowsShown = 0;
   return 0;
}

#ifndef TOUCH_SCREEN
static int on_ok_softkey(SsdWidget this, const char *new_value, void *context){
	on_ok(this, new_value);
	ssd_dialog_hide_all(dec_ok);
	return 0;
}
#endif

static void on_close_dialog (int exit_code, void* context){
#ifdef TOUCH_SCREEN
	if (exit_code == dec_ok)
		on_ok(NULL, NULL);
#endif
}

/////////////////////////////////////////////////////////////////////
static SsdWidget space(int height){
	SsdWidget space;
	space = ssd_container_new ("spacer", NULL, SSD_MAX_SIZE, height, SSD_WIDGET_SPACE|SSD_END_ROW);
	ssd_widget_set_color (space, NULL,NULL);
	return space;
}

void quick_settins_exit(int exit_code, void* context){

   if (exit_code != dec_ok)
		return;

   yesno[0] = "Yes";
   yesno[1] = "No";

   if(!strcasecmp( ( const char* ) ssd_dialog_get_data("light"), yesno[0] )){
   	  roadmap_skin_set_subskin ("day");
   } else {
      roadmap_skin_set_subskin ("night");
   }

   if(!strcasecmp( ( const char* ) ssd_dialog_get_data("view"), yesno[0] )){
   	  roadmap_screen_set_view (VIEW_MODE_2D);
   } else {
      roadmap_screen_set_view (VIEW_MODE_3D);
   }

   roadmap_config_set (&NavigateConfigNavigationGuidance,
                        (const char *)ssd_dialog_get_data ("navigationguidance"));

}

static void on_mood_changed(void){

   char *icon[2];
   char mood_txt[100];
   icon[0] = (char *)roadmap_mood_get();
   icon[1] = NULL;

   ssd_dialog_change_button(roadmap_lang_get ("Mood"), (const char **)&icon[0], 1);
   sprintf(mood_txt,"%s: %s", roadmap_lang_get("My mood"), roadmap_lang_get(roadmap_mood_get_name()));
   ssd_dialog_change_text("Mood Text", mood_txt);
   free(icon[0]);
}

static int on_mood_select( SsdWidget this, const char *new_value){
   roadmap_mood_dialog(on_mood_changed);
   return 0;
}


int callback (SsdWidget widget, const char *new_value){
   yesno[0] = "Yes";
   yesno[1] = "No";
   if(!strcasecmp( ( const char* ) ssd_dialog_get_data("light"), yesno[0] )){
        roadmap_skin_set_subskin ("day");
   } else {
      roadmap_skin_set_subskin ("night");
   }
   ssd_dialog_draw();
   return 1;
}

SsdWidget create_quick_setting_menu(){
#ifdef TOUCH_SCREEN
	int tab_flag = SSD_WS_TABSTOP;
#else
   	int tab_flag = SSD_WS_TABSTOP;
#endif
    BOOL checked = FALSE;
    SsdWidget box;
    SsdWidget quick_container, container, button;
	 int width = SSD_MAX_SIZE;
	 int height = 45;
	 char *icon[2];
	 const char *buttons[2];
	 char mood_txt[100];
	 const char *edit_button[] = {"edit_right", "edit_left"};
#if (defined(__SYMBIAN32__) && defined(TOUCH_SCREEN))
	 height = 53;
#endif

	     //Quick Setting Container
        quick_container = ssd_container_new ("__quick_settings", NULL, width, SSD_MIN_SIZE,
              SSD_ALIGN_CENTER|SSD_WIDGET_SPACE|SSD_END_ROW|SSD_ROUNDED_CORNERS|SSD_ROUNDED_WHITE|SSD_POINTER_NONE|SSD_CONTAINER_BORDER);

   	  //Mute
   	  box = ssd_container_new ("Mute group", NULL, SSD_MAX_SIZE, height,
                            SSD_WIDGET_SPACE|SSD_END_ROW|tab_flag);
   	  ssd_widget_set_color (box, "#000000", "#ffffff");

   	  if ( navgiate_main_voice_guidance_enabled() )
	  {
   	   	  ssd_widget_add (box,
   	      	ssd_text_new ("navigationguidance_label",
   	                     roadmap_lang_get ("Navigation guidance"),
   	                    -1, SSD_TEXT_LABEL|SSD_ALIGN_VCENTER|SSD_WIDGET_SPACE));
   		if  ( roadmap_config_match(&NavigateConfigNavigationGuidance, "yes"))
   		{
   		  checked = TRUE;
   		}
   		ssd_widget_add (box,
      	   ssd_checkbox_new ("navigationguidance", checked,  SSD_ALIGN_RIGHT, NULL,NULL,NULL,CHECKBOX_STYLE_ON_OFF));

		  ssd_widget_add(box, space(1));
		  ssd_widget_add(box, ssd_separator_new("separator", SSD_ALIGN_BOTTOM));
		  ssd_widget_add (quick_container, box);
   	  }
   	  //View 2D/3D
   	  box = ssd_container_new ("View group", NULL, SSD_MAX_SIZE, height,
                            SSD_WIDGET_SPACE|SSD_END_ROW|tab_flag);
   	  ssd_widget_set_color (box, "#000000", "#ffffff");

   	  ssd_widget_add (box,
      	ssd_text_new ("view label",
                     roadmap_lang_get ("Display"),
                    -1, SSD_TEXT_LABEL|SSD_ALIGN_VCENTER|SSD_WIDGET_SPACE));

   	  if (roadmap_screen_get_view_mdode() == VIEW_MODE_2D)
           checked = TRUE;
        else
           checked = FALSE;

   	  ssd_widget_add (box,
      	   ssd_checkbox_new ("view", checked,  SSD_ALIGN_RIGHT, NULL,"button_2d","button_3d",CHECKBOX_STYLE_ON_OFF));
   	  ssd_widget_add(box, space(1));
   	  ssd_widget_add(box, ssd_separator_new("separator", SSD_ALIGN_BOTTOM));
   	  ssd_widget_add (quick_container, box);

   	  //Light day/night
   	  box = ssd_container_new ("Light group", NULL, SSD_MAX_SIZE, height,
                            SSD_WIDGET_SPACE|SSD_END_ROW|tab_flag);
   	  ssd_widget_set_color (box, "#000000", "#ffffff");

   	  ssd_widget_add (box,
      	ssd_text_new ("light label",
                     roadmap_lang_get ("Light"),
                    -1, SSD_TEXT_LABEL|SSD_ALIGN_VCENTER|SSD_WIDGET_SPACE));

   	  ssd_widget_add (box,
      	   ssd_checkbox_new ("light", (roadmap_skin_state() == 0),  SSD_ALIGN_RIGHT, callback ,"button_day","button_night",CHECKBOX_STYLE_ON_OFF));

   	  ssd_widget_add(box, space(1));
        ssd_widget_add(box, ssd_separator_new("separator", SSD_ALIGN_BOTTOM));
        ssd_widget_add (quick_container, box);

        //Mood
   	   box = ssd_container_new ("Mood group", NULL,
   	                  SSD_MAX_SIZE,height,
   	              SSD_WIDGET_SPACE|SSD_END_ROW|tab_flag);
   	   ssd_widget_set_color (box, NULL, NULL);
   	   box->callback = on_mood_select;
   	   icon[0] = (char *)roadmap_mood_get();
   	   icon[1] = NULL;
   	   container =  ssd_container_new ("space", NULL, 47, SSD_MIN_SIZE,SSD_ALIGN_VCENTER);
   	   free(icon[0]);
   	   ssd_widget_set_color(container, NULL, NULL);
   	   
   	   ssd_widget_add (container,
   	         ssd_button_new (roadmap_lang_get ("Mood"), roadmap_lang_get(roadmap_mood_get()), (const char **)&icon[0], 1,
   	                        SSD_ALIGN_VCENTER|SSD_ALIGN_CENTER,
   	                        on_mood_select));
   	   ssd_widget_add(box, container);
   	   sprintf(mood_txt,"%s: %s", roadmap_lang_get("My mood"), roadmap_lang_get(roadmap_mood_get_name()));
   	   ssd_widget_add (box,
   	         ssd_text_new ("Mood Text", mood_txt, -1,
   	                        SSD_ALIGN_VCENTER));
#ifdef TOUCH_SCREEN
   	   if (!ssd_widget_rtl(NULL)){
   	         buttons[0] = edit_button[0];
   	         buttons[1] = edit_button[0];
   	   }else{
   	         buttons[0] = edit_button[1];
   	         buttons[1] = edit_button[1];
   	   }
   	   button = ssd_button_new ("edit_button", "", &buttons[0], 2,
   	                          SSD_ALIGN_VCENTER|SSD_ALIGN_RIGHT, on_mood_select);
   	   ssd_widget_add(box, button);
#endif
   	   ssd_widget_add (quick_container, box);
	  return quick_container;
}


void roadmap_general_settings_show(void) {

   static int initialized = 0;
   const char *pVal;
#ifdef TOUCH_SCREEN
	int tab_flag = SSD_WS_TABSTOP;
#else
   	int tab_flag = SSD_WS_TABSTOP;
#endif

   if (!initialized) {
      initialized = 1;

      roadmap_config_declare
         ("user", &RoadMapConfigConnectionAuto, "yes", NULL);
      roadmap_config_declare
         ("user", &RoadMapConfigBackLight, "yes", NULL);
      roadmap_config_declare
         ("user", &RoadMapConfigVolControl, SND_DEFAULT_VOLUME_LVL, NULL);
	  roadmap_config_declare_enumeration
      	("preferences", &RoadMapConfigGeneralUnit, NULL, "metric", "imperial", NULL);

      roadmap_config_declare_enumeration
      	("user", &RoadMapConfigShowTicker, NULL, "yes", "no", NULL);

      roadmap_config_declare_enumeration
         ("user", &RoadMapConfigShowScreenIconsOnTap, NULL, "no", "yes", NULL);

      // Define the labels and values
	 yesno_label[0] = roadmap_lang_get ("Yes");
	 yesno_label[1] = roadmap_lang_get ("No");
	 yesno[0] = "Yes";
	 yesno[1] = "No";
   }

   if (!ssd_dialog_activate (title, NULL)) {

      SsdWidget dialog;
      SsdWidget box, box2;
      SsdWidget container;
      dialog = ssd_dialog_new (title, roadmap_lang_get(title), on_close_dialog,
                               SSD_CONTAINER_TITLE);

#ifdef TOUCH_SCREEN
	  ssd_widget_add(dialog, space(5));
#endif

      container = ssd_container_new ("Conatiner Group", NULL, SSD_MAX_SIZE, SSD_MIN_SIZE,
              SSD_WIDGET_SPACE|SSD_END_ROW|SSD_ROUNDED_CORNERS|SSD_ROUNDED_WHITE|SSD_POINTER_NONE|SSD_CONTAINER_BORDER);

#ifdef __SYMBIAN32__
      //////////// Automatic connection selection box /////////////
      // TODO :: Move to another settings directory
      box = ssd_container_new ("AutoConnect Group", NULL, SSD_MAX_SIZE, SSD_MIN_SIZE,
              SSD_WIDGET_SPACE|SSD_END_ROW|tab_flag);
      ssd_widget_set_color (box, "#000000", "#ffffff");

      ssd_widget_add (box,
         ssd_text_new ( "AutoConnectLabel",
                        roadmap_lang_get ("Automatic Connection"),
                        -1, SSD_TEXT_LABEL|SSD_ALIGN_VCENTER|SSD_WIDGET_SPACE ) );


       ssd_widget_add (box,
            ssd_checkbox_new ("AutoConnect", TRUE, SSD_ALIGN_RIGHT, NULL,NULL, NULL,CHECKBOX_STYLE_ON_OFF));

      ssd_widget_add(box, space(1));
      ssd_widget_add(box, ssd_separator_new("separator", SSD_ALIGN_BOTTOM));
      ssd_widget_add (container, box);
#endif

#if (defined(__SYMBIAN32__) || defined(ANDROID))
      //////////////////////////////////////////////////////////

      ////////////  Backlight control  /////////////
      // TODO :: Move to another settings directory
      box = ssd_container_new ("BackLight Group", NULL, SSD_MAX_SIZE, SSD_MIN_SIZE,
              SSD_WIDGET_SPACE|SSD_END_ROW|tab_flag);

      ssd_widget_set_color (box, "#000000", "#ffffff");

      ssd_widget_add (box,
         ssd_text_new ( "BackLightLabel",
                        roadmap_lang_get ("Back Light On"),
                        -1, SSD_TEXT_LABEL|SSD_ALIGN_VCENTER|SSD_WIDGET_SPACE ) );

      ssd_widget_add (box,
            ssd_checkbox_new ( "BackLight", TRUE, SSD_ALIGN_RIGHT, NULL,NULL, NULL,CHECKBOX_STYLE_ON_OFF ) );

      ssd_widget_add(box, space(1));
      ssd_widget_add(box, ssd_separator_new("separator", SSD_ALIGN_BOTTOM));
      ssd_widget_add (container, box);

      //////////////////////////////////////////////////////////

      ////////////  Volume control  /////////////
      // TODO :: Move to another settings directory
      box = ssd_container_new ("Volume Control Group", NULL, SSD_MAX_SIZE, SSD_MIN_SIZE,
              SSD_WIDGET_SPACE|SSD_END_ROW|tab_flag);

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
      ssd_widget_add(box, space(1));
      ssd_widget_add(box, ssd_separator_new("separator", SSD_ALIGN_BOTTOM));
      ssd_widget_add (container, box);

      /////////////////////////////////////////////////////////
#endif // Symbian or android

   box = ssd_container_new ("autozoom group", NULL, SSD_MAX_SIZE, SSD_MIN_SIZE,
                            SSD_WIDGET_SPACE|SSD_END_ROW|tab_flag);
   ssd_widget_set_color (box, "#000000", "#ffffff");

   ssd_widget_add (box,
      ssd_text_new ("autozoom_label",
                     roadmap_lang_get ("Auto zoom"),
                    -1, SSD_TEXT_LABEL|SSD_ALIGN_VCENTER|SSD_WIDGET_SPACE));

   ssd_widget_add (box,
         ssd_checkbox_new ("autozoom", TRUE,  SSD_ALIGN_RIGHT, NULL,NULL,NULL,CHECKBOX_STYLE_ON_OFF));

   ssd_widget_add(box, space(1));
   ssd_widget_add(box, ssd_separator_new("separator", SSD_ALIGN_BOTTOM));
   ssd_widget_add (container, box);

#ifndef TOUCH_SCREEN
   box = ssd_container_new ("navigationguidance group", NULL, SSD_MAX_SIZE, SSD_MIN_SIZE,
                            SSD_WIDGET_SPACE|SSD_END_ROW|tab_flag);
   ssd_widget_set_color (box, "#000000", "#ffffff");

   ssd_widget_add (box,
      ssd_text_new ("navigationguidance_label",
                     roadmap_lang_get ("Navigation guidance"),
                    -1, SSD_TEXT_LABEL|SSD_ALIGN_VCENTER|SSD_WIDGET_SPACE));

    ssd_widget_add (box,
         ssd_checkbox_new ("navigationguidance", TRUE,  SSD_ALIGN_RIGHT, NULL,NULL,NULL,CHECKBOX_STYLE_ON_OFF));

   ssd_widget_add(box, space(1));
   ssd_widget_add(box, ssd_separator_new("separator", SSD_ALIGN_BOTTOM));


   ssd_widget_add (container, box);
#endif

   //Show Ticker
   box = ssd_container_new ("show ticker group", NULL, SSD_MAX_SIZE, SSD_MIN_SIZE,
                            SSD_WIDGET_SPACE|SSD_END_ROW|tab_flag);
   ssd_widget_set_color (box, "#000000", "#ffffff");

   ssd_widget_add (box,
        ssd_text_new ("show_ticker_label",
                      roadmap_lang_get ("Show points ticker"),
                     -1, SSD_TEXT_LABEL|SSD_ALIGN_VCENTER|SSD_WIDGET_SPACE));

   ssd_widget_add (box,
         ssd_checkbox_new ("show_ticker", TRUE,  SSD_ALIGN_RIGHT, NULL,NULL,NULL,CHECKBOX_STYLE_ON_OFF));

   ssd_widget_add(box, space(1));
   ssd_widget_add(box, ssd_separator_new("separator", SSD_ALIGN_BOTTOM));
   ssd_widget_add (container, box);


   //Auto Night mode
    box = ssd_container_new ("Auto night mode group", NULL, SSD_MAX_SIZE, SSD_MIN_SIZE,
                             SSD_WIDGET_SPACE|SSD_END_ROW|tab_flag);
    ssd_widget_set_color (box, "#000000", "#ffffff");

    ssd_widget_add (box,
         ssd_text_new ("auto_night_label",
                       roadmap_lang_get ("Automatic night mode"),
                      -1, SSD_TEXT_LABEL|SSD_ALIGN_VCENTER));

    ssd_widget_add (box,
          ssd_checkbox_new ("auto_night_mode", TRUE,  SSD_ALIGN_RIGHT, NULL,NULL,NULL,CHECKBOX_STYLE_ON_OFF));

    ssd_widget_add(box, space(1));
    ssd_widget_add(box, ssd_separator_new("separator", SSD_ALIGN_BOTTOM));
    ssd_widget_add (container, box);

#ifdef TOUCH_SCREEN
   //Show Icons
   box = ssd_container_new ("show icons group", NULL, SSD_MAX_SIZE, SSD_MIN_SIZE,
                            SSD_WIDGET_SPACE|SSD_END_ROW|tab_flag);
   ssd_widget_set_color (box, "#000000", "#ffffff");

   box2 = ssd_container_new ("show icons group", NULL, roadmap_canvas_width()/2, SSD_MIN_SIZE,
                            SSD_ALIGN_VCENTER|tab_flag);
   ssd_widget_set_color (box, "#000000", "#ffffff");

   ssd_widget_add (box2,
        ssd_text_new ("show_icons_label",
                      roadmap_lang_get ("Display map controls on tap"),
                     -1, SSD_TEXT_LABEL|SSD_ALIGN_VCENTER));

   ssd_widget_add(box, box2);
   ssd_widget_add (box,
         ssd_checkbox_new ("show_icons", TRUE,  SSD_ALIGN_RIGHT, NULL,NULL,NULL,CHECKBOX_STYLE_ON_OFF));

   ssd_widget_add(box, space(1));
   ssd_widget_add(box, ssd_separator_new("separator", SSD_ALIGN_BOTTOM));
   ssd_widget_add (container, box);
#endif
   
   //General Units
   box = ssd_container_new ("use_metric group", NULL, SSD_MAX_SIZE, SSD_MIN_SIZE,
                            SSD_WIDGET_SPACE|SSD_END_ROW|tab_flag);
   ssd_widget_set_color (box, "#000000", "#ffffff");

   ssd_widget_add (box,
      ssd_text_new ("use_metric_label",
                     roadmap_lang_get ("Measurement system"),
                    -1, SSD_TEXT_LABEL|SSD_ALIGN_VCENTER|SSD_WIDGET_SPACE));

   ssd_widget_add (box,
         ssd_checkbox_new ("use_metric", TRUE,  SSD_ALIGN_RIGHT, NULL,"button_meters", "button_miles", CHECKBOX_STYLE_ON_OFF));

   ssd_widget_add(box, space(1));
   
   ssd_widget_add (container, box);

   
   ssd_widget_add(dialog, container);

#ifndef TOUCH_SCREEN
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
#endif

#if (defined(__SYMBIAN32__) || defined(ANDROID))
   	  pVal = roadmap_config_match( &RoadMapConfigBackLight, yesno[0] ) ? yesno[0] : yesno[1];
      ssd_dialog_set_data("BackLight", pVal );
      ssd_dialog_set_data("Volume Control", ( void* ) roadmap_config_get_integer( &RoadMapConfigVolControl ) );
#endif // Symbian or android

	  if (roadmap_config_match(&NavigateConfigAutoZoom, "yes")) pVal = yesno[0];
   	  else pVal = yesno[1];
   	  ssd_dialog_set_data ("autozoom", (void *) pVal);

#ifndef TOUCH_SCREEN
   	  if (roadmap_config_match(&NavigateConfigNavigationGuidance, "yes")) pVal = yesno[0];
   	  else pVal = yesno[1];
   	  ssd_dialog_set_data ("navigationguidance", (void *) pVal);

#else
   	  if (roadmap_config_match(&RoadMapConfigShowScreenIconsOnTap, "yes")) pVal = yesno[0];
        else pVal = yesno[1];
        ssd_dialog_set_data ("show_icons", (void *) pVal);
#endif
   	  if (roadmap_config_match(&RoadMapConfigGeneralUnit, "metric")) pVal = yesno[0];
   	  else pVal = yesno[1];
   	  ssd_dialog_set_data ("use_metric", (void *) pVal);

  	     if (roadmap_config_match(&RoadMapConfigShowTicker, "yes")) pVal = yesno[0];
  	     else pVal = yesno[1];
  	     ssd_dialog_set_data ("show_ticker", (void *) pVal);

        if (roadmap_config_match(&RoadMapConfigAutoNightMode, "yes")) pVal = yesno[0];
        else pVal = yesno[1];
        ssd_dialog_set_data ("auto_night_mode", (void *) pVal);

   }

   DialogShowsShown = 1;
   ssd_dialog_draw ();
}
