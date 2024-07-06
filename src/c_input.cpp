/*******************************************************************************
    crudNES - A NES emulator for reverse engineering purposes
    o_input Handler
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

#include <stdio.h>
#include "include/allegro.h"
#include "include/c_input.h"
#include "include/c_machine.h"
#include "include/c_nes.h"
#include "include/c_graphics.h"

/******************************************************************************/
/** External Data                                                            **/
/******************************************************************************/

extern c_machine *o_machine;

/******************************************************************************/
/** Installer                                                                **/
/******************************************************************************/

c_input :: c_input (void)
{
//	__DBG_INSTALLING ("Input");

	full_strobe = FALSE;
	half_strobe = FALSE;
	last_press  = 0;
	bit_shifter = 0;
    
	install_keyboard ();
	set_keyboard_rate (0, 0);
	install_joystick (JOY_TYPE_AUTODETECT);

//	__DBG_INSTALLED ();
}

/******************************************************************************/
/** Uninstaller                                                              **/
/******************************************************************************/

c_input :: ~c_input (void)
{
	__DBG_UNINSTALLING ("Input");
	__DBG_UNINSTALLED ();
}

/******************************************************************************/
/** write_strobe ()                                                           **/
/******************************************************************************/

void c_input :: write_strobe (__UINT_8 value)
{
	if (value == BIT_0) { full_strobe = FALSE; half_strobe = TRUE; }
	else if (value == 0) { full_strobe = TRUE; half_strobe = FALSE; bit_shifter = 0; }
}

/******************************************************************************/
/** read_bitstream ()                                                         **/
/******************************************************************************/

__UINT_8 c_input :: read_bitstream (void)
{
	if (!full_strobe) return 0;

	__UINT_8 joypad_data = (last_press >> bit_shifter ++) & 1;

	if (bit_shifter == 8) full_strobe = FALSE;

	return (joypad_data);
}

/******************************************************************************/
/** handle_key ()                                                        **/
/******************************************************************************/

__UINT_8 c_input :: handle_key (void)
{
	if (key [KEY_ESC]) { return CPU_INT_QUIT; }
	if (key [KEY_F5]) { nes->o_state->save (); clear_keybuf (); }
	if (key [KEY_F6]) { nes->o_state->load (); clear_keybuf (); }
	if (key [KEY_F7]) nes->o_state->DecrementSlot ();
	if (key [KEY_F8]) nes->o_state->IncrementSlot ();
	if (key [KEY_F10]) nes->o_cpu->reset ();
	if (key [KEY_F12]) nes->o_cpu->toggle_logtracer ();
	if (key [KEY_P])
	{
	    key [KEY_P] = FALSE;
	    nes->pause ();
	}

	return CPU_INT_NONE;
}

/******************************************************************************/
/** handle_joypad ()                                                     **/
/******************************************************************************/

void c_input :: handle_joypad (void)
{
	last_press = 0;

	//Joypad #1     
	if (!poll_keyboard ())
	{
		if(key [KEY_S])	last_press |= BIT_0;
		if(key [KEY_A])	last_press |= BIT_1;
		if(key [KEY_SPACE])	last_press |= BIT_2;
		if(key [KEY_ENTER]) last_press |= BIT_3;
		if(key [KEY_UP])	last_press |= BIT_4;
		else if(key [KEY_DOWN])	last_press |= BIT_5;
		if(key [KEY_LEFT]) last_press |= BIT_6;
		else if(key [KEY_RIGHT]) last_press |= BIT_7;
	}
	if (!poll_joystick())
	{
		if (joy [0].button [0].b) last_press |= BIT_0;
		if (joy [0].button [1].b) last_press |= BIT_1;
		if (joy [0].button [2].b) last_press |= BIT_2;
		if (joy [0].button [3].b) last_press |= BIT_3;
		if (joy [0].button [7].b) last_press |= BIT_3;
		if (joy [0].stick [0].axis [0].d1) last_press |= BIT_6;
		if (joy [0].stick [0].axis [0].d2) last_press |= BIT_7;
		if (joy [0].stick [0].axis [1].d1) last_press |= BIT_4;
		if (joy [0].stick [0].axis [1].d2) last_press |= BIT_5;
	}
}

/******************************************************************************/
/** save_state ()                                                             **/
/******************************************************************************/

void c_input :: save_state (c_tracer o_writer)
{
	o_writer.write (&last_press, 1);
	o_writer.write (&bit_shifter, 1);
	o_writer.write (&full_strobe, 1);
	o_writer.write (&half_strobe, 1);
}

/******************************************************************************/
/** load_state ()                                                             **/
/******************************************************************************/

void c_input :: load_state (c_tracer o_reader)
{
	o_reader.read (&last_press, 1);
	o_reader.read (&bit_shifter, 1);
	o_reader.read (&full_strobe, 1);
	o_reader.read (&half_strobe, 1);
}
