/* RealtimeTrafficInfo.c - Manage real time traffic info
 *
 * LICENSE:
 *
 *   Copyright 2008 Avi B.S 
 *
 *   This file is part of RoadMap.
 *
 *   RoadMap is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   RoadMap is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with RoadMap; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *
 */

#include <stdlib.h>
#include <string.h> 
#include <assert.h>

#include "../roadmap_main.h"
#include "../roadmap_factory.h"
#include "../roadmap_config.h"
#include "../roadmap_math.h"
#include "../roadmap_object.h"
#include "../roadmap_types.h"
#include "../roadmap_lang.h"
#include "../roadmap_trip.h"
#include "../roadmap_line.h"
#include "../roadmap_res.h"
#include "../roadmap_layer.h"
#include "../roadmap_square.h"
#include "../roadmap_locator.h"
#include "../roadmap_line_route.h"
#include "../roadmap_street.h"
#include "../roadmap_math.h"

#include "Realtime.h"
#include "RealtimeNet.h"
#include "RealtimeTrafficInfo.h"
#include "RealtimeAlerts.h" 
#include "RealtimeTrafficInfoPlugin.h"


static RTTrafficInfos gTrafficInfoTable;
static RTTrafficLines gRTTrafficInfoLinesTable;
 
  
static BOOL RTTrafficInfo_GenerateAlert(RTTrafficInfo *pTrafficInfo, int iNodeNumber);
static BOOL RTTrafficInfo_DeleteAlert(int iID);

 /** 
 * Initialize the Traffic info structure 
 * @param pTrafficInfo - pointer to the Traffic info
 * @return None
 */
void RTTrafficInfo_InitRecord(RTTrafficInfo *pTrafficInfo)
{ 
	int i;
	 
	pTrafficInfo->iID            = -1;
	pTrafficInfo->iType        = -1;
	pTrafficInfo->fSpeed      = 0.0L;
	pTrafficInfo->sCity[0]    =  0;
	pTrafficInfo->sStreet[0] =  0;
	pTrafficInfo->sEnd[0]    =  0;
	pTrafficInfo->sStart[0]   = 0;
	pTrafficInfo->sDescription[0] = 0;
	pTrafficInfo->iNumNodes = 0;
	pTrafficInfo->iDirection = -1;
	for ( i=0; i<RT_TRAFFIC_INFO_MAX_NODES;i++){
		pTrafficInfo->sNodes[i].iNodeId = -1;
		pTrafficInfo->sNodes[i].Position.latitude    = -1;
		pTrafficInfo->sNodes[i].Position.longitude = -1;
	}
	
} 

/**
 * Initialize the Traffic Info 
 * @param None
 * @return None
 */
void RTTrafficInfo_Init()
{
	int i;
	gTrafficInfoTable.iCount = 0;
	for (i=0; i< RT_TRAFFIC_INFO_MAXIMUM_TRAFFIC_INFO_COUNT; i++){
		gTrafficInfoTable.pTrafficInfo[i] = NULL;
	}
  
   gRTTrafficInfoLinesTable.iCount = 0;
   for (i=0;i <MAX_LINES; i++){
		gRTTrafficInfoLinesTable.pRTTrafficInfoLines[i] = NULL;
	}
	
		
   RealtimeTrafficInfoPluginInit();
}
 
/** 
 * Terminate the TrafficInfo
 * @param None
 * @return None
 */
void RTTrafficInfo_Term()
{
	RTTrafficInfo_ClearAll(); 
	RealtimeTrafficInfoPluginTerm();
}
 
/**
 * Reset the TrafficInfo table
 * @param None
 * @return None
 */
void RTTrafficInfo_Reset()
{ 
   RTTrafficInfo_ClearAll();
}

/**
 * Clears all entries in the TrafficInfo table
 * @param None
 * @return None
 */
void RTTrafficInfo_ClearAll()
{
   int i; 
   RTTrafficInfo *pRTTrafficInfo;
   int count;
   
   count = gTrafficInfoTable.iCount;
   gTrafficInfoTable.iCount = 0;
   for( i=0; i< count; i++)
   {
      pRTTrafficInfo = gTrafficInfoTable.pTrafficInfo[i];
       RTTrafficInfo_DeleteAlert(pRTTrafficInfo->iID);
      free(pRTTrafficInfo);
      gTrafficInfoTable.pTrafficInfo[i] = NULL;
   }
   
   count = gRTTrafficInfoLinesTable.iCount;
   gRTTrafficInfoLinesTable.iCount = 0;
   for (i=0; i<count;i++)
   {
   	free (gRTTrafficInfoLinesTable.pRTTrafficInfoLines[i]);
   	gRTTrafficInfoLinesTable.pRTTrafficInfoLines[i] = NULL;
   }
 
}
 
/**
 * Checks whether the TrafficInfo table is empty
 * @param None
 * @return TRUE if the table is empty. FALSE, otherwise
 */
BOOL RTTrafficInfo_IsEmpty( )
{ return (0 == gTrafficInfoTable.iCount);}


/**
 * The number of TrafficInfo Record currently in the table
 * @param None
 * @return the number of Records
 */
int RTTrafficInfo_Count(void)
{
    return gTrafficInfoTable.iCount;
}

/**
 * Retrieves a TrafficInfo record by its ID
 * @param iInfoID - the id of the traffic info
 * @return a pointer to the roud inf, NULL if not found
 */
RTTrafficInfo *RTTrafficInfo_RecordByID(int iInfoID){
	 int i;

    for (i=0; i< gTrafficInfoTable.iCount; i++)
        if (gTrafficInfoTable.pTrafficInfo[i]->iID == iInfoID)
        {
            return (gTrafficInfoTable.pTrafficInfo[i]);
        }

    return NULL;
}

/**
 * Checks whether a traffic info record exists
 * @param iInfoID - the id of the traffic info
 * @return TRUE, the record exists. FALSE, the record does not exists
 */
BOOL  RTTrafficInfo_Exists  (int    iInfoID)
{
   if (NULL == RTTrafficInfo_RecordByID(iInfoID))
      return FALSE;
         
   return TRUE;
}

/**
 * Generate an Alert from TrafficInfo 
  * @param pTrafficInfo - pointer to the TrafficInfo
 * @return TRUE operation was successful
 */
static BOOL RTTrafficInfo_GenerateAlert(RTTrafficInfo *pTrafficInfo, int iNodeNumber)
{
	RTAlert alert;
	int speed;
	
	RTAlerts_Alert_Init(&alert);

	alert.iID = pTrafficInfo->iID +  ALERT_ID_OFFSET;
		
	alert.iType = RT_ALERT_TYPE_TRAFFIC_INFO;
	alert.iSpeed = (int)(roadmap_math_meters_p_second_to_speed_unit( (float)pTrafficInfo->fSpeed)+0.5F);
	
	strncpy_safe(alert.sLocationStr, pTrafficInfo->sDescription,RT_ALERT_LOCATION_MAX_SIZE);
	
	speed = (int)(roadmap_math_meters_p_second_to_speed_unit((float)pTrafficInfo->fSpeed));
	sprintf(alert.sDescription, roadmap_lang_get("Average speed %d %s"), speed,  roadmap_lang_get(roadmap_math_speed_unit()) );
	alert.iDirection = RT_ALERT_MY_DIRECTION;
	alert.i64ReportTime = time(NULL); 
	
	alert.bAlertByMe = FALSE;
	alert.iLatitude = pTrafficInfo->sNodes[iNodeNumber].Position.latitude;
	alert.iLongitude = pTrafficInfo->sNodes[iNodeNumber].Position.longitude;
	
	return RTAlerts_Add(&alert);
	
}

/**
 * Remove a Alert that was generated from TrafficInfo
 * @param iID - Id of the TrafficInfo
 * @return TRUE - the delete was successfull, FALSE the delete failed
 */ 
static BOOL RTTrafficInfo_DeleteAlert(int iID)
{
	return RTAlerts_Remove(iID + ALERT_ID_OFFSET);
}

/**
 * Checks whether 2 GPS points are equal (or very close according to the allowed deciation)
 * @param a - Pointer to first opoint
 * @param a - Pointer to second opoint
 * @param allowedDeviation - The allowed differnces to be considered equal
 * @return TRUE - If the points are equal
 */ 
static BOOL samePosition(RoadMapPosition *a, RoadMapPosition *b ,int allowedDeviation){
	if ((a->latitude == b->latitude) && (a->longitude == b->longitude))
		return TRUE;
	else if ( (abs (a->latitude - b->latitude) < allowedDeviation) && (abs (a->longitude - b->longitude) < allowedDeviation) )
			return TRUE;
	else
			return FALSE;
}

/**
 * Extend line callback
 * @param line - pointer to line
 * @param context - context passed to callback
 * @param extend_flags 
 * 
 */ 
static int  extendCallBack (const PluginLine *line, void *context, int extend_flags){

 	int first_shape, last_shape;
	RoadMapPosition from;
   RoadMapPosition to;
   RoadMapShapeItr shape_itr;
   RTTrafficInfo *pTrafficInfo = (RTTrafficInfo *)context;
	int iLinesCount;

	if (gRTTrafficInfoLinesTable.iCount == MAX_LINES){
		roadmap_log( ROADMAP_ERROR, "RealtimeTraffic extendCallBack line table is full");
		return 0 ;
	}
	
	iLinesCount = gRTTrafficInfoLinesTable.iCount;
	
	gRTTrafficInfoLinesTable.pRTTrafficInfoLines[iLinesCount] = calloc(1, sizeof(RTTrafficInfoLines));
		
	gRTTrafficInfoLinesTable.pRTTrafficInfoLines[iLinesCount]->pluginLine = *line;
	gRTTrafficInfoLinesTable.pRTTrafficInfoLines[iLinesCount]->iType = pTrafficInfo->iType;
	gRTTrafficInfoLinesTable.pRTTrafficInfoLines[iLinesCount]->iSpeed = (int) roadmap_math_meters_p_second_to_speed_unit((float)pTrafficInfo->fSpeed);
	
	gRTTrafficInfoLinesTable.pRTTrafficInfoLines[iLinesCount]->iTrafficInfoId =pTrafficInfo->iID; 

	roadmap_square_set_current(line->square);
	 		
   roadmap_plugin_get_line_points (line,
            	                             &from, &to,
               	                          &first_shape, &last_shape,
                  	                       &shape_itr);
   gRTTrafficInfoLinesTable.pRTTrafficInfoLines[iLinesCount]->positionFrom.longitude = from.longitude;
   gRTTrafficInfoLinesTable.pRTTrafficInfoLines[iLinesCount]->positionFrom.latitude = from.latitude;
   gRTTrafficInfoLinesTable.pRTTrafficInfoLines[iLinesCount]->positionTo.longitude = to.longitude;
   gRTTrafficInfoLinesTable.pRTTrafficInfoLines[iLinesCount]->positionTo.latitude = to.latitude;
   gRTTrafficInfoLinesTable.pRTTrafficInfoLines[iLinesCount]->iFirstShape = first_shape;
   gRTTrafficInfoLinesTable.pRTTrafficInfoLines[iLinesCount]->iLastShape = last_shape;
   gRTTrafficInfoLinesTable.pRTTrafficInfoLines[iLinesCount]->iDirection = pTrafficInfo->iDirection;
   gRTTrafficInfoLinesTable.iCount++;
   return 0;
}

/**
 * Finds a line ID given 2 GPS points
 * @param pTrafficInfo - pointer to traffic info 
 * @param start - first node point
 * @param end - second node point
 * @param Line - point to line to return the found line
 * @param allowedDeviation - allowed deviation when searching the nodes
 * @return TRUE - If a line is found
 */ 
static BOOL RTTrafficInfo_Get_LineId(RTTrafficInfo *pTrafficInfo, RoadMapPosition *start, RoadMapPosition *end, char *name, PluginLine *Line, int allowedDeviation){
	RoadMapNeighbour neighbours_start[6], neighbours_end[6];
	int count_start, count_end = 0;
   int layers[128];
   int layers_count;
   RoadMapPosition context_save_pos;
   int context_save_zoom;
	RoadMapPosition from;
	RoadMapPosition to;
	int i;


    roadmap_math_get_context(&context_save_pos, &context_save_zoom);
    
    layers_count = roadmap_layer_all_roads(layers, 128);

    roadmap_math_set_context(start, 20);
    count_start = roadmap_street_get_closest(start, 0, layers, layers_count,
            &neighbours_start[0], 5);

    // first try to find it in the same segment
	for (i=0; i<count_start; i++){
		RoadMapStreetProperties properties;
		const char *street_name;
		roadmap_square_set_current(neighbours_start[i].line.square);
		roadmap_street_get_properties(neighbours_start[i].line.line_id, &properties);
		street_name = roadmap_street_get_street_fename(&properties);
		if (strcmp(street_name, pTrafficInfo->sStreet))
			continue;
	    roadmap_plugin_line_from (&neighbours_start[i].line, &from);
	    roadmap_plugin_line_to (&neighbours_start[i].line, &to);
 		if ( ( samePosition(&from, start,allowedDeviation) && samePosition(&to, end,allowedDeviation)) || (samePosition(&from, end,allowedDeviation) && samePosition(&to, start,allowedDeviation))  ){
		   		if (samePosition(&from, start,allowedDeviation))
		   			pTrafficInfo->iDirection = ROUTE_DIRECTION_WITH_LINE;
		   		else
		   			pTrafficInfo->iDirection = ROUTE_DIRECTION_AGAINST_LINE;
 		 			roadmap_math_set_context(&context_save_pos, context_save_zoom);
		   		*Line = neighbours_start[i].line;
 					return TRUE;
 		}
	}
	    
	//if not try to extend the line on the starting point
	for (i=0; i<count_start; i++){
		 RoadMapStreetProperties properties;
		 const char *street_name;
		 roadmap_square_set_current(neighbours_start[i].line.square);
		 roadmap_street_get_properties(neighbours_start[i].line.line_id, &properties);
		 street_name = roadmap_street_get_street_fename(&properties);
		 if (strcmp(street_name, pTrafficInfo->sStreet)){
			continue;		
		 }

 		 roadmap_street_extend_line_ends (&neighbours_start[i].line, &from, &to, FLAG_EXTEND_BOTH, NULL, NULL);
 		 if ( ( samePosition(&from, start,allowedDeviation) && samePosition(&to, end,allowedDeviation)) || (samePosition(&from, end,allowedDeviation) && samePosition(&to, start,allowedDeviation))  ){
		   		if (samePosition(&from, start,allowedDeviation))
		   			pTrafficInfo->iDirection = ROUTE_DIRECTION_WITH_LINE;
		   		else
		   			pTrafficInfo->iDirection = ROUTE_DIRECTION_AGAINST_LINE;
 		 			roadmap_street_extend_line_ends (&neighbours_start[i].line, &from, &to, FLAG_EXTEND_BOTH, extendCallBack, (void *)pTrafficInfo);
 		 			roadmap_math_set_context(&context_save_pos, context_save_zoom);
		   		*Line = neighbours_start[i].line;
 					return TRUE;
 		 }
	}

	roadmap_math_set_context(end, 20);       
    count_end = roadmap_street_get_closest(end, 0, layers, layers_count,
            &neighbours_end[0], 4);

	for (i=0; i<count_end; i++){
		 RoadMapStreetProperties properties;
		 const char *street_name;
		 roadmap_square_set_current(neighbours_end[i].line.square);
		 roadmap_street_get_properties(neighbours_end[i].line.line_id, &properties);
		 street_name = roadmap_street_get_street_fename(&properties);
		 if (strcmp(street_name, pTrafficInfo->sStreet)){
			continue;		
		 }

 		 roadmap_street_extend_line_ends (&neighbours_end[i].line, &from, &to, FLAG_EXTEND_BOTH, NULL, NULL);
 		 if ( ( samePosition(&from, start,allowedDeviation) && samePosition(&to, end,allowedDeviation)) || (samePosition(&from, end,allowedDeviation) && samePosition(&to, start,allowedDeviation))  ){
		     		if (samePosition(&to, end,allowedDeviation))
		   			pTrafficInfo->iDirection = ROUTE_DIRECTION_WITH_LINE;
		   		else
		   			pTrafficInfo->iDirection = ROUTE_DIRECTION_AGAINST_LINE;
		   
 		 			roadmap_street_extend_line_ends (&neighbours_end[i].line, &from, &to, FLAG_EXTEND_BOTH, extendCallBack, (void *)pTrafficInfo);
 		 			roadmap_math_set_context(&context_save_pos, context_save_zoom);
		   		*Line = neighbours_end[i].line;
 					return TRUE;
 		 }
	}

	roadmap_math_set_context(&context_save_pos, context_save_zoom);
	return FALSE;
}	

/**
 * Add a TrafficInfo Segment to list of segments
 * @param pTrafficInfo - pointer to the TrafficInfo
 * @return TRUE operation was successful
 */
static BOOL RTTrafficInfo_AddSegments(RTTrafficInfo *pTrafficInfo){
	int i;
	RoadMapPosition start, end;
	PluginLine line;
	BOOL found;


	
	 for (i=0; i < pTrafficInfo->iNumNodes-1; i++){
	 	start.latitude = pTrafficInfo->sNodes[i].Position.latitude;
	 	start.longitude = pTrafficInfo->sNodes[i].Position.longitude;
	 	
	 	end.latitude = pTrafficInfo->sNodes[i+1].Position.latitude;
	 	end.longitude = pTrafficInfo->sNodes[i+1].Position.longitude;
	 	
	 	found = RTTrafficInfo_Get_LineId(pTrafficInfo, &start, &end, pTrafficInfo->sStreet, &line,300);
	 	if (found){
			extendCallBack(&line,  (void *)pTrafficInfo, 0);
	 	}
	 	else{
	 	}
    }
    
    return TRUE;
} 

static void RTTraficInfo_RemoveSegments(int iRecordNumber){
 	int i;

    free(gRTTrafficInfoLinesTable.pRTTrafficInfoLines[iRecordNumber]);

    for (i=iRecordNumber; i<(gRTTrafficInfoLinesTable.iCount-1); i++) {
            gRTTrafficInfoLinesTable.pRTTrafficInfoLines[i] = gRTTrafficInfoLinesTable.pRTTrafficInfoLines[i+1];
     }
  
     gRTTrafficInfoLinesTable.iCount--;

     gRTTrafficInfoLinesTable.pRTTrafficInfoLines[gRTTrafficInfoLinesTable.iCount] = NULL;
}

/**
 * Remove a TrafficInfo Segment from the list of segments
 * @param iTrafficInfoID - The pointer to the traffic ID
 * @return TRUE operation was successful
 */
static BOOL RTTraficInfo_DeleteSegments(int iTrafficInfoID){
	 int i = 0;
	 BOOL found = FALSE;
	    
	 //   Are we empty?
    if ( 0 == gRTTrafficInfoLinesTable.iCount)
        return FALSE;
    
    while (i< gRTTrafficInfoLinesTable.iCount){
    	if (gRTTrafficInfoLinesTable.pRTTrafficInfoLines[i]->iTrafficInfoId == iTrafficInfoID){
    		RTTraficInfo_RemoveSegments(i);
    		found = TRUE;
    	}
    	else
    		i++;
    }
  	   
	return found;
}

/**
 * Add a TrafficInfo element to the list of the Traffic Info table 
 * @param pTrafficInfo - pointer to the TrafficInfo
 * @return TRUE operation was successful
 */
BOOL RTTrafficInfo_Add(RTTrafficInfo *pTrafficInfo)
{
	int i;
	

    // Full?
    if ( RT_TRAFFIC_INFO_MAXIMUM_TRAFFIC_INFO_COUNT == gTrafficInfoTable.iCount)
        return FALSE;

    // Already exists?
    if (RTTrafficInfo_Exists(pTrafficInfo->iID))
    {
        roadmap_log( ROADMAP_INFO, "RTTrafficInfo_Add - traffic_info record (%d) already exist", pTrafficInfo->iID);
        return TRUE;
    }
    
    gTrafficInfoTable.pTrafficInfo[gTrafficInfoTable.iCount] = calloc(1, sizeof(RTTrafficInfo));
    if (gTrafficInfoTable.pTrafficInfo[gTrafficInfoTable.iCount] == NULL)
    {
        roadmap_log( ROADMAP_ERROR, "RTTrafficInfo_Add - cannot add traffic_info  (%d) calloc failed", pTrafficInfo->iID);
        return FALSE;
    }
    
    RTTrafficInfo_InitRecord(gTrafficInfoTable.pTrafficInfo[gTrafficInfoTable.iCount]);
	gTrafficInfoTable.pTrafficInfo[gTrafficInfoTable.iCount]->iID = pTrafficInfo->iID;
	gTrafficInfoTable.pTrafficInfo[gTrafficInfoTable.iCount]->fSpeed = pTrafficInfo->fSpeed;
	switch (pTrafficInfo->iType){
		case 0:
		case 1:
			gTrafficInfoTable.pTrafficInfo[gTrafficInfoTable.iCount]->iType = TRAFFIC_OK;
			break; 	
		case 2:
			gTrafficInfoTable.pTrafficInfo[gTrafficInfoTable.iCount]->iType = TRAFFIC_MILD; 	
			break;
		case 3:
		case 4:
			gTrafficInfoTable.pTrafficInfo[gTrafficInfoTable.iCount]->iType = TRAFFIC_BAD;
			break;
	}

    strncpy( gTrafficInfoTable.pTrafficInfo[gTrafficInfoTable.iCount]->sCity, pTrafficInfo->sCity, RT_TRAFFIC_INFO_ADDRESS_MAXSIZE);
    strncpy( gTrafficInfoTable.pTrafficInfo[gTrafficInfoTable.iCount]->sStreet, pTrafficInfo->sStreet, RT_TRAFFIC_INFO_ADDRESS_MAXSIZE);
    strncpy( gTrafficInfoTable.pTrafficInfo[gTrafficInfoTable.iCount]->sStart, pTrafficInfo->sStart, RT_TRAFFIC_INFO_ADDRESS_MAXSIZE);
    strncpy( gTrafficInfoTable.pTrafficInfo[gTrafficInfoTable.iCount]->sEnd, pTrafficInfo->sEnd, RT_TRAFFIC_INFO_ADDRESS_MAXSIZE);
    

    if ((gTrafficInfoTable.pTrafficInfo[gTrafficInfoTable.iCount]->sStreet[0] != 0) & (gTrafficInfoTable.pTrafficInfo[gTrafficInfoTable.iCount]->sCity[0] != 0) ){
    		sprintf(gTrafficInfoTable.pTrafficInfo[gTrafficInfoTable.iCount]->sDescription,roadmap_lang_get("Traffic jam on %s, %s"), gTrafficInfoTable.pTrafficInfo[gTrafficInfoTable.iCount]->sStreet, gTrafficInfoTable.pTrafficInfo[gTrafficInfoTable.iCount]->sCity );
    }
    else if ((gTrafficInfoTable.pTrafficInfo[gTrafficInfoTable.iCount]->sStreet[0] != 0) & (gTrafficInfoTable.pTrafficInfo[gTrafficInfoTable.iCount]->sStart[0] != 0)  & (gTrafficInfoTable.pTrafficInfo[gTrafficInfoTable.iCount]->sEnd[0] != 0) ){
    	if (!strcmp(gTrafficInfoTable.pTrafficInfo[gTrafficInfoTable.iCount]->sStart, gTrafficInfoTable.pTrafficInfo[gTrafficInfoTable.iCount]->sEnd))
    		sprintf(gTrafficInfoTable.pTrafficInfo[gTrafficInfoTable.iCount]->sDescription,roadmap_lang_get("Traffic jam on %s in the neighborhood of %s"), gTrafficInfoTable.pTrafficInfo[gTrafficInfoTable.iCount]->sStreet, gTrafficInfoTable.pTrafficInfo[gTrafficInfoTable.iCount]->sStart );
    	else
    		sprintf(gTrafficInfoTable.pTrafficInfo[gTrafficInfoTable.iCount]->sDescription,roadmap_lang_get("Traffic jam on %s between %s and %s"), gTrafficInfoTable.pTrafficInfo[gTrafficInfoTable.iCount]->sStreet, gTrafficInfoTable.pTrafficInfo[gTrafficInfoTable.iCount]->sStart, gTrafficInfoTable.pTrafficInfo[gTrafficInfoTable.iCount]->sEnd );
    }
   
    gTrafficInfoTable.pTrafficInfo[gTrafficInfoTable.iCount]->iNumNodes = pTrafficInfo->iNumNodes;
    for (i=0; i<pTrafficInfo->iNumNodes; i++){ 
    	gTrafficInfoTable.pTrafficInfo[gTrafficInfoTable.iCount]->sNodes[i].iNodeId = pTrafficInfo->sNodes[i].iNodeId;
    	gTrafficInfoTable.pTrafficInfo[gTrafficInfoTable.iCount]->sNodes[i].Position.latitude= pTrafficInfo->sNodes[i].Position.latitude;
    	gTrafficInfoTable.pTrafficInfo[gTrafficInfoTable.iCount]->sNodes[i].Position.longitude = pTrafficInfo->sNodes[i].Position.longitude;
    }
    
    RTTrafficInfo_AddSegments(gTrafficInfoTable.pTrafficInfo[gTrafficInfoTable.iCount]);
    
    RTTrafficInfo_GenerateAlert(gTrafficInfoTable.pTrafficInfo[gTrafficInfoTable.iCount], (int)(gTrafficInfoTable.pTrafficInfo[gTrafficInfoTable.iCount]->iNumNodes -1)/2);

    gTrafficInfoTable.iCount++;
    
    return TRUE;
}

/**
 * Remove a TrafficInfo Record from table
 * @param iID - Id of the TrafficInfo
 * @return TRUE - the delete was successfull, FALSE the delete failed
 */ 
BOOL RTTrafficInfo_Remove(int iID)
{
    BOOL bFound= FALSE;

    //   Are we empty?
    if ( 0 == gTrafficInfoTable.iCount)
        return FALSE;

    if (gTrafficInfoTable.pTrafficInfo[gTrafficInfoTable.iCount-1]->iID == iID)
    {
        free(gTrafficInfoTable.pTrafficInfo[gTrafficInfoTable.iCount-1]);
        bFound = TRUE;
    }
    else
    {
        int i;

        for (i=0; i<(gTrafficInfoTable.iCount-1); i++)
        {
            if (bFound)
                gTrafficInfoTable.pTrafficInfo[i] = gTrafficInfoTable.pTrafficInfo[i+1];
            else
            {
                if (gTrafficInfoTable.pTrafficInfo[i]->iID == iID)
                {
                    free(gTrafficInfoTable.pTrafficInfo[i]);
                    gTrafficInfoTable.pTrafficInfo[i] = gTrafficInfoTable.pTrafficInfo[i+1];
                    bFound = TRUE;
                }
            }
        }
    }

    if (bFound)
    {
        gTrafficInfoTable.iCount--;

        gTrafficInfoTable.pTrafficInfo[gTrafficInfoTable.iCount] = NULL;
       
        RTTraficInfo_DeleteSegments(iID);
         
        RTTrafficInfo_DeleteAlert (iID);
	 	  
	 }	
	 
    return bFound;
}


/**
 * Get a traffic info record.
 * @param index - the index of the record 
 * @return pointer to the traffic info record at that index.
 */ 
RTTrafficInfo *RTTrafficInfo_Get(int index){
	if ((index >= RT_TRAFFIC_INFO_MAXIMUM_TRAFFIC_INFO_COUNT) || (index < 0))
		return NULL;
	return gTrafficInfoTable.pTrafficInfo[index];
}


/**
 * Get the number of lines in the lines table
 * @None
 * @return Number of lines in the lines table
 */ 
int RTTrafficInfo_GetNumLines(){
	return gRTTrafficInfoLinesTable.iCount;
}

/**
 * Get a line from the lines table 
 * @param Record - index of th record 
 * @return pointer to Line info from Lines table
 */ 
RTTrafficInfoLines *RTTrafficInfo_GetLine(int Record)
{ 
	if ((Record >= MAX_LINES) || (Record < 0))
		return NULL;
	
	return gRTTrafficInfoLinesTable.pRTTrafficInfoLines[Record];
}

/**
 * Find a line from the lines table 
  * @param line - line id
 * @param square - the square of the line 
 * @return Index of line in the LInes table, -1 if no lines is found
 */ 
 static int RTTrafficInfo_Get_Line(int line, int square,  int against_dir){
	int i;
	int direction;
	
	if (gRTTrafficInfoLinesTable.iCount == 0)
		return -1;
	
	if (against_dir)
		direction = ROUTE_DIRECTION_AGAINST_LINE;
	else
		direction = ROUTE_DIRECTION_WITH_LINE;
		
	for (i = 0; i < gRTTrafficInfoLinesTable.iCount; i++){
		if ((gRTTrafficInfoLinesTable.pRTTrafficInfoLines[i]->pluginLine.line_id == line) && 
			  (gRTTrafficInfoLinesTable.pRTTrafficInfoLines[i]->iDirection == direction) &&
			  (gRTTrafficInfoLinesTable.pRTTrafficInfoLines[i]->pluginLine.square == square))
			return i;
	}
	
	return -1;
}

/**
 * Finds the index in the lines tables corresponsinf to a line and square
 * @param line - line id
 * @param square - the square of the line 
 * @return the line_id if line is found in the lines table, -1 otherwise
 */ 
static int RTTrafficInfo_Get_LineNoDirection(int line, int square){
	int i;
	
	if (gRTTrafficInfoLinesTable.iCount == 0)
		return -1;
	
	for (i = 0; i < gRTTrafficInfoLinesTable.iCount; i++){
		if ((gRTTrafficInfoLinesTable.pRTTrafficInfoLines[i]->pluginLine.line_id == line) && (gRTTrafficInfoLinesTable.pRTTrafficInfoLines[i]->pluginLine.square == square))
			return i;
	}
	
	return -1;
}

/**
 * Get The Average speed of a line 
 * @param line - line id
 * @param against_dir 
 * @return Averge real  time speed if exist, 0 otherwise
 */ 
int RTTrafficInfo_Get_Avg_Speed(int line, int square, int against_dir){
	
	int lineRecord = RTTrafficInfo_Get_Line(line, square, against_dir);
	if (lineRecord != -1){
		return gRTTrafficInfoLinesTable.pRTTrafficInfoLines[lineRecord]->iSpeed;
	}
	else
		return 0;
}		

/**
 * Get The Average cross time of a line 
 * @param line - line id
 * @param against_dir 
 * @return Averge real  time cross time if exist, 0 otherwise
 */ 
int RTTrafficInfo_Get_Avg_Cross_Time (int line, int square, int against_dir) {

   int speed;
   int length, length_m;

	speed = RTTrafficInfo_Get_Avg_Speed(line, square, against_dir);
	if (speed ==0)
		return 0;
		
   length = roadmap_line_length (line);
	
   length_m = roadmap_math_to_cm(length) / 100;

   return (int)(length_m*3.6  / speed) + 1;

}
 
/**
 * Returns the alertId corresponding to a line 
 * @param line - line id
 * @param square - the square of the line 
 * @return the alert id If line_id has alert, -1 otherwise
 */ 
int RTTrafficInfo_GetAlertForLine(int iLineid, int iSquareId){
	
	int lineRecord = RTTrafficInfo_Get_LineNoDirection(iLineid, iSquareId);
	if (lineRecord != -1){
		return gRTTrafficInfoLinesTable.pRTTrafficInfoLines[lineRecord]->iTrafficInfoId + ALERT_ID_OFFSET;
	}
	else
		return -1;
}

/**
 * Recalculate all line IDs
 * @param None
 * @return None
 */ 
void RTTrafficInfo_RecalculateSegments(){
	int i;
	
	for (i=0;i<gRTTrafficInfoLinesTable.iCount; i++){
			RTTraficInfo_RemoveSegments(i);	
	}
	
	for (i=0;i<gTrafficInfoTable.iCount; i++){
		RTTrafficInfo_AddSegments(gTrafficInfoTable.pTrafficInfo[i]);
	}
}

