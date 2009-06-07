/* FreeMapAppUi.cpp
 *
 * LICENSE:
 *
 *   Copyright 2008 Ehud Shabtai
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

// INCLUDE FILES
#include <avkon.hrh>
#include <aknmessagequerydialog.h>
#include <aknnotewrappers.h>
#include <stringloader.h>
#include <f32file.h>
#include <s32file.h>
#include <hlplch.h>

#include <FreeMap.rsg>

//@@ #include "FreeMap_0x2001EB29.hlp.hrh"
#include "FreeMap.hrh"
#include "FreeMap.pan"
#include "FreeMapApplication.h"
#include "FreeMapAppUi.h"
#include "FreeMapAppView.h"

#include <PtiEngine.h>
#include <PtiKeyMappings.h>
#include <ptidefs.h>
#include <featdiscovery.h>
#include <featureinfo.h>
#include <utf.h>

extern "C" {
#include "roadmap_start.h"
#include "editor/editor_main.h"
extern TKeyResponse roadmap_main_process_key(TUint code, TEventCode aType);
}

extern void roadmap_canvas_new (RWindow& aWindow, int initWidth, int initHeight);
int global_FreeMapLock();
int global_FreeMapUnlock();
static RMutex* pSyncMutex = NULL; 
static int FM_ref = 0;

#define BACKLIGHT_DISABLE_INTERVAL	2000		// Miliseconds 


// ============================ MEMBER FUNCTIONS ===============================

int global_FreeMapLock()
{
  if ( pSyncMutex == NULL )
  {
    pSyncMutex = new RMutex();
    if ( pSyncMutex == NULL )
    {
      roadmap_log(ROADMAP_ERROR, "Mutex not created!");
      return KErrGeneral;
    }
    pSyncMutex->CreateLocal();
  }
  
  if (FM_ref > 0) {
    return 1;
  }
  FM_ref++;
  pSyncMutex->Wait();
  return KErrNone;
}

int global_FreeMapUnlock()
{
  if ( pSyncMutex == NULL )
  {
    //ERROR!
    roadmap_log(ROADMAP_ERROR, "Mutex not initialized!");
  }
  FM_ref--;
  pSyncMutex->Signal();
  return KErrNone;
}


static void roadmap_start_event (int event) {
   switch (event) {
   case ROADMAP_START_INIT:
      editor_main_check_map ();
      break;
   }
}





CStartTimer::CStartTimer(TInt aPriority)
  : CTimer(aPriority)
{
  m_pWaiter = NULL;
  m_State = EContinue;
}

CStartTimer::~CStartTimer()
{
  Cancel();
  Deque();
}

CStartTimer* CStartTimer::NewL()
{
  CStartTimer* self = new (ELeave) CStartTimer(EPriorityLess);
  CleanupStack::PushL(self);
  self->ConstructL();
  CleanupStack::Pop();
  return self;
}

void CStartTimer::ConstructL()
{
  m_pWaiter = new (ELeave) CActiveSchedulerWait();
  CTimer::ConstructL();
  CActiveScheduler::Add(this);
}

void CStartTimer::DoCancel()
{
  CTimer::DoCancel();
}

void CStartTimer::RunL()
{
  m_pWaiter->AsyncStop();
  if ( m_State == EContinue )
  {// for now we do NOT do it again, man
    m_State = EDismiss;
    //Start();
  }
}

void CStartTimer::Start()
{
  TTimeIntervalMicroSeconds32 delay(1000);
  After(delay);
  m_pWaiter->Start();
}



// -----------------------------------------------------------------------------
// CFreeMapAppUi::ConstructL()
// Symbian 2nd phase constructor can leave.
// -----------------------------------------------------------------------------
//
void CFreeMapAppUi::ConstructL()
	{
	// Initialise app UI with standard value.
	BaseConstructL(CAknAppUi::EAknEnableSkin);

	// Init rotation params
	HandleRotationChange(true);
	 
	// Create view object
	iAppView = CFreeMapAppView::NewL( ApplicationRect() );
	
	// View is fullscreen but we also want to hide the pane and softkeys
	//StatusPane()->MakeVisible(EFalse);
	//Cba()->MakeVisible(EFalse);
	iAppView->SetMopParent(this);
	AddToStackL(iAppView);
	
	// Init the qwerty mapping
	InitQwertyMappingsL();
	
	// Init and start the backlight timer active object
    if ( m_pBLTimer == NULL ) 
	{
		m_pBLTimer = CBackLightTimer::NewL( ETrue );
	}
    m_pBLTimer->Start();
    
	// Return control to the system before showing anything onscreen
	if ( m_pStartTimer == NULL )
	{
	  m_pStartTimer = CStartTimer::NewL();
	}
  
  
	m_pStartTimer->Start();
  
	global_FreeMapLock();
   roadmap_start_subscribe (roadmap_start_event);
	roadmap_start(0, NULL);
	global_FreeMapUnlock();
	}
// -----------------------------------------------------------------------------
// CFreeMapAppUi::CFreeMapAppUi()
// C++ default constructor can NOT contain any code, that might leave.
// -----------------------------------------------------------------------------
//
CFreeMapAppUi::CFreeMapAppUi() : m_InputCapabilities( TCoeInputCapabilities::ENone )  
{
  	m_pStartTimer = NULL;
  	m_pBLTimer = NULL;
  	// No implementation required
}

// -----------------------------------------------------------------------------
// CFreeMapAppUi::~CFreeMapAppUi()
// Destructor.
// -----------------------------------------------------------------------------
//
CFreeMapAppUi::~CFreeMapAppUi()
{
	if ( iAppView )
	{
		RemoveFromStack(iAppView);
		delete iAppView;
		iAppView = NULL;
	}
	if (m_pStartTimer != NULL)
	{
		delete m_pStartTimer;
		m_pStartTimer = NULL;
	}
	if ( m_pBLTimer != NULL )
	{
		delete m_pBLTimer;
		m_pBLTimer = NULL;
	}	
	if( m_pQwertyKeyMappings )
	{
		delete m_pQwertyKeyMappings;
		m_pQwertyKeyMappings = NULL;
	}
	if( m_pPtiEngine )
	{
		delete m_pPtiEngine;
		m_pQwertyKeyMappings = NULL;				
	}
}

// -----------------------------------------------------------------------------
// CFreeMapAppUi::HandleCommandL()
// Takes care of command handling.
// -----------------------------------------------------------------------------
//
void CFreeMapAppUi::HandleCommandL( TInt aCommand )
	{
	switch( aCommand )
		{
		case EEikCmdExit:
		case EAknSoftkeyExit:
			Exit();
			break;
		default:
			Panic( EFreeMapUi );
			break;
		}
	}

TKeyResponse CFreeMapAppUi::HandleKeyEventL(const TKeyEvent& aKeyEvent,TEventCode aType)
{
	if (global_FreeMapLock() != 0) return EKeyWasNotConsumed;
	TKeyResponse res = EKeyWasNotConsumed;
	// Process the data
	res = roadmap_main_process_key( aKeyEvent.iScanCode, aType );	
	global_FreeMapUnlock();
	return res;
}

// -----------------------------------------------------------------------------
//  Called by the framework when the application status pane
//  size is changed.  Passes the new client rectangle to the
//  AppView
// -----------------------------------------------------------------------------
//
void CFreeMapAppUi::HandleStatusPaneSizeChange()
{
  iAppView->SetRect( ApplicationRect() );
  iAppView->SetExtentToWholeScreen();
}

void CFreeMapAppUi::HandleRotationChange(bool bInitOnly)
{
  TPixelsTwipsAndRotation sizeAndRot;
  CWsScreenDevice* pScreenDevice = CEikonEnv::Static()->ScreenDevice();
  pScreenDevice->GetScreenModeSizeAndRotation(pScreenDevice->CurrentScreenMode(),
                                              sizeAndRot);
  if ( !bInitOnly && (sizeAndRot.iPixelSize != m_ScreenSize) )
  {// rotation changed
    roadmap_canvas_new (iAppView->GetWindow(), 
                        sizeAndRot.iPixelSize.iWidth,
                        sizeAndRot.iPixelSize.iHeight);
    iAppView->Draw(ApplicationRect());  //  or just use roadmap_canvas_refresh();
  }
  //  keep for next time
  //m_Orientation = sizeAndRot.iRotation;
  m_ScreenSize = sizeAndRot.iPixelSize;
}

void CFreeMapAppUi::HandleResourceChangeL(TInt aType)
{
  CAknAppUi::HandleResourceChangeL(aType);
  iAppView->HandleResourceChange(aType);
  HandleRotationChange(false);
}

CArrayFix<TCoeHelpContext>* CFreeMapAppUi::HelpContextL() const
	{
	#warning "Please see comment about help and UID3..."
	// Note: help will not work if the application uid3 is not in the
	// protected range.  The default uid3 range for projects created
	// from this template (0xE0000000 - 0xEFFFFFFF) are not in the protected range so they
	// can be self signed and installed on the device during testing.
	// Once you get your official uid3 from Symbian Ltd. and find/replace
	// all occurrences of uid3 in your project, the context help will
	// work.
	CArrayFixFlat<TCoeHelpContext>* array = new(ELeave)CArrayFixFlat<TCoeHelpContext>(1);
	CleanupStack::PushL(array);
//@@	array->AppendL(TCoeHelpContext(KUidFreeMapApp, KGeneral_Information));
	CleanupStack::Pop(array);
	return array;
	}

void CFreeMapAppUi::InitQwertyMappingsL()
{
	/* Currently hebrew only !!! */
	/* TODO :: initialize the set of the languages */
	TLanguage lang = ELangHebrew;
    CPtiCoreLanguage* pCoreLanguage;    
	
    if ( !CFeatureDiscovery::IsFeatureSupportedL( KFeatureIdQwertyInput ) )
	{
		m_pQwertyKeyMappings = NULL;
		m_pPtiEngine = NULL;
		USING_PHONE_KEYPAD = 1;
		roadmap_log( ROADMAP_INFO, "InitQwertyMappingsL() - No qwerty - setting the phone keyboard" );
		return;
	}
    else
    {
    	roadmap_log( ROADMAP_INFO, "InitQwertyMappingsL() - Qwerty exists - setting the qwerty keyboard" );
    	USING_PHONE_KEYPAD = 0;
    }
    // Instantiate the engine
    m_pPtiEngine = CPtiEngine::NewL( ETrue );
    
	// Make a language object based on current language
    pCoreLanguage = static_cast<CPtiCoreLanguage*> ( m_pPtiEngine->GetLanguage( lang ) );
	
	// Get the keyboard mappings for the language
	m_pQwertyKeyMappings = static_cast<CPtiQwertyKeyMappings*>( pCoreLanguage->GetQwertyKeymappings());

	
}

TBool  CFreeMapAppUi::GetUnicodeForScanCodeL( TInt aScanCode, TUint16 &aUnicodeOut ) const
{
	// Scan code of the key you are interested in
    TPtiKey key = ( TPtiKey ) aScanCode;
    TBuf<20> iResUnicode;
    TBuf8<20> iResUtf8;
    TBool iRes = EFalse;
    
    
    // Exceptional cases handling 
    switch ( key )
	{
    	case EPtiKeyQwertySpace:	// SPACE - return the utf8 0x20 
		{
			aUnicodeOut = 0x20;
			return ETrue;
		}
    	default: break;
	}
    
    
    // If no qwerty return false
    if ( m_pQwertyKeyMappings )
    {
	    m_pQwertyKeyMappings->GetDataForKey( key, iResUnicode, EPtiCaseLower );
	    
	    // If the mapping fail return false
	    if ( iResUnicode.Length() )
		{
			HBufC8* pTmpBuf;
			TUint8 *ptr;
			
			// Convert to UTF8
			CnvUtfConverter::ConvertFromUnicodeToUtf8( iResUtf8, iResUnicode );
			pTmpBuf = iResUtf8.AllocLC(); 
			ptr = reinterpret_cast<TUint8*>( &aUnicodeOut );
			Mem::Copy( ptr, pTmpBuf->Ptr(), iResUtf8.Length() );
			CleanupStack::PopAndDestroy( pTmpBuf );
	    	iRes = ETrue;
		}
	    else
    	{
    		roadmap_log( ROADMAP_INFO, "GetUnicodeForScanCodeL(..) - No qwerty mapping for the ScanCode %d", aScanCode );
    	}
    }
    return ( iRes ); 
}

void CFreeMapAppUi::SetBackLiteOn( TBool aValue )
{
	m_pBLTimer->SetBLEnable( aValue );
}


TCoeInputCapabilities CFreeMapAppUi::InputCapabilities() const
{	
	TCoeInputCapabilities caps(  CCoeAppUi::InputCapabilities() );
	caps.SetCapabilities( m_InputCapabilities );
	return caps;
}

void CFreeMapAppUi::SetInputCapabilities( TUint aCapabilities )
{
	m_InputCapabilities = aCapabilities;
}


// -----------------------------------------------------------------------------

CBackLightTimer::CBackLightTimer( TInt aPriority, TBool aEnable ) : 
	CTimer( aPriority ), iEnable( aEnable )
{
}

CBackLightTimer::~CBackLightTimer()
{
	Cancel();
	Deque();
}

CBackLightTimer* CBackLightTimer::NewL( TBool aEnable )
{
	// Create an object
	CBackLightTimer* self = new ( ELeave ) CBackLightTimer( EPriorityLess, aEnable );
	CleanupStack::PushL( self );
	self->ConstructL();
	CleanupStack::Pop();
	return self;
}

void CBackLightTimer::ConstructL()
{
	// Enable the timer work
	iEnable = ETrue;
	SetBLDisableInterval( BACKLIGHT_DISABLE_INTERVAL );
	
	CTimer::ConstructL();
	// Add to active scheduler	
	CActiveScheduler::Add( this );
}

void CBackLightTimer::DoCancel()
{
  CTimer::DoCancel();
}

void CBackLightTimer::RunL()
{
	// If an error occurred (admittedly unlikely)
	// deal with the problem in RunError()
	User::LeaveIfError( iStatus.Int() );

	// Otherwise reset the inactivity and resubmit the timer
	if ( iEnable )	
	{
		User::ResetInactivityTime();
	}
	After( iInterval );		// SetActive inside	
}

void CBackLightTimer::Start()
{
	if ( iEnable )
	{
		// First reset
		User::ResetInactivityTime();
	}
	// Set the timer
	After( iInterval );		// SetActive inside
	// SetActive();
}

inline void CBackLightTimer::SetBLDisableInterval( TInt aValue )
{
	iInterval = aValue;
}
inline TInt CBackLightTimer::GetBLDisableInterval( void ) const
{
	return iInterval.Int();
}

inline void CBackLightTimer::SetBLEnable( TBool aValue )
{
	iEnable = aValue;
}
inline TBool CBackLightTimer::GetBLEnable( void ) const
{
	return iEnable;
}

TInt CBackLightTimer::RunError( TInt aError )
{
	roadmap_log( ROADMAP_ERROR, "BackLightTimer leaves with error: %d", aError );
	return (KErrNone);                // Error has been handled
}

// End of File



