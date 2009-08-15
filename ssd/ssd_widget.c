/* ssd_widget.c - small screen devices Widgets (designed for touchscreens)
 * (requires agg)
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
 *   See ssd_widget.h
 */

#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include "roadmap.h"
#include "roadmap_canvas.h"
#include "roadmap_lang.h"
#include "ssd_widget.h"
#include "roadmap_bar.h"
#include "roadmap_softkeys.h"

#ifdef _WIN32
   #ifdef   TOUCH_SCREEN
      #pragma message("    Target device type:    TOUCH-SCREEN")
   #else
      #pragma message("    Target device type:    MENU ONLY (NO TOUCH-SCREEN)")
   #endif   // TOUCH_SCREEN
#endif   // _WIN32

#define SSD_WIDGET_SEP 2
#define MAX_WIDGETS_PER_ROW 100

// Get child by ID (name)
// Can return child child...
SsdWidget ssd_widget_get (SsdWidget child, const char *name) {

   if (!name) return child;

   while (child != NULL) {
      if (0 == strcmp (child->name, name)) {
         return child;
      }

      if (child->children != NULL) {
         SsdWidget w = ssd_widget_get (child->children, name);
         if (w) return w;
      }

      child = child->next;
   }

   return NULL;
}


static void calc_pack_size (SsdWidget w_cur, RoadMapGuiRect *rect,
                            SsdSize *size) {

   int         width       = rect->maxx - rect->minx + 1;
   int         height      = rect->maxy - rect->miny + 1;
   int         cur_width   = 0;
   int         max_height  = 0;
   int         max_width   = 0;
   int         index_in_row= 0;
   SsdWidget   w_last_drawn= NULL;
   SsdWidget   w_prev      = NULL;

   while( w_cur)
   {
      SsdSize max_size;
      SsdSize size = {0, 0};

      // If hiding widget - hide also all children:
      if (w_cur->flags & SSD_WIDGET_HIDE) {
         w_cur = w_cur->next;
         continue;
      }

      if (0 == index_in_row) {
         width  = rect->maxx - rect->minx + 1;

         // Padd with space?
         if (w_cur->flags & SSD_WIDGET_SPACE) {
            rect->miny  += 2;
            width       -= 2;
            cur_width   += 2;
         }
      }

      if (!(w_cur->flags & SSD_START_NEW_ROW)) {
         max_size.width = width;
         max_size.height= rect->maxy - rect->miny + 1;
         ssd_widget_get_size (w_cur, &size, &max_size);
      }

      if ((index_in_row == MAX_WIDGETS_PER_ROW) ||
         ((index_in_row > 0) &&
                     ((cur_width + size.width) > width)) ||
                     (w_cur->flags & SSD_START_NEW_ROW)) {

         if (cur_width > max_width) max_width = cur_width;

         if (w_last_drawn &&
            (w_last_drawn->flags & SSD_WIDGET_SPACE)) {

            rect->miny += SSD_WIDGET_SEP;
         }

         index_in_row = 0;
         cur_width = 0;
         w_last_drawn = w_cur;
         w_prev = NULL;

         rect->miny += max_height;
         max_height = 0;

         if (w_cur->flags & SSD_WIDGET_SPACE) {
            rect->miny += 2;
            width -= 2;
            cur_width += 2;
         }

         max_size.width = width;
         max_size.height = rect->maxy - rect->miny + 1;
         ssd_widget_get_size (w_cur, &size, &max_size);
      }

      index_in_row++;
      cur_width += size.width;
      if (w_prev && w_prev->flags & SSD_WIDGET_SPACE) {
         cur_width += SSD_WIDGET_SEP;
      }

      if (size.height > max_height) max_height = size.height;

      if (w_cur->flags & SSD_END_ROW) {
         if (cur_width > max_width) max_width = cur_width;

         index_in_row = 0;
         cur_width = 0;
         rect->miny += max_height;
         if (w_last_drawn &&
            (w_last_drawn->flags & SSD_WIDGET_SPACE)) {

            rect->miny += SSD_WIDGET_SEP;
         }
         max_height = 0;
         w_last_drawn = w_cur;
         w_prev = NULL;
      }

      w_prev = w_cur;
      w_cur = w_cur->next;
   }

   if (index_in_row) {
      if (cur_width > max_width) max_width = cur_width;

      index_in_row = 0;
      cur_width = 0;
      rect->miny += max_height;
      if (w_last_drawn &&
            (w_last_drawn->flags & SSD_WIDGET_SPACE)) {

         rect->miny += SSD_WIDGET_SEP;
      }
   }

   size->width = max_width;
   size->height = height - (rect->maxy - rect->miny);
}


static void ssd_widget_draw_one (SsdWidget w, int x, int y, int height) {

   RoadMapGuiRect rect;
   SsdSize size;

   ssd_widget_get_size (w, &size, NULL);

   if (w->flags & SSD_ALIGN_VCENTER) {
      y += (height - size.height) / 2;
   }

   w->position.x = x;
   w->position.y = y;

   if (size.width && size.height) {
      rect.minx = x;
      rect.miny = y;
      rect.maxx = x + size.width - 1;
      rect.maxy = y + size.height - 1;

#if 0
      if (!w->parent) printf("****** start draw ******\n");
      printf("draw - %s:%s %d-%d\n", w->_typeid, w->name, rect.miny, rect.maxy);
#endif

      w->draw(w, &rect, 0);

      if (w->children) ssd_widget_draw (w->children, &rect, w->flags);
   }
}


static int ssd_widget_draw_row (SsdWidget *w, int count,
                                int width, int height,
                                int x, int y) {
   int row_height = 0;
   int cur_x;
   int total_width;
   int space;
   int vcenter = 0;
   int bottom = 0;
   int i;
   int rtl = ssd_widget_rtl(w[0]->parent);
   SsdWidget prev_widget;

   if (w[0]->flags & SSD_ALIGN_LTR) rtl = 0;

   if (rtl) cur_x = x;
   else cur_x = x + width;

   for (i=0; i<count; i++) {
      SsdSize size;
      ssd_widget_get_size (w[i], &size, NULL);

      if (size.height > row_height) row_height = size.height;
      if (w[i]->flags & SSD_ALIGN_VCENTER) vcenter = 1;
      if (w[i]->flags & SSD_ALIGN_BOTTOM) bottom = 1;
   }

   if (bottom) {
      y += (height - row_height);

   } else if (vcenter) {
      y += (height - row_height) / 2;
   }

   for (i=count-1; i>=0; i--) {
      SsdSize size;
      ssd_widget_get_size (w[i], &size, NULL);

      if (w[i]->flags & SSD_ALIGN_RIGHT) {
         cur_x += w[i]->offset_x;
         if (rtl) {
            if ((i < (count-1)) && (w[i+1]->flags & SSD_WIDGET_SPACE)) {
               cur_x += SSD_WIDGET_SEP;
            }
            ssd_widget_draw_one (w[i], cur_x, y + w[i]->offset_y, row_height);
            cur_x += size.width;
         } else {
            cur_x -= size.width;
            if ((i < (count-1)) && (w[i+1]->flags & SSD_WIDGET_SPACE)) {
               cur_x -= SSD_WIDGET_SEP;
            }
            ssd_widget_draw_one (w[i], cur_x, y + w[i]->offset_y, row_height);
         }
         //width -= size.width;
         if ((i < (count-1)) && (w[i+1]->flags & SSD_WIDGET_SPACE)) {
            //width -= SSD_WIDGET_SEP;
         }
         count--;
         if (i != count) {
            memmove (&w[i], &w[i+1], sizeof(w[0]) * count - i);
         }
      }
   }

   if (rtl) cur_x = x + width;
   else cur_x = x;

   prev_widget = NULL;

   while (count > 0) {
      SsdSize size;

      if (w[0]->flags & SSD_ALIGN_CENTER) {
         break;
      }

      ssd_widget_get_size (w[0], &size, NULL);

      if (rtl) {
         cur_x -= size.width;
         cur_x -= w[0]->offset_x;
         if (prev_widget && (prev_widget->flags & SSD_WIDGET_SPACE)) {
            cur_x -= SSD_WIDGET_SEP;
         }
         ssd_widget_draw_one (w[0], cur_x, y + w[0]->offset_y, row_height);
      } else {
         cur_x += w[0]->offset_x;
         if (prev_widget && (prev_widget->flags & SSD_WIDGET_SPACE)) {
            cur_x += SSD_WIDGET_SEP;
         }
         ssd_widget_draw_one (w[0], cur_x, y + w[0]->offset_y, row_height);
         cur_x += size.width;
      }

      width -= size.width;
      if (prev_widget && (prev_widget->flags & SSD_WIDGET_SPACE)) {
         width -= SSD_WIDGET_SEP;
      }
      prev_widget = *w;
      w++;
      count--;
   }

   if (count == 0) return row_height;

   /* align center */

   total_width = 0;
   for (i=0; i<count; i++) {
      SsdSize size;
      ssd_widget_get_size (w[i], &size, NULL);

      total_width += size.width;
   }

   space = (width - total_width) / (count + 1);

   for (i=0; i<count; i++) {
      SsdSize size;
      ssd_widget_get_size (w[i], &size, NULL);

      if (rtl) {
         cur_x -= space;
         cur_x -= size.width;
         cur_x -= w[i]->offset_x;
         ssd_widget_draw_one (w[i], cur_x, y + w[i]->offset_y, row_height);
      } else {
         cur_x += space;
         cur_x += w[i]->offset_x;
         ssd_widget_draw_one (w[i], cur_x, y + w[i]->offset_y, row_height);
         cur_x += size.width;
      }
   }

   return row_height;
}


static int ssd_widget_draw_grid_row (SsdWidget *w, int count,
                                     int width,
                                     int avg_width,
                                     int height,
                                     int x, int y) {
   int cur_x;
   int space;
   int i;
   int rtl = ssd_widget_rtl (w[0]->parent);

   if (w[0]->flags & SSD_ALIGN_LTR) rtl = 0;

   /* align center */
   space = (width - avg_width*count) / (count + 1);

   if (rtl) cur_x = x + width;
   else cur_x = x;

   for (i=0; i<count; i++) {

      if (rtl) {
         cur_x -= space;
         cur_x -= avg_width;
         cur_x -= w[i]->offset_x;
         ssd_widget_draw_one (w[i], cur_x, y + w[i]->offset_y, height);
      } else {
         cur_x += space;
         cur_x += w[i]->offset_x;
         ssd_widget_draw_one (w[i], cur_x, y + w[i]->offset_y, height);
         cur_x += avg_width;
      }
   }

   return height;
}


static void ssd_widget_draw_grid (SsdWidget w, const RoadMapGuiRect *rect) {
   SsdWidget widgets[MAX_WIDGETS_PER_ROW];
   int width  = rect->maxx - rect->minx + 1;
   int height = rect->maxy - rect->miny + 1;
   SsdSize max_size;
   int cur_y = rect->miny;
   int max_height = 0;
   int avg_width = 0;
   int count = 0;
   int width_per_row;
   int cur_width = 0;
   int rows;
   int num_widgets;
   int space;
   int i;

   max_size.width = width;
   max_size.height = height;

   while (w != NULL) {
      SsdSize size;
      ssd_widget_get_size (w, &size, &max_size);

      if (size.height > max_height) max_height = size.height;
      avg_width += size.width;

      widgets[count] = w;
      count++;

      if (count == sizeof(widgets) / sizeof(SsdWidget)) {
         roadmap_log (ROADMAP_FATAL, "Too many widgets in grid!");
      }

      w = w->next;
   }

   max_height += SSD_WIDGET_SEP;
   avg_width = avg_width / count + 1;

   rows = height / max_height;

   while ((rows > 1) && ((count * avg_width / rows) < (width * 3 / 5))) rows--;

   if ((rows == 1) && (count > 2)) rows++;

   width_per_row = count * avg_width / rows;
   num_widgets = 0;

   space = (height - max_height*rows) / (rows + 1);

   for (i=0; i < count; i++) {
      SsdSize size;
      ssd_widget_get_size (widgets[i], &size, NULL);

      cur_width += avg_width;

      if (size.width > avg_width*1.5) {
         cur_width += avg_width;
      }

      num_widgets++;

      if ((cur_width >= width_per_row) || (i == (count-1))) {
         cur_y += space;

          ssd_widget_draw_grid_row
               (&widgets[i-num_widgets+1], num_widgets,
                width, avg_width, max_height, rect->minx, cur_y);
         cur_y += max_height;
         cur_width = 0;
         num_widgets = 0;
      }
   }
}


static void ssd_widget_draw_pack (SsdWidget w, const RoadMapGuiRect *rect) {
   SsdWidget row[MAX_WIDGETS_PER_ROW];
   int width  = rect->maxx - rect->minx + 1;
   int height = rect->maxy - rect->miny + 1;
   int minx   = rect->minx;
   int cur_y  = rect->miny;
   int cur_width = 0;
   int count = 0;
   SsdWidget w_last_drawn = NULL;
   SsdWidget w_prev = NULL;

   while (w != NULL) {

      SsdSize size =   {0,0};
      SsdSize max_size;

      if (w->flags & SSD_WIDGET_HIDE) {
         w = w->next;
         continue;
      }

      if (!count) {
         width  = rect->maxx - rect->minx + 1;
         height = rect->maxy - cur_y + 1;
         minx   = rect->minx;

         if (w->flags & SSD_WIDGET_SPACE) {
            width  -= 4;
            height -= 2;
            cur_y  += 2;
            minx   += 2;
         }
      }

      if (!(w->flags & SSD_START_NEW_ROW)) {
         max_size.width = width - cur_width;
         max_size.height = height;
         ssd_widget_get_size (w, &size, &max_size);
      }

      if ((count == MAX_WIDGETS_PER_ROW) ||
         ((count > 0) &&
                     (((cur_width + size.width) > width) ||
                     (w->flags & SSD_START_NEW_ROW)))) {

         if (w_last_drawn &&
            (w_last_drawn->flags & SSD_WIDGET_SPACE)) {

            cur_y += SSD_WIDGET_SEP;
         }

         cur_y += ssd_widget_draw_row
                     (row, count, width, height, minx, cur_y);
         count = 0;
         cur_width = 0;
         w_last_drawn = w_prev;
         w_prev = NULL;

         width  = rect->maxx - rect->minx + 1;
         height = rect->maxy - cur_y + 1;
         minx   = rect->minx;

         if (w->flags & SSD_WIDGET_SPACE) {
            width  -= 4;
            height -= 2;
            cur_y  += 2;
            minx   += 2;
         }

         max_size.width = width;
         max_size.height = height;
         ssd_widget_get_size (w, &size, &max_size);
      }

      row[count++] = w;

      cur_width += size.width;
      if (w_prev && w_prev->flags & SSD_WIDGET_SPACE) {
         cur_width += SSD_WIDGET_SEP;
      }

      if (w->flags & SSD_END_ROW) {

         if (w_last_drawn &&
            (w_last_drawn->flags & SSD_WIDGET_SPACE)) {

            cur_y += SSD_WIDGET_SEP;
         }

         cur_y += ssd_widget_draw_row
                     (row, count, width, height, minx, cur_y);
         count = 0;
         cur_width = 0;
         w_last_drawn = w;
         w_prev = NULL;
      }

      w_prev = w;
      w = w->next;
   }

   if (count) {
      if (w_last_drawn &&
            (w_last_drawn->flags & SSD_WIDGET_SPACE)) {

         cur_y += SSD_WIDGET_SEP;
      }
      ssd_widget_draw_row (row, count, width, height, minx, cur_y);
   }
}


void ssd_widget_draw (SsdWidget w, const RoadMapGuiRect *rect,
                      int parent_flags) {

   if (parent_flags & SSD_ALIGN_GRID) ssd_widget_draw_grid (w, rect);
   else ssd_widget_draw_pack (w, rect);
}

static BOOL ssd_widget_default_on_key_pressed( SsdWidget w, const char* utf8char, uint32_t flags)
{ return FALSE;}

roadmap_input_type ssd_widget_get_input_type( SsdWidget this)
{
   roadmap_input_type t = inputtype_none;

   if( this->get_input_type)
      t = this->get_input_type( this);

   if( inputtype_none == t)
   {
      SsdWidget next = this->children;
      while( (inputtype_none == t) && next)
      {
         t     = ssd_widget_get_input_type( next);
         next  = next->next;
      }
   }

   return t;
}

SsdWidget ssd_widget_new (const char *name,
                          PFN_WIDGET_ONKEYPRESSED pfn_on_key_pressed,
                          int flags) {

   static int tab_order_sequence = 0;

   SsdWidget w;

   w = (SsdWidget) calloc (1, sizeof (*w));

   roadmap_check_allocated(w);

   w->name           = strdup(name);
   w->size.height    = SSD_MIN_SIZE;
   w->size.width     = SSD_MIN_SIZE;
   w->in_focus       = FALSE;
   w->default_widget = (SSD_WS_DEFWIDGET  & flags)? TRUE: FALSE;
   w->tab_stop       = (SSD_WS_TABSTOP & flags)? TRUE: FALSE;
   w->prev_tabstop   = NULL;
   w->next_tabstop   = NULL;
   w->left_tabstop   = NULL;
   w->right_tabstop  = NULL;
   w->tabstop_above  = NULL;
   w->tabstop_below  = NULL;
   w->parent_dialog  = NULL;
   w->get_input_type = NULL;
   w->tab_sequence   = tab_order_sequence++;

   if( pfn_on_key_pressed)
      w->key_pressed = pfn_on_key_pressed;
   else
      w->key_pressed = ssd_widget_default_on_key_pressed;

   w->cached_size.height = w->cached_size.width = -1;

   return w;
}


SsdWidget ssd_widget_find_by_pos (SsdWidget widget,
                                  const RoadMapGuiPoint *point) {

   while (widget != NULL) {
      SsdSize size;
      ssd_widget_get_size (widget, &size, NULL);

      if ((widget->position.x <= point->x) &&
          ((widget->position.x + size.width) >= point->x) &&
          (widget->position.y <= point->y) &&
          ((widget->position.y + size.height) >= point->y)) {

         return widget;
      }

      widget = widget->next;
   }

   return NULL;
}


void ssd_widget_set_callback (SsdWidget widget, SsdCallback callback) {
   widget->callback = callback;
}


int ssd_widget_rtl (SsdWidget parent) {

   if (parent && (parent->flags & SSD_ALIGN_LTR)) return 0;
   else return roadmap_lang_rtl ();
}


int ssd_widget_pointer_down (SsdWidget widget, const RoadMapGuiPoint *point) {

   widget = ssd_widget_find_by_pos (widget, point);

   if (!widget) return 0;

   if (widget->pointer_down && widget->pointer_down(widget, point)) {
      return 1;

   } else if (widget->children != NULL) {

      return ssd_widget_pointer_down (widget->children, point);
   }

   return 0;
}


int ssd_widget_short_click (SsdWidget widget, const RoadMapGuiPoint *point) {

   widget = ssd_widget_find_by_pos (widget, point);

   if (!widget) return 0;

   if (widget->short_click && widget->short_click(widget, point)) {
      return 1;

   } else if (widget->children != NULL) {

      return ssd_widget_short_click (widget->children, point);
   }

   return 0;
}

void ssd_widget_set_backgroundfocus( SsdWidget w, BOOL set)
{
   if(w->tab_stop)
      w->background_focus = set;
   else
      w->background_focus = FALSE;
}

BOOL ssd_widget_set_focus( SsdWidget w)
{
   if( w->tab_stop)
   {
      roadmap_input_type input_type;

      w->in_focus = TRUE;

      input_type = ssd_widget_get_input_type( w );

      roadmap_input_type_set_mode( input_type );
   }

   w->background_focus = FALSE;

   return w->in_focus;
}

void ssd_widget_loose_focus   ( SsdWidget w)
{
   w->in_focus          = FALSE;
   w->background_focus  = FALSE;
}

static SsdWidget ssd_widget_focus_backward( SsdWidget w)
{
   if( !w->prev_tabstop || (w->prev_tabstop == w))
      return w;   // Only one tab-stop in w dialog. Nothing changed.

   return w->prev_tabstop;
}

static SsdWidget ssd_widget_focus_forward(SsdWidget w)
{
   if(!w->next_tabstop || (w->next_tabstop == w))
      return w;   // Only one tab-stop in w dialog. Nothing changed.

   return w->next_tabstop;
}

static SsdWidget ssd_widget_focus_left( SsdWidget w) {

   if( !w->left_tabstop) return ssd_widget_focus_backward( w);
   else return w->left_tabstop;
}

static SsdWidget ssd_widget_focus_right( SsdWidget w) {

   if( !w->right_tabstop) return ssd_widget_focus_forward( w);
   else return w->right_tabstop;
}

static SsdWidget ssd_widget_focus_up( SsdWidget w) {

   if( !w->tabstop_above) return ssd_widget_focus_backward( w);
   else return w->tabstop_above;
}

static SsdWidget ssd_widget_focus_down( SsdWidget w)
{
   if( !w->tabstop_below) return ssd_widget_focus_forward( w);
   else return w->tabstop_below;
}

static void ssd_widget_sort_children (SsdWidget widget) {
   SsdWidget first = widget;
   SsdWidget prev = NULL;
   SsdWidget bottom = NULL;

   if (!widget) return;

   /* No support for first widget as ORDER_LAST */
   assert (! (widget->flags & SSD_ORDER_LAST));

   assert( widget != widget->next);

   while (widget) {

      if (widget->flags & SSD_ORDER_LAST) {
         SsdWidget tmp = widget->next;

         if (prev) prev->next = widget->next;
         widget->next = bottom;
         bottom = widget;
         widget = tmp;

      } else {
         prev = widget;
         widget = widget->next;
      }
   }

   if (bottom) prev->next = bottom;

   while (first) {
      ssd_widget_sort_children (first->children);
      first = first->next;
   }
}


SsdWidget ssd_widget_move_focus (SsdWidget w, SSD_FOCUS_DIRECTION direction) {
   SsdWidget next_w;
   if (!w) return NULL;

   switch (direction) {
   case FOCUS_BACK: next_w = ssd_widget_focus_backward(w); break;
   case FOCUS_FORWARD: next_w = ssd_widget_focus_forward(w); break;
   case FOCUS_UP: next_w = ssd_widget_focus_up(w); break;
   case FOCUS_DOWN: next_w = ssd_widget_focus_down(w); break;
   case FOCUS_LEFT: next_w = ssd_widget_focus_left(w); break;
   case FOCUS_RIGHT: next_w = ssd_widget_focus_right(w); break;
   default: next_w = w; break;
   }

   if(w == next_w) return w;   // Only one tab-stop in w dialog. Nothing changed.

   ssd_widget_loose_focus  (w);
   ssd_widget_set_focus    (next_w);

   return next_w;
}

int ssd_widget_long_click (SsdWidget widget, const RoadMapGuiPoint *point) {

   widget = ssd_widget_find_by_pos (widget, point);

   if (!widget) return 0;

   if (widget->long_click && widget->long_click(widget, point)) {
      return 1;

   } else if (widget->children != NULL) {

      return ssd_widget_long_click (widget->children, point);
   }

   return 0;
}

const char *ssd_widget_get_value (const SsdWidget widget, const char *name) {
   SsdWidget w = widget;

   if (name) w = ssd_widget_get (w, name);
   if (!w) return "";

   if (w->get_value) return w->get_value (w);

   return w->value;
}


const void *ssd_widget_get_data (const SsdWidget widget, const char *name) {
   SsdWidget w = ssd_widget_get (widget, name);
   if (!w) return NULL;

   if (w->get_data) return w->get_data (w);
   else return NULL;
}


int ssd_widget_set_value (const SsdWidget widget, const char *name,
                          const char *value) {

   SsdWidget w = ssd_widget_get (widget, name);
   if (!w || !w->set_value) return -1;

   return w->set_value(w, value);
}


int ssd_widget_set_data (const SsdWidget widget, const char *name,
                          const void *value) {

   SsdWidget w = ssd_widget_get (widget, name);
   if (!w || !w->set_data) return -1;

   return w->set_data(w, value);
}


void ssd_widget_add (SsdWidget parent, SsdWidget child) {

   SsdWidget last = parent->children;

   child->parent = parent;

   if (!last) {
      parent->children = child;
      return;
   }

   while (last->next) last=last->next;
   last->next = child;

   /* TODO add some dirty flag and only resort when needed */
   ssd_widget_sort_children(parent->children);
}

SsdWidget ssd_widget_remove(SsdWidget parent, SsdWidget child)
{
   SsdWidget cur_child     = parent->children;
   SsdWidget child_behind  = NULL;

   assert(parent);
   assert(child);

   while( cur_child)
   {
      if( cur_child == child)
      {
         if( child_behind)
            child_behind->next= child->next;
         else
            parent->children  = child->next;

         //assert(0 && "need to re-sort tab order!");
         return child;
      }

      child_behind= cur_child;
      cur_child   = cur_child->next;
   }

   return NULL;
}

SsdWidget ssd_widget_replace(SsdWidget parent, SsdWidget old_child, SsdWidget new_child)
{
   SsdWidget cur_child     = parent->children;
   SsdWidget child_behind  = NULL;

   assert(parent);
   assert(old_child);
   assert(new_child);

   while( cur_child)
   {
      if( cur_child == old_child)
      {
         if( child_behind)
            child_behind->next= new_child;
         else
            parent->children  = new_child;

         new_child->next   = old_child->next;
         new_child->parent = old_child->parent;

         return old_child;
      }

      child_behind= cur_child;
      cur_child   = cur_child->next;
   }

   return NULL;
}

void ssd_widget_set_size (SsdWidget widget, int width, int height) {

   widget->size.width  = width;
   widget->size.height = height;
}


void ssd_widget_set_offset (SsdWidget widget, int x, int y) {

   widget->offset_x = x;
   widget->offset_y = y;
}


void ssd_widget_set_context (SsdWidget widget, void *context) {

   widget->context = context;
}


void ssd_widget_get_size (SsdWidget w, SsdSize *size, const SsdSize *max) {

   SsdSize pack_size = {0, 0};

   RoadMapGuiRect max_size = {0, 0, 0, 0};
   int total_height_below = 0;

   *size = w->size;

   if ((w->size.height >= 0) && (w->size.width >= 0)) {
      return;
   }

   if (!max && (w->cached_size.width < 0)) {
       static SsdSize canvas_size;

       canvas_size.width   = roadmap_canvas_width();
       canvas_size.height  = roadmap_canvas_height() - roadmap_bar_bottom_height();

       max = &canvas_size;
   }
   else{
    if (!max)
        max = &w->cached_size;
   }



   if ((w->cached_size.width >= 0) && (w->cached_size.height >= 0)) {
      *size = w->cached_size;
      return;
   }

   ///[BOOKMARK]:[NOTE]:[PAZ] - Special case solution:
   ///   Line(n) asked for MAX-HEIGHT, but it needs to accomodate
   ///   for the height of line(n+1).
   ///   This solution is for such case, where there is only
   ///   a sinlge container with this flag (SSD_ORDER_LAST).
   ///   E.g. - List box (n) and softkeys buttons (n+1)
   if (size->height == SSD_MAX_SIZE) {
      /* Check if other siblings exists and should be placed below this one */
      SsdWidget below_w = w->next;

      while (below_w) {

         if (below_w->flags & SSD_ORDER_LAST) {
            SsdSize s;
            ssd_widget_get_size (below_w, &s, max);

            total_height_below += s.height;
         }
         below_w = below_w->next;
      }
   }

   if (w->flags & SSD_DIALOG_FLOAT) {

      if (size->width == SSD_MAX_SIZE) size->width = max->width -10;
      if (size->height== SSD_MAX_SIZE) size->height= max->height - total_height_below;

   } else {

      if (size->width == SSD_MAX_SIZE) size->width = max->width;
      if (size->height== SSD_MAX_SIZE) size->height= max->height - total_height_below;
   }

   if ((size->height >= 0) && (size->width >= 0)) {
      w->cached_size = *size;
      return;
   }

   if (size->width >= 0)  {
      max_size.maxx = size->width - 1;
   } else {
      if (!max){
                static SsdSize canvas_size;
                canvas_size.width = roadmap_canvas_width();
                canvas_size.height = roadmap_canvas_height() - roadmap_bar_bottom_height();
                max = &canvas_size;
      }
      max_size.maxx = max->width - 1;
   }

   if (size->height >= 0) {
      max_size.maxy = size->height - 1;
   } else {
      max_size.maxy = max->height - 1;
   }

   if (!(w->flags & SSD_VAR_SIZE) && w->children) {
      RoadMapGuiRect container_rect = max_size;
      int container_width;
      int container_height;

      w->draw (w, &max_size, SSD_GET_SIZE);

      container_width  = max_size.minx - container_rect.minx +
                         container_rect.maxx - max_size.maxx;
      container_height = max_size.miny - container_rect.miny +
                         container_rect.maxy - max_size.maxy;

      calc_pack_size (w->children, &max_size, &pack_size);

      pack_size.width  += container_width;
      pack_size.height += container_height;

   } else {
      w->draw (w, &max_size, SSD_GET_SIZE);
      pack_size.width  = max_size.maxx - max_size.minx + 1;
      pack_size.height = max_size.maxy - max_size.miny + 1;
   }

   if (size->height< 0) size->height = pack_size.height;
   if (size->width < 0) size->width  = pack_size.width;

   w->cached_size = *size;
}


void ssd_widget_set_color (SsdWidget w, const char *fg_color,
                           const char *bg_color) {
   w->fg_color = fg_color;
   w->bg_color = bg_color;
}


void ssd_widget_container_size (SsdWidget dialog, SsdSize *size) {

   SsdSize max_size;

   /* Calculate size recurisvely */
   if (dialog->parent) {
      ssd_widget_container_size (dialog->parent, size);
      max_size.width = size->width;
      max_size.height = size->height;

   } else {
      max_size.width = roadmap_canvas_width ();
      max_size.height = roadmap_canvas_height () - roadmap_bar_bottom_height();
   }

   ssd_widget_get_size (dialog, size, &max_size);

   if (dialog->draw) {
      RoadMapGuiRect rect;
      rect.minx = 0;
      rect.miny = 0;
      rect.maxx = size->width - 1;
      rect.maxy = size->height - 1;

      dialog->draw (dialog, &rect, SSD_GET_SIZE|SSD_GET_CONTAINER_SIZE);

      size->width = rect.maxx - rect.minx + 1;
      size->height = rect.maxy - rect.miny + 1;
   }
}


void *ssd_widget_get_context (SsdWidget w) {

   return w->context;
}


void ssd_widget_reset_cache (SsdWidget w) {

   SsdWidget child = w->children;

   w->cached_size.width = w->cached_size.height = -1;

   while (child != NULL) {

      ssd_widget_reset_cache (child);
      child = child->next;
   }
}


void ssd_widget_hide (SsdWidget w) {
   w->flags |= SSD_WIDGET_HIDE;
}


void ssd_widget_show (SsdWidget w) {
   w->flags &= ~SSD_WIDGET_HIDE;
}


void ssd_widget_set_flags (SsdWidget widget, int flags) {

   widget->flags = flags;
   widget->default_widget = (SSD_WS_DEFWIDGET  & flags)? TRUE: FALSE;
   widget->tab_stop       = (SSD_WS_TABSTOP & flags)? TRUE: FALSE;
}

// Macros used for GUI-wize tab-sort (left,right,up,down)
#define  WIDGET_WIDTH(_w_)                            ((_w_->cached_size.width < 0)? max_width:  _w_->cached_size.width)
#define  WIDGET_HEIGHT(_w_)                           ((_w_->cached_size.height< 0)? max_height: _w_->cached_size.height)
#define  WIDGET_HALF_WIDTH(_w_)                       ((int)(WIDGET_WIDTH(_w_)/2))
#define  WIDGET_HALF_HEIGHT(_w_)                      ((int)(WIDGET_HEIGHT(_w_)/2))
#define  GET_X_DISTANCE(_w_,_other_)                  (((_w_)->position.x + WIDGET_HALF_WIDTH (_w_)) - ((_other_)->position.x + WIDGET_HALF_WIDTH (_other_)))
#define  GET_Y_DISTANCE(_w_,_other_)                  (((_w_)->position.y + WIDGET_HALF_HEIGHT(_w_)) - ((_other_)->position.y + WIDGET_HALF_HEIGHT(_other_)))
#define  GET_ABSOLUTE_X_DISTANCE(_w_,_other_)         (abs(GET_X_DISTANCE(_w_,_other_)))
#define  GET_ABSOLUTE_Y_DISTANCE(_w_,_other_)         (abs(GET_Y_DISTANCE(_w_,_other_)))
#define  OBJECT_IS_TO_THE_LEFT(_w_,_other_)           (_other_->position.x < _w_->position.x)
#define  OBJECT_IS_TO_THE_RIGHT(_w_,_other_)          (_w_->position.x < _other_->position.x)
#define  OBJECT_IS_ABOVE(_w_,_other_)                 (_other_->position.y < _w_->position.y)
#define  OBJECT_IS_BELOW(_w_,_other_)                 (_w_->position.y < _other_->position.y)
#define  CONSIDER_A_NEW_NEIGHBOUR(_next_,_prev_)                        \
{                                                                       \
   if( w->_next_)                                                       \
   {                                                                    \
      /* Maybe our current neighbour with w new one (other)?   */       \
      w->_next_ = ssd_widget_select_nearest_neighbour(                  \
            w,                /* THE object  (w)               */       \
            w->_next_,        /* Option a (current)            */       \
            other);              /* Option b (new)             */       \
                                                                        \
      /* Set also other-side pointer, to point back at us      */       \
      if( w->_next_->_prev_)                                            \
      w->_next_->_prev_ = ssd_widget_select_nearest_neighbour(          \
            w->_next_,        /* THE object  (w)               */       \
            w->_next_->_prev_,/* Option a (current)            */       \
            w);               /* Option b (us)                 */       \
      else                                                              \
      w->_next_->_prev_ = w;                                            \
   }                                                                    \
   else                                                                 \
   w->_next_ = other;                                                   \
}

// Method to return a widget-pointer (left,right,top,bottom)
typedef SsdWidget*   (*PFN_GET_WIDGET_PTR)   (SsdWidget);

// Helper for 'ssd_widget_sort_gui_tab_order__fix_orphan_pointers()' below
void ssd_widget_sort_gui_tab_order__fix_corners(SsdWidget   w,
      PFN_GET_WIDGET_PTR      pfn_back_ptr,
      PFN_GET_WIDGET_PTR      pfn_next_ptr)
{
   int         max_width   = roadmap_canvas_width();
   int         max_height  = roadmap_canvas_height() - roadmap_bar_bottom_height();
   SsdWidget   p           = w;
   SsdWidget   far_widget  = NULL;     // This will be used for cases of loops
   int         far_distance= 0;
   SsdWidget   loop_watch  = w;     // Loop watch-dog
   BOOL        inc_watch   = FALSE; // Loop watch-dog
   int         distance;

   if( *(pfn_next_ptr(w)))
      return;  // Already set

   while( *(pfn_back_ptr(p)))
   {
      p = *pfn_back_ptr(p);

      // Keep track on widget, which is the most-far (farest?) from us:
      // The 'far_widget' will be used as a match for cases of internal loop,
      // discovered by the watchdog:
      distance = GET_ABSOLUTE_X_DISTANCE(w,p) + GET_ABSOLUTE_Y_DISTANCE(w,p);
      if( far_distance < distance)
      {
         far_distance= distance;
         far_widget  = p;
      }

      // Watch-dog:
      if( loop_watch == p)
      {
         if( !far_widget || (far_widget==w))
            return;     // Internal closed loop
         p = far_widget;
         break;
      }

      // Increment watchdog
      if( inc_watch)
         loop_watch = *pfn_back_ptr(loop_watch);
      inc_watch = !inc_watch;


      if( p == w)
         return;     // Looped back to 'w' (logically w cannot happen)
   }

   // Set 'next' pointer:
   *(pfn_next_ptr(w)) = p;

   // Set neighbour's 'back' pointer to us:
   if( NULL == *(pfn_back_ptr(p)))
      *(pfn_back_ptr(p)) = w;
}

SsdWidget ssd_widget_select_nearest_neighbour(
      SsdWidget   w,       // Me
      SsdWidget   option_a,   // Option a (current?)
      SsdWidget   option_b)   // Option b (new?)
{
   int   max_width         = roadmap_canvas_width();
   int   max_height        = roadmap_canvas_height() - roadmap_bar_bottom_height();
   int   option_a_distance = GET_ABSOLUTE_X_DISTANCE(w,option_a) + GET_ABSOLUTE_Y_DISTANCE(w,option_a);
   int   option_b_distance = GET_ABSOLUTE_X_DISTANCE(w,option_b) + GET_ABSOLUTE_Y_DISTANCE(w,option_b);

   if( option_a_distance < option_b_distance)
      return option_a;

   return option_b;
}

// Consider other widget to be our nearest neighbour
// If it is suited to be a neighbour, set it as a neighbour
static void ssd_widget_consider_as_nieghbour( SsdWidget w, SsdWidget other)
{
   int   max_width            = roadmap_canvas_width();
   int   max_height           = roadmap_canvas_height() - roadmap_bar_bottom_height();
   int   absolute_x_distance  = GET_ABSOLUTE_X_DISTANCE(w,other);
   int   absolute_y_distance  = GET_ABSOLUTE_Y_DISTANCE(w,other);
   BOOL  x_axis               = (absolute_y_distance < absolute_x_distance);

   if( w == other)
      return;

   if( x_axis)
   {
      if( OBJECT_IS_TO_THE_LEFT(w,other))
         CONSIDER_A_NEW_NEIGHBOUR(left_tabstop,right_tabstop)
      else if( OBJECT_IS_TO_THE_RIGHT(w,other))
         CONSIDER_A_NEW_NEIGHBOUR(right_tabstop,left_tabstop)
   }
   else
   {
      if( OBJECT_IS_ABOVE(w,other))
         CONSIDER_A_NEW_NEIGHBOUR(tabstop_above,tabstop_below)
      else if( OBJECT_IS_BELOW(w,other))
         CONSIDER_A_NEW_NEIGHBOUR(tabstop_below,tabstop_above)
   }
}

SsdWidget*  get_back_left_ptr (SsdWidget w){ return &(w->right_tabstop);}
SsdWidget*  get_next_left_ptr (SsdWidget w){ return &(w->left_tabstop);}
SsdWidget*  get_back_right_ptr(SsdWidget w){ return &(w->left_tabstop);}
SsdWidget*  get_next_right_ptr(SsdWidget w){ return &(w->right_tabstop);}
SsdWidget*  get_back_above_ptr(SsdWidget w){ return &(w->tabstop_below);}
SsdWidget*  get_next_above_ptr(SsdWidget w){ return &(w->tabstop_above);}
SsdWidget*  get_back_below_ptr(SsdWidget w){ return &(w->tabstop_above);}
SsdWidget*  get_next_below_ptr(SsdWidget w){ return &(w->tabstop_below);}
// After left/right/above/below pointers were set, some corner widgets will not
// have anything to point at.
// Why?  For example - the top-left cornered widget does not have anything above it
//       or to the left of it.
// The next method will search for these 'orphan' pointers, and set them to point
// to other widgets:
void ssd_widget_sort_gui_tab_order__fix_orphan_pointers( SsdWidget first)
{
   SsdWidget p = first;

   do
   {
      ssd_widget_sort_gui_tab_order__fix_corners( p, get_back_left_ptr, get_next_left_ptr);
      ssd_widget_sort_gui_tab_order__fix_corners( p, get_back_right_ptr,   get_next_right_ptr);
      ssd_widget_sort_gui_tab_order__fix_corners( p, get_back_above_ptr,   get_next_above_ptr);
      ssd_widget_sort_gui_tab_order__fix_corners( p, get_back_below_ptr,   get_next_below_ptr);

      p = p->next_tabstop;

   }  while( p != first);
}

void ssd_widget_sort_gui_tab_order( SsdWidget first)
{
   SsdWidget   w = first;
   SsdWidget   next = NULL;

   if(   !first || !first->next_tabstop)
      return;

   do
   {
      next = w->next_tabstop;

      do
      {
         // Maybe neighbours?
         ssd_widget_consider_as_nieghbour(w,next);

         next = next->next_tabstop;

      }  while( next != w);

      w = w->next_tabstop;

   }  while( w != first);

   // See remarks above the next method for more info:
   ssd_widget_sort_gui_tab_order__fix_orphan_pointers( first);
}

void ssd_widget_reset_tab_order_recursive( SsdWidget w)

{
   SsdWidget next;

   w->prev_tabstop   = NULL;
   w->next_tabstop   = NULL;
   w->left_tabstop   = NULL;
   w->right_tabstop  = NULL;
   w->tabstop_above  = NULL;
   w->tabstop_below  = NULL;
   w->in_focus       = FALSE;

   next = w->children;
   while( next)
   {
      ssd_widget_reset_tab_order_recursive( next);
      next = next->next;
   }

   if( (NULL == w->parent) && w->next)
      ssd_widget_reset_tab_order_recursive( w->next);
}

void ssd_widget_reset_tab_order( SsdWidget head)
{
   if( head)
      ssd_widget_reset_tab_order_recursive( head);
}

void ssd_widget_sort_tab_order_recursive(
      void*       parent_dialog,
      SsdWidget   current_position,
      SsdWidget*  previous_tabstop,
      SsdWidget*  default_widget,
      SsdWidget*  first_tabstop,
      SsdWidget*  last_tabstop)
{
   SsdWidget next;

   current_position->parent_dialog = parent_dialog;

   if( (current_position->tab_stop) && !(SSD_WIDGET_HIDE & current_position->flags))
   {
      // Set first and last:
      if( NULL == (*first_tabstop))
         (*first_tabstop)  = current_position;
      (*last_tabstop) = current_position;

      // Tie-up the previous and the current:
      if( NULL == (*previous_tabstop))
         (*previous_tabstop)= current_position;
      else
      {
         (*previous_tabstop)->next_tabstop= current_position;
         current_position->prev_tabstop   = (*previous_tabstop);
         (*previous_tabstop)              = current_position;
      }
   }

   if( current_position->default_widget)
   {
      assert( NULL == (*default_widget)); // More then one default-widget in a construct?
      (*default_widget) = current_position;
   }

   next = current_position->children;
   while( next)
   {
      ssd_widget_sort_tab_order_recursive(
            parent_dialog,
            next,
            previous_tabstop,
            default_widget,
            first_tabstop,
            last_tabstop);

      next = next->next;
   }

   // Only for top-level widgets we need to check brothers.
   // Other widgets brothers were already covered by the children-enumaration of their parent
   if( (NULL == current_position->parent) && current_position->next)
      ssd_widget_sort_tab_order_recursive(
            parent_dialog,
            current_position->next,
            previous_tabstop,
            default_widget,
            first_tabstop,
            last_tabstop);
}

void switch_widgets_tab_order( SsdWidget a, SsdWidget b)
{
   struct ssd_widget a_data_with_b_pointers = *a;
   struct ssd_widget b_data_with_a_pointers = *b;

   a_data_with_b_pointers.prev_tabstop = b->prev_tabstop;
   a_data_with_b_pointers.next_tabstop = b->next_tabstop;
   b_data_with_a_pointers.prev_tabstop = a->prev_tabstop;
   b_data_with_a_pointers.next_tabstop = a->next_tabstop;

   *a = b_data_with_a_pointers;
   *b = a_data_with_b_pointers;
}

void fix_widget_tab_order_sequence(SsdWidget widget)
{
   SsdWidget w = widget->next_tabstop;

   while( w != widget)
   {
      if( w != w->next_tabstop)
      {
         int   my_seq      = w->tab_sequence;
         int   my_next_seq = w->next_tabstop->tab_sequence;
         int   w_seq       = w->tab_sequence;
         BOOL  wrong_order = FALSE;

         if( my_seq < my_next_seq)
         {
            if( (w_seq < my_next_seq) && (my_seq < w_seq))
               wrong_order = TRUE;
         }
         else
            if( w_seq < my_next_seq)
               wrong_order = TRUE;

         if( wrong_order)
         {
            assert(0 && "fix_tab_order_sequence() - unexpected order");
            switch_widgets_tab_order( w->next_tabstop, w);
         }
      }

      w = w->next_tabstop;
   };
}

void fix_tab_order_sequence( SsdWidget widget)
{
   SsdWidget w = widget->next_tabstop;

   while( w != widget)
   {
      fix_widget_tab_order_sequence( w);
      w = w->next_tabstop;
   };
}

SsdWidget ssd_widget_sort_tab_order(void* parent_dialog, SsdWidget head)
{
   SsdWidget   previous_tabstop  = NULL;
   SsdWidget   default_widget    = NULL;
   SsdWidget   first_tabstop     = NULL;
   SsdWidget   last_tabstop      = NULL;

   if( !head)
      return NULL;

   // Sort internal items:
   ssd_widget_sort_tab_order_recursive(parent_dialog,
         head,
         &previous_tabstop,
         &default_widget,
         &first_tabstop,
         &last_tabstop);

#ifdef _DEBUG
   // Check against bug in the code:
   if( (last_tabstop && !first_tabstop) || (!last_tabstop && first_tabstop))
   {
      assert( 0);
      return NULL;
   }
#endif   // _DEBUG

   // Found any?
   if(!last_tabstop)
      return NULL;   // No tab-stops found

   if( first_tabstop != last_tabstop)
   {
      // Make 'last' and 'first' point at each other:
      last_tabstop->next_tabstop = first_tabstop;
      first_tabstop->prev_tabstop= last_tabstop;

      ///[BOOKMARK]:[VERIFY]:[PAZ] - See if next code is needed:
      fix_tab_order_sequence( first_tabstop);
   }
if (!default_widget)
    default_widget = first_tabstop;
#ifndef TOUCH_SCREEN
   // Set first widget with focus:
   if(default_widget)
      ssd_widget_set_focus(default_widget);
   else
   {
      ssd_widget_set_focus(first_tabstop);

   }
#endif
   return default_widget;
}

BOOL ssd_widget_on_key_pressed( SsdWidget w, const char* utf8char, uint32_t flags)
{
   SsdWidget child;

   if(w->key_pressed && w->key_pressed( w, utf8char, flags))
      return TRUE;

   child =  w->children;
   while( child)
   {
      if( ssd_widget_on_key_pressed( child, utf8char, flags))
         return TRUE;

      child = child->next;
   }

   return FALSE;
}


int ssd_widget_set_right_softkey_text(SsdWidget widget, const char *value) {

   widget->right_softkey = value;

   switch (roadmap_softkeys_orientation ()) {

         case SOFT_KEYS_ON_BOTTOM:
               ssd_widget_set_value (widget, "right_title_text", "");
               if (value != NULL && *value)
                  return ssd_widget_set_value (widget, "right_softkey_text", value);
              break;
       case SOFT_KEYS_ON_RIGHT:
              if (widget->left_softkey != NULL)
                 ssd_widget_set_value (widget, "right_softkey_text", widget->left_softkey);
              if (value != NULL && *value)
                 return ssd_widget_set_value (widget, "right_title_text", value);
              break;
      default:
              return -1;
    }

    return 0;

}

int ssd_widget_set_left_softkey_text(SsdWidget widget, const char *value) {




   widget->left_softkey = value;

    switch (roadmap_softkeys_orientation ()) {

         case SOFT_KEYS_ON_BOTTOM:
               if (widget->right_softkey != NULL)
                  ssd_widget_set_value (widget, "right_softkey_text", widget->right_softkey);
               if (value != NULL && *value)
                  return ssd_widget_set_value (widget, "left_softkey_text", value);
              break;
       case SOFT_KEYS_ON_RIGHT:
              ssd_widget_set_value (widget, "left_softkey_text", "");
                 if (value != NULL && *value)
                    return ssd_widget_set_value (widget, "right_softkey_text", value);
              break;
      default:
              return -1;
     }

    return 0;
}

void ssd_widget_set_left_softkey_callback (SsdWidget widget, SsdSoftKeyCallback callback) {

   widget->left_softkey_callback = callback;
}

void ssd_widget_set_right_softkey_callback (SsdWidget widget, SsdSoftKeyCallback callback) {
   widget->right_softkey_callback = callback;
}
