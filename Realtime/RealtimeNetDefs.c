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

#include <stdio.h>
#include <string.h>
#include "RealtimeNetDefs.h"
#include "RealtimeTrafficInfo.h"

//////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////
// Init: memset + basic settings
void RTConnectionInfo_Init(LPRTConnectionInfo   this,
                           PFN_ONUSER           pfnOnAddUser,
                           PFN_ONUSER           pfnOnMoveUser,
                           PFN_ONUSER           pfnOnRemoveUser)
{
   memset( this, 0, sizeof(RTConnectionInfo));
   this->bLoggedIn   = FALSE;
   this->iServerID   = RT_INVALID_LOGINID_VALUE;
   this->eParserState= PS_Initialized;
   this->LastError   = ERR_Success;
   RTUsers_Init( &(this->Users), pfnOnAddUser, pfnOnMoveUser, pfnOnRemoveUser);
}

// Reset all data:
void RTConnectionInfo_FullReset( LPRTConnectionInfo this)
{
      RTConnectionInfo_ResetLogin( this);
/*11*/this->LastError            = ERR_Success;
/*12*/this->AdditionalErrorInfo  = 0;
}

// Reset data related to a login session:
void RTConnectionInfo_ResetLogin( LPRTConnectionInfo this)
{
/* 1*/memset( this->UserNm,      0, sizeof(this->UserNm));
/* 2*/memset( this->UserPW,      0, sizeof(this->UserPW));
/* 3*/memset( this->UserNk,      0, sizeof(this->UserNk));
/* 4*/memset( this->ServerCookie,0, sizeof(this->ServerCookie));
/* 5*/this->bLoggedIn = FALSE;
/* 6*/this->iServerID = RT_INVALID_LOGINID_VALUE;
/* 7*/memset( &(this->LastMapPosSent), 0, sizeof(RoadMapArea));
/* 8*/RTUsers_Reset( &(this->Users));
/* 9*/RTTrafficInfo_Reset();

      RTConnectionInfo_ResetTransaction( this);
}

// Reset at the end of a transaction:
void RTConnectionInfo_ResetTransaction( LPRTConnectionInfo this)
{
/* 9*/this->eTransactionStatus= TS_Idle;
      RTConnectionInfo_ResetParser( this);
}

// Reset between packet parsings:

void RTConnectionInfo_ResetParser( LPRTConnectionInfo this)
{
/*10*/this->eParserState = PS_Initialized;
}
//////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////
const char* ERTPacketRequest_to_string( ERTPacketRequest e)
{
   switch( e)
   {
      case REQ_Login:            return "Login";
      case REQ_GuestLogin:       return "GuestLogin";
      case REQ_LogOut:           return "LogOut";
      case REQ_At:               return "At";
      case REQ_MapDisplayed:     return "MapDisplayed";
      case REQ_SeeMe:            return "SeeMe";
      case REQ_StartFollowUsers: return "StartFollowUsers";

      default: return "<unknown>";
   }
}

ERTPacketRequest ERTPacketRequest_from_string( const char* szE)
{
   if( !szE || !(*szE))
      return REQ__invalid;

        if( 0==strcmp( szE, "Login"))           return REQ_Login;
   else if( 0==strcmp( szE, "GuestLogin"))      return REQ_GuestLogin;
   else if( 0==strcmp( szE, "LogOut"))          return REQ_LogOut;
   else if( 0==strcmp( szE, "At"))              return REQ_At;
   else if( 0==strcmp( szE, "MapDisplayed"))    return REQ_MapDisplayed;
   else if( 0==strcmp( szE, "SeeMe"))           return REQ_SeeMe;
   else if( 0==strcmp( szE, "StartFollowUsers"))return REQ_StartFollowUsers;

   return REQ__invalid;
}

const char* ERTPacketResponse_to_string( ERTPacketResponse e)
{
   switch( e)
   {
      case RES_LoginSuccessful:     return "LoginSuccessful";
      case RES_LoginError:          return "LoginError";
      case RES_LogoutSuccessful:    return "LogoutSuccessful";
      case RES_AddUser:             return "AddUser";
      case RES_UpdateUserLocation:  return "UpdateUserLocation";
      case RES_RmUser:              return "RmUser";
      case RES_ReturnAfter:         return "ReturnAfter";
      case RES_Fatal:               return "Fatal";
      case RES_Warning:             return "Warning";
      case RES_AddAlert:            return "AddAlert";
      case RES_AddAlertComment:     return "AddAlertComment";
      case RES_RemoveAlert:         return "RmAlert";
      case RES_SystemMessage:       return "SystemMessage";
      case RES_VersionUpgrade:      return "UpgradeClient";
        case RES_AddRoadInfo:         return "AddRoadInfo";
        case RES_RmRoadInfo:         return "RmRoadInfo";
      default: return "<unknown>";
   }

}

ERTPacketResponse ERTPacketResponse_from_string( const char* szE)
{
   if( !szE || !(*szE))
      return RES__invalid;

        if( 0==strcmp( szE, "LoginSuccessful"))    return RES_LoginSuccessful;
   else if( 0==strcmp( szE, "LoginError"))         return RES_LoginError;
   else if( 0==strcmp( szE, "LogoutSuccessful"))   return RES_LogoutSuccessful;
   else if( 0==strcmp( szE, "AddUser"))            return RES_AddUser;
   else if( 0==strcmp( szE, "UpdateUserLocation")) return RES_UpdateUserLocation;
   else if( 0==strcmp( szE, "RmUser"))             return RES_RmUser;
   else if( 0==strcmp( szE, "ReturnAfter"))        return RES_ReturnAfter;
   else if( 0==strcmp( szE, "Fatal"))              return RES_Fatal;
   else if( 0==strcmp( szE, "Warning"))            return RES_Warning;
   else if (0==strcmp( szE, "AddAlert"))           return RES_AddAlert;
   else if (0==strcmp( szE, "RmAlert"))            return RES_RemoveAlert;
   else if (0==strcmp( szE, "SystemMessage"))      return RES_SystemMessage;
   else if (0==strcmp( szE, "UpgradeClient"))      return RES_VersionUpgrade;
   else if (0==strcmp( szE, "AddAlertComment"))    return RES_AddAlertComment;
   else if (0==strcmp( szE, "AddRoadInfo"))        return RES_AddRoadInfo;
   else if (0==strcmp( szE, "RmRoadInfo"))         return RES_RmRoadInfo;
   return RES__invalid;
}
//////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////
void HttpAsyncParserInfo_Init( LPHttpAsyncParserInfo this)
{
   ResponseBuffer_Init  ( &(this->RB));
   this->eParserState           = PS_Initialized;
   this->bProcessingHttpHeader  = TRUE;
}

void HttpAsyncTransactionInfo_Init( LPHttpAsyncTransactionInfo this)
{
   memset( this, 0, sizeof(HttpAsyncTransactionInfo));
   this->Socket = ROADMAP_INVALID_SOCKET;
   HttpAsyncParserInfo_Init( &(this->API));
   RTPacket_Init( &(this->Packet));
}

void HttpAsyncTransactionInfo_Reset( LPHttpAsyncTransactionInfo this)
{
   RoadMapSocket Socket = this->Socket;

   RTPacket_Free( &(this->Packet));
   memset( this, 0, sizeof(HttpAsyncTransactionInfo));
   this->Socket         = Socket;
   this->starting_time  = 0;
   HttpAsyncParserInfo_Init( &(this->API));
}
//////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////
const char* GetErrorString( ERTErrorCode e)
{
   switch(e)
   {
      case ERR_Success:             return "Success";
      case ERR_NoDataToSend:        return "Client has no data to send";
      case ERR_GeneralError:        return "General Error";
      case ERR_NetworkError:        return "Network Error";
      case ERR_Aborted:             return "Session Aborted";
      case ERR_SessionTimedout:     return "Session Timed-out";
      case ERR_ParsingError:        return "Parsing Error";
      case ERR_LoginFailed:         return "Login Failed";
      case ERR_WrongNameOrPassword: return "Login Failed: Wrong name/pw";
      case ERR_WrongNetworkSettings:return "Wrong Network Settings";
      case ERR_UnknownLoginID:      return "Server does not recognize session";
      default: return "<unknown>";
   }
}
//////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////
