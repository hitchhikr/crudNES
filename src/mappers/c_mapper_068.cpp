/*******************************************************************************
    crudNES - A NES emulator for reverse engineering purposes
    SUNSOFT
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
#include "../include/mappers/c_mapper_068.h"
#include "../include/c_nes.h"
#include "../include/c_ppu.h"
#include "../include/c_rom.h"
#include "../include/datatypes.h"

/******************************************************************************/
/** External Data                                                            **/
/******************************************************************************/

extern c_machine *o_machine;

c_mapper_068 :: c_mapper_068 (void)
{
	__DBG_INSTALLING ("Mapper #068");
	__DBG_INSTALLED ();
}

c_mapper_068 :: ~c_mapper_068 (void)
{
	__DBG_UNINSTALLING ("Mapper #068");
	__DBG_UNINSTALLED ();
}

void c_mapper_068 :: reset (void)
{
	Registers [0] = Registers [1] = Registers [2] = Registers [3] = 0;

	nes->o_cpu->swap_page (0x8000, 0, _16K_);
	nes->o_cpu->swap_page (0xc000, nes->o_rom->information ().prg_pages - 1, _16K_);
}

void c_mapper_068 :: write_byte (__UINT_16 address, __UINT_8 value)
{
	switch (address)
	{
		case 0x8000: nes->o_ppu->swap_page (0x0000, value & _2K_chr_mask, _2K_); break;
		case 0x9000: nes->o_ppu->swap_page (0x0800, value & _2K_chr_mask, _2K_); break;
		case 0xa000: nes->o_ppu->swap_page (0x1000, value & _2K_chr_mask, _2K_); break;
		case 0xb000: nes->o_ppu->swap_page (0x1800, value & _2K_chr_mask, _2K_); break;
		case 0xc000: Registers [2] = value; UpdateBanks (); break;
		case 0xd000: Registers [3] = value; UpdateBanks (); break;
		case 0xe000: 
			Registers [0] = (value & 0x10) >> 4;
			Registers [1] = value & 3;
			UpdateBanks ();
			break;
		case 0xf000: nes->o_cpu->swap_page (0x8000, value & _16K_prg_mask, _16K_); break;
	}
}

void c_mapper_068 :: UpdateBanks (void)
{
	if (Registers [0]) {
		switch (Registers [1]) 
		{
			case 0: 
				nes->o_ppu->swap_page (nes->o_ppu->get_nametables (), 0x0000, (Registers [2] + 0x80) & _1K_chr_mask, _1K_);
				nes->o_ppu->swap_page (nes->o_ppu->get_nametables () + 1, 0x0000, (Registers [3] + 0x80) & _1K_chr_mask, _1K_);
				nes->o_ppu->swap_page (nes->o_ppu->get_nametables () + 2, 0x0000, (Registers [2] + 0x80) & _1K_chr_mask, _1K_);
				nes->o_ppu->swap_page (nes->o_ppu->get_nametables () + 3, 0x0000, (Registers [3] + 0x80) & _1K_chr_mask, _1K_);
				break;
			case 1:
				nes->o_ppu->swap_page (nes->o_ppu->get_nametables (), 0x0000, (Registers [2] + 0x80) & _1K_chr_mask, _1K_);
				nes->o_ppu->swap_page (nes->o_ppu->get_nametables () + 1, 0x0000, (Registers [2] + 0x80) & _1K_chr_mask, _1K_);
				nes->o_ppu->swap_page (nes->o_ppu->get_nametables () + 2, 0x0000, (Registers [3] + 0x80) & _1K_chr_mask, _1K_);
				nes->o_ppu->swap_page (nes->o_ppu->get_nametables () + 3, 0x0000, (Registers [3] + 0x80) & _1K_chr_mask, _1K_);
				break;
			case 2:
				nes->o_ppu->swap_page (nes->o_ppu->get_nametables (), 0x0000, (Registers [2] + 0x80) & _1K_chr_mask, _1K_);
				nes->o_ppu->swap_page (nes->o_ppu->get_nametables () + 1, 0x0000, (Registers [2] + 0x80) & _1K_chr_mask, _1K_);
				nes->o_ppu->swap_page (nes->o_ppu->get_nametables () + 2, 0x0000, (Registers [2] + 0x80) & _1K_chr_mask, _1K_);
				nes->o_ppu->swap_page (nes->o_ppu->get_nametables () + 3, 0x0000, (Registers [2] + 0x80) & _1K_chr_mask, _1K_);
				break;
			case 3: 
				nes->o_ppu->swap_page (nes->o_ppu->get_nametables (), 0x0000, (Registers [3] + 0x80) & _1K_chr_mask, _1K_);
				nes->o_ppu->swap_page (nes->o_ppu->get_nametables () + 1, 0x0000, (Registers [3] + 0x80) & _1K_chr_mask, _1K_);
				nes->o_ppu->swap_page (nes->o_ppu->get_nametables () + 2, 0x0000, (Registers [3] + 0x80) & _1K_chr_mask, _1K_);
				nes->o_ppu->swap_page (nes->o_ppu->get_nametables () + 3, 0x0000, (Registers [3] + 0x80) & _1K_chr_mask, _1K_);
				break;
		}
	}
	else {
		switch (Registers [1])
		{
			case 0:	nes->o_ppu->set_mirroring (_2C02_HORIZONTAL_MIRRORING); break;
			case 1:	nes->o_ppu->set_mirroring (_2C02_VERTICAL_MIRRORING); break;
			case 2:	nes->o_ppu->set_mirroring (_2C02_2000_MIRRORING); break;
			case 3:	nes->o_ppu->set_mirroring (_2C02_2400_MIRRORING); break;
		}
	}
}

void c_mapper_068 :: save_state (c_tracer &o_writer)
{
	o_writer.write ((__UINT_8 *)(Registers), 8);
}

void c_mapper_068 :: load_state (c_tracer &o_reader)
{
	o_reader.read ((__UINT_8 *)(Registers), 8);
}