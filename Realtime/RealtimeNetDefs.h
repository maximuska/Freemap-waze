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

#ifndef   __FREEMAP_REALTIMENETDEFS_H__
#define   __FREEMAP_REALTIMENETDEFS_H__
//////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////
#include <time.h>
#include "RealtimeDefs.h"
#include "roadmap_net.h"
#include "../roadmap_gps.h"
//////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////
#define  RTNET_SESSION_TIME                     (75)  /* seconds */
#define  RTNET_RESPONSE_TAG_MAXSIZE             (31)
#define  RTNET_SERVERCOOKIE_MAXSIZE             (63)
#define  RTNET_WEBSERVICE_ADDRESS               ("")
#define  RTNET_PROTOCOL_VERSION                 (105)
#define  RTNET_PACKET_MAXSIZE                   MESSAGE_MAX_SIZE__AllTogether
#define  RTNET_PACKET_MAXSIZE__dynamic(_GPSPointsCount_,_NodesPointsCount_)      \
               MESSAGE_MAX_SIZE__AllTogether__dynamic(_GPSPointsCount_,_NodesPointsCount_)

#define  RTNET_HTTP_STATUS_STRING_MAXSIZE       (63)
#define  RTTRK_GPSPATH_MAX_POINTS               (100)
#define  RTTRK_NODEPATH_MAX_POINTS              (60)
#define  RTTRK_CREATENEWROADS_MAX_TOGGLES       (40)
#define  RTTRK_MIN_VARIANT_THRESHOLD            (5)
#define  RTTRK_MIN_DISTANCE_BETWEEN_POINTS      (2)
#define  RTTRK_MAX_DISTANCE_BETWEEN_POINTS      (1000)
#define  RTTRK_COMPRESSION_RATIO_THRESHOLD      (0.8F)
#define  RTTRK_MAX_SECONDS_BETWEEN_GPS_POINTS   (2)

#define  RTNET_SYSTEMMESSAGE_TITLE_MAXSIZE      (64)
#define  RTNET_SYSTEMMESSAGE_TEXT_MAXSIZE       (512)
//////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////
// BUFFER SIZES   //
////////////////////

// RTNet_HttpAsyncTransaction:
//    Q: How big should be the buffer to hold the formatted input string?
//    A: strlen(format) + (maximum number of place-holders (%) * size of its data)
//    The command with the highest number of place-holders is 'At' (RTNet_At)
//    with the following string:    "At,%f,%f,%f,%d,%f,%d,%d"
//    thus maximum additional size is:  20 20 20 10 20 10 10      (110)
//    To be on the safe-side, call it: 256
#define  HttpAsyncTransaction_MAXIMUM_SIZE_NEEDED_FOR_ADDITIONAL_PARAMS    (256)

#define  HTTP_HEADER_MAX_SIZE                                           \
         /* POST /rtserver/Login HTTP/1.0\r\n         */    (60 +       \
         /* Host: http://62.219.147.126:80\r\n        */     40 +       \
         /* User-Agent: FreeMap/1.0.0.0\r\n           */     32 +       \
         /* Content-type: binary/octet-stream\r\n     */     38 +       \
         /* Content-Length: 10000\r\n                 */     32 +       \
         /* \r\n                                      */      2)

#define  CUSTOM_HEADER_MAX_SIZE              (20 + RTNET_SERVERCOOKIE_MAXSIZE)
//             "UID,<%ddddddddd>,<RTNET_SERVERCOOKIE_MAXSIZE>\r\n"


//                                             4 float strings (coordinate)           ','
#define  RoadMapArea_STRING_MAXSIZE          ((4 * COORDINATE_VALUE_STRING_MAXSIZE) +  3)

//                                             3 float strings (coordinate)           speed/dir     ','
#define  RoadMapGpsPosition_STRING_MAXSIZE   ((3 * COORDINATE_VALUE_STRING_MAXSIZE) + (2 * 10)   +   4)

//                                             2 float strings (coordinate)          ','
#define  RoadMapPosition_STRING_MAXSIZE      ((2 * COORDINATE_VALUE_STRING_MAXSIZE) + 2 + 1)

#define  DB_Point_STRING_MAXSIZE             (49)
#define  Point_STRING_MAXSIZE                (37)

#define  MESSAGE_MAX_SIZE__At                (32 + RoadMapGpsPosition_STRING_MAXSIZE)
#define  MESSAGE_MAX_SIZE__MapDisplyed       (100)
#define  MESSAGE_MAX_SIZE__SeeMe             (20)
#define  MESSAGE_MAX_SIZE__CreateNewRoads    (14 + 1 + 10 + 1 + 1 + 1)
#define MESSAGE_MAX_SIZE__Auth                  (4 + 1 + \
                                                             10 + 1 + \
                                                             RTNET_SERVERCOOKIE_MAXSIZE + 1 + \
                                                             RT_USERNM_MAXSIZE * 2 + 1 + \
                                                             10 + 1 + \
                                                             64)


#define  MESSAGE_MAX_SIZE__AllTogether             \
         (  HTTP_HEADER_MAX_SIZE                +  \
            CUSTOM_HEADER_MAX_SIZE              +  \
            MESSAGE_MAX_SIZE__At                +  \
            MESSAGE_MAX_SIZE__MapDisplyed       +  \
            MESSAGE_MAX_SIZE__CreateNewRoads    +  \
            RTNET_GPSPATH_BUFFERSIZE            +  \
            RTNET_NODEPATH_BUFFERSIZE           +  \
            RTNET_CREATENEWROADS_BUFFERSIZE     +  \
            MESSAGE_MAX_SIZE__SeeMe)
#define  MESSAGE_MAX_SIZE__AllTogether__dynamic(_GPSPointsCount_,_NodesPointsCount_,_AllowNewRoadCount_) \
         (  HTTP_HEADER_MAX_SIZE                                           +  \
            CUSTOM_HEADER_MAX_SIZE                                         +  \
            MESSAGE_MAX_SIZE__At                                           +  \
            MESSAGE_MAX_SIZE__MapDisplyed                                  +  \
            MESSAGE_MAX_SIZE__CreateNewRoads                               +  \
            RTNET_GPSPATH_BUFFERSIZE__dynamic(_GPSPointsCount_)            +  \
            RTNET_NODEPATH_BUFFERSIZE__dynamic(_NodesPointsCount_)         +  \
            RTNET_CREATENEWROADS_BUFFERSIZE__dynamic(_AllowNewRoadCount_)  +  \
            MESSAGE_MAX_SIZE__SeeMe)

// Buffer sizes for GPS path string
// --------------------------------
//    Command name (GPSPath)                             ==  7 chars
//    Time offset:                  <32>,                == 33 chars
//    First row (row count)         <10>,                == 11 chars
//    A single row:                 <32>,<32>,<10>,      == 74 chars
//      Optional GPSDisconnect directive        <15>
#define  RTNET_GPSPATH_BUFFERSIZE_command_name           ( 7)
#define  RTNET_GPSPATH_BUFFERSIZE_time_offset            (33)
#define  RTNET_GPSPATH_BUFFERSIZE_row_count              (11)
#define  RTNET_GPSPATH_BUFFERSIZE_single_row             (74+4+15)
#define  RTNET_GPSPATH_BUFFERSIZE_rows                                  \
                           (RTNET_GPSPATH_BUFFERSIZE_single_row * RTTRK_GPSPATH_MAX_POINTS)
#define  RTNET_GPSPATH_BUFFERSIZE_rows__dynamic(_n_)                    \
                           (RTNET_GPSPATH_BUFFERSIZE_single_row * _n_)

#define  RTNET_GPSPATH_BUFFERSIZE                                       \
                           (RTNET_GPSPATH_BUFFERSIZE_command_name    +  \
                            RTNET_GPSPATH_BUFFERSIZE_time_offset     +  \
                            RTNET_GPSPATH_BUFFERSIZE_row_count       +  \
                            RTNET_GPSPATH_BUFFERSIZE_rows)

#define  RTNET_GPSPATH_BUFFERSIZE__dynamic(_n_)                         \
                           (RTNET_GPSPATH_BUFFERSIZE_command_name    +  \
                            RTNET_GPSPATH_BUFFERSIZE_time_offset     +  \
                            RTNET_GPSPATH_BUFFERSIZE_row_count       +  \
                            RTNET_GPSPATH_BUFFERSIZE_rows__dynamic(_n_))

#define   RTNET_CREATENEWROADS_BUFFERSIZE                                  \
                           (MESSAGE_MAX_SIZE__CreateNewRoads * RTTRK_CREATENEWROADS_MAX_TOGGLES)

#define   RTNET_CREATENEWROADS_BUFFERSIZE__dynamic(_n_)                    \
                           (MESSAGE_MAX_SIZE__CreateNewRoads * (_n_))


// Buffer sizes for Node path string
// ---------------------------------
//    Command name (NodePath)                       ==  8 chars
//    Time offset:                  <32>,           == 33 chars
//    First row (row count)         <10>            == 11 chars
//    A single row:                 <10>,<10>,      == 22 chars
#define  RTNET_NODEPATH_BUFFERSIZE_command_name           ( 7)
#define  RTNET_NODEPATH_BUFFERSIZE_time_offset            (33)
#define  RTNET_NODEPATH_BUFFERSIZE_row_count              (11)
#define  RTNET_NODEPATH_BUFFERSIZE_single_row             (22)
#define  RTNET_NODEPATH_BUFFERSIZE_temp                   (33)
#define  RTNET_NODEPATH_BUFFERSIZE_rows                   \
                           (RTNET_NODEPATH_BUFFERSIZE_single_row * RTTRK_NODEPATH_MAX_POINTS)
#define  RTNET_NODEPATH_BUFFERSIZE_rows__dynamic(_n_)                    \
                           (RTNET_NODEPATH_BUFFERSIZE_single_row * _n_)

#define  RTNET_NODEPATH_BUFFERSIZE                                      \
                           (RTNET_NODEPATH_BUFFERSIZE_command_name    + \
                            RTNET_NODEPATH_BUFFERSIZE_time_offset     + \
                            RTNET_NODEPATH_BUFFERSIZE_row_count       + \
                            RTNET_NODEPATH_BUFFERSIZE_rows)

#define  RTNET_NODEPATH_BUFFERSIZE__dynamic(_n_)                        \
                           (RTNET_NODEPATH_BUFFERSIZE_command_name    + \
                            RTNET_NODEPATH_BUFFERSIZE_time_offset     + \
                            RTNET_NODEPATH_BUFFERSIZE_row_count       + \
                            RTNET_NODEPATH_BUFFERSIZE_rows__dynamic(_n_))
//////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////

//   POST requests from the server:
typedef enum tagERTPacketRequest
{
   REQ_Login,
   REQ_GuestLogin,
   REQ_LogOut,
   REQ_At,
   REQ_MapDisplayed,
   REQ_SeeMe,
   REQ_StartFollowUsers,
   REQ_StopFollowUsers,

   REQ__count,
   REQ__invalid

}  ERTPacketRequest;

const char*       ERTPacketRequest_to_string( ERTPacketRequest e);
ERTPacketRequest  ERTPacketRequest_from_string( const char* szE);

//   Server responses to the above requests:
typedef enum tagERTPacketResponse
{
   RES_LoginSuccessful,
   RES_LoginError,
   RES_LogoutSuccessful,
   RES_AddUser,
   RES_UpdateUserLocation,
   RES_RmUser,
   RES_ReturnAfter,
   RES_Fatal,
   RES_Warning,
   RES_AddAlert,
   RES_RemoveAlert,
   RES_AddAlertComment,
   RES_SystemMessage,
   RES_VersionUpgrade,
   RES_AddRoadInfo,
   RES_RmRoadInfo,

   /* Must be last! */
   RES__count,
   RES__invalid

}  ERTPacketResponse;

const char*         ERTPacketResponse_to_string( ERTPacketResponse e);
ERTPacketResponse   ERTPacketResponse_from_string( const char* szE);
//////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////
//   Response Parser State
typedef enum tagEParserState
{
   PS_Initialized,
   PS_StatusVerified,
   PS_Completed

}  EParserState;

typedef enum tagERealtimeErrors
{
   ERR_Success,

   ERR_NoDataToSend,
   ERR_GeneralError,
   ERR_NetworkError,
   ERR_Aborted,
   ERR_SessionTimedout,
   ERR_ParsingError,
   ERR_LoginFailed,
   ERR_WrongNameOrPassword,
   ERR_WrongNetworkSettings,
   ERR_UnknownLoginID            // Server lost link to our login-id (maybe server was restarted?)

}  ERTErrorCode;

const char* GetErrorString( ERTErrorCode e);
//////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////
//   Response Parser
//      Structure to hold details of the custom status ("200 OK")
typedef struct tagRTResponseStatus
{
   int   iRC;
   char  RC[RTNET_HTTP_STATUS_STRING_MAXSIZE+1];

}  RTResponseStatus, *LPRTResponseStatus;

void                 RTResponseStatus_Init( LPRTResponseStatus this);
ETransactionResult   RTResponseStatus_Load( LPRTResponseStatus this, const char* szResponse, int* pBytesRead);
//////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////
//   Connection status information
typedef struct tagRTConnectionInfo
{
/* 1*/   char                 UserNm      [RT_USERNM_MAXSIZE         +1];
/* 2*/   char                 UserPW      [RT_USERPW_MAXSIZE         +1];
/* 3*/   char                 UserNk      [RT_USERNK_MAXSIZE         +1];
/* 4*/   char                 ServerCookie[RTNET_SERVERCOOKIE_MAXSIZE+1];
/* 5*/   BOOL                 bLoggedIn;
/* 6*/   int                  iServerID;
/* 7*/   RoadMapArea          LastMapPosSent;
/* 8*/   RTUsers              Users;
/* 9*/   ETransactionStatus   eTransactionStatus;
/*10*/   EParserState         eParserState;
/*11*/   ERTErrorCode         LastError;
/*12*/   int                  AdditionalErrorInfo;

}  RTConnectionInfo, *LPRTConnectionInfo;

void RTConnectionInfo_Init             ( LPRTConnectionInfo this, PFN_ONUSER pfnOnAddUser, PFN_ONUSER pfnOnMoveUser, PFN_ONUSER pfnOnRemoveUser);
void RTConnectionInfo_FullReset        ( LPRTConnectionInfo this);
void RTConnectionInfo_ResetLogin       ( LPRTConnectionInfo this);
void RTConnectionInfo_ResetTransaction ( LPRTConnectionInfo this);
void RTConnectionInfo_ResetParser      ( LPRTConnectionInfo this);
//////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////
#define  RESPONSE_BUFFER_SIZE       (2 * 1024)

//   Response Parser
//      Structure used to follow the parser state
typedef struct tagResponseBuffer
{
                                       //   The data.
   BYTE  Data[RESPONSE_BUFFER_SIZE+1]; //      Data is a NULL-terminated string

                                       //   For each 'read' iteration:
   int   iDataSize;                    //   o   Size of the data read into the buffer 'Data'
   int   iProcessed;                   //   o   Out of 'iDataSize', how much was processed

                                       //   For the overall transaction:
   int   iCustomDataSize;              //   o   Overall size of all custom data
   int   iCustomDataProcessed;         //   o   Out of 'iCustomDataSize', how much was processed

   //   Internal usage:
   BYTE* pNextRead;
   int   iFreeSize;

}   ResponseBuffer, *LPResponseBuffer;

void ResponseBuffer_Init      ( LPResponseBuffer this);
void ResponseBuffer_Recycle   ( LPResponseBuffer this);
//////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////
//   Prototype for parser callback method:
typedef ETransactionResult(*PFN_ONDATARECEIVED)( LPResponseBuffer pRI, void* pContext);

//   Callbacks for data processing:
ETransactionResult   OnHttpResponse    ( LPResponseBuffer pRI, void* pContext);
ETransactionResult   OnLoginResponse   ( LPResponseBuffer pRI, void* pContext);
ETransactionResult   OnRegisterResponse( LPResponseBuffer pRI, void* pContext);
ETransactionResult   OnLogOutResponse  ( LPResponseBuffer pRI, void* pContext);
ETransactionResult   OnGeneralResponse ( LPResponseBuffer pRI, void* pContext);
//////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////
#define   RTNET_PREPARE_NETPACKET_HTTPREQUEST(_hdrTag_,_data_)    \
               snprintf(AsyncPacket,                              \
                        AsyncPacketSize,                          \
                        "Content-type: binary/octet-stream\r\n"   \
                        "Content-Length: %ld\r\n"                 \
                        "\r\n"                                    \
                        "%s",                                     \
                        strlen( _data_),                          \
                        _data_);                                  \
                                                                  \
               snprintf(FullCommand,                              \
                        sizeof(FullCommand),                      \
                        "%s/%s",                                  \
                        gs_WebServiceAddress,                     \
                        _hdrTag_)

#define   RTNET_FORMAT_NETPACKET_OUR_HEADER(_command_)                  \
         "UID,%d,%s\r\n%s",                                             \
         pCI->iServerID, pCI->ServerCookie,_command_

#define   RTNET_CALC_BYTES_PROCESSED(_CurrentOffset_)                   \
         (int)((size_t)_CurrentOffset_ - (size_t)pRI->Data);

#define   RTNET_ONRESPONSE_BEGIN(_method_,_expectedTag_)                \
   LPRTConnectionInfo   pCI = (LPRTConnectionInfo)pContext;             \
                                                                        \
   /* Verify status (OK), and verify the expected tag             */    \
   if( PS_Initialized == pCI->eParserState)                             \
   {                                                                    \
      ETransactionResult   Res;                                         \
                                                                        \
      /* Verify we have any data:                                 */    \
      Res = VerifyStatusAndTag( pRI, _expectedTag_, &(pCI->LastError)); \
      if( transaction_succeeded != Res)                                 \
      {                                                                 \
         if( transaction_failed == Res)                                 \
            roadmap_log( ROADMAP_ERROR, "RTNet::%s() - 'VerifyStatusAndTag()' had failed", _method_);   \
/*       Else                                                   */      \
/*          transaction_in_progress  (Keep on reading data...)  */      \
                                                                        \
         return Res;                                                    \
      }                                                                 \
                                                                        \
      pCI->eParserState = PS_StatusVerified;                            \
   }

#define   RTNET_SERVER_REQUEST_HANDLER(_case_)                          \
         case RES_##_case_:                                             \
         {                                                              \
            bHandlerRes = _case_( pRI, pCI, pNext);                     \
            break;                                                      \
         }


#define  RTNET_FORMAT_NETPACKET_6Login                ("Login,%d,%s,%s,%d,%s,%s")
#define  RTNET_FORMAT_NETPACKET_3Register             ("Register,%d,%d,%s")
#define  RTNET_FORMAT_NETPACKET_3GuestLogin           ("GuestLogin,%d,%d,%s")
#define  RTNET_FORMAT_NETPACKET_3At                   ("At,%s,%d,%d\n")
#define  RTNET_FORMAT_NETPACKET_2MapDisplayed         ("MapDisplayed,%s,%u\n")
#define  RTNET_FORMAT_NETPACKET_6ReportMarker         ("SubmitMarker,%s,%s,%s,%d,%s,%s\n")
#define  RTNET_FORMAT_NETPACKET_5ReportSegment        ("SubmitSegment,%s,%s,%s,%s,%d")
#define  RTNET_FORMAT_NETPACKET_5ReportSegmentAttr    (",%d,road_type,%s,street_name,%s,test2speech,%s,city_name,%s\n")
#define  RTNET_FORMAT_NETPACKET_1ReportSegmentNoAttr  (",%d\n")
#define  RTNET_FORMAT_NETPACKET_2CreateNewRoads       ("CreateNewRoads,%u,%s\n")
#define  RTNET_FORMAT_NETPACKET_4NavigateTo           ("NavigateTo,%s,,%s,%s,%s\n")
#define  RTNET_FORMAT_NETPACKET_5Auth                 ("Auth,%d,%s,%s,%d,%s\n")
#define  RTNET_FORMAT_NETPACKET_1SendSMS              ("BridgeTo,DOWNLOADSMS,send_download,2,phone_number,%s\n")
//////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////
#include "RealtimePacket.h"

typedef void(*PFN_ONASYNCCOMPLETED)(BOOL bResult);

typedef struct tagHttpAsyncParserInfo
{
   ResponseBuffer RB;
   EParserState   eParserState;
   BOOL           bProcessingHttpHeader;

}     HttpAsyncParserInfo, *LPHttpAsyncParserInfo;
void  HttpAsyncParserInfo_Init( LPHttpAsyncParserInfo this);

typedef struct tagHttpAsyncTransactionInfo
{
   LPRTConnectionInfo   pCI;
   PFN_ONDATARECEIVED   pfnOnDataReceived;
   PFN_ONASYNCCOMPLETED pfnOnAsyncCompleted;
   RTPacket             Packet;
   RoadMapSocket        Socket;
   BOOL                 in_use;
   HttpAsyncParserInfo  API;
   BOOL                 async_receive_started;
   time_t               starting_time;

}     HttpAsyncTransactionInfo, *LPHttpAsyncTransactionInfo;
void  HttpAsyncTransactionInfo_Init  ( LPHttpAsyncTransactionInfo this);
void  HttpAsyncTransactionInfo_Reset ( LPHttpAsyncTransactionInfo this);

typedef enum tag_transaction_type
{
   login_transaction,
   command_transaction,
   static_transaction

}  transaction_type;

BOOL RTNet_HttpAsyncTransaction(
               LPRTConnectionInfo   pCI,                 // Connection Info
               transaction_type     type,                // Login | Command | Static | ...
               PFN_ONDATARECEIVED   pfnOnDataReceived,   // Callback to handle data received
               PFN_ONASYNCCOMPLETED pfnOnCompleted,      // Callback for a-sync trans completion
               const char*          szFormat,            // Custom data for the HTTP request
               ...);                                     // Parameters

// Same function, but buffer is ready formatted:
BOOL RTNet_HttpAsyncTransaction_FormattedBuffer(
               LPRTConnectionInfo   pCI,                 // Connection Info
               transaction_type     type,                // Login | Command | Static | ...
               PFN_ONDATARECEIVED   pfnOnDataReceived,   // Callback to handle data received
               PFN_ONASYNCCOMPLETED pfnOnCompleted,      // Callback for a-sync trans completion
               char*                Data);               // Custom data for the HTTP request

//////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////
#endif   //   __FREEMAP_REALTIMENETDEFS_H__
