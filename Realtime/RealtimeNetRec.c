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
#include "RealtimeString.h"
#include "../roadmap_messagebox.h"
#include "RealtimeNet.h"
#include "WebServiceAddress.h" 
#include "RealtimeAlerts.h"
#include "RealtimeTrafficInfo.h"
#include "RealtimeOffline.h"

//////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////
//   Forward declaration for server requests handlers:
BOOL  AddUser        ( LPResponseBuffer pRI, LPRTConnectionInfo   pCI, const char* pNext);
BOOL  AddAlert       ( LPResponseBuffer pRI, LPRTConnectionInfo   pCI, const char* pNext);
BOOL  AddAlertComment( LPResponseBuffer pRI, LPRTConnectionInfo   pCI, const char* pNext);
BOOL  RemoveAlert    ( LPResponseBuffer pRI, LPRTConnectionInfo   pCI, const char* pNext);
BOOL  SystemMessage  ( LPResponseBuffer pRI, LPRTConnectionInfo   pCI, const char* pNext);
BOOL  VersionUpgrade ( LPResponseBuffer pRI, LPRTConnectionInfo   pCI, const char* pNext);
BOOL  AddRoadInfo    ( LPResponseBuffer pRI, LPRTConnectionInfo   pCI, const char* pNext);
BOOL  RmRoadInfo     ( LPResponseBuffer pRI, LPRTConnectionInfo   pCI, const char* pNext);
//////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////
void ResponseBuffer_Init( LPResponseBuffer this)
{
   memset( this->Data, 0, sizeof(this->Data));
   this->iDataSize           =  0; //   Size of the data read into the buffer (this->Data)
   this->iProcessed          =  0; //   Out of 'iDataSize', how much was processed
   this->iCustomDataSize     = -1; //   Overall size of all custom data
   this->iCustomDataProcessed=  0; //   Out of 'iCustomDataSize', how much was processed
   
   //   Internal usage:
   this->pNextRead           = this->Data;
   this->iFreeSize           = RESPONSE_BUFFER_SIZE;
}

//   Recylce buffer data, before going into the next 'read' statement
void ResponseBuffer_Recycle( LPResponseBuffer this)
{
   //   Update overall bytes processed:
   this->iCustomDataProcessed += this->iProcessed;

   //   Did we process LESS THEN buffer size?
   if( this->iProcessed < this->iDataSize)
   {
      //   Yes
      if( this->iProcessed)
      {
         //   If anything was processed, then loose processed data, and copy the 
         //      unprocessed-data to the buffer beginning:
         int i;
         int iRemaining = (this->iDataSize - this->iProcessed);
         
         //   Shift data:
         for( i=0; i<iRemaining; i++)
            this->Data[i]   = this->Data[this->iProcessed + i];

         this->Data[i]    = '\0';         //   Terminate string with a NULL
         this->iDataSize  = iRemaining;   //   Update data size to unprocessed-data size
      }

      //   Internal usage:
      this->pNextRead = (this->Data         + this->iDataSize);
      this->iFreeSize = (RESPONSE_BUFFER_SIZE- this->iDataSize);
   }
   else
   {
      //   All data was processed.
      //      Reset pointers and data
         
      this->iDataSize  = 0;
      this->Data[0]    = '\0';
   
      //   Internal usage:
      this->pNextRead  = this->Data;
      this->iFreeSize  = RESPONSE_BUFFER_SIZE;
   }

   //   Anyhow, in new buffer, nothing is processed:
   this->iProcessed = 0;
}
//////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////
void RTResponseStatus_Init( LPRTResponseStatus this)
{
   this->iRC   = -1;
   memset( this->RC, 0, sizeof(this->RC));
}

ETransactionResult RTResponseStatus_Load( LPRTResponseStatus   this, 
                                          const char*          szResponse, 
                                          int*                 pBytesRead)
{
   int         iRC = 0;
   const char* p   = szResponse;
   int         iBufferSize;
   
   RTResponseStatus_Init( this);
   
   //   Expected string:               RC,<int>,<string>\r\n
   //   Minimum size for Expected string:   6

   if( !p || !pBytesRead)
      return transaction_failed;
   
   (*pBytesRead) = 0;
   
   if( !(*p) || (NULL == strchr( p, '\n')))
      return transaction_in_progress;   //   Continue reading...

   if( strncmp( p, "RC,", 3))
      return transaction_failed;
   p+=3;   //   Jump over the RC

   //   Get status code:
   p = ReadIntFromString(  p,                //   [in]      Source string
                           ",",              //   [in,opt]  Value termination
                           NULL,             //   [in,opt]  Allowed padding
                           &iRC,             //   [out]     Put it here
                           TRIM_ALL_CHARS);  //   [in]      Remove additional termination CHARS

   if( !p || !(*p)|| !iRC)
      return transaction_failed;
      
   //   Copy exracted value:
   this->iRC = iRC;
   
   //   Copy status string:
   iBufferSize = RTNET_HTTP_STATUS_STRING_MAXSIZE;
   p           = ExtractNetworkString(  
                  p,                // [in]     Source string
                  this->RC,         // [out,opt]Output buffer
                  &iBufferSize,     // [in,out] Buffer size / Size of extracted string
                  "\r\n",           // [in]     Array of chars to terminate the copy operation
                  TRIM_ALL_CHARS);  // [in]     Remove additional termination chars
   if( !p)
      return transaction_failed;
      
   //   Update [out] parameter 'pBytesRead':
   if( !(*p))
      (*pBytesRead) = (int)strlen( szResponse);
   else
      (*pBytesRead) = (int)((size_t)p - (size_t)szResponse);

   return transaction_succeeded;
}
//////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////
//   General HTTP packet parser
//   Used prior to any response by all response-cases
ETransactionResult OnHttpResponse( LPResponseBuffer pRI, void* pContext)
{
   EParserState*  pState = (EParserState*)pContext;
   const char*    pDataSize;
   const char*    pHeaderEnd;
   int            iCustomDataSize = 0;

   //   Did we find the 'OK' status already?
   if( (*pState) != PS_StatusVerified)
   {
      char* szDelimiter = strstr( (const char*)pRI->Data, "\r\n");
      if( !szDelimiter)
         return transaction_in_progress;   //   Continue reading...

      // Lower case:
      ToLowerN( (char*)pRI->Data, (size_t)(szDelimiter - (char*)pRI->Data));
      
      //   Verify we have the '200 OK' status:
      if( !strstr( (const char*)pRI->Data, "200 ok"))
      {
         roadmap_log( ROADMAP_ERROR, "RTNet::OnHttpResponse() - Response not successfull (%s)", pRI->Data);
         return transaction_failed;         //   Quit reading loop
      }
      
      //   Found the OK status:
      (*pState) = PS_StatusVerified;
   }
   
   //   Search for the data sign:
   pHeaderEnd = strstr( (const char*)pRI->Data, "\r\n\r\n");
   if( !pHeaderEnd)
      return transaction_in_progress;   //   Continue reading...
   
   // Lower case:
   ToLowerN( (char*)pRI->Data, (size_t)(pHeaderEnd - (char*)pRI->Data));
   
   //   Search for content length:
   pDataSize = strstr( (const char*)pRI->Data, "content-length:");
   if( !pDataSize)
   {
      roadmap_log( ROADMAP_ERROR, "RTNet::OnHttpResponse() - Did not find 'Content-Length:' in response (%s)", pRI->Data);
      return transaction_failed;         //   Quit reading loop
   }
   
   //   Move pointer:
   pDataSize += strlen("Content-Length:");
   //   Read size:
   pDataSize  = ReadIntFromString(   
                     pDataSize,           //   [in]     Source string
                     "\r\n",              //   [in,opt] Value termination
                     " ",                 //   [in,opt] Allowed padding
                     &iCustomDataSize,    //   [out]    Put it here
                     DO_NOT_TRIM);        //   [in]     Remove additional termination chars

   if( !pDataSize || !(*pDataSize)|| !iCustomDataSize)
   {
      roadmap_log( ROADMAP_ERROR, "RTNet::OnHttpResponse() - Did not find custom-data size value in the response (%s)", pRI->Data);
      return transaction_failed;      //   Quit reading loop
   }
   
   //   Log on findings:
   roadmap_log( ROADMAP_DEBUG, "RTNet::OnHttpResponse() - Custom data size: %d; Packet: '%s'", iCustomDataSize, pRI->Data);
   pRI->iCustomDataSize = iCustomDataSize;
   (*pState)            = PS_Completed;

   //   Update buffer processed-size:
   pRI->iProcessed = (int)(((size_t)pHeaderEnd  + strlen("\r\n\r\n")) - (size_t)(pRI->Data));
   
   //   Note on the 'ResponseBuffer_Recycle( pRI)' call below:
   //      This method also automatically updates the 'iCustomDataProcessed', which keeps track 
   //      on how much of the custom data was already processed.
   //      The call below is the only case when 'iProcessed' (has a positive value and) refers
   //      to bytes, which are not part of the custom data.
   //      For this reason, after calling 'ResponseBuffer_Recycle()' we will manualy 
   //      reset the value of 'iCustomDataProcessed' to zero:
   ResponseBuffer_Recycle( pRI);
   pRI->iCustomDataProcessed = 0;
   
   return transaction_succeeded;      //   Quit loop
}

ETransactionResult VerifyStatusAndTag( LPResponseBuffer pRI, const char* szExpectedTag, ERTErrorCode* Error)
{
   char                 Tag[RTNET_RESPONSE_TAG_MAXSIZE+1];
   RTResponseStatus     Status;
   ETransactionResult   Res;
   int                  iBytesRead     = 0;
   const char*          szTagOffset    = NULL;
   BOOL                 bTryingToLogin = FALSE;
   int                  iBufferSize;
   
   (*Error) = ERR_Success;

   if( szExpectedTag && (*szExpectedTag) && !strcmp( szExpectedTag, "LoginSuccessful"))
	   bTryingToLogin = TRUE;
   
   //   Verify we have any data:
   if( pRI->iCustomDataSize < 1)
   {
      (*Error) = ERR_ParsingError;
      roadmap_log( ROADMAP_ERROR, "RTNet::VerifyStatusAndTag() - Invalid custom data size (%d)", pRI->iCustomDataSize);
      return transaction_failed;   //   Quit the 'receive' loop
   }
   
   //   Search for response status:
   Res = RTResponseStatus_Load( &Status, (const char*)pRI->Data, &iBytesRead);
   if( transaction_succeeded != Res)
   {
      if( transaction_failed == Res)
      {
         (*Error) = ERR_ParsingError;
         roadmap_log( ROADMAP_ERROR, "RTNet::VerifyStatusAndTag() - Failed to read server response (%s)", pRI->Data);
      }
      
      return Res;
   }
   
   //   Verify response is ok:
   if( 200 != Status.iRC)
   {
      switch( Status.iRC)
      {
         case 501:
            (*Error) = ERR_UnknownLoginID;
            break;
         
         default:
         {
            if( bTryingToLogin)
               (*Error) = ERR_LoginFailed;
            else
               (*Error) = ERR_GeneralError;
            
            break;
         }
      }
   
      roadmap_log( ROADMAP_ERROR, "RTNet::VerifyStatusAndTag() - Server response is %d: '%s'", Status.iRC, Status.RC);
      return transaction_failed;   //   Quit the 'receive' loop
   }
   
   //////////////////////////////////
   //   Response status is Cosher   //
   //////////////////////////////////
   

   //   Where we asked to read tag?
   if( !szExpectedTag)
   {
      //   Update bytes-processed:
      pRI->iProcessed = iBytesRead;
      
      return transaction_succeeded;
   }

   //   Set response-tag offset:
   szTagOffset = (const char*)pRI->Data + iBytesRead;
   
   //   Before parsing - see if we have the whole line:
   if( NULL == strchr( szTagOffset, '\n'))
      return transaction_in_progress;   //   Continue reading...
   
   iBufferSize = RTNET_RESPONSE_TAG_MAXSIZE;
   szTagOffset = ExtractNetworkString(   
                  szTagOffset,      // [in]     Source string
                  Tag,              // [out,opt]Output buffer
                  &iBufferSize,     // [in,out] Buffer size / Size of extracted string
                  ",\r\n",          // [in]     Array of chars to terminate the copy operation
                  TRIM_ALL_CHARS);  // [in]     Remove additional termination chars
   if( !szTagOffset)
   {
      (*Error) = ERR_ParsingError;
      roadmap_log( ROADMAP_ERROR, "RTNet::VerifyStatusAndTag() - Failed to read server-response tag");
      return transaction_failed;   //   Quit the 'receive' loop
   }
   
   //   Found expected tag:
   if( strcmp( szExpectedTag, Tag))
   {
      (*Error) = ERR_ParsingError;

      // Special case:  login error
      if( bTryingToLogin && !strcmp( Tag, "LoginError"))
      {
         (*Error) = ERR_LoginFailed;
         
         if( szTagOffset && (*szTagOffset))
         {
            int iErrorCode = atoi(szTagOffset);
            if( 1 == iErrorCode)
               (*Error) = ERR_WrongNameOrPassword;
         }
      }
   
      roadmap_log( ROADMAP_ERROR, "RTNet::VerifyStatusAndTag() - Tag found (%s) is not the expected tag (%s)",
                  Tag, szExpectedTag);
      return transaction_failed;   //   Quit the 'receive' loop
   }
   
   //   Update bytes-processed:
   pRI->iProcessed  = RTNET_CALC_BYTES_PROCESSED( szTagOffset);

   return transaction_succeeded;
}

ETransactionResult OnRegisterResponse( LPResponseBuffer pRI, void* pContext)
{
   const char* pNext = NULL;
   int         iBufferSize;

   RTNET_ONRESPONSE_BEGIN( "OnRegisterResponse", "RegisterSuccessful")

   //   Move data pointer:
   pNext = (const char*)pRI->Data + pRI->iProcessed;
   
   // 1. Auto generated name   
   iBufferSize = RT_USERNM_MAXSIZE;
   pNext       = ExtractNetworkString(   
               pNext,               // [in]     Source string
               pCI->UserNm,         // [out,opt]Output buffer
               &iBufferSize,        // [in,out] Buffer size / Size of extracted string
               ",",                 // [in]     Array of chars to terminate the copy operation
               TRIM_ALL_CHARS);     // [in]     Remove additional termination chars
   if( !pNext)
   {
      roadmap_log( ROADMAP_ERROR, "RTNet::OnRegisterResponse() - Did not find (auto generated) user-name in the response");
      return transaction_failed;   //   Quit the 'receive' loop
   }
   
   // 2. Auto generated pw
   iBufferSize = RT_USERPW_MAXSIZE;
   pNext       = ExtractNetworkString(   
               pNext,               // [in]     Source string
               pCI->UserPW,         // [out,opt]Output buffer
               &iBufferSize,        // [in,out] Buffer size / Size of extracted string
               ",\r\n",             // [in]     Array of chars to terminate the copy operation
               TRIM_ALL_CHARS);     // [in]     Remove additional termination chars
   if( !pNext)
   {
      roadmap_log( ROADMAP_ERROR, "RTNet::OnRegisterResponse() - Did not find (auto generated) user-pw in the response");
      return transaction_failed;   //   Quit the 'receive' loop
   }
   
   // Reset nick-name (not relevant here)
   memset( pCI->UserNk, 0, RT_USERNK_MAXSIZE);
   
   //   Done
   
   //   Update status variables:
   pRI->iProcessed   = (int)((size_t)pNext + strlen( pNext) - (size_t)pRI->Data);
   pCI->eParserState = PS_Completed;
   
   return transaction_succeeded;   
}

ETransactionResult OnLoginResponse( LPResponseBuffer pRI, void* pContext)
{
   const char* pNext = NULL;
   const char* pLast = NULL;   //   For logging
   int         iBufferSize;

   RTNET_ONRESPONSE_BEGIN( "OnLoginResponse", "LoginSuccessful")

   //   Move data pointer:
   pNext = (const char*)pRI->Data + pRI->iProcessed;
   
   //   1.   Read login-id:
   pLast = pNext;
   pNext = ReadIntFromString( pNext,            //   [in]      Source string
                              ",",              //   [in,opt]  Value termination
                              NULL,             //   [in,opt]  Allowed padding
                              &pCI->iServerID,  //   [out]     Put it here
                              TRIM_ALL_CHARS);  //   [in]      Remove additional termination chars
   if( !pNext || (RT_INVALID_LOGINID_VALUE == pCI->iServerID))
   {
      roadmap_log( ROADMAP_ERROR, "RTNet::OnLoginResponse() - Failed to read Login-ID from response (%s)", pLast);
      return transaction_failed;   //   Quit the 'receive' loop
   }
   
   //   2.   Next value is the server cookie:
   if( !(*pNext))
   {
      roadmap_log( ROADMAP_ERROR, "RTNet::OnLoginResponse() - Did not find server-cookie (secretKet) in the response");
      return transaction_failed;   //   Quit the 'receive' loop
   }
   
   iBufferSize = RTNET_SERVERCOOKIE_MAXSIZE;
   pNext       = ExtractNetworkString(   
               pNext,               // [in]     Source string
               pCI->ServerCookie,   // [out,opt]Output buffer
               &iBufferSize,        // [in,out] Buffer size / Size of extracted string
               "\r\n",              // [in]     Array of chars to terminate the copy operation
               TRIM_ALL_CHARS);     // [in]     Remove additional termination chars
   if( !pNext)
   {
      roadmap_log( ROADMAP_ERROR, "RTNet::OnLoginResponse(case 2) - Did not find server-cookie (secretKet) in the response");
      return transaction_failed;   //   Quit the 'receive' loop
   }
   
   //   Done
   
   //   Update status variables:
   pCI->bLoggedIn    = TRUE;
   pRI->iProcessed   = (int)((size_t)pNext + strlen( pNext) - (size_t)pRI->Data);
   pCI->eParserState = PS_Completed;
   
   return transaction_succeeded;   
}

ETransactionResult OnLogOutResponse( LPResponseBuffer pRI, void* pContext)
{
   RTNET_ONRESPONSE_BEGIN( "OnLogOutResponse", "LogoutSuccessful")
   
   //   Update status variables:
   pCI->bLoggedIn    = FALSE;
   pCI->iServerID    = RT_INVALID_LOGINID_VALUE;
   pCI->eParserState = PS_Completed;
   memset( pCI->ServerCookie, 0, sizeof( pCI->ServerCookie));

   return transaction_succeeded;   
}

ETransactionResult OnGeneralResponse( LPResponseBuffer pRI, void* pContext)
{
   char              Tag[RTNET_RESPONSE_TAG_MAXSIZE+1];
   BOOL              bHandlerRes;
   ERTPacketResponse eTag  = RES__invalid;
   const char*       pNext = NULL;
   const char*       pLast = NULL;   //   For logging
   int               iBufferSize;

   RTNET_ONRESPONSE_BEGIN( "OnGeneralResponse", NULL)
   
   //   As long as we have data - keep on parsing:
   while( (pRI->iCustomDataProcessed + pRI->iProcessed) < pRI->iCustomDataSize)
   {
      //   Do we need to read more bytes?
      if( pRI->iDataSize <= pRI->iProcessed)
         return transaction_in_progress;   //   Continue reading...
      
      //   Move data pointer:
      pNext = (const char*)pRI->Data + pRI->iProcessed;
      
      //   Test against end-of-string (error in packet)
      if( !(*pNext))
      {
         roadmap_log(ROADMAP_ERROR, 
                     "RTNet::OnGeneralResponse() - CORRUPTED PACKET: Although remaining custom data size is %d, a NULL was found in the data...",
                     (pRI->iCustomDataSize - (pRI->iCustomDataProcessed + pRI->iProcessed)));
         return transaction_failed;   //   Quit the 'receive' loop
      }

      //   In order to parse a full statement we must have a full line:
      if( NULL == strchr( pNext, '\n'))
         return transaction_in_progress;   //   Continue reading...

      //   Read next tag:
      pLast       = pNext;
      iBufferSize = RTNET_RESPONSE_TAG_MAXSIZE;
      pNext       = ExtractNetworkString(   
                  pNext,           // [in]     Source string
                  Tag,             // [out,opt]Output buffer
                  &iBufferSize,    // [in,out] Buffer size / Size of extracted string
                  ",\r\n",         // [in]     Array of chars to terminate the copy operation
                  TRIM_ALL_CHARS); // [in]     Remove additional termination chars
      if( !pNext || !(*pNext))
      {
         roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse() - Failed to read server-response tag from packet location '%s'", pLast);
         return transaction_failed;   //   Quit the 'receive' loop
      }
      
      //   Find response tag:
      eTag = ERTPacketResponse_from_string( Tag);
      
      //   Activate the appropriate server-request handler function:
      bHandlerRes = FALSE;
      switch( eTag)
      {
         RTNET_SERVER_REQUEST_HANDLER(AddUser)
         RTNET_SERVER_REQUEST_HANDLER(AddAlert)
         RTNET_SERVER_REQUEST_HANDLER(RemoveAlert)
         RTNET_SERVER_REQUEST_HANDLER(SystemMessage)
         RTNET_SERVER_REQUEST_HANDLER(VersionUpgrade)
         RTNET_SERVER_REQUEST_HANDLER(AddAlertComment)
         RTNET_SERVER_REQUEST_HANDLER(AddRoadInfo)
         RTNET_SERVER_REQUEST_HANDLER(RmRoadInfo)
         
         
         default:
         {
            roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse() - Server request was unexpected at this time: '%s'", Tag);
            return transaction_failed;   //   Quit the 'receive' loop
         }
      }
      
      if( !bHandlerRes)
         //   Handler failed - Just log and continue processing packet:
         roadmap_log( ROADMAP_WARNING, "RTNet::OnGeneralResponse() - Failed to process server request '%s'. Continueing with rest of packet (if any)", Tag);

      //   Update bytes-processed:
      pNext = strchr( pLast, '\n');                      //   (Was already tested with success, thus must succeed...)
      pNext = EatChars( pNext, "\r\n", TRIM_ALL_CHARS);  //   Skip possible additional CRLN chars
      pRI->iProcessed  = RTNET_CALC_BYTES_PROCESSED( pNext);
   }
   
   return transaction_succeeded;   
}
//////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////
//   <UID:int>,[USER-NAME:string],<longitude:double>,<latitude:double>,<azimuth:int(0..359)>,<speed:double>,<last access time:long>
BOOL AddUser( LPResponseBuffer pRI, LPRTConnectionInfo pCI, const char* pNext)
{
   int            iBufferSize;
   RTUserLocation UL;

   //   Initialize structure:
   RTUserLocation_Init( &UL);
   
   //   1.   Read user ID:
   pNext = ReadIntFromString(   pNext,         //   [in]      Source string
                              ",",           //   [in,opt]   Value termination
                              NULL,          //   [in,opt]   Allowed padding
                              &UL.iID,       //   [out]      Put it here
                              DO_NOT_TRIM);  //   [in]      Remove additional termination CHARS
   
   if( !pNext || (',' != (*pNext)) || (RT_INVALID_LOGINID_VALUE == UL.iID))
   {
      roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse::AddUser() - Failed to read user ID");
      return FALSE;
   }
   
   //   Jump over the first ',' that 'ReadIntFromString()' (above) did encounter:
   pNext++;
   
   //   2.   User Name - is optional:
   if( ',' == (*pNext))
   {
      roadmap_log( ROADMAP_DEBUG, "RTNet::OnGeneralResponse::AddUser() - User name was not supplied. Probably a guest-login...");
      pNext++;   //   Jump over the comma (,)
   }
   else
   {
      //   Read user name:
      iBufferSize = RT_USERNM_MAXSIZE;
      pNext       = ExtractNetworkString(   
                  pNext,           // [in]     Source string
                  UL.sName,        // [out,opt]Output buffer
                  &iBufferSize,    // [in,out] Buffer size / Size of extracted string
                  ",",             // [in]     Array of chars to terminate the copy operation
                  TRIM_ALL_CHARS); // [in]     Remove additional termination chars

      if( !pNext || !(*pNext))
      {
         roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse::AddUser() - Failed to read user name");
         return FALSE;
      }
   }
   
   //   3.   Longitude
   pNext = ReadDoubleFromString(   
               pNext,            //   [in]      Source string
               ",",              //   [in,opt]   Value termination
               NULL,             //   [in,opt]   Allowed padding
               &UL.fLongitude,   //   [out]      Put it here
               TRIM_ALL_CHARS);  //   [in]      Remove additional termination CHARS
   
   if( !pNext || !(*pNext))
   {
      roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse::AddUser() - Failed to read longitude");
      return FALSE;
   }

   //   4.   Latitude
   pNext = ReadDoubleFromString(   
               pNext,            //   [in]      Source string
               ",",              //   [in,opt]   Value termination
               NULL,             //   [in,opt]   Allowed padding
               &UL.fLatitude,    //   [out]      Put it here
               TRIM_ALL_CHARS);  //   [in]      Remove additional termination CHARS
   
   if( !pNext || !(*pNext))
   {
      roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse::AddUser() - Failed to read altitude");
      return FALSE;
   }

   //   5.   Azimuth
   pNext = ReadIntFromString(   
               pNext,            //   [in]      Source string
               ",",              //   [in,opt]   Value termination
               NULL,             //   [in,opt]   Allowed padding
               &UL.iAzimuth,     //   [out]      Put it here
               TRIM_ALL_CHARS);  //   [in]      Remove additional termination CHARS
   
   if( !pNext || !(*pNext))
   {
      roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse::AddUser() - Failed to read azimuth");
      return FALSE;
   }
   

   //   6.   Speed
   pNext = ReadDoubleFromString(   
                        pNext,            //   [in]      Source string
                        ",",              //   [in,opt]   Value termination
                        NULL,             //   [in,opt]   Allowed padding
                        &UL.fSpeed,       //   [out]      Put it here
                        TRIM_ALL_CHARS);  //   [in]      Remove additional termination CHARS
   
   if( !pNext || !(*pNext))
   {
      roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse::AddUser() - Failed to read speed");
      return FALSE;
   }

   //   7.   Last access
   pNext = ReadInt64FromString(   
                  pNext,            //   [in]      Source string
                  ",\r\n",          //   [in,opt]   Value termination
                  NULL,             //   [in,opt]   Allowed padding
                  &UL.i64LastAccessTime,   
                                    //   [out]      Put it here
                  TRIM_ALL_CHARS);  //   [in]      Remove additional termination CHARS
   
   if( !pNext)
   {
      roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse::AddUser() - Failed to read last access");
      return FALSE;
   }
   
   //   Create unique ID for the GUI object-system:
   RTUserLocation_CreateGUIID( &UL);
   
   //   If users exists: Update, Otherwize: Add:
   if( !RTUsers_UpdateOrAdd( &(pCI->Users), &UL))
   {
      roadmap_log(   ROADMAP_ERROR, 
                  "RTNet::OnGeneralResponse::AddUser() - Failed to 'UpdateOrAdd' user (ID: %d); Maybe users-list is full? (Size: %d)",
                  UL.iID, RTUsers_Count( &(pCI->Users)));
      return FALSE;
   }
   
                 
   return TRUE;
}

//   <MsgType:int>,<MsgShowType:int>,[<MsgTitle:String[64]>],<MsgString:String[512]>
BOOL  SystemMessage( LPResponseBuffer pRI, LPRTConnectionInfo   pCI, const char* pNext)
{
   RTSystemMessage   Msg;
   int               iBufferSize;
   const char*       pLast;
   
   RTSystemMessage_Init( &Msg);
   
   //   1.  Message type:
   pNext = ReadIntFromString( pNext,            // [in]     Source string
                              ",",              // [in,opt] Value termination
                              NULL,             // [in,opt] Allowed padding
                              &Msg.iType,       // [out]    Put it here
                              TRIM_ALL_CHARS);  // [in]     Remove additional termination CHARS
   
   if( !pNext)
   {
      roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse::SystemMessage() - Failed to read message type");
      return FALSE;
   }
   
   //   2.  Show type:
   pNext = ReadIntFromString( pNext,            // [in]     Source string
                              ",",              // [in,opt] Value termination
                              NULL,             // [in,opt] Allowed padding
                              &Msg.iShow,       // [out]    Put it here
                              TRIM_ALL_CHARS);  // [in]     Remove additional termination CHARS
   
   if( !pNext)
   {
      roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse::SystemMessage() - Failed to read message SHOW type");
      return FALSE;
   }
   
   //   3.  Title:
   iBufferSize = RTNET_SYSTEMMESSAGE_TITLE_MAXSIZE;
   pLast       = pNext;
   pNext       = ExtractNetworkString(   
               pNext,         // [in]     Source string
               NULL,          // [out,opt]Output buffer
               &iBufferSize,  // [in,out] Buffer size / Size of extracted string
               ",",           // [in]     Array of chars to terminate the copy operation
               DO_NOT_TRIM);  // [in]     Remove additional termination chars

   if( !pNext)
   {
      roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse::SystemMessage() - Failed to read message title");
      return FALSE;
   }
   
   if( iBufferSize)
   {
      iBufferSize++; // Additional space for the terminating null...
      Msg.Title   = malloc(iBufferSize);
      pNext       = ExtractNetworkString(   
                  pLast,         // [in]     Source string
                  Msg.Title,     // [out,opt]Output buffer
                  &iBufferSize,  // [in,out] Buffer size / Size of extracted string
                  ",",           // [in]     Array of chars to terminate the copy operation
                  DO_NOT_TRIM);  // [in]     Remove additional termination chars

      if( !pNext)
      {
         assert(0);
         RTSystemMessage_Free( &Msg);
         roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse::SystemMessage() - Failed to read message title (case-II)");
         return FALSE;
      }
   }
   
   if( ',' == (*pNext))
      pNext++;
   
   //   4.  Text:
   pLast       = pNext;
   iBufferSize = RTNET_SYSTEMMESSAGE_TEXT_MAXSIZE;
   pNext       = ExtractNetworkString(   
               pNext,            // [in]     Source string
               NULL,             // [out,opt]Output buffer
               &iBufferSize,     // [in,out] Buffer size / Size of extracted string
               ",\r\n",          // [in]     Array of chars to terminate the copy operation
               TRIM_ALL_CHARS);  // [in]     Remove additional termination chars

   if( !pNext)
   {
      RTSystemMessage_Free( &Msg);
      roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse::SystemMessage() - Failed to read message title");
      return FALSE;
   }
   
   if( !iBufferSize)
   {
      RTSystemMessage_Free( &Msg);
      roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse::SystemMessage() - Failed to read message title - Messsage-text was not supplied");
      return FALSE;
   }
   
   iBufferSize++; // Additional space for the terminating null...
   Msg.Text    = malloc(iBufferSize);
   pNext       = ExtractNetworkString(   
               pLast,            // [in]     Source string
               Msg.Text,         // [out,opt]Output buffer
               &iBufferSize,     // [in,out] Buffer size / Size of extracted string
               ",\r\n",          // [in]     Array of chars to terminate the copy operation
               TRIM_ALL_CHARS);  // [in]     Remove additional termination chars
                 
   // Push item to queue
   RTSystemMessageQueue_Push( &Msg);
   // Detach from object:
   RTSystemMessage_Init( &Msg);
   
   return TRUE;
}

//   <Upgrade-Type:int>,<Latest-Version:String[32]>,<URL:String[256]>
BOOL  VersionUpgrade( LPResponseBuffer pRI, LPRTConnectionInfo   pCI, const char* pNext)
{
   extern 
   VersionUpgradeInfo   gs_VU;
   int                  iBufferSize;
   int                  i;

   
   gs_VU.eSeverity = VUS_NA;
   
   // 1. Severity:
   pNext = ReadIntFromString( pNext,            // [in]     Source string
                              ",",              // [in,opt] Value termination
                              NULL,             // [in,opt] Allowed padding
                              &i,               // [out]    Put it here
                              TRIM_ALL_CHARS);  // [in]     Remove additional termination CHARS
   if( !pNext)
   {
      roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse::VersionUpgrade() - Failed to read severity");
      return FALSE;
   }
   
   if( (i<1) || (3<i))
   {
      roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse::VersionUpgrade() - Invalid value for VU severity (%d)", i);
      return FALSE;
   }
   
   gs_VU.eSeverity = (EVersionUpgradeSeverity)i;
   
   // 2. New version value:
   iBufferSize = VERSION_STRING_MAXSIZE;
   pNext       = ExtractNetworkString(   
               pNext,           // [in]     Source string
               gs_VU.NewVersion,// [out,opt]Output buffer
               &iBufferSize,    // [in,out] Buffer size / Size of extracted string
               ",",             // [in]     Array of chars to terminate the copy operation
               TRIM_ALL_CHARS); // [in]     Remove additional termination chars

   if( !pNext || !(*pNext))
   {
      VersionUpgradeInfo_Init( &gs_VU);
      roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse::VersionUpgrade() - Failed to read version value");
      return FALSE;
   }
   
   // 3. URL:
   iBufferSize = GENERAL_URL_MAXSIZE;
   pNext       = ExtractNetworkString(   
               pNext,            // [in]     Source string
               gs_VU.URL,        // [out,opt]Output buffer
               &iBufferSize,     // [in,out] Buffer size / Size of extracted string
               ",\r\n",          // [in]     Array of chars to terminate the copy operation
               TRIM_ALL_CHARS);  // [in]     Remove additional termination chars

   if( !pNext)
   {
      VersionUpgradeInfo_Init( &gs_VU);
      roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse::VersionUpgrade() - Failed to read version URL");
      return FALSE;
   }
   
   roadmap_log( ROADMAP_INFO, "RTNet::OnGeneralResponse::VersionUpgrade() - !!! HAVE A NEW VERSION !!! (Severity: %d)", gs_VU.eSeverity);

   return TRUE;
}


//////////////////////////////////////////////////////////////////////////////////////////////////
//   <AID:int>,<latitude:double>,<longitude:double>,<node1:???><node2:???><direction:int(0..2)><azymuth:int(0..359)><reportTime:long><reportBy:string><speed:int><type:int><description:string>
BOOL AddAlert( LPResponseBuffer pRI, LPRTConnectionInfo pCI, const char* pNext)
{
   RTAlert  alert;
   double   fLatitue;
   double   fLongtitude;
   int      iBufferSize;
   char     reportedBy[5];

   //   Initialize structure:
   RTAlerts_Alert_Init(&alert); 
   
   //   1.   Read Alert ID:
   pNext = ReadIntFromString(   
                  pNext,         //   [in]      Source string
                  ",",           //   [in,opt]   Value termination
                  NULL,          //   [in,opt]   Allowed padding
                  &alert.iID,    //   [out]      Put it here
                  DO_NOT_TRIM);  //   [in]      Remove additional termination CHARS
   
   if( !pNext || (',' != (*pNext)) || (RT_INVALID_LOGINID_VALUE == alert.iID))
   {
      roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse::AddAlert() - Failed to read  ID");
      return FALSE;
   }
   
   //   Jump over the first ',' that 'ReadIntFromString()' (above) did encounter:
   pNext++;

   //   2.   Type
   pNext = ReadIntFromString(   
                  pNext,            //   [in]      Source string
                  ",",              //   [in,opt]   Value termination
                  NULL,             //   [in,opt]   Allowed padding
                  &alert.iType,     //   [out]      Put it here
                  1);               //   [in]      Remove additional termination CHARS
   
   if( !pNext || !(*pNext))
   {
      roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse::AddAlert() - Failed to read type");
      return FALSE;
   }
   
   //   3.   Longitude
   pNext = ReadDoubleFromString(   
                  pNext,            //   [in]      Source string
                  ",",              //   [in,opt]   Value termination
                  NULL,             //   [in,opt]   Allowed padding
                  &fLongtitude,      //   [out]      Put it here
                  1);               //   [in]      Remove additional termination CHARS
   
   if( !pNext || !(*pNext))
   {
      roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse::AddAlert() - Failed to read longitude");
      return FALSE;
   }
   alert.iLongitude =  (int)(fLongtitude * 1000000);
   
   //   4.   Latitude
   pNext = ReadDoubleFromString(   
               pNext,            //   [in]      Source string
               ",",              //   [in,opt]   Value termination
               NULL,             //   [in,opt]   Allowed padding
               &fLatitue,        //   [out]      Put it here
               1);               //   [in]      Remove additional termination CHARS
   
   if( !pNext || !(*pNext))
   {
      roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse::AddAlert() - Failed to read altitude");
      return FALSE;
   }
   alert.iLatitude = (int)(fLatitue  * 1000000);
   //   5.   Node1
   pNext = ReadIntFromString(   
               pNext,            //   [in]      Source string
               ",",              //   [in,opt]   Value termination
               "-",              //   [in,opt]   Allowed padding
               &alert.iNode1,    //   [out]      Put it here
               1);               //   [in]      Remove additional termination CHARS
   
   if( !pNext || !(*pNext))
   {
      roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse::AddAlert() - Failed to read node1");
      // return FALSE;
   }
   

   //   6.   Node2
   pNext = ReadIntFromString(   
                  pNext,            //   [in]      Source string
                  ",",              //   [in,opt]   Value termination
                  "-",              //   [in,opt]   Allowed padding
                  &alert.iNode2,    //   [out]      Put it here
                  1);               //   [in]      Remove additional termination CHARS
   
   if( !pNext || !(*pNext))
   {
      // roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse::AddAlert() - Failed to read node2");
      return FALSE;
   }
   
   //   7.   Direction
   pNext = ReadIntFromString(   
            pNext,            //   [in]      Source string
            ",",              //   [in,opt]   Value termination
            NULL,             //   [in,opt]   Allowed padding
            &alert.iDirection,//   [out]      Put it here
            1);               //   [in]      Remove additional termination CHARS
   
   if( !pNext || !(*pNext))
   {
      roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse::AddAlert() - Failed to read direction");
      return FALSE;
   }
   
   //   8.   Azymuth
   pNext = ReadIntFromString(   
            pNext,            //   [in]      Source string
            ",",              //   [in,opt]   Value termination
            NULL,             //   [in,opt]   Allowed padding
            &alert.iAzymuth,  //   [out]      Put it here
            1);               //   [in]      Remove additional termination CHARS
   
   if( !pNext || !(*pNext))
   {
      roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse::AddAlert() - Failed to read azymuth");
      return FALSE;
   }
   
   
   //   9.   Description  - is optional:
   if(',' == (*pNext))
   {
      roadmap_log( ROADMAP_DEBUG, "RTNet::OnGeneralResponse::AddAlert() - Description by was not supplied. ");
      pNext++;   //   Jump over the comma (,)
   }
   else
   {
      //   Read user name:
      iBufferSize = RT_ALERT_DESCRIPTION_MAXSIZE;
      pNext       = ExtractNetworkString(   
                  pNext,               // [in]     Source string
                  alert.sDescription,  // [out,opt]Output buffer
                  &iBufferSize,        // [in,out] Buffer size / Size of extracted string
                  ",",                 // [in]     Array of chars to terminate the copy operation
                  1);                  // [in]     Remove additional termination chars

      if( !pNext || !(*pNext))
      {
         roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse::AddAlert() - Failed to read description");
         return FALSE;
      }
   }
   
   //   10.   Report Time
   pNext = ReadInt64FromString(   
               pNext,               //   [in]      Source string
               ",",                 //   [in,opt]  Value termination
               NULL,                //   [in,opt]  Allowed padding
               &alert.i64ReportTime,//   [out]     Put it here
               1);                  //   [in]      Remove additional termination CHARS
   
   if( !pNext)
   {
      roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse::AddAlert() - Failed to read report time");
      return FALSE;
   }
   
   //   Read user name:
   iBufferSize = RT_ALERT_USERNM_MAXSIZE;
   pNext = ExtractNetworkString(   
                  pNext,            //   [in]   Source string
                  alert.sReportedBy, // [out,opt]Output buffer
                  &iBufferSize,   //   [in]   Output buffer size
                  ",",               // [in]     Array of chars to terminate the copy operation
                  TRIM_ALL_CHARS);   //   [in]   Remove additional termination chars

    if( !pNext)
    {
         roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse::AddAlert() - Failed to read PostedByMe flag");
         alert.bAlertByMe = FALSE;
    }

   //   Read If the alert was reported by me
   iBufferSize = sizeof(reportedBy);
   pNext       = ExtractNetworkString(   
                  pNext,             // [in]     Source string
                  reportedBy,//   [out]   Output buffer
                  &iBufferSize,      // [in,out] Buffer size / Size of extracted string
                  ",\r\n",          //   [in]   Array of chars to terminate the copy operation
                  TRIM_ALL_CHARS);   // [in]     Remove additional termination chars

    if( !pNext)
    {
         roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse::AddAlert() - Failed to read user name");
         return FALSE;
    }

 	
  	if (reportedBy[0] == 'T')
  		 alert.bAlertByMe = TRUE;
  	else
  		alert.bAlertByMe = FALSE;
  	 

   alert.iSpeed = 0 ;

   //   Add the Alert
   if( !RTAlerts_Add(&alert))
   {
      roadmap_log(ROADMAP_ERROR, 
                  "RTNet::OnGeneralResponse::AddAlert() - Failed to 'Add' alert (ID: %d);  (Alert List Size: %d)",
                  alert.iID, RTAlerts_Count());
      return FALSE;
   }


   return TRUE;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//   <AID:int>
BOOL AddAlertComment( LPResponseBuffer pRI, LPRTConnectionInfo pCI, const char* pNext)
{
	RTAlertComment comment;
	int iBufferSize;
	char reportedBy[5];
	
	   //   Initialize structure:
	   RTAlerts_Comment_Init(&comment); 
	   
	   //   1.   Read Alert ID:
	   pNext = ReadIntFromString(   
	                  pNext,         //   [in]      Source string
	                  ",",           //   [in,opt]   Value termination
	                  NULL,          //   [in,opt]   Allowed padding
	                  &comment.iAlertId,  //   [out]      Put it here
	                  DO_NOT_TRIM);  //   [in]      Remove additional termination CHARS
	   
	   if( !pNext || (',' != (*pNext)) || (RT_INVALID_LOGINID_VALUE == comment.iAlertId))
	   {
	      roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse::AddAlertComment() - Failed to read alert ID");
	      return FALSE;
	   }
	   
	   //   Jump over the first ',' that 'ReadIntFromString()' (above) did encounter:
	   pNext++;

	   //   2.   Comment Id
	   pNext = ReadIntFromString(   
	                  pNext,            //   [in]      Source string
	                  ",",              //   [in,opt]   Value termination
	                  NULL,             //   [in,opt]   Allowed padding
	                  &comment.iID,  //   [out]      Put it here
	                  1);               //   [in]      Remove additional termination CHARS
	   
	   if( !pNext || !(*pNext))
	   {
	      roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse::AddAlertComment() - Failed to read Comment Id");
	      return FALSE;
	   }
   
	   //  3. Description
		iBufferSize = RT_ALERT_DESCRIPTION_MAXSIZE;
	   pNext = ExtractNetworkString(   
	                  pNext,            //   [in]   Source string
	                  comment.sDescription,//   [out]   Output buffer
	                  &iBufferSize,   
	                                    //   [in]   Output buffer size
	                  ",",          //   [in]   Array of chars to terminate the copy operation
	                  TRIM_ALL_CHARS);   //   [in]   Remove additional termination chars
	   if( !pNext)
	   {
	      roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse::AddAlertComment() - Failed to read description");
	      return FALSE;
	   }

	   //   4.   Post Time
	   pNext = ReadInt64FromString(   
		             pNext,               //   [in]      Source string
		             ",",                 //   [in,opt]   Value termination
		             NULL,                //   [in,opt]   Allowed padding
		             &comment.i64ReportTime,//   [out]      Put it here
		             1);                  //   [in]      Remove additional termination CHARS
		   
		if( !pNext)
		{
		   roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse::AddAlertComment() - Failed to read report time");
		   return FALSE;
		}

		//  5. Read who posted the comment
		iBufferSize = RT_ALERT_USERNM_MAXSIZE;
		pNext = ExtractNetworkString(   
    		  		  pNext,               //   [in]   Source string
	                  comment.sPostedBy,  //   [out]   Output buffer
	                  &iBufferSize,   
	                                       //   [in]   Output buffer size
	                  ",",                 //   [in]   Array of chars to terminate the copy operation
	                  1);                  //   [in]   Remove additional termination chars

		if( !pNext)
		{
        	roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse::AddAlertComment() - Failed to read user name");
        	return FALSE;
		}
	   
	   
	      //   Read If the comment was posted by me
		 iBufferSize = sizeof(reportedBy);
  		 pNext = ExtractNetworkString(   
                  pNext,            //   [in]   Source string
                  reportedBy,    //   [out]   Output buffer
                  &iBufferSize,  //   [in]   Output buffer size
                  ",\r\n",          //   [in]   Array of chars to terminate the copy operation
                  TRIM_ALL_CHARS);   //   [in]   Remove additional termination chars

         if( !pNext)
        {
             roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse::AddAlertComment() - Failed to read PostedByMe flag");
             comment.bCommentByMe = FALSE;
         }
  	
  	     if (reportedBy[0] == 'T')
  		    comment.bCommentByMe= TRUE;
  	    else
  		    comment.bCommentByMe = FALSE;
		
		//   Add the Comment
		if( !RTAlerts_Comment_Add(&comment))
		{
			roadmap_log(ROADMAP_ERROR, 
						"RTNet::OnGeneralResponse::AddAlertComment() - Failed to add comment (ID: %d)",
						comment.iID);
			return FALSE;
		}
     
		return TRUE;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//   <AID:int>
BOOL RemoveAlert( LPResponseBuffer pRI, LPRTConnectionInfo pCI, const char* pNext)
{
   int    iID;

   //   1.   Read Alert ID:
   pNext = ReadIntFromString( pNext,         // [in]     Source string
                              ",\r\n",       // [in,opt] Value termination
                              NULL,          // [in,opt] Allowed padding
                              &iID,          // [out]    Put it here
                              DO_NOT_TRIM);  // [in]     Remove additional termination CHARS

   if( !pNext || (-1 == iID))
   {
      roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse::RemoveAlert() - Failed to read  ID");
      return FALSE;
   }


   //   Remove the alert
   if( !RTAlerts_Remove(iID))
   {
      roadmap_log(ROADMAP_ERROR, 
                  "RTNet::OnGeneralResponse::AddAlert() - Failed to 'Remove' alert (ID: %d);",iID);
      return FALSE;
   }

   return TRUE;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//   <ID><Speed><type><street><city><start><end><num_segs> <nodeid><lat><long>
BOOL AddRoadInfo( LPResponseBuffer pRI, LPRTConnectionInfo pCI, const char* pNext)
{
   RTTrafficInfo trafficInfo;
	int iBufferSize;
	int i;
	double   fLatitue;
   double   fLongtitude;
   int 			iNumElements;
	
   //   Initialize structure:
   RTTrafficInfo_InitRecord(&trafficInfo); 
   
   //   1.   RoadInfo ID:
   pNext = ReadIntFromString(   
                  pNext,         //   [in]      Source string
                  ",",           //   [in,opt]   Value termination
                  NULL,          //   [in,opt]   Allowed padding
                  &trafficInfo.iID,    //   [out]      Put it here
                  1);  //   [in]      Remove additional termination CHARS
   
   if( !pNext || ! (*pNext) || (-1 == trafficInfo.iID))
   {
      roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse::AddRoadInfo() - Failed to read  ID");
      return FALSE;
   }
   
   //   2.   Speed
   
   pNext = ReadDoubleFromString(   
                  pNext,            //   [in]      Source string
                  ",",              //   [in,opt]   Value termination
                  NULL,             //   [in,opt]   Allowed padding
                  &trafficInfo.fSpeed,     //   [out]      Put it here
                  1);               //   [in]      Remove additional termination CHARS
   
   if( !pNext || !(*pNext))
   {
      roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse::AddRoadInfo() - Failed to read speed");
      return FALSE;
   }
   
   //   3.   Type
   pNext = ReadIntFromString(   
                  pNext,            //   [in]      Source string
                  ",",              //   [in,opt]   Value termination
                  NULL,             //   [in,opt]   Allowed padding
                  &trafficInfo.iType,     //   [out]      Put it here
                  1);               //   [in]      Remove additional termination CHARS
   
   if( !pNext || !(*pNext))
   {
      roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse::AddRoadInfo() - Failed to read type");
      return FALSE;
   }

   //   4.   Street  - is optional
   if(',' == (*pNext))
   {
      roadmap_log( ROADMAP_DEBUG, "RTNet::OnGeneralResponse::AddRoadInfo() - Street is empty. ");
      pNext++;   //   Jump over the comma (,)
   }
   else
   {
      iBufferSize = RT_TRAFFIC_INFO_ADDRESS_MAXSIZE;
      pNext       = ExtractNetworkString(   
                  pNext,               // [in]     Source string
                  trafficInfo.sStreet,  // [out,opt]Output buffer
                  &iBufferSize,        // [in,out] Buffer size / Size of extracted string
                  ",",                 // [in]     Array of chars to terminate the copy operation
                  1);                  // [in]     Remove additional termination chars

      if( !pNext || !(*pNext))
      {
         roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse::AddRoadInfo() - Failed to read Street");
         return FALSE;
      }
   }

   //   5.   City  - is optional
   if(',' == (*pNext))
   {
      roadmap_log( ROADMAP_DEBUG, "RTNet::OnGeneralResponse::AddRoadInfo() - City is empty. ");
      pNext++;   //   Jump over the comma (,)
   }
   else
   {
      iBufferSize = RT_TRAFFIC_INFO_ADDRESS_MAXSIZE;
      pNext       = ExtractNetworkString(   
                  pNext,               // [in]     Source string
                  trafficInfo.sCity,  // [out,opt]Output buffer
                  &iBufferSize,        // [in,out] Buffer size / Size of extracted string
                  ",",                 // [in]     Array of chars to terminate the copy operation
                  1);                  // [in]     Remove additional termination chars

      if( !pNext || !(*pNext))
      {
         roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse::AddRoadInfo() - Failed to read City");
         return FALSE;
      }
   }

   //   6.   Start  - is optional
   if(',' == (*pNext))
   {
      roadmap_log( ROADMAP_DEBUG, "RTNet::OnGeneralResponse::AddRoadInfo() - Start is emprty. ");
      pNext++;   //   Jump over the comma (,)
   }
   else
   {
      iBufferSize = RT_TRAFFIC_INFO_ADDRESS_MAXSIZE;
      pNext       = ExtractNetworkString(   
                  pNext,               // [in]     Source string
                  trafficInfo.sStart,  // [out,opt]Output buffer
                  &iBufferSize,        // [in,out] Buffer size / Size of extracted string
                  ",",                 // [in]     Array of chars to terminate the copy operation
                  1);                  // [in]     Remove additional termination chars

      if( !pNext || !(*pNext))
      {
         roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse::AddRoadInfo() - Failed to read Start");
         return FALSE;
      }
   }

   //   7.   End  - is optional
   if(',' == (*pNext))
   {
      roadmap_log( ROADMAP_DEBUG, "RTNet::OnGeneralResponse::AddRoadInfo() - End is empty. ");
      pNext++;   //   Jump over the comma (,)
   }
   else
   {
      iBufferSize = RT_TRAFFIC_INFO_ADDRESS_MAXSIZE;
      pNext       = ExtractNetworkString(   
                  pNext,               // [in]     Source string
                  trafficInfo.sEnd,  // [out,opt]Output buffer
                  &iBufferSize,        // [in,out] Buffer size / Size of extracted string
                  ",",                 // [in]     Array of chars to terminate the copy operation
                  1);                  // [in]     Remove additional termination chars

      if( !pNext || !(*pNext))
      {
         roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse::AddRoadInfo() - Failed to read End");
         return FALSE;
      }
   }


   //   8.   NumSegments
   pNext = ReadIntFromString(   
                  pNext,            //   [in]      Source string
                  ",",              //   [in,opt]   Value termination
                  NULL,             //   [in,opt]   Allowed padding
                  &iNumElements,     //   [out]      Put it here
                  1);               //   [in]      Remove additional termination CHARS
   
   if( !pNext || !(*pNext))
   {
      roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse::AddRoadInfo() - Failed to read number of segments");
      return FALSE;
   }
   
   trafficInfo.iNumNodes = iNumElements/3;
   
	for (i=0; i<trafficInfo.iNumNodes;i++){

			   	//   NodeId
   				pNext = ReadIntFromString( 
               	pNext,            //   [in]      Source string
               	",",              //   [in,opt]   Value termination
               	NULL,             //   [in,opt]   Allowed padding
               	&trafficInfo.sNodes[i].iNodeId,        //   [out]      Put it here
               	1);               //   [in]      Remove additional termination CHARS
   
   				if( !pNext || !(*pNext))
   				{
      				roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse::AddRoadInfo() - Failed to read NodeId for segment %d, ID=%d", i,trafficInfo.iID );
      				return FALSE;
   				}

			   	//   Latitude of the segment
   				pNext = ReadDoubleFromString(   
               	pNext,            //   [in]      Source string
               	",",              //   [in,opt]   Value termination
               	NULL,             //   [in,opt]   Allowed padding
               	&fLatitue,        //   [out]      Put it here
               	1);               //   [in]      Remove additional termination CHARS
   
   				if( !pNext || !(*pNext))
   				{
      				roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse::AddRoadInfo() - Failed to read altitude for segment %d, ID=%d", i,trafficInfo.iID );
      				return FALSE;
   				}
   				trafficInfo.sNodes[i].Position.latitude = (int)(fLatitue  * 1000000);
		
			   	//   Longtitue of the segment
   				pNext = ReadDoubleFromString(   
               	pNext,            //   [in]      Source string
               	",\r\n",              //   [in,opt]   Value termination
               	NULL,             //   [in,opt]   Allowed padding
               	&fLongtitude,        //   [out]      Put it here
               	TRIM_ALL_CHARS);               //   [in]      Remove additional termination CHARS
   
   				if( !pNext)
   				{
      				roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse::AddRoadInfo() - Failed to read longtitude for segment %d, ID=%d", i,trafficInfo.iID );
      				return FALSE;
   				}
   				trafficInfo.sNodes[i].Position.longitude = (int)(fLongtitude  * 1000000);
	}

   //   Add the RoadInfo
   
   if( !RTTrafficInfo_Add(&trafficInfo))
   {
      roadmap_log(ROADMAP_ERROR, 
                  "RTNet::OnGeneralResponse::AddRoadInfo() - Failed to 'Add' road_info (ID: %d);  (List Size: %d)",
                  trafficInfo.iID, RTTrafficInfo_Count());
      return FALSE;
   }


   return TRUE;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//   <AID:int>
BOOL RmRoadInfo( LPResponseBuffer pRI, LPRTConnectionInfo pCI, const char* pNext)
{
   int    iID;

   //   1.   Read RoadInfo ID:
   pNext = ReadIntFromString( pNext,         // [in]     Source string
                              ",\r\n",       // [in,opt] Value termination
                              NULL,          // [in,opt] Allowed padding
                              &iID,          // [out]    Put it here
                              DO_NOT_TRIM);  // [in]     Remove additional termination CHARS

   if( !pNext || (-1 == iID))
   {
      roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse::RemoveRoadInfo() - Failed to read  ID");
      return FALSE;
   }


   //   Remove the RoadInfo
   if( !RTTrafficInfo_Remove(iID) )
   {
      roadmap_log(ROADMAP_ERROR, 
                  "RTNet::OnGeneralResponse::RemoveRoadInfo() - Failed to 'Remove' RoadInfo (ID: %d);",iID);
      return FALSE;
   }

   return TRUE;
}
//////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////
