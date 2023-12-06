/*******************************************************************************
    crudNES - A NES emulator for reverse engineering purposes
    AOROM
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

#include "../include/datatypes.h"
#include "../include/mappers/c_mapper_007.h"

#include "../include/c_cpu.h"
#include "../include/c_tracer.h"
#include "../include/mappers/c_mapper.h"
#include "../include/c_nes.h"
#include "../include/c_ppu.h"
#include "../include/c_rom.h"

/******************************************************************************/
/** External Data                                                            **/
/******************************************************************************/

extern c_machine *o_machine;

/******************************************************************************/
/** Installer                                                                **/
/******************************************************************************/

c_mapper_007 :: c_mapper_007 (void)
{
    int i;
	int alias = 0;
    int start = 0;
    s_label_node *pages = NULL;

	__DBG_INSTALLING ("Mapper #007");

    // All 32k pages start at 0x8000
    // No chr pages
    pages = pages->create_page(pages, 0, 0x8000, _32K_, start, 0, 0, 0, 0);
    max_pages++;
	alias += 8;
    start += _32K_;
    nes->prg_pages = pages;
    for(i = 1; i < (int) nes->o_rom->information ().prg_pages - 1; i++)
    {
        pages = pages->create_page(pages, i, 0x8000, _32K_, start, i, i, alias, start);
		alias += 8;
        max_pages++;
        start += _32K_;
		if(start >= (int) nes->o_rom->ROM.get_size()) break;
    }
	last_prg_page = max_pages - 1;
	max_alias = alias - 8;

	__DBG_INSTALLED ();
}

/******************************************************************************/
/** Uninstaller                                                              **/
/******************************************************************************/

c_mapper_007 :: ~c_mapper_007 (void)
{
	__DBG_UNINSTALLING ("Mapper #007");
	__DBG_UNINSTALLED ();
}

/******************************************************************************/
/** reset                                                                    **/
/******************************************************************************/

void c_mapper_007 :: reset (void)
{
	nes->o_cpu->swap_page (0x8000, 0, _32K_);
	last_page_switched = 0;
}

/******************************************************************************/
/** write_byte ()                                                             **/
/******************************************************************************/

void c_mapper_007 :: write_byte (__UINT_16 address, __UINT_8 value)
{
	if (address < 0x8000)
	{
		if (nes->o_rom->information ().o_sram) nes->o_sram->write_byte (address, value);
	}
	else
	{
		nes->o_cpu->swap_page (0x8000, value & _32K_prg_mask, _32K_);
		last_page_switched = value & _32K_prg_mask;
	
		if (value & BIT_4) nes->o_ppu->set_mirroring (_2C02_2400_MIRRORING);
		else nes->o_ppu->set_mirroring (_2C02_2000_MIRRORING);
	}
}
