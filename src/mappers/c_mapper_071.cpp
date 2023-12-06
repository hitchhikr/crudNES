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
    int i;
	int alias = 0;
    int start = 0;
    s_label_node *pages = NULL;

	__DBG_INSTALLING ("Mapper #071");

    // All pages start at 0x8000
    // except the last one which starts at 0xc000
    // No chr pages
    pages = pages->create_page(pages, 0, 0x8000, _16K_, start, 0, 0, 0, 0);
    max_pages++;
	alias += 4;
    start += _16K_;
    nes->prg_pages = pages;
    for(i = 1; i < (int) nes->o_rom->information ().prg_pages - 1; i++)
    {
        pages = pages->create_page(pages, i, 0x8000, _16K_, start, i, i, alias, start);
		alias += 4;
        max_pages++;
        start += _16K_;
    }
	last_prg_page = max_pages;
	max_alias = alias;
    pages->create_page(pages, i, 0xc000, _16K_, start, i, i, alias, start);
    max_pages++;

	__DBG_INSTALLED ();
}

c_mapper_071 :: ~c_mapper_071 (void)
{
	__DBG_UNINSTALLING ("Mapper #071");
	__DBG_UNINSTALLED ();
}

void c_mapper_071 :: reset (void)
{
	last_page_switched = last_prg_page;
	last_page_switched_8000 = 0;
	nes->o_cpu->swap_page (0x8000, 0, _16K_);
	nes->o_cpu->swap_page (0xc000, last_prg_page, _16K_);
}

void c_mapper_071 :: write_byte (__UINT_16 address, __UINT_8 value)
{
	if (address < 0xc000) 
	{
		if (0x9000 == (address & 0xf000))
		{
			if (!(value & BIT_4)) nes->o_ppu->set_mirroring (_2C02_2400_MIRRORING);
			else nes->o_ppu->set_mirroring (_2C02_2000_MIRRORING);
		}
	}
	else if (address > 0xbfff)
	{
		nes->o_cpu->swap_page (0x8000, value & _16K_prg_mask, _16K_);
		last_page_switched_8000 = value & _16K_prg_mask;
	}
}
