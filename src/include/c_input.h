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

#ifndef _CNESINPUT_H
#define _CNESINPUT_H

#ifdef __CRUDNES_ALLEGRO
	#pragma warning (disable : 4200)
	#include "allegro.h"
#elif defined CRUDNES_SDL
	#include <sdl.h>
#endif

#include "c_save_state.h"
#include "c_cpu.h"

#include "c_machine.h"
#include "c_nes.h"

/******************************************************************************/
/** External Data                                                            **/
/******************************************************************************/

extern c_machine *o_machine;

class c_input
{
	public:
		c_input (void);
		~c_input (void);

		__UINT_8 handle_input (void)
		{
			handle_joypad ();
			return handle_key ();
		}

		void write_strobe (__UINT_8 value);
		__UINT_8 read_bitstream (void);

		void save_state (c_tracer o_writer);
		void load_state (c_tracer o_reader);

	private:

		__UINT_8 handle_key (void);
		void handle_joypad (void);

		__UINT_8 last_press,
			  bit_shifter;
	
		__BOOL full_strobe,
			 half_strobe;
};

#endif