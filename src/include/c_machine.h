/*******************************************************************************
    crudNES - A NES emulator for reverse engineering purposes
    
    Copyright (C) 2003-2004 Sadai Sarmiento
    Copyright (C) 2023-2024 Franck "hitchhikr" Charlet

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*******************************************************************************/

#ifndef _CMACHINE_H
#define _CMACHINE_H

#include "datatypes.h"

class c_machine {

	public:

		c_machine (void) { bIsPowerOff = TRUE; }

		virtual void Open (int PAL, const char *FilePath) {};
		virtual void close (void) {};

		virtual void run (void) {};
		virtual void reset (void) {};
		virtual void pause (void) {};
		virtual void stop (void) { bis_running = FALSE; };

		__BOOL IsPowerOff (void) { return bIsPowerOff; }

		virtual void save_state (void) {};
		virtual void load_state (void) {};
		virtual void set_slot (__INT_32 Slot) {}; 

		virtual void set_sram (const char *) {};
		virtual void dump_header (const char *) {};

		virtual void load_config (void) {};

		__BOOL is_running () { return bis_running; }
		__BOOL is_paused () { return bis_paused; }

	protected:

		__BOOL bIsPowerOff, bis_running, bis_paused;
		char *SRamPath;
};

#endif