/* roadmap_location.m - iPhone location services
 *
 * LICENSE:
 *
 *   Copyright 2008 Avi R.
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
 *   See roadmap_location.h
 */

#include <stdlib.h>
#include "roadmap.h"
#include "roadmap_location.h";
#include "roadmap_iphonelocation.h";
#include "roadmap_state.h";
#include "roadmap_types.h";
#include "roadmap_trip.h";
#include "roadmap_screen.h";
#include "roadmap_math.h";
#include "roadmap_messagebox.h";
#include "roadmap_main.h";

enum { LOCATION_MODE_OFF = 0,
       LOCATION_MODE_ON
};

static CLLocationManager *RoadMapLocationManager = NULL;
static int RoadMapLocationMode                   = LOCATION_MODE_OFF;
static int RoadMapLocationNavigating             = 0;
static int RoadMapLoactionActive                 = 0;
static int RoadMapLocationSpeed    = 0;
static int RoadMapLocationSteering = 0;

#define MAX_HOR_ACCURACY   1000 //was 80
#define MAX_VER_ACCURACY   1000 //was 120
#define MIN_DISTANCE       15 //meters
#define SPEED_STEP         5  //aprox 10Kph
#define UNIT_MPS_TO_KNOTS  1.9438
#define LOCATION_TIMEOUT   4200 //4.2 seconds timeout

static RoadMapGpsdNavigation RoadmapLocationNavigationListener = NULL;
static RoadMapGpsdDilution   RoadmapLocationDilutionListener   = NULL;
static RoadMapGpsdSatellite RoadmapLocationSatelliteListener  = NULL;

static int SpeedHistory;

struct RoadMapLocationHistory {
   RoadMapPosition                  position;
   struct RoadMapLocationHistory    *next;
};

static struct RoadMapLocationHistory *RoadMapOldestPoint = NULL;


static int roadmap_location_get_location_mode (void);

void roadmap_location_initialize (void){
   SpeedHistory = 0;
   
   roadmap_state_add ("location_mode", &roadmap_location_get_location_mode);
   [[RoadMapLocationController alloc] init];
}

void roadmap_location_start (void){
   [RoadMapLocationManager startUpdatingLocation];
}

void roadmap_location_stop (void){
   [RoadMapLocationManager stopUpdatingLocation];
}

void roadmap_location_toggle (void){
   if (RoadMapLocationMode == LOCATION_MODE_ON) {
      if (!RoadMapLocationNavigating) {
         roadmap_location_stop();
      }
      RoadMapLocationMode = LOCATION_MODE_OFF;
      roadmap_state_refresh ();
   } else {
      if (!RoadMapLocationNavigating) {
         roadmap_location_start();
      } else {
         roadmap_location_stop();
         roadmap_location_start();
      }
      RoadMapLocationMode = LOCATION_MODE_ON;
      roadmap_state_refresh ();
   }
}

void location_timeout (void) {
   if (RoadMapLocationNavigating && RoadMapLoactionActive) {
      RoadmapLocationNavigationListener ('V', ROADMAP_NO_VALID_DATA, ROADMAP_NO_VALID_DATA, 
                                             ROADMAP_NO_VALID_DATA, ROADMAP_NO_VALID_DATA,
                                             ROADMAP_NO_VALID_DATA, ROADMAP_NO_VALID_DATA);
      RoadMapLoactionActive = 0;
   }
}

int roadmap_location_get_location_mode (void) {
   return RoadMapLocationMode;
}

void roadmap_location_subscribe_to_navigation (RoadMapGpsdNavigation navigation) {

   if (RoadMapLocationNavigating == 0) {
      RoadMapLocationNavigating = 1;
      roadmap_main_set_periodic (LOCATION_TIMEOUT, location_timeout);
      if (RoadMapLocationMode == LOCATION_MODE_OFF) {
         roadmap_location_start();
      }
   }
   RoadmapLocationNavigationListener = navigation;
}


void roadmap_location_subscribe_to_dilution (RoadMapGpsdDilution dilution) {

   RoadmapLocationDilutionListener = dilution;
}

void roadmap_location_subscribe_to_satellites (RoadMapGpsdSatellite satellite) {

   RoadmapLocationSatelliteListener = satellite;
}




@implementation RoadMapLocationController

@synthesize locationManager;


- (id) init {
    self = [super init];
    if (self != nil) {
        self.locationManager = [[CLLocationManager alloc] init];
        self.locationManager.delegate = self;
        self.locationManager.distanceFilter = kCLDistanceFilterNone;
        self.locationManager.desiredAccuracy = kCLLocationAccuracyBest;
        
        RoadMapLocationManager = self.locationManager;
    }
    return self;
}

- (void)locationManager:(CLLocationManager *)manager
    didUpdateToLocation:(CLLocation *)newLocation
    fromLocation:(CLLocation *)oldLocation {
    
   
   static int location_is_set = 0;
   int    dimension_of_fix = 3;
   double pdop             = 0.0;
   double hdop             = 0.0;
   double vdop             = 0.0;
   
   char status = 'A';
   int gmt_time;
   int altitude;
   int speed;
   int steering;
   
   int distance;
   int i;
   int NewSpeed;
   NSTimeInterval IntervalSinceOld;
   
   
   RoadMapPosition position1;
   RoadMapPosition position2;
   
   static struct RoadMapLocationHistory *historyPoint = NULL;

   //Check age of the new location and location validity
   if ([newLocation.timestamp timeIntervalSinceNow] < -2 || newLocation.horizontalAccuracy < 0) {
      return;
   }
   
   //GPS handling
   if (RoadMapLocationNavigating) {
      roadmap_main_remove_periodic (location_timeout); //this is safe even if not set before
      roadmap_main_set_periodic (LOCATION_TIMEOUT, location_timeout);
      RoadMapLoactionActive = 1;
      
      //Do we have valid GPS data?
      if (newLocation.verticalAccuracy >= 0 &&
            newLocation.horizontalAccuracy <= MAX_HOR_ACCURACY &&
            newLocation.verticalAccuracy <= MAX_VER_ACCURACY) {
         //Dilution data
         if (newLocation.horizontalAccuracy <= 20) {
            hdop = 1;
         } else if (newLocation.horizontalAccuracy <= 50) {
            hdop = 2;
         } else if (newLocation.horizontalAccuracy <= 100) {
            hdop = 3;
         } else {
            hdop = 4;
         }
         RoadmapLocationDilutionListener (dimension_of_fix, pdop, hdop, vdop);
         
         //Sattelite data
         for (i = 1; i <= 3; i++) {
               RoadmapLocationSatelliteListener (i, 0, 0, 0, 0, 1);
         }
         if (hdop < 3) {
            RoadmapLocationSatelliteListener (4, 0, 0, 0, 0, 1);
         }
         RoadmapLocationSatelliteListener (0, 0, 0, 0, 0, 0);
         
         //Navigation data
         gmt_time = [newLocation.timestamp timeIntervalSince1970];
         
         position1.latitude = floor (oldLocation.coordinate.latitude* 1000000);
         position1.longitude = floor (oldLocation.coordinate.longitude* 1000000);
         position2.latitude = floor (newLocation.coordinate.latitude* 1000000);
         position2.longitude = floor (newLocation.coordinate.longitude* 1000000);
         
         roadmap_gps_raw(gmt_time, position2.longitude, position2.latitude, INVALID_STEERING, 0);
         
         IntervalSinceOld = [newLocation.timestamp 
                              timeIntervalSinceDate:oldLocation.timestamp];
                     
         if (/*IntervalSinceOld < 2 &&*/
             IntervalSinceOld > 0 &&
             oldLocation.horizontalAccuracy <= MAX_HOR_ACCURACY &&
             newLocation.verticalAccuracy <= MAX_VER_ACCURACY) {

            //Check if this is the first time after init
            if (RoadMapOldestPoint == NULL) {
               RoadMapOldestPoint = malloc (sizeof (RoadMapOldestPoint));
               roadmap_check_allocated(RoadMapOldestPoint);
               RoadMapOldestPoint->position.latitude = position1.latitude;
               RoadMapOldestPoint->position.longitude = position1.longitude;
               RoadMapOldestPoint->next = NULL;
            }
            
            //Calculate speed
            distance = roadmap_math_distance(&position1, &position2);
            NewSpeed = floor (distance * UNIT_MPS_TO_KNOTS / 
                                            IntervalSinceOld);
                                            
            if (NewSpeed > SpeedHistory + SPEED_STEP) NewSpeed = SpeedHistory + SPEED_STEP;
            if (NewSpeed < SpeedHistory - SPEED_STEP) NewSpeed = SpeedHistory - SPEED_STEP;
            
            RoadMapLocationSpeed = floor ((NewSpeed*2 + SpeedHistory) / 3);
            SpeedHistory = RoadMapLocationSpeed;

            
            //Calculate azimuth
            distance = roadmap_math_distance (&RoadMapOldestPoint->position, &position2);
            if (distance >= MIN_DISTANCE) {
               RoadMapLocationSteering = roadmap_math_azymuth (&RoadMapOldestPoint->position, &position2);
               while ((distance >= MIN_DISTANCE) &&
                        (RoadMapOldestPoint->next != NULL)) {
                  historyPoint = RoadMapOldestPoint;
                  RoadMapOldestPoint = RoadMapOldestPoint->next;
                  free (historyPoint);
                  distance = roadmap_math_distance (&RoadMapOldestPoint->position, &position2);
               }
            }
            
            //save the new point in history
            historyPoint = RoadMapOldestPoint;
            while (historyPoint->next != NULL) {
               historyPoint = historyPoint->next;
            }
            historyPoint->next = malloc (sizeof (historyPoint));
            historyPoint = historyPoint->next;
            roadmap_check_allocated(historyPoint);
            historyPoint->next = NULL;
            historyPoint->position.latitude = position2.latitude;
            historyPoint->position.longitude = position2.longitude;
         }
         
         speed = RoadMapLocationSpeed;
         steering = RoadMapLocationSteering;

         altitude = floor (newLocation.altitude);
         
         RoadmapLocationNavigationListener (status, gmt_time, position2.latitude, 
                                             position2.longitude, altitude, speed, steering);
         
         if (location_is_set) {
            const char *focus = roadmap_trip_get_focus_name ();
            if (focus && (!strcmp(focus, "Location"))) {
               roadmap_trip_set_focus ("GPS");
            }
            roadmap_trip_remove_point ("Location");
            location_is_set = 0;
         }
         return;
      } else { //No valid GPS data
         RoadmapLocationNavigationListener ('V', ROADMAP_NO_VALID_DATA, ROADMAP_NO_VALID_DATA, 
                                             ROADMAP_NO_VALID_DATA, ROADMAP_NO_VALID_DATA,
                                             ROADMAP_NO_VALID_DATA, ROADMAP_NO_VALID_DATA);
      }
   } else if (RoadMapLocationMode == LOCATION_MODE_OFF) {
      return;
   }
   
   RoadMapPosition position;
   position.longitude = floor (newLocation.coordinate.longitude* 1000000);
   position.latitude = floor (newLocation.coordinate.latitude* 1000000);
   
   location_is_set = 1;
   roadmap_trip_set_point ("Location", &position);
   roadmap_trip_set_focus ("Location");
   roadmap_screen_refresh ();
}

- (void)locationManager:(CLLocationManager *)manager
       didFailWithError:(NSError *)error {
       
   if ([error code] == kCLErrorLocationUnknown) {
      roadmap_log (ROADMAP_WARNING, "Could not find location\n");
      if (RoadMapLocationNavigating) {
         RoadmapLocationNavigationListener ('V', ROADMAP_NO_VALID_DATA, ROADMAP_NO_VALID_DATA, 
                                             ROADMAP_NO_VALID_DATA, ROADMAP_NO_VALID_DATA,
                                             ROADMAP_NO_VALID_DATA, ROADMAP_NO_VALID_DATA);
      RoadMapLoactionActive = 0;
      }
   } else if ([error code] == kCLErrorDenied) {
      roadmap_log (ROADMAP_WARNING, "Location Services denied\n");
      roadmap_messagebox ("Error", "No access to Location Services. Please make sure Location Services is ON");
   }
}

@end
