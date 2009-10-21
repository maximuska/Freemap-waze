
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

#include "../roadmap.h"
#include "../roadmap_config.h"
#include "../roadmap_start.h"
#include "../websvc_trans/websvc_trans.h"
#include "../roadmap_navigate.h"
#include "../roadmap_utf8.h"
#include "../roadmap_trip.h"
#include "local_search.h"
#include "generic_search.h"
#include "../Realtime/Realtime.h"


extern void convert_int_coordinate_to_float_string(char* buffer, int value);

extern const char* VerifyStatus( /* IN  */   const char*       pNext,
                                 /* IN  */   void*             pContext,
                                 /* OUT */   BOOL*             more_data_needed,
                                 /* OUT */   roadmap_result*   rc);
const char* on_local_option(   /* IN  */   const char*       data,
                                 /* IN  */   void*             context,
                                 /* OUT */   BOOL*             more_data_needed,
                                 /* OUT */   roadmap_result*   rc);

static wst_handle                s_websvc             = INVALID_WEBSVC_HANDLE;
static BOOL                      s_initialized_once   = FALSE;
static RoadMapConfigDescriptor   s_web_service_name   =
            ROADMAP_CONFIG_ITEM(
               LSR_WEBSVC_CFG_TAB,
               LSR_WEBSVC_ADDRESS);

static wst_parser data_parser[] =
{
   { "RC",                 VerifyStatus},
   { "AddressCandidate",   on_local_option}
}; 


static const char* get_webservice_address()
{ return roadmap_config_get( &s_web_service_name);}



BOOL local_candidate_build_address_string( address_candidate* this)
{
   BOOL have_city    = (this->city[0] != 0);
   BOOL has_name     = (this->name[0] != 0);
   BOOL has_phone    = (this->phone[0] != 0);
   BOOL have_street  = (this->street[0] != 0);
   BOOL have_house   = (-1 != this->house);
   BOOL has_state    = (this->state[0] != 0);
   this->address[0] = '\0';

   if (has_name)
      snprintf( this->address, ADSR_ADDRESS_MAX_SIZE, "%s\n", this->name);

   if (has_state)
   {
      if( have_city)
      {
         if( have_street)
         {
            if( have_house)
               snprintf( this->address + strlen(this->address), ADSR_ADDRESS_MAX_SIZE-strlen(this->address), "%d %s, %s, %s", this->house, this->street, this->city, this->state);
            else
               snprintf( this->address + strlen(this->address), ADSR_ADDRESS_MAX_SIZE-strlen(this->address), "%s, %s, %s", this->street, this->city, this->state);
         }
         else
            snprintf( this->address + strlen(this->address), ADSR_ADDRESS_MAX_SIZE-strlen(this->address), "%s, %s", this->city, this->state);
      }
      else
      {
         if( have_street)
         {
            if( have_house)
               snprintf( this->address + strlen(this->address), ADSR_ADDRESS_MAX_SIZE-strlen(this->address), "%d %s, ,%s ", this->house, this->street, this->state);
            else
               snprintf( this->address + strlen(this->address), ADSR_ADDRESS_MAX_SIZE-strlen(this->address), "%s, ,%s ", this->street, this->state);
         }
         else
         {
            snprintf( this->address + strlen(this->address), ADSR_ADDRESS_MAX_SIZE-strlen(this->address), " ");
         }
      }
   }
   else{
      if( have_city)
      {
         if( have_street)
         {
            if( have_house)
               snprintf( this->address+ strlen(this->address), ADSR_ADDRESS_MAX_SIZE-strlen(this->address), "%d %s, %s", this->house, this->street, this->city);
            else
               snprintf( this->address+ strlen(this->address), ADSR_ADDRESS_MAX_SIZE-strlen(this->address), "%s, %s", this->street, this->city);
         }
         else
            snprintf( this->address+ strlen(this->address), ADSR_ADDRESS_MAX_SIZE-strlen(this->address), "%s", this->city);
      }
      else
      {
         if( have_street)
         {
            if( have_house)
               snprintf( this->address+ strlen(this->address), ADSR_ADDRESS_MAX_SIZE-strlen(this->address), "%d %s, ", this->house, this->street);
            else
               snprintf( this->address+ strlen(this->address), ADSR_ADDRESS_MAX_SIZE-strlen(this->address), "%s, ", this->street);
         }
         else
         {
            snprintf( this->address + strlen(this->address), ADSR_ADDRESS_MAX_SIZE-strlen(this->address), " ");
         }
      }
   }

   if (has_phone)
       snprintf( this->address + strlen(this->address), ADSR_ADDRESS_MAX_SIZE-strlen(this->address), "\n%s", this->phone);

   return TRUE;
}

//   Module initialization/termination - Called once, when the process starts/terminates
BOOL local_search_init()
{
   const char* address;

   if( INVALID_WEBSVC_HANDLE != s_websvc)
   {
      assert(0);  // Called twice?
      return TRUE;
   }

   if( !s_initialized_once)
   {
      //   Web-service address
      roadmap_config_declare( LSR_WEBSVC_CFG_FILE,
                              &s_web_service_name,
                              LSR_WEBSVC_DEFAULT_ADDRESS,
                              NULL);
      s_initialized_once = TRUE;
   }

   address  = get_webservice_address();
   s_websvc = wst_init( get_webservice_address(), "application/x-www-form-urlencoded; charset=utf-8");

   if( INVALID_WEBSVC_HANDLE != s_websvc)
   {
      roadmap_log(ROADMAP_DEBUG,
                  "local_search_init() - Web-Service Address: '%s'",
                  address);
      return TRUE;
   }

   roadmap_log(ROADMAP_ERROR, "local_search_init() - 'wst_init()' failed");
   return FALSE;
}

void local_search_term()
{
   if( INVALID_WEBSVC_HANDLE == s_websvc)
      return;

   roadmap_log( ROADMAP_DEBUG, "local_search_term() - TERM");

   wst_term( s_websvc);
   s_websvc = INVALID_WEBSVC_HANDLE;
}




// q=beit a&mobile=true&max_results=10&longtitude=10.943983489&latitude=23.984398
roadmap_result local_search_resolve_address(
                  void*                context,
                  CB_OnAddressResolved cbOnAddressResolved,
                  const char*          address)
{
   
   return generic_search_resolve_address(s_websvc, data_parser,sizeof(data_parser)/sizeof(wst_parser), "external_search",context, cbOnAddressResolved, address );
} 

// Callback:   on_local_option
//
// Abstract:   Client data parser
//
// Input:
//    data              -  [in]  Null terminated string to parse
//    context           -  [in]  Caller context
//    more_data_needed  -  [out] Caller need more data
//    rc                -  [out] In case of failure - additional information
//                               regarding the failure
//
// Return:
//    On success     -  Pointer to location following the parsed data.
//                      Example: If 'data' was "abc\ncbs" and method parsed
//                               "abc\n", then method should return pointer
//                               to "cbs".
//    On failure     -  NULL
//                      In this case parsing is stopped and transaction fails.
//
// Expected string:
//    <longtitude>,<latitude>,[state],[county],<city>,<street>,<house number>\n
//
//       Note: The prefix 'AddressCandidate,' was already extracted
//
const char* on_local_option(   /* IN  */   const char*       data,
                                 /* IN  */   void*             context,
                                 /* OUT */   BOOL*             more_data_needed,
                                 /* OUT */   roadmap_result*   rc)
{
   address_candidate ac;
   int               size;

   address_candidate_init( &ac);

   // Default error for early exit:
   (*rc) = err_parser_unexpected_data;

   // Expected data:
   //    <longtitude>,<latitude>,[state],[county],<city>,<street>,<house number>\n

   // 1. Longtitude
   data = ReadDoubleFromString(  data,          //   [in]      Source string
                                 ",",           //   [in,opt]  Value termination
                                 NULL,          //   [in,opt]  Allowed padding
                                 &ac.longtitude,//   [out]     Output value
                                 1);            //   [in]      TRIM_ALL_CHARS, DO_NOT_TRIM, or 'n'

   if( !data)
   {
      roadmap_log( ROADMAP_ERROR, "local_search::on_local_option() - Failed to read 'longtitude'");
      return NULL;
   }

   // Expected data:
   //    <latitude>,[state],[county],<city>,<street>,<house number>\n

   // 2. Latitude
   data = ReadDoubleFromString(  data,          //   [in]      Source string
                                 ",",           //   [in,opt]  Value termination
                                 NULL,          //   [in,opt]  Allowed padding
                                 &ac.latitude,  //   [out]     Output value
                                 1);            //   [in]      TRIM_ALL_CHARS, DO_NOT_TRIM, or 'n'

   if( !data)
   {
      roadmap_log( ROADMAP_ERROR, "local_search::on_local_option() - Failed to read 'latitude'");
      return NULL;
   }

   // Expected data:
   //    [state],[county],<city>,<street>,<house number>\n

   // 3. State [OPTIONAL]
   if( ',' == *data)
      data++;
   else
   {
      size  = ADSR_STATE_STRING_MAX_SIZE;
      data  = ExtractNetworkString(
                  data,       // [in]     Source string
                  ac.state,   // [out,opt]Output buffer
                  &size,      // [in,out] Buffer size / Size of extracted string
                  ",",        // [in]     Array of chars to terminate the copy operation
                  1);         // [in]     Remove additional termination chars

      if( !data)
      {
         roadmap_log( ROADMAP_ERROR, "local_search::on_local_option() - Failed to read 'state'");
         return NULL;
      }
   }

   // Expected data:
   //    [county],<city>,<street>,<house number>\n

   // 4. County [OPTIONAL]
   if( ',' == *data)
      data++;
   else
   {
      size  = ADSR_COUNTY_STRING_MAX_SIZE;
      data  = ExtractNetworkString(
                  data,       // [in]     Source string
                  ac.county,  // [out,opt]Output buffer
                  &size,      // [in,out] Buffer size / Size of extracted string
                  ",",        // [in]     Array of chars to terminate the copy operation
                  1);         // [in]     Remove additional termination chars

      if( !data)
      {
         roadmap_log( ROADMAP_ERROR, "local_search::on_local_option() - Failed to read 'county'");
         return NULL;
      }
   }


   // Expected data:
   //    <city>,<street>,<house number>\n

   // 5. City
   size  = ADSR_CITY_STRING_MAX_SIZE;
   data  = ExtractNetworkString(
               data,    // [in]     Source string
               ac.city, // [out,opt]Output buffer
               &size,   // [in,out] Buffer size / Size of extracted string
               ",",     // [in]     Array of chars to terminate the copy operation
               1);      // [in]     Remove additional termination chars

   if( !data)
   {
      roadmap_log( ROADMAP_ERROR, "local_search::on_local_option() - Failed to read 'city'");
      return NULL;
   }

   // Expected data:
   //    <street>,<house number>\n

   // 6. Street
   if( ',' == *data)
      data++;
   else
   {
      size  = ADSR_STREET_STRING_MAX_SIZE;
      data  = ExtractNetworkString(
                     data,       // [in]     Source string
                     ac.street,  // [out,opt]Output buffer
                     &size,      // [in,out] Buffer size / Size of extracted string
                     ",",        // [in]     Array of chars to terminate the copy operation
                     1);         // [in]     Remove additional termination chars

      if( !data)
      {
         roadmap_log( ROADMAP_ERROR, "local_search::on_local_option() - Failed to read 'street'");
         return NULL;
      }
   }

   // Expected data:
   //    <house number>\n

   // 7. House number

   if( ',' == *data)
         data++;
   else
   {
      data = ReadIntFromString(
                  data,             //   [in]      Source string
                  ",",             //   [in,opt]  Value termination
                  NULL,             //   [in,opt]  Allowed padding
                  &ac.house,        //   [out]     Put it here
                  1);  //   [in]      Remove additional termination CHARS

      if( !data)
      {
         roadmap_log( ROADMAP_ERROR, "address_search::on_local_option() - Failed to read 'house number'");
         return NULL;
      }
   }

   // 8. Name
   if( ',' == *data)
      data++;
   else
   {
      size  = ADSR_NAME_STRING_MAX_SIZE;
      data  = ExtractNetworkString(
                  data,       // [in]     Source string
                  ac.name,  // [out,opt]Output buffer
                  &size,      // [in,out] Buffer size / Size of extracted string
                  ",",        // [in]     Array of chars to terminate the copy operation
                  1);         // [in]     Remove additional termination chars

      if( !data)
      {
         roadmap_log( ROADMAP_ERROR, "address_search::on_local_option() - Failed to read 'name'");
         return NULL;
      }
   }

   // 9. Phone
   if( ',' == *data)
      data++;
   else
   {
      size  = ADSR_PHONE_STRING_MAX_SIZE;
      data  = ExtractNetworkString(
                  data,       // [in]     Source string
                  ac.phone,  // [out,opt]Output buffer
                  &size,      // [in,out] Buffer size / Size of extracted string
                  ",",        // [in]     Array of chars to terminate the copy operation
                  1);         // [in]     Remove additional termination chars

      if( !data)
      {
         roadmap_log( ROADMAP_ERROR, "address_search::on_local_option() - Failed to read 'phone'");
         return NULL;
      }
   }

   // 10. URL
   if( '\n' == *data)
      data++;
   else
   {
      size  = ADSR_URL_STRING_MAX_SIZE;
      data  = ExtractNetworkString(
                  data,       // [in]     Source string
                  ac.url,  // [out,opt]Output buffer
                  &size,      // [in,out] Buffer size / Size of extracted string
                  "\n",        // [in]     Array of chars to terminate the copy operation
                  TRIM_ALL_CHARS);         // [in]     Remove additional termination chars

      if( !data)
      {
         roadmap_log( ROADMAP_ERROR, "address_search::on_local_option() - Failed to read 'url'");
         return NULL;
      }
   }

//   if( !ac.city[0] && !ac.street[0])
//   {
//      roadmap_log( ROADMAP_ERROR, "local_search::on_local_option() - Result does not have 'city-name' AND 'street-name'");
//      return NULL;
//   }

#ifdef _DEBUG
   assert( local_candidate_build_address_string( &ac));
#else
   local_candidate_build_address_string( &ac);
#endif   // _DEBUG

   generic_address_add(ac);

   // Fix [out] param:
   (*rc) = succeeded;

   return data;
}
