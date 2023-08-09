/*******************************************************************************
    crudNES - A NES emulator for reverse engineering purposes
 
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

#include <stdio.h>
#include <string.h>

#include "../include/c_cpu.h"
#include "../include/mappers/c_mapper.h"
#include "../include/c_nes.h"
#include "../include/c_ppu.h"
#include "../include/c_rom.h"
#include "../include/c_tracer.h"
#include "../include/datatypes.h"

/******************************************************************************/
/** External Data                                                            **/
/******************************************************************************/

extern c_machine *o_machine;

/******************************************************************************/
/** Installer                                                                **/
/******************************************************************************/

c_mapper :: c_mapper (void)
{
	__DBG_INSTALLING ("Mapper");

	_1K_chr_mask = (nes->o_rom->information ().chr_pages << 3) - 1;
	_2K_chr_mask = (nes->o_rom->information ().chr_pages << 2) - 1;
	_4K_chr_mask = (nes->o_rom->information ().chr_pages << 1) - 1;
	_8K_chr_mask = nes->o_rom->information ().chr_pages - 1;
	_8K_prg_mask = (nes->o_rom->information ().prg_pages << 1) - 1;
	_16K_prg_mask = nes->o_rom->information ().prg_pages - 1;
    _32K_prg_mask = (nes->o_rom->information ().prg_pages >> 1) - 1;

	last_page_switched = 0;
	vectors_address = 0xfffa;
	max_pages = 0;

	__DBG_INSTALLED ();
}

/******************************************************************************/
/** Uninstaller                                                              **/
/******************************************************************************/

c_mapper :: ~c_mapper (void)
{
	__DBG_UNINSTALLING ("Mapper");
	__DBG_UNINSTALLED ();
}

/******************************************************************************/
/** reset                                                                    **/
/******************************************************************************/

void c_mapper :: reset (void)
{
	nes->o_cpu->swap_page (0x8000, 0, _16K_);
	nes->o_cpu->swap_page (0xc000, nes->o_rom->information ().prg_pages - 1, _16K_);

	nes->o_ppu->swap_page (0x0000, 0, _8K_);
}

/******************************************************************************/
/** write_byte                                                                **/
/******************************************************************************/

void c_mapper :: write_byte (__UINT_16 address, __UINT_8 value)
{
	if (nes->o_rom->information ().o_sram) nes->o_sram->write_byte (address, value);
}

__UINT_8 c_mapper :: read_byte (__UINT_16 address)
{
	return 0;
}

void c_mapper :: update (void *vData)
{
}

void c_mapper :: h_blank (void)
{
}

void c_mapper :: save_state (c_tracer &o_writer)
{
}

void c_mapper :: load_state (c_tracer &o_reader)
{
}

void c_mapper :: set_vectors()
{
    if (_2A03_labelHolder)
	{
	    nes->BankJMPList->insert_label(vectors_address, TYPE_DATA, TYPE_WORD, 0, vectors_address);
	    nes->BankJMPList->insert_label(vectors_address + 2, TYPE_DATA, TYPE_WORD, 0, 0);
	    nes->BankJMPList->insert_label(vectors_address + 4, TYPE_DATA, TYPE_WORD, 0, 0);
	    nes->BankJMPList->insert_label(NMIAddr, TYPE_CODE, TYPE_CODE, 0, 0);
	    nes->BankJMPList->insert_label(resetAddr, TYPE_CODE, TYPE_CODE, 0, 0);
	    nes->BankJMPList->insert_label(IRQAddr, TYPE_CODE, TYPE_CODE, 0, 0);
    }
}
