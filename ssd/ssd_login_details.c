/* ssd_login_details.c
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
#include "ssd_dialog.h"
#include "ssd_button.h"
#include "ssd_container.h"
#include "ssd_text.h"
#include "ssd_entry.h"
#include "ssd_choice.h"
#include "../roadmap_keyboard.h"
#include "../roadmap_config.h"
#include "../roadmap_lang.h"
#include "Realtime/Realtime.h"
#include "Realtime/RealtimeDefs.h"
#include "ssd/ssd_checkbox.h"
#include "ssd_login_details.h"
#include "roadmap_device.h"
#include "roadmap_sound.h"
#include "roadmap_car.h"
#include "roadmap_path.h"

static const char*   title = "Profile";
static int SsdLoginShown = 0;
static const char *yesno_label[2];
static const char *yesno[2];

extern RoadMapConfigDescriptor RT_CFG_PRM_NAME_Var;
extern RoadMapConfigDescriptor RT_CFG_PRM_PASSWORD_Var;
extern RoadMapConfigDescriptor RT_CFG_PRM_NKNM_Var;
static RoadMapConfigDescriptor RoadMapConfigCarName =
                        ROADMAP_CONFIG_ITEM("Trip", "Car");

/*static void relogin (void) {

   Realtime_Start();
}*/
void OnLoginDetailsTestResults( BOOL bDetailsVerified, int iErrorCode)
{
   if( bDetailsVerified)
   {
   }
   else
   {
      // iErrorCode;
   }
}

static int on_ok( SsdWidget this, const char *new_value) {

   roadmap_config_set(&RT_CFG_PRM_NAME_Var, ssd_dialog_get_value("UserName"));
   roadmap_config_set(&RT_CFG_PRM_PASSWORD_Var, ssd_dialog_get_value("Password"));
   roadmap_config_set(&RT_CFG_PRM_NKNM_Var, ssd_dialog_get_value("Nickname"));

   Realtime_VerifyLoginDetails( OnLoginDetailsTestResults );

   ssd_dialog_hide_all(dec_cancel);

   SsdLoginShown = 0;
   return 0;
}

#ifndef TOUCH_SCREEN
static int on_ok_softkey(SsdWidget this, const char *new_value, void *context){
    return on_ok(this, new_value);
}
#endif

void on_car_changed(void){

    char *icon[2];
    char *car_name;
    const char *config_car;

    config_car = roadmap_config_get (&RoadMapConfigCarName);
    car_name = roadmap_path_join("cars", config_car);
    icon[0] = car_name;
    icon[1] = NULL;

    ssd_dialog_change_button(roadmap_lang_get ("Car"), (const char **)&icon[0], 1);
}

static int on_car_select( SsdWidget this, const char *new_value){
    roadmap_car_dialog(on_car_changed);
    return 0;
}

/*
   cnt edits      cnt labels
   +--------------+--------------+
   |  cnt1+edt1   |  name        |
   |  cnt2+edt2   |  password    |
   +--------------+--------------+
   |  space container            |
   +-----------------------------+
      btn-cancel     btn-ok

*/
void ssd_login_details_dialog_show(void) {

   static int initialized = 0;
   char *car_name;
   const char *config_car;
   char *icon[2];

   if (!initialized) {
      initialized = 1;

      roadmap_config_declare
         ("user", &RT_CFG_PRM_NAME_Var, "", NULL);
      roadmap_config_declare_password
         ("user", &RT_CFG_PRM_PASSWORD_Var, "");
      roadmap_config_declare
         ("user", &RT_CFG_PRM_NKNM_Var, "", NULL);
      roadmap_config_declare
        ("user", &RoadMapConfigCarName, "car_blue", NULL);


      // Define the labels and values
     yesno_label[0] = roadmap_lang_get ("Yes");
     yesno_label[1] = roadmap_lang_get ("No");
     yesno[0] = "Yes";
     yesno[1] = "No";
   }

   if (!ssd_dialog_activate (title, NULL)) {

      SsdWidget dialog;
      SsdWidget group;

      dialog = ssd_dialog_new (title, roadmap_lang_get(title), NULL,
                               SSD_CONTAINER_BORDER|SSD_CONTAINER_TITLE|SSD_ROUNDED_CORNERS);

      group = ssd_container_new ("Name group", NULL,
                        SSD_MAX_SIZE,SSD_MIN_SIZE,
          SSD_WIDGET_SPACE|SSD_END_ROW|SSD_WS_TABSTOP);

      ssd_widget_set_color (group, "#000000", "#ffffff");
      ssd_widget_add (group,
         ssd_text_new ("Label", roadmap_lang_get("User Name"), -1,
                        SSD_TEXT_LABEL|SSD_ALIGN_VCENTER));
      ssd_widget_add (group,
         ssd_entry_new ("UserName", "", SSD_ALIGN_VCENTER|SSD_ALIGN_RIGHT, 0));
      ssd_widget_add (dialog, group);

      group = ssd_container_new ("PW group", NULL,
                                SSD_MAX_SIZE,SSD_MIN_SIZE,
          SSD_WIDGET_SPACE|SSD_END_ROW|SSD_WS_TABSTOP);
      ssd_widget_set_color (group, "#000000", "#ffffff");

      ssd_widget_add (group,
         ssd_text_new ("Label", roadmap_lang_get("Password"), -1,
                        SSD_TEXT_LABEL|SSD_ALIGN_VCENTER));
      ssd_widget_add (group,
         ssd_entry_new ("Password", "", SSD_ALIGN_VCENTER|SSD_ALIGN_RIGHT, SSD_TEXT_PASSWORD));
      ssd_widget_add (dialog, group);

      group = ssd_container_new ("Nick group", NULL,
                  SSD_MAX_SIZE, SSD_MIN_SIZE,
          SSD_WIDGET_SPACE|SSD_END_ROW|SSD_WS_TABSTOP);
      ssd_widget_set_color (group, "#000000", "#ffffff");

      ssd_widget_add (group,
         ssd_text_new ("Label", roadmap_lang_get("Nickname"), -1,
                        SSD_TEXT_LABEL|SSD_ALIGN_VCENTER));

      ssd_widget_add (group,
         ssd_entry_new ("Nickname", "", SSD_ALIGN_VCENTER|SSD_ALIGN_RIGHT, 0));
      ssd_widget_add (dialog, group);

      group = ssd_container_new ("Car group", NULL,
                  SSD_MAX_SIZE,SSD_MIN_SIZE,
          SSD_WIDGET_SPACE|SSD_END_ROW|SSD_WS_TABSTOP);
      ssd_widget_set_color (group, "#000000", "#ffffff");

      config_car = roadmap_config_get (&RoadMapConfigCarName);
      car_name = roadmap_path_join("cars", config_car);
      icon[0] = car_name;
      icon[1] = NULL;
      ssd_widget_add (group,
         ssd_text_new ("Car Text", roadmap_lang_get("Car"), -1,
                        SSD_TEXT_LABEL|SSD_ALIGN_VCENTER));
      ssd_widget_add (group,
           ssd_button_new (roadmap_lang_get ("Car"), roadmap_lang_get(config_car), (const char **)&icon[0], 1,
                           SSD_ALIGN_CENTER,
                           on_car_select));

      ssd_widget_add (dialog, group);



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

   if (!SsdLoginShown) {
      // Case insensitive comparison
      ssd_dialog_set_value("UserName", roadmap_config_get( &RT_CFG_PRM_NAME_Var));
      ssd_dialog_set_value("Password", roadmap_config_get( &RT_CFG_PRM_PASSWORD_Var));
      ssd_dialog_set_value("Nickname", roadmap_config_get( &RT_CFG_PRM_NKNM_Var));
   }

   SsdLoginShown = 1;
   ssd_dialog_draw ();
}
