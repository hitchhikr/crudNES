/*******************************************************************************
    udNES - A NES emulator for reverse engineering purposes
    MMC4
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
#include "../include/mappers/c_mapper_010.h"
#include "../include/c_nes.h"
#include "../include/c_ppu.h"
#include "../include/c_rom.h"
#include "../include/datatypes.h"

/******************************************************************************/
/** External Data                                                            **/
/******************************************************************************/

extern c_machine *o_machine;

c_mapper_010 :: c_mapper_010 (void)
{
    int i;
	int alias = 0;
    int start = 0;
	s_label_node *pages = NULL;

	__DBG_INSTALLING ("Mapper #010");

    // All pages start at 0x8000
    // except the last one which starts at 0xc000
    pages = pages->create_page(pages, 0, 0x8000, _16K_, start, 0, 0, alias, 0);
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
    pages->create_page(pages, i, 0xc000, _16K_, start, i, i, alias, start);
    max_pages++;
	max_alias = alias;

    // Chr pages
    pages = NULL;
    pages = pages->create_page(pages, 0, 0x0000, _4K_, start, 0, 0, 0, start);
    max_pages++;
    start += _4K_;
    nes->chr_pages = pages;
    for(i = 1; i < (int) nes->o_rom->information().chr_pages; i++)
    {
        pages = pages->create_page(pages, i, 0x0000, _4K_, start, i, i, i, start);
        max_pages++;
        start += _4K_;
    }

/*    // Chr pages
    pages = NULL;
    pages = pages->create_page(pages, 0, 0x0000, _4K_, start, 0, 0, 0, start);
    nes->chr_pages = pages;
    max_pages++;
    start += _4K_;
    pages = pages->create_page(pages, 1, 0x0000, _4K_, start, 1, 1, 1, start);
    max_pages++;
    start += _4K_;
    pages = pages->create_page(pages, 2, 0x1000, _4K_, start, 2, 2, 2, start);
    max_pages++;
    start += _4K_;
    pages = pages->create_page(pages, 3, 0x1000, _4K_, start, 3, 3, 3, start);
    max_pages++;
    start += _4K_;
*/
	__DBG_INSTALLED ();
}

c_mapper_010 :: ~c_mapper_010 (void)
{
	__DBG_UNINSTALLING ("Mapper #010");
	__DBG_UNINSTALLED ();
}

void c_mapper_010 :: reset (void)
{
	last_page_switched = nes->o_rom->information ().prg_pages - 1;
	last_page_switched_8000 = 0;
	nes->o_cpu->swap_page (0x8000, 0, _16K_);
	nes->o_cpu->swap_page (0xc000, nes->o_rom->information ().prg_pages - 1, _16K_);

/*	nes->o_cpu->swap_page (0x2000, _8K_prg_mask - 2, _8K_);     // 0x2000-0x3fff
	nes->o_cpu->swap_page (0x6000, _8K_prg_mask, _8K_);         // 0x6000-0x7fff
	nes->o_cpu->swap_page (0x8000, 0, _8K_);                    // 0x8000-0x9fff
	nes->o_cpu->swap_page (0xc000, _8K_prg_mask - 1, _8K_);     // 0xC000-0xdfff*/
//	nes->o_cpu->swap_page (0x8000, 0, _16K_);
//	nes->o_cpu->swap_page (0xc000, nes->o_rom->information ().prg_pages - 1, _16K_);

/*	nes->o_cpu->swap_page (0x2000, _8K_prg_mask - 2, _8K_);     // 0x2000-0x3fff
	nes->o_cpu->swap_page (0x6000, _8K_prg_mask, _8K_);         // 0x6000-0x7fff
	nes->o_cpu->swap_page (0x8000, 0, _16K_);                   // 0x8000-0x9fff
	nes->o_cpu->swap_page (0xc000, nes->o_rom->information ().prg_pages - 1, _16K_);
*/
	bLatches[0] = 0xfe;
	bLatches[1] = 0xfe;

	pages[0] = 0;
	pages[1] = 0;
	pages[2] = 0;
	pages[3] = 0;

	nes->o_ppu->swap_page (0x0000, 0, _4K_); 
	nes->o_ppu->swap_page (0x1000, 0, _4K_); 
}

void c_mapper_010 :: update (void *vData)
{
	__UINT_16 uiSelector = (*((__UINT_16 *) (vData))) & 0x1fff;

	if (uiSelector == 0x0fd0 &&
	    bLatches [0] != 0xfd)
	{
		bLatches [0] = 0xfd;
		nes->o_ppu->swap_page (0x0000, pages [0], _4K_); 
	}
	else if (uiSelector == 0x0fe0 &&
	         bLatches [0] != 0xfe)
	{
		bLatches [0] = 0xfe;
		nes->o_ppu->swap_page (0x0000, pages [1], _4K_); 
	}

	else if (uiSelector == 0x1fd0 &&
	         bLatches [1] != 0xfd)
	{
		bLatches [1] = 0xfd;
		nes->o_ppu->swap_page (0x1000, pages [2], _4K_); 
	}

	else if (uiSelector == 0x1fe0 &&
	         bLatches [1] != 0xfe)
	{
		bLatches [1] = 0xfe;
		nes->o_ppu->swap_page (0x1000, pages [3], _4K_); 
	}
}

void c_mapper_010 :: write_byte (__UINT_16 address, __UINT_8 value)
{
	switch (address & 0xf000)
	{
	    case 0xa000:
		    nes->o_cpu->swap_page (0x8000, value & _16K_prg_mask, _16K_);
			last_page_switched_8000 = value & _16K_prg_mask;
		    break;

	    case 0xb000:
		    pages [0] = value & _4K_chr_mask;
		    if (bLatches [0] == 0xfd) nes->o_ppu->swap_page (0x0000, pages [0], _4K_); 
		    break;
	    case 0xc000:
		    pages [1] = value & _4K_chr_mask;
		    if (bLatches [0] == 0xfe) nes->o_ppu->swap_page (0x0000, pages [1], _4K_); 
		    break;
	    case 0xd000:
		    pages [2] = value & _4K_chr_mask;
		    if (bLatches [1] == 0xfd) nes->o_ppu->swap_page (0x1000, pages [2], _4K_); 
		    break;
	    case 0xe000:
		    pages [3] = value & _4K_chr_mask;
		    if (bLatches [1] == 0xfe) nes->o_ppu->swap_page (0x1000, pages [3], _4K_); 
		    break;

	    case 0xf000:
		    if (value & BIT_0) nes->o_ppu->set_mirroring (_2C02_HORIZONTAL_MIRRORING);
		    else nes->o_ppu->set_mirroring (_2C02_VERTICAL_MIRRORING);
	}
}

void c_mapper_010 :: save_state (c_tracer &o_writer)
{
	o_writer.write ((__UINT_8 *)(bLatches), 4);
	o_writer.write ((__UINT_8 *)(pages), 8);
}

void c_mapper_010 :: load_state (c_tracer &o_reader)
{
	o_reader.read ((__UINT_8 *)(bLatches), 4);
	o_reader.read ((__UINT_8 *)(pages), 8);
}
