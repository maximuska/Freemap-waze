/* roadmap_dbread.c - a module to read a roadmap database.
 *
 * LICENSE:
 *
 *   Copyright 2002 Pascal F. Martin
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
 *
 * SYNOPSYS:
 *
 *   #include "roadmap_dbread.h"
 *
 *   roadmap_db_model *roadmap_db_register
 *          (char *section, roadmap_db_handler *handler);
 *
 *   int  roadmap_db_open (char *name, roadmap_db_model *model);
 *
 *   void roadmap_db_activate (char *name);
 *
 *   roadmap_db *roadmap_db_get_subsection (roadmap_db *parent, char *path);
 *
 *   roadmap_db *roadmap_db_get_first (roadmap_db *parent);
 *   char       *roadmap_db_get_name  (roadmap_db *section);
 *   unsigned    roadmap_db_get_size  (roadmap_db *section);
 *   int         roadmap_db_get_count (roadmap_db *section);
 *   char       *roadmap_db_get_data  (roadmap_db *section);
 *   roadmap_db *roadmap_db_get_next  (roadmap_db *section);
 *
 *   void roadmap_db_close (char *name);
 *   void roadmap_db_end   (void);
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "roadmap_file.h"

#include "roadmap.h"
#include "roadmap_path.h"
#include "roadmap_dbread.h"
#include "roadmap_gzm.h"


typedef struct roadmap_db_context_s {

   roadmap_db_model *model;
   void                 *handler_context;

   struct roadmap_db_context_s *next;
} roadmap_db_context;


typedef struct roadmap_db_database_s {

   char *name;
   char *section;

   RoadMapFileContext file;
   int gzm_id;
   roadmap_db_data_file data;

   struct roadmap_db_database_s *next;
   struct roadmap_db_database_s *previous;

   roadmap_db_model         *model;
   roadmap_db_context   *context;

} roadmap_db_database;

static roadmap_db_database *RoadmapDatabaseFirst  = NULL;


static unsigned int roadmap_db_aligned_offset (const roadmap_db_data_file *data, unsigned int unaligned_offset) {

    return (unaligned_offset + data->byte_alignment_add) & data->byte_alignment_mask;
}

static unsigned int roadmap_db_entry_offset (const roadmap_db_data_file *data, unsigned int entry_id) {

    if (entry_id > 0)
        return roadmap_db_aligned_offset (data, data->index[entry_id - 1].end_offset);
    else
        return 0;
}

static unsigned int roadmap_db_size (const roadmap_db_data_file *data, unsigned int from, unsigned int to) {

    return data->index[to].end_offset - roadmap_db_entry_offset (data, from);
}

static unsigned int roadmap_db_sector_size (const roadmap_db_data_file *data, const roadmap_db_sector *sector) {

    return roadmap_db_size (data, sector->first, sector->last);
}

static unsigned int roadmap_db_entry_size (const roadmap_db_data_file *data, unsigned int entry_id) {

    return roadmap_db_size (data, entry_id, entry_id);
}


static int roadmap_db_call_map (roadmap_db_database *database) {

   roadmap_db_model *model;
   int res = 1;

   for (model = database->model;
      model != NULL;
      model = model->next) {

        void *handler_context = NULL;
        roadmap_db_context *context;

    if (roadmap_db_sector_size (&(database->data), &model->sector)) {

        handler_context = model->handler->map (&database->data);
        if (!handler_context) res = 0;
    }

        context = (roadmap_db_context *) malloc (sizeof (roadmap_db_context));
        roadmap_check_allocated (context);

        context->model = model;
        context->next = database->context;
        context->handler_context = handler_context;

                database->context = context;
   }

   return res;
}


static void roadmap_db_call_activate (const roadmap_db_database *database) {

   roadmap_db_context *context;


   /* Activate each module declared in the model.
    * Modules with no context (context->handler_context is NULL) will be deactivated
    */
   for (context = database->context;
        context != NULL;
        context = context->next) {

        if (context->model &&
             context->model->handler &&
             context->model->handler->activate) {

            context->model->handler->activate (context->handler_context);
        }
   }
}


static void roadmap_db_call_unmap (roadmap_db_database *database) {

   roadmap_db_context *context;
   roadmap_db_context *next;


   /* Activate each module declared in the model.
    * Modules with no context (context->handler_context is NULL) will be deactivated
    */
   for (context = database->context;
        context != NULL;
        context = next) {

        next = context->next;

        if (context->model &&
             context->model->handler &&
             context->model->handler->unmap &&
             context->handler_context) {

            context->model->handler->unmap (context->handler_context);
        }
        free (context);
   }

   database->context = NULL;
}


static void roadmap_db_close_database (roadmap_db_database *database) {

   roadmap_db_call_unmap (database);

    if (database->gzm_id >= 0) {
        roadmap_gzm_free_section (database->gzm_id, database->data.header);
        roadmap_gzm_close (database->gzm_id);
    }

   if (database->file != NULL) {
      roadmap_file_unmap (&database->file);
   }

   if (database->next != NULL) {
      database->next->previous = database->previous;
   }
   if (database->previous == NULL) {
      RoadmapDatabaseFirst = database->next;
   } else {
      database->previous->next = database->next;
   }

    if (database->section) free (database->section);
   free(database->name);
   free(database);
}


roadmap_db_model *roadmap_db_register
                      (roadmap_db_model *model,
                       const roadmap_db_sector *sector,
                       roadmap_db_handler *handler) {

   roadmap_db_model *registered;

   registered = malloc (sizeof(roadmap_db_model));

   roadmap_check_allocated(registered);

   registered->sector  = *sector;
   registered->handler = handler;
   registered->next    = model;

   return registered;
}




roadmap_db_database *roadmap_db_find (const char *name, const char *section) {

   roadmap_db_database *database;

   for (database = RoadmapDatabaseFirst;
        database != NULL;
        database = database->next) {

      if (strcmp (name, database->name) == 0) {
        if ((section && database->section && strcmp (section, database->section) == 0) ||
             (section == NULL && database->section == NULL)) {
            break;
        }
      }
   }

   return database;
}


static int roadmap_db_fill_data (roadmap_db_database *database, void *base, unsigned int size) {

    if (size < sizeof (roadmap_data_header)) {
       roadmap_log (ROADMAP_ERROR, "data file open: header size %u too small", size);
       return 0;
    }


    database->data.header = (roadmap_data_header *)base;

    if (memcmp (database->data.header->signature, ROADMAP_DATA_SIGNATURE, sizeof (database->data.header->signature))) {
       roadmap_log (ROADMAP_ERROR, "data file open: invalid signature %c%c%c%c",
                        database->data.header->signature[0],
                        database->data.header->signature[1],
                        database->data.header->signature[2],
                        database->data.header->signature[3]);
       return 0;
    }

    if (database->data.header->endianness != ROADMAP_DATA_ENDIAN_CORRECT) {
       roadmap_log (ROADMAP_ERROR, "data file open: invalid endianness value %08ux", database->data.header->endianness);
       return 0;
    }
    if (database->data.header->version != ROADMAP_DATA_CURRENT_VERSION) {
       roadmap_log (ROADMAP_ERROR, "data file open: invalid version %08ux", database->data.header->version);
       return 0;
    }

    database->data.byte_alignment_add = (1 << database->data.header->byte_alignment_bits) - 1;
    database->data.byte_alignment_mask = ~database->data.byte_alignment_add;

    if (size < sizeof (roadmap_data_header) +
                  database->data.header->num_sections * sizeof (roadmap_data_entry)) {
       roadmap_log (ROADMAP_ERROR, "data file open: size %u cannot contain index", size);
       return 0;
    }
    database->data.index = (roadmap_data_entry *)(database->data.header + 1);

    if (database->data.header->num_sections > 0 &&
         size < sizeof (roadmap_data_header) +
                  database->data.header->num_sections * sizeof (roadmap_data_entry) +
                  database->data.index[database->data.header->num_sections - 1].end_offset) {

       roadmap_log (ROADMAP_ERROR, "data file open: size %u cannot contain data", size);
       return 0;
    }
    database->data.data = (unsigned char *)(database->data.index + database->data.header->num_sections);

    return 1;
}


int roadmap_db_open (const char *name, const char *section, roadmap_db_model *model, const char* mode) {

   char *full_name;
   const char *type;
   RoadMapFileContext file;
   void *base = NULL;
   int size = 0;

   roadmap_db_database *database = roadmap_db_find (name, section);


   if (database) {

      roadmap_db_call_activate (database);
      return 1; /* Already open. */
   }

    if (!section) {
        type = ROADMAP_DATA_TYPE;
    } else {
        type = ROADMAP_GZM_TYPE;
    }
   full_name = malloc (strlen(name) + strlen(type) + 4);
   roadmap_check_allocated(full_name);

   strcpy (full_name, name);
   strcat (full_name, type);

    if (!section) {
       if (roadmap_file_map ("maps", full_name, NULL, mode, &file) == NULL) {

          roadmap_log (ROADMAP_INFO, "cannot open database file %s", full_name);
          free (full_name);
          return 0;
       }

       roadmap_log (ROADMAP_INFO, "Opening database file %s", full_name);
       database = malloc(sizeof(*database));
       roadmap_check_allocated(database);

       database->file = file;
       database->name = strdup(name);
       base = roadmap_file_base (file);
       size = roadmap_file_size (file);
       database->gzm_id = -1;
       database->section = NULL;
    } else {
       database = malloc(sizeof(*database));
        database->gzm_id = roadmap_gzm_open (full_name);
        if (database->gzm_id == -1) {

          roadmap_log (ROADMAP_INFO, "cannot open map file %s", full_name);
          free (full_name);
          free (database);
          return 0;
        }
        if (roadmap_gzm_get_section (database->gzm_id, section, &base, &size) == -1) {

          roadmap_log (ROADMAP_INFO, "cannot find section %s in map file %s", section, name);
          free (full_name);
          free (database);
          return 0;
        }

        database->name = strdup (name);
        database->section = strdup (section);
        database->file = NULL;
    }

    if (!roadmap_db_fill_data (database, base, (unsigned int) size)) {

       roadmap_log (ROADMAP_INFO, "file %s has invalid format", full_name);
      free (full_name);
      free (database);
      return 0;
    }

   free (full_name);

   if (RoadmapDatabaseFirst != NULL) {
      RoadmapDatabaseFirst->previous = database;
   }
   database->next       = RoadmapDatabaseFirst;
   database->previous   = NULL;
   RoadmapDatabaseFirst = database;


   database->model = model;
   database->context = NULL;

   if (! roadmap_db_call_map  (database)) {
      roadmap_db_close_database (database);
      return 0;
   }

   roadmap_db_call_activate (database);

   return 1;
}


void roadmap_db_activate (const char *name, const char *section) {

   roadmap_db_database *database = roadmap_db_find (name, section);

   if (database) {

      roadmap_log (ROADMAP_DEBUG, "Activating database %s", name);

      roadmap_db_call_activate (database);

      return;
   }

   roadmap_log
      (ROADMAP_ERROR, "cannot activate database %s (not found)", name);
}


int roadmap_db_exists (const roadmap_db_data_file *file, const roadmap_db_sector *sector) {

    return roadmap_db_sector_size (file, sector) > 0;
}


int roadmap_db_get_data (const roadmap_db_data_file *file,
                                    unsigned int data_id,
                                    unsigned int item_size,
                                    void             **data,
                                    int              *num_items) {

    unsigned int offset = roadmap_db_entry_offset (file, data_id);
    unsigned int size = roadmap_db_entry_size (file, data_id);

    assert (item_size > 0);

    if (size % item_size) {
      roadmap_log (ROADMAP_WARNING, "Invalid data size - item size %u data size %u", item_size, size);
      return 0;
    }

    if (data) {
        if (size)   *data = file->data + offset;
        else            *data = NULL;
    }
    if (num_items) *num_items = size / item_size;
    return 1;
}


void roadmap_db_sync (char *name) {

   roadmap_db_database *database = roadmap_db_find (name, NULL);

   if (database) roadmap_file_sync (database->file);
}


void roadmap_db_close (const char *name, const char *section) {

   roadmap_db_database *database = roadmap_db_find (name, section);

   if (database) roadmap_db_close_database (database);
}


void roadmap_db_end (void) {

   roadmap_db_database *database;
   roadmap_db_database *next;

   for (database = RoadmapDatabaseFirst; database != NULL; ) {

      next = database->next;
      roadmap_db_close_database (database);
      database = next;
   }
}

const char *roadmap_db_map_path (void) {

   const char *map_path;
#ifdef WIN32
    map_path = roadmap_path_join (roadmap_path_user(), "maps");
#else
   map_path = roadmap_path_first ("maps");
   while (map_path && !roadmap_file_exists (map_path,"")) {
    map_path = roadmap_path_next ("maps", map_path);
   }
#endif

   return map_path;
}

