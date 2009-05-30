/* editor_sync.c - synchronize data and maps
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
 *   See editor_sync.h
 */

#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>

#include "roadmap.h"
#include "roadmap_file.h"
#include "roadmap_path.h"
#include "roadmap_locator.h"
#include "roadmap_metadata.h"
#include "roadmap_lang.h"
#ifdef SSD
#include "ssd/ssd_dialog.h"
#include "ssd/ssd_text.h"
#include "ssd/ssd_button.h"
#include "ssd/ssd_container.h"
#else
#include "roadmap_dialog.h"
#endif
#include "roadmap_main.h"
#include "roadmap_messagebox.h"
#include "roadmap_label.h"

#include "editor_download.h"
#include "../editor_main.h"
#include "editor_sync.h"
#include "editor_upload.h"

#include "navigate/navigate_graph.h"
#include "Realtime/RealtimeOffline.h"

#define MAX_MSGS 10
#define MIN_FREE_SPACE 5000 //Free space in KB

static int SyncProgressItems;
static int SyncProgressCurrentItem;
static int SyncProgressTarget;
static int SyncProgressLoaded;

static int roadmap_download_request (int size) {

   SyncProgressTarget = size;
   SyncProgressLoaded = 0;
   SyncProgressCurrentItem++;
   return 1;
}


static void roadmap_download_error (const char *format, ...) {

   va_list ap;
   char message[2048];

   va_start(ap, format);
   vsnprintf (message, sizeof(message), format, ap);
   va_end(ap);

   roadmap_log (ROADMAP_ERROR, "Sync error: %s", message);
   roadmap_messagebox ("Download Error", message);
}


#ifdef SSD

static void roadmap_download_progress (int loaded) {

   const char *sync_icon[] = {"sync_icon"};

   SsdWidget dialog = ssd_dialog_activate ("Sync process", NULL);

   if (!dialog) {
      SsdWidget group;

      dialog = ssd_dialog_new ("Sync process", "Waze", NULL,
            SSD_CONTAINER_BORDER|SSD_CONTAINER_TITLE|SSD_DIALOG_FLOAT|
            SSD_ALIGN_CENTER|SSD_ALIGN_VCENTER|SSD_ROUNDED_CORNERS);

      ssd_widget_add  (dialog, 
         ssd_button_new ("sync", "", sync_icon, 1,
            SSD_ALIGN_RIGHT|SSD_END_ROW, NULL));

      group = ssd_container_new ("Status group", NULL,
                  SSD_MIN_SIZE, SSD_MIN_SIZE, SSD_WIDGET_SPACE|SSD_END_ROW);
      ssd_widget_set_color (group, NULL, NULL);
      ssd_widget_add (group,
         ssd_text_new ("Label", roadmap_lang_get("Progress status"), -1,
                        SSD_TEXT_LABEL));
      ssd_widget_add (group, ssd_text_new ("Progress status", "", -1, 0));
      ssd_widget_add (dialog, group);

      group = ssd_container_new ("Progress group", NULL,
                  SSD_MIN_SIZE, SSD_MIN_SIZE, SSD_WIDGET_SPACE|SSD_END_ROW);
      ssd_widget_set_color (group, NULL, NULL);
      ssd_widget_add (group,
         ssd_text_new ("Label", "%", -1, 0));
      ssd_widget_add (group, ssd_text_new ("Progress", "", -1, 0));
      ssd_widget_add (dialog, group);

      dialog = ssd_dialog_activate ("Sync process", NULL);

      ssd_dialog_draw ();
   }

   if ((SyncProgressLoaded > loaded) || !loaded || !SyncProgressItems) {
      ssd_widget_set_value (dialog, "Progress", "0");
      ssd_dialog_draw ();
      SyncProgressLoaded = loaded;
   } else {

      char progress_str[10];

      if (SyncProgressLoaded == loaded) {
         return;

      } else if ((loaded < SyncProgressTarget) &&
            (100 * (loaded - SyncProgressLoaded) / SyncProgressTarget) < 5) {
         return;
      }

      SyncProgressLoaded = loaded;

      snprintf (progress_str, sizeof(progress_str), "%d", 
         100 / SyncProgressItems * (SyncProgressCurrentItem - 1) +
         (100 / SyncProgressItems) * SyncProgressLoaded / SyncProgressTarget);
      ssd_widget_set_value (dialog, "Progress", progress_str);
      ssd_dialog_draw ();
   }

   roadmap_main_flush ();
}

#else

static void roadmap_download_progress (int loaded) {

   if (roadmap_dialog_activate ("Sync process", NULL, 1)) {

      const char *icon = roadmap_path_join (roadmap_path_user(), "skins/default/sync.bmp");
      roadmap_dialog_new_image  ("Sync", icon);
      roadmap_dialog_new_label  ("Sync", "Progress status");
      roadmap_dialog_new_progress  ("Sync", "Progress");
      roadmap_path_free (icon);

      roadmap_dialog_complete (0);
   }

   if ((SyncProgressLoaded > loaded) || !loaded || !SyncProgressItems) {
      roadmap_dialog_set_progress ("Sync", "Progress", 0);
      SyncProgressLoaded = loaded;
   } else {

      if (SyncProgressLoaded == loaded) {
         return;

      } else if ((loaded < SyncProgressTarget) &&
            (100 * (loaded - SyncProgressLoaded) / SyncProgressTarget) < 5) {
         return;
      }

      SyncProgressLoaded = loaded;

      roadmap_dialog_set_progress ("Sync", "Progress",
         100 / SyncProgressItems * (SyncProgressCurrentItem - 1) +
         (100 / SyncProgressItems) * SyncProgressLoaded / SyncProgressTarget);
   }

   roadmap_main_flush ();
}

#endif


static RoadMapDownloadCallbacks SyncDownloadCallbackFunctions = {
   roadmap_download_request,
   roadmap_download_progress,
   roadmap_download_error
};


const char *editor_sync_get_export_path (void) {

	static char path[1024];
	char *joined;
	static int initialized = 0; 
	
	if (!initialized) {
		joined = roadmap_path_join (roadmap_db_map_path (), "queue");
		strncpy_safe (path, joined, 1024);
		
	   roadmap_path_create (path);
	   initialized = 1;
	}
	
	return path;
}


const char *editor_sync_get_export_name (void) {
	
	static char name[255];
   struct tm *tm;
   time_t now;

   time(&now);
   tm = gmtime (&now);

   snprintf (name, sizeof(name), "rm_%02d_%02d_%02d_%02d%02d%02d.wud",
                     tm->tm_mday, tm->tm_mon+1, tm->tm_year-100,
                     tm->tm_hour, tm->tm_min, tm->tm_sec);

	return name;
}



static int sync_do_upload (char *messages[MAX_MSGS], int *num_msgs) {

   char **files;
   char **cursor;
   const char* directory = editor_sync_get_export_path();
   int count;

   files = roadmap_path_list (directory, ".wud");

   count = 0;
   for (cursor = files; *cursor != NULL; ++cursor) {
      count++;
   }

   SyncProgressItems = count;
   SyncProgressCurrentItem = 0;
   *num_msgs = 0;

   for (cursor = files; *cursor != NULL; ++cursor) {
      
      char *full_name = roadmap_path_join (directory, *cursor);
      int res = editor_upload_auto (full_name, &SyncDownloadCallbackFunctions,
                                    messages + *num_msgs);

      if (res == 0) {
         roadmap_file_remove (NULL, full_name);
         if (messages[*num_msgs] != NULL) (*num_msgs)++;
      }

      free (full_name);

      if (*num_msgs == MAX_MSGS) break;
   }

   roadmap_path_list_free (files);

   return 0;
}


int export_sync (void) {

   int i;
   int res;
   char *messages[MAX_MSGS];
   int num_msgs = 0;
   int fips;

   if (!editor_is_enabled ()) {
      return 0;
   }

   res = roadmap_file_free_space (roadmap_path_user());

#ifndef __SYMBIAN32__
   if ((res >= 0) && (res < MIN_FREE_SPACE)) {
      roadmap_messagebox ("Error",
                  "Please free at least 5MB of space before synchronizing.");
      return -1;
   }
#endif
   
   roadmap_download_progress (0);

#ifdef SSD
   ssd_dialog_set_value ("Progress status",
                            roadmap_lang_get ("Preparing export data..."));
   ssd_dialog_draw ();
#else

   roadmap_dialog_set_data ("Sync", "Progress status",
                            roadmap_lang_get ("Preparing export data..."));
#endif

   roadmap_main_flush ();
   roadmap_download_progress (0);

#ifdef SSD
   ssd_dialog_set_value ("Progress status",
                            roadmap_lang_get ("Uploading data..."));
   ssd_dialog_draw ();
#else
   roadmap_dialog_set_data ("Sync", "Progress status",
                            roadmap_lang_get ("Uploading data..."));
#endif

   roadmap_main_flush ();
   
   Realtime_OfflineClose ();
   res = sync_do_upload (messages, &num_msgs);
	Realtime_OfflineOpen (editor_sync_get_export_path (), 
								 editor_sync_get_export_name ());
   

   fips = roadmap_locator_active ();

   if (fips < 0) {
      fips = 77001;
   }

#if 0
   if (roadmap_locator_activate (fips) == ROADMAP_US_OK) {
      now_t = time (NULL);
      map_time_t = atoi(roadmap_metadata_get_attribute ("Version", "UnixTime"));

      if ((map_time_t + 3600*24) > now_t) {
         /* Map is less than 24 hours old.
          * A new version may still be available.
          */

         now_tm = *gmtime(&now_t);
         map_time_tm = *gmtime(&map_time_t);

         if (now_tm.tm_mday == map_time_tm.tm_mday) {

            goto end_sync;
         } else {
            /* new day - only download if new maps were already generated. */
            if (now_tm.tm_hour < 2) goto end_sync;
         }
      }
   }
#endif

   SyncProgressItems = 1;
   SyncProgressCurrentItem = 0;
   roadmap_download_progress (0);

#ifdef SSD
   ssd_dialog_set_value ("Progress status",
                            roadmap_lang_get ("Downloading new maps..."));
   ssd_dialog_draw ();
#else
   roadmap_dialog_set_data ("Sync", "Progress status",
                            roadmap_lang_get ("Downloading new maps..."));
#endif

	roadmap_label_clear ();
	navigate_graph_clear ();
	
   roadmap_main_flush ();
   res = editor_download_update_map (&SyncDownloadCallbackFunctions);

   if (res == -1) {
      roadmap_messagebox ("Download Error", roadmap_lang_get("Error downloading map update"));
   }

   for (i=0; i<num_msgs; i++) {
      roadmap_messagebox ("Info", messages[i]);
      free (messages[i]);
   }

#ifdef SSD
   ssd_dialog_hide ("Sync process", dec_close);
#else
   roadmap_dialog_hide ("Sync process");
#endif

   return 0;
}

