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
#include <stdlib.h>
#include "RealtimeTransactionQueue.h"
//////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////
void TransactionQueueItem_Init( LPTransactionQueueItem this)
{ memset( this, 0, sizeof(TransactionQueueItem));}

void TransactionQueueItem_Release( LPTransactionQueueItem this)
{
   if( this->Packet)
   {
      free( this->Packet);
      this->Packet = NULL;
   }

   TransactionQueueItem_Init( this);
}
//////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////
void  TransactionQueue_Init   ( LPTransactionQueue this)
{ memset( this, 0, sizeof(TransactionQueue));}

int TransactionQueue_Size( LPTransactionQueue this)
{ return this->size;}

BOOL TransactionQueue_IsEmpty( LPTransactionQueue this)
{ return (0 == this->size);}

// Loose all items
void  TransactionQueue_Clear( LPTransactionQueue this)
{
   int i;
   for( i=0; i<this->size; i++)
      TransactionQueueItem_Release( &(this->Queue[i]));
   this->size = 0;
}
//////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL  TransactionQueue_Enqueue( LPTransactionQueue this, LPTransactionQueueItem pItem)
{
   if(!this                      ||
      !pItem                     ||
      !pItem->Packet             ||
      !pItem->Packet[0]          ||
      !pItem->pCI                ||
      !pItem->pfnOnCompleted     ||
      !pItem->pfnOnDataReceived)
   {
      roadmap_log( ROADMAP_ERROR, "TransactionQueue_Enqueue() - Invalid argument");
      return FALSE;  // Invalid argument
   }

   if( TRANSACTION_QUEUE_SIZE <= this->size)
   {
      roadmap_log( ROADMAP_ERROR, "TransactionQueue_Enqueue() - Queue is full");
      return FALSE;  // Queue is full
   }

   this->Queue[this->size++] = *pItem;
    return TRUE;
}

BOOL  TransactionQueue_Dequeue( LPTransactionQueue this, LPTransactionQueueItem pItem)
{
   if(!this || !pItem)
   {
      if( pItem)
         TransactionQueueItem_Init( pItem);

      roadmap_log( ROADMAP_ERROR, "TransactionQueue_Enqueue() - Invalid argument");
      return FALSE;  // Invalid argument
   }

   TransactionQueueItem_Init( pItem);

   if( !this->size)
   {
      roadmap_log( ROADMAP_DEBUG, "TransactionQueue_Enqueue() - Queue is empty");
      return FALSE;
   }

   (*pItem) = this->Queue[0];
   this->size--;

   if( 0 == this->size)
      TransactionQueueItem_Init( &(this->Queue[0]));
   else
   {
      void* dest     = &(this->Queue[0]);
      void* src      = &(this->Queue[1]);
      int   count    =(this->size * sizeof(TransactionQueueItem));
      memmove( dest, src, count);
      TransactionQueueItem_Init( &(this->Queue[this->size]));
   }

   return TRUE;
}
