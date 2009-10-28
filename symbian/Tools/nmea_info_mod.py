__doc__ = """
Graphical python script to talk to a GPS, and display all sorts
 of useful information from it. Can talk to a NMEA Bluetooth GPS,
 or an internal GPS on a 3rd Edition Phone.
 
 mk: Saves a log in a format acceptable by Symbian GPS emulator
 (In Symbian documentation:
 S60 3rd Edition SDK for Symbian OS, Supporting Feature Pack 1, for C++ >
	Tools and Utilities >
		Tools Collection for Location-Based Application Development > 
			Simulation PSY User's Guide)

Series 60 v3 / 3rd Edition Users:
 You need to be running Python 1.3.22 or later, owing to a bug
  with fonts in earlier versions. Ideally, use 1.4.0 or later.
 If you want to use the Location API, rather than bluetooth (eg to
  acess the internal GPS), you must have installed LocationRequestor
  (<http://discussion.forum.nokia.com/forum/showpost.php?p=311182> or 
  <http://www.iyouit.eu/portal/Software.aspx> or
  <http://gagravarr.org/code/locationrequestor_3rd_opensign.sis>)
  and signed all the related .sis files (this, python and 
  LocationRequestor) with your devcert, or using open signed online.

Shows where the satellites are in the sky, and what signal strength
 they have (two different views)
Converts your position into OSGB 36 values, and then generates OS
 easting and northing values + six figure grid ref
Also allows you to log your journeys to a file. In future, the logging
 details will be configurable, and you'll be able send this to another
 bluetooth device (eg a computer), but for now this functionality is
 handled by upload_track.py.
Optionally also logs the GSM location to the file used by stumblestore,
 which you can later uplaod with stumblestore to http://gsmloc.org/
Can take photos which have the exif GPS tags, which indicate where and
 when they were taken.

In future, should have a config pannel
In future, the OS co-ords stuff should be made more general, to handle
 other countries
In future, some of the messages out to be translated into other languages

For now, on the main screen, hit * to increase logging frequency, # to
 decrease it, 0 to immediately log a point, and 8 to toggle logging 
 on and off. Use 5 to toggle stumblestore GSM logging on and off. Use
 2 to switch between metric and imperial speeds (mph vs kmph)
On the Direction Of screen, use 1 and 3 to move between waypoints, 5
 to add a waypoint for the current location, and 8 to delete the
 current waypoint.
On the photos screen, use 1 and 3 to cycle between the possible
 resolutions, and enter or 0 to take a photo.

GPL
  Contributions from Cashman Andrus and Christopher Schmit

Nick Burch - v0.28 (04/05/2008)
"""

# Core imports - special ones occur later
import appuifw
import e32
import e32db
import math
import socket
import time
import os
import sysinfo
import thread
from location import gsm_location

# All of our preferences live in a dictionary called 'pref'
pref = {}


# Default bluetooth address to connect to
# If blank, will prompt you to pick one
pref['def_gps_addr']=''

# By default, LocationRequestor will be used if found, otherwise
#  bluetooth will be used. This may be used to force bluetooth
#  even when LocationRequestor is found
pref['force_bluetooth'] = False

# Should we start off with metric or imperial speeds?
# (Switch by pressing 2 on the main screen)
pref['imperial_speeds'] = True

# How many GGA sentences between logging
# Set to 0 to prevent logging
pref['gga_log_interval'] = 1 # mk

# Threshhold change in GGA sentence to log again 
#  (lessens redundant entries while stationary)
# Values for lat and long are in minutes of arc, as stored in 
#  location['lat_dec'] and location['long_dec'].
#  eg 0.00005 is ~5.6 meters at the equator)  
# Value for alt is in meters.
pref['gga_log_min_lat'] = 0.0
pref['gga_log_min_long'] = 0.0
pref['gga_log_min_alt'] = 0.0
#pref['gga_log_min_lat'] = 0.0001
#pref['gga_log_min_long'] = 0.0001
#pref['gga_log_min_alt'] = 6.0

# Do we have an E: disk (memory card?)
pref['disk'] = 'e:'
if not os.path.exists('e:\\System'):
    pref['disk'] = 'c:'

# Where we store our data and settings
pref['base_dir'] = pref['disk'] + '\\System\\Apps\\NMEA_Info\\'

# File to log GGA sentences into
# May optionally contain macros for datetime elements, as used in
#  time.strftime, eg "file_%y-%m-%d_%H:%M.txt"
# Should end in .txt, so the send script will spot them
#pref['gga_log_file'] = pref['base_dir'] + 'nmea_gga_log.txt'
#pref['gga_log_file'] = pref['base_dir'] + 'nmea_gga_log_%y-%m-%d.txt'
pref['gga_log_file'] = 'e:\\' + 'nmea_info_log_%d-%m_%H-%M.nme'

# File to log debug info into
pref['debug_log_file'] = ''
#pref['debug_log_file'] = pref['base_dir'] + 'nmea_debug_log_%y-%m-%d.txt'

# DB file to hold waypoints for direction-of stuff
pref['waypoints_db'] = pref['base_dir'] + 'waypoints.db'


# Should we also log GSM+lat+long in the stumblestore log file?
# See http://gsmloc.org/ for more details
pref['gsmloc_logging'] = 0


# We want icons etc
# Set this to 'large' if you want the whole screen used
pref['app_screen'] = 'normal'

# Define title etc
pref['app_title'] = "NMEA Info Disp"

# Default location for "direction of"
pref['direction_of_lat'] = '51.858141'
pref['direction_of_long'] = '-1.480210'
pref['direction_of_name'] = '(default)'

#############################################################################

# Ensure our helper libraries are found
import sys
sys.path.append('C:/Python')
sys.path.append(pref['disk'] + '/Python')
try:
    from geo_helper import *
except ImportError:
    appuifw.note(u"geo_helper.py module wasn't found!\nDownload at http://gagravarr.org/code/", "error")
    print "\n"
    print "Error: geo_helper.py module wasn't found\n"
    print "Please download it from http://gagravarr.org/code/ and install, before using program"
    # Try to exit without a stack trace - doesn't always work!
    sys.__excepthook__=None
    sys.excepthook=None
    sys.exit()

has_pexif = None
try:
    from pexif import JpegFile
    has_pexif = True
except ImportError:
    # Will alert them later on
    has_pexif = False

has_camera = None
try:
    import camera
    has_camera = True
except ImportError:
    # Will alert them later on
    has_camera = False
except SymbianError:
    # Will alert them later on
    has_camera = False

# LocationRequestor provides more data
has_locationrequestor = None
try:
    import locationrequestor
    has_locationrequestor = True
except ImportError:
    has_locationrequestor = False

# But positioning is built in
has_positioning = None
try:
    import positioning
    has_positioning = True

    # Big bugs in positioning still, don't use
    # (See 1842737 and 1842719 for starters)
    has_positioning = False
except ImportError:
    has_positioning = False


#############################################################################

# Set the screen size, and title
appuifw.app.screen=pref['app_screen']
appuifw.app.title=unicode(pref['app_title'])

#############################################################################

# Ensure our data directory exists
if not os.path.exists(pref['base_dir']):
    os.makedirs(pref['base_dir'])

# Load the settings
# TODO

#############################################################################

#mk+
def _mkGGA( timestamp, lat ,lon, alt ):
    prefix = "GPGGA"
    suffix = "1,04,1.5,1,M,-24,M,," # Fake (irrelevant) parameters: alt, etc.

    # Construct entry
    line = "%s,%s,%s,%s*" %( prefix,
                             timestamp,
                             ",".join(_encode_latlon(lat, lon)),
                             suffix )
    # Calc checksum
    line = "$%s%02X" %( line, _checksum(line) )
    return line

def _mkGSA( ):
    # Satelites info. fake
    return "$GPGSA,A,3,,,,,,18,19,,26,28,29,,2.2,2.2,1.0*37"

def _mkGLL( timestamp, lat ,lon ):
    # Location info
    prefix = "GPGLL"
    suffix = "A" # Const parameter: "GPS data valid"

    # Construct entry
    line = "%s,%s,%s,%s*" %( prefix,
                             ",".join(_encode_latlon(lat, lon)),
                             timestamp,
                             suffix )
    # Calc checksum
    line = "$%s%02X" %( line, _checksum(line) )

    return line

def _mkRMC( timestamp, lat ,lon, speed_kmh, trueCourse = "" ):
    # Location info
    prefix = "GPRMC"
    suffix = "," # Const parameter: "GPS data valid"

    # Convert speed in km/h to knots:
    speed = speed_kmh / 1.852

    # Fake
    date       = "040609"

    # Construct entry
    line = "%s,%s,A,%s,%.2f,%s,%s,%s*" % (
        prefix,
        timestamp,
        ",".join(_encode_latlon(lat, lon)),
        speed,              # in knots
        str(trueCourse),    # TBD
        date,
        suffix )
    # Calc checksum
    line = "$%s%02X" %( line, _checksum(line) )

    return line

def _encode_latlon(latitude, longitude):
    # Converting latitude in days to a NMEA format ( days|minutes )
    latitude = float(latitude)
    longitude = float(longitude)
    if latitude  < 0: ns = 'S'; latitude *= -1
    else: ns = 'N'
    if longitude < 0: ew = 'W'; latitude *= -1
    else: ew = 'E'
# Bug in formatting code?
#    latitude = "%.4f" %  (int(latitude)  * 100 + float(latitude - int(latitude))*60.0)
#    longitude = "%.4f" % (int(longitude) * 100 + float(longitude - int(longitude))*60.0)
# alternative:
    def format_ll(abs_ll):
        hour_ll = int(abs_ll)
        min_ll  = (abs_ll - hour_ll) * 60
        str_ll = "%02d%02d.%04d" % ( hour_ll,
									 int(min_ll),
									 10000*(min_ll-int(min_ll)) )
        return str_ll

    return (format_ll(latitude), ns, format_ll(longitude), ew)
        
def _checksum(sentence):
    nmeadata,cksum = sentence.split('*', 1)
    csum = 0
    for c in nmeadata:
        csum = csum ^ ord(c)
    return csum
#mk-

#############################################################################

waypoints = []
current_waypoint = 0

# Path to DB needs to be in unicode
pref['waypoints_db'] = unicode(pref['waypoints_db'])

def open_waypoints_db():
    """Open the waypoints DB file, creating if needed"""
    global prefs

    db = e32db.Dbms()
    try:
        db.open(pref['waypoints_db'])
    except:
        # Doesn't exist yet
        db.create(pref['waypoints_db'])
        db.open(pref['waypoints_db'])
        db.execute(u"CREATE TABLE waypoints (name VARCHAR, lat FLOAT, long FLOAT, added TIMESTAMP)")
    return db

def add_waypoint(name,lat,long):
    """Adds a waypoint to the database"""
    global waypoints
    global current_waypoint

    # Escape the name
    name = name.replace(u"'",u"`")

    # Add to the db
    db = open_waypoints_db()
    sql = "INSERT INTO waypoints (name,lat,long,added) VALUES ('%s',%f,%f,#%s#)" % ( name, lat, long, e32db.format_time(time.time()) )
    print sql
    db.execute( unicode(sql) )
    db.close()

    # We would update the waypoints array, but that seems to cause a 
    #  s60 python crash!
    ##waypoints.append( (unicode(name), lat, long) )
    ##current_waypoint = len(waypoints) - 1
    current_waypoint = -1

def delete_current_waypoint():
    """Deletes the current waypoint from the database"""
    global waypoints
    global current_waypoint

    if current_waypoint == 0:
        return
    name = waypoints[current_waypoint][0]

    # Delete from the array
    for waypoint in waypoints:
        if waypoint[0] == name:
            waypoints.remove(waypoint)
    current_waypoint = 0

    # Delete from the db
    db = open_waypoints_db()
    sql = "DELETE FROM waypoints WHERE name='%s'" % ( name )
    print sql
    db.execute( unicode(sql) )
    db.close()

def load_waypoints():
    """Loads our direction-of waypoints"""
    global waypoints
    global current_waypoint

    # First up, go with the default
    waypoints = []
    waypoints.append( (pref['direction_of_name'],pref['direction_of_lat'],pref['direction_of_long']) )

    # Now load from disk
    db = open_waypoints_db()
    dbv = e32db.Db_view()
    dbv.prepare(db, u"SELECT name, lat, long FROM waypoints ORDER BY name ASC")
    dbv.first_line()
    for i in range(dbv.count_line()):
        dbv.get_line()
        waypoints.append( (dbv.col(1), dbv.col(2), dbv.col(3)) )
        dbv.next_line()
    db.close()

# Load our direction-of waypoints
load_waypoints()

#############################################################################

# This is set to 0 to request a quit
going = 1
# Our current location
location = {}
location['valid'] = 1 # Default to valid, in case no GGA/GLL sentences
# Our current motion
motion = {}
# What satellites we're seeing
satellites = {}
# Warnings / errors
disp_notices = ''
disp_notices_count = 0
# Our last written location (used to detect position changes)
last_location = {}
# Our logging parameters
gga_log_interval = 0
gga_log_count = 0
gga_log_fh = ''
debug_log_fh = ''
gsm_log_fh = ''
# Photo parameters
all_photo_sizes = None
photo_size = None
preview_photo = None
# How many times have we shown the current preview?
photo_displays = 0
# Are we currently (in need of) taking a preview photo?
taking_photo = 0

#############################################################################

# Generate the checksum for some data
# (Checksum is all the data XOR'd, then turned into hex)
def generate_checksum(data):
    """Generate the NMEA checksum for the supplied data"""
    csum = 0
    for c in data:
        csum = csum ^ ord(c)
    hex_csum = "%02x" % csum
    return hex_csum.upper()

# Format a NMEA timestamp into something friendly
def format_time(time):
    """Generate a friendly form of an NMEA timestamp"""
    hh = time[0:2]
    mm = time[2:4]
    ss = time[4:]
    return "%s:%s:%s UTC" % (hh,mm,ss)

# Format a NMEA date into something friendly
def format_date(date):
    """Generate a friendly form of an NMEA date"""
    dd = int(date[0:2])
    mm = int(date[2:4])
    yy = int(date[4:6])
    yyyy = yy + 2000
    return format_date_from_parts(yyyy,mm,dd)

def format_date_from_parts(yyyy,mm,dd):
    """Generate a friendly date from yyyy,mm,dd"""
    months = ('Jan', 'Feb', 'Mar', 'Apr', 'May', 'Jun', 'Jul', 'Aug', 'Sep', 'Oct', 'Nov', 'Dec')
    return "%02d %s %d" % (dd, months[(int(mm)-1)], yyyy)

def nmea_format_latlong(lat,long):
    """Turn lat + long into nmea format"""

    def format_ll(hour_digits,ll):
        abs_ll = abs(ll)
        hour_ll = int(abs_ll)
        min_ll = (abs_ll - hour_ll) * 60
        # 4dp, so *10,000
        str_ll = "%03d%02d.%04d" % ( hour_ll, int(min_ll), 10000*(min_ll-int(min_ll)) )
        if hour_digits == 2 and str_ll[0] == '0':
            str_ll = str_ll[1:]
        return str_ll

    if str(lat) == 'NaN':
        flat = '0000.0000N'
    else:
        flat = format_ll(2, lat) + ","
        if lat < 0:
            flat += "S"
        else:
            flat += "N"

    if str(long) == 'NaN':
        flong = '00000.0000E'
    else:
        flong = format_ll(3, long) + ","
        if long < 0:
            flong += "W"
        else:
            flong += "E"
    return (flat,flong)
def user_format_latlong(lat,long):
    """Turn lat + long into a user facing format"""
    if str(lat) == 'NaN':
        flat = '00:00:00N'
    else:
        alat = abs(lat)
        mins = (alat-int(alat))*60
        secs = (mins-int(mins))*60
        flat = "%02d:%02d:%02d" % (int(alat),int(mins),int(secs))
        if lat < 0:
            flat += "S"
        else:
            flat += "N"
    if str(long) == 'NaN':
        flong = '000:00:00W'
    else:
        along = abs(long)
        mins = (along-int(along))*60
        secs = (mins-int(mins))*60
        flong = "%03d:%02d:%02d" % (int(along),int(mins),int(secs))
        if long < 0:
            flong += "W"
        else:
            flong += "E"
    return (flat,flong)

# NMEA data is HHMM.nnnn where nnnn is decimal part of second
def format_latlong(data):
    """Turn HHMM.nnnn into HH:MM.SS"""

    # Check to see if it's HMM.nnnn or HHMM.nnnn or HHHMM.nnnn
    if data[5:6] == '.':
        # It's HHHMM.nnnn
        hh_mm = data[0:3] + ":" + data[3:5]
        dddd = data[6:]
    elif data[3:4] == '.':
        # It's HMM.nnnn
        hh_mm = data[0:1] + ":" + data[1:3]
        dddd = data[4:]
    else:
        # Assume HHMM.nnnn
        hh_mm = data[0:2] + ":" + data[2:4]
        dddd = data[5:]

    # Turn from decimal into seconds, and strip off last 2 digits
    sec = int( float(dddd) / 100.0 * 60.0 / 100.0 )
    return hh_mm + ":" + str(sec)

def format_latlong_dec(data):
    """Turn HHMM.nnnn into HH.ddddd"""
    
    # Check to see if it's HMM.nnnn or HHMM.nnnn or HHHMM.nnnn
    if data[5:6] == '.':
        hours = data[0:3]
        mins = float(data[3:])
    elif data[3:4] == '.':
        hours = data[0:1]
        mins = float(data[1:])
    else:
        hours = data[0:2]
        mins = float(data[2:])

    dec = mins / 60.0 * 100.0
    # Cap at 6 digits - currently nn.nnnnnnnn
    dec = dec * 10000.0
    str_dec = "%06d" % dec
    return hours + "." + str_dec

def get_latlong_floats():
    global location
    wgs_lat = location['lat_dec'];
    wgs_long = location['long_dec'];
    if wgs_lat[-1:] == 'S':
        wgs_lat = '-' + wgs_lat;
    if wgs_long[-1:] == 'W':
        wgs_long = '-' + wgs_long;
    wgs_lat = float(wgs_lat[0:-1])
    wgs_long = float(wgs_long[0:-1])

    return (wgs_lat,wgs_long)

#############################################################################

def readline(sock):
    """Read one single line from the socket"""
    line = ""
    while 1:
        char = sock.recv(1)
        if not char: break
        line += char
        if char == "\n": break
    return line

#############################################################################

def do_gga_location(data):
    """Get the location from a GGA sentence"""
    global location

    # TODO: Detect if we're not getting speed containing sentences, but
    #        we are geting location ones, so we need to compute the speed
    #        for ourselves

    d = data.split(',')
    location['type'] = 'GGA'
    location['lat'] = "%s%s" % (format_latlong(d[1]),d[2])
    location['long'] = "%s%s" % (format_latlong(d[3]),d[4])
    location['lat_dec'] = "%s%s" % (format_latlong_dec(d[1]),d[2])
    location['long_dec'] = "%s%s" % (format_latlong_dec(d[3]),d[4])
    location['lat_raw'] = "%s%s" % (d[1],d[2])
    location['long_raw'] = "%s%s" % (d[3],d[4])
    location['alt'] = "%s %s" % (d[8],d[9])
    location['time'] = format_time(d[0])
    location['tsecs'] = long(time.time())
    if d[5] == '0':
        location['valid'] = 0
    else:
        location['valid'] = 1

def do_gll_location(data):
    """Get the location from a GLL sentence"""
    global location

    d = data.split(',')
    location['type'] = 'GLL'
    location['lat'] = "%s%s" % (format_latlong(d[0]),d[1])
    location['long'] = "%s%s" % (format_latlong(d[2]),d[3])
    location['lat_dec'] = "%s%s" % (format_latlong_dec(d[0]),d[1])
    location['long_dec'] = "%s%s" % (format_latlong_dec(d[2]),d[3])
    location['lat_raw'] = "%s%s" % (d[0],d[1])
    location['long_raw'] = "%s%s" % (d[2],d[3])
    location['time'] = format_time(d[4])
    if d[5] == 'A':
        location['valid'] = 1
    elif d[5] == 'V':
        location['valid'] = 0

def do_rmc_location(data):
    """Get the location from a RMC sentence"""
    global location

    d = data.split(',')
    location['type'] = 'RMC'
    location['lat'] = "%s%s" % (format_latlong(d[2]),d[3])
    location['long'] = "%s%s" % (format_latlong(d[4]),d[5])
    location['lat_dec'] = "%s%s" % (format_latlong_dec(d[2]),d[3])
    location['long_dec'] = "%s%s" % (format_latlong_dec(d[4]),d[5])
    location['lat_raw'] = "%s%s" % (d[2],d[3])
    location['long_raw'] = "%s%s" % (d[4],d[5])
    location['time'] = format_time(d[0])

#############################################################################

def do_gsv_satellite_view(data):
    """Get the list of satellites we can see from a GSV sentence"""
    global satellites
    d = data.split(',')

    # Are we starting a new set of sentences, or continuing one?
    full_view_in = d[0]
    sentence_no = d[1]
    tot_in_view = d[2]

    if int(sentence_no) == 1:
        satellites['building_list'] = []

    # Loop over the satellites in the sentence, grabbing their data
    sats = d[3:]
    while len(sats) > 0:
        prn_num = sats[0]
        elevation = float(sats[1])
        azimuth = float(sats[2])
        sig_strength = float(sats[3])

        satellites[prn_num] = {
            'prn':prn_num,
            'elevation':elevation,
            'azimuth':azimuth,
            'sig_strength':sig_strength
        }

        satellites['building_list'].append(prn_num)
        sats = sats[4:]

    # Have we got all the details from this set?
    if sentence_no == full_view_in:
        satellites['in_view'] = satellites['building_list']
        satellites['in_view'].sort()
        satellites['building_list'] = []
    # All done

def do_gsa_satellites_used(data):
    """Get the list of satellites we are using to get the fix"""
    global satellites
    d = data.split(',')

    sats = d[2:13]
    overall_dop = d[14]
    horiz_dop = d[15]
    vert_dop = d[16]

    while (len(sats) > 0) and (not sats[-1]):
        sats.pop()

    satellites['in_use'] = sats
    satellites['in_use'].sort()
    satellites['overall_dop'] = overall_dop
    satellites['horiz_dop'] = horiz_dop
    satellites['vert_dop'] = vert_dop

def do_vtg_motion(data):
    """Get the current motion, from the VTG sentence"""
    global motion
    d = data.split(',')

    if not len(d[6]):
        d[6] = 0.0
    motion['speed_kmph'] = float(d[6])
    motion['speed_mph'] = float(d[6]) / 1.609344
    motion['true_heading'] = d[0]

    motion['mag_heading'] = ''
    if d[2] and int(d[2]) > 0:
        motion['mag_heading'] = d[2]

#############################################################################

def process_lr_update(args):
    """Process a location update from LocationRequestor"""
    global satellites
    global location
    global motion
    global gps
    #print args

    if len(args) > 2:
        latlong = user_format_latlong(args[1],args[2])
        location['lat']  = latlong[0]
        location['long']  = latlong[1]

        if str(args[1]) == 'NaN':
            location['lat_dec'] = "0.0"
        else:
            location['lat_dec'] = "%02.6f" % args[1]
        if str(args[2]) == 'NaN':
            location['long_dec'] = "0.0"
        else:
            location['long_dec'] = str(args[2])

        location['alt'] = "%3.1f m" % (args[3])

        satellites['horiz_dop'] = "%0.1f" % args[4]
        satellites['vert_dop'] = "%0.1f" % args[5]
        satellites['overall_dop'] = "%0.1f" % ((args[4]+args[5])/2)
    if len(args) > 8:
        if(str(args[8])) == 'NaN':
            #             if motion.has_key('speed'):
            #                 del motion['speed']
            motion['speed_kmph'] = 0
        else:
            motion['speed_kmph'] = float(args[8])
            motion['speed_mph'] = float(args[8]) / 1.609344
        if(str(args[10])) == 'NaN':
            #             if motion.has_key('true_heading'):
            #                 del motion['true_heading']
            motion['true_heading'] = ""
        else:
            motion['true_heading'] = "%0.1f" % args[10]

        location['tsecs'] = long(args[12]/1000) # ms not sec
        timeparts = time.gmtime( location['tsecs'] )
        location['time'] = "%02d:%02d:%02d" % (timeparts[3:6])
        location['date'] = format_date_from_parts(*timeparts[0:3])
        gga_time = "%02d%02d%02d.%02d" % (timeparts[3:7])

        if args[14] == 0:
            location['valid'] = 0
        else:
            location['valid'] = 1
    else:
        if str(args[1]) == 'NaN':
            location['valid'] = 0
        else:
            location['valid'] = 1

        location['tsecs'] = long(args[7]/1000) # ms not sec
        timeparts = time.gmtime( location['tsecs'] )
        location['time'] = "%02d:%02d:%02d" % (timeparts[3:6])
        location['date'] = format_date_from_parts(*timeparts[0:3])
        gga_time = "%02d%02d%02d.%02d" % (timeparts[3:7])


    # Figure out the satellites in view
    in_view = []
    in_use = []

    num_in_view = args[13] + 1 # 0 or 1 based?
    for i in range(num_in_view):
        try:
            sat = gps.lr.GetSatelliteData(i)
            if sat != None and len(sat) >= 5:
                prn = str(sat[0])
                satellites[prn] = {
                    'prn': prn,
                    'elevation': sat[2],
                    'azimuth': sat[1],
                    'sig_strength': sat[3]
                }
                in_view.append(sat[0])
                if sat[4]:
                    in_use.append(sat[0])
        except Exception, reason:
            #print "%d - %s" % (i,reason)
            pass

    # Sometimes in random orders
    in_view.sort()
    in_use.sort()
    # Store, as strings
    satellites['in_view'] = [str(prn) for prn in in_view]
    satellites['in_use']  = [str(prn) for prn in in_use]

    # Fake a GGA sentence, so we can log if required
    #     latlong = nmea_format_latlong(args[1],args[2])
    #     fake_gga = "$GPGGA,%s,%s,%s,%d,%02d,0.0,%0.2f,M,,,,\n" % \
    #         (gga_time,latlong[0],latlong[1],location['valid'],
    #             len(in_view),args[3])
    #     gga_log(fake_gga)
    gga_log( _mkGGA(gga_time, location['lat_dec'], location['long_dec'], location['alt'] ) + "\n" )
    gga_log( _mkGSA() + "\n" )
    gga_log( _mkGLL(gga_time, location['lat_dec'], location['long_dec']) + "\n" )
    gga_log( _mkRMC(gga_time, location['lat_dec'], location['long_dec'],motion['speed_kmph'],motion['true_heading']) + "\n" )


def process_positioning_update(data):
    """Process a location update from the Python Positioning module"""
    global satellites
    global location
    global motion
    global gps

    print data
    return

    latlong = (0,0)
    height = 0
    if data.has_key("position"):
        pos = data["position"]
        latlong = user_format_latlong(pos["latitude"], pos["longitude"])
        location['lat']  = latlong[0]
        location['long']  = latlong[1]

        if str(pos["latitude"]) == 'NaN':
            location['lat_dec'] = "0.0"
            location['valid'] = 0
        else:
            location['lat_dec'] = "%02.6f" % pos["latitude"]
            location['valid'] = 1
        if str(pos["longitude"]) == 'NaN':
            location['long_dec'] = "0.0"
        else:
            location['long_dec'] = "%02.6f" % pos["longitude"]

        height = pos["altitude"]
        location['alt'] = "%3.1f m" % (pos["alitude"])

        satellites['horiz_dop'] = "%0.1f" % pos["horizontal_accuracy"]
        satellites['vert_dop'] = "%0.1f"  % pos["vertical_accuracy"]
        satellites['overall_dop'] = "%0.1f" % \
                    ((pos["horizontal_accuracy"]+pos["vertical_accuracy"])/2)
    if data.has_key("course"):
        cor = data["course"]
        if str(cor["speed"]) == 'NaN':
            if motion.has_key('speed'):
                del motion['speed']
        else:
            # symbian gps speed is in meters per second
            mps = cor["speed"]
            motion['speed_kmph'] = mps / 1000.0 * 60 * 60
            motion['speed_mph'] = motion['speed_kmph'] / 1.609344

        if str(cor["heading"]) == 'NaN':
            if motion.has_key('true_heading'):
                del motion['true_heading']
        else:
            motion['true_heading'] = "%0.1f" % cor["heading"]

    if data.has_key("satellites"):
        sats = data["satellites"]
        location['tsecs'] = sats["time"]
        timeparts = time.gmtime( location['tsecs'] )
        location['time'] = "%02d:%02d:%02d" % (timeparts[3:6])
        location['date'] = format_date_from_parts(*timeparts[0:3])
        gga_time = "%02d%02d%02d.%02d" % (timeparts[3:7])

        # We don't yet get data on the satellites, just the numbers
        satellites['in_view'] = ["??" for prn in range(sats['satellites'])]
        satellites['in_use']  = ["??" for prn in range(sats['used_satellites'])]

    # Fake a GGA sentence, so we can log if required
    latlong = nmea_format_latlong(args[1],args[2])
    fake_gga = "$GPGGA,%s,%s,%s,%d,%02d,0.0,%0.2f,M,,,,\n" % \
               (gga_time,latlong[0],latlong[1],location['valid'],
                len(in_view),height)
    gga_log(fake_gga)

#############################################################################

def expand_log_file_name(proto):
    """Expand a filename prototype, which optionally includes date and time macros."""
    # eg "%y/%m/%d %H:%M"
    expanded = time.strftime(proto, time.localtime(time.time()))
    return expanded

def rename_current_gga_log(new_name):
    """Swap the current position log, for one with the new name"""
    global pref

    close_gga_log()
    pref['gga_log_file'] = new_name
    init_gga_log()

def init_gga_log():
    """Initialize the position log, using pref information"""
    global pref
    global gga_log_count
    global gga_log_interval
    global gga_log_fh

    gga_log_count = 0
    gga_log_interval = pref['gga_log_interval']

    if pref['gga_log_file']:
        # Open the GGA log file, in append mode
        gga_log_fh = open(expand_log_file_name(pref['gga_log_file']),'a');
    else:
        # Set the file handle to False
        gga_log_fh = ''

def close_gga_log():
    """Close the position log file, if it's open"""
    global gga_log_fh
    if gga_log_fh:
        gga_log_fh.flush()
        gga_log_fh.close()
        gga_log_fh = ''

def init_debug_log():
    """Initialise the debug log, using pref information"""
    global pref
    global debug_log_fh

    if pref['debug_log_file']:
        # Open the debug log file, in append mode
        debug_log_fh = open(expand_log_file_name(pref['debug_log_file']),'a')
        debug_log_fh.write("Debug Log Opened at %s\n" % time.strftime('%H:%M:%S, %Y-%m-%d', time.localtime(time.time())))
    else:
        # Set the file handle to False
        debug_log_fh = ''

def close_debug_log():
    global debug_log_fh
    if debug_log_fh:
        debug_log_fh.write("Debug Log Closed at %s\n" % time.strftime('%H:%M:%S, %Y-%m-%d', time.localtime(time.time())))
        debug_log_fh.close()
        debug_log_fh = ''

def init_stumblestore_gsm_log():
    """Initialise the stumblestore GSM log file"""
    global gsm_log_fh
    gsm_log_fh = open("E:\\gps.log",'a')
def close_stumblestore_gsm_log():
    global gsm_log_fh
    if gsm_log_fh:
        gsm_log_fh.close()
        gsm_log_fh = ''

#############################################################################

def location_changed(location, last_location):
    """Checks to see if the location has changed (enough) since the last write"""
    if (not 'lat_dec' in location) or (not 'long_dec' in location):
        return 1
    if (not 'lat_dec' in last_location) or (not 'long_dec' in last_location):
        return 1
    llat = float(location['lat_dec'][:-1])
    llong = float(location['long_dec'][:-1])
    lalt = float(location['alt'][:-2])
    plat = float(last_location['lat_dec'][:-1])
    plong = float(last_location['long_dec'][:-1])
    palt = float(last_location['alt'][:-2])
    if (abs(llat-plat) < pref['gga_log_min_lat']) and (abs(llong-plong) < pref['gga_log_min_long']) and (abs(lalt-palt) < pref['gga_log_min_alt']):
        return 0
    return 1

def gga_log(rawdata):
    """Periodically log GGA data to a file, optionally only if it has changed"""
    global pref
    global gga_log_count
    global gga_log_interval
    global gga_log_fh
    global location
    global last_location

    # If we have a fix, and the location has changed enough, and
    #  we've waited long enough, write out the current position
    if location['valid']:
        gga_log_count = gga_log_count + 1
        if gga_log_count >= gga_log_interval:
            gga_log_count = 0
            if location_changed(location, last_location):
                if gga_log_fh:
                    gga_log_fh.write(rawdata)
                if pref['gsmloc_logging']:
                    gsm_stumblestore_log()
                # Save this location, so we can check changes from it
                last_location['lat_dec'] = location['lat_dec']
                last_location['long_dec'] = location['long_dec']
                last_location['alt'] = location['alt']

def debug_log(rawdata):
    """Log debug data to a file when requested (if enabled)"""
    global debug_log_fh

    if debug_log_fh:
        debug_log_fh.write(rawdata+"\n")

def gsm_stumblestore_log():
    """Log the GSM location + GPS location to the stumblestore log file"""
    global location
    global gsm_log_fh

    # Ensure we have our log file open
    if not gsm_log_fh:
        init_stumblestore_gsm_log()

    # Grab the details of what cell we're on
    cell = gsm_location()

    # Write this out
    gsm_log_fh.write("%s,%s,%s,%s,%s,%s,%s,%s\n"%(cell[0],cell[1],cell[2],cell[3],sysinfo.signal(),location['lat_dec'],location['long_dec'],time.time()))

# Kick of logging, if required
init_gga_log()
init_debug_log()

#############################################################################

# Lock, so python won't exit during non canvas graphical stuff
lock = e32.Ao_lock()

def exit_key_pressed():
    """Function called when the user requests exit"""
    global going
    going = 0
    appuifw.app.exit_key_handler = None
    lock.signal()

def callback(event):
    global gga_log_count
    global gga_log_interval
    global current_waypoint
    global waypoints
    global current_state
    global all_photo_sizes
    global photo_size
    global taking_photo
    global pref

    # If they're on the main page, handle changing logging frequency
    if current_state == 'main' or current_state == 'details':
        if event['type'] == appuifw.EEventKeyDown:
            # * -> more frequently
            if event['scancode'] == 42:
                if gga_log_interval > 0:
                    gga_log_interval -= 1;
            # # -> less frequently
            if event['scancode'] == 127:
                if gga_log_interval > 0:
                    gga_log_interval += 1;
            # 0 -> log a point right now
            if event['scancode'] == 48:
                gga_log_count = gga_log_interval
            # 8 -> toggle on/off
            if event['scancode'] == 56:
                if gga_log_interval > 0:
                    gga_log_interval = 0;
                else:
                    gga_log_interval = 10;
                    gga_log_count = 0;
            # 5 -> toggle stumblestore on/off
            if event['scancode'] == 53:
                if pref['gsmloc_logging']:
                    pref['gsmloc_logging'] = 0
                else:
                    pref['gsmloc_logging'] = 1
            # 2 -> toggle kmph / mph
            if event['scancode'] == 50:
                pref['imperial_speeds'] = not pref['imperial_speeds']
    if current_state == 'direction_of':
        if event['type'] == appuifw.EEventKeyUp:
            # 1 - prev waypoint
            if event['scancode'] == 49:
                current_waypoint = current_waypoint - 1
                if current_waypoint < 0:
                    current_waypoint = len(waypoints) - 1
            # 3 - next waypoint
            if event['scancode'] == 51:
                current_waypoint = current_waypoint + 1
                if current_waypoint >= len(waypoints):
                    current_waypoint = 0
            # 5 - make this a waypoint
            if event['scancode'] == 53:
                do_add_as_waypoint()
                # No redraw just yet
                return
            # 8 - remove this waypoint
            if event['scancode'] == 56:
                delete_current_waypoint()
    if current_state == 'take_photo':
        if event['type'] == appuifw.EEventKeyUp:
            size_index = 0
            for i in range(len(all_photo_sizes)):
                if photo_size == all_photo_sizes[i]:
                    size_index = i

            # 1 - prev resolution
            if event['scancode'] == 49:
                size_index = size_index - 1
                if size_index < 0:
                    size_index = len(all_photo_sizes) - 1
                photo_size = all_photo_sizes[size_index]
            # 3 - next resolution
            if event['scancode'] == 51:
                size_index = size_index + 1
                if size_index >= len(all_photo_sizes):
                    size_index = 0
                photo_size = all_photo_sizes[size_index]
            # 0 or enter - take photo
            if event['scancode'] == 48 or event['scancode'] == 167:
                # Request the main thread take it
                # (Takes too long to occur in the event thread)
                taking_photo = 2

    # Whatever happens request a re-draw
    draw_state()

def do_nothing(picked):
    """Does nothing"""

def render_rectangle(tl_x,tl_y,br_x,br_y, outline=0x000000, width=1):
    """Draw a normal rectangle"""
    canvas.line([tl_x,tl_y,br_x,tl_y], outline=outline, width=width)
    canvas.line([tl_x,br_y,br_x,br_y], outline=outline, width=width)
    canvas.line([tl_x,tl_y,tl_x,br_y], outline=outline, width=width)
    canvas.line([br_x,tl_y,br_x,br_y], outline=outline, width=width)

def draw_main():
    global location
    global motion
    global satellites
    global gps
    global gga_log_interval
    global disp_notices
    global disp_notices_count

    canvas.clear()

    # Draw the top box
    top_box_right = screen_width-10
    top_box_bottom = (line_spacing*2) + 4
    render_rectangle(10,2,top_box_right,top_box_bottom)

    # Draw the two boxes below
    mid = int(screen_width/2)
    left_box_top = (line_spacing*3)
    left_box_bottom = (line_spacing*11)
    right_box_top = (line_spacing*6)
    right_box_bottom = (line_spacing*11)
    right_box_right = screen_width-10

    render_rectangle(10,left_box_top,(mid-10),left_box_bottom)
    render_rectangle((mid+10),right_box_top,right_box_right,right_box_bottom)

    # Draw the heading circle
    heading_centre_r = int(screen_width/4.0*3.0)
    heading_centre_t = int(line_spacing*4.25)
    heading_radius = int(line_spacing*1.5)
    canvas.ellipse([heading_centre_r-heading_radius,heading_centre_t-heading_radius,heading_centre_r+heading_radius,heading_centre_t+heading_radius], outline=0x000000, width=1)

    # If we're connected, show the location at the top
    # Otherwise, show waiting
    yPos = line_spacing + 3
    indent_box = 3*indent_slight
    indent_box_2 = 8*indent_slight
    if gps.connected:
        if (location['valid'] == 0) or (not location.has_key('lat')) or (not location.has_key('long')):
            canvas.text( (indent_box, int(line_spacing*1.5)),
                u'(invalid location)', 0x008000, font )
        else:
            canvas.text( (indent_box-line_spacing, yPos),
                u'Latitude', 0x008000, font=font )
            canvas.text( (indent_box-line_spacing, yPos+line_spacing),
                u'Longitude', 0x008000, font=font )

            canvas.text( (indent_box_2, yPos),
                unicode(location['lat']), font=font )
            canvas.text( (indent_box_2, yPos+line_spacing),
                unicode(location['long']), font=font )
    else:
        canvas.text( (indent_box,yPos), 
            u"-waiting for gps-", 0xdd0000, font)
        canvas.text( (indent_box,yPos+line_spacing),
            unicode(str(gps)), 0xdd0000, font)

    # Heading circle
    if motion.has_key('true_heading'):
        # TODO - finish
        pass
    else:
        canvas.text( (heading_centre_r,heading_centre_t), u'?', 0x008000, font)

    # Left box:
    #   time, date, sats, used, logging

    yPos = left_box_top + line_spacing + 4
    if not location.has_key('time'):
        cur_time = u'(no time)'
    else:
        cur_time = location['time']
        if cur_time[-4:] == ' UTC':
            cur_time = cur_time[:-4]
    canvas.text( (13,yPos), unicode(cur_time), font=font )

    yPos += line_spacing
    if not location.has_key('date'):
        cur_date = u'(no date)'
    else:
        cur_date = location['date']
    canvas.text( (13,yPos), unicode(cur_date), font=font )

    yPos += int(line_spacing*0.5)
    sPos = mid - 3*line_spacing

    yPos += line_spacing
    canvas.text( (13, yPos), u'Sats', 0x008000, font)
    if satellites.has_key('in_view'):
        sat_text = len(satellites['in_view'])
    else:
        sat_text = '(u)'
    canvas.text( (sPos, yPos), unicode(sat_text), font=font )

    yPos += line_spacing
    canvas.text( (13, yPos), u'Used', 0x008000, font)
    if satellites.has_key('in_use'):
        sat_text = len(satellites['in_use'])
    else:
        sat_text = '(u)'
    canvas.text( (sPos, yPos), unicode(sat_text), font=font )

    yPos += int(line_spacing*0.5)

    yPos += line_spacing
    if gga_log_interval > 0:
        canvas.text( (13, yPos), u'Logging', 0x008000, font)

        yPos += line_spacing
        logging = unicode(gga_log_interval) + u' secs'
        if pref['gsmloc_logging']:
            logging = logging + u' +GSM'
        canvas.text( (13,yPos), logging, font=font)
    else:
        canvas.text( (13, yPos), u'No Logging', 0x008000, font)


    # Right box:
    #   speed, heading, altitude?

    yPos = right_box_top + line_spacing + 4
    canvas.text( (mid+13,yPos), u'Speed', 0x008000, font)
    if motion.has_key('speed_mph'):
        if pref['imperial_speeds']:
            cur_speed = "%0.1f mph" % motion['speed_mph']
        else:
            cur_speed = "%0.1f kmph" % motion['speed_kmph']
        cur_speed = unicode(cur_speed)
    else:
        cur_speed = u'(no speed)'
    yPos += line_spacing
    canvas.text( (mid+13,yPos), cur_speed, font=font)

    yPos += line_spacing
    canvas.text( (mid+13,yPos), u'Heading', 0x008000, font)
    if motion.has_key('true_heading'):
        mag = "%s deg" % motion['true_heading']
        mag = unicode(mag)
    else:
        mag = u'(no heading)'
    yPos += line_spacing
    canvas.text( (mid+13,yPos), mag, font=font)

    if not disp_notices == '':
        yPos = left_box_bottom + line_spacing
        canvas.text( (0,yPos), unicode(disp_notices), 0x000080, font)
        disp_notices_count = disp_notices_count + 1
        if disp_notices_count > 60:
            disp_notices = ''
            disp_notices_count = 0

def draw_details():
    global location
    global motion
    global satellites
    global gps
    global gga_log_interval
    global disp_notices
    global disp_notices_count

    canvas.clear()
    yPos = line_spacing

    canvas.text( (0,yPos), u'GPS', 0x008000, font)
    if gps.connected:
        canvas.text( (indent_data,yPos), unicode(str(gps)), font=font)
    else:
        indent = int(indent_data/2)
        canvas.text( (indent,yPos), u"-waiting-"+unicode(str(gps)), 0xdd0000, font)

    yPos += line_spacing
    canvas.text( (0,yPos), u'Time:', 0x008000, font)
    if not location.has_key('time'):
        cur_time = u'(unavailable)'
    else:
        cur_time = unicode(location['time'])
    canvas.text( (indent_data,yPos), cur_time, font=font)

    yPos += line_spacing
    canvas.text( (0,yPos), u'Speed', 0x008000, font)
    if motion.has_key('speed_mph'):
        if pref['imperial_speeds']:
            cur_speed = "%0.1f mph" % motion['speed_mph']
        else:
            cur_speed = "%0.1f kmph" % motion['speed_kmph']
        cur_speed = unicode(cur_speed)
    else:
        cur_speed = u'(unavailable)'
    canvas.text( (indent_data,yPos), cur_speed, font=font)

    yPos += line_spacing
    canvas.text( (0,yPos), u'Heading', 0x008000, font)
    if motion.has_key('true_heading'):
        if motion.has_key('mag_heading') and motion['mag_heading']:
            mag = 'True: ' + motion['true_heading']
            mag = mag + '    Mag: ' + motion['mag_heading']
        else:
            mag = "%s deg" % motion['true_heading']
        mag = unicode(mag)
    else:
        mag = u'(unavailable)'
    canvas.text( (indent_data,yPos), mag, font=font)

    yPos += line_spacing
    canvas.text( (0,yPos), u'Location', 0x008000, font)
    if location.has_key('alt'):
        canvas.text( (indent_large,yPos), unicode(location['alt']), font=font )
    if (not location.has_key('lat')) or (not location.has_key('long')):
        cur_loc = u'(unavailable)'
    else:
        if location['valid'] == 0:
            cur_loc = u'(invalid location)'
        else:
            cur_loc = unicode(location['lat']) + '  ' + unicode(location['long'])
    canvas.text( (indent_slight,yPos+line_spacing), cur_loc, font=font)

    yPos += (line_spacing*2)
    canvas.text( (0, yPos), u'Satellites in view', 0x008000, font)
    if satellites.has_key('in_view'):
        canvas.text( (indent_large,yPos), unicode( len(satellites['in_view']) ), font=font )
        canvas.text( (indent_slight,yPos+line_spacing), unicode(' '.join(satellites['in_view'])), font=font )
    else:
        canvas.text( (indent_slight,yPos+line_spacing), u'(unavailable)', font=font)

    yPos += (line_spacing*2)
    canvas.text( (0, yPos), u'Satellites used', 0x008000, font)
    if satellites.has_key('in_use'):
        used = len(satellites['in_use'])
        if satellites.has_key('overall_dop'):
            used = str(used) + "  err " + satellites['overall_dop']
        canvas.text( (indent_large,yPos), unicode(used), font=font )
        canvas.text( (indent_slight,yPos+line_spacing), unicode(' '.join(satellites['in_use'])), font=font )
    else:
        canvas.text( (indent_slight,yPos+line_spacing), u'(unavailable)', font=font )

    yPos += (line_spacing*2)
    canvas.text( (0, yPos), u'Logging locations', 0x008000, font)
    if gga_log_interval > 0:
        logging = unicode(gga_log_interval) + u' secs'
    else:
        logging = u'no'
    if pref['gsmloc_logging']:
        logging = logging + u'  +GSM'
    canvas.text( (indent_large,yPos), logging, font=font)

    if not disp_notices == '':
        yPos += line_spacing
        canvas.text( (0,yPos), unicode(disp_notices), 0x000080, font)
        disp_notices_count = disp_notices_count + 1
        if disp_notices_count > 60:
            disp_notices = ''
            disp_notices_count = 0

def draw_sat_list():
    global satellites

    canvas.clear()
    if not satellites.has_key('in_view'):
        canvas.text( (0,line_spacing), u'No satellites in view', 0x008000, font)
        return

    sats_in_use = []
    if satellites.has_key('in_use'):
        sats_in_use = satellites['in_use']

    pos = 0
    for sat in satellites['in_view']:
        if not satellites.has_key(sat):
            continue
        pos = pos + line_spacing

        # Draw signal strength on back
        # Strength should be between 0 and 99
        str_len = 0
        if (not satellites[sat]['sig_strength'] == '') and (int(satellites[sat]['sig_strength']) > 0):
            max_len = indent_large + indent_slight
            str_len = int( max_len * float(satellites[sat]['sig_strength']) / 100.0 )
        if str_len > 0:
            canvas.rectangle( [indent_data,pos-line_spacing+2, indent_data+str_len,pos], outline=0xbb0000, fill=0xbb0000 )

        # Draw info on front. Used satellites get a different colour
        if sat in sats_in_use:
            canvas.text( (0,pos), unicode('Sat ' + sat), 0x00dd00, font)
        else:
            canvas.text( (0,pos), unicode('Sat ' + sat), 0x003000, font)
        text = "e%02d a%03d   sig %02d" % \
            (satellites[sat]['elevation'],satellites[sat]['azimuth'],satellites[sat]['sig_strength'])
        canvas.text( (indent_data,pos), unicode(text), font=font )

    # Display if we have a valid fix or now
    pos = pos + (line_spacing*2)
    if pos < (line_spacing*12):
        if location['valid'] == 0:
            canvas.text( (indent_data,pos), u"no position lock", 0x800000, font)
        else:
            canvas.text( (indent_data,pos), u"valid position", 0x00dd00, font)

def draw_sat_view():
    global satellites

    canvas.clear()
    if not satellites.has_key('in_view'):
        canvas.text( (0,line_spacing), u'No satellites in view', 0x008000, font)
        return

    # Decide where things need to be
    outer_dia = min(screen_width,screen_height)
    outer_rad = int(outer_dia/2)
    inner_dia = int(outer_dia/2.5)
    inner_rad = int(inner_dia/2)
    centre = outer_rad

    # Draw the outer and inner circle
    canvas.ellipse([00,00,outer_dia,outer_dia], outline=0x000000, width=2)
    canvas.ellipse([centre-inner_rad,centre-inner_rad,centre+inner_rad,centre+inner_rad], outline=0x000000, width=1)

    # Draw on N-S, E-W
    canvas.line([centre,00,centre,outer_dia], outline=0x000000, width=1)
    canvas.line([0,centre,outer_dia,centre], outline=0x000000, width=1)
    canvas.text( (centre-3,line_spacing), u'N', font=font )

    # Render each of the satelites
    # Elevation in deg, 0=edge, 90=centre
    # Azimuth in deg, 0=top, round clockwise
    for sat in satellites['in_view']:
        if not satellites.has_key(sat):
            continue
        if not satellites[sat]['elevation']:
            continue
        if not satellites[sat]['azimuth']:
            continue

        # Where to draw the points:
        # Offset so nice and central
        x_pos = centre - 2 
        # Can't write at y=0, so offset everything
        y_pos = centre + int(line_spacing/2)

        elev = float(satellites[sat]['elevation']) / 360.0 * 2 * math.pi
        azim = float(satellites[sat]['azimuth']) / 360.0 * 2 * math.pi

        # azim gives us round the circle
        # elev gives us how far in or out
        #  (elev=0 -> edge, elev=90 -> centre)

        radius = outer_rad * math.cos(elev)

        y_disp = radius * math.cos(azim)
        x_disp = radius * math.sin(azim)

        x_pos = x_pos + x_disp
        y_pos = y_pos - y_disp # 0 is at the top

        canvas.text( (x_pos,y_pos), unicode(sat), 0x008000, font )

def draw_os_data():
    global location

    # We pick up these values as we go
    wgs_height = 0
    wgs_lat = None
    wgs_long = None

    canvas.clear()
    if (not location.has_key('lat')) or (not location.has_key('long')):
        canvas.text( (0,line_spacing), u'No location data available', 0x008000, font)
        return

    yPos = line_spacing
    indent_mid = indent_large-indent_slight
    canvas.text( (0,yPos), u'Location (WGS84)', 0x008000, font)
    if location.has_key('alt'):
        canvas.text( (indent_large,yPos), unicode(location['alt']), font=font )

        # Remove ' M'
        wgs_height = location['alt']
        wgs_height = wgs_height[0:-1]
        if wgs_height[-1:] == '':
            wgs_height = wgs_height[0:-1]
    if location['valid'] == 0:
        canvas.text( (indent_slight,yPos+line_spacing), u'(invalid location)', font=font )
    else:
        canvas.text( (indent_slight,yPos+line_spacing), unicode(location['lat']), font=font )
        canvas.text( (indent_mid,yPos+line_spacing), unicode(location['long']), font=font )

        yPos += line_spacing
        canvas.text( (indent_slight,yPos+line_spacing), unicode(location['lat_dec']), font=font )
        canvas.text( (indent_mid,yPos+line_spacing), unicode(location['long_dec']), font=font )

        # remove N/S E/W
        wgs_ll = get_latlong_floats();
        wgs_lat = wgs_ll[0]
        wgs_long = wgs_ll[1]

    # Convert these values from WGS 84 into OSGB 36
    osgb_data = []
    if (not wgs_lat == None) and (not wgs_long == None):
        osgb_data = turn_wgs84_into_osgb36(wgs_lat,wgs_long,wgs_height)
    # And display
    yPos += (line_spacing*2)
    canvas.text( (0,yPos), u'Location (OSGB 36)', 0x008000, font)
    if osgb_data == []:
        canvas.text( (indent_slight,yPos+line_spacing), u'(invalid location)', font=font )
    else:
        osgb_lat = "%02.06f" % osgb_data[0]
        osgb_long = "%02.06f" % osgb_data[1]
        canvas.text( (indent_slight,yPos+line_spacing), unicode(osgb_lat), font=font )
        canvas.text( (indent_mid,yPos+line_spacing), unicode(osgb_long), font=font )

    # And from OSG36 into easting and northing values
    en = []
    if not osgb_data == []:
        en = turn_osgb36_into_eastingnorthing(osgb_data[0],osgb_data[1])    
    # And display
    yPos += (line_spacing*2)
    canvas.text( (0,yPos), u'OS Easting and Northing', 0x008000, font)
    if en == []:
        canvas.text( (indent_slight,yPos+line_spacing), u'(invalid location)', font=font )
    else:
        canvas.text( (indent_slight,yPos+line_spacing), unicode('E ' + str(int(en[0]))), font=font )
        canvas.text( (indent_mid,yPos+line_spacing), unicode('N ' + str(int(en[1]))), font=font )

    # Now do 6 figure grid ref
    yPos += (line_spacing*2)
    canvas.text( (0,yPos), u'OS 6 Figure Grid Ref', 0x008000, font)
    if en == []:
        canvas.text( (indent_slight,yPos+line_spacing), u'(invalid location)', font=font )
    else:
        six_fig = turn_easting_northing_into_six_fig(en[0],en[1])
        canvas.text( (indent_slight,yPos+line_spacing), unicode(six_fig), font=font )

    # Print the speed in kmph and mph
    yPos += (line_spacing*2)
    canvas.text( (0,yPos), u'Speed', 0x008000, font)
    done_speed = 0
    if motion.has_key('speed_mph'):
        mph_speed = "%0.2f mph" % motion['speed_mph']
        kmph_speed = "%0.2f kmph" % motion['speed_kmph']

        canvas.text( (indent_slight,yPos+line_spacing), unicode(mph_speed), font=font)
        canvas.text( (indent_mid,yPos+line_spacing), unicode(kmph_speed), font=font)
        done_speed = 1
    if done_speed == 0:
        cur_speed = u'(unavailable)'
        canvas.text( (indent_slight,yPos+line_spacing), u'(unavailable)', font=font)

def draw_direction_of():
    global current_waypoint
    global new_waypoints
    global waypoints
    global location
    global motion
    global pref

    yPos = line_spacing
    canvas.clear()
    if (not location.has_key('lat')) or (not location.has_key('long')):
        canvas.text( (0,yPos), u'No location data available', 0x008000, font)
        return
    if (not motion.has_key('true_heading')):
        canvas.text( (0,yPos), u'No movement data available', 0x008000, font)
        return

    # Do we need to refresh the list?
    if current_waypoint == -1:
        load_waypoints()
        current_waypoint = len(waypoints)-1

    # Grab the waypoint of interest
    waypoint = waypoints[current_waypoint]

    # Ensure we're dealing with floats
    direction_of_lat = float(waypoint[1])
    direction_of_long = float(waypoint[2])

    wgs_ll = get_latlong_floats();
    wgs_lat = wgs_ll[0]
    wgs_long = wgs_ll[1]

    # How far is it to where we're going?
    dist_bearing = calculate_distance_and_bearing(wgs_lat,wgs_long,direction_of_lat,direction_of_long)
    if dist_bearing[0] > 100000:
        distance = "%4d km" % (dist_bearing[0]/1000.0)
    else:
        if dist_bearing[0] < 2000:
            distance = "%4d m" % dist_bearing[0]
        else:
            distance = "%3.02f km" % (dist_bearing[0]/1000.0)
    bearing = dist_bearing[1]
    if bearing < 0:
        bearing = bearing + 360
    bearing = "%03d" % bearing
    heading = "%03d" % float(motion['true_heading'])

    # Display
    yPos = line_spacing
    indent_mid = indent_large - indent_slight
    indent_more_mid = indent_large - (2*indent_slight)
    # Cheat - v2 vs v3
    if line_spacing == 12:
        indent_mid = indent_large + indent_slight

    canvas.text( (0,yPos), u'Location (WGS84)', 0x008000, font)
    if location['valid'] == 0:
        canvas.text( (indent_slight,yPos+line_spacing), u'(invalid location)', font=font )
    else:
        canvas.text( (indent_slight,yPos+line_spacing), unicode(location['lat_dec']), font=font )
        canvas.text( (indent_more_mid,yPos+line_spacing), unicode(location['long_dec']), font=font )

    # Where are we going?
    yPos += (line_spacing*2)
    canvas.text( (0,yPos), u'Heading to (WGS84)', 0x008000, font)
    heading_lat = "%02.06f" % direction_of_lat
    heading_long = "%02.06f" % direction_of_long
    canvas.text( (indent_slight,yPos+line_spacing), unicode(heading_lat), font=font )
    canvas.text( (indent_more_mid,yPos+line_spacing), unicode(heading_long), font=font )

    # Draw our big(ish) circle
    #  radius of 3.75 line spacings, centered on
    #   4.25 line spacings, 8 line spacings
    radius = int(line_spacing*3.75)
    centre = ( int(line_spacing*4.25), int(line_spacing*8) )
    canvas.ellipse([centre[0]-radius,centre[1]-radius,centre[0]+radius,centre[1]+radius], outline=0x000000, width=2)
    canvas.point([centre[0],centre[1]], outline=0x000000, width=1)

    def do_line(radius,angle):
        rads = float( angle ) / 360.0 * 2.0 * math.pi
        radius = float(radius)
        t_x = radius * math.sin(rads) + centre[0]
        t_y = -1.0 * radius * math.cos(rads) + centre[1]
        b_x = radius * math.sin(rads + math.pi) + centre[0]
        b_y = -1.0 * radius * math.cos(rads + math.pi) + centre[1]
        return (t_x,t_y,b_x,b_y)


    # What't this waypoint called?
    yPos = line_spacing*5
    canvas.text( (indent_mid,yPos), unicode(waypoint[0]), 0x000080, font )

    # How far, and what dir?
    yPos += line_spacing
    canvas.text( (indent_mid,yPos), u'Distance', 0x008000, font )
    yPos += line_spacing
    canvas.text( (indent_mid,yPos), unicode(distance), font=font )
    yPos += line_spacing
    canvas.text( (indent_mid,yPos), u'Cur Dir', 0x008000, font )
    yPos += line_spacing
    canvas.text( (indent_mid,yPos), unicode(heading), font=font )
    yPos += line_spacing
    canvas.text( (indent_mid,yPos), u'Head In', 0x008000, font )
    yPos += line_spacing
    canvas.text( (indent_mid,yPos), unicode(bearing), 0x800000, font )

    # The way we are going is always straight ahead
    # Draw a line + an arrow head
    top = centre[1] - radius
    asize = int(line_spacing/2)
    canvas.line([centre[0],top,centre[0],centre[1]+radius], outline=0x000000,width=3)
    canvas.line([centre[0],top,centre[0]+asize,top+asize], outline=0x000000,width=3)
    canvas.line([centre[0],top,centre[0]-asize,top+asize], outline=0x000000,width=3)

    # Make sure the true heading is a float
    true_heading = float(motion['true_heading'])

    # Draw NS-EW lines, relative to current direction
    ns_coords = do_line(radius, 0 - true_heading)
    ew_coords = do_line(radius, 0 - true_heading + 90)

    n_pos = do_line(radius+4, 0 - true_heading)
    e_pos = do_line(radius+4, 0 - true_heading + 90)
    s_pos = do_line(radius+4, 0 - true_heading + 180)
    w_pos = do_line(radius+4, 0 - true_heading + 270)

    canvas.line( ns_coords, outline=0x008000, width=2)
    canvas.line( ew_coords, outline=0x008000, width=2)
    canvas.text( (n_pos[0]-2,n_pos[1]+4), u'N', 0x008000, font )
    canvas.text( (s_pos[0]-2,s_pos[1]+4), u'S', 0x008000, font )
    canvas.text( (e_pos[0]-2,e_pos[1]+4), u'E', 0x008000, font )
    canvas.text( (w_pos[0]-2,w_pos[1]+4), u'W', 0x008000, font )

    # Draw on the aim-for line
    # Make it relative to the heading
    bearing_coords = do_line(radius, dist_bearing[1] - true_heading)
    b_a_coords = do_line(radius-asize, dist_bearing[1] - true_heading + 8)
    b_b_coords = do_line(radius-asize, dist_bearing[1] - true_heading - 8)
    b_a = (bearing_coords[0],bearing_coords[1],b_a_coords[0],b_a_coords[1])
    b_b = (bearing_coords[0],bearing_coords[1],b_b_coords[0],b_b_coords[1])

    canvas.line( bearing_coords, outline=0x800000, width=2)
    canvas.line( b_a, outline=0x800000, width=2)
    canvas.line( b_b, outline=0x800000, width=2)

def draw_take_photo():
    global location
    global all_photo_sizes
    global photo_size
    global preview_photo
    global photo_displays
    global taking_photo

    canvas.clear()

    # Do we have pexif?
    if not has_pexif:
        yPos = line_spacing
        canvas.text( (0,yPos), u'pexif not found', 0x800000, font)
        yPos += line_spacing
        canvas.text( (0,yPos), u'Please download and install', 0x800000, font)
        yPos += line_spacing
        canvas.text( (0,yPos), u'so photos can be gps tagged', 0x800000, font)
        yPos += line_spacing
        canvas.text( (0,yPos), u'http://benno.id.au/code/pexif/', 0x008000, font)
        return

    # Display current photo resolution and fix
    yPos = line_spacing
    indent_mid = indent_large - indent_slight

    canvas.text( (0,yPos), u'Location (WGS84)', 0x008000, font)
    if location['valid'] == 0:
        canvas.text( (indent_slight,yPos+line_spacing), u'(invalid location)', font=font )
    else:
        canvas.text( (indent_slight,yPos+line_spacing), unicode(location['lat_dec']), font=font )
        canvas.text( (indent_mid,yPos+line_spacing), unicode(location['long_dec']), font=font )
    yPos += (line_spacing*2)

    canvas.text( (0,yPos), u'Resolution', 0x008000, font)
    canvas.text( (indent_mid,yPos), unicode(photo_size), font=font)
    yPos += line_spacing

    # Display a photo periodically
    if (taking_photo == 0) and (photo_displays > 15 or preview_photo == None):
        taking_photo = 1

    # Only increase the count after the photo's taken
    if (taking_photo == 0):
        photo_displays = photo_displays + 1

    # Only display if we actually have a photo to show
    if not preview_photo == None:
        canvas.blit(preview_photo,target=(5,yPos))


# Handle config entry selections
config_lb = ""
def config_menu():
    # Do nothing for now
    global config_lb
    global canvas
    appuifw.body = canvas

# Select the right draw state
current_state = 'main'
def draw_state():
    """Draw the currently selected screen"""
    global current_state
    if current_state == 'sat_list':
        draw_sat_list()
    elif current_state == 'sat_view':
        draw_sat_view()
    elif current_state == 'os_data':
        draw_os_data()
    elif current_state == 'direction_of':
        draw_direction_of()
    elif current_state == 'take_photo' and has_camera:
        draw_take_photo()
    elif current_state == 'details':
        draw_details()
    else:
        draw_main()

# Menu selectors
def pick_main():
    global current_state
    current_state = 'main'
    draw_state()
def pick_details():
    global current_state
    current_state = 'details'
    draw_state()
def pick_sat_list():
    global current_state
    current_state = 'sat_list'
    draw_state()
def pick_sat_view():
    global current_state
    current_state = 'sat_view'
    draw_state()
def pick_os_data():
    global current_state
    current_state = 'os_data'
    draw_state()
def pick_direction_of():
    global current_state
    current_state = 'direction_of'
    draw_state()
def pick_take_photo():
    global current_state
    current_state = 'take_photo'
    draw_state()
def pick_config():
    """TODO: Make me work!"""
    global config_lb
    config_entries = [ u"GPS", u"Default GPS",
        u"Logging Interval", u"Default Logging" ]
    #config_lb = appuifw.Listbox(config_entries,config_menu)
    #appuifw.body = config_lb
    appuifw.note(u'Configuration menu not yet supported!\nEdit script header to configure',"info")
def pick_upload():
    """TODO: Implement me!"""
    appuifw.note(u'Please use upload_track.py\nSee http://gagravarr.org/code/', "info")
def pick_new_file():
    do_pick_new_file(u"_nmea.txt")
def do_pick_new_file(def_name):
    global pref

    # Get new filename
    new_name = appuifw.query(u"Name for new file?", "text", def_name)

    if len(new_name) > 0:
        # Check it doesn't exist
        new_file_name = pref['base_dir'] + new_name
        if os.path.exists(new_file_name):
            appuifw.note(u"That file already exists", "error")
            pick_new_file(new_name)
    
        # Rename
        rename_current_gga_log(new_file_name)
        appuifw.note(u"Now logging to new file")

def do_add_as_waypoint():
    """Prompt for a name, then add a waypoint for the current location"""
    global location

    name = appuifw.query(u'Waypoint name?', 'text')
    if name:
        wgs_ll = get_latlong_floats();
        lat = wgs_ll[0]
        long = wgs_ll[1]

        add_waypoint(name, lat, long)
        appuifw.note(u'Waypoint Added','info')

#############################################################################

class GPS(object):
    connected = False
    def connect(self):
        """Try connect to the GPS, and return if it worked or not"""
        return False
    def identify_gps(self):
        """Figure out what GPS to use, and do any setup for that"""
        pass
    def process(self):
        """Process any pending GPS info, and return if a screen update is needed or not"""
        return 0
    def shutdown(self):
        """Do any work required for the shutdown"""
        pass

class BT(GPS):
    def __init__(self):
        self.gps_addr = None
        self.target = None
        self.sock = None
    def __repr__(self):
        return self.gps_addr
    def identify_gps(self):
        """Decide which Bluetooth GPS to connect to"""
        if not pref['def_gps_addr'] == '':
            self.gps_addr = pref['def_gps_addr']
            self.target=(self.gps_addr,1)
            # Alert them to the GPS we're going to connect
            #  to automatically
            appuifw.note(u"Will connect to GPS %s" % self.gps_addr, 'info')
        else:
            # Prompt them to select a bluetooth GPS
            self.gps_addr,services=socket.bt_discover()
            self.target=(self.gps_addr,services.values()[0])
    def shutdown(self):
        self.sock.close()
        self.connected = False
    def connect(self):
        try:
            # Connect to the bluetooth GPS using the serial service
            self.sock = socket.socket(socket.AF_BT, socket.SOCK_STREAM)
            self.sock.connect(self.target)
            self.connected = True
            debug_log("CONNECTED to GPS: target=%s at %s" % (str(self.target), time.strftime('%H:%M:%S', time.localtime(time.time()))))
            disp_notices = "Connected to GPS."
            appuifw.note(u"Connected to the GPS")
            return True
        except socket.error, inst:
            self.connected = False
            disp_notices = "Connect to GPS failed.  Retrying..."
            #appuifw.note(u"Could not connected to the GPS. Retrying in 5 seconds...")
            #e32.ao_sleep(5)
            return False
    def process(self):
        try:
            rawdata = readline(self.sock)
        except socket.error, inst:
            # GPS has disconnected, bummer
            self.connected = False
            debug_log("DISCONNECTED from GPS: socket.error %s at %s" % (str(inst), time.strftime('%H:%M:%S, %Y-%m-%d', time.localtime(time.time()))))
            location = {}
            location['valid'] = 1
            appuifw.note(u"Disconnected from the GPS. Retrying...")
            return 0

        # Try to process the data from the GPS
        # If it's gibberish, skip that line and move on
        # (Not all bluetooth GPSs are created equal....)
        try:
            data = rawdata.strip()

            # Discard fragmentary sentences -  start with the last '$'
            startsign = rawdata.rfind('$')
            data = data[startsign:]

            # Ensure it starts with $GP
            if not data[0:3] == '$GP':
                return 0

            # If it has a checksum, ensure that's correct
            # (Checksum follows *, and is XOR of everything from
            #  the $ to the *, exclusive)
            if data[-3] == '*':
                exp_checksum = generate_checksum(data[1:-3])
                if not exp_checksum == data[-2:]:
                    disp_notices = "Invalid checksum %s, expecting %s" % (data[-2:], exp_checksum)
                    return 0
                
                # Strip the checksum
                data = data[:-3]

            # Grab the parts of the sentence
            talker = data[1:3]
            sentence_id = data[3:6]
            sentence_data = data[7:]

            # Do we need to re-draw the screen?
            redraw = 0

            # The NMEA location sentences we're interested in are:
            #  GGA - Global Positioning System Fix Data
            #  GLL - Geographic Position
            #  RMC - GPS Transit Data
            if sentence_id == 'GGA':
                do_gga_location(sentence_data)
                redraw = 1

                # Log GGA packets periodically
                gga_log(rawdata)
            if sentence_id == 'GLL':
                do_gll_location(sentence_data)
                redraw = 1
            if sentence_id == 'RMC':
                do_rmc_location(sentence_data)
                redraw = 1

            # The NMEA satellite sentences we're interested in are:
            #  GSV - Satellites in view
            #  GSA - Satellites used for positioning
            if sentence_id == 'GSV':
                do_gsv_satellite_view(sentence_data)
                redraw = 1
            if sentence_id == 'GSA':
                do_gsa_satellites_used(sentence_data)
                redraw = 1

            # The NMEA motion sentences we're interested in are:
            #  VTG - Track made good
            # (RMC - GPS Transit - only in knots)
            if sentence_id == 'VTG':
                do_vtg_motion(sentence_data)
                redraw = 1

        # Catch exceptions cased by the GPS sending us crud
        except (RuntimeError, TypeError, NameError, ValueError, ArithmeticError, LookupError, AttributeError), inst:
            print "Exception: %s" % str(inst)
            debug_log("EXCEPTION: %s" % str(inst))
            return 0

        return redraw

class LocReq(GPS):
    "LocationRequestor powered GPS Functionality"
    def __init__(self):
        self.lr = locationrequestor.LocationRequestor()
        self.id = None
        self.default_id = self.lr.GetDefaultModuleId()
        self.type = "Unknown"
    def __repr__(self):
        return self.type
    def identify_gps(self):
        # Try for the internal first
        count = self.lr.GetNumModules()
        for i in range(count):
            info = self.lr.GetModuleInfoByIndex(i)
            if ((info[3] == locationrequestor.EDeviceInternal) and ((info[2] & locationrequestor.ETechnologyNetwork) == 0)):
                try:
                    self.id = info[0]
                    self.type = "Internal"
                    self.lr.Open(self.id)
                    print "Picked Internal GPS with ID %s" % self.id
                    self.lr.Close
                    return
                except Exception, reason:
                    # This probably means that the GPS is disabled
                    print "Error querying GPS %d - %s" % (i,reason)

        # Look for external if there's no internal one
        for i in range(count):
            info = self.lr.GetModuleInfoByIndex(i)
            if ((info[3] == locationrequestor.EDeviceExternal) and ((info[2] & locationrequestor.ETechnologyNetwork) == 0)):
                self.id = info[0]
                self.type = "External"
                print "Picked External GPS with ID %s" % self.id
                return
        # Go with the default
        self.id = self.default_id
    def connect(self):
        self.lr.SetUpdateOptions(1, 45, 0, 1)
        self.lr.Open(self.id)

        # Install the callback
        try:
            self.lr.InstallPositionCallback(process_lr_update)
            self.connected = True
            appuifw.note(u"Connected to the GPS Location Service")
            return True
        except Exception, reason:
            disp_notices = "Connect to GPS failed with %s, retrying" % reason
            self.connected = False
            return False
    def process(self):
        e32.ao_sleep(0.4)
        return 1
    def shutdown(self):
        self.lr.Close()

class PythonPositioning(GPS):
    "S60 Python Positioning module powered GPS Functionality"
    def __init__(self):
        self.id = None
        self.default_id = positioning.default_module()
        self.type = "Unknown"
    def __repr__(self):
        return self.type
    def identify_gps(self):
        modules = positioning.modules()

        # Try for assisted, then internal, then BT, then default
        #for wanted in ("Assisted", "Integrated", "Bluetooth"):
        for wanted in ("Integrated", "Bluetooth"):
            for module in modules:
                if self.id: 
                    continue
                if not module['available']:
                    continue
                if module['name'].startswith(wanted):
                    self.id = module['id']

                    name = module['name']
                    if name.endswith(" GPS"):
                        name = name[0:-4]
                    self.type = name
                    print "Picked %s with ID %d" % (module['name'], self.id)
        # Go with the default if all else fails
        if not self.id:
            self.id = self.default_id
    def connect(self):
        # Connect, and install the callback
        try:
            print "Activating module with id '%d'" % self.id
            positioning.select_module(self.id)
            positioning.set_requestors([{"type":"service",
                                    "format":"application",
                                    "data":"nmea_info"}])
            
            positioning.position(
                                course=1,
                                satellites=1,
                                callback=process_positioning_update,
                                interval=1000,
                                partial=1
            )
            self.connected = True
            appuifw.note(u"Connected to the GPS Location Service")
            return True
        except Exception, reason:
            disp_notices = "Connect to GPS failed with %s, retrying" % reason
            self.connected = False
            return False
    def process(self):
        e32.ao_sleep(0.4)
        return 1
    def shutdown(self):
        positioning.stop_position()

if has_locationrequestor and not pref['force_bluetooth']:
    gps = LocReq()
elif has_positioning and not pref['force_bluetooth']:
    gps = PythonPositioning()
else:
    gps = BT()
gps.identify_gps()

# Not yet connected
gps.connected = False

#############################################################################

# Enable these displays, now all prompts are over
canvas=appuifw.Canvas(event_callback=callback,
        redraw_callback=lambda rect:draw_state())

# Figure out how big our screen is
# Note - don't use sysinfo.display_pixels(), as we have phone furnature
screen_width,screen_height = canvas.size
if screen_width > (screen_height*1.25):
    appuifw.note(u"This application is optimised for portrait view, some things may look odd", "info")
print "Detected a resolution of %d by %d" % (screen_width,screen_height)

# Decide on font and line spacings. We want ~12 lines/page
line_spacing = int(screen_height/12)
all_fonts = appuifw.available_fonts()
font = None
if u'LatinBold%d' % line_spacing in all_fonts:
    font = u'LatinBold%d' % line_spacing
elif u'LatinPlain%d' % line_spacing in all_fonts:
    font = u'LatinPlain%d' % line_spacing
#elif u'Nokia Sans S60' in all_fonts:
#   font = u'Nokia Sans S60'
else:
    # Look for one with the right spacing, or close to it
    for f in all_fonts:
        if f.endswith(str(line_spacing)):
            font = f
    if font == None:
        for f in all_fonts:
            if f.endswith(str(line_spacing-1)):
                font = f
    if font == None:
        # Give up and go with the default
        font = "normal"
print "Selected line spacing of %d and the font %s" % (line_spacing,font)
# while converting
#font = u"LatinPlain12"

# How far across the screen to draw things in the list views
indent_slight = int(line_spacing * 0.85)   #v2=10
indent_data = int(line_spacing * 4.0) + 12 #v2=60
indent_large = int(line_spacing * 8.75)    #v2=105

# Get the sizes for photos
all_photo_sizes = []
preview_size = None
if has_camera:
    all_photo_sizes = camera.image_sizes()
    all_photo_sizes.sort(lambda x,y: cmp(x[0],y[0]))
    photo_size = all_photo_sizes[0]
    for size in all_photo_sizes:
        if size[0] < screen_width and size[1] < screen_height:
            preview_size = size
    if preview_size == None:
        preview_size = all_photo_sizes[0]
    print "Selected a preview photo size of %d by %d" % (preview_size[0],preview_size[1])

# Make the canvas active
appuifw.app.body=canvas

# TODO: Make canvas and Listbox co-exist without crashing python
appuifw.app.menu=[
    (u'Main Screen',pick_main),
    (u'Details Screen',pick_details), (u'Satellite List',pick_sat_list), 
    (u'Satellite View',pick_sat_view), (u'OS Data',pick_os_data),
    (u'Direction Of',pick_direction_of), (u'Take Photo',pick_take_photo),
    (u'Upload',pick_upload), (u'Configuration',pick_config), 
    (u'New Log File', pick_new_file)]

#############################################################################

# Start the lock, so python won't exit during non canvas graphical stuff
lock = e32.Ao_lock()

# Loop while active
appuifw.app.exit_key_handler = exit_key_pressed
while going > 0:
    # Connect to the GPS, if we're not already connected
    if not gps.connected:
        worked = gps.connect()
        if not worked:
            # Sleep for a tiny bit, then retry
            e32.ao_sleep(0.2)
            continue

    # Take a preview photo, if they asked for one
    # (Need to do it in this thread, otherwise it screws up the display)
    if taking_photo == 1:
        new_photo = camera.take_photo(
                            mode='RGB12', size=preview_size,
                            flash='none', zoom=0, exposure='auto', 
                            white_balance='auto', position=0 )
        preview_photo = new_photo
        if taking_photo == 1:
            # In case they requested a photo take while doing the preview
            taking_photo = 0
        photo_displays = 0
    # Take a real photo, and geo-tag it
    # (Need to do it in this thread, otherwise it screws up the display)
    if taking_photo == 2:
        new_photo = camera.take_photo(
                                mode='RGB16', size=photo_size,
                                flash='none', zoom=0, exposure='auto', 
                                white_balance='auto', position=0 )
        # Write out
        filename = "E:\\Images\\GPS-%d.jpg" % int(time.time())
        new_photo.save(filename, format='JPEG',
                            quality=75, bpp=24, compression='best')
        # Grab the lat and long, and trim to 4dp
        # (Otherwise we'll cause a float overflow in pexif)
        wgs_ll = get_latlong_floats();
        print "Tagging as %s %s" % (wgs_ll[0],wgs_ll[1])
        # Geo Tag it
        geo_photo = JpegFile.fromFile(filename)
        geo_photo.set_geo( wgs_ll[0], wgs_ll[1] )
        geo_photo.writeFile(filename)
        # Done
        appuifw.note(u"Taken photo", 'info')
        taking_photo = 0

    # If we are connected to the GPS, read a line from it
    if gps.connected:
        redraw = gps.process()

        # Update the state display if required
        if redraw == 1:
            draw_state()
    else:
        # Sleep for a tiny bit before re-trying
        e32.ao_sleep(0.2)

else:
    # All done
    gps.shutdown()
    close_gga_log()
    close_debug_log()
    close_stumblestore_gsm_log()

print "All done"
#appuifw.app.set_exit()
