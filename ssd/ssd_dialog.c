/* ssd_dialog.c - small screen devices Widgets (designed for touchscreens)
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
 *   See ssd_dialog.h
 */

#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include "roadmap.h"
#include "roadmap_types.h"
#include "roadmap_lang.h"
#include "roadmap_main.h"
#include "roadmap_keyboard.h"
#include "roadmap_pointer.h"
#include "roadmap_start.h"
#include "roadmap_res.h"
#include "roadmap_screen.h"
#include "roadmap_bar.h"
#include "roadmap_softkeys.h"

#include "ssd_widget.h"
#include "ssd_container.h"
#include "ssd_entry.h"
#include "ssd_button.h"
#include "ssd_dialog.h"

struct ssd_dialog_item;
typedef struct ssd_dialog_item *SsdDialog;

struct ssd_dialog_item {

   struct ssd_dialog_item* next;
   struct ssd_dialog_item* activated_prev;
   char*                   name;
   void*                   context;                // References a caller-specific context.
   PFN_ON_DIALOG_CLOSED    on_dialog_closed;       // Called before hide
   SsdWidget               container;
   SsdWidget               in_focus;               // Widget currently in focus
   SsdWidget               in_focus_default;       // Widget currently in focus
   BOOL                    tab_order_sorted;       // Did we sort the tab-order already?
   BOOL                    use_gui_tab_order;
   BOOL                    gui_tab_order_sorted;   // Did we sort the GUI tab-order already?
};

static SsdDialog RoadMapDialogWindows = NULL;
static SsdDialog RoadMapDialogCurrent = NULL;

BOOL ssd_dialog_is_currently_active()
{ return (NULL != RoadMapDialogCurrent);}

void * ssd_dialog_get_current_data(){
    return RoadMapDialogCurrent->container->data;
}


BOOL ssd_dialog_is_currently_vertical(){

    if (NULL == RoadMapDialogCurrent)
        return FALSE;
    if (RoadMapDialogCurrent->container->flags & SSD_DIALOG_VERTICAL)
        return TRUE;
    else
        return FALSE;
}

static RoadMapGuiPoint LastPointerPoint;

static int RoadMapScreenFrozen = 0;
static int RoadMapDialogKeyEnabled = 0;

extern void ssd_tabcontrol_move_tab_left ( SsdWidget dialog);
extern void ssd_tabcontrol_move_tab_right( SsdWidget dialog);

static SsdDialog ssd_dialog_get (const char *name) {

   SsdDialog child;

   child = RoadMapDialogWindows;

   while (child != NULL) {
      if (strcmp (child->name, name) == 0) {
         return child;
      }
      child = child->next;
   }

   return NULL;
}


static void ssd_dialog_enable_callback (void) {
   RoadMapDialogKeyEnabled = 1;
   roadmap_main_remove_periodic (ssd_dialog_enable_callback);
}


static void ssd_dialog_disable_key (void) {
   RoadMapDialogKeyEnabled = 0;
   roadmap_main_set_periodic (350, ssd_dialog_enable_callback);
}


static int ssd_dialog_pressed (RoadMapGuiPoint *point) {
   SsdWidget container = RoadMapDialogCurrent->container;
   if (!ssd_widget_find_by_pos (container, point)) {
      LastPointerPoint.x = -1;
      return 0;
   }

   if (!RoadMapDialogKeyEnabled) {
      LastPointerPoint.x = -1;
      return 1;
   }

   if (RoadMapDialogCurrent->container->drag_start){
    ssd_widget_pointer_down (RoadMapDialogCurrent->container, point);
    roadmap_screen_redraw ();
    LastPointerPoint = *point;
    return 0;
   }

   LastPointerPoint = *point;
   ssd_widget_pointer_down (RoadMapDialogCurrent->container, point);
   roadmap_screen_redraw ();
   return 1;
}

static int ssd_dialog_short_click (RoadMapGuiPoint *point) {
   int res;
   if (LastPointerPoint.x < 0) {
      SsdWidget container = RoadMapDialogCurrent->container;
      if (ssd_widget_find_by_pos (container, point)) {
         //return 1;
         LastPointerPoint = *point;
      } else {
         return 0;
      }
   }

   res = ssd_widget_short_click (RoadMapDialogCurrent->container, &LastPointerPoint);

   roadmap_screen_redraw ();

   return 1;
}

static int ssd_dialog_long_click (RoadMapGuiPoint *point) {
   if (!LastPointerPoint.x < 0) {
      SsdWidget container = RoadMapDialogCurrent->container;
      if (ssd_widget_find_by_pos (container, point)) {
         return 1;
      } else {
         return 0;
      }
   }
   ssd_widget_long_click (RoadMapDialogCurrent->container, &LastPointerPoint);
   roadmap_screen_redraw ();

   return 1;
}

static void append_child (SsdWidget child) {

   ssd_widget_add (RoadMapDialogWindows->container, child);
}

BOOL ssd_dialog_set_dialog_focus( SsdDialog dialog, SsdWidget new_focus)
{
   SsdWidget last_focus;
   SsdWidget w;

   if( new_focus && !new_focus->tab_stop)
   {
      assert( 0 && "ssd_dialog_set_dialog_focus() - Invalid input");
      return FALSE;
   }


    w = ssd_widget_get(new_focus,"focus_image");
    if (w != NULL)
        ssd_widget_show(w->children);

   // Setting the same one twice...
   if (dialog->in_focus == new_focus)
      return TRUE;

   if( new_focus && (dialog != new_focus->parent_dialog))
   {
      assert( 0 && "ssd_dialog_set_dialog_focus() - widget does not belong to this dialog");
      return FALSE;
   }

   last_focus = dialog->in_focus;
   if (dialog->in_focus)
   {
      ssd_widget_loose_focus(dialog->in_focus);
      dialog->in_focus = NULL;
   }

   if( !new_focus)
      return TRUE;

   if( !ssd_widget_set_focus(new_focus))
   {
      assert(0);
      if( ssd_widget_set_focus( last_focus))
         dialog->in_focus = last_focus;
      return FALSE;
   }

   dialog->in_focus = new_focus;

   return TRUE;
}

SsdWidget ssd_dialog_get_focus()
{
   if( RoadMapDialogCurrent && RoadMapDialogCurrent->in_focus)
      return RoadMapDialogCurrent->in_focus;
   return NULL;
}

BOOL ssd_dialog_set_focus( SsdWidget new_focus)
{
   if( !RoadMapDialogCurrent)
   {
      assert( 0 && "ssd_dialog_set_focus() - Invalid state");
      return FALSE;
   }

   return ssd_dialog_set_dialog_focus( RoadMapDialogCurrent, new_focus);
}


void ssd_dialog_move_focus (int direction) {
   assert(RoadMapDialogCurrent);

#ifdef TOUCH_SCREEN
   if (!RoadMapDialogCurrent->in_focus) {
      ssd_dialog_set_dialog_focus(RoadMapDialogCurrent,
                                  RoadMapDialogCurrent->in_focus_default);
      return;
   }
#endif
   RoadMapDialogCurrent->in_focus =
      ssd_widget_move_focus(RoadMapDialogCurrent->in_focus, direction);

}


SsdWidget ssd_dialog_new (const char *name, const char *title,
                          PFN_ON_DIALOG_CLOSED on_dialog_closed, int flags)
{
   SsdDialog   dialog;

   int width   = SSD_MAX_SIZE;
   int height  = SSD_MAX_SIZE;

   dialog = (SsdDialog)calloc( sizeof( struct ssd_dialog_item), 1);
   roadmap_check_allocated(dialog);

   dialog->name                  = strdup(name);
   dialog->on_dialog_closed      = on_dialog_closed;
   dialog->in_focus              = NULL;
   dialog->tab_order_sorted      = FALSE;
   dialog->gui_tab_order_sorted  = FALSE;
   dialog->use_gui_tab_order     = (SSD_DIALOG_GUI_TAB_ORDER & flags)? TRUE: FALSE;

   if (flags & SSD_DIALOG_FLOAT) {
    if (flags & SSD_DIALOG_VERTICAL)
        width = SSD_MIN_SIZE;
    else
        width = SSD_MAX_SIZE;
      height = SSD_MIN_SIZE;
   }

   dialog->container = ssd_container_new (name, title, width, height, flags);
   dialog->next      = RoadMapDialogWindows;

   RoadMapDialogWindows = dialog;

   ssd_widget_set_right_softkey_text(dialog->container, roadmap_lang_get("Back_key"));
   ssd_widget_set_left_softkey_text (dialog->container, roadmap_lang_get("Exit_key"));

   return dialog->container;

}

void ssd_dialog_sort_tab_order_by_gui_position()
{
   if( !RoadMapDialogCurrent || !RoadMapDialogCurrent->use_gui_tab_order || RoadMapDialogCurrent->gui_tab_order_sorted)
      return;

   ssd_widget_sort_gui_tab_order(RoadMapDialogCurrent->in_focus_default);
   RoadMapDialogCurrent->gui_tab_order_sorted = TRUE;
}

void ssd_dialog_redraw_screen_recursive( SsdDialog Dialog)
{
   RoadMapGuiRect rect;
   SsdWidget      container = Dialog->container;

   if( (SSD_DIALOG_FLOAT & container->flags) && Dialog->next)
      ssd_dialog_redraw_screen_recursive( Dialog->next);

   rect.minx = 0;
   rect.miny = 0;
   rect.maxx = roadmap_canvas_width() - 1;
   rect.maxy = roadmap_canvas_height() - 1;

   ssd_widget_reset_cache  ( container);
   ssd_widget_draw         ( container, &rect, 0);
}

void ssd_dialog_redraw_screen()
{
   if( !RoadMapDialogCurrent)
      return;

   ssd_dialog_redraw_screen_recursive( RoadMapDialogCurrent);
}

void ssd_dialog_draw (void) {

   if (!RoadMapDialogCurrent) {
      return;

   } else {
      RoadMapGuiRect rect;
      rect.minx = 0;
      rect.miny = 0;
      rect.maxx = roadmap_canvas_width() - 1;
      rect.maxy = roadmap_canvas_height() - 1 - roadmap_bar_bottom_height() ;

      ssd_widget_reset_cache (RoadMapDialogCurrent->container);
      ssd_widget_draw (RoadMapDialogCurrent->container, &rect, 0);

      roadmap_bar_draw_objects();
      roadmap_canvas_refresh ();
      ssd_dialog_sort_tab_order_by_gui_position();
   }
}


void ssd_dialog_new_entry (const char *name, const char *value,
                           int flags, SsdCallback callback) {

   SsdWidget child = ssd_entry_new (name, value, flags, 0);
   append_child (child);

   ssd_widget_set_callback (child, callback);
}


SsdWidget ssd_dialog_new_button (const char *name, const char *value,
                                 const char **bitmaps, int num_bitmaps,
                                 int flags, SsdCallback callback) {

   SsdWidget child =
      ssd_button_new (name, value, bitmaps, num_bitmaps, flags, callback);
   append_child (child);

   return child;
}

void ssd_dialog_change_button(const char *name, const char **bitmaps, int num_bitmaps){
    SsdWidget button = ssd_widget_get(RoadMapDialogCurrent->container, name);
    if (button)
        ssd_button_change_icon(button, bitmaps, num_bitmaps);
}

void ssd_dialog_new_line (void) {

   SsdWidget last = RoadMapDialogWindows->container->children;

   while (last->next) last=last->next;

   last->flags |= SSD_END_ROW;
}

static BOOL OnKeyPressed( const char* utf8char, uint32_t flags) {
   BOOL        key_handled = TRUE;
   SsdWidget   in_focus    = NULL;

   if( !RoadMapDialogCurrent)
      return FALSE;

   // Let the control handle the key:
   in_focus = RoadMapDialogCurrent->in_focus;
   if( in_focus && ssd_widget_on_key_pressed (in_focus, utf8char, flags)) {
      roadmap_screen_redraw();
      return TRUE;
   }

   // The control did not handle the key...
   //    Supply general handling for virtual keys:
   if( KEYBOARD_VIRTUAL_KEY & flags)
   {
      SsdWidget container = RoadMapDialogCurrent->container;
      switch( *utf8char) {
         case VK_Back:
            ssd_dialog_hide_current(dec_cancel);
            break;

         case VK_Arrow_left:
            if( SSD_TAB_CONTROL & RoadMapDialogCurrent->container->flags)
               ssd_tabcontrol_move_tab_left( RoadMapDialogCurrent->container);
            else
               ssd_dialog_move_focus(FOCUS_LEFT);
            break;

         case VK_Arrow_up:
            ssd_dialog_move_focus(FOCUS_UP);
            break;

         case VK_Arrow_right:
            if( SSD_TAB_CONTROL & RoadMapDialogCurrent->container->flags)
               ssd_tabcontrol_move_tab_right( RoadMapDialogCurrent->container);
            else
               ssd_dialog_move_focus( FOCUS_RIGHT);
            break;

         case VK_Arrow_down:
            ssd_dialog_move_focus(FOCUS_DOWN);
            break;

         case VK_Softkey_right:
            if (container->right_softkey_callback != NULL)
               container->right_softkey_callback(container, container->name, container->context);
            else
               ssd_dialog_hide_current(dec_cancel);
            break;

         case VK_Softkey_left:
            if (container->left_softkey_callback != NULL)
               container->left_softkey_callback(container, container->name, container->context);
            else
               ssd_dialog_hide_all(dec_cancel);
            break;

         default:
            key_handled = FALSE;
      }

   }
   else
   {
      assert(utf8char);
      assert(*utf8char);

      // Other special keys:
      if( KEYBOARD_ASCII & flags)
      {
         switch(*utf8char)
         {
            case ESCAPE_KEY:
               ssd_dialog_hide_current(dec_cancel);
               break;

            case TAB_KEY:
               ssd_dialog_move_focus(FOCUS_FORWARD);
               break;

            default:
               key_handled = FALSE;
         }
      }
   }

   if( key_handled)
      roadmap_screen_redraw ();

   return key_handled;
}

void ssd_dialog_send_keyboard_event( const char* utf8char, uint32_t flags)
{ OnKeyPressed( utf8char, flags);}

void ssd_dialog_sort_tab_order( SsdDialog dialog)
{
   if( !dialog || dialog->tab_order_sorted)
      return;

   dialog->in_focus_default = ssd_widget_sort_tab_order(dialog, dialog->container);
#ifndef TOUCH_SCREEN
   dialog->in_focus = dialog->in_focus_default;
#endif
   dialog->tab_order_sorted = TRUE;
}

static void left_softkey_callback(void){
    SsdDialog   current  = RoadMapDialogCurrent;

    if (current == NULL)
        return;

    if (current->container->left_softkey_callback)
        (*current->container->left_softkey_callback)(current->container, "", current->context);
    else
         ssd_dialog_hide_all(dec_cancel);

}

static void right_softkey_callback(void){
    SsdDialog   current  = RoadMapDialogCurrent;

    if (current == NULL)
        return;

    if (current->container->right_softkey_callback)
        (*current->container->right_softkey_callback)(current->container, "", current->context);
    else
         ssd_dialog_hide_current(dec_cancel);
}

static void set_softkeys(SsdDialog d){
    static Softkey right,left;


    if (d->container->right_softkey)
        strcpy(right.text, d->container->right_softkey);
    else
        strcpy(right.text, "Back_key");

    right.callback = right_softkey_callback;
    roadmap_softkeys_set_right_soft_key(d->name, &right);

    if (d->container->left_softkey)
        strcpy(left.text, d->container->left_softkey);
    else
        strcpy(left.text, "Exit_key");
    left.callback = left_softkey_callback;
    roadmap_softkeys_set_left_soft_key(d->name, &left);
}

static void hide_softkeys(SsdDialog d){
    roadmap_softkeys_remove_right_soft_key(d->name);
    roadmap_softkeys_remove_left_soft_key(d->name);
}

int ssd_dialog_drag_start (RoadMapGuiPoint *point) {
   SsdDialog dialog = RoadMapDialogCurrent;
   if (dialog == NULL){
    return 0;
   }

   if (dialog->container->drag_start)
        return (*dialog->container->drag_start)(dialog->container, point);

   return 0;
}

int ssd_dialog_drag_end (RoadMapGuiPoint *point) {

   SsdDialog dialog = RoadMapDialogCurrent;
   if (dialog == NULL)
    return 0;

   if (dialog->container->drag_end)
    return (*dialog->container->drag_end)(dialog->container, point);

   return 0;
}

int ssd_dialog_drag_motion (RoadMapGuiPoint *point) {
   SsdDialog dialog = RoadMapDialogCurrent;

   if (dialog == NULL)
    return 0;

   if (dialog->container->drag_motion)
    return (*dialog->container->drag_motion)(dialog->container, point);

   return 0;
}

void ssd_dialog_set_drag(SsdWidget dialog, SsdWidget l){

#ifdef IPHONE
      dialog->drag_start = l->drag_start;
      dialog->drag_end = l->drag_end;
      dialog->drag_motion = l->drag_motion;

      roadmap_pointer_register_drag_start
                (&ssd_dialog_drag_start,POINTER_HIGHEST );

      roadmap_pointer_register_drag_end
                (&ssd_dialog_drag_end,POINTER_HIGHEST);

      roadmap_pointer_register_drag_motion
                (&ssd_dialog_drag_motion,POINTER_HIGHEST);
#endif
}

SsdWidget ssd_dialog_activate (const char *name, void *context) {

   SsdDialog   prev     = NULL;
   SsdDialog   current  = RoadMapDialogCurrent;
   SsdDialog   dialog   = ssd_dialog_get (name);

   if (!dialog) {
      return NULL; /* Tell the caller this is a new, undefined, dialog. */
   }

   while (current && strcmp(current->name, name)) {
      prev = current;
      current = current->activated_prev;
   }

   if (current) {
      if (prev) {
         prev->activated_prev = current->activated_prev;
         current->activated_prev = RoadMapDialogCurrent;
         RoadMapDialogCurrent = current;
      }
      return current->container;
   }

   dialog->context = context;

   dialog->activated_prev = RoadMapDialogCurrent;

   if (!RoadMapDialogCurrent) {
      roadmap_keyboard_register_to_event__key_pressed( OnKeyPressed);
      /* Grab pointer hooks */
      roadmap_pointer_register_pressed
         (ssd_dialog_pressed, POINTER_HIGHEST);
      roadmap_pointer_register_short_click
         (ssd_dialog_short_click, POINTER_HIGHEST);
      roadmap_pointer_register_long_click
         (ssd_dialog_long_click, POINTER_HIGHEST);

   }

   RoadMapDialogCurrent = dialog;
   set_softkeys(dialog);

   // Sort tab-order:
   ssd_dialog_sort_tab_order( dialog);

   // Reset focus
#ifdef TOUCH_SCREEN
   ssd_dialog_set_dialog_focus(dialog, NULL);
#else
   ssd_dialog_set_dialog_focus(dialog, dialog->in_focus_default);
#endif

   if (!RoadMapScreenFrozen &&
      !(dialog->container->flags & (SSD_DIALOG_FLOAT|SSD_DIALOG_TRANSPARENT))) {
      roadmap_start_screen_refresh (0);
      RoadMapScreenFrozen = 1;
   }

   ssd_dialog_disable_key ();
   return dialog->container; /* Tell the caller the dialog already exists. */
}


void ssd_dialog_hide (const char *name, int exit_code) {

   SsdDialog prev = NULL;
   SsdDialog dialog = RoadMapDialogCurrent;

   while (dialog && strcmp(dialog->name, name)) {
      prev = dialog;
      dialog = dialog->activated_prev;
   }

   if (!dialog) {
      return;
   }


   if (prev == NULL) {
      RoadMapDialogCurrent = RoadMapDialogCurrent->activated_prev;
   } else {
      prev->activated_prev = dialog->activated_prev;
   }

   if (RoadMapDialogCurrent) {
      ssd_dialog_disable_key ();
   } else {
      roadmap_pointer_unregister_pressed     (ssd_dialog_pressed);
      roadmap_pointer_unregister_short_click (ssd_dialog_short_click);
      roadmap_pointer_unregister_long_click  (ssd_dialog_long_click);
      roadmap_keyboard_unregister_from_event__key_pressed(OnKeyPressed);

   }

      if (dialog->container->drag_start)
         roadmap_pointer_unregister_drag_start
                (&ssd_dialog_drag_start);
      if (dialog->container->drag_end)
        roadmap_pointer_unregister_drag_end
                (&ssd_dialog_drag_end);
       if (dialog->container->drag_motion)
        roadmap_pointer_unregister_drag_motion
                (&ssd_dialog_drag_motion);

   hide_softkeys(dialog);
   if (dialog->on_dialog_closed) {
      dialog->on_dialog_closed( exit_code, dialog->context);
   }

   if (RoadMapScreenFrozen) {
      dialog = RoadMapDialogCurrent;
      while (dialog) {
         if ( !(dialog->container->flags &
                (SSD_DIALOG_FLOAT|SSD_DIALOG_TRANSPARENT))) {
            ssd_dialog_draw ();
            return;
         }
         dialog = dialog->activated_prev;
      }
   }

    roadmap_input_type_set_mode( inputtype_numeric );
    RoadMapScreenFrozen = 0;
   roadmap_start_screen_refresh (1);
}


void ssd_dialog_hide_current( int exit_code) {

   if (!RoadMapDialogCurrent) {
      roadmap_log (ROADMAP_FATAL,
         "Trying to hide a dialog, but no active dialogs exist");
   }

   ssd_dialog_hide (RoadMapDialogCurrent->name, exit_code);
}

void  ssd_dialog_hide_all(int exit_code)
{
   while( RoadMapDialogCurrent)
      ssd_dialog_hide( RoadMapDialogCurrent->name, exit_code);
}

const char *ssd_dialog_get_value (const char *name) {

   return ssd_widget_get_value (RoadMapDialogCurrent->container, name);
}


const void *ssd_dialog_get_data (const char *name) {

   return ssd_widget_get_data (RoadMapDialogCurrent->container, name);
}


int ssd_dialog_set_value (const char *name, const char *value) {

   return ssd_widget_set_value (RoadMapDialogCurrent->container, name, value);
}


int ssd_dialog_set_data  (const char *name, const void *value) {
   return ssd_widget_set_data (RoadMapDialogCurrent->container, name, value);
}


void ssd_dialog_set_callback( PFN_ON_DIALOG_CLOSED on_dialog_closed) {

   if (!RoadMapDialogCurrent) {
      roadmap_log (ROADMAP_FATAL,
         "Trying to set a dialog callback, but no active dialogs exist");
   }

   RoadMapDialogCurrent->on_dialog_closed = on_dialog_closed;
}


void *ssd_dialog_context (void) {
   if (!RoadMapDialogCurrent) {
      roadmap_log (ROADMAP_FATAL,
         "Trying to get dialog context, but no active dialogs exist");
   }

   return RoadMapDialogCurrent->context;
}

// Tab-Order:  Force a re-sort, even if was already sorted
void ssd_dialog_resort_tab_order()
{
   SsdDialog dialog = RoadMapDialogCurrent;
   if( !dialog)
      return;

   // Un-sort:
   ssd_widget_reset_tab_order( dialog->container);

   // Undo the 'was already sorted' flags:
   dialog->tab_order_sorted      = FALSE;
   dialog->gui_tab_order_sorted  = FALSE;

   // Sort:
   ssd_dialog_sort_tab_order( dialog);
   ssd_dialog_sort_tab_order_by_gui_position();
}

void ssd_dialog_wait (void) {
   while (RoadMapDialogCurrent) {
      roadmap_main_flush ();
   }
}
