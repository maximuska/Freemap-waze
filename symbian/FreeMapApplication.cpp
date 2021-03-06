/* FreeMapApplication.c
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
#include "FreeMap.hrh"
#include "FreeMapDocument.h"
#include "FreeMapApplication.h"

// ============================ MEMBER FUNCTIONS ===============================

// -----------------------------------------------------------------------------
// CFreeMapApplication::CreateDocumentL()
// Creates CApaDocument object
// -----------------------------------------------------------------------------
//
CApaDocument* CFreeMapApplication::CreateDocumentL()
	{
	// Create an FreeMap document, and return a pointer to it
	return (static_cast<CApaDocument*>
					( CFreeMapDocument::NewL( *this ) ) );
	}

// -----------------------------------------------------------------------------
// CFreeMapApplication::AppDllUid()
// Returns application UID
// -----------------------------------------------------------------------------
//
TUid CFreeMapApplication::AppDllUid() const
	{
	// Return the UID for the FreeMap application
	return KUidFreeMapApp;
	}

// End of File
