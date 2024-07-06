/*******************************************************************************
    crudNES - A NES emulator for reverse engineering purposes
    Color Dreams 
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

c_mapper_011 :: c_mapper_011 (void)
{
    int i;
	int alias = 0;
    int start = 0;
    s_label_node *pages = NULL;

	__DBG_INSTALLING ("Mapper #011");

    // All 32k pages start at 0x8000
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
    }
	max_alias = alias - 8;

    // Chr pages
    pages = NULL;
    pages = pages->create_page(pages, 0, 0x0000, _8K_, start, 0, 0, 0, start);
    max_pages++;
    start += _8K_;
    nes->chr_pages = pages;
    for(i = 1; i < (int) nes->o_rom->information().chr_pages; i++)
    {
        pages = pages->create_page(pages, i, 0x0000, _8K_, start, i, i, i, start);
        max_pages++;
        start += _8K_;
    }

	__DBG_INSTALLED ();
}

c_mapper_011 :: ~c_mapper_011 (void)
{
	__DBG_UNINSTALLING ("Mapper #011");
	__DBG_UNINSTALLED ();
}

void c_mapper_011 :: reset (void)
{
	last_page_switched = 0;

	nes->o_cpu->swap_page (0x8000, 0, _32K_);
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
		nes->o_cpu->swap_page (0x8000, value & 3, _32K_);
		last_page_switched = value & 3;
		nes->o_ppu->swap_page (0x0000, value >> 4, _8K_);
	}
}
