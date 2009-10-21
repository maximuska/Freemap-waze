/* roadmap_tile_storage.c - Tile persistency.
 *
 * LICENSE:
 *
 *   Copyright 2009 Ehud Shabtai.
 *
 *   This file is part of Waze.
 *
 *   Waze is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   Waze is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with Waze; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>

#include "roadmap_tile_storage.h"

#include "roadmap.h"
#include "roadmap_file.h"
#include "roadmap_path.h"
#include "roadmap_locator.h"


static const char * get_tile_filename (int fips, int tile_index, int create_path) {

   const char *map_path = roadmap_db_map_path ();
   char name[30];
   char path[512];
   static char filename[512];

   if (tile_index == -1) {
      /* Global square id */

      const char *suffix = "index";
      static char name[512];

      snprintf (name, sizeof (name), "%05d_%s%s", fips, suffix,
            ROADMAP_DATA_TYPE);
	   roadmap_path_format (filename, sizeof (filename), map_path, name);
      return filename;
   }

#ifdef J2ME
   snprintf (filename, sizeof (filename), "recordstore://map%05d.%d:1", fips, tile_index);
#else
	snprintf (path, sizeof (path), "%05d", fips);
	roadmap_path_format (path, sizeof (path), map_path, path);
	if (create_path) roadmap_path_create (path);
	snprintf (name, sizeof (name), "%02x", tile_index >> 24);
	roadmap_path_format (path, sizeof (path), path, name);
	if (create_path) roadmap_path_create (path);
	snprintf (name, sizeof (name), "%02x", (tile_index >> 16) & 255);
	roadmap_path_format (path, sizeof (path), path, name);
	if (create_path) roadmap_path_create (path);
	snprintf (name, sizeof (name), "%02x", (tile_index >> 8) & 255);
	roadmap_path_format (path, sizeof (path), path, name);
	if (create_path) roadmap_path_create (path);
	snprintf (name, sizeof (name), "%05d_%08x%s", fips, tile_index,
         ROADMAP_DATA_TYPE);
	roadmap_path_format (filename, sizeof (filename), path, name);
#endif

   return filename;
}

int roadmap_tile_store (int fips, int tile_index, void *data, size_t size) {

   int res = 0;

   RoadMapFile file = roadmap_file_open(get_tile_filename(fips, tile_index, 1), "w");

   if (ROADMAP_FILE_IS_VALID(file)) {
      res = (roadmap_file_write(file, data, size) != (int)size);

      roadmap_file_close(file);
   } else {
      res = -1;
      roadmap_log(ROADMAP_ERROR, "Can't save tile data for %d", tile_index);
   }

   return res;
}

void roadmap_tile_remove (int fips, int tile_index) {

   roadmap_file_remove(NULL, get_tile_filename(fips, tile_index, 0));
}

int roadmap_tile_load (int fips, int tile_index, void **base, size_t *size) {

   RoadMapFile		file;
   int				res;

   const char		*full_name = get_tile_filename(fips, tile_index, 0);

   file = roadmap_file_open (full_name, "r");

   if (!ROADMAP_FILE_IS_VALID(file)) {
      return -1;
   }

#ifdef J2ME
   *size = favail(file);
#else
   *size = roadmap_file_length (NULL, full_name);
#endif
   *base = malloc (*size);

   res = roadmap_file_read (file, *base, *size);
   roadmap_file_close (file);

   if (res != (int)*size) {
      free (*base);
      return -1;
   }

   return 0;
}


