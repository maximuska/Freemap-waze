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

                                               
#ifndef	__FREEMAP_REALTIMETRANSACTIONQUEUE_H__
#define	__FREEMAP_REALTIMETRANSACTIONQUEUE_H__
//////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////
#include "RealtimeNetDefs.h"

#define  TRANSACTION_QUEUE_SIZE              (5)
//////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////
typedef struct tagTransactionQueueItem
{
   LPRTConnectionInfo   pCI;              // Connection Info
   transaction_type     type;             // Login | Command | Static | ...
   PFN_ONDATARECEIVED   pfnOnDataReceived;// Callback to handle data received
   PFN_ONASYNCCOMPLETED pfnOnCompleted;   // Callback for transaction completion
   char*                Packet;           // Custom data for the HTTP request

}  TransactionQueueItem, *LPTransactionQueueItem;

void TransactionQueueItem_Init   ( LPTransactionQueueItem this);
void TransactionQueueItem_Release( LPTransactionQueueItem this);
//////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////
typedef struct tagTransactionQueue
{
   TransactionQueueItem Queue[TRANSACTION_QUEUE_SIZE];
   int                  size;

}  TransactionQueue, *LPTransactionQueue;

void  TransactionQueue_Init   ( LPTransactionQueue this);
int   TransactionQueue_Size   ( LPTransactionQueue this);
BOOL  TransactionQueue_IsEmpty( LPTransactionQueue this);
void  TransactionQueue_Clear  ( LPTransactionQueue this);  // Loose all items

BOOL  TransactionQueue_Enqueue( LPTransactionQueue this, LPTransactionQueueItem pItem);
BOOL  TransactionQueue_Dequeue( LPTransactionQueue this, LPTransactionQueueItem pItem);
//////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////
#endif	//	__FREEMAP_REALTIMETRANSACTIONQUEUE_H__
