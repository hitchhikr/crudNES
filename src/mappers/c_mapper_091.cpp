/*******************************************************************************
    crudNES - A NES emulator for reverse engineering purposes
    HK-SF3
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
    int i;
    int start = 0;
    int old_start = 0;
	int alias = 0;
	int bank = 0;
    s_label_node *pages = NULL;

	__DBG_INSTALLING ("Mapper #091");

	max_code_pages = 0;

    pages = pages->create_page(pages, bank, 0x8000, _8K_, start, bank, bank, alias, alias * _8K_);
    nes->prg_pages = pages;
    pages = pages->create_page(pages, bank, 0xa000, _8K_, start, bank, bank, alias + 1, (alias + 1) * _8K_);
	alias += 4;
	bank++;
    max_code_pages += 2;
    start += _8K_;
    start += _8K_;
    for(i = 1; i < (int) ((nes->o_rom->information ().prg_pages) / 2); i++)
    {
        pages = pages->create_page(pages, bank, 0x8000, _8K_, start, bank, bank, alias, alias * _8K_);
        pages = pages->create_page(pages, bank, 0xa000, _8K_, start, bank, bank, alias + 1, (alias + 1) * _8K_);
		alias += 4;
        start += _8K_;
        start += _8K_;
		bank++;
		max_code_pages += 2;
    }

	last_prg_page = bank;
	max_alias = ((nes->o_rom->information ().prg_pages - 1) * _16K_) >> 12;
	pages->create_page(pages, bank, 0xc000, _16K_, 0xc000, bank, bank, ((nes->o_rom->information ().prg_pages - 1) * _16K_) >> 12, (nes->o_rom->information ().prg_pages - 1) * _16K_);
    start = ((nes->o_rom->information ().prg_pages - 1) * _16K_) + _16K_;
    max_code_pages++;

    // Chr pages
	alias = 0;
	bank = 0;
	old_start = start;
	pages = NULL;// 64 pages de 2k
	pages = pages->create_page(pages, bank, 0x0000, _2K_, start, bank, bank, bank, start);
	nes->chr_pages = pages;
	bank++;
	start += _2K_;
	pages = pages->create_page(pages, bank, 0x0800, _2K_, start, bank, bank, bank, start);
	bank++;
	start += _2K_;
	pages = pages->create_page(pages, bank, 0x1000, _2K_, start, bank, bank, bank, start);
	bank++;
	start += _2K_;
	pages = pages->create_page(pages, bank, 0x1800, _2K_, start, bank, bank, bank, start);
	bank++;
	start += _2K_;
	max_pages++;
	for(i = 1; i < (int) nes->o_rom->information().chr_pages; i++)
	{
		pages = pages->create_page(pages, bank, 0x0000, _2K_, start, bank, bank, bank, start);
		bank++;
		start += _2K_;
		pages = pages->create_page(pages, bank, 0x0800, _2K_, start, bank, bank, bank, start);
		bank++;
		start += _2K_;
		pages = pages->create_page(pages, bank, 0x1000, _2K_, start, bank, bank, bank, start);
		bank++;
		start += _2K_;
		pages = pages->create_page(pages, bank, 0x1800, _2K_, start, bank, bank, bank, start);
		bank++;
		start += _2K_;
		max_pages++;
	}

	__DBG_INSTALLED ();
}

c_mapper_091 :: ~c_mapper_091 (void)
{
	__DBG_UNINSTALLING ("Mapper #091");
	__DBG_UNINSTALLED ();
}

void c_mapper_091 :: reset (void)
{
	bIRQEnabled = TRUE;
	iIRQCounter = 0;

	last_page_switched = last_prg_page;
	last_page_switched_8000 = 0;
	last_page_switched_a000 = 0;
	nes->o_cpu->swap_page (0x8000, 0, _8K_);
	nes->o_cpu->swap_page (0xa000, 1, _8K_);
	nes->o_cpu->swap_page (0xc000, nes->o_rom->information ().prg_pages - 1, _16K_);

	nes->o_ppu->swap_page (0x0000, 0, _2K_);
	nes->o_ppu->swap_page (0x0800, _2K_chr_mask, _2K_);
	nes->o_ppu->swap_page (0x1000, _2K_chr_mask * 2, _2K_);
	nes->o_ppu->swap_page (0x1800, _2K_chr_mask * 3, _2K_);
}

void c_mapper_091 :: write_byte (__UINT_16 address, __UINT_8 value)
{
	switch (address)
	{
		case 0x6000:
			nes->o_ppu->swap_page (0x0000, value & _2K_chr_mask, _2K_);
			break;
		case 0x6001:
			nes->o_ppu->swap_page (0x0800, value & _2K_chr_mask, _2K_);
			break;
		case 0x6002:
			nes->o_ppu->swap_page (0x1000, value & _2K_chr_mask, _2K_);
			break;
		case 0x6003:
			nes->o_ppu->swap_page (0x1800, value & _2K_chr_mask, _2K_);
			break;
		case 0x6006:
			iIRQCounter &= 0xff00;
			iIRQCounter |= value;
			break;
		case 0x6007:
			iIRQCounter &= 0x00ff;
			iIRQCounter |= (value << 8);
			break;
		case 0x7000:
			nes->o_cpu->swap_page (0x8000, value & _8K_prg_mask, _8K_);
			last_page_switched_8000 = value & _8K_prg_mask;
			break;
		case 0x7001:
			nes->o_cpu->swap_page (0xa000, value & _8K_prg_mask, _8K_);
			last_page_switched_a000 = value & _8K_prg_mask;
			break;
		case 0x7002:
		case 0x7003:
		case 0x7006:
			//iIRQCounter = 0; 
			bIRQEnabled = FALSE;
			nes->o_cpu->set_irq_line (FALSE);
			break;
		case 0x7007:
			iIRQCounter = 0;
			bIRQEnabled = TRUE;
			break;
	}
}

void c_mapper_091 :: h_blank (void)
{
	if (nes->o_ppu->information ().scanline > 240) return;

	if (iIRQCounter < 8 && bIRQEnabled)
	{
		iIRQCounter++;
		if (iIRQCounter >= 8)
		{
			nes->o_cpu->request_irq ();
//*			nes->o_cpu->set_irq_line (TRUE);
			iIRQCounter = 0;
		}
	}
}
