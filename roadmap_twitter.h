/* roadmap_twitter.h - Manages Twitter accont
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
 
 
#define  TWITTER_USER_NAME_MAXSIZE          (128)
#define  TWITTER_USER_PASSWORD_MAXSIZE      (128)

#define  TWITTER_USER_NAME_MIN_SIZE     (2)
#define  TWITTER_PASSWORD_MIN_SIZE     (2)

#define  TWITTER_CONFIG_TYPE			    ("user")
#define  TWITTER_CONFIG_TAB             ("Twitter")

//   Twitter User name
#define  TWITTER_CFG_PRM_NAME_Name          ("Name")
#define  TWITTER_CFG_PRM_NAME_Default       ("")

//   Twitter User password
#define  TWITTER_CFG_PRM_PASSWORD_Name      ("Password")
#define  TWITTER_CFG_PRM_PASSWORD_Default   ("")

//  Send to Twitter Enable / Disable:
#define  RT_CFG_PRM_SEND_TWITTS_Name       	("Send Twitts")
#define  RT_CFG_PRM_SEND_TWITTS_Enabled     ("Enabled")
#define  RT_CFG_PRM_SEND_TWITTS_Disabled    ("Disabled")


#define  TWITTER_DIALOG_NAME                ("twitter_settings")
#define  TWITTER_TITTLE						("Twitter")

BOOL roadmap_twitter_initialize(void);

BOOL roadmap_twitter_is_sending_enabled(void);
void roadmap_twitter_enable_sending(void);
void roadmap_twitter_diable_sending(void);

const char *roadmap_twitter_get_username(void);
const char *roadmap_twitter_get_password(void);

void roadmap_twitter_set_username(const char *user_name);
void roadmap_twitter_set_password(const char *password);

void roadmap_twitter_setting_dialog(void);

