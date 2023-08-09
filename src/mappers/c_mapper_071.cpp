/*******************************************************************************
    crudNES - A NES emulator for reverse engineering purposes
    CAMERICA
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
#include "../include/mappers/c_mapper_071.h"
#include "../include/c_nes.h"
#include "../include/c_ppu.h"
#include "../include/c_rom.h"
#include "../include/datatypes.h"

/******************************************************************************/
/** External Data                                                            **/
/******************************************************************************/

extern c_machine *o_machine;

c_mapper_071 :: c_mapper_071 (void)
{
	__DBG_INSTALLING ("Mapper #071");
	__DBG_INSTALLED ();
}

c_mapper_071 :: ~c_mapper_071 (void)
{
	__DBG_UNINSTALLING ("Mapper #071");
	__DBG_UNINSTALLED ();
}

void c_mapper_071 :: reset (void)
{
	last_page_switched = 0x00;
	nes->o_cpu->swap_page (0x8000, 0, _16K_);
	nes->o_cpu->swap_page (0xc000, nes->o_rom->information ().prg_pages - 1, _16K_);
}

void c_mapper_071 :: write_byte (__UINT_16 address, __UINT_8 value)
{
	if (address < 0xc000) 
	{
		if (0x9000 == (address & 0xf000)) {
			if (!(value & BIT_4)) nes->o_ppu->set_mirroring (_2C02_2400_MIRRORING);
			else nes->o_ppu->set_mirroring (_2C02_2000_MIRRORING);
		}
	}
	else if (address > 0xbfff)
	{
		nes->o_cpu->swap_page (0x8000, value & _16K_prg_mask, _16K_);
		last_page_switched = value & _16K_prg_mask;
	}
}
