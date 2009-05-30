/*
 * LICENSE:
 *
 *   Copyright 2008 PazO
 *
 *   RoadMap is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License V2 as published by
 *   the Free Software Foundation.
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


#ifndef __SSD_CONTEXT_MENU_DEFS_H__
#define __SSD_CONTEXT_MENU_DEFS_H__

#include "ssd_widget.h"

#define  CONTEXT_MENU_MIN_ITEMS_COUNT        ( 1)
#define  CONTEXT_MENU_MAX_ITEMS_COUNT        (16)
#define  CONTEXT_MENU_LABEL_MAX_SIZE         (32)

// Menu item flags:
#define  CONTEXT_MENU_FLAG_NORMAL            (0x00)
#define  CONTEXT_MENU_FLAG_POPUP             (0x01)
#define  CONTEXT_MENU_FLAG_HIDDEN            (0x02)
#define  CONTEXT_MENU_FLAG_SEPERATOR         (0x04)

typedef struct tag_RoadMapAction
{
   const char *name;
   const char *label_long;
   const char *label_short;
   const char *label_terse;
   const char *tip;

   RoadMapCallback callback;

}  RoadMapAction;

// Forward declaration
//    Popup-menu descriptor
struct tag_ssd_cm_popup_info;
typedef struct tag_ssd_cm_popup_info* ssd_cm_popup_info_ptr;   

// ITEM:
typedef struct tag_ssd_cm_item
{
   const char*          label;
   const char* 			icon;
   unsigned char        flags;   // CONTEXT_MENU_FLAG_...
   SsdWidget            row;     // Internal use
   const RoadMapAction* action;
   
   union
   {
      int                     id;
      ssd_cm_popup_info_ptr   popup;   // (flags & CONTEXT_MENU_FLAG_POPUP)
   };

}     ssd_cm_item, *ssd_cm_item_ptr;
void  ssd_cm_item_show( ssd_cm_item_ptr item);
void  ssd_cm_item_hide( ssd_cm_item_ptr item);


// MENU:
typedef struct tag_ssd_contextmenu
{
   ssd_cm_item_ptr   item;             // Array of items
   int               item_count;
   int               item_selected;    // Internal use
   
}     ssd_contextmenu, *ssd_contextmenu_ptr;
ssd_contextmenu_ptr  
      ssd_contextmenu_clone(     ssd_contextmenu_ptr  _this,
                                 BOOL                 alloc_labels);
void  ssd_contextmenu_delete(    ssd_contextmenu_ptr  _this, 
                                 BOOL                 delete_labels);
void  ssd_contextmenu_show_item( ssd_contextmenu_ptr  _this, 
                                 int                  item_id,
                                 BOOL                 show,       // Or hide
                                 BOOL                 recursive); // Into nested popups
void  ssd_contextmenu_show_item__by_action_name( 
                                 ssd_contextmenu_ptr  _this, 
                                 const char*          name,       // NOTE: this is not the label, it is the name from the x.menu file
                                 BOOL                 show,       // Or hide
                                 BOOL                 recursive); // Into nested popups


// POPUP:
typedef struct tag_ssd_cm_popup_info
{
   ssd_contextmenu_ptr  menu;       // Nested context menu
   SsdWidget            container;  // Internal use

}  ssd_cm_popup_info;
   

#define  SSD_CM_INIT_POPUP_INFO(_menu_)               \
            {&_menu_,NULL}
            
#define  SSD_CM_INIT_ITEM(_label_,_id_)               \
            {_label_,NULL, CONTEXT_MENU_FLAG_NORMAL,NULL,NULL,{_id_}}
            
#define  SSD_CM_INIT_POPUP(_label_,_popup_)           \
            {_label_, NULL,CONTEXT_MENU_FLAG_POPUP,NULL,NULL,(int)&_popup_}
            
#define  SSD_CM_INIT_MENU(_items_)                    \
            {_items_,sizeof(_items_)/sizeof(ssd_cm_item),0}

#endif // __SSD_CONTEXT_MENU_DEFS_H__
