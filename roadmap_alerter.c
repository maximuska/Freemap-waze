/* roadmap_alerter.c -Handles the alerting of users about nearby alerts
 *
 * LICENSE:
 *
 *   Copyright 2008 Avi B.S
 *   Copyright 2008 Ehud Shabtai
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
 * SYNOPSYS:
 *
 *   see roadmap_alerter.h.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "roadmap.h"
#include "roadmap_dbread.h"
#include "roadmap_config.h"
#include "roadmap_res.h"
#include "roadmap_canvas.h"
#include "roadmap_screen.h"
#include "roadmap_screen_obj.h"
#include "roadmap_trip.h"
#include "roadmap_display.h"
#include "roadmap_math.h"
#include "roadmap_navigate.h"
#include "roadmap_pointer.h"
#include "roadmap_line.h"
#include "roadmap_point.h"
#include "roadmap_line_route.h"
#include "roadmap_shape.h"
#include "roadmap_square.h"
#include "roadmap_layer.h"
#include "roadmap_main.h"
#include "roadmap_sound.h"
#include "roadmap_alert.h"
#include "roadmap_alerter.h" 
#include "roadmap_lang.h"
#include "roadmap_softkeys.h"
#include "roadmap_messagebox.h"
#include "editor/db/editor_db.h" 
#include "ssd/ssd_dialog.h"
#include "ssd/ssd_confirm_dialog.h"
#include "navigate/navigate_main.h"

static RoadMapConfigDescriptor AlertsEnabledCfg =
ROADMAP_CONFIG_ITEM("Alerts", "Enable Alerts");

static RoadMapConfigDescriptor AlertsAudioEnabledCfg =
ROADMAP_CONFIG_ITEM("Alerts", "Enable Audio Alerts");

static RoadMapConfigDescriptor MinSpeedToAlertCfg =
ROADMAP_CONFIG_ITEM("Alerts", "Minimum Speed to Alert");



typedef struct {
	int          	active_alert_id;
	int          	distance_to_alert;
	int 	        alert_type;
	int 			alert_providor;	
} active_alert_st;

static int alert_active;
static active_alert_st  the_active_alert;
static BOOL alert_should_be_visible;

static roadmap_alert_providors RoadMapAlertProvidors;

#define AZYMUTH_DELTA  45
 


#define ALERT 1
#define WARN 2

#ifndef TRUE
	#define FALSE              0
	#define TRUE               1
#endif



int  config_alerts_enabled (void) {
	return(roadmap_config_match(&AlertsEnabledCfg, "yes"));
}

int  config_audio_alerts_enabled (void) {
	return(roadmap_config_match(&AlertsAudioEnabledCfg, "yes"));
}

int  config_alerts_min_speed (void) {
	return roadmap_config_get_integer (&MinSpeedToAlertCfg);
}

void roadmap_alerter_register(roadmap_alert_providor *providor){
	  RoadMapAlertProvidors.providor[RoadMapAlertProvidors.count] = providor;
	  RoadMapAlertProvidors.count++;
}


void roadmap_alerter_initialize(void) {

	//minimum speed to check alerts
	roadmap_config_declare
	("preferences", &MinSpeedToAlertCfg, "10", NULL);

	// Enable/Diable audio alerts
	roadmap_config_declare_enumeration
	("preferences", &AlertsAudioEnabledCfg, NULL, "yes", "no", NULL);

	// Enable/Diable alerts
	roadmap_config_declare_enumeration
	("preferences", &AlertsEnabledCfg, NULL, "yes", "no", NULL);

	RoadMapAlertProvidors.count = 0;
	
	alert_should_be_visible = FALSE;
	alert_active = FALSE;
	the_active_alert.active_alert_id = -1;
	the_active_alert.alert_providor = 	-1;

	roadmap_alerter_register(&RoadmapAlertProvidor);


}
//return the ID of the active alert
int roadmap_alerter_get_active_alert_id(){
	return the_active_alert.active_alert_id;
}

static void get_street_from_line (int square, int line, const char **street_name, const char **city_name) {
	
	RoadMapStreetProperties properties;
	roadmap_square_set_current(square);
	roadmap_street_get_properties (roadmap_line_get_street (line), &properties);
	*street_name = roadmap_street_get_street_name (&properties);
	*city_name = roadmap_street_get_city_name (&properties);
}


//return the street name given a GPS position
static int get_street(const RoadMapPosition *position, const char **street_name, const char **city_name){

	int count = 0;
	int layers[128];
	int layers_count;
	RoadMapNeighbour neighbours;

	layers_count =  roadmap_layer_all_roads (layers, 128);
	if (layers_count > 0) {
		count = roadmap_street_get_closest
				(position, 0, layers, layers_count, &neighbours, 1);
		if (count > 0) {
			get_street_from_line (neighbours.line.square, neighbours.line.line_id, street_name, city_name);
			return 0;
		}
		else
			return -1;
	}

	return -1;
}

// checks if a positions is on the same street as a segment
static int check_same_street(const PluginLine *line, const RoadMapPosition *point_position){

	const char *street_name;
	const char *city_name;
	char			current_street_name[512];
	char			current_city_name[512];
	int point_res;
	int square_current = roadmap_square_active ();

	get_street_from_line (line->square, line->line_id, &street_name, &city_name);
	strncpy_safe (current_street_name, street_name, sizeof (current_street_name)); 
	strncpy_safe (current_city_name, city_name, sizeof (current_city_name)); 

	point_res = get_street(point_position, &street_name, &city_name);

	roadmap_square_set_current (square_current);
	
	if (point_res == -1)
		return FALSE;
	
	if (strcmp (current_street_name, street_name) == 0 &&
		 strcmp (current_city_name, city_name) == 0)
		return TRUE;
	else
		return FALSE;

}

static int azymuth_delta (int az1, int az2) {

	int delta;

	delta = az1 - az2;

	while (delta > 180)	 delta -= 360;
	while (delta < -180) delta += 360;

	return delta;
}

static int alert_is_on_route (const RoadMapPosition *point_position, int steering) {
	
	int count = 0;
	int layers[128];
	int layers_count;
	RoadMapNeighbour neighbours;
	int rc = 0;
	int from;
	int to;
	RoadMapPosition from_position;
	RoadMapPosition to_position;
	int road_azymuth;
	int delta;
	int square_current = roadmap_square_active ();

	layers_count =  roadmap_layer_all_roads (layers, 128);
	if (layers_count > 0) {
		count = roadmap_street_get_closest
				(point_position, 0, layers, layers_count, &neighbours, 1);
		if (count > 0) {
			roadmap_square_set_current (neighbours.line.square);
			roadmap_line_points (neighbours.line.line_id, &from, &to);
			roadmap_point_position (from, &from_position);
			roadmap_point_position (to, &to_position);
			road_azymuth = roadmap_math_azymuth (&from_position, &to_position);
			delta = azymuth_delta (steering, road_azymuth);
			if (delta >= -90 && delta < 90)
				rc = navigate_is_line_on_route (neighbours.line.square, neighbours.line.line_id, from, to);
			else
				rc = navigate_is_line_on_route (neighbours.line.square, neighbours.line.line_id, to, from);
		}
	}
	
	roadmap_square_set_current (square_current);
	return rc;
}


//checks whether an alert is within range
static int is_alert_in_range(const RoadMapGpsPosition *gps_position, const PluginLine *line){

	int count;
	int i;
	int distance ;
	int steering;
	RoadMapPosition pos;      
	int azymuth;
	int delta;
	int j;
	unsigned int speed;
	RoadMapPosition gps_pos;
	int square;
	int squares[9];
	int count_squares; 
	
	gps_pos.latitude = gps_position->latitude;
	gps_pos.longitude = gps_position->longitude;
	count_squares = roadmap_square_find_neighbours (&gps_pos, 0, squares);

	for (square = 0; square < count_squares; square++) {
			
		roadmap_square_set_current(squares[square]);
				
		// loop alll prvidor for an alert
		for (j = 0 ; j < RoadMapAlertProvidors.count; j++){
		
			count =  (* (RoadMapAlertProvidors.providor[j]->count)) ();
			
			
			// no alerts for this providor
			if (count == 0) {
				continue;
			}
		
			for (i=0; i<count; i++) {
		
				// if the alert is not alertable, continue. (dummy speed cams, etc.)
				if (!(* (RoadMapAlertProvidors.providor[j]->is_alertable))(i)) {
					continue;
				}
		
				(* (RoadMapAlertProvidors.providor[j]->get_position)) (i, &pos, &steering);
		
				// check that the alert is within alert distance
				distance = roadmap_math_distance(&pos, &gps_pos);
		      
				if (distance > (*(RoadMapAlertProvidors.providor[j]->get_distance))(i)) {
					continue;
				}
		

				// check if the alert is on the navigation route
				if (!alert_is_on_route (&pos, steering)) {
		
					// check that the alert is in the direction of driving
					delta = azymuth_delta(gps_position->steering, steering);
					if (delta > AZYMUTH_DELTA || delta < (0-AZYMUTH_DELTA)) {
						continue;
					}
			
					// check that we didnt pass the alert
					azymuth = roadmap_math_azymuth (&gps_pos, &pos);
					delta = azymuth_delta(azymuth, steering);
					if (delta > 90|| delta < (-90)){
						continue;
			
					}
		
					// check that the alert is on the same street
					if (!check_same_street(line, &pos)) {
						continue;
					}
				}
		
				speed = (* (RoadMapAlertProvidors.providor[j]->get_speed))(i);
				// check that the driving speed is over the allowed speed for that alert
				if ((unsigned int)roadmap_math_to_speed_unit(gps_position->speed) < speed) {
					the_active_alert.alert_type = WARN;
					the_active_alert.distance_to_alert 	= distance;
					the_active_alert.active_alert_id 	=  (* (RoadMapAlertProvidors.providor[j]->get_id))(i);
					the_active_alert.alert_providor = j;
					return TRUE;
				}
				else{
					the_active_alert.alert_type = ALERT;
					the_active_alert.alert_providor = j;	
					the_active_alert.distance_to_alert 	= distance;
					the_active_alert.active_alert_id 	= (* (RoadMapAlertProvidors.providor[j]->get_id))(i);
					return TRUE;
				}
			}
		}
	}
	
	return FALSE;
} 


// Checks if there are alerts to be displayed
void roadmap_alerter_check(const RoadMapGpsPosition *gps_position, const PluginLine *line){

	//check that alerts are enabled
	if (!config_alerts_enabled()) {
		return;
	}

	// check minimum speed
	if (roadmap_math_to_speed_unit(gps_position->speed) < config_alerts_min_speed()) {
		// If there is any active alert turn it off.
		the_active_alert.active_alert_id = -1;
		alert_should_be_visible = FALSE; 
		return;
	}

	// check if there is a alert in range
	if (is_alert_in_range(gps_position, line) > 0) {
		alert_should_be_visible = TRUE;
	} else {
		alert_should_be_visible = FALSE;    
	}
}


// generate the audio alert
void roadmap_alerter_audio(){
	RoadMapSoundList sound_list;

	if (config_audio_alerts_enabled()) {
		sound_list = (* (RoadMapAlertProvidors.providor[the_active_alert.alert_providor]->get_sound)) (roadmap_alerter_get_active_alert_id());
		roadmap_sound_play_list (sound_list);
	}
}


static void delete_callback(int exit_code, void *context){
    BOOL success;
    
    if (exit_code != dec_yes)
         return;
   	success = (* (RoadMapAlertProvidors.providor[the_active_alert.alert_providor]->cancel))(the_active_alert.active_alert_id);
   	if (success)
       	roadmap_messagebox_timeout("Thank you!!!", "Your request was sent to the server",3);
}



static void softkey_callback(void){
	char message[200];
    PluginLine line;
    int direction;
	RoadMapGpsPosition 	*CurrentGpsPoint;
    
    CurrentGpsPoint = malloc(sizeof(*CurrentGpsPoint));
	if (roadmap_navigate_get_current
        (CurrentGpsPoint, &line, &direction) == -1) {
        roadmap_messagebox ("Error", "Can't find current street.");
        return;
    }
    
    roadmap_trip_set_gps_position ("AlertSelection", "Selection", NULL, CurrentGpsPoint);
    
    sprintf(message,"%s\n%s",roadmap_lang_get("Please confirm that the following alert is not relevant:"), roadmap_lang_get((* (RoadMapAlertProvidors.providor[the_active_alert.alert_providor]->get_string)) (the_active_alert.active_alert_id) ));
   
     ssd_confirm_dialog("Delete Alert", message,FALSE, delete_callback,  (void *)NULL);
     alert_should_be_visible = FALSE; 
}

void set_softkey(void){
	static Softkey s;
	strcpy(s.text, "Not there");
	s.callback = softkey_callback;
	
	roadmap_softkeys_set_right_soft_key("Alerter", &s);
}

void remove_softkey(void){
	roadmap_softkeys_remove_right_soft_key("Alerter");
}


// Draw the warning on the screen
void roadmap_alerter_display(void){
	const char * iconName;
	int 	alertId;
	char AlertStr[200];
	RoadMapGuiPoint AlertIconLocation;
	int opacity;	
	char distance_str[100];
//	RoadMapGpsPosition pos;
	RoadMapImage theIcon;
	BOOL is_cancelable;
	
	AlertStr[0] = 0;

	if (alert_should_be_visible) {

			
		if (!alert_active && the_active_alert.alert_type == ALERT) {
		    
			roadmap_alerter_audio();
		}
		
		alertId = roadmap_alerter_get_active_alert_id();
		is_cancelable = (* (RoadMapAlertProvidors.providor[the_active_alert.alert_providor]->is_cancelable)) (alertId);
		if (!alert_active && is_cancelable)
			set_softkey();
		
//		alert_active = TRUE;
//		
//		if (the_active_alert.alert_type == ALERT){
//				iconName =  (* (RoadMapAlertProvidors.providor[the_active_alert.alert_providor]->get_alert_icon)) (alertId);
//		}
//		else {  
//				iconName =  (* (RoadMapAlertProvidors.providor[the_active_alert.alert_providor]->get_warning_icon)) (alertId);
//		} 
//	  
//        snprintf(AlertStr + strlen(AlertStr), sizeof(AlertStr)
//                - strlen(AlertStr), "<b1>%s%s\n",roadmap_lang_get("Approaching"), roadmap_lang_get((* (RoadMapAlertProvidors.providor[the_active_alert.alert_providor]->get_string)) (alertId) ));
//
//        snprintf(AlertStr + strlen(AlertStr), sizeof(AlertStr)
//                - strlen(AlertStr), roadmap_lang_get("%s: %d %s\n"),roadmap_lang_get("In"), the_active_alert.distance_to_alert, roadmap_lang_get(roadmap_math_distance_unit()));
//
// 		roadmap_navigate_get_current (&pos, NULL, NULL);
//        snprintf(AlertStr + strlen(AlertStr), sizeof(AlertStr)
//                - strlen(AlertStr), roadmap_lang_get("%d %s"),roadmap_math_to_speed_unit(pos.speed), roadmap_lang_get(roadmap_math_speed_unit()));
//
//	    roadmap_display_pop_up("Approach Alert", iconName , NULL, AlertStr);

	alertId = roadmap_alerter_get_active_alert_id();
		
		if (the_active_alert.alert_type == ALERT){
				iconName =  (* (RoadMapAlertProvidors.providor[the_active_alert.alert_providor]->get_alert_icon)) (alertId);
				if (iconName == NULL){
					return;
				}
				theIcon = (RoadMapImage) roadmap_res_get(RES_BITMAP, RES_SKIN, iconName );
				if (theIcon == NULL){	
					roadmap_log( ROADMAP_ERROR, "Alerts - Icon Not found (%s)", iconName);
					return;
				}
				alert_active = TRUE;
		}
		else {  
				iconName =  (* (RoadMapAlertProvidors.providor[the_active_alert.alert_providor]->get_warning_icon)) (alertId);
				theIcon = (RoadMapImage) roadmap_res_get(RES_BITMAP, RES_SKIN, iconName);
				if (theIcon == NULL){
					roadmap_log( ROADMAP_ERROR, "Alerts - Icon Not found (%s)", iconName);
					return;
				}
				alert_active = TRUE;
		} 
	  
		AlertIconLocation.x = roadmap_canvas_width () - roadmap_canvas_image_width(theIcon);
		AlertIconLocation.y = roadmap_canvas_height ()/2 - roadmap_canvas_image_height(theIcon)/2 ;
		opacity = 0;
		
		sprintf(distance_str, "%d %s",  the_active_alert.distance_to_alert, roadmap_math_distance_unit());
		roadmap_display_text("Distance To Alert", distance_str);
		roadmap_canvas_draw_image (theIcon, &AlertIconLocation,opacity, IMAGE_NORMAL);

		
	} else {
		if (alert_active && !alert_should_be_visible) {
			alert_active = FALSE;
			if ((* (RoadMapAlertProvidors.providor[the_active_alert.alert_providor]->is_cancelable)) (roadmap_alerter_get_active_alert_id()))
				remove_softkey();
			the_active_alert.active_alert_id = -1;
			roadmap_display_hide("Distance To Alert");
		}
	}
}



