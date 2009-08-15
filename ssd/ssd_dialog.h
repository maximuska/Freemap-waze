/* ssd_dialog.h - small screen devices Widgets (designed for touchscreens)
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
 */

#ifndef __SSD_DIALOG_H_
#define __SSD_DIALOG_H_

#include "../roadmap.h"
#include "../roadmap_canvas.h"
#include "ssd_widget.h"

typedef enum tagEDialogExitCode {
   dec_close,

   dec_ok,
   dec_cancel,

   dec_yes,
   dec_no,

   dec_commit,
   dec_abort,
   dec_retry,
   dec_ignore

}  EDialogExitCode;


typedef void(*PFN_ON_DIALOG_CLOSED)       (int exit_code, void* context);
typedef void(*PFN_ON_INPUT_DIALOG_CLOSED) (int exit_code, const char* input,
                                           void* context);
typedef void(*PFN_ON_INPUT_CHANGED)       (const char* new_text, void* context);

SsdWidget   ssd_dialog_new (  const char*          name,
                              const char*          title,
                              PFN_ON_DIALOG_CLOSED on_dialog_closed,
                              int                  flags);

SsdWidget ssd_dialog_activate (const char *name, void *context);
void ssd_dialog_hide (const char *name, int exit_code);
void ssd_dialog_new_line (void);
void ssd_dialog_draw (void);
void ssd_dialog_redraw_screen();
void ssd_dialog_new_entry (const char *name, const char *value,
                           int flags, SsdCallback callback);

SsdWidget ssd_dialog_new_button (const char *name, const char *value,
                                 const char **bitmaps, int num_bitmaps,
                                 int flags, SsdCallback callback);
void *ssd_dialog_get_current_data(void);
const char* ssd_dialog_get_value (const char *name);
const void* ssd_dialog_get_data  (const char *name);
int         ssd_dialog_set_value (const char *name, const char *value);
int         ssd_dialog_set_data  (const char *name, const void *value);
BOOL        ssd_dialog_set_focus(SsdWidget new_focus);
SsdWidget   ssd_dialog_get_focus();
void*       ssd_dialog_context (void);
void        ssd_dialog_hide_current (int exit_code);
void        ssd_dialog_hide_all (int exit_code);
void        ssd_dialog_set_callback (PFN_ON_DIALOG_CLOSED on_dialog_closed);
void        ssd_dialog_resort_tab_order ();
BOOL        ssd_dialog_is_currently_active ();
BOOL        ssd_dialog_is_currently_vertical();
void        ssd_dialog_move_focus( int direction);
void        ssd_dialog_send_keyboard_event( const char* utf8char, uint32_t flags);

int ssd_dialog_drag_start (RoadMapGuiPoint *point) ;
int ssd_dialog_drag_end (RoadMapGuiPoint *point) ;
int ssd_dialog_drag_motion (RoadMapGuiPoint *point);
void ssd_dialog_change_button(const char *name, const char **bitmaps, int num_bitmaps);

void ssd_dialog_set_drag(SsdWidget dialog, SsdWidget l);
#endif // __SSD_DIALOG_H_

