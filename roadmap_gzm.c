/* roadmap_gzm.c - Open and read a gzm compressed map file.
 *
 * LICENSE:
 *
 *   Copyright 2008 Israel Disatnik
 *   Copyright 2008 Ehud Shabtai
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
 */

#include <string.h>
#include <stdlib.h>

#include "zlib.h"

#include "roadmap_gzm.h"
#include "roadmap.h"
#include "roadmap_db_square.h"
#include "roadmap_path.h"
#include "roadmap_file.h"
#include "roadmap_path.h"

typedef struct gzm_file_s {

	char *name;
	RoadMapSquareIndex *index;
	int num_entries;
	int ref_count;
	
	RoadMapFile file;
	
} gzm_file;

#define MAX_GZM	3

static gzm_file GzmFile[MAX_GZM];
static int GzmMax = 0;

int roadmap_gzm_open (const char *name) {

	int id;
	const char *map_path;
	char *full_path;
	RoadMapSquareIndex first;
	
	/* look for already open file */
	for (id = 0; id < GzmMax; id++) {
		if (GzmFile[id].name && !strcmp (name, GzmFile[id].name)) {
			GzmFile[id].ref_count++;
			return id;
		}
	}
	
	/* find an empty slot */
	for (id = 0; id < GzmMax; id++) {
		if (!GzmFile[id].name) break;
	}
	if (id >= MAX_GZM) return -1;
	
	/* find the file in maps folders */
	map_path = roadmap_path_first ("maps");
	GzmFile[id].file = ROADMAP_INVALID_FILE;
	
	while (map_path && !ROADMAP_FILE_IS_VALID (GzmFile[id].file)) {
		
		full_path = roadmap_path_join (map_path, name);
		GzmFile[id].file = roadmap_file_open (full_path, "r");
		roadmap_path_free (full_path);

		map_path = roadmap_path_next ("maps", map_path);
	}
	
	if (!ROADMAP_FILE_IS_VALID (GzmFile[id].file)) {
		roadmap_log (ROADMAP_ERROR, "failed to open map file %s", name);
		return -1;
	}
	
	/* read first entry to calculate size */
	if ((!roadmap_file_read (GzmFile[id].file, &first, sizeof (first))) ||
		 (first.offset % sizeof (first))) {
		roadmap_log (ROADMAP_ERROR, "bad map file format of %s", name);
		roadmap_file_close (GzmFile[id].file);
		return -1;
	}
	
	GzmFile[id].num_entries = first.offset / sizeof (first);
	GzmFile[id].name = strdup (name);
	GzmFile[id].index = malloc (first.offset);
	GzmFile[id].index[0] = first;
	roadmap_file_read (GzmFile[id].file, GzmFile[id].index + 1, first.offset - sizeof (first));
	
	GzmFile[id].ref_count = 1;
	if (id >= GzmMax) GzmMax = id + 1;
	return id;
}


void roadmap_gzm_close (int gzm_id) {
	
	if (GzmFile[gzm_id].name) {
		if (--GzmFile[gzm_id].ref_count == 0) {
			if (ROADMAP_FILE_IS_VALID (GzmFile[gzm_id].file)) roadmap_file_close (GzmFile[gzm_id].file);
			if (GzmFile[gzm_id].index) free (GzmFile[gzm_id].index);
			if (GzmFile[gzm_id].name) free (GzmFile[gzm_id].name);
			GzmFile[gzm_id].name = NULL;
		}
	}
}


static RoadMapSquareIndex *roadmap_gzm_locate_entry (int gzm_id, const char *name) {

	int hi = GzmFile[gzm_id].num_entries - 1;
	int lo = 0;
	RoadMapSquareIndex *index = GzmFile[gzm_id].index;
	
	while (hi >= lo) {
	
		int mid = (hi + lo) / 2;
		int dir = strncmp (name, index[mid].name, 8);
		
		if (dir < 0) {
			hi = mid - 1;
		} else if (dir > 0) {
			lo = mid + 1;
		} else {
			return index + mid;
		}
	}
	
	return NULL;
}


int roadmap_gzm_get_section (int gzm_id, const char *name,
									  void **section, int *length) {
									  	
	RoadMapSquareIndex *entry = roadmap_gzm_locate_entry (gzm_id, name);
	void *compressed_buf;
	int success;
	unsigned long ul;
	
	if (!entry) {
		roadmap_log (ROADMAP_ERROR, "failed to find tile %s", name);
		return -1;
	}
	
	ul = *length = entry->raw_size;
	*section = malloc (*length);
	compressed_buf = malloc (entry->compressed_size);
	
	success = 
		roadmap_file_seek (GzmFile[gzm_id].file, entry->offset, ROADMAP_SEEK_START) > 0 &&
		roadmap_file_read (GzmFile[gzm_id].file, compressed_buf, entry->compressed_size) == entry->compressed_size &&
		uncompress (*section, &ul, compressed_buf, entry->compressed_size) == Z_OK;
		
	free (compressed_buf);
	if (!success) {
		roadmap_log (ROADMAP_ERROR, "failed to load tile %s", name);
		free (*section);
		return -1;
	} 
	
	return 0;
}


void roadmap_gzm_free_section (int gzm_id, void *section) {

	free (section);	
}

