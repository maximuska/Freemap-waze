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


#ifndef   __REALTIME_ROADMAP_NET_H__
#define   __REALTIME_ROADMAP_NET_H__
//////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////
// A-syncronious receive:
typedef void(*PFN_ON_DATA_RECEIVED)( void* data, int size, void* context);

BOOL roadmap_net_async_receive(  RoadMapSocket        s, 
                                 void*                data,
                                 int                  size,
                                 PFN_ON_DATA_RECEIVED on_data_received,
                                 void*                context);

void roadmap_net_async_receive_end(RoadMapSocket      s);
//////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////
#endif   //   __REALTIME_ROADMAP_NET_H__
