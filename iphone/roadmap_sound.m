/* roadmap_sound.m - Play sound.
 *
 * LICENSE:
 *
 *   Copyright 2008 Avi R.
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
 *   See roadmap_sound.h
 */

#include <stdlib.h>
#include <string.h>
#include "roadmap.h"
#include "roadmap_sound.h"
#include "roadmap_file.h"
#include "roadmap_path.h"
#include "roadmap_res.h"
#include "roadmap_main.h"

#include <UIKit/UIKit.h>
#include <AudioToolbox/AudioServices.h>

const char* SND_DEFAULT_VOLUME_LVL = "2";

static int              current_list  = -1;
static int              current_index = -1;

#define MAX_LISTS 2

static RoadMapSoundList sound_lists[MAX_LISTS];


RoadMapSoundList roadmap_sound_list_create (int flags) {

   RoadMapSoundList list =
            (RoadMapSoundList) calloc (1, sizeof(struct roadmap_sound_list_t));
   list->flags = flags;
   
   return list;
}


int roadmap_sound_list_add (RoadMapSoundList list, const char *name) {

   if (list->count == MAX_SOUND_LIST) return -1;

   strncpy (list->list[list->count], name, sizeof(list->list[0]));
   list->list[list->count][sizeof(list->list[0])-1] = '\0';
   list->count++;

   return list->count - 1;
}


int roadmap_sound_list_count (const RoadMapSoundList list) {

   return list->count;
}


const char *roadmap_sound_list_get (const RoadMapSoundList list, int i) {

   if (i >= MAX_SOUND_LIST) return NULL;

   return list->list[i];
}


void roadmap_sound_list_free (RoadMapSoundList list) {

   free(list);
}


RoadMapSound roadmap_sound_load (const char *path, const char *file, int *mem) {

   return 0;
}


int roadmap_sound_free (RoadMapSound sound) {

   return 0;
}


int roadmap_sound_play      (RoadMapSound sound) {
/*
   int res;
   void *mem;

   if (!sound) return -1;
   mem = roadmap_file_base ((RoadMapFileContext)sound);

   //TODO: res = PlaySound((LPWSTR)mem, NULL, SND_SYNC | SND_MEMORY);

   if (res) return 0;
   else return -1;*/
   
   return -1;
}


static void play_next_file (void) {
                           
      RoadMapSoundList list;

      if (current_list == -1 || current_index == -1) { // move to next list
         current_list = (current_list + 1) % MAX_LISTS;
         current_index = 0;
         
         if (sound_lists[current_list] == NULL) {
            /* nothing to play */
            current_list = -1;
         }

         if (current_list == -1) {
            return;
         }
      } else { //same list, next index
         current_index++;
      }

      list = sound_lists[current_list];

      const char *name = roadmap_sound_list_get (list, current_index);
      RoadMapSound sound =
                     roadmap_res_get (RES_SOUND, RES_NOCREATE, name);
                     
      if (current_index == roadmap_sound_list_count (list) - 1) {
         if (!(list->flags & SOUND_LIST_NO_FREE)) {
            roadmap_sound_list_free (list);
         }
         sound_lists[current_list] = NULL;
         current_index = -1;
      }
      
      if (!roadmap_main_should_mute()) {
         if (sound) {
            roadmap_sound_play (sound);
         } else {
            roadmap_sound_play_file (name);
         }
      } else { //skip this file, as mute is ON
         play_next_file ();
      }
}

void sound_complete (SystemSoundID  ssID, 
                     void*          clientData){

   AudioServicesDisposeSystemSoundID (ssID);
   play_next_file();
}


int play_file (const char *file_name) {

   CFURLRef fileURL;
   fileURL = CFURLCreateFromFileSystemRepresentation(NULL, (UInt8*)file_name, strlen(file_name), false);
   
   if (!fileURL) {
      roadmap_log (ROADMAP_WARNING, "File not found: %s", file_name);
      play_next_file ();
      return 0;
   }
   

   SystemSoundID soundID;
   OSStatus status;
   status = AudioServicesCreateSystemSoundID (fileURL, &soundID);
   
   if (status == 0) {
      status = AudioServicesAddSystemSoundCompletion (soundID, NULL, NULL, sound_complete, NULL);
   }
   
   if (status == 0) {
      AudioServicesPlaySystemSound (soundID);
   }
   
   if(status != 0)
   {
     roadmap_log (ROADMAP_WARNING, "Play failed ; Code: %ld\n", status);
     play_next_file ();
     return 0;
   }
   
   return 1;
}



int roadmap_sound_play_file (const char *file_name) {

   char full_name[256];
   //LPWSTR file_name_unicode;
   int res;
   
   //check if we have .wav extention
   char *result;
   char *p;

   result = strdup(file_name);
   roadmap_check_allocated(result);
   p = roadmap_path_skip_directories (result); // TODO: clean this code
   p = strrchr (p, '.');
   if (p == NULL) {
      strcat (result,".wav");
   }
   
   file_name = result;

   if (roadmap_path_is_full_path (file_name)) {
      //file_name_unicode = ConvertToWideChar(file_name, CP_UTF8); 
      snprintf (full_name, sizeof(full_name), "%s", file_name); //TODO: fix this ; add support for unicode
   } else {

      snprintf (full_name, sizeof(full_name), "%s/sound/%s",
                roadmap_path_user (), file_name);

      //file_name_unicode = ConvertToWideChar(full_name, CP_UTF8);
   }

   res = play_file (full_name);

   //free(file_name_unicode);

   if (res) return 0;
   else return -1;
}




int roadmap_sound_play_list (const RoadMapSoundList list) {
   
   if (current_list == -1) {
      /* not playing */
      
      sound_lists[0] = list;
      play_next_file ();

   } else {

      int next = (current_list + 1) % MAX_LISTS;

      if (sound_lists[next] != NULL) {
		  if (!(sound_lists[next]->flags & SOUND_LIST_NO_FREE)) {
			  roadmap_sound_list_free (sound_lists[next]);
		  }
      }

      sound_lists[next] = list;
   }

   return 0;
}


int roadmap_sound_record (const char *file_name, int seconds) {

   return 0;
}



void roadmap_sound_initialize (void) {
   
   AudioSessionInitialize (NULL, NULL, NULL, NULL);
   UInt32 sessionCategory = kAudioSessionCategory_AmbientSound;
   AudioSessionSetProperty (kAudioSessionProperty_AudioCategory, sizeof (sessionCategory), &sessionCategory);
   AudioSessionSetActive (true);
}


void roadmap_sound_shutdown   (void) {
}

