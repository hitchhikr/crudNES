/*******************************************************************************
    crudNES - A NES emulator for reverse engineering purposes
    No mapper
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
#include "../include/mappers/c_mapper_000.h"
#include "../include/mappers/c_mapper.h"
#include "../include/c_cpu.h"
#include "../include/c_tracer.h"
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

c_mapper_000 :: c_mapper_000 (void)
{
    int i;
    int start = 0;
	__DBG_INSTALLING ("Mapper #000");

    // No mapping there's only a 16k prg page at c000
    // (mirrored at 8000 if the ROM is 24k)
    // and a chr page of 8k)

    s_label_node *pages = NULL;
    
    if(nes->o_rom->information().prg_pages == 1)
    {
        if(nes->o_rom->information().mirroring)
        {
            pages = pages->Create(pages, 0xc000, _16K_, start, 0, 1, 0);
            nes->prg_pages = pages;
        }
        else
        {
            // Binary land starts at 0xc000 and Exerion at 0x8000
            // we'll probably need crc32 checking.
            pages = pages->Create(pages, 0x8000, _16K_, start, 0, 1, 0);
        }
        nes->prg_pages = pages;
        start += _16K_;
        max_pages++;
    }
    else
    {
        // 32k
        pages = pages->Create(pages, 0x8000, _32K_, start, 0, 1, 0);
        nes->prg_pages = pages;
        start += _32K_;
        max_pages += 2;
    }

    // Chr pages
    pages = NULL;
    pages = pages->Create(pages, 0x0000, _8K_, start, 0, 0, 0);
    max_pages++;
    start += _8K_;
    nes->chr_pages = pages;
    for(i = 1; i < (int) nes->o_rom->information().chr_pages; i++)
    {
        pages = pages->Create(pages, 0x0000, _8K_, start, i, i, i);
        max_pages++;
        start += _8K_;
    }

	__DBG_INSTALLED ();
}

/******************************************************************************/
/** Uninstaller                                                              **/
/******************************************************************************/

c_mapper_000 :: ~c_mapper_000 (void)
{
	__DBG_UNINSTALLING ("Mapper #000");
	__DBG_UNINSTALLED ();
}

/******************************************************************************/
/** reset                                                                    **/
/******************************************************************************/

void c_mapper_000 :: reset (void)
{
	nes->o_cpu->swap_page (0x8000, 0, _16K_);
	nes->o_cpu->swap_page (0xc000, nes->o_rom->information ().prg_pages - 1, _16K_);

	nes->o_ppu->swap_page (0x0000, 0, _8K_);

    // Vectors will be located here as the page is not mirrorred
    if(nes->o_rom->information().prg_pages == 1 && !nes->o_rom->information().mirroring)
    {
        vectors_address = 0xbffa;
    }       
}

/******************************************************************************/
/** write_byte                                                                **/
/******************************************************************************/

void c_mapper_000 :: write_byte (__UINT_16 address, __UINT_8 value)
{
	if (nes->o_rom->information ().o_sram) nes->o_sram->write_byte (address, value);
}
