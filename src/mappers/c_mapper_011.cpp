/*******************************************************************************
    crudNES - A NES emulator for reverse engineering purposes
    Color Dreams 
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

#include "../include/c_cpu.h"
#include "../include/c_tracer.h"
#include "../include/mappers/c_mapper.h"
#include "../include/mappers/c_mapper_011.h"
#include "../include/c_nes.h"
#include "../include/c_ppu.h"
#include "../include/c_rom.h"
#include "../include/datatypes.h"

/******************************************************************************/
/** External Data                                                            **/
/******************************************************************************/

extern c_machine *o_machine;

/******************************************************************************/
/** reset ()                                                                 **/
/******************************************************************************/

void c_mapper_011 :: reset (void)
{
	nes->o_cpu->swap_page (0x0000, 0, _32K_);
	nes->o_ppu->swap_page (0x0000, 0, _8K_);
}

/******************************************************************************/
/** write_byte ()                                                             **/
/******************************************************************************/

void c_mapper_011 :: write_byte (__UINT_16 address, __UINT_8 value)
{
	if (address < 0x8000)
	{
		if (nes->o_rom->information ().o_sram) nes->o_sram->write_byte (address, value);
	}
	else
	{
		nes->o_cpu->swap_page (0x8000, value & 0xf, _32K_);
		nes->o_ppu->swap_page (0x0000, value >> 4, _8K_);
	}
}
