/* roadmap_text.cpp - Text handling
 *
 * LICENSE:
 *
 *   Copyright 2008 Ehud Shabtai
 *
 *   This file is part of RoadMap.
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
 *
 * SYNOPSYS:
 *
 *   See roadmap_text.h
 */


extern "C"{
#include "roadmap_input_type.h"
#include "roadmap.h"
}
#include <eikenv.h>
#include "FreeMapAppUi.h"


void roadmap_input_type_set_mode( roadmap_input_type mode )
{
	TUint newCapabilities;
	CFreeMapAppUi* pAppUi;
	switch( mode )
	{
		case inputtype_alphabetic : 
		case inputtype_alpha_spaces :
		{
			newCapabilities = TCoeInputCapabilities::EWesternAlphabetic;
			break;
		}
		case inputtype_numeric :
		{
			newCapabilities = TCoeInputCapabilities::EWesternNumericIntegerPositive;
			break;
		}
		case inputtype_alphanumeric :
		case inputtype_none :
		default:
		{
			newCapabilities = TCoeInputCapabilities::ENone;
			break;
		}
	} // switch
	// Update the UI object
	pAppUi = static_cast<CFreeMapAppUi*>( CEikonEnv::Static()->EikAppUi() );
	pAppUi->SetInputCapabilities( newCapabilities );
	
	// Log the operation
	roadmap_log( ROADMAP_DEBUG, "roadmap_text_set_input_mode() - Current capabilities : %d", newCapabilities );
}
