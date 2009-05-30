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
#include "RealtimeNet.h"
//////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////
int   Statistics_StaticAllocationsCount   = 0;
int   Statistics_DynamicAllocationsCount  = 0;
//////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////
void  RTPacket_Init( LPRTPacket this)
{ memset( this, 0, sizeof(RTPacket));}

char* RTPacket_Alloc( LPRTPacket this, int iSize)
{
   RTPacket_Free( this);

   iSize++; // For the string terminating-NULL char
   
   if( iSize <= RTPACKET_STATIC_SIZE)
   {
      Statistics_StaticAllocationsCount++;
      this->Size = iSize;
      return this->StaticBuffer;
   }
   
   // Else
   Statistics_DynamicAllocationsCount++;
   this->DynamicBuffer = malloc( iSize);
   
   if( this->DynamicBuffer)
      this->Size = iSize;
   
   return this->DynamicBuffer;
}

void RTPacket_Free( LPRTPacket this)
{
   if( !this->Size)
      return;

   if( this->DynamicBuffer)
   {
      free( this->DynamicBuffer);
      this->DynamicBuffer = NULL;
   }
   else
      RTPacket_Init( this);
   
   this->Size = 0;
}

char* RTPacket_GetBuffer( LPRTPacket this)
{
   if( !this->Size)
      return NULL;
      
   if( this->DynamicBuffer)
      return this->DynamicBuffer;
   
   return this->StaticBuffer;
}

int RTPacket_GetStringSize( LPRTPacket this)
{
   char* p = RTPacket_GetBuffer( this);
   
   if( !p || !(*p))
      return 0;
   
   return strlen( p);
}

int RTPacket_GetBufferSize( LPRTPacket this)
{ return this->Size;}
//////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////
void  RTPacket_GetStatistics( int*  StaticAllocationsCount,
                              int*  DynamicAllocationsCount)
{
   (*StaticAllocationsCount)  = Statistics_StaticAllocationsCount;
   (*DynamicAllocationsCount) = Statistics_DynamicAllocationsCount;
}
//////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////
