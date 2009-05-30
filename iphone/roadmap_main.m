/* roadmap_main.c - The main function of the RoadMap application.
 *
 * LICENSE:
 *
 *   Copyright 2002 Pascal F. Martin
 *   Copyright 2008 Morten Bek Ditlevsen
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
 *   int main (int argc, char **argv);
 */

#include <stdlib.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#include "../roadmap.h"
#include "../roadmap_path.h"
#include "../roadmap_file.h"
#include "../roadmap_start.h"
#include "../roadmap_device_events.h"
#include "../roadmap_config.h"
#include "../roadmap_history.h"
#include "roadmap_iphonecanvas.h"
#include "roadmap_iphonemain.h"

#include "../roadmap_main.h"
#include "../roadmap_time.h"

#include "../roadmap_lang.h"
#include "../roadmap_download.h"
#include "../editor/editor_main.h"

#include "../ssd/ssd_dialog.h"
#include "../ssd/ssd_text.h"
#include "../ssd/ssd_widget.h"
#include "../ssd/ssd_container.h"
#include "../Realtime/RealtimeAlerts.h"

#include <UIKit/UIDevice.h>
#include <UIKit/UIApplication.h>
#include <UIKit/UIScreen.h>
#include <CoreFoundation/CFNumber.h>
#include <CoreFoundation/CFString.h>
#include <CFNetwork/CFProxySupport.h>



//////
// The below should be removed once proper header file is used
enum {
UIViewAutoresizingNone = 0, // is 0
UIViewAutoresizingFlexibleLeftMargin = 1 << 0, // is 1
UIViewAutoresizingFlexibleWidth = 1 << 1, // is 2
UIViewAutoresizingFlexibleRightMargin = 1 << 2, // is 4
UIViewAutoresizingFlexibleTopMargin = 1 << 3, // is 8
UIViewAutoresizingFlexibleHeight = 1 << 4,  // is 16
UIViewAutoresizingFlexibleBottomMargin = 1 << 5  //is 32
};
typedef NSUInteger UIViewAutoresizing;
//////////////




int USING_PHONE_KEYPAD = 0;

static RoadMapApp *TheApp;
static int sArgc;
static char ** sArgv;


struct roadmap_main_io {
   NSFileHandle *fh;
   CFRunLoopSourceRef rl;
   RoadMapIO io;
   RoadMapInput callback;
};

#define ROADMAP_MAX_IO 16
static struct roadmap_main_io RoadMapMainIo[ROADMAP_MAX_IO];

struct roadmap_main_timer {
   NSTimer *timer;
   RoadMapCallback callback;
};

#define ROADMAP_MAX_TIMER 16
static struct roadmap_main_timer RoadMapMainPeriodicTimer[ROADMAP_MAX_TIMER];

static RoadMapCallback idle_callback = NULL;
static char *RoadMapMainTitle = NULL;

static RoadMapKeyInput     RoadMapMainInput           = NULL;
static UIWindow            *RoadMapMainWindow         = NULL;
static RoadMapViewController    *RoadMapMainViewController = NULL;
static UIView              *RoadMapMainBox            = NULL;
static RoadMapCanvasView   *RoadMapCanvasBox          = NULL;
static UIView              *RoadMapMainMenuBar        = NULL;
static UIView              *RoadMapMainToolbar        = NULL;
static UIView              *RoadMapMainStatus         = NULL;

static int                 IsGoingToSuspend           = 0; //will become true if suspending not from home button (phone call etc)
static int                 IsKeepInBackground         = 0;
static int                 RoadMapMainPlatform        = ROADMAP_MAIN_PLATFORM_NA;
static char                RoadMapMainProxy[256];


static int iPhoneIconsInitialized = 0;

static int RoadMapMainSavedSkinMode = 0;


static UIView *roadmap_main_toolbar_icon (const char *icon) {

   if (icon != NULL) {

      const char *icon_file = roadmap_path_search_icon (icon);

      if (icon_file != NULL) {
         iPhoneIconsInitialized = 1;
         return NULL; // gtk_image_new_from_file (icon_file);
      }
   }
   return NULL;
}

/*
static void roadmap_main_close (UIView *widget,
                                GdkEvent *event,
                                gpointer data) {

   roadmap_main_exit ();
}


static void roadmap_main_activate (UIView *widget, gpointer data) {

   if (data != NULL) {
      (* (RoadMapCallback) data) ();
   }
}


static gint roadmap_main_key_pressed (UIView *w, GdkEventKey *event) {

   char *key = NULL;
   char regular_key[2];


   switch (event->keyval) {

      case GDK_Left:   key = "Button-Left";           break;
      case GDK_Right:  key = "Button-Right";          break;
      case GDK_Up:     key = "Button-Up";             break;
      case GDK_Down:   key = "Button-Down";           break;

      case GDK_Return: key = "Enter";                 break;

      // These binding are for the iPAQ buttons: 
      case 0x1008ff1a: key = "Button-Menu";           break;
      case 0x1008ff20: key = "Button-Calendar";       break;
      case 0xaf9:      key = "Button-Contact";        break;
      case 0xff67:     key = "Button-Start";          break;

      // Regular keyboard keys: 
      default:

         if (event->keyval > 0 && event->keyval < 128) {

            regular_key[0] = event->keyval;
            regular_key[1] = 0;
            key = regular_key;
         }
         break;
   }

   if (key != NULL && RoadMapMainInput != NULL) {
      (*RoadMapMainInput) (key);
   }

   return FALSE;
}


*/

void roadmap_main_toggle_full_screen (void) {
}

void roadmap_main_new (const char *title, int width, int height) {
    [TheApp newWithTitle: title andWidth: width andHeight: height];
	
      editor_main_set (1);
}


void roadmap_main_title(char *fmt, ...) {

   char newtitle[200];
   va_list ap;
   int n;

   n = snprintf(newtitle, 200, "%s", RoadMapMainTitle);
   va_start(ap, fmt);
   vsnprintf(&newtitle[n], 200 - n, fmt, ap);
   va_end(ap);

 //  gtk_window_set_title (GTK_WINDOW(RoadMapMainWindow), newtitle);
}

void roadmap_main_set_keyboard
  (struct RoadMapFactoryKeyMap *bindings, RoadMapKeyInput callback) {
   RoadMapMainInput = callback;
}

RoadMapMenu roadmap_main_new_menu (void) {

   return NULL;
}


void roadmap_main_free_menu (RoadMapMenu menu) {
     //NSLog (@"roadmap_main_free_menu\n");

   
}


void roadmap_main_add_menu (RoadMapMenu menu, const char *label) {
     //NSLog (@"roadmap_main_add_menu label: %s\n", label);
/*
   UIView *menu_item;

   if (RoadMapMainMenuBar == NULL) {

      RoadMapMainMenuBar = gtk_menu_bar_new();

      gtk_box_pack_start
         (GTK_BOX(RoadMapMainBox), RoadMapMainMenuBar, FALSE, TRUE, 0);
   }

   menu_item = gtk_menu_item_new_with_label (label);
   gtk_menu_shell_append (GTK_MENU_SHELL(RoadMapMainMenuBar), menu_item);

   gtk_menu_item_set_submenu (GTK_MENU_ITEM(menu_item), (UIView *) menu);
*/
}


void roadmap_main_popup_menu (RoadMapMenu menu, int x, int y) {
     NSLog (@"roadmap_main_popup_menu\n");

 /*  if (menu != NULL) {
      gtk_menu_popup (GTK_MENU(menu),
                      NULL,
                      NULL,
                      NULL,
                      NULL,
                      0,
                      gtk_get_current_event_time());
   }
   */
}


void roadmap_main_add_menu_item (RoadMapMenu menu,
                                 const char *label,
                                 const char *tip,
                                 RoadMapCallback callback) {

     //NSLog (@"roadmap_main_add_menu_item label: %s tip: %s\n", label, tip);
/*
   UIView *menu_item;

   if (label != NULL) {

      menu_item = gtk_menu_item_new_with_label (label);
      g_signal_connect (menu_item, "activate",
                        (GCallback)roadmap_main_activate,
                        callback);
   } else {
      menu_item = gtk_menu_item_new ();
   }
   gtk_menu_shell_append (GTK_MENU_SHELL(menu), menu_item);
   gtk_widget_show(menu_item);

   if (tip != NULL) {
      gtk_tooltips_set_tip (gtk_tooltips_new (), menu_item, tip, NULL);
   }
   */
}


void roadmap_main_add_separator (RoadMapMenu menu) {
   //NSLog (@"roadmap_main_add_seperator\n");

   roadmap_main_add_menu_item (menu, NULL, NULL, NULL);
}


void roadmap_main_add_toolbar (const char *orientation) {
     //NSLog (@"roadmap_main_add_toolbar orientation: %s\n", orientation);

    if (RoadMapMainToolbar == NULL) {
        int on_top = 1;

        //   GtkOrientation gtk_orientation = GTK_ORIENTATION_HORIZONTAL;

        // RoadMapMainToolbar = gtk_toolbar_new ();

        switch (orientation[0]) {
            case 't':
            case 'T': on_top = 1; break;

            case 'b':
            case 'B': on_top = 0; break;

            case 'l':
            case 'L': on_top = 1;
                      //            gtk_orientation = GTK_ORIENTATION_VERTICAL;
                      break;

            case 'r':
            case 'R': on_top = 0;
                      //           gtk_orientation = GTK_ORIENTATION_VERTICAL;
                      break;

            default: /*roadmap_log (ROADMAP_FATAL,
                             "Invalid toolbar orientation %s", orientation);
                             */
                             break;
        }
        /*      gtk_toolbar_set_orientation (GTK_TOOLBAR(RoadMapMainToolbar),
                gtk_orientation);

                if (gtk_orientation == GTK_ORIENTATION_VERTICAL) {
         */
        /* In this case we need a new box, since the "normal" box
         * is a vertical one and we need an horizontal one. */

        /*         RoadMapCanvasBox = gtk_hbox_new (FALSE, 0);
                   gtk_container_add (GTK_CONTAINER(RoadMapMainBox), RoadMapCanvasBox);
         */
        // }

        if (on_top) {
            // gtk_box_pack_start (GTK_BOX(RoadMapCanvasBox),
            //                     RoadMapMainToolbar, FALSE, FALSE, 0);
        } else {
            // gtk_box_pack_end   (GTK_BOX(RoadMapCanvasBox),
            //                     RoadMapMainToolbar, FALSE, FALSE, 0);
        }
    }
}

void roadmap_main_add_tool (const char *label,
                            const char *icon,
                            const char *tip,
                            RoadMapCallback callback) {
   //NSLog (@"roadmap_main_add_tool label: %s icon: %s tip: %s\n", label, icon, tip);
   if (RoadMapMainToolbar == NULL) {
      roadmap_main_add_toolbar ("");
   }

/*
   gtk_toolbar_append_item (GTK_TOOLBAR(RoadMapMainToolbar),
                            label, tip, NULL,
                            roadmap_main_toolbar_icon (icon),
                            (GtkSignalFunc) roadmap_main_activate, callback);

   if (gdk_screen_height() < 550)
   {
*/
      /* When using a small screen, we want either the labels or the icons,
       * but not both (small screens are typical with PDAs). */
       
/*      gtk_toolbar_set_style
         (GTK_TOOLBAR(RoadMapMainToolbar),
          GtkIconsInitialized?GTK_TOOLBAR_ICONS:GTK_TOOLBAR_TEXT);
   }
   */
}


void roadmap_main_add_tool_space (void) {
   //NSLog (@"roadmap_main_add_tool_space\n");
   if (RoadMapMainToolbar == NULL) {
   //   roadmap_log (ROADMAP_FATAL, "Invalid toolbar space: no toolbar yet");
   }

/*
   gtk_toolbar_append_space (GTK_TOOLBAR(RoadMapMainToolbar));
   */
}

static unsigned long roadmap_main_busy_start;

void roadmap_main_set_cursor (int cursor) {
   NSLog (@"roadmap_main_set_cursor\n");
   //TODO: This dialog does not work
   
   switch (cursor) {

      case ROADMAP_CURSOR_NORMAL:
         ssd_dialog_hide ("Main Processing", 0);
         break;

      case ROADMAP_CURSOR_WAIT: ;
            SsdWidget dialog = ssd_dialog_activate ("Main Processing", NULL);
            if (!dialog) {
               SsdWidget group;

               dialog = ssd_dialog_new ("Main Processing", "", NULL,
                     SSD_CONTAINER_BORDER|SSD_CONTAINER_TITLE|SSD_DIALOG_FLOAT|
                     SSD_ALIGN_CENTER|SSD_ALIGN_VCENTER);

               group = ssd_container_new ("Status group", NULL,
                           SSD_MIN_SIZE, SSD_MIN_SIZE, SSD_WIDGET_SPACE|SSD_END_ROW);
               ssd_widget_set_color (group, NULL, NULL);
               ssd_widget_add (group,
                  ssd_text_new ("Label", roadmap_lang_get("Please wait..."), -1,
                                 SSD_TEXT_LABEL));
               ssd_widget_add (dialog, group);
               
               ssd_dialog_draw ();
            }
         break;
   }
}

void roadmap_main_busy_check(void) {

   if (roadmap_main_busy_start == 0)
      return;

   if (roadmap_time_get_millis() - roadmap_main_busy_start > 1000) {
      roadmap_main_set_cursor (ROADMAP_CURSOR_WAIT);
   }
}

void roadmap_main_add_canvas (void) {
//    NSLog ("roadmap_main_add_canvas\n");
    //struct CGRect rect = [UIHardware fullScreenApplicationContentRect];
    struct CGRect rect = [RoadMapMainBox frame];
	rect.origin.x = rect.origin.y = 0.0f;

    RoadMapCanvasBox = [[RoadMapCanvasView alloc] initWithFrame: rect];
	[RoadMapCanvasBox setAutoresizesSubviews: YES];
	[RoadMapCanvasBox setAutoresizingMask: UIViewAutoresizingFlexibleHeight | UIViewAutoresizingFlexibleWidth];
	//[RoadMapCanvasBox setAutoresizingMask: UIViewAutoresizingFlexibleRightMargin  | UIViewAutoresizingFlexibleBottomMargin |
							//UIViewAutoresizingFlexibleLeftMargin  | UIViewAutoresizingFlexibleTopMargin];
    [RoadMapMainBox addSubview: RoadMapCanvasBox];
}

void roadmap_main_add_status (void) {
   //NSLog (@"roadmap_main_add_status\n");
}


void roadmap_main_show (void) {
   //NSLog (@"roadmap_main_show\n");

   if (RoadMapMainWindow != NULL) {
   }
}


void roadmap_main_set_input (RoadMapIO *io, RoadMapInput callback) {
    [TheApp setInput: io andCallback: callback];
}

static void outputCallback (
   CFSocketRef s,
   CFSocketCallBackType callbackType,
   CFDataRef address,
   const void *data,
   void *info
)
{
   int i;
   int fd = CFSocketGetNative(s);

   for (i = 0; i < ROADMAP_MAX_IO; ++i) {
      if (RoadMapMainIo[i].io.os.file == fd) {
         (* RoadMapMainIo[i].callback) (&RoadMapMainIo[i].io);
         break;
      }
   }
}

static void internal_set_output (CFSocketRef s, RoadMapIO *io, RoadMapInput callback) {
    int i;
    int fd = io->os.file; /* All the same on UNIX. */

    for (i = 0; i < ROADMAP_MAX_IO; ++i) {
        if (RoadMapMainIo[i].io.subsystem == ROADMAP_IO_INVALID) {
            RoadMapMainIo[i].io = *io;
            RoadMapMainIo[i].callback = callback;
            RoadMapMainIo[i].fh = NULL;
            RoadMapMainIo[i].rl = CFSocketCreateRunLoopSource (NULL, s, 0);

            CFRunLoopAddSource (CFRunLoopGetCurrent( ), RoadMapMainIo[i].rl,
                                kCFRunLoopCommonModes);
            break;
        }
    }
}

int roadmap_main_async_connect(RoadMapIO *io, struct sockaddr *addr, RoadMapInput callback) {

   CFSocketRef s = CFSocketCreateWithNative(NULL, io->os.socket, kCFSocketConnectCallBack, outputCallback, NULL);

   CFSocketSetSocketFlags (s, kCFSocketAutomaticallyReenableReadCallBack|
                              kCFSocketAutomaticallyReenableAcceptCallBack|
                              kCFSocketAutomaticallyReenableDataCallBack);

   CFDataRef address = CFDataCreate(NULL, (const UInt8 *)addr, sizeof(*addr));
   
   internal_set_output(s, io, callback);
   
   int res =  CFSocketConnectToAddress (s, address, -1);
   
   CFRelease (address);

   if (res < 0) {
      roadmap_main_remove_input (io);
      return -1;
   }
   
   return res;
}

void roadmap_main_remove_input (RoadMapIO *io) {
    [TheApp removeIO: io];
}

void roadmap_main_set_periodic (int interval, RoadMapCallback callback) {
    [TheApp setPeriodic: (interval*0.001) andCallback: callback];
}


void roadmap_main_remove_periodic (RoadMapCallback callback) {
   [TheApp removePeriodic: callback];
}


void roadmap_main_set_status (const char *text) {
//   NSLog (@"roadmap_main_set_status text: %s\n", text);
}

void roadmap_main_set_idle_function (RoadMapCallback callback) {
    idle_callback = callback;
    [TheApp setPeriodic: 0.00 andCallback: callback];
}

void roadmap_main_remove_idle_function (void) {
   [TheApp removePeriodic: idle_callback];
   idle_callback = NULL;
}


void roadmap_main_flush (void) {
   
   //NSLog (@"roadmap_main_flush\n");
   //[[NSRunLoop currentRunLoop] run];
   
   double resolution = 0.0001;

   NSDate* next = [NSDate dateWithTimeIntervalSinceNow:resolution];

   [[NSRunLoop currentRunLoop] runMode:NSDefaultRunLoopMode
                              beforeDate:next];

/*
   while (gtk_events_pending ()) {
      if (gtk_main_iteration ()) {
         exit(0);  // gtk_main_quit() called 
      }
   }
   */
}


int roadmap_main_flush_synchronous (int deadline) {
   NSLog (@"roadmap_main_flush_synchronous\n");

   long start_time, duration;

   start_time = roadmap_time_get_millis();

  /* while (gtk_events_pending ()) {
      if (gtk_main_iteration ()) {
         exit(0); 
      }
   }
   */
//   gdk_flush();

   duration = roadmap_time_get_millis() - start_time;

   if (duration > deadline) {

      roadmap_log (ROADMAP_DEBUG, "processing flush took %d", duration);

      return 0; /* Busy. */
   }

   return 1; /* Not so busy. */
}

void roadmap_main_exit (void) {
   [TheApp terminate];
}

static void roadmap_start_event (int event) {
   switch (event) {
   case ROADMAP_START_INIT:
      editor_main_check_map (); //TODO: enable this
      break;
   }
}

int roadmap_main_should_mute () {
   int shouldMute =  IsGoingToSuspend || 
                     ![UIHardware ringerState]; 
   return (shouldMute);
}

static void roadmap_suspend_timeout () {
   if (IsGoingToSuspend != 0) {
      IsGoingToSuspend = 0;
      roadmap_main_remove_periodic(roadmap_suspend_timeout);
   }
   roadmap_main_exit ();   
}

int roadmap_main_get_platform () {
   return (RoadMapMainPlatform);
}

void set_statusbar_icon () {
   if (IsKeepInBackground == 0 && IsGoingToSuspend == 0) {
      [TheApp removeStatusBarImageNamed:@"waze"];
   } else {
      if ([TheApp respondsToSelector:@selector(addStatusBarImageNamed:removeOnExit:)]) 
         [TheApp addStatusBarImageNamed:@"waze" removeOnExit:YES]; 
      else 
         [TheApp addStatusBarImageNamed:@"waze" removeOnAbnormalExit:YES];
   }
}

void roadmap_main_minimize (void){
   IsKeepInBackground = 1;
   
   set_statusbar_icon();

   int numAlerts = RTAlerts_Count();
   [TheApp setApplicationBadge:[NSString stringWithFormat:@"%d",numAlerts]];

   //[RoadMapMainBox setHidden: YES];
   [[UIApplication sharedApplication] setIdleTimerDisabled: NO];
   [RoadMapMainWindow setHidden: YES];
   [TheApp _sendApplicationSuspend:NULL];
}   
/////////////////
// The following should be included in the UIApplication header file. It is added due to incompatible headers.
typedef enum {
    UIStatusBarStyleDefault,
    UIStatusBarStyleBlackTranslucent,
    UIStatusBarStyleBlackOpaque
} UIStatusBarStyle;
/////////////////

void roadmap_main_adjust_skin (int state) {
   //Set status bar style
   if (state == 0)
      [[UIApplication sharedApplication] setStatusBarStyle:UIStatusBarStyleDefault];
   else
      [[UIApplication sharedApplication] setStatusBarStyle:UIStatusBarStyleBlackTranslucent];
      
   RoadMapMainSavedSkinMode = state;
}

int roadmap_main_is_in_background () {
 //keep navigation state before exit?
 return (IsKeepInBackground || IsGoingToSuspend);
                               //If we close the app while in background,
                               // it is due to low memory
                               // => keep the navigation for next launch
}

void GetProxy (const char* url) {

   strcpy (RoadMapMainProxy, "");

   CFStringRef URLString = CFStringCreateWithCString (NULL, url, kCFStringEncodingUTF8);
   CFURLRef    CFurl = CFURLCreateWithString(NULL, URLString, NULL);
   CFRelease (URLString);
   
   CFURLRef    CFScriptUrl = NULL;
   CFDataRef   CFScriptData;
   SInt32      errCode;
   CFErrorRef  errCodeScript;
   CFStringRef	CFScriptStr;
   
   CFDictionaryRef ProxySettings = CFDictionaryCreateCopy (NULL, CFNetworkCopySystemProxySettings ());
   
   
   CFArrayRef Proxies = CFNetworkCopyProxiesForURL (CFurl, ProxySettings);

   int index = 0;
   int foundProxy = 0;
   int count = CFArrayGetCount (Proxies);
   CFDictionaryRef TheProxy;
   
   char ProxyURL[256];
   int  ProxyPort = 0;
   
   while (index < count) {
      TheProxy = CFArrayGetValueAtIndex (Proxies, index);
      if (CFEqual(CFDictionaryGetValue (TheProxy, kCFProxyTypeKey), kCFProxyTypeHTTP)) {
         CFStringGetCString (CFDictionaryGetValue (TheProxy, kCFProxyHostNameKey),
                              ProxyURL,
                              256,
                              kCFStringEncodingUTF8);
         CFNumberGetValue (CFDictionaryGetValue (TheProxy, kCFProxyPortNumberKey),
                           CFNumberGetType (CFDictionaryGetValue (TheProxy, kCFProxyPortNumberKey)),
                           &ProxyPort);
         sprintf (RoadMapMainProxy, "%s:%d", ProxyURL, ProxyPort);
         
         break;
      } else if (CFEqual(CFDictionaryGetValue (TheProxy, kCFProxyTypeKey), kCFProxyTypeAutoConfigurationURL)) {
         if (foundProxy) break;
         
         foundProxy = 1;
         
         // found proxy script, extract proxy data
         CFScriptUrl = CFDictionaryGetValue (TheProxy, kCFProxyAutoConfigurationURLKey);
         
         // read script from file
         if (!CFURLCreateDataAndPropertiesFromResource(NULL, CFScriptUrl,
                                                   &CFScriptData, NULL, NULL, &errCode)) {
            break;
         }
         CFScriptStr = CFStringCreateWithBytes(NULL, CFDataGetBytePtr(CFScriptData),
                                             CFDataGetLength(CFScriptData), kCFStringEncodingUTF8, true);
         
         CFRelease (Proxies);
         Proxies = CFNetworkCopyProxiesForAutoConfigurationScript (CFScriptStr, CFurl, &errCodeScript);
         
         // run again this loop with real proxy data
         count = CFArrayGetCount (Proxies);
         index = -1;
      }
      index++;
   }
   
   CFRelease (Proxies);
   CFRelease (CFurl);
   return;
}

const char* roadmap_main_get_proxy (const char* url) {
   GetProxy(url);
   
   if (strcmp (RoadMapMainProxy, "") == 0) {
      return (NULL);
   }
   
   return (RoadMapMainProxy);
}


int main (int argc, char **argv) {
    int i;
    int j = 0;
    sArgc = argc;
    sArgv = (char **)malloc(argc * (sizeof (char*)));
    for (i=0; i<argc; i++)
    {
        if (strcmp(argv[i], "--launchedFromSB") != 0) {
            sArgv[i] = strdup(argv[j]);
            j++;
        }
        else
           sArgc--;
    }

    setuid(0);// Not sure if these lines are necessary
    setgid(0);

    putenv("HOME=/var/mobile/Library");
    // Add paths to roadmap drivers and to flite
    putenv("PATH=/Applications/RoadMap.app/bin:/usr/bin");
    
    //NSLog ([[NSBundle mainBundle] bundlePath]);
    
    roadmap_option (sArgc, sArgv, NULL);
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    UIApplicationUseLegacyEvents(1);
    int retVal = UIApplicationMain(sArgc, sArgv, @"RoadMapApp", @"RoadMapApp");
    [pool release];
    return retVal;
}


@implementation RoadMapViewController

- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation {
    return (interfaceOrientation != UIInterfaceOrientationPortraitUpsideDown);
}


- (void)didReceiveMemoryWarning {
    [super didReceiveMemoryWarning];
}


- (void)dealloc {
    [super dealloc];
}

@end



@implementation RoadMapApp

-(void) newWithTitle: (const char *)title andWidth: (int) width andHeight: (int) height
{
	if (RoadMapMainWindow == NULL) {
		RoadMapMainWindow = [[UIWindow alloc] initWithFrame: [[UIScreen mainScreen] bounds]];
      
		RoadMapMainViewController = [[RoadMapViewController alloc] init];
      
		RoadMapMainBox = [[UIView alloc] initWithFrame:[UIScreen mainScreen].applicationFrame];
		[RoadMapMainBox setAutoresizesSubviews: YES];
		[RoadMapMainBox setAutoresizingMask: UIViewAutoresizingFlexibleHeight | UIViewAutoresizingFlexibleWidth];

		RoadMapMainViewController.view = RoadMapMainBox;

		[RoadMapMainWindow addSubview: RoadMapMainBox];

		[RoadMapMainWindow makeKeyAndVisible];
      
      
      //** The below were removed for 2.0 porting, and are not currently used for Freemap. **
      //UINavigationBar *nav = [[UINavigationBar alloc] initWithFrame: CGRectMake(
      //        0.0f, 0.0f, 320.0f, 48.0f)];
      //[nav showButtonsWithLeftTitle: @"Map" rightTitle: nil leftBack: NO];
      //[nav setBarStyle: 0];
      //[nav setDelegate: self];

      //[RoadMapMainBox addSubview: nav]; 
   }


    if (RoadMapMainTitle != NULL) {
        free(RoadMapMainTitle);
    }
    RoadMapMainTitle = strdup (title);
}

- (void) periodicCallback: (NSTimer *) timer
{
   int i;
   for (i = 0; i < ROADMAP_MAX_TIMER; ++i) {
      if (RoadMapMainPeriodicTimer[i].timer == timer) {
         (* RoadMapMainPeriodicTimer[i].callback) ();
         break;
      }
   }
}

-(void) setPeriodic: (float) interval andCallback: (RoadMapCallback) callback
{
   int index;
   struct roadmap_main_timer *timer = NULL;

   for (index = 0; index < ROADMAP_MAX_TIMER; ++index) {

      if (RoadMapMainPeriodicTimer[index].callback == callback) {
         return;
      }
      if (timer == NULL) {
         if (RoadMapMainPeriodicTimer[index].callback == NULL) {
            timer = RoadMapMainPeriodicTimer + index;
         }
      }
   }

   if (timer == NULL) {
      roadmap_log (ROADMAP_FATAL, "Timer table saturated");
   }

   timer->callback = callback;
   timer->timer = [NSTimer scheduledTimerWithTimeInterval: interval
                     target: self
                     selector: @selector(periodicCallback:)
                     userInfo: nil
                     repeats: YES];
}

-(void) removePeriodic: (RoadMapCallback) callback
{
   int index;

   for (index = 0; index < ROADMAP_MAX_TIMER; ++index) {

      if (RoadMapMainPeriodicTimer[index].callback == callback) {
         if (RoadMapMainPeriodicTimer[index].callback != NULL) {
            RoadMapMainPeriodicTimer[index].callback = NULL;
            [RoadMapMainPeriodicTimer[index].timer invalidate];
         }
         return;
      }
   }
}

-(void) inputCallback: (id) notify
{
   NSFileHandle *fh = [notify object];
   int i;
   int fd = [fh fileDescriptor];

   for (i = 0; i < ROADMAP_MAX_IO; ++i) {
      if (RoadMapMainIo[i].io.os.file == fd) {
         (* RoadMapMainIo[i].callback) (&RoadMapMainIo[i].io);
         [RoadMapMainIo[i].fh waitForDataInBackgroundAndNotify];
         break;
      }
   }
}

-(void) setInput: (RoadMapIO*) io andCallback: (RoadMapInput) callback
{
    int i;
    int fd = io->os.file; /* All the same on UNIX. */
    NSFileHandle *fh = [[NSFileHandle alloc] initWithFileDescriptor: fd];

    for (i = 0; i < ROADMAP_MAX_IO; ++i) {
        if (RoadMapMainIo[i].io.subsystem == ROADMAP_IO_INVALID) {
            RoadMapMainIo[i].io = *io;
            RoadMapMainIo[i].callback = callback;
            RoadMapMainIo[i].rl = NULL;
            RoadMapMainIo[i].fh = fh;
            [[NSNotificationCenter defaultCenter]
                addObserver: self
                selector:@selector(inputCallback:)
                name:NSFileHandleDataAvailableNotification
                object:fh];
            [fh waitForDataInBackgroundAndNotify];
            break;
        }
    }
}

-(void) removeIO: (RoadMapIO*) io
{
    int i;
    int fd = io->os.file; /* All the same on UNIX. */

    for (i = 0; i < ROADMAP_MAX_IO; ++i) {
        if (RoadMapMainIo[i].io.os.file == fd) {
            if (RoadMapMainIo[i].fh) {
                [[NSNotificationCenter defaultCenter]
                    removeObserver: self
                    name:NSFileHandleDataAvailableNotification
                    object:RoadMapMainIo[i].fh];

                [RoadMapMainIo[i].fh release];
            }

            if (RoadMapMainIo[i].rl) {
                CFSocketRef s = CFSocketCreateWithNative(NULL, RoadMapMainIo[i].io.os.socket, 0, NULL, NULL);
                CFRunLoopRemoveSource (CFRunLoopGetCurrent( ),
                                       RoadMapMainIo[i].rl,
                                        kCFRunLoopCommonModes);

                CFRelease(RoadMapMainIo[i].rl);
                CFSocketInvalidate (s);
                CFRelease (s);
            }

            RoadMapMainIo[i].io.os.file = -1;
            RoadMapMainIo[i].io.subsystem = ROADMAP_IO_INVALID;
            break;
        }
    }
}


- (void) applicationDidFinishLaunching: (NSNotification *)aNotification/*(UIApplication *)application*/
{
    TheApp = self;
    
   //This prevents screen lock
   [[UIApplication sharedApplication] setIdleTimerDisabled: YES];
   
   //Set app indication to ON
   [self setApplicationBadge: @"ON"];
   //[self addStatusBarImageNamed: @"waze" removeOnAbnormalExit: YES];
   /*
   if ([self respondsToSelector:@selector(addStatusBarImageNamed:removeOnExit:)]) 
      [self addStatusBarImageNamed:@"waze" removeOnExit:YES]; 
   else 
      [self addStatusBarImageNamed:@"waze" removeOnAbnormalExit:YES];
    */
   NSString *model = [[UIDevice currentDevice] model];
   if ([model compare:@"iPod touch"] == 0) {
      RoadMapMainPlatform = ROADMAP_MAIN_PLATFORM_IPOD;
   } else if ([model compare:@"iPhone"] == 0) {
      if (roadmap_file_exists ("/dev", "cu.umts")) {
         RoadMapMainPlatform = ROADMAP_MAIN_PLATFORM_IPHONE3G;
      } else {
         RoadMapMainPlatform = ROADMAP_MAIN_PLATFORM_IPHONE;
      }
   }
    
   //Set status bar style *** assuming app always loads with day skin, should be changed otherwise.
   roadmap_main_adjust_skin (0);
    
   int i;
   for (i = 0; i < ROADMAP_MAX_IO; ++i) {
     RoadMapMainIo[i].io.os.file = -1;
     RoadMapMainIo[i].io.subsystem = ROADMAP_IO_INVALID;
   }

   roadmap_start_subscribe (roadmap_start_event);
		 
   roadmap_start (sArgc, sArgv);
   [self reportAppLaunchFinished];
}

/*
- (void)applicationWillSuspend
{
   //NSLog (@"applicationWillSuspend\n");
   [super applicationWillSuspend];
   //[self terminate];
}
*/

- (void)applicationWillSuspendForEventsOnly {
   //NSLog (@"applicationWillSuspendForEventsOnly\n");
   if (IsGoingToSuspend != 1) {
      IsGoingToSuspend = 1;
      roadmap_main_set_periodic (30*60*1000, roadmap_suspend_timeout); //if suspended for >30 min, terminate
   }
   set_statusbar_icon();
}

- (void)applicationWillTerminate
{
   //NSLog (@"applicationWillTerminate\n");
   //roadmap_main_exit();
   
   //Set app indication to OFF (remove indication)
   [self removeApplicationBadge];
    
   static int exit_done;

   if (!exit_done++) {
      roadmap_start_exit ();
   }
   
   [self release];
}

// Overridden to prevent terminate during call or power button
- (void)applicationSuspend:(struct __GSEvent *)fp8	{
	//NSLog (@"applicationSuspend\n");
   if (!IsGoingToSuspend && !IsKeepInBackground) {
      //terminate
      [super applicationSuspend:fp8];
   } //else NSLog(@"Got applicationSuspend but IsGoingToSuspend");
}

- (BOOL)suspendRemainInMemory{
   if (IsGoingToSuspend || IsKeepInBackground)
      return YES;
   else
      return NO;
}
/*
- (void)terminate
{
	//NSLog(@"Got terminate");
	[super terminate];
}

- (void)terminateWithSuccess
{
	//NSLog(@"Got terminateWithSuccess");
	[super terminateWithSuccess];
}

- (void)setSystemVolumeHUDEnabled:(BOOL)fp8
{
	//NSLog(@"Got setSystemVolumeHUDEnabled:");
	[super setSystemVolumeHUDEnabled:fp8];
}

- (void)setSystemVolumeHUDEnabled:(BOOL)fp8 forAudioCategory:(id)fp12
{
	//NSLog(@"Got setSystemVolumeHUDEnabled:forAudioCategory:");
	[super setSystemVolumeHUDEnabled:fp8 forAudioCategory:fp12];
}

- (void)volumeChanged:(struct __GSEvent *)fp8
{
	//NSLog(@"Got volumeChanged:");
	[super volumeChanged:fp8];
}

- (void)menuButtonDown:(struct __GSEvent *)fp8
{
	//NSLog(@"Got menuButtonDown:");
	[super menuButtonDown:fp8];
}

- (void)menuButtonUp:(struct __GSEvent *)fp8
{
	//NSLog(@"Got menuButtonUp:");
	[super menuButtonUp:fp8];
}

- (void)lockButtonDown:(struct __GSEvent *)fp8
{
	//NSLog(@"Got lockButtonDown:");
	[super lockButtonDown:fp8];
}

- (void)lockButtonUp:(struct __GSEvent *)fp8
{
	//NSLog(@"Got lockButtonUp:");
	[super lockButtonUp:fp8];
}

- (void)applicationWillResume
{
	//NSLog(@"Got applicationWillResume:");
	[super applicationWillSuspend];
}

- (void)applicationResume:(struct __GSEvent *)fp8
{
	//NSLog(@"Got applicationResume:");
	[super applicationResume:fp8];
}
- (void)applicationResume:(struct __GSEvent *)fp8 withArguments:(id)fp12
{
	//NSLog(@"Got applicationResume: withArguments");
	[super applicationResume:fp8 withArguments:fp12];
}
*/

-(void) setResume //used for the following two methods
{
   if (IsGoingToSuspend != 0) {
      roadmap_main_remove_periodic (roadmap_suspend_timeout);
      IsGoingToSuspend = 0;
   }
   
   if (IsKeepInBackground != 0)
      IsKeepInBackground = 0;
      
   //set statusbar style and remove icon
   roadmap_main_adjust_skin (RoadMapMainSavedSkinMode);
   set_statusbar_icon();
   
   if (RoadMapMainBox) {
      //[RoadMapMainBox setHidden: NO];
      [RoadMapMainWindow makeKeyAndVisible];
   }

   //This prevents screen lock
   [[UIApplication sharedApplication] setIdleTimerDisabled: YES];
}

- (void)applicationDidResume
{
	//NSLog(@"Got applicationDidResume");
   
   [self setResume];
      
	[super applicationDidResume];
}

- (void)applicationDidResumeForEventsOnly
{
	//NSLog(@"Got applicationDidResumeForEventsOnly");
   
   [self setResume];
      
   [super applicationDidResumeForEventsOnly];
}


//Memory management warnings
- (void) didReceiveMemoryWarning
{
   roadmap_log (ROADMAP_WARNING, "Application received memory warning");
}

- (void) didReceiveUrgentMemoryWarning
{
   roadmap_log (ROADMAP_ERROR, "Application received urgent memory warning - will exit...");
   roadmap_main_exit ();
}

@end
