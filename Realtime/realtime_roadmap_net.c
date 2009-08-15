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


#include <string.h>
#include "RealtimeNet.h"
#include "../roadmap_main.h"

#include "realtime_roadmap_net.h"
//////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////
typedef struct tagDataInfo
{
   RoadMapIO            IO;
   void*                pData;
   int                  iSize;
   PFN_ON_DATA_RECEIVED pfnOnDataReceived;
   void*                pContext;

}  DataInfo, *LPDataInfo;

inline void DataInfo_Init( LPDataInfo this)
{ memset( this, 0, sizeof(DataInfo));}
//////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////
#define  RECEIVER_QUEUE_SIZE           (20)
static   DataInfo AsyncJobs[RECEIVER_QUEUE_SIZE];
//////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////
static void on_socket_has_data( RoadMapIO* io)
{
   int         i  = 0;
   LPDataInfo  pDI= NULL;

   assert(io);
   if( !io)
      return;

   for( i=0; i<RECEIVER_QUEUE_SIZE; i++)
      if(roadmap_io_same(&(AsyncJobs[i].IO), io))
      {
         pDI = &(AsyncJobs[i]);
         break;
      }

   if( !pDI)
   {
      assert( 0 && "on_socket_has_data()");
      return;
   }

   pDI->iSize = roadmap_net_receive( io->os.socket, pDI->pData, pDI->iSize);
   pDI->pfnOnDataReceived( pDI->pData, pDI->iSize, pDI->pContext);
}

LPDataInfo find_receive_info( const RoadMapSocket s)
{
   int i;

   if( !s)
      return NULL;

   for( i=0; i<RECEIVER_QUEUE_SIZE; i++)
      if( s == AsyncJobs[i].IO.os.socket)
         return &(AsyncJobs[i]);

   return NULL;
}

void roadmap_net_async_receive_end( RoadMapSocket s)
{
   LPDataInfo  pDI = find_receive_info( s);

   if( pDI)
   {
      roadmap_main_remove_input( &(pDI->IO));
      DataInfo_Init( pDI);
   }
}

BOOL roadmap_net_async_receive(  RoadMapSocket        s,
                                 void*                data,
                                 int                  size,
                                 PFN_ON_DATA_RECEIVED on_data_received,
                                 void*                context)
{
   LPDataInfo  pDI      = find_receive_info( s);
   BOOL        set_input= (NULL == pDI)? TRUE: FALSE;

   if( !s || !data || !size || !on_data_received)
      return FALSE;

   if( !pDI)   // First time round?
   {
      int i;

      for( i=0; i<RECEIVER_QUEUE_SIZE; i++)
      {
         if( (NULL == AsyncJobs[i].pfnOnDataReceived) && (0 == AsyncJobs[i].IO.os.socket))
         {
            pDI = &(AsyncJobs[i]);
            break;
         }
      }

      if( !pDI)
      {
         assert(0);
         return FALSE;
      }
   }

   pDI->IO.os.socket       = s;
   pDI->IO.subsystem       = ROADMAP_IO_NET;
   pDI->pData              = data;
   pDI->iSize              = size;
   pDI->pfnOnDataReceived  = on_data_received;
   pDI->pContext           = context;

   if( set_input)
      roadmap_main_set_input( &(pDI->IO), on_socket_has_data);

   return TRUE;
}
//////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////
