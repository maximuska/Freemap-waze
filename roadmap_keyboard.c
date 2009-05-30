
/* roadmap_keyboard.h
 *
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

#include "roadmap_keyboard.h"

#include <stdlib.h>
/////////////////////////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////////////////////////
#define  RMKB_MAXIMUM_REGISTERED_CALLBACKS         (20)
static   PFN_ONKEYPRESSED  gs_CB_OnKeyPressed[RMKB_MAXIMUM_REGISTERED_CALLBACKS];
static   int               gs_CB_OnKeyPressed_count = 0;
/////////////////////////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////////////////////////
//   Message loop should call these methods:
BOOL roadmap_keyboard_handler__key_pressed( const char* utf8char, uint32_t flags)
{
   int i;
   
   for( i=0; i<gs_CB_OnKeyPressed_count; i++)
      if( gs_CB_OnKeyPressed[i]( utf8char, flags))
         return TRUE;

   return FALSE;
}
/////////////////////////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////////////////////////
BOOL roadmap_keyboard_register_to_event__general(   
                     void*   Callbacks[],      //   Callbacks array
                     int*   pCallbacksCount,   //   Current size of array
                     void*   pfnNewCallback)   //   New callback
{
   if( !Callbacks || !pCallbacksCount || !pfnNewCallback)
      return FALSE;   //   Invalid parameters
   
   //   Verify we are not already registered:
   if( (*pCallbacksCount))
   {
      int   i; 
      for( i=0; i<(*pCallbacksCount); i++)
         if( Callbacks[i] == pfnNewCallback)
            return FALSE;   //   Callback already included in our array
   }

   //   Add only if we did not reach maximum callbacks count:
   if( (*pCallbacksCount) < RMKB_MAXIMUM_REGISTERED_CALLBACKS)
   {
      Callbacks[(*pCallbacksCount)++] = pfnNewCallback;
      return TRUE;
   }
   
   //   Maximum count exeeded...
   return FALSE;
}

BOOL roadmap_keyboard_unregister_from_event__general(   
                     void*   Callbacks[],      //   Callbacks array
                     int*   pCallbacksCount,   //   Current size of array
                     void*   pfnCallback)      //   Callback to remove
{
   BOOL bFound = FALSE;
   
   if( !Callbacks || !pCallbacksCount || !(*pCallbacksCount) || !pfnCallback)
      return FALSE;   //   Invalid parameters...
   
   if( Callbacks[(*pCallbacksCount)-1] == pfnCallback)
      bFound = TRUE;
   else
   {
      int   i; 
      
      //   Remove event and shift all higher events one position back:
      for( i=0; i<((*pCallbacksCount)-1); i++)
      {
         if( bFound)
            Callbacks[i] = Callbacks[i+1];
         else
         {
            if( Callbacks[i] == pfnCallback)
            {
               Callbacks[i] = Callbacks[i+1];
               bFound = TRUE;
            }
         }
      }
   }
   
   if( bFound)
   {
      (*pCallbacksCount)--;
      Callbacks[(*pCallbacksCount)] = NULL;
   }
   
   return bFound;
}
/////////////////////////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////////////////////////
BOOL roadmap_keyboard_register_to_event__key_pressed( PFN_ONKEYPRESSED pfnOnKeyPressed)
{
   return roadmap_keyboard_register_to_event__general( 
                  (void*)gs_CB_OnKeyPressed, //   Callbacks array
                  &gs_CB_OnKeyPressed_count, //   Current size of array
                  (void*)pfnOnKeyPressed);   //   New callback
}

BOOL roadmap_keyboard_unregister_from_event__key_pressed( PFN_ONKEYPRESSED pfnOnKeyPressed)
{
   return roadmap_keyboard_unregister_from_event__general( 
                  (void*)gs_CB_OnKeyPressed, //   Callbacks array
                  &gs_CB_OnKeyPressed_count, //   Current size of array
                  (void*)pfnOnKeyPressed);   //   Callback to remove
}
/////////////////////////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////////////////////////
const char* roadmap_keyboard_virtual_key_name( EVirtualKey vk)
{
   switch( vk)
   {
      case VK_Back:         return "Back";
      case VK_Softkey_left: return "Softkey-left";
      case VK_Softkey_right:return "Softkey-right";
      case VK_Arrow_up:     return "Arrow-up";
      case VK_Arrow_down:   return "Arrow-down";
      case VK_Arrow_left:   return "Arrow-left";
      case VK_Arrow_right:  return "Arrow-right";
      default:              return "<unknown>";
   }
}
/////////////////////////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////////////////////////
