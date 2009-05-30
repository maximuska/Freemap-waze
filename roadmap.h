/* roadmap.h - general definitions use by the RoadMap program.
 *
 * LICENSE:
 *
 *   Copyright 2002 Pascal F. Martin
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
 */

#ifndef INCLUDE__ROADMAP__H
#define INCLUDE__ROADMAP__H

#include <assert.h>

#ifdef __SYMBIAN32__
typedef	unsigned int		uint32_t;
typedef unsigned short      uint16_t;
#elif (defined WIN32 || defined _WIN32 || defined WINCE || defined _WIN32_WCE)
   #include "Win32\stdint.h"
   #if(defined _DEBUG || defined DEBUG)
      #define  WIN32_DEBUG
   #endif   // DEBUG
   #define INVALID_THREAD_ID        (0xFFFFFFFF)
#else
   #include <stdint.h>
#endif   

#define  CLIENT_VERSION_MAJOR       (0)
#define  CLIENT_VERSION_MINOR       (14)
#define  CLIENT_VERSION_SUB         (2)
#define  CLIENT_VERSION_CFG         (0)

#include "roadmap_types.h"
#if defined (_WIN32) && !defined (__SYMBIAN32__)
#include "win32/roadmap_win32.h"
#elif defined (__SYMBIAN32__)
#include "symbian/roadmap_symbian_porting.h"
  #ifdef __SERIES60_31__
  #define __REMOVE_PLATSEC_DIAGNOSTIC_STRINGS__
  #warning "SERIES60_31 : __REMOVE_PLATSEC_DIAGNOSTIC_STRINGS__ defined!"
  #endif
#include <e32def.h>
#endif

#if defined (__SYMBIAN32__) || (!defined (_WIN32) && !defined (IPHONE))
#if((!defined BOOL_DEFINED))
	#define	BOOL_DEFINED
	typedef signed char	BOOL;
#endif	//	BOOL_DEFINED
#elif defined (IPHONE)
#include <objc/objc.h>
#endif

extern int USING_PHONE_KEYPAD;

#ifndef	TRUE
	#define	TRUE	(1)
#endif	//	TRUE
#ifndef	FALSE
	#define	FALSE	(0)
#endif	//	FALSE

#ifndef	FREE
	#define	FREE(_p_)				if((_p_)){ free((_p_)); (_p_)=NULL;}
#endif	//	FREE

#ifdef J2ME

#ifdef assert
#undef assert
#endif

#include <java/lang.h>
#include <javax/microedition/lcdui.h>

static NOPH_Display_t assert_display;

static inline void do_assert(char *text) {

  printf ("do_assert:%s********************************************************************************************\n", text);
  if (!assert_display) assert_display = NOPH_Display_getDisplay(NOPH_MIDlet_get());
  NOPH_Alert_t msg = NOPH_Alert_new("ASSERT!", text, 0, NOPH_AlertType_get(NOPH_AlertType_INFO));
  NOPH_Alert_setTimeout(msg, NOPH_Alert_FOREVER);
  NOPH_Display_setCurrent(assert_display, msg);
  NOPH_Thread_sleep( 10000 );
}

# define assert(x) do { \
 if ( ! (x) ) \
 {\
     char msg[256]; \
     snprintf (msg, sizeof(msg), \
        "ASSERTION FAILED at %d in %s:%s\n", __LINE__, __FILE__, __FUNCTION__); \
     do_assert(msg); \
     exit(1); \
 } \
 } while(0)

#endif

#define strncpy_safe(dest, src, size) { strncpy ((dest), (src), (size)); (dest)[(size)-1] = '\0'; }

#define ROADMAP_MESSAGE_DEBUG      1
#define ROADMAP_MESSAGE_INFO       2
#define ROADMAP_MESSAGE_WARNING    3
#define ROADMAP_MESSAGE_ERROR      4
#define ROADMAP_MESSAGE_FATAL      5

#define ROADMAP_DEBUG   ROADMAP_MESSAGE_DEBUG,__FILE__,__LINE__
#define ROADMAP_INFO    ROADMAP_MESSAGE_INFO,__FILE__,__LINE__
#define ROADMAP_WARNING ROADMAP_MESSAGE_WARNING,__FILE__,__LINE__
#define ROADMAP_ERROR   ROADMAP_MESSAGE_ERROR,__FILE__,__LINE__
#define ROADMAP_FATAL   ROADMAP_MESSAGE_FATAL,__FILE__,__LINE__

#if(defined _DEBUG || defined DEBUG)
   ///[BOOKMARK]:[NOTE]:[PAZ] -  Enable logs in debug build:
   ///#define  SKIP_DEBUG_LOGS
#endif   // DEBUG

///[BOOKMARK]:[NOTE]:[PAZ] - Set default log level:
#define  DEFAULT_LOG_LEVEL       ROADMAP_MESSAGE_WARNING

#ifdef   WIN32_DEBUG
   ///[BOOKMARK]:[NOTE]:[PAZ] - In case of fatal error - halt process and wait for debugger to attach
   #define  FREEZE_ON_FATAL_ERROR
#endif   // WIN32_DEBUG

typedef void (*roadmap_log_msgbox_handler) (const char *title, const char *msg);

void roadmap_log_push        (const char *description);
void roadmap_log_pop         (void);
void roadmap_log_reset_stack (void);

void roadmap_log (int level, char *source, int line, char *format, ...);

void roadmap_log_save_all  (void);
void roadmap_log_save_none (void);
void roadmap_log_purge     (void);

int  roadmap_log_enabled (int level, char *source, int line);

void roadmap_log_register_msgbox (roadmap_log_msgbox_handler handler);

#define roadmap_check_allocated(p) \
            roadmap_check_allocated_with_source_line(__FILE__,__LINE__,p)

#define ROADMAP_SHOW_AREA        1
#define ROADMAP_SHOW_SQUARE      2



void roadmap_option_initialize (void);

int  roadmap_option_is_synchronous (void);

char *roadmap_debug (void);

int   roadmap_verbosity  (void); /* return a minimum message level. */
int   roadmap_is_visible (int category);
char *roadmap_gps_source (void);

int roadmap_option_cache  (void);
int roadmap_option_width  (const char *name);
int roadmap_option_height (const char *name);


typedef void (*RoadMapUsage) (const char *section);

void roadmap_option (int argc, char **argv, RoadMapUsage usage);


/* This function is hidden by a macro: */
void roadmap_check_allocated_with_source_line
                (char *source, int line, const void *allocated);

typedef void (* RoadMapCallback) (void);

#endif // INCLUDE__ROADMAP__H

