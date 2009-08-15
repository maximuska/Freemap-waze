/* FreeMapAppUi.h
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


#ifndef __FREEMAPAPPUI_h__
#define __FREEMAPAPPUI_h__

// INCLUDES
#include <aknappui.h>


// FORWARD DECLARATIONS
class CFreeMapAppView;
class CEikEdwin;
class CPtiQwertyKeyMappings;
class CPtiEngine;
class CStartTimer;
class CBackLightTimer;

// CLASS DECLARATION
/**
* CFreeMapAppUi application UI class.
* Interacts with the user through the UI and request message processing
* from the handler class
*/
class CFreeMapAppUi : public CAknAppUi
    {
    public: // Constructors and destructor

        /**
        * ConstructL.
        * 2nd phase constructor.
        */
        void ConstructL();

        /**
        * CFreeMapAppUi.
        * C++ default constructor. This needs to be public due to
        * the way the framework constructs the AppUi
        */
        CFreeMapAppUi();


        /**
         * Initialization of the qwerty mapping
         */
        void InitQwertyMappingsL();

        /**
         * Keyboard mapping data for scan code - call InitQwertyMappingsL first !
         */
        TBool  GetUnicodeForScanCodeL( TInt aScanCode, TUint16 &aUnicodeOut ) const;

        /**
         * Keyboard mapping data for scan code - call InitQwertyMappingsL first !
         */
        void SetBackLiteOn( TBool aValue );
        /**
         * Input capabilities override
         */
        virtual TCoeInputCapabilities InputCapabilities() const;
        /**
         * Set the input capabilities
         */
        void SetInputCapabilities( TUint aCapabilities );
        /**
        * ~CFreeMapAppUi.
        * Virtual Destructor.
        */
        virtual ~CFreeMapAppUi();

    protected:


    private:  // Functions from base classes

        /**
        * From CEikAppUi, HandleCommandL.
        * Takes care of command handling.
        * @param aCommand Command to be handled.
        */
        void HandleCommandL( TInt aCommand );

        TKeyResponse HandleKeyEventL(const TKeyEvent& aKeyEvent,TEventCode aType);

        /**
        *  HandleStatusPaneSizeChange.
        *  Called by the framework when the application status pane
        *  size is changed.
        */
        void HandleStatusPaneSizeChange();

        void HandleRotationChange(bool bInitOnly);

        //  Handle resource change
        //  It is used to handle changes in Orientation.
        void HandleResourceChangeL(TInt aType);

        /**
        *  From CCoeAppUi, HelpContextL.
        *  Provides help context for the application.
        *  size is changed.
        */
        CArrayFix<TCoeHelpContext>* HelpContextL() const;

    private: // Data

        /**
        * The application view
        * Owned by CFreeMapAppUi
        */
        CFreeMapAppView* iAppView;

        //CFbsBitGc::TGraphicsOrientation m_Orientation;
        TSize m_ScreenSize;

        CStartTimer* m_pStartTimer;

        CBackLightTimer* m_pBLTimer;

        /* Qwerty mappings for the keyboard translation */
        /* TODO :: Initialize list of the available languages */
        CPtiQwertyKeyMappings* m_pQwertyKeyMappings;
        CPtiEngine* m_pPtiEngine;

        /* Input mode constraint */
        TUint   m_InputCapabilities;

    };

#endif // __FREEMAPAPPUI_h__


class CStartTimer : public CTimer
{
public:
  virtual ~CStartTimer();
  static CStartTimer* NewL();
  void ConstructL();
  void Start();

protected:
  CStartTimer(TInt aPriority);
  void RunL();
  void DoCancel();

  CActiveSchedulerWait* m_pWaiter;
  TInt m_State;
  enum {
    EContinue,
    EDismiss,
  };
};


// CLASS DECLARATION
/**
* CBackLightTimer responsible for the management of the backlight function
* in current implementation it disables the backlight every 4 seconds
*/
class CBackLightTimer : public CTimer
{
    public:
        static CBackLightTimer* NewL( TBool aEnable );
        void ConstructL();
        void Start();

        void SetBLDisableInterval( TInt aValue );
        TInt GetBLDisableInterval( void ) const;

        void SetBLEnable( TBool aValue );
        TBool GetBLEnable( void ) const;

        virtual ~CBackLightTimer();


    protected:

        CBackLightTimer( TInt aPriority, TBool aEnable  );
        virtual void RunL( void );
        virtual void DoCancel( void );
        virtual TInt RunError( TInt aError );

    private:

        TTimeIntervalMicroSeconds32 iInterval;
        TBool   iEnable;

};


// End of File

