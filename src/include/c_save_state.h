/*******************************************************************************
    crudNES - A NES emulator for reverse engineering purposes
	o_state Saver/Loader
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

#ifndef _CSAVESTATE_H
#define _CSAVESTATE_H

#include "allegro.h"
#include <fcntl.h>
#include <io.h>
#include <stdio.h>

#include "datatypes.h"

enum e_save_state { MAIN = 0, OTHER };

class c_save_state {

public:

	void reset (void);

	const char *get_filename (void);
	const char *get_filename (const char *extension);
    char *get_filename(char *filepath, const char *extension, int strip);

	void IncrementSlot (void) { iSlot = (iSlot + 1) & 7; rest (100); }
	void DecrementSlot (void) { iSlot = (iSlot - 1) & 7; rest (100); }

	void set_slot (__INT_32 Slot) { iSlot = Slot & 7; }
	void save (void); 
	void load (void);

private:
	__INT_32 iSlot;
};

#endif