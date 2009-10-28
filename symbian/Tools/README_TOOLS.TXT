opensignsis.py:
	Handy script for signing sis files on https://www.symbiansigned.com/app/page/public/openSignedOnline.do
	from command line (you still need to solve capcha each time).
	You should edit opensignsis.config to set up details of your email accound and
	IMEI of your phone.
	The original version and more details are available here:
		http://blog.wahlig.eu/2008/08/opensignsis-error-detection.html
	
nmea_info_mod.py:
	The S60 python script which can be used to record log of the trip to be feed
	to Symbian location emulator (PSY) to allow testing and of location based features.
	The script saves a log in a format acceptable by Symbian GPS emulator
	(In Symbian documentation look for:
	S60 3rd Edition SDK for Symbian OS, Supporting Feature Pack 1, for C++ >
		Tools and Utilities >
			Tools Collection for Location-Based Application Development > 
				Simulation PSY User's Guide  for more details)
	To run this script you need to install S60 python and locationrequester library,
	signed for your phone.
	The original version and more details are available here: http://gagravarr.org/code/
