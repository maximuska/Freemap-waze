/*
============================================================================
 Name       : RoadmapQueryDialog.h
 Author     : AGA
 Copyright  : GPL
 Description: Query dialog class customization
============================================================================
*/
#include <aknquerydialog.h>

// Extension of the query dialog class
class CRoadMapQueryDialog : public CAknTextQueryDialog
{
public:
    CRoadMapQueryDialog (TDes &aDataText, const TTone &aTone=ENoTone) :
    CAknTextQueryDialog(aDataText, aTone), iLeftSoftKeyVisible( ETrue ) {}

    static CRoadMapQueryDialog* NewL(TDes& aDataText, const TTone& aTone = ENoTone)
    {
        CRoadMapQueryDialog* self = new (ELeave) CRoadMapQueryDialog(aDataText, aTone);
        CleanupStack::PushL(self);
        return self;
    }

    virtual void UpdateLeftSoftKeyL ()
    {
        TBool leftSoftKeyVisible = ( iLeftSoftKeyVisible || CheckIfEntryTextOk() );
        MakeLeftSoftkeyVisible ( leftSoftKeyVisible );
    }

    void SetLeftSoftKeyVisible( TBool aVisible )
    {
        iLeftSoftKeyVisible = aVisible;
    }
protected:
    TBool iLeftSoftKeyVisible;
};
