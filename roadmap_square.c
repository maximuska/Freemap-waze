/* roadmap_square.c - Manage a county area, divided in small squares.
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
 *   See roadmap_square.h.
 *
 * These functions are used to retrieve the squares that make the county
 * area. A special square (ROADMAP_SQUARE_GLOBAL) is used to describe the
 * global county area (vs. a piece of it).
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "roadmap.h"
#include "roadmap_math.h"
#include "roadmap_dbread.h"
#include "roadmap_tile_model.h"
#include "roadmap_county_model.h"
#include "roadmap_db_square.h"
#include "roadmap_point.h"
#include "roadmap_shape.h"
#include "roadmap_line.h"
#include "roadmap_line_route.h"
#include "roadmap_street.h"
#include "roadmap_polygon.h"
#include "roadmap_line_speed.h"
#include "roadmap_dictionary.h"
#include "roadmap_city.h"
#include "roadmap_range.h"
#include "roadmap_locator.h"
#include "roadmap_path.h"
#include "roadmap_alert.h"
#include "roadmap_metadata.h"

#include "roadmap_square.h"

static char *RoadMapSquareType = "RoadMapSquareContext";

typedef struct {
   roadmap_db_sector    sector;
   roadmap_db_handler   *handler;
} RoadMapSquareSubHandler;

static RoadMapSquareSubHandler SquareHandlers[] = {
   { {model__tile_string_first,model__tile_string_last}, &RoadMapDictionaryHandler },
   { {model__tile_shape_first,model__tile_shape_last}, &RoadMapShapeHandler },
   { {model__tile_line_first,model__tile_line_last}, &RoadMapLineHandler },
   { {model__tile_point_first,model__tile_point_last}, &RoadMapPointHandler },
   { {model__tile_line_route_first,model__tile_line_route_last}, &RoadMapLineRouteHandler },
   { {model__tile_street_first,model__tile_street_last}, &RoadMapStreetHandler },
   { {model__tile_polygon_first,model__tile_polygon_last}, &RoadMapPolygonHandler },
   { {model__tile_line_speed_first,model__tile_line_speed_last}, &RoadMapLineSpeedHandler },
   { {model__tile_range_first,model__tile_range_last}, &RoadMapRangeHandler },
   { {model__tile_alert_first,model__tile_alert_last}, &RoadMapAlertHandler },
	{ {model__tile_metadata_first, model__tile_metadata_last}, &RoadMapMetadataHandler }
};

#define NUM_SUB_HANDLERS ((int) (sizeof (SquareHandlers) / sizeof (SquareHandlers[0])))


typedef struct {
   RoadMapSquare        *square;
   void                 *subs[NUM_SUB_HANDLERS];
   int						cache_slot;
} RoadMapSquareData;


#define ROADMAP_SQUARE_CACHE_SIZE	512

#define ROADMAP_SQUARE_UNAVAILABLE	((RoadMapSquareData *)-1)
#define ROADMAP_SQUARE_NOT_LOADED	NULL

typedef struct {
	int	square;
	int	next;
	int	prev;
} SquareCacheNode;

typedef struct {

   char *type;

   RoadMapGrid     	*SquareGrid;
   RoadMapGlobal     *SquareGlobal;
   RoadMapScale		*SquareScale;
   RoadMapSquareData **Square;

	int					*SquareBaseIndex;
	int					SquareRange;
	SquareCacheNode	SquareCache[ROADMAP_SQUARE_CACHE_SIZE + 1];
} RoadMapSquareContext;


static RoadMapSquareContext *RoadMapSquareActive = NULL;

static int RoadMapScaleCurrent = 0;
static int RoadMapSquareCurrent = -1;

static void roadmap_square_unload_all (void);

static void *roadmap_square_map (const roadmap_db_data_file *file) {

   RoadMapSquareContext *context;

   int i;
   int j;
   int k;
   int count;
   int size;
   char *grid_data;
   int square_range;

	roadmap_city_init ();
	
   context = malloc(sizeof(RoadMapSquareContext));
   roadmap_check_allocated(context);

	RoadMapSquareActive = context;
	
   context->type = RoadMapSquareType;

   if (!roadmap_db_get_data (file,
   								  model__county_global_data,
   								  sizeof (RoadMapGlobal),
   								  (void**)&(context->SquareGlobal),
   								  NULL)) {
      roadmap_log (ROADMAP_FATAL, "invalid global/data structure");
   }

   if (!roadmap_db_get_data (file,
   								  model__county_global_grid,
   								  sizeof (RoadMapGrid),
   								  (void**)&(context->SquareGrid),
   								  NULL)) {
      roadmap_log (ROADMAP_FATAL, "invalid global/grid structure");
   }

   if (!roadmap_db_get_data (file,
   								  model__county_global_scale,
   								  sizeof (RoadMapScale),
   								  (void**)&(context->SquareScale),
   								  NULL)) {
      roadmap_log (ROADMAP_FATAL, "invalid global/scale structure");
   }

	context->SquareBaseIndex = (int *) malloc (context->SquareGrid->num_scales * sizeof (int));
   roadmap_check_allocated(context->SquareBaseIndex);
	
	square_range = 0;
	for (i = 0; i < context->SquareGrid->num_scales; i++) {
		
		context->SquareBaseIndex[i] = square_range;
		
	   square_range += context->SquareScale[i].count_longitude
	              			* context->SquareScale[i].count_latitude;
	}
	context->SquareRange = square_range;              
	
   context->Square = calloc (square_range, sizeof (RoadMapSquareData *));
   roadmap_check_allocated(context->Square);

   if (!roadmap_db_get_data (file,
   								  model__county_global_bitmask,
   								  sizeof (char),
   								  (void**)&(grid_data),
   								  NULL)) {
      roadmap_log (ROADMAP_FATAL, "invalid global/grid structure");
   }

	for (i = 0, k = 0; i < context->SquareGrid->num_scales; i++) {
		
	   count = context->SquareScale[i].count_longitude
	              * context->SquareScale[i].count_latitude;
	              
	   size = (count + 7) / 8;
	
   	for (j = 0; j < count; j++, k++) {

			if (grid_data[j / 8] & (1 << (j % 8))) {   			
   			context->Square[k] = ROADMAP_SQUARE_NOT_LOADED;
			} else {
				context->Square[k] = ROADMAP_SQUARE_UNAVAILABLE;
			}
   	}
   	
   	grid_data += size;
	}
	
	for (i = 0; i <= ROADMAP_SQUARE_CACHE_SIZE; i++) {
		context->SquareCache[i].square = -1;
		context->SquareCache[i].next = (i + 1) % (ROADMAP_SQUARE_CACHE_SIZE + 1);
		context->SquareCache[i].prev = (i + ROADMAP_SQUARE_CACHE_SIZE) % (ROADMAP_SQUARE_CACHE_SIZE + 1);
	}

   RoadMapSquareCurrent = -1;
	
	if (RoadMapScaleCurrent >= RoadMapSquareActive->SquareGrid->num_scales) {
		RoadMapScaleCurrent = RoadMapSquareActive->SquareGrid->num_scales - 1;
	}
   
   RoadMapSquareActive = NULL;
   return context;
}

static void roadmap_square_activate (void *context) {

   RoadMapSquareContext *square_context = (RoadMapSquareContext *) context;

   if ((square_context != NULL) &&
       (square_context->type != RoadMapSquareType)) {
      roadmap_log(ROADMAP_FATAL, "cannot unmap (bad context type)");
   }
   RoadMapSquareActive = square_context;
   RoadMapSquareCurrent = -1;
	if (RoadMapScaleCurrent >= RoadMapSquareActive->SquareGrid->num_scales) {
		RoadMapScaleCurrent = RoadMapSquareActive->SquareGrid->num_scales - 1;
	}
}

static void roadmap_square_unmap (void *context) {

   RoadMapSquareContext *square_context = (RoadMapSquareContext *) context;

   if (square_context->type != RoadMapSquareType) {
      roadmap_log(ROADMAP_FATAL, "cannot unmap (bad context type)");
   }

   roadmap_city_write_file (roadmap_db_map_path(), "city_index", 0);
   roadmap_city_free ();
   
   roadmap_square_unload_all ();
   
   if (RoadMapSquareActive == square_context) {
      RoadMapSquareActive = NULL;
   }

   free (square_context->SquareBaseIndex);
   free (square_context->Square);
   free (square_context);
}

roadmap_db_handler RoadMapSquareHandler = {
   "global",
   roadmap_square_map,
   roadmap_square_activate,
   roadmap_square_unmap
};

time_t	roadmap_square_global_timestamp (void) {

	if (RoadMapSquareActive == NULL) return 0;
	
	return RoadMapSquareActive->SquareGlobal->timestamp;	
}


#if 0 // currently not used
static int roadmap_square_distance_score (int square1, int square2) {
	
	RoadMapSquare *s1 = RoadMapSquareActive->Square[square1]->square;
	RoadMapSquare *s2 = RoadMapSquareActive->Square[square2]->square;
	int coeff1 = 256 / RoadMapSquareActive->SquareScale[s1->scale_index].scale_factor;
	int coeff2 = 256 / RoadMapSquareActive->SquareScale[s2->scale_index].scale_factor;
	
	return (s1->lon_index * coeff1 - s2->lon_index * coeff2) * (s1->lon_index * coeff1 - s2->lon_index * coeff2) + 
			 (s1->lat_index * coeff1 - s2->lat_index * coeff2) * (s1->lat_index * coeff1 - s2->lat_index * coeff2) +
			 (coeff1 - coeff2) * (coeff1 - coeff2); 
}
#endif

static void roadmap_square_unload (int square) {
	
	if (RoadMapSquareActive->Square[square]) {
		
		RoadMapSquare *s = RoadMapSquareActive->Square[square]->square;
		roadmap_locator_unload_tile (s->scale_index, s->lon_index, s->lat_index);
	
	}
}


static void roadmap_square_promote (int square) {

	int slot = RoadMapSquareActive->Square[square]->cache_slot;
	SquareCacheNode *cache = RoadMapSquareActive->SquareCache;
	
	if (cache[0].next != slot) {
		cache[cache[slot].next].prev = cache[slot].prev;
		cache[cache[slot].prev].next = cache[slot].next;
		
		cache[slot].next = cache[0].next;
		cache[slot].prev = 0;
		
		cache[cache[0].next].prev = slot;
		cache[0].next = slot;
	}	
}


static void roadmap_square_unload_all (void) {

	int i;
	
	for (i = 1; i <= ROADMAP_SQUARE_CACHE_SIZE; i++) {
	
		if (RoadMapSquareActive->SquareCache[i].square >= 0) {
			roadmap_square_unload (RoadMapSquareActive->SquareCache[i].square);
			RoadMapSquareActive->SquareCache[i].square = -1;
		}		
		RoadMapSquareActive->SquareCache[i].next = (i + 1) % (ROADMAP_SQUARE_CACHE_SIZE + 1);
		RoadMapSquareActive->SquareCache[i].prev = (i + ROADMAP_SQUARE_CACHE_SIZE) % (ROADMAP_SQUARE_CACHE_SIZE + 1);
	}
}


static void roadmap_square_cache (int square) {

	int slot = RoadMapSquareActive->SquareCache[0].prev;
	SquareCacheNode *node = RoadMapSquareActive->SquareCache + slot;
		
	if (node->square >= 0) {
		roadmap_square_unload (node->square);
	}
	node->square = square;
	RoadMapSquareActive->Square[square]->cache_slot = slot;
}


static int roadmap_square_index (RoadMapSquare *square) {
	
   int lon = square->lon_index;
   int lat = square->lat_index;
   int scale = square->scale_index;

	return RoadMapSquareActive->SquareBaseIndex[scale] +
				lon * RoadMapSquareActive->SquareScale[scale].count_latitude + lat;
}


//static int TotalSquares = 0;
static void *roadmap_square_map_one (const roadmap_db_data_file *file) {

   RoadMapSquareData *context;

   int j;
   int index;

   context = malloc(sizeof(RoadMapSquareData));
   roadmap_check_allocated(context);

   if (!roadmap_db_get_data (file,
   								  model__tile_square_data,
   								  sizeof (RoadMapSquare),
   								  (void**)&(context->square),
   								  NULL)) {
      roadmap_log (ROADMAP_FATAL, "invalid square/data structure");
   }

	index = roadmap_square_index (context->square); 
	RoadMapSquareActive->Square[index] = context;
		
	//RoadMapScaleCurrent = context->square->scale_index;
	RoadMapSquareCurrent = index;

   for (j = 0; j < NUM_SUB_HANDLERS; j++) {
   	if (roadmap_db_exists (file, &(SquareHandlers[j].sector))) {

         context->subs[j] = SquareHandlers[j].handler->map(file);
			SquareHandlers[j].handler->activate (context->subs[j]);
      } else {
         context->subs[j] = NULL;
      }
   }
   
   roadmap_square_cache (index);
   //printf ("Loaded square %d, total squares = %d\n", index, ++TotalSquares);
   return context;
}

static void roadmap_square_activate_one (void *context) {

}

static void roadmap_square_unmap_one (void *context) {

   RoadMapSquareData *square_data = (RoadMapSquareData *) context;
   int j;
   int index;

//   if (square_context->type != RoadMapSquareType) {
//      roadmap_log(ROADMAP_FATAL, "cannot unmap (bad context type)");
//   }

   for (j = 0; j < NUM_SUB_HANDLERS; j++) {
      if (square_data->subs[j]) {
         SquareHandlers[j].handler->unmap (square_data->subs[j]);
      }
   }

	index = roadmap_square_index (square_data->square); 

	RoadMapSquareActive->Square[index] = ROADMAP_SQUARE_NOT_LOADED;

   //printf ("Unloaded square %d, total squares = %d\n", index, --TotalSquares);
   free(square_data);
}

roadmap_db_handler RoadMapSquareOneHandler = {
   "square",
   roadmap_square_map_one,
   roadmap_square_activate_one,
   roadmap_square_unmap_one
};



static int roadmap_square_location (const RoadMapPosition *position, int scale_index) {

   int x;
   int y;

   RoadMapGrid *global = RoadMapSquareActive->SquareGrid;
   RoadMapScale *scale = RoadMapSquareActive->SquareScale + scale_index;

   x = (position->longitude - global->edges.west) / scale->step_longitude;
   if (x < 0 || x > scale->count_longitude) {
      return -1;
   }

   y = (position->latitude - global->edges.south)  / scale->step_latitude;
   if (y < 0 || y > scale->count_latitude) {
      return -1;
   }

   if (x >= scale->count_longitude) {
      x = scale->count_longitude - 1;
   }
   if (y >= scale->count_latitude) {
      y = scale->count_latitude - 1;
   }
   
   return RoadMapSquareActive->SquareBaseIndex[scale_index] + (x * scale->count_latitude) + y;
}


int roadmap_square_range  (void) {

	if (!RoadMapSquareActive) {
		return 0;
	}
	
	return RoadMapSquareActive->SquareRange;	
}


int roadmap_square_search (const RoadMapPosition *position, int scale_index) {

   int square;
   int scale = scale_index;

   if (RoadMapSquareActive == NULL) return ROADMAP_SQUARE_OTHER;

	if (scale == -1) scale = RoadMapScaleCurrent;
   square = roadmap_square_location (position, scale);

   if (RoadMapSquareActive->Square[square] == ROADMAP_SQUARE_UNAVAILABLE) {
      return ROADMAP_SQUARE_GLOBAL;
   }

   return square;
}


int roadmap_square_find_neighbours (const RoadMapPosition *position, int scale_index, int squares[9]) {
	
	int					count = 0;
	int					i;
	RoadMapPosition	pos;
	int					square;
	static int			neighbours[9][2] = {{0, 0}, {0, 1}, {0, -1}, {1, 0}, {-1, 0}, {1, 1}, {1, -1}, {-1, 1}, {-1, -1}};
	
	for (i = 0; i < 9; i++) {
		pos.latitude = position->latitude + 
								neighbours[i][0] * RoadMapSquareActive->SquareScale[scale_index].step_latitude; 
		pos.longitude = position->longitude + 
								neighbours[i][1] * RoadMapSquareActive->SquareScale[scale_index].step_longitude; 
		square = roadmap_square_search (&pos, scale_index);
		if (square != 	ROADMAP_SQUARE_GLOBAL) {
			squares[count++] = square;
		}	
	}
	return count;
}


void  roadmap_square_min (int square, RoadMapPosition *position) {

   if (RoadMapSquareActive == NULL) return;

	roadmap_square_set_current (square);
	
   position->longitude = RoadMapSquareActive->Square[square]->square->edges.west;
   position->latitude  = RoadMapSquareActive->Square[square]->square->edges.south;
}


void  roadmap_square_edges (int square, RoadMapArea *edges) {

   edges->west = 0;
   edges->east = 0;
   edges->north = 0;
   edges->south = 0;

   if (RoadMapSquareActive == NULL) return;

   if (square == ROADMAP_SQUARE_GLOBAL) {

      RoadMapGrid *global = RoadMapSquareActive->SquareGrid;

      *edges = global->edges;

      return;
   }

	roadmap_square_set_current (square);
	
	if (!RoadMapSquareActive->Square[square]) return;
	
   *edges = RoadMapSquareActive->Square[square]->square->edges;
}


int   roadmap_square_cross_pos (RoadMapPosition *position) {

	/* return the equivalent edge on the next square */
   int scale = roadmap_square_current_scale_factor ();
   RoadMapArea *edges = &RoadMapSquareActive->Square[RoadMapSquareCurrent]->square->edges;

   if (position->latitude < edges->south + scale) {
      position->latitude -= scale;
      return ROADMAP_DIRECTION_NORTH;
   }
   if (position->latitude > edges->north - scale) {
      position->latitude += scale;
      return ROADMAP_DIRECTION_SOUTH;
   }
   if (position->longitude < edges->west + scale) {
      position->longitude -= scale;
      return ROADMAP_DIRECTION_EAST;
   }
   if (position->longitude > edges->east - scale) {
      position->longitude += scale;
      return ROADMAP_DIRECTION_WEST;
   }

   return -1;
}

int roadmap_square_view (int *square, int size) {

   RoadMapGrid *global;
   RoadMapScale * scale;

   RoadMapArea screen;
   int x0;
   int x1;
   int x;
   int y0;
   int y1;
   int y;
   int count;
   int index;
   int base_index;


   if (RoadMapSquareActive == NULL) return 0;

   global = RoadMapSquareActive->SquareGrid;
   scale = RoadMapSquareActive->SquareScale + RoadMapScaleCurrent;
   base_index = RoadMapSquareActive->SquareBaseIndex[RoadMapScaleCurrent];

   roadmap_math_screen_edges (&screen);

   x0 = (screen.west - global->edges.west) / scale->step_longitude;
   x1 = (screen.east - global->edges.west) / scale->step_longitude;
   if ((x1 < 0) || (x0 >= scale->count_longitude)) {
      return 0;
   }
   if (x0 < 0) {
      x0 = 0;
   }
   if (x1 >= scale->count_longitude) {
      x1 = scale->count_longitude - 1;
   }

   y0 = (screen.north  - global->edges.south)  / scale->step_latitude;
   y1 = (screen.south  - global->edges.south)  / scale->step_latitude;
   if ((y0 < 0) || (y1 >= scale->count_latitude)) {
      return 0;
   }
   if (y1 < 0) {
      y1 = 0;
   }
   if (y0 >= scale->count_latitude) {
      y0 = scale->count_latitude - 1;
   }

   count = 0;

   for (x = x0; x <= x1; ++x) {

      for (y = y1; y <= y0; ++y) {

         index = base_index + x * scale->count_latitude + y;

         if (RoadMapSquareActive->Square[index] != ROADMAP_SQUARE_UNAVAILABLE) {

            square[count] = index;
            count  += 1;
            if (count >= size) {
               roadmap_log (ROADMAP_ERROR,
                            "too many square are visible: %d is not enough",
                            size);
               return size;
            }
         }
      }
   }

   return count;
}


int roadmap_square_first_point (int square) {

   roadmap_square_set_current (square);
   return 0;
}


int roadmap_square_points_count (int square) {

   roadmap_square_set_current (square);

   return roadmap_point_count ();
}

int roadmap_square_has_shapes (int square) {

   roadmap_square_set_current (square);

   return roadmap_shape_count ();
}


int roadmap_square_first_shape (int square) {

   roadmap_square_set_current (square);
   return 0;
}


static void roadmap_square_load (int square) {
	
	int scale;
	int lon;
	int lat;
	
	scale = roadmap_square_scale (square);
	if (scale < 0) return;
		
	square -= RoadMapSquareActive->SquareBaseIndex[scale];
	lon = square / RoadMapSquareActive->SquareScale[scale].count_latitude;
	lat = square % RoadMapSquareActive->SquareScale[scale].count_latitude;
	
	roadmap_locator_load_tile (scale, lon, lat);
}


void roadmap_square_load_index (void) {   

   /* temporary - force load all hi-res tiles */
	int i;
	int rc;

   rc = roadmap_city_read_file ("city_index");
	if (!rc) return;
		
	for (i = RoadMapSquareActive->SquareScale[0].count_latitude * RoadMapSquareActive->SquareScale[0].count_longitude - 1;
			i >= 0; i--) {
		
		if (RoadMapSquareActive->Square[i] != 	ROADMAP_SQUARE_UNAVAILABLE) {	
			roadmap_square_set_current (i);
			roadmap_street_update_city_index ();
		}
	}
}


void roadmap_square_rebuild_index (void) {   

   roadmap_file_remove(roadmap_db_map_path(), "city_index");
   roadmap_square_load_index();
   roadmap_city_write_file (roadmap_db_map_path(), "city_index", 0);
}


int roadmap_square_set_current (int square) {

   int j;

   if (square != RoadMapSquareCurrent) {
   
      if (square < 0) {
      	roadmap_log (ROADMAP_ERROR, "roadmap_square_set_current() : illegal square no. %d", square);
         return 0;
      }

   	if (!RoadMapSquareActive->Square[square]) {
   		
   		roadmap_square_load (square);
   		if (!RoadMapSquareActive->Square[square]) {
      		roadmap_log (ROADMAP_ERROR, "roadmap_square_set_current() : square loading FAILED. square no. %d", square);
   			return 0;
   		}
   	}
   	
      for (j = 0; j < NUM_SUB_HANDLERS; j++) {
         SquareHandlers[j].handler->activate (RoadMapSquareActive->Square[square]->subs[j]);
      }

      RoadMapSquareCurrent = square;
   	roadmap_square_promote (square);
   }

   return 1;
}

int roadmap_square_active (void) {

   return RoadMapSquareCurrent;
}


void	roadmap_square_adjust_scale (int zoom_factor) {

	int scale;
	
	if (RoadMapSquareActive == NULL) return;
	
	for (scale = 1; 
		  scale < RoadMapSquareActive->SquareGrid->num_scales &&
		  RoadMapSquareActive->SquareScale[scale].scale_factor <= zoom_factor;
		  scale++)
		  ;

	roadmap_square_set_screen_scale (scale - 1); 	
}


void	roadmap_square_set_screen_scale (int scale) {

	if (scale < 0) {
		scale = 0;
	}
	
	if (RoadMapSquareActive &&
		 RoadMapSquareActive->SquareGrid && 
		 scale >= RoadMapSquareActive->SquareGrid->num_scales) {
		scale = RoadMapSquareActive->SquareGrid->num_scales - 1;
	}
		 
	RoadMapScaleCurrent = scale;
}


int	roadmap_square_get_screen_scale (void) {

	if (!RoadMapSquareActive ||
		 !RoadMapSquareActive->SquareGrid) {
		 return 0;
	}
	
	return RoadMapScaleCurrent;
}


int	roadmap_square_get_num_scales (void) {

	if (!RoadMapSquareActive ||
		 !RoadMapSquareActive->SquareGrid) {
		 return 0;
	}
	
	return RoadMapSquareActive->SquareGrid->num_scales;
}


int roadmap_square_screen_scale_factor (void) {

	if (!RoadMapSquareActive) {
		return 1;
	}
	
	return RoadMapSquareActive->SquareScale[RoadMapScaleCurrent].scale_factor;
}


int roadmap_square_current_scale_factor (void) {
/*
	if (!RoadMapSquareActive) {
		return 1;
	}
*/	
	return RoadMapSquareActive->SquareScale[RoadMapSquareActive->Square[RoadMapSquareCurrent]->square->scale_index].scale_factor;
}


int	roadmap_square_scale (int square) {

	int scale;
	
	if (!RoadMapSquareActive ||
		 !RoadMapSquareActive->SquareGrid ||
		 square < 0) {
		return -1;
	}

	for (scale = RoadMapSquareActive->SquareGrid->num_scales - 1; scale >= 0; scale--) {
		if (square >= RoadMapSquareActive->SquareBaseIndex[scale]) break;
	}

	return scale;
}


int roadmap_square_at_current_scale (int square) {

	return roadmap_square_scale (square) == RoadMapScaleCurrent;
}

