/* roadmap_border.c - Handle Drawing of borders
 *
 * LICENSE:
 *
 *   Copyright 2008 Avi B.S
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

#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "roadmap.h"
#include "roadmap_types.h"
#include "roadmap_gui.h"
#include "roadmap_math.h"
#include "roadmap_line.h"
#include "roadmap_street.h"
#include "roadmap_config.h"
#include "roadmap_canvas.h"
#include "roadmap_message.h"
#include "roadmap_sprite.h"
#include "roadmap_voice.h"
#include "roadmap_skin.h"
#include "roadmap_plugin.h"
#include "roadmap_square.h"
#include "roadmap_math.h"
#include "roadmap_res.h"
#include "roadmap_bar.h"
#include "roadmap_border.h"

#include "roadmap_display.h"
#include "roadmap_device.h"

static border_image s_images[border_img__count];

static const char* get_img_filename( border_images image_id)
{
   switch( image_id)
   {
      case border_heading_red_left:     return "Header_Red_left";
      case border_heading_red_right:    return "Header_Red_right";
      case border_heading_red_middle:   return "Header_Red_Mid";
      case border_heading_green_left:   return "Header_Green_left";
      case border_heading_green_right:  return "Header_Green_right";
      case border_heading_green_middle: return "Header_Green_Mid";
      case border_heading_black_left:   return "Header_Black_left";
      case border_heading_black_right:  return "Header_Black_right";
      case border_heading_black_middle: return "Header_Black_Mid";
      case border_heading_gray_left:    return "Header_Gray_left";
      case border_heading_gray_right:   return "Header_Gray_right";
      case border_heading_gray_middle:  return "Header_Gray_Mid";
      case border_image_top:            return "Top";
      case border_image_top_right:      return "TopRight";
      case border_image_top_left:       return "TopLeft";
      case border_image_bottom_no_frame: return "Bottom_NoFrame";
      case border_image_bottom:         return "Bottom";
      case border_image_bottom_right:   return "BottomRight";
      case border_image_bottom_left:    return "BottomLeft";
      case border_image_left:           return "Left";
      case border_image_right:          return "Right";

      case border_trap_top:             return "Trap_top";
      case border_trap_top_right:       return "Trap_top_right";
      case border_trap_top_left:        return "Trap_top_left";
      case border_trap_bottom:          return "Trap_bottom";
      case border_trap_bottom_right:    return "Trap_bottom_right";
      case border_trap_bottom_left:     return "Trap_bottom_left";
      case border_trap_left:            return "Trap_left";
      case border_trap_right:           return "Trap_right";

      case border_pointer_comment:      return "PointerComment";
      case border_pointer_menu:         return "PointerMenu";

      default:
         break;
   }

   return NULL;
}

BOOL load_border_images()
{
   static BOOL border_images_loaded = FALSE;
   int   i;

   if( border_images_loaded)
      return TRUE;

   for( i=0; i<border_img__count; i++)
   {
      const char* image_filename = get_img_filename(i);

      assert(NULL != image_filename);

      s_images[i].image = (RoadMapImage)roadmap_res_get(
                                    RES_BITMAP,
                                    RES_SKIN|RES_LOCK,
                                    image_filename);

      if( !s_images[i].image)
      {
         roadmap_log(ROADMAP_ERROR,
                     "load_border_images::load_border_images() - Failed to load image file '%s.png'",
                     image_filename);
         return FALSE;
      }

      s_images[i].height = roadmap_canvas_image_height(s_images[i].image);
      s_images[i].width = roadmap_canvas_image_width(s_images[i].image);

   }
   border_images_loaded = TRUE;
   return TRUE;
}

int roadmap_display_border(int style, int header, int pointer_type, RoadMapGuiPoint *bottom, RoadMapGuiPoint *top, const char* background, RoadMapPosition *position){
    //static const char *fill_bg = "#f3f3f5";// "#e4f1f9";
    RoadMapPen fill_pen;
    int screen_width, screen_height, sign_width, sign_height;
    RoadMapGuiPoint right_point, left_point, start_sides_point, point, new_point;
    int i, num_items;
    int count, start_pos_x, top_height;
    RoadMapGuiPoint sign_bottom, sign_top;

    RoadMapGuiPoint fill_points[4];


    load_border_images();

    screen_width = roadmap_canvas_width();
    screen_height = roadmap_canvas_height();

    if (top == NULL)
    {
        sign_top.x = 1;
        sign_top.y = roadmap_bar_top_height();
    }
    else{
        sign_top.x = top->x ;
        sign_top.y = top->y ;
    }

    if (bottom == NULL)
    {
        sign_bottom.x = screen_width-1;
        sign_bottom.y = screen_height - roadmap_bar_bottom_height();
    }
    else{
        sign_bottom.x = bottom->x ;
        sign_bottom.y = bottom->y ;
    }

    sign_width = sign_bottom.x  - sign_top.x;
    sign_height = sign_bottom.y - sign_top.y;

    if (header != HEADER_NONE){
        roadmap_canvas_draw_image (s_images[header].image, &sign_top, 0, IMAGE_NORMAL);

        left_point.x = sign_top.x-1;
        right_point.x = sign_top.x + sign_width - s_images[header+2].width ;

        num_items = (int)ceil((right_point.x - left_point.x - s_images[header].width)/s_images[header+1].width)+1;
        for (i = 0; i<num_items; i++){
            point.x = left_point.x + s_images[header].width + i * s_images[header+1].width ;
            point.y = sign_top.y;
            roadmap_canvas_draw_image (s_images[header+1].image, &point, 0, IMAGE_NORMAL);
        }


        point.x = right_point.x -1;
        point.y = sign_top.y;
        roadmap_canvas_draw_image (s_images[header+2].image, &point, 0, IMAGE_NORMAL);

        start_sides_point.y = sign_top.y + s_images[header].height  ;
        top_height = s_images[header].height+7;
    }
    else{

        left_point.x = sign_top.x;;
        right_point.x = sign_bottom.x - s_images[border_image_right+style].width;

        start_sides_point.y = sign_top.y + s_images[border_image_top_right+style].height;

        left_point.y = sign_top.y ;
        roadmap_canvas_draw_image (s_images[border_image_top_left+style].image, &left_point, 0, IMAGE_NORMAL);


        right_point.y = sign_top.y;
        roadmap_canvas_draw_image (s_images[border_image_top_right+style].image, &right_point, 0, IMAGE_NORMAL);

        num_items = (int)ceil((right_point.x - left_point.x - s_images[border_image_left+style].width)/s_images[border_image_top+style].width);
        for (i = 0; i<num_items; i++){
            point.x = left_point.x + s_images[border_image_top_left+style].width + i * s_images[border_image_top+style].width ;
            point.y = left_point.y;
            roadmap_canvas_draw_image (s_images[border_image_top+style].image, &point, 0, IMAGE_NORMAL);
        }

        top_height = s_images[border_image_top+style].height;
    }

    num_items = (int)ceil( sign_height - start_sides_point.y +sign_top.y - s_images[border_image_bottom+style].height ) / s_images[border_image_right+style].height;


    for (i = 0; i<num_items; i++){
        point.x = right_point.x;
        point.y = start_sides_point.y + i* s_images[border_image_right+style].height;
        roadmap_canvas_draw_image (s_images[border_image_right+style].image, &point, 0, IMAGE_NORMAL);

        point.x = left_point.x;
        point.y = start_sides_point.y + i* s_images[border_image_left+style].height;
        roadmap_canvas_draw_image (s_images[border_image_left+style].image, &point, 0, IMAGE_NORMAL);
    }


    left_point.y = point.y +  s_images[border_image_left+style].height ;
    roadmap_canvas_draw_image (s_images[border_image_bottom_left+style].image, &left_point, 0, IMAGE_NORMAL);


    right_point.y = point.y + s_images[border_image_right+style].height;
    roadmap_canvas_draw_image (s_images[border_image_bottom_right+style].image, &right_point, 0, IMAGE_NORMAL);

    if (pointer_type == POINTER_NONE){
        num_items = (int)ceil((right_point.x  - s_images[border_image_bottom_left+style].width - left_point.x)/s_images[border_image_bottom+style].width) ;

        for (i = 0; i<num_items; i++){
            point.x = left_point.x + s_images[border_image_bottom_left+style].width + i * s_images[border_image_bottom+style].width ;
            point.y = left_point.y;
            roadmap_canvas_draw_image (s_images[border_image_bottom+style].image, &point, 0, IMAGE_NORMAL);
        }
    }
    else{
        if (pointer_type == POINTER_MENU)
            num_items = 1;
        else if (pointer_type == POINTER_COMMNET)
            num_items = ((right_point.x - left_point.x)/s_images[border_image_bottom+style].width)/4 -2;
        else if (pointer_type == POINTER_POSITION)
            num_items = (right_point.x - 100)/s_images[border_image_bottom+style].width;
        else
            num_items = ((right_point.x - left_point.x)/s_images[border_image_bottom+style].width)/2 -2;
        for (i = 0; i<num_items; i++){
            point.x = left_point.x + s_images[border_image_bottom_left+style].width + i * s_images[border_image_bottom+style].width ;
            point.y = left_point.y;
            roadmap_canvas_draw_image (s_images[border_image_bottom+style].image, &point, 0, IMAGE_NORMAL);
        }


        if (pointer_type == POINTER_POSITION){
            int visible = roadmap_math_point_is_visible (position);
            if (visible){
                int count = 3;
                RoadMapGuiPoint points[3];
                RoadMapPen pointer_pen;
                roadmap_math_coordinate (position, points);
                roadmap_math_rotate_coordinates (1, points);
                if (points[0].y > (point.y +10) ){
                points[1].x = point.x;
                points[1].y = point.y+s_images[border_image_bottom_no_frame+style].height-4;
                points[2].x = point.x+40;
                points[2].y = point.y+s_images[border_image_bottom_no_frame+style].height-4;

                pointer_pen = roadmap_canvas_create_pen ("fill_pop_up_pen");
                roadmap_canvas_set_foreground("#e4f1f9");
                roadmap_canvas_draw_multiple_polygons (1, &count, points, 1, 0);
                roadmap_canvas_set_foreground("#8f979c");
                roadmap_canvas_draw_multiple_polygons (1, &count, points, 0, 0);
                point.x += 1;
                for (i=0; i<39; i++){
                    point.x = point.x + s_images[border_image_bottom_no_frame+style].width ;
                    point.y = point.y;
                    roadmap_canvas_draw_image (s_images[border_image_bottom_no_frame+style].image, &point, 0, IMAGE_NORMAL);
                }
                }

            }
            start_pos_x = point.x+1;
        }
        else{
            roadmap_canvas_draw_image (s_images[pointer_type].image, &point, 0, IMAGE_NORMAL);
            start_pos_x = point.x + s_images[pointer_type].width;
        }



        num_items = (int)ceil((right_point.x - start_pos_x )/s_images[border_image_bottom+style].width) ;
        for (i = 0; i<num_items; i++){
            new_point.x = start_pos_x + i * s_images[border_image_bottom+style].width ;
            new_point.y = point.y;
            roadmap_canvas_draw_image (s_images[border_image_bottom+style].image, &new_point, 0, IMAGE_NORMAL);
        }
    }

    //Fill the
    fill_points[0].x =right_point.x+7 ;
    fill_points[0].y =point.y +7;
    fill_points[1].x =right_point.x +7;
    fill_points[1].y = top->y + top_height -7;
    fill_points[2].x = left_point.x + s_images[border_image_left+style].width -7;
    fill_points[2].y = top->y + top_height - 7;
    fill_points[3].x =left_point.x + s_images[border_image_left+style].width -7;
    fill_points[3].y = point.y +7;
    count = 4;

    fill_pen = roadmap_canvas_create_pen ("fill_pop_up_pen");
    roadmap_canvas_set_foreground(background);

    roadmap_canvas_draw_multiple_polygons (1, &count, fill_points, 1,0 );

    return sign_width;

}

int get_heading_height(int header_type){
    return s_images[header_type].height;
}

void roadmap_border_load_images(){
    load_border_images();
}
