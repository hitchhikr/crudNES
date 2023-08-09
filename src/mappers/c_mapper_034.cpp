/*******************************************************************************
    crudNES - A NES emulator for reverse engineering purposes
    NINA-1
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
#include "../include/mappers/c_mapper_034.h"
#include "../include/c_nes.h"
#include "../include/c_ppu.h"
#include "../include/c_rom.h"
#include "../include/datatypes.h"

/******************************************************************************/
/** External Data                                                            **/
/******************************************************************************/

extern c_machine *o_machine;

c_mapper_034 :: c_mapper_034 (void)
{
    int i;
    int start = 0;

	__DBG_INSTALLING ("Mapper #034");


    // No mapping there's only a 16k prg page at c000
    // (mirrored at 8000 if the ROM is 24k)
    // and a chr page of 8k)

    s_label_node *pages = NULL;
    
    // 32k
    pages = pages->Create(pages, 0x8000, _32K_, start, 0, 0, 0);
    nes->prg_pages = pages;
    start += _32K_;
    max_pages++;
    for(i = 1; i < (int) nes->o_rom->information().prg_pages; i++)
    {
        pages = pages->Create(pages, 0x8000, _32K_, start, i, i, i);
        max_pages++;
        start += _32K_;
    }

    // Chr pages
    if(nes->o_rom->information().chr_pages)
    {
        pages = NULL;
        pages = pages->Create(pages, 0x0000, _4K_, start, 0, 0, 0);
        max_pages++;
        start += _4K_;
        nes->chr_pages = pages;
        for(i = 1; i < (int) nes->o_rom->information().chr_pages; i++)
        {
            pages = pages->Create(pages, 0x0000, _4K_, start, i, i, i);
            max_pages++;
            start += _4K_;
        }
    }

	__DBG_INSTALLED ();
}

c_mapper_034 :: ~c_mapper_034 (void)
{
	__DBG_UNINSTALLING ("Mapper #034");
	__DBG_UNINSTALLED ();
}

void c_mapper_034 :: reset (void)
{
	nes->o_cpu->swap_page (0x8000, 0, _32K_);
	nes->o_ppu->swap_page (0x0000, 0, _8K_);
}

void c_mapper_034 :: write_byte (__UINT_16 address, __UINT_8 value)
{
	if (address < 0x8000) nes->o_sram->write_byte (address, value);

	if (0x7ffd == address)
	{
	    nes->o_cpu->swap_page (0x8000, value & _32K_prg_mask, _32K_);
        last_page_switched = value & _32K_prg_mask;
        set_vectors();
	}
	else if (0x7ffe == address) nes->o_ppu->swap_page (0x0000, value & _4K_chr_mask, _4K_);
	else if (0x7fff == address) nes->o_ppu->swap_page (0x1000, value & _4K_chr_mask, _4K_);
	else if (address > 0x7fff)
	{
	    nes->o_cpu->swap_page (0x8000, value & _32K_prg_mask, _32K_);
        last_page_switched = value & _32K_prg_mask;
        set_vectors();
    }
}
