/*
 * LICENSE:
 *
 *   Copyright 2009 Maxim Kalaev
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

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include "RealtimeNet.h"
#include "WebServiceAddress.h"
#include "RealtimeString.h"
#include "RealtimeTransactionQueue.h"
#include "RealtimePacket.h"
#include "RealtimeAlerts.h"
#include "RealtimeOffline.h"
#include "Realtime.h"

#include "../roadmap_gps.h"
#include "../roadmap_navigate.h"
#include "../roadmap_trip.h"
#include "../roadmap_net.h"
#include "../roadmap_messagebox.h"
#include "../roadmap_start.h"
#include "../roadmap_layer.h"
#include "../roadmap_main.h"
#include "../editor/db/editor_marker.h"
#include "../editor/db/editor_shape.h"
#include "../editor/db/editor_trkseg.h"
#include "../editor/db/editor_line.h"
#include "../editor/db/editor_point.h"
#include "../editor/db/editor_street.h"
#include "../editor/track/editor_track_report.h"

//////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////
static char                      gs_WebServiceAddress [ WSA_STRING_MAXSIZE       + 1];
static char                      gs_WebServerURL      [ WSA_SERVER_URL_MAXSIZE   + 1];
static char                      gs_WebServiceName    [ WSA_SERVICE_NAME_MAXSIZE + 1];
static int                       gs_WebServerPort;
BOOL                             gs_WebServiceParamsLoaded = FALSE;
static HttpAsyncTransactionInfo  g_AsyncInfo;
static TransactionQueue          g_Queue;

static void on_socket_connected( RoadMapSocket Socket, void* context, network_error ec);
static void on_data_received   ( void* data, int size, void* context);

extern const char* RT_GetWebServiceAddress();

//////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL  RTNet_Init()
{
   HttpAsyncTransactionInfo_Init( &g_AsyncInfo);
   TransactionQueue_Init( &g_Queue);
   return TRUE;
}

void  RTNet_Term()
{
}
//////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////
#define  MEGA               (1000000)
void convert_int_coordinate_to_float_string(char* buffer, int value)
{
   /* Buffer minimum size is COORDINATE_VALUE_STRING_MAXSIZE  */

   int   precision_part;
   int   integer_part;
   BOOL  negative = FALSE;

   if( !value)
   {
      strcpy( buffer, "0");
      return;
   }

   if( value < 0)
   {
      negative = TRUE;
      value   *= -1;
   }

   precision_part= value%MEGA;
   integer_part  = value/MEGA;

   if( negative)
      sprintf( buffer, "-%d.%06d", integer_part, precision_part);
   else
      sprintf( buffer,  "%d.%06d", integer_part, precision_part);
}


void format_RoadMapPosition_string( char* buffer, const RoadMapPosition* position)
{
   char  float_string[COORDINATE_VALUE_STRING_MAXSIZE+1];

   convert_int_coordinate_to_float_string( float_string, position->longitude);
   sprintf( buffer, "%s,", float_string);
   convert_int_coordinate_to_float_string( float_string, position->latitude);
   strcat( buffer, float_string);
}

void format_RoadMapGpsPosition_string( char* buffer, const RoadMapGpsPosition* position)
{
   /* Buffer minimum size is RoadMapGpsPosition_STRING_MAXSIZE */

   char  float_string_longitude[COORDINATE_VALUE_STRING_MAXSIZE+1];
   char  float_string_latitude [COORDINATE_VALUE_STRING_MAXSIZE+1];
   char  float_string_altitude [COORDINATE_VALUE_STRING_MAXSIZE+1];

   convert_int_coordinate_to_float_string( float_string_longitude, position->longitude);
   convert_int_coordinate_to_float_string( float_string_latitude , position->latitude);
   convert_int_coordinate_to_float_string( float_string_altitude , position->altitude);
   sprintf( buffer,
            "%s,%s,%s,%d,%d",
            float_string_longitude,
            float_string_latitude,
            float_string_altitude,
            position->steering,
            position->speed);
}

void format_DB_point_string( char* buffer, int longitude, int latitude, time_t timestamp, int db_id)
{
   /* Buffer minimum size is DB_Point_STRING_MAXSIZE */

   char  float_string_longitude[COORDINATE_VALUE_STRING_MAXSIZE+1];
   char  float_string_latitude [COORDINATE_VALUE_STRING_MAXSIZE+1];

   convert_int_coordinate_to_float_string( float_string_longitude, longitude);
   convert_int_coordinate_to_float_string( float_string_latitude , latitude);

   sprintf( buffer,
            "%d,%s,%s,%d",
            db_id,
            float_string_longitude,
            float_string_latitude,
            (int)timestamp);
}

void format_point_string( char* buffer, int longitude, int latitude, time_t timestamp)
{
   /* Buffer minimum size is Point_STRING_MAXSIZE */

   char  float_string_longitude[COORDINATE_VALUE_STRING_MAXSIZE+1];
   char  float_string_latitude [COORDINATE_VALUE_STRING_MAXSIZE+1];

   convert_int_coordinate_to_float_string( float_string_longitude, longitude);
   convert_int_coordinate_to_float_string( float_string_latitude , latitude);

   sprintf( buffer,
            ",%s,%s,%d",
            float_string_longitude,
            float_string_latitude,
            (int)timestamp);
}

void format_RoadMapArea_string( char* buffer, const RoadMapArea* area)
{
   /* Buffer minimum size is RoadMapArea_STRING_MAXSIZE */

   char  float_string_east [COORDINATE_VALUE_STRING_MAXSIZE+1];
   char  float_string_north[COORDINATE_VALUE_STRING_MAXSIZE+1];
   char  float_string_west [COORDINATE_VALUE_STRING_MAXSIZE+1];
   char  float_string_south[COORDINATE_VALUE_STRING_MAXSIZE+1];

   convert_int_coordinate_to_float_string( float_string_east , area->east );
   convert_int_coordinate_to_float_string( float_string_north, area->north);
   convert_int_coordinate_to_float_string( float_string_west , area->west );
   convert_int_coordinate_to_float_string( float_string_south, area->south);

   // Note: Order expected by server:  West, South, East, North
   sprintf( buffer,
            "%s,%s,%s,%s",
            float_string_west ,
            float_string_south,
            float_string_east ,
            float_string_north);
}


BOOL format_ParamPair_string( char*       buffer,
                              int         iBufSize,
                              int         nParams,
                              const char*   szParamKey[],
                              const char*   szParamVal[])
{
   int   iParam;
   int   iBufPos = 0;

   sprintf( buffer, "%d", nParams * 2);
   iBufPos = strlen( buffer );

   for (iParam = 0; iParam < nParams; iParam++)
   {
      if (szParamKey[iParam] && (*szParamKey[iParam]) &&
          szParamVal[iParam] && (*szParamVal[iParam]))
      {
         if (iBufPos == iBufSize)
         {
            roadmap_log( ROADMAP_ERROR, "format_ParamPair_string() - Failed to print params");
            roadmap_messagebox ("Error", "Sending Report failed");
            return FALSE;
         }
         buffer[iBufPos++] = ',';

         if(!PackNetworkString( szParamKey[iParam], buffer + iBufPos, iBufSize - iBufPos))
         {
            roadmap_log( ROADMAP_ERROR, "format_ParamPair_string() - Failed to print params");
            roadmap_messagebox ("Error", "Sending Report failed");
            return FALSE;
         }
         iBufPos += strlen (buffer + iBufPos);

         if (iBufPos == iBufSize)
         {
            roadmap_log( ROADMAP_ERROR, "format_ParamPair_string() - Failed to print params");
            roadmap_messagebox ("Error", "Sending Report failed");
            return FALSE;
         }
         buffer[iBufPos++] = ',';

         if(!PackNetworkString( szParamVal[iParam], buffer + iBufPos, iBufSize - iBufPos))
         {
            roadmap_log( ROADMAP_ERROR, "format_ParamPair_string() - Failed to print params");
            roadmap_messagebox ("Error", "Sending Report failed");
            return FALSE;
         }
         iBufPos += strlen (buffer + iBufPos);
      }
   }

   return TRUE;
}


BOOL RTNet_LoadParams()
{
   if( !gs_WebServiceParamsLoaded)
   {
      const char*   szWebServiceAddress = RT_GetWebServiceAddress();

      //   Break full name into parameters:
      if(!WSA_ExtractParams(
         szWebServiceAddress, //   IN        -   Web service full address (http://...)
         gs_WebServerURL,     //   OUT,OPT   -   Server URL[:Port]
         &gs_WebServerPort,   //   OUT,OPT   -   Server Port
         gs_WebServiceName))  //   OUT,OPT   -   Web service name
      {
         roadmap_log( ROADMAP_ERROR, "RTNet_LoadParams() - Invalid web-service address (%s)", szWebServiceAddress);

         //   Web-Service address string is invalid...
         return FALSE;
      }

      //   Copy full address:
      strncpy_safe( gs_WebServiceAddress, szWebServiceAddress, sizeof (gs_WebServiceAddress));

      gs_WebServiceParamsLoaded = TRUE;
   }

   return TRUE;
}

int RTNet_Send( RoadMapSocket socket, const char* szData)
{
   int iRes;

   if( ROADMAP_INVALID_SOCKET == socket)
   {
      roadmap_log( ROADMAP_ERROR, "RTNet_Send() - Socket is invalid");
      return -1;
   }

   iRes = roadmap_net_send( socket, szData, (int)strlen(szData), 1 /* Wait */);

   if( -1 == iRes)
      roadmap_log( ROADMAP_ERROR, "RTNet_Send( SOCKET: %d) - 'roadmap_net_send()' returned -1", socket);
   else
      roadmap_log( ROADMAP_DEBUG, "RTNet_Send( SOCKET: %d) - Sent %d bytes", socket, strlen(szData));

   return iRes;
}
//////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL RTNet_Login( LPRTConnectionInfo   pCI,
                  const char*          szUserName,
                  const char*          szUserPW,
                  const char*          szUserNickname,
                  PFN_ONASYNCCOMPLETED pfnOnCompleted)
{
   //   Do we have a name/pw
   if( !szUserName || !(*szUserName) || !szUserPW || !(*szUserPW))
   {
      roadmap_log( ROADMAP_ERROR, "RTNet_Login() - name and/or password were not supplied");
      return FALSE;
   }

   //   Verify sizes:
   if( (RT_USERNM_MAXSIZE < strlen(szUserName))   ||
       (RT_USERPW_MAXSIZE < strlen(szUserPW)))
   {
      roadmap_log( ROADMAP_ERROR, "RTNet_Login() - Size of name/password is bigger then maximum (%d/%d)",
                  RT_USERNM_MAXSIZE,RT_USERPW_MAXSIZE);
      return FALSE;
   }

   //   Copy name/pw
   PackNetworkString( szUserName,      pCI->UserNm, RT_USERNM_MAXSIZE);
   PackNetworkString( szUserPW,        pCI->UserPW, RT_USERPW_MAXSIZE);

   if( szUserNickname && (*szUserNickname))
      PackNetworkString( szUserNickname,  pCI->UserNk, RT_USERNK_MAXSIZE);
   else
      szUserNickname = "";

   //   Perform the HTTP request/response transaction:
   if( RTNet_HttpAsyncTransaction(
         pCI,                          // Connection Info
         login_transaction,            // Login | Command | Static | ...
         OnLoginResponse,              // Callback to handle data received
         pfnOnCompleted,               // Callback for transaction completion
         RTNET_FORMAT_NETPACKET_6Login,// Custom data for the HTTP request
            RTNET_PROTOCOL_VERSION,
            szUserName,
            szUserPW,
            RT_DEVICE_ID,
            roadmap_start_version(),
            szUserNickname))
      return TRUE;

   memset( pCI->UserNm, 0, sizeof(pCI->UserNm));
   memset( pCI->UserPW, 0, sizeof(pCI->UserPW));
   return FALSE;
}

BOOL RTNet_Register( LPRTConnectionInfo pCI, PFN_ONASYNCCOMPLETED pfnOnCompleted)
{
   //   Verify identity is reset:
   memset( pCI->UserNm, 0, sizeof(pCI->UserNm));
   memset( pCI->UserPW, 0, sizeof(pCI->UserPW));
   memset( pCI->UserNk, 0, sizeof(pCI->UserNk));

   //   Perform the HTTP request/response transaction:
   return RTNet_HttpAsyncTransaction(
         pCI,                 //   Connection Info
         static_transaction,  //   Login | Command | Static | ...
         OnRegisterResponse,  //   Callback to handle data received
         pfnOnCompleted,      //   Callback for transaction completion
                              //   Custom data for the HTTP request
         RTNET_FORMAT_NETPACKET_3Register,
         RTNET_PROTOCOL_VERSION,
         RT_DEVICE_ID,
         roadmap_start_version());
}

BOOL RTNet_GuestLogin( LPRTConnectionInfo pCI, PFN_ONASYNCCOMPLETED pfnOnCompleted)
{
   //   Verify identity is reset:
   memset( pCI->UserNm, 0, sizeof(pCI->UserNm));
   memset( pCI->UserPW, 0, sizeof(pCI->UserPW));
   memset( pCI->UserNk, 0, sizeof(pCI->UserNk));

   //   Perform the HTTP request/response transaction:
   return RTNet_HttpAsyncTransaction(
         pCI,              //   Connection Info
         login_transaction,//   Login | Command | Static | ...
         OnLoginResponse,  //   Callback to handle data received
         pfnOnCompleted,   //   Callback for transaction completion
                           //   Custom data for the HTTP request
         RTNET_FORMAT_NETPACKET_3GuestLogin,
         RTNET_PROTOCOL_VERSION,
         RT_DEVICE_ID,
         roadmap_start_version());
}

BOOL RTNet_Logout( LPRTConnectionInfo pCI, PFN_ONASYNCCOMPLETED pfnOnCompleted)
{
   return RTNet_HttpAsyncTransaction(
                                 pCI,                 //   Connection Info
                                 command_transaction, //   Login | Command | Static | ...
                                 OnLogOutResponse,    //   Callback to handle data received
                                 pfnOnCompleted,      //   Callback for transaction completion
                                 "Logout");           //   Custom data for the HTTP request
}

BOOL RTNet_At( LPRTConnectionInfo   pCI,
               const
               RoadMapGpsPosition*  pGPSPosition,
               int                  from_node,
               int                  to_node,
               PFN_ONASYNCCOMPLETED pfnOnCompleted,
               char*                packet_only)
{
   char  GPSPosString[RoadMapGpsPosition_STRING_MAXSIZE+1];

   format_RoadMapGpsPosition_string( GPSPosString, pGPSPosition);

   if( packet_only)
   {
      sprintf( packet_only,
               RTNET_FORMAT_NETPACKET_3At,
                  GPSPosString,
                  from_node,
                  to_node);
      return TRUE;
   }

   // Else:
   return RTNet_HttpAsyncTransaction(
         pCI,                       // Connection Info
         command_transaction,       // Login | Command | Static | ...
         OnGeneralResponse,         // Callback to handle data received
         pfnOnCompleted,            // Callback for transaction completion
         RTNET_FORMAT_NETPACKET_3At,// Custom data for the HTTP request
            GPSPosString,
            from_node,
            to_node);
}

BOOL RTNet_NavigateTo(  LPRTConnectionInfo   pCI,
                        const
                        RoadMapPosition*     cordinates,
                        address_info_ptr     ai,
                        PFN_ONASYNCCOMPLETED pfnOnCompleted)
{
   char  gps_point[RoadMapPosition_STRING_MAXSIZE+1];

   format_RoadMapPosition_string( gps_point, cordinates);

   return RTNet_HttpAsyncTransaction(
         pCI,                                // Connection Info
         command_transaction,                // Login | Command | Static | ...
         OnGeneralResponse,                  // Callback to handle data received
         pfnOnCompleted,                     // Callback for transaction completion
         RTNET_FORMAT_NETPACKET_4NavigateTo, // Custom data for the HTTP request
            gps_point,
            ai->city,
            ai->street,
            ai->house);
}
BOOL RTNet_MapDisplyed( LPRTConnectionInfo   pCI,
                        const RoadMapArea*   pRoadMapArea,
                        unsigned int         scale,
                        PFN_ONASYNCCOMPLETED pfnOnCompleted,
                        char*                packet_only)
{
   char  MapArea[RoadMapArea_STRING_MAXSIZE+1];

   format_RoadMapArea_string( MapArea, pRoadMapArea);

   if( packet_only)
   {
      sprintf( packet_only, RTNET_FORMAT_NETPACKET_2MapDisplayed, MapArea, scale);
      return TRUE;
   }

   // Else:
   return RTNet_HttpAsyncTransaction(
         pCI,                 //   Connection Info
         command_transaction, //   Login | Command | Static | ...
         OnGeneralResponse,   //   Callback to handle data received
         pfnOnCompleted,      //   Callback for transaction completion
                              //   Custom data for the HTTP request
         RTNET_FORMAT_NETPACKET_2MapDisplayed,
            MapArea, scale);
}

BOOL   RTNet_CreateNewRoads (
                           LPRTConnectionInfo   pCI,
                           int                  nToggles,
                           const time_t*         toggle_time,
                           BOOL                  bStatusFirst,
                           PFN_ONASYNCCOMPLETED pfnOnCompleted,
                           char*                packet_only)
{
   RTPacket    Packet;
   char*       CreateNewRoadsBuffer = NULL;
   int          i;
   BOOL          bStatus;
   BOOL         bRes;

   RTPacket_Init( &Packet);

   CreateNewRoadsBuffer = RTPacket_Alloc( &Packet, RTNET_CREATENEWROADS_BUFFERSIZE__dynamic(nToggles));
   memset( CreateNewRoadsBuffer, 0, RTNET_CREATENEWROADS_BUFFERSIZE__dynamic(nToggles));

   bStatus = bStatusFirst;
   for (i = 0; i < nToggles; i++)
   {
      char *buffer = CreateNewRoadsBuffer + strlen (CreateNewRoadsBuffer);
      sprintf (buffer, RTNET_FORMAT_NETPACKET_2CreateNewRoads,
               (unsigned int)toggle_time[i],
               bStatus ? "T" : "F");
      bStatus = !bStatus;
   }

   assert(*CreateNewRoadsBuffer);
   roadmap_log(ROADMAP_DEBUG, "RTNet_CreateNewRoads() - Output command: '%s'", CreateNewRoadsBuffer);

   if( packet_only)
   {
      strcpy( packet_only, CreateNewRoadsBuffer);
      bRes = TRUE;
   }
   else
   {
      bRes = RTNet_HttpAsyncTransaction(
                                    pCI,                 //   Connection Info
                                    command_transaction, //   Login | Command | Static | ...
                                    OnGeneralResponse,   //   Callback to handle data received
                                    pfnOnCompleted,      //   Callback for transaction completion
                                    CreateNewRoadsBuffer);      //   Custom data
   }

   RTPacket_Free( &Packet);
   return bRes;
}


void RTNet_GPSPath_BuildCommand( char*             Packet,
                                 LPGPSPointInTime  points,
                                 int               count,
                                 BOOL                   end_track)
{
   int      i;
   char     temp[RTNET_GPSPATH_BUFFERSIZE_single_row+1];

   if( (count >= 2) && (RTTRK_GPSPATH_MAX_POINTS >= count))
   {
       sprintf( Packet, "GPSPath,%u,%u", (uint32_t)points->GPS_time, (3 * count));

       for( i=0; i<count; i++)
       {
          char  gps_point[RoadMapPosition_STRING_MAXSIZE+1];
          int   seconds_gap = 0;

          if( i)
             seconds_gap = (int)(points[i].GPS_time - points[i-1].GPS_time);

          assert( !GPSPOINTINTIME_IS_INVALID(points[i]));
/*
          if (points[i].Position.longitude < 20000000 ||
             points[i].Position.longitude > 40000000 ||
             points[i].Position.latitude < 20000000 ||
             points[i].Position.latitude > 40000000 ||
             seconds_gap < 0 ||
             seconds_gap > 1000) {

            roadmap_log (ROADMAP_ERROR, "Invalid GPS sequence: %d,%d,%d",
                             points[i].Position.longitude,
                             points[i].Position.latitude,
                             seconds_gap);
          }
*/
          format_RoadMapPosition_string( gps_point, &(points[i].Position));
          sprintf( temp, ",%s,%d", gps_point, seconds_gap);
          strcat( Packet, temp);
       }
       strcat( Packet, "\n");
   }

   if (end_track)
   {
    strcat( Packet, "GPSDisconnect\n");
   }
}

BOOL RTNet_GPSPath(  LPRTConnectionInfo   pCI,
                     time_t               period_begin,
                     LPGPSPointInTime     points,
                     int                  count,
                     PFN_ONASYNCCOMPLETED pfnOnCompleted,
                     char*                packet_only)
{
   RTPacket Packet;
   char*    GPSPathBuffer = NULL;
   int      iRangeBegin;
   BOOL     bRes;
   int      i;

   if( count < 2)
      return FALSE;

   RTPacket_Init( &Packet);

   if( RTTRK_GPSPATH_MAX_POINTS < count) {
      roadmap_log (ROADMAP_ERROR, "GPSPath too long, dropping first %d points", count - RTTRK_GPSPATH_MAX_POINTS);
      points += count - RTTRK_GPSPATH_MAX_POINTS;
    points[0].Position.longitude = INVALID_COORDINATE;
    points[0].Position.latitude = INVALID_COORDINATE;
    points[0].GPS_time = 0;
      count = RTTRK_GPSPATH_MAX_POINTS;
   }

   GPSPathBuffer = RTPacket_Alloc( &Packet, RTNET_GPSPATH_BUFFERSIZE__dynamic(count));
   memset( GPSPathBuffer, 0, RTNET_GPSPATH_BUFFERSIZE__dynamic(count));

   iRangeBegin = 0;
   for( i=0; i<count; i++)
   {
      if( GPSPOINTINTIME_IS_INVALID( points[i]))
      {
         int               iPointsCount= i - iRangeBegin;
         LPGPSPointInTime  FirstPoint  = points + iRangeBegin;
         char* Buffer = GPSPathBuffer + strlen(GPSPathBuffer);

         roadmap_log(ROADMAP_DEBUG,
                     "RTNet_GPSPath(GPS-DISCONNECTION TAG) - Adding %d points to packet. Range offset: %d",
                     iPointsCount, iRangeBegin);
         RTNet_GPSPath_BuildCommand( Buffer, FirstPoint, iPointsCount, TRUE);
         iRangeBegin = i+1;
      }
   }

   if( iRangeBegin < (count - 1))
   {
      LPGPSPointInTime  FirstPoint  = points + iRangeBegin;
      int               iPointsCount= count - iRangeBegin;
      char*             Buffer      = GPSPathBuffer + strlen(GPSPathBuffer);

      roadmap_log(ROADMAP_DEBUG,
                  "RTNet_GPSPath() - Adding range to packet. Range begin: %d; Range end: %d (count-1)",
                  iRangeBegin, (count - 1));
      RTNet_GPSPath_BuildCommand( Buffer, FirstPoint, iPointsCount, FALSE);
   }

   assert(*GPSPathBuffer);
   roadmap_log(ROADMAP_DEBUG, "RTNet_GPSPath() - Output command: '%s'", GPSPathBuffer);

   if( packet_only)
   {
      sprintf( packet_only, "%s", GPSPathBuffer);
      bRes = TRUE;
   }
   else
      bRes = RTNet_HttpAsyncTransaction(
                                    pCI,                 //   Connection Info
                                    command_transaction, //   Login | Command | Static | ...
                                    OnGeneralResponse,   //   Callback to handle data received
                                    pfnOnCompleted,      //   Callback for transaction completion
                                    GPSPathBuffer);      //   Custom data

   RTPacket_Free( &Packet);
   return bRes;
}

BOOL RTNet_NodePath( LPRTConnectionInfo   pCI,
                     time_t               period_begin,
                     LPNodeInTime         nodes,
                     int                  count,
                     PFN_ONASYNCCOMPLETED pfnOnCompleted,
                     char*                packet_only)
{
   RTPacket Packet;
   char*    NodePathBuffer = NULL;
   int      i;
   char     temp[RTNET_NODEPATH_BUFFERSIZE_temp+1];
   BOOL     bRes;

   if( count < 1)
      return FALSE;

   RTPacket_Init( &Packet);

   if( RTTRK_NODEPATH_MAX_POINTS < count) {
      // patch by SRUL
      //return FALSE;
      nodes += count - RTTRK_NODEPATH_MAX_POINTS;
      count = RTTRK_NODEPATH_MAX_POINTS;
   }

   NodePathBuffer = RTPacket_Alloc( &Packet, RTNET_GPSPATH_BUFFERSIZE__dynamic(count));

   memset( NodePathBuffer, 0, sizeof(NodePathBuffer));
   sprintf( NodePathBuffer, "NodePath,%ld,", period_begin);//(period_end-period_begin));
   sprintf( temp, "%d", 2 * count);
   strcat( NodePathBuffer, temp);

   for( i=0; i<count; i++)
   {
      int   seconds_gap = 0;

      if( i)
         seconds_gap = (int)(nodes[i].GPS_time - nodes[i-1].GPS_time);
/*
    if (nodes[i].node < -2 ||
         nodes[i].node > 1000000 ||
         seconds_gap < 0 ||
         seconds_gap > 1000000) {

        roadmap_log (ROADMAP_ERROR, "Invalid Node sequence: %d,%d",
                         nodes[i].node,
                         seconds_gap);
    }
*/
      sprintf( temp, ",%d,%d", nodes[i].node, seconds_gap);
      strcat( NodePathBuffer, temp);
   }

   if( packet_only)
   {
      sprintf( packet_only, "%s\n", NodePathBuffer);
      bRes = TRUE;
   }
   else
      bRes = RTNet_HttpAsyncTransaction(
                                    pCI,                 //   Connection Info
                                    command_transaction, //   Login | Command | Static | ...
                                    OnGeneralResponse,   //   Callback to handle data received
                                    pfnOnCompleted,      //   Callback for transaction completion
                                    NodePathBuffer);     //   Custom data

   RTPacket_Free( &Packet);
   return bRes;
}

BOOL RTNet_ReportAlert( LPRTConnectionInfo   pCI,
                        int                  iType,
                        const char*          szDescription,
                        int                  iDirection,
                        PFN_ONASYNCCOMPLETED pfnOnCompleted)
{
   const RoadMapGpsPosition   *TripLocation;

   TripLocation = roadmap_trip_get_gps_position("AlertSelection");
   if (TripLocation == NULL){
        roadmap_messagebox ("Error", "Can't find current street.");
        return FALSE;
    }

   if ((TripLocation == NULL) || (TripLocation->latitude <= 0) || (TripLocation->longitude <= 0)) {
             return FALSE;
   }else{
      roadmap_trip_restore_focus();

      return RTNet_ReportAlertAtPosition(pCI, iType, szDescription, iDirection, TripLocation, pfnOnCompleted);
   }
}

BOOL RTNet_SendSMS( LPRTConnectionInfo   pCI,
                    const char*          szPhoneNumber,
                    PFN_ONASYNCCOMPLETED pfnOnCompleted)
{

   return RTNet_HttpAsyncTransaction(
         pCI,                       // Connection Info
         command_transaction,       // Login | Command | Static | ...
         OnGeneralResponse,         // Callback to handle data received
         pfnOnCompleted,            // Callback for transaction completion
         RTNET_FORMAT_NETPACKET_1SendSMS,// Custom data for the HTTP request
         szPhoneNumber
        );
}

BOOL RTNet_PostAlertComment( LPRTConnectionInfo   pCI,
                        int                  iAlertId,
                        const char*          szDescription,
                        PFN_ONASYNCCOMPLETED pfnOnCompleted)
{

   char        PackedString[(RT_ALERT_DESCRIPTION_MAXSIZE*2)+1];
   const char* szPackedString = "";

   if( szDescription && (*szDescription))
   {
      if(!PackNetworkString( szDescription, PackedString, sizeof(PackedString)))
      {
         roadmap_log( ROADMAP_ERROR, "RTNet_PostAlertComment() - Failed to pack network string");
         roadmap_messagebox ("Error", "Sending Comment failed");
         return FALSE;
      }

      szPackedString = PackedString;
   }

   return RTNet_HttpAsyncTransaction(  pCI,                 // Connection Info
                                       command_transaction, // Login | Command | Static | ...
                                       OnGeneralResponse,   // Callback to handle data received
                                       pfnOnCompleted,
                                       "PostAlertComment,%d,%s", // Custom data for the HTTP request
                                          iAlertId,
                                          szPackedString);

}


BOOL RTNet_ReportAlertAtPosition( LPRTConnectionInfo   pCI,
                        int                  iType,
                        const char*     szDescription,
                        int                  iDirection,
                        const RoadMapGpsPosition   *MyLocation,
                        PFN_ONASYNCCOMPLETED pfnOnCompleted)
{
   char  GPSPosString[RoadMapGpsPosition_STRING_MAXSIZE+1];
   char        PackedString[(RT_ALERT_DESCRIPTION_MAXSIZE*2)+1];
   const char* szPackedString = "";

   if( szDescription && (*szDescription))
   {
      if(!PackNetworkString( szDescription, PackedString, sizeof(PackedString)))
      {
         roadmap_log( ROADMAP_ERROR, "RTNet_ReportAlertAtPosition() - Failed to pack network string");
         roadmap_messagebox ("Error", "Sending Report failed");
         return FALSE;
      }

      szPackedString = PackedString;
   }

   format_RoadMapGpsPosition_string( GPSPosString, MyLocation);

   return RTNet_HttpAsyncTransaction(
                  pCI,                             // Connection Info
                  command_transaction,             // Login | Command | Static | ...
                  OnGeneralResponse,               // Callback to handle data received
                  pfnOnCompleted,
                  "At,%s\r\nReportAlert,%d,%s,%d", // Custom data for the HTTP request
                     GPSPosString,
                     iType,
                     szPackedString,
                     iDirection);
}


BOOL RTNet_RemoveAlert( LPRTConnectionInfo   pCI,
                        int                  iAlertId,
                        PFN_ONASYNCCOMPLETED pfnOnCompleted)
{
   return RTNet_HttpAsyncTransaction(  pCI,                 // Connection Info
                                       command_transaction, // Login | Command | Static | ...
                                       OnGeneralResponse,   // Callback to handle data received
                                       pfnOnCompleted,
                                       "ReportRmAlert,%d",  // Custom data for the HTTP request
                                          iAlertId);
}


BOOL  RTNet_ReportMarker(  LPRTConnectionInfo   pCI,
                           const char*          szType,
                           int                  iLongitude,
                           int                  iLatitude,
                           int                  iAzimuth,
                           const char*          szDescription,
                           int                  nParams,
                           const char*            szParamKey[],
                           const char*            szParamVal[],
                           PFN_ONASYNCCOMPLETED pfnOnCompleted,
                           char*                  packet_only)
{
   char        PackedString[(ED_MARKER_MAX_STRING_SIZE*2)+1];
   char         AttrString[ED_MARKER_MAX_ATTRS * 2 * (ED_MARKER_MAX_STRING_SIZE * 2 + 1) + 4];
   const char* szPackedString = "";
   char        float_string_longitude[COORDINATE_VALUE_STRING_MAXSIZE+1];
   char        float_string_latitude [COORDINATE_VALUE_STRING_MAXSIZE+1];

   convert_int_coordinate_to_float_string( float_string_longitude, iLongitude);
   convert_int_coordinate_to_float_string( float_string_latitude , iLatitude);

   if( szDescription && (*szDescription))
   {
      if(!PackNetworkString( szDescription, PackedString, sizeof(PackedString)))
      {
         roadmap_log( ROADMAP_ERROR, "RTNet_ReportMarker() - Failed to pack network string");
         roadmap_messagebox ("Error", "Sending Report failed");
         return FALSE;
      }

      szPackedString = PackedString;
   }

   if (!format_ParamPair_string(AttrString,
                                sizeof(AttrString),
                                nParams,
                                szParamKey,
                                szParamVal))
   {
      roadmap_log( ROADMAP_ERROR, "RTNet_ReportMarker() - Failed to serialize attributes");
      roadmap_messagebox ("Error", "Sending Report failed");
      return FALSE;
   }

   if (packet_only)
   {
      sprintf( packet_only,
               RTNET_FORMAT_NETPACKET_6ReportMarker, // Custom data for the HTTP request
               szType,
               float_string_longitude,
               float_string_latitude,
               iAzimuth,
               szPackedString,
               AttrString);
      return TRUE;
   }

   return RTNet_HttpAsyncTransaction(
                  pCI,                 // Connection Info
                  command_transaction, // Login | Command | Static | ...
                  OnGeneralResponse,   // Callback to handle data received
                  pfnOnCompleted,
                  RTNET_FORMAT_NETPACKET_6ReportMarker, // Custom data for the HTTP request
                     szType,
                     float_string_longitude,
                     float_string_latitude,
                     iAzimuth,
                     szPackedString,
                     AttrString);
}

BOOL RTNet_StartFollowUsers(  LPRTConnectionInfo   pCI,
                              PFN_ONASYNCCOMPLETED pfnOnCompleted,
                              char*                packet_only)
{ return FALSE;}

BOOL RTNet_StopFollowUsers(   LPRTConnectionInfo   pCI,
                              PFN_ONASYNCCOMPLETED pfnOnCompleted,
                              char*                packet_only)
{ return FALSE;}

BOOL RTNet_SetMyVisability(LPRTConnectionInfo   pCI,
                           ERTVisabilityGroup   eVisability,
                           ERTVisabilityReport  eVisabilityReport,
                           PFN_ONASYNCCOMPLETED pfnOnCompleted,
                           char*                packet_only)
{
   if( packet_only)
   {
      sprintf( packet_only, "SeeMe,%d,%d\n", eVisability,eVisabilityReport);
      return TRUE;
   }

   // Else:
   return RTNet_HttpAsyncTransaction(  pCI,                 //   Connection Info
                                       command_transaction, //   Login | Command | Static | ...
                                       OnGeneralResponse,   //   Callback to handle data received
                                       pfnOnCompleted,      //   Callback for transaction completion
                                       "SeeMe,%d",          //   Custom data for the HTTP request
                                          eVisability);
}

BOOL RTNet_SendTrafficInfo ( LPRTConnectionInfo   pCI,
                                                              int mode,
                                                               PFN_ONASYNCCOMPLETED pfnOnCompleted){

   return RTNet_HttpAsyncTransaction(  pCI,                 // Connection Info
                                       command_transaction, // Login | Command | Static | ...
                                       OnGeneralResponse,   // Callback to handle data received
                                       pfnOnCompleted,
                                       "SendRoadInfo,%d",   // Custom data for the HTTP request
                                          mode);
}


BOOL RTNet_GeneralPacket(  LPRTConnectionInfo   pCI,
                           const char*          Packet,
                           PFN_ONASYNCCOMPLETED pfnOnCompleted)
{
    //Realtime_OfflineWrite (Packet);
   return RTNet_HttpAsyncTransaction(  pCI,                 //   Connection Info
                                       command_transaction, //   Login | Command | Static | ...
                                       OnGeneralResponse,   //   Callback to handle data received
                                       pfnOnCompleted,      //   Callback for transaction completion
                                       Packet);             //   Custom data for the HTTP request
}
//////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////
void  RTNet_TransactionQueue_Clear()
{ TransactionQueue_Clear( &g_Queue);}

BOOL RTNet_TransactionQueue_ProcessSingleItem( BOOL* pTransactionStarted)
{
   TransactionQueueItem Item;
   BOOL                 bRes;

   (*pTransactionStarted) = FALSE;

   if( TransactionQueue_IsEmpty( &g_Queue))
      return TRUE;

   if( !TransactionQueue_Dequeue( &g_Queue, &Item))
   {
      roadmap_log( ROADMAP_ERROR, "RTNet_ProcessQueueItem() - 'TransactionQueue_Dequeue()' had failed!");
      return FALSE;
   }

   bRes = RTNet_HttpAsyncTransaction(  Item.pCI,               // Connection Info
                                       Item.type,              // Login | Command | Static | ...
                                       Item.pfnOnDataReceived, // Callback to handle data received
                                       Item.pfnOnCompleted,    // Callback for transaction completion
                                       Item.Packet);           // Custom data for the HTTP request

   TransactionQueueItem_Release( &Item);

   if( bRes)
      (*pTransactionStarted) = TRUE;

   return bRes;
}

// This method terminates the a-sync transaction:
// 1. Call callback
// 2. Reset context
void RTNet_HttpAsyncTransactionCompleted( BOOL rc)
{
   PFN_ONASYNCCOMPLETED pfn = g_AsyncInfo.pfnOnAsyncCompleted;

   if( g_AsyncInfo.async_receive_started)
   {
      roadmap_net_async_receive_end( g_AsyncInfo.Socket);
      g_AsyncInfo.async_receive_started = FALSE;
   }

   if( ROADMAP_INVALID_SOCKET != g_AsyncInfo.Socket)
   {
      roadmap_net_close( g_AsyncInfo.Socket);
      g_AsyncInfo.Socket = ROADMAP_INVALID_SOCKET;
   }

   HttpAsyncTransactionInfo_Init( &g_AsyncInfo);

   if(pfn)
      pfn( rc);
}

void RTNet_Watchdog( LPRTConnectionInfo pCI)
{
   time_t   now            = time(NULL);
   int      seconds_passed = (int)(now - g_AsyncInfo.starting_time);

   if( !g_AsyncInfo.starting_time || (seconds_passed < RTNET_SESSION_TIME))
      return;

   roadmap_log( ROADMAP_ERROR, "RTNet_Watchdog() - TERMINATING SESSION !!! - Session is running already %d seconds", seconds_passed);
   pCI->LastError = ERR_SessionTimedout;
   RTNet_HttpAsyncTransactionCompleted( FALSE);
}

BOOL RTNet_QueueHttpAsyncTransaction(
            LPRTConnectionInfo   pCI,              // Connection Info
            transaction_type     type,             // Login | Command | Static | ...
            PFN_ONDATARECEIVED   pfnOnDataReceived,// Callback to handle data received
            PFN_ONASYNCCOMPLETED pfnOnCompleted,   // Callback for transaction completion
            const char*          szPacket)         // Custom data for the HTTP request
{
   TransactionQueueItem TQI;

   if(!pCI                 ||
      !pfnOnDataReceived   ||
      !pfnOnCompleted      ||
      !szPacket            ||
      !szPacket[0])
   {

      roadmap_log( ROADMAP_ERROR, "RTNet_QueueHttpAsyncTransaction() - Invalid argument");
      return FALSE;  // Invalid argument
   }


   TransactionQueueItem_Init( &TQI);

   TQI.pCI              = pCI;
   TQI.type             = type;
   TQI.pfnOnDataReceived= pfnOnDataReceived;
   TQI.pfnOnCompleted   = pfnOnCompleted;
   TQI.Packet           = malloc( strlen( szPacket) + 1);
   if( !TQI.Packet)
   {
      roadmap_log( ROADMAP_ERROR, "RTNet_QueueHttpAsyncTransaction() - Out of memory");
      return FALSE;  // Out of memory...
   }
   strcpy( TQI.Packet, szPacket);

   if( TransactionQueue_Enqueue( &g_Queue, &TQI))
      return TRUE;

   // Else (queue will log on error)
   TransactionQueueItem_Release( &TQI);
   return FALSE;
}

// Assumptions and implementation notes:
// 1. This method is called from a single thread, thus no MT locks are used.
// 2. This method supports only one request at a time.
//       Until the full transaction is completed (including all callbacks), any new
//       requests will be inserted into queue.
//       If queue is full, the method will return an error.
// 3. Signal on a-sync method termination:
//       If method returned TRUE it means that the a-sync operation had begun.
//       When the a-sync operation will terminate the callback 'pfnOnTransactionCompleted'
//       method will be called.
//       If method returned FALSE - the a-sync operation did not begin and the callback
//       'pfnOnTransactionCompleted' will not be called.
BOOL RTNet_HttpAsyncTransaction_FormattedBuffer(
            LPRTConnectionInfo   pCI,              // Connection Info
            transaction_type     type,             // Login | Command | Static | ...
            PFN_ONDATARECEIVED   pfnOnDataReceived,// Callback to handle data received
            PFN_ONASYNCCOMPLETED pfnOnCompleted,   // Callback for transaction completion
            char*                Data)             // Custom data for the HTTP request
{
   char* AsyncPacket    = NULL;
   int   AsyncPacketSize= 0;
   char  FullCommand[WSA_STRING_MAXSIZE+32];

   if( g_AsyncInfo.in_use)
      return RTNet_QueueHttpAsyncTransaction(
            pCI,              // Connection Info
            type,             // Login | Command | Static | ...
            pfnOnDataReceived,// Callback to handle data received
            pfnOnCompleted,   // Callback for transaction completion
            Data);            // Custom data for the HTTP request

   if( ROADMAP_INVALID_SOCKET != g_AsyncInfo.Socket)
   {
      assert(0);
      return FALSE;
   }

   // Reset global context buffer:
   HttpAsyncTransactionInfo_Reset( &g_AsyncInfo);

   //   Load network parameters:
   if( !RTNet_LoadParams())
   {
      pCI->LastError = ERR_WrongNetworkSettings;
      roadmap_log( ROADMAP_ERROR, "RTNet_HttpAsyncTransaction() - 'RTNet_LoadParams()' had failed");
      return FALSE;
   }

   // Allocate buffer for the async-info object:
   AsyncPacketSize= HTTP_HEADER_MAX_SIZE + CUSTOM_HEADER_MAX_SIZE + strlen(Data);
   AsyncPacket    = RTPacket_Alloc( &(g_AsyncInfo.Packet), AsyncPacketSize);

   switch( type)
   {
      case login_transaction:
      case static_transaction:
      {
         const char* tag = "login";

         if( static_transaction == type)
            tag = "static";

         //   Sanity-1:   Are we already logged-in?
         if( pCI->bLoggedIn || (RT_INVALID_LOGINID_VALUE != pCI->iServerID))
         {
            roadmap_log( ROADMAP_ERROR, "RTNet_HttpAsyncTransaction() - Already logged in");
            RTPacket_Free( &(g_AsyncInfo.Packet));
            return FALSE;
         }

         //   Format packet:
         RTNET_PREPARE_NETPACKET_HTTPREQUEST(tag,Data);

         break;
      }

      case command_transaction:
      {
         if( !pCI->bLoggedIn || (RT_INVALID_LOGINID_VALUE == pCI->iServerID))
         {
            roadmap_log( ROADMAP_ERROR, "RTNet_HttpAsyncTransaction() - User is currently NOT logged in...");
            RTPacket_Free( &(g_AsyncInfo.Packet));
            return FALSE;
         }

         //   Prepare data:
         snprintf(AsyncPacket,
                  AsyncPacketSize,
                  RTNET_FORMAT_NETPACKET_OUR_HEADER( Data));
         strcpy( Data, AsyncPacket);

         //   Format packet:
         RTNET_PREPARE_NETPACKET_HTTPREQUEST("command",Data);

         break;
      }

      default:
         assert(0);
   }

   //    Inital values:
   g_AsyncInfo.in_use               = TRUE;
   g_AsyncInfo.pCI                  = pCI;
   g_AsyncInfo.pfnOnDataReceived    = pfnOnDataReceived;
   g_AsyncInfo.pfnOnAsyncCompleted  = pfnOnCompleted;

   //   Reset custom parser state:
   RTConnectionInfo_ResetParser( g_AsyncInfo.pCI);

   // Add request to queue:
   if( -1 == roadmap_net_connect_async("http_post",
                                       FullCommand,
                                       gs_WebServerPort,
                                       on_socket_connected,
                                       &g_AsyncInfo))
   {
      roadmap_log( ROADMAP_ERROR, "RTNet_HttpAsyncTransaction() - 'roadmap_net_connect_async' had failed (Invalid params or queue is full?)");
      HttpAsyncTransactionInfo_Reset( &g_AsyncInfo);
      return FALSE;
   }

   // Mark starting time:
   g_AsyncInfo.starting_time = time(NULL);

   return TRUE;
}

BOOL RTNet_HttpAsyncTransaction(
            LPRTConnectionInfo   pCI,              // Connection Info
            transaction_type     type,             // Login | Command | Static | ...
            PFN_ONDATARECEIVED   pfnOnDataReceived,// Callback to handle data received
            PFN_ONASYNCCOMPLETED pfnOnCompleted,   // Callback for transaction completion
            const char*          szFormat,         // Custom data for the HTTP request
            ...)                                   //    Parameters
{
   va_list  vl;
   int      i;
   RTPacket Packet;
   char*    Data;
   int      SizeNeeded;
   BOOL     bRes;

   if( !pfnOnCompleted || !pfnOnDataReceived || !pCI || !szFormat)
   {
      assert(0);
      return FALSE;
   }

   RTPacket_Init( &Packet);

   SizeNeeded  =  HTTP_HEADER_MAX_SIZE    +
                  CUSTOM_HEADER_MAX_SIZE  +
                  strlen(szFormat)        +
                  HttpAsyncTransaction_MAXIMUM_SIZE_NEEDED_FOR_ADDITIONAL_PARAMS;
   Data        = RTPacket_Alloc( &Packet, SizeNeeded);

   va_start(vl, szFormat);
   i = vsnprintf( Data, SizeNeeded, szFormat, vl);
   va_end(vl);

   if( i < 0)
   {
      roadmap_log( ROADMAP_ERROR, "RTNet_HttpAsyncTransaction() - Failed to format command '%s' (buffer size too small?)", szFormat);
      RTPacket_Free( &Packet);
      return FALSE;
   }

   bRes = RTNet_HttpAsyncTransaction_FormattedBuffer(
                                    pCI,              // Connection Info
                                    type,             // Login | Command | Static | ...
                                    pfnOnDataReceived,// Callback to handle data received
                                    pfnOnCompleted,   // Callback for transaction completion
                                    Data);            // Custom data for the HTTP request

   RTPacket_Free( &Packet);

   return bRes;
}

ETransactionResult on_data_received_( void* data, int size, void* context)
{
   LPHttpAsyncTransactionInfo pATI = (LPHttpAsyncTransactionInfo)context;

   LPResponseBuffer     pRB;
   BOOL*                pProcessingHttpHeader;
   ETransactionResult   eResult;   //   Default

   if( size < 0 )
   {
      roadmap_log( ROADMAP_ERROR, "on_data_received( SOCKET: %d) - 'roadmap_net_receive()' returned %d bytes", pATI->Socket, size);
      return transaction_failed;
   }

   if( !data || !pATI || !pATI->pCI)
   {
      roadmap_log( ROADMAP_ERROR, "on_data_received( SOCKET: %d) - One or more pointers are NULL", pATI->Socket);
      return transaction_failed;
   }

   // Where we asked to abort transaction?
   if( TS_Stopping == pATI->pCI->eTransactionStatus)
   {
      pATI->pCI->LastError = ERR_Aborted;
      roadmap_log( ROADMAP_ERROR, "on_data_received() - Was asked to abort session");
      return transaction_failed;
   }

   roadmap_log( ROADMAP_DEBUG, "on_data_received( SOCKET: %d) - Received %d bytes", pATI->Socket, size);

   pRB                  = &(pATI->API.RB);
   pProcessingHttpHeader= &(pATI->API.bProcessingHttpHeader);
   eResult              = transaction_failed;   //   Default

   //   Terminate data with NULL, so it can be processed as string:
   pRB->iDataSize += size;
   pRB->Data[ pRB->iDataSize] = '\0';

   // Data processing State Machine
   switch( (*pProcessingHttpHeader) ) {
   case TRUE:
       if( size == 0 ) {
           // Final packet: no more data will arrive.
           // We expect  HTTP headers -> report an error!
           eResult = transaction_failed;
           break;
       }

       //   Expecting http header:
      eResult = OnHttpResponse( pRB, &(pATI->API.eParserState));
      if( eResult != transaction_succeeded )
          break;

      (*pProcessingHttpHeader) = FALSE; // Switch state
      /* --fallthrough */
   case FALSE:
   default:
       if( size == 0 ) {
           // Final packet: no more data will arrive. If not all the data has been processed,
           //  that means that more data is required, and transaction data has been truncated.
           eResult = ( pRB->iProcessed < pRB->iDataSize ? transaction_failed : transaction_succeeded );
           break;
       }

       eResult = pATI->pfnOnDataReceived( pRB, pATI->pCI);
       break;
   }

   if( transaction_in_progress != eResult ) {
      roadmap_log(ROADMAP_DEBUG,
                  "on_data_received() - Finish to process all data; Status: %s",
                  ((transaction_succeeded==eResult)? "Succeeded": "Failed"));
      return eResult;
   }

   //   Recycle buffer:
   ResponseBuffer_Recycle( pRB );

   //   Sanity:
   if( (pRB->iFreeSize < 1) || (RESPONSE_BUFFER_SIZE < pRB->iFreeSize) )
   {
      roadmap_log( ROADMAP_ERROR, "on_data_received() - [BUG] Invalid value in 'iFreeSize': %d", pRB->iFreeSize);
      return transaction_failed;
   }

   //   Read next data
   if( !roadmap_net_async_receive( pATI->Socket, pRB->pNextRead, pRB->iFreeSize, on_data_received, pATI) )
   {
      roadmap_log( ROADMAP_ERROR, "on_data_received( SOCKET: %d) - 'roadmap_net_async_receive()' had failed", pATI->Socket);
      return transaction_failed;
   }

   pATI->async_receive_started = TRUE;
   return transaction_in_progress;
}

static void on_data_received( void* data, int size, void* context)
{
   ETransactionResult eResult = on_data_received_( data, size, context);

   switch(eResult)
   {
      case transaction_succeeded:
         RTNet_HttpAsyncTransactionCompleted( TRUE);
         break;

      case transaction_failed:
         RTNet_HttpAsyncTransactionCompleted( FALSE);
         break;

      case transaction_in_progress:
         break;
   }
}

BOOL RTNet_AsyncReceive( LPHttpAsyncTransactionInfo pATI)
{
   LPResponseBuffer pRB = &(pATI->API.RB);

   if( ROADMAP_INVALID_SOCKET == pATI->Socket)
   {
      roadmap_log( ROADMAP_ERROR, "RTNet_AsyncReceive() - Socket is invalid");
      return FALSE;
   }

   if( !pATI->pfnOnDataReceived)
   {
      roadmap_log( ROADMAP_ERROR, "RTNet_AsyncReceive() - Pointer to callback was not supplied");
      return FALSE;
   }

   //   Read next data
   if( !roadmap_net_async_receive( pATI->Socket, pRB->pNextRead, pRB->iFreeSize, on_data_received, pATI))
   {
      roadmap_log( ROADMAP_ERROR, "RTNet_AsyncReceive( SOCKET: %d) - 'roadmap_net_async_receive()' had failed", pATI->Socket);
      return FALSE;
   }

   pATI->async_receive_started = TRUE;
   return TRUE;
}
//////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////
ETransactionResult on_socket_connected_( RoadMapSocket Socket, void* context, network_error ec)
{
   LPHttpAsyncTransactionInfo pATI  = (LPHttpAsyncTransactionInfo)context;
   const char*                Packet;

   // Verify Socket is valid:
   if( ROADMAP_INVALID_SOCKET == Socket)
   {
      assert( neterr_success != ec);

      if(pATI && pATI->pCI)
      {
         pATI->pCI->LastError          = ERR_NetworkError;
         pATI->pCI->AdditionalErrorInfo= ec;
      }

      roadmap_log( ROADMAP_ERROR, "on_socket_connected() - INVALID SOCKET - Failed to create Socket");
      return transaction_failed;
   }

   assert( neterr_success == ec);

   if( !pATI || !pATI->pCI)
   {
      roadmap_log( ROADMAP_ERROR, "on_socket_connected() - One or more context-pointers are invalid");
      return transaction_failed;
   }

   // Where we asked to abort transaction?
   if( TS_Stopping == pATI->pCI->eTransactionStatus)
   {
      pATI->pCI->LastError = ERR_Aborted;
      roadmap_log( ROADMAP_ERROR, "on_socket_connected() - Was asked to abort session");
      return transaction_failed;
   }
   Packet      = RTPacket_GetBuffer( &(pATI->Packet));
   pATI->Socket= Socket;

   // Try to send packet:
   if( !RTNet_Send( Socket, Packet))
   {
      pATI->pCI->LastError = ERR_NetworkError;

      roadmap_log( ROADMAP_ERROR, "on_socket_connected() - 'RTNet_Send()' had failed");
      return transaction_failed;
   }

   if( !RTNet_AsyncReceive( pATI))
   {
      roadmap_log( ROADMAP_ERROR, "on_socket_connected() - 'RTNet_AsyncReceive()' had failed");
      return transaction_failed;
   }

   return transaction_in_progress;
}

void on_socket_connected( RoadMapSocket Socket, void* context, network_error ec)
{
   ETransactionResult eResult = on_socket_connected_( Socket, context, ec);

   switch(eResult)
   {
      case transaction_succeeded:
         RTNet_HttpAsyncTransactionCompleted( TRUE);
         break;

      case transaction_failed:
         RTNet_HttpAsyncTransactionCompleted( FALSE);
         break;

      case transaction_in_progress:
         break;
   }
}


//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL   RTNet_ReportOneSegment_Encode (char *packet, int iSegment)
{
   int trkseg;
   int line_flags;
   int num_shapes;
   int first_shape;
   int last_shape;
   const char *action;
   const char *status;
   RoadMapPosition from;
   RoadMapPosition to;
   int cfcc;
   int trkseg_flags;
   int from_point;
   int to_point;
   int from_id;
   int to_id;
   char from_string[DB_Point_STRING_MAXSIZE];
   char to_string[DB_Point_STRING_MAXSIZE];
   time_t start_time;
   time_t end_time;
   int num_attr;
   int street = -1;
   char name_string[513];
   char t2s_string[513];
   char city_string[513];

   if (editor_line_committed (iSegment))
   {
      *packet = '\0';
      return TRUE;
   }

   editor_line_get (iSegment, &from, &to, &trkseg, &cfcc, &line_flags);
   if (!(line_flags & ED_LINE_DIRTY))
   {
      *packet = '\0';
      return TRUE;
   }

   editor_trkseg_get (trkseg, NULL, &first_shape, &last_shape, &trkseg_flags);
   if (first_shape >= 0) num_shapes = last_shape - first_shape + 1;
   else                   num_shapes = 0;

   editor_trkseg_get_time (trkseg, &start_time, &end_time);

   editor_line_get_points (iSegment, &from_point, &to_point);
   from_id = editor_point_db_id (from_point);
   to_id = editor_point_db_id (to_point);

   format_DB_point_string (from_string, from.longitude, from.latitude, start_time, from_id);
   format_DB_point_string (to_string, to.longitude, to.latitude, end_time, to_id);

   if (line_flags & ED_LINE_DELETED) action = "delete";
   else                               action = "update";

   status = "fake"; // should be omitted from command line

   sprintf (packet,
            RTNET_FORMAT_NETPACKET_5ReportSegment,
            action,
            status,
            from_string,
            to_string,
            num_shapes * 3);

   if (num_shapes)
   {
      char shape_string[Point_STRING_MAXSIZE];
      int shape;

      RoadMapPosition curr_position = from;
      time_t curr_time = start_time;

      for (shape = first_shape; shape <= last_shape; shape++)
      {
         editor_shape_position (shape, &curr_position);
         editor_shape_time (shape, &curr_time);
         format_point_string (shape_string, curr_position.longitude, curr_position.latitude, curr_time);
         strcat (packet, shape_string);
      }
   }

   num_attr = 0;
   if ((line_flags & ED_LINE_DELETED) == 0)
   {
      editor_line_get_street (iSegment, &street);
      if (street >= 0)
      {
         num_attr = 4;

         PackNetworkString (editor_street_get_street_name (street), name_string, sizeof (name_string) - 1);
         PackNetworkString (editor_street_get_street_t2s (street),  t2s_string,  sizeof (t2s_string)  - 1);
         PackNetworkString (editor_street_get_street_city (street), city_string, sizeof (city_string) - 1);

         sprintf (packet + strlen (packet),
                  RTNET_FORMAT_NETPACKET_5ReportSegmentAttr,
                  num_attr * 2,
                  roadmap_layer_cfcc2type (cfcc),
                  name_string,
                  t2s_string,
                  city_string);
      }
   }

   if (num_attr == 0)
   {
      sprintf (packet + strlen (packet),
               RTNET_FORMAT_NETPACKET_1ReportSegmentNoAttr,
               num_attr * 2);
   }


   return TRUE;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
int   RTNet_ReportOneSegment_MaxLength (int iSegment)
{
   int trkseg;
   int flags;
   int num_shapes;
   int first_shape;
   int last_shape;
   int num_bytes;

   if (editor_line_committed (iSegment)) return 0;

   editor_line_get (iSegment, NULL, NULL, &trkseg, NULL, &flags);
   if (!(flags & ED_LINE_DIRTY)) return 0;

   editor_trkseg_get (trkseg, NULL, &first_shape, &last_shape, NULL);
   if (first_shape >= 0) num_shapes = last_shape - first_shape + 1;
   else                   num_shapes = 0;

   num_bytes =
      + 14                           // command
      + 7                           // action
      + 13                           // status
      + (12 + 12 + 12 + 12) * 2      // from,to
      + 12                           // num points
      + (12 + 12 + 12) * num_shapes // shape points
      + 12;                           // num attributes

   if ((flags & ED_LINE_DELETED) == 0)
   {
      num_bytes +=
         4 * (256 * 2 + 2);         // street attributes
   }

   num_bytes += 1024;               // just in case

   return num_bytes;
}

void    RTNet_Auth_BuildCommand (   char*               Command,
                                            int             ServerId,
                                            const char*     ServerCookie,
                                            const char*     UserName,
                                            int             DeviceId,
                                            const char*     Version)
{

    sprintf (Command, RTNET_FORMAT_NETPACKET_5Auth,
                ServerId, ServerCookie, UserName, DeviceId, Version);
}



