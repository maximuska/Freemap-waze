
#include "../ssd/ssd_contextmenu.h"

static ssd_cm_item   popup_items_L5[] = 
{
   SSD_CM_INIT_ITEM( "L-5 - Item", 2),
   SSD_CM_INIT_ITEM( "L-5 - Item", 2),
   SSD_CM_INIT_ITEM( "L-5 - Item", 2),
   SSD_CM_INIT_ITEM( "L-5 - Item", 2),
   SSD_CM_INIT_ITEM( "L-5 - Item", 2),
   SSD_CM_INIT_ITEM( "L-5 - Item", 2),
   SSD_CM_INIT_ITEM( "L-5 - Item", 2),
   SSD_CM_INIT_ITEM( "L-5 - Item", 2),
   SSD_CM_INIT_ITEM( "L-5 - Item", 2),
};

static ssd_contextmenu     popup_menu_L5 = SSD_CM_INIT_MENU( popup_items_L5);
static ssd_cm_popup_info   popup_info_L5 = SSD_CM_INIT_POPUP_INFO(popup_menu_L5);

static ssd_cm_item   popup_items_L4[] = 
{
   SSD_CM_INIT_ITEM  ( "L-4 - Item", 2),
   SSD_CM_INIT_POPUP ( "L-4 - Popup", popup_info_L5),
   SSD_CM_INIT_ITEM  ( "L-4 - Item", 2),
   SSD_CM_INIT_ITEM  ( "L-4 - Item", 2),
   SSD_CM_INIT_ITEM  ( "L-4 - Item", 2),
   SSD_CM_INIT_ITEM  ( "L-4 - Item", 2),
   SSD_CM_INIT_ITEM  ( "L-4 - Item", 2),
   SSD_CM_INIT_ITEM  ( "L-4 - Item", 2),
   SSD_CM_INIT_ITEM  ( "L-4 - Item", 2),
};

static ssd_contextmenu     popup_menu_L4 = SSD_CM_INIT_MENU( popup_items_L4);
static ssd_cm_popup_info   popup_info_L4 = SSD_CM_INIT_POPUP_INFO(popup_menu_L4);

static ssd_cm_item   popup_items_L3[] = 
{
   SSD_CM_INIT_ITEM  ( "L-3 - Item", 2),
   SSD_CM_INIT_ITEM  ( "L-3 - Item", 2),
   SSD_CM_INIT_ITEM  ( "L-3 - Item", 2),
   SSD_CM_INIT_ITEM  ( "L-3 - Item", 2),
   SSD_CM_INIT_POPUP ( "L-3 - Popup", popup_info_L4),
   SSD_CM_INIT_ITEM  ( "L-3 - Item", 2),
   SSD_CM_INIT_ITEM  ( "L-3 - ItemL-3 - ItemL-3 - ItemL-3", 2),
   SSD_CM_INIT_ITEM  ( "L-3 - Item", 2),
   SSD_CM_INIT_ITEM  ( "L-3 - Item", 2),
};

static ssd_contextmenu     popup_menu_L3 = SSD_CM_INIT_MENU( popup_items_L3);
static ssd_cm_popup_info   popup_info_L3 = SSD_CM_INIT_POPUP_INFO(popup_menu_L3);

static ssd_cm_item   popup_items_L2a[] = 
{
   SSD_CM_INIT_ITEM  ( "L-2a - Item", 2),
   SSD_CM_INIT_ITEM  ( "L-2a - Item", 2),
   SSD_CM_INIT_POPUP ( "L-2a - Popup", popup_info_L3),
   SSD_CM_INIT_ITEM  ( "L-2a - Item", 2),
   SSD_CM_INIT_ITEM  ( "L-2a - Item", 2),
   SSD_CM_INIT_ITEM  ( "L-2a - Item", 2),
};

static ssd_cm_item   popup_items_L2b[] = 
{
   SSD_CM_INIT_ITEM( "L-2b - Item", 2),
   SSD_CM_INIT_ITEM( "L-2b - Item", 2),
   SSD_CM_INIT_ITEM( "L-2b - Item", 2),
   SSD_CM_INIT_ITEM( "L-2b - Item", 2),
   SSD_CM_INIT_ITEM( "L-2b - Item", 2),
   SSD_CM_INIT_ITEM( "L-2b - Item", 2),
};

static ssd_cm_item   popup_items_L2c[] = 
{
   SSD_CM_INIT_ITEM( "L-2c - Item", 2),
   SSD_CM_INIT_ITEM( "L-2c - Item", 2),
   SSD_CM_INIT_ITEM( "L-2c - Item", 2),
   SSD_CM_INIT_ITEM( "L-2c - Item", 2),
   SSD_CM_INIT_ITEM( "L-2c - Item", 2),
   SSD_CM_INIT_ITEM( "L-2c - Item", 2),
};

static ssd_contextmenu     popup_menu_L2a = SSD_CM_INIT_MENU( popup_items_L2a);
static ssd_cm_popup_info   popup_info_L2a = SSD_CM_INIT_POPUP_INFO(popup_menu_L2a);
static ssd_contextmenu     popup_menu_L2b = SSD_CM_INIT_MENU( popup_items_L2b);
static ssd_cm_popup_info   popup_info_L2b = SSD_CM_INIT_POPUP_INFO(popup_menu_L2b);
static ssd_contextmenu     popup_menu_L2c = SSD_CM_INIT_MENU( popup_items_L2c);
static ssd_cm_popup_info   popup_info_L2c = SSD_CM_INIT_POPUP_INFO(popup_menu_L2c);

static ssd_cm_item popup_items_L1[] = 
{
   SSD_CM_INIT_POPUP ( "L-1 - Popup", popup_info_L2a),
   SSD_CM_INIT_ITEM  ( "L-1 - Item",   1),
   SSD_CM_INIT_ITEM  ( "L-1 - Item",   1),
   SSD_CM_INIT_ITEM  ( "L-1 - Item",   1),
   SSD_CM_INIT_ITEM  ( "L-1 - Item",   1),
   SSD_CM_INIT_POPUP ( "L-1 - Popup", popup_info_L2b),
   SSD_CM_INIT_ITEM  ( "L-1 - Item",   1),
   SSD_CM_INIT_ITEM  ( "L-1 - Item",   1),
   SSD_CM_INIT_ITEM  ( "L-1 - Item",   1),
   SSD_CM_INIT_ITEM  ( "L-1 - Item",   1),
   SSD_CM_INIT_POPUP ( "L-1 - Popup", popup_info_L2c)
};

static ssd_contextmenu     popup_menu_L1a = SSD_CM_INIT_MENU( popup_items_L1);
static ssd_cm_popup_info   popup_info_L1a = SSD_CM_INIT_POPUP_INFO(popup_menu_L1a);

// Context menu items:
static ssd_cm_item main_menu_items[] = 
{
   SSD_CM_INIT_POPUP ( "Popup", popup_info_L1a),
   SSD_CM_INIT_ITEM  ( "Item",   0),
   SSD_CM_INIT_ITEM  ( "Item",   0),
   SSD_CM_INIT_ITEM  ( "Item",   0),
   SSD_CM_INIT_ITEM  ( "Item",   0),
   SSD_CM_INIT_ITEM  ( "Item",   0),
};

ssd_contextmenu  main_menu = SSD_CM_INIT_MENU( main_menu_items);
