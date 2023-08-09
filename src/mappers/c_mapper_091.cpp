/*******************************************************************************
    crudNES - A NES emulator for reverse engineering purposes
    HK-SF3
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
#include "../include/mappers/c_mapper_091.h"
#include "../include/c_nes.h"
#include "../include/c_ppu.h"
#include "../include/c_rom.h"
#include "../include/datatypes.h"

/******************************************************************************/
/** External Data                                                            **/
/******************************************************************************/

extern c_machine *o_machine;

c_mapper_091 :: c_mapper_091 (void)
{
	__DBG_INSTALLING ("Mapper #091");
	__DBG_INSTALLED ();
}

c_mapper_091 :: ~c_mapper_091 (void)
{
	__DBG_UNINSTALLING ("Mapper #091");
	__DBG_UNINSTALLED ();
}

void c_mapper_091 :: reset (void)
{
	bIRQEnabled = FALSE;
	iIRQCounter = 0;

	last_page_switched = 0x00;
	nes->o_cpu->swap_page (0x8000, nes->o_rom->information ().prg_pages - 1, _16K_);
	nes->o_cpu->swap_page (0xc000, nes->o_rom->information ().prg_pages - 1, _16K_);
}

void c_mapper_091 :: write_byte (__UINT_16 address, __UINT_8 value)
{
	if (address < 0x6ffc)
	{
		address = address & 3;
		if (0 == address) nes->o_ppu->swap_page (0x0000, value & _2K_chr_mask, _2K_);
		else if (1 == address) nes->o_ppu->swap_page (0x0800, value & _2K_chr_mask, _2K_);
		else if (2 == address) nes->o_ppu->swap_page (0x1000, value & _2K_chr_mask, _2K_);
		else if (3 == address) nes->o_ppu->swap_page (0x1800, value & _2K_chr_mask, _2K_);
	}
	else if (address < 0x7ffc) {
		address = address & 3;
		if (0 == address) nes->o_cpu->swap_page (0x8000, value & _8K_prg_mask, _8K_);
		else if (1 == address) nes->o_cpu->swap_page (0x2000, value & _8K_prg_mask, _8K_);
		else if (2 == address) { iIRQCounter = 0; bIRQEnabled = FALSE; }
		else if (3 == address) { bIRQEnabled = TRUE; }
	}
}

void c_mapper_091 :: h_blank (void)
{
	if (nes->o_ppu->information ().scanline > 240) return;

	if (bIRQEnabled) iIRQCounter ++;
	if (iIRQCounter >= 8) nes->o_cpu->request_irq ();
}