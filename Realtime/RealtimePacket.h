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


#ifndef   __REALTIMEPACKET_H__
#define   __REALTIMEPACKET_H__
//////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////
#define  RTPACKET_ECONOMIC_BUFFER_FACTOR           (5.5)
#define  RTPACKET_STATIC_SIZE                      \
               ((int)((double)RTNET_PACKET_MAXSIZE/RTPACKET_ECONOMIC_BUFFER_FACTOR))

typedef struct tagRTPacket
{
   char  StaticBuffer[RTPACKET_STATIC_SIZE+1];
   char* DynamicBuffer;
   int   Size;

}  RTPacket, *LPRTPacket;

void  RTPacket_Init           ( LPRTPacket   this);
char* RTPacket_Alloc          ( LPRTPacket   this, int iSize);
void  RTPacket_Free           ( LPRTPacket   this);

char* RTPacket_GetBuffer      ( LPRTPacket   this);
int   RTPacket_GetBufferSize  ( LPRTPacket   this);
int   RTPacket_GetStringSize  ( LPRTPacket   this);

void  RTPacket_GetStatistics( int*  StaticAllocationsCount,
                              int*  DynamicAllocationsCount);
//////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////
#endif   //   __REALTIMEPACKET_H__
