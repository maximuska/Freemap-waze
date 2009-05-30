/* roadmap_square.h - Manage a county area, divided in small squares.
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
 */

#ifndef _ROADMAP_SQUARE__H_
#define _ROADMAP_SQUARE__H_

#include <time.h>
#include "roadmap_types.h"
#include "roadmap_dbread.h"

#define ROADMAP_SQUARE_GLOBAL -1
#define ROADMAP_SQUARE_OTHER  -2

time_t	roadmap_square_global_timestamp (void);

int   roadmap_square_range  (void);
int   roadmap_square_search (const RoadMapPosition *position, int scale_index);
int 	roadmap_square_find_neighbours (const RoadMapPosition *position, int scale_index, int squares[9]);
void  roadmap_square_min    (int square, RoadMapPosition *position);

void  roadmap_square_edges  (int square, RoadMapArea *edges);
int   roadmap_square_cross_pos (RoadMapPosition *position);

int   roadmap_square_view (int *square, int size);
int   roadmap_square_first_point  (int square);
int   roadmap_square_points_count (int square);
int   roadmap_square_has_shapes   (int square);
int   roadmap_square_first_shape  (int square);

void 	roadmap_square_load_index (void);
void  roadmap_square_rebuild_index (void);
int   roadmap_square_set_current (int square);
int	roadmap_square_active (void);

void	roadmap_square_adjust_scale (int zoom_factor);
void	roadmap_square_set_screen_scale (int scale);
int	roadmap_square_get_screen_scale (void);
int	roadmap_square_get_num_scales (void);
int 	roadmap_square_screen_scale_factor (void);
int 	roadmap_square_current_scale_factor (void);
int	roadmap_square_scale (int square);
int 	roadmap_square_at_current_scale (int square);

extern roadmap_db_handler RoadMapSquareHandler;
extern roadmap_db_handler RoadMapSquareOneHandler;

#endif // _ROADMAP_SQUARE__H_
