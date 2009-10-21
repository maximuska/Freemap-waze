/* roadmap_twitter.c - Manages Twitter accont
 *
 * LICENSE:
 *
 *   Copyright 2008 Avi B.S
 *
 *   This file is part of RoadMap.
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
 *
 *
 */

#include <stdlib.h>
#include <string.h>

#include "roadmap_main.h"
#include "roadmap_config.h"
#include "roadmap_twitter.h"
#include "roadmap_dialog.h"
#include "roadmap_messagebox.h"
#include "ssd/ssd_dialog.h"
#include "ssd/ssd_container.h"
#include "ssd/ssd_text.h"
#include "ssd/ssd_choice.h"
#include "ssd/ssd_button.h"
#include "ssd/ssd_entry.h"
#include "ssd/ssd_checkbox.h"
#include "ssd/ssd_separator.h"

#include "Realtime/Realtime.h"
#include "Realtime/RealtimeDefs.h"

static const char *yesno[2];

//   User name
RoadMapConfigDescriptor TWITTER_CFG_PRM_NAME_Var = ROADMAP_CONFIG_ITEM(
      TWITTER_CONFIG_TAB,
      TWITTER_CFG_PRM_NAME_Name);

//   Nickname
RoadMapConfigDescriptor TWITTER_CFG_PRM_PASSWORD_Var = ROADMAP_CONFIG_ITEM(
      TWITTER_CONFIG_TAB,
      TWITTER_CFG_PRM_PASSWORD_Name);

//   Enable / Disable Sending Twitts
RoadMapConfigDescriptor TWITTER_CFG_PRM_SEND_TWITTS_Var = ROADMAP_CONFIG_ITEM(
      TWITTER_CONFIG_TAB,
      RT_CFG_PRM_SEND_TWITTS_Name);

/////////////////////////////////////////////////////////////////////////////////////
BOOL roadmap_twitter_initialize(void) {

   // Name
   roadmap_config_declare(TWITTER_CONFIG_TYPE, &TWITTER_CFG_PRM_NAME_Var,
         TWITTER_CFG_PRM_NAME_Default, NULL);

   // Password
   roadmap_config_declare_password(TWITTER_CONFIG_TYPE,
         &TWITTER_CFG_PRM_PASSWORD_Var, TWITTER_CFG_PRM_PASSWORD_Default);

   //   Enable / Disable sending to Twitter
   roadmap_config_declare_enumeration(TWITTER_CONFIG_TYPE,
         &TWITTER_CFG_PRM_SEND_TWITTS_Var, NULL,
         RT_CFG_PRM_SEND_TWITTS_Disabled, RT_CFG_PRM_SEND_TWITTS_Enabled, NULL);
   yesno[0] = "Yes";
   yesno[1] = "No";

   return TRUE;

}

/////////////////////////////////////////////////////////////////////////////////////
const char *roadmap_twitter_get_username(void) {
   return roadmap_config_get(&TWITTER_CFG_PRM_NAME_Var);
}

/////////////////////////////////////////////////////////////////////////////////////
const char *roadmap_twitter_get_password(void) {
   return roadmap_config_get(&TWITTER_CFG_PRM_PASSWORD_Var);
}

/////////////////////////////////////////////////////////////////////////////////////
void roadmap_twitter_set_username(const char *user_name) {
   roadmap_config_set(&TWITTER_CFG_PRM_NAME_Var, user_name);
}

/////////////////////////////////////////////////////////////////////////////////////
void roadmap_twitter_set_password(const char *password) {
   roadmap_config_set(&TWITTER_CFG_PRM_PASSWORD_Var, password);
}

/////////////////////////////////////////////////////////////////////////////////////
BOOL roadmap_twitter_is_sending_enabled(void) {
   if (0 == strcmp(roadmap_config_get(&TWITTER_CFG_PRM_SEND_TWITTS_Var),
         RT_CFG_PRM_SEND_TWITTS_Enabled))
      return TRUE;
   return FALSE;
}

/////////////////////////////////////////////////////////////////////////////////////
void roadmap_twitter_enable_sending() {
   roadmap_config_set(&TWITTER_CFG_PRM_SEND_TWITTS_Var,
         RT_CFG_PRM_SEND_TWITTS_Enabled);
}

/////////////////////////////////////////////////////////////////////////////////////
void roadmap_twitter_diable_sending() {
   roadmap_config_set(&TWITTER_CFG_PRM_SEND_TWITTS_Var,
         RT_CFG_PRM_SEND_TWITTS_Disabled);
   roadmap_config_save(TRUE);
}

static void twitter_un_empty(void){
   roadmap_main_remove_periodic (twitter_un_empty);
   roadmap_twitter_setting_dialog();
   roadmap_messagebox("Error", "Twitter user name is empty");
}

static void twitter_pw_empty(void){
   roadmap_main_remove_periodic (twitter_pw_empty);
   roadmap_twitter_setting_dialog();
   roadmap_messagebox("Error", "Twitter password is empty");
}

static void twitter_network_error(void){
   roadmap_main_remove_periodic (twitter_network_error);
   roadmap_twitter_setting_dialog();
   roadmap_messagebox("Error", roadmap_lang_get("There is no network connection.Updating your twitter account details failed."));
}
/////////////////////////////////////////////////////////////////////////////////////
static int on_ok() {
   BOOL success;
   roadmap_twitter_set_username(ssd_dialog_get_value("TwitterUserName"));
   roadmap_twitter_set_password(ssd_dialog_get_value("TwitterPassword"));
   if (!strcasecmp((const char*) ssd_dialog_get_data("TwitterSendTwitts"),
         yesno[0])) {
      const char * user_name = ssd_dialog_get_value("TwitterUserName");
      const char * password = ssd_dialog_get_value("TwitterPassword");

      roadmap_twitter_enable_sending();
      if (user_name[0] == 0) {
         roadmap_main_set_periodic (100, twitter_un_empty);
         roadmap_twitter_diable_sending();
         return 1;
      }

      if (password[0] == 0) {
         roadmap_main_set_periodic (100, twitter_pw_empty);
         roadmap_twitter_diable_sending();
         return 1;
      }

      success = Realtime_TwitterConnect(user_name, password);
      if (success) {
         roadmap_config_save(TRUE);
      } else {
         roadmap_main_set_periodic (100, twitter_network_error);
      }
   } else {
      roadmap_twitter_diable_sending();
   }

   return 0;
}

/////////////////////////////////////////////////////////////////////////////////////
static void on_dlg_close(int exit_code, void* context) {
   if (exit_code == dec_ok)
      on_ok();
}


#ifndef TOUCH_SCREEN
/////////////////////////////////////////////////////////////////////////////////////
static int on_ok_softkey(SsdWidget widget, const char *new_value, void *context) {
   ssd_dialog_hide_current(dec_ok);
   return 0;
}
#endif
/////////////////////////////////////////////////////////////////////
static SsdWidget space(int height) {
   SsdWidget space;
   space = ssd_container_new("spacer", NULL, SSD_MAX_SIZE, height,
         SSD_WIDGET_SPACE | SSD_END_ROW);
   ssd_widget_set_color(space, NULL, NULL);
   return space;
}

/////////////////////////////////////////////////////////////////////////////////////
static void create_dialog(void) {

   SsdWidget dialog;
   SsdWidget group;
   SsdWidget box;
   int width;
   int tab_flag = SSD_WS_TABSTOP;

   width = roadmap_canvas_width()/2;
   
   dialog = ssd_dialog_new(TWITTER_DIALOG_NAME,
         roadmap_lang_get(TWITTER_TITTLE), on_dlg_close, SSD_CONTAINER_TITLE);

#ifdef TOUCH_SCREEN
   ssd_widget_add(dialog, space(5));
#endif	  

   box = ssd_container_new("UN/PW group", NULL, SSD_MAX_SIZE, SSD_MIN_SIZE,
         SSD_WIDGET_SPACE | SSD_END_ROW | SSD_ROUNDED_CORNERS
               | SSD_ROUNDED_WHITE | SSD_POINTER_NONE | SSD_CONTAINER_BORDER);

   //Send Twitts Yes/No
   group = ssd_container_new("Send_Twitts group", NULL, SSD_MAX_SIZE, SSD_MIN_SIZE,
         SSD_WIDGET_SPACE | SSD_END_ROW | tab_flag);
   ssd_widget_set_color(group, "#000000", "#ffffff");

   ssd_widget_add(group, ssd_text_new("Send_twitts_label", roadmap_lang_get(
         "Tweet my reports to my followers"), -1, SSD_TEXT_LABEL
         | SSD_ALIGN_VCENTER | SSD_WIDGET_SPACE));

   ssd_widget_add(group, ssd_checkbox_new("TwitterSendTwitts", TRUE,
         SSD_END_ROW | SSD_ALIGN_RIGHT | SSD_ALIGN_VCENTER, NULL, NULL, NULL,
         CHECKBOX_STYLE_ON_OFF));

   ssd_widget_add(group, ssd_separator_new("separator", SSD_ALIGN_BOTTOM));
   ssd_widget_add(box, group);
   //User name

   group = ssd_container_new("Twitter Name group", NULL, SSD_MAX_SIZE,SSD_MIN_SIZE,
         SSD_WIDGET_SPACE | SSD_END_ROW);
   ssd_widget_add(group, space(5));
   ssd_widget_set_color(group, "#000000", "#ffffff");
   ssd_widget_add(group, ssd_text_new("Label", roadmap_lang_get("User name"),
         -1, SSD_TEXT_LABEL | SSD_ALIGN_VCENTER));
   ssd_widget_add(group, ssd_entry_new("TwitterUserName", "", SSD_ALIGN_VCENTER
         | SSD_ALIGN_RIGHT | tab_flag, 0, width, SSD_MIN_SIZE,
         roadmap_lang_get("User name")));
   ssd_widget_add(box, group);

   //Password
   group = ssd_container_new("Twitter PW group", NULL, SSD_MAX_SIZE, 40,
         SSD_WIDGET_SPACE | SSD_END_ROW);
   ssd_widget_set_color(group, "#000000", "#ffffff");

   ssd_widget_add(group, ssd_text_new("Label", roadmap_lang_get("Password"),
         -1, SSD_TEXT_LABEL | SSD_ALIGN_VCENTER));
   ssd_widget_add(group, ssd_entry_new("TwitterPassword", "", SSD_ALIGN_VCENTER
         | SSD_ALIGN_RIGHT | tab_flag, SSD_TEXT_PASSWORD, width, SSD_MIN_SIZE,
         roadmap_lang_get("Password")));
   ssd_widget_add(box, group);

   ssd_widget_add(dialog, box);

#ifndef TOUCH_SCREEN      
   ssd_widget_set_left_softkey_text ( dialog, roadmap_lang_get("Ok"));
   ssd_widget_set_left_softkey_callback ( dialog, on_ok_softkey);
#endif

}

/////////////////////////////////////////////////////////////////////////////////////
void roadmap_twitter_setting_dialog(void) {
   const char *pVal;

   if (!ssd_dialog_activate(TWITTER_DIALOG_NAME, NULL)) {
      create_dialog();
      ssd_dialog_activate(TWITTER_DIALOG_NAME, NULL);
   }

   ssd_dialog_set_value("TwitterUserName", roadmap_twitter_get_username());
   ssd_dialog_set_value("TwitterPassword", roadmap_twitter_get_password());
   if (roadmap_twitter_is_sending_enabled())
      pVal = yesno[0];
   else
      pVal = yesno[1];
   ssd_dialog_set_data("TwitterSendTwitts", (void *) pVal);
}

