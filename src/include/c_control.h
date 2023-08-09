/*******************************************************************************
    crudNES - A NES emulator for reverse engineering purposes
    Hardware Controller
    Copyright (C) 2003-2004 Sadai Sarmiento
    Copyright (C) 2023 Franck "hitchhikr" Charlet

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

#ifndef _CNESCONTROL_H
#define _CNESCONTROL_H

#include "c_machine.h"
#include "c_nes.h"

/******************************************************************************/
/** External Data                                                            **/
/******************************************************************************/

extern c_machine *o_machine;

enum NESControlList
{
	APU = 1, DMA = 2
};

class c_nes_control
{
	public:

		c_nes_control (void);
		~c_nes_control (void);
		
		__UINT_8 read_byte (__UINT_16 address);
		__UINT_16 read_word (__UINT_16 address);

		void write_byte (__UINT_16 address, __UINT_8 value);

		void save_state (c_tracer o_writer)
		{
			o_writer.write (control [APU], 0x18);
			o_writer.write (control [DMA], 0x01);
		}

		void load_state (c_tracer o_reader)
		{
			o_reader.read (control [APU], 0x18);
			o_reader.read (control [DMA], 0x01);
		}

	private:
		__UINT_8 ApuDmaReadByte (const __UINT_16 &);
		void ApuDmawrite_byte (const __UINT_16 &, const __UINT_8 &);

		__UINT_8 *control [3];
};

#endif
