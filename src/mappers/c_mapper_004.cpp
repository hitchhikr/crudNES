/*******************************************************************************
    crudNES - A NES emulator for reverse engineering purposes
    MMC3 / MMC6
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
#include "../include/mappers/c_mapper_004.h"
#include "../include/c_nes.h"
#include "../include/c_ppu.h"
#include "../include/c_rom.h"
#include "../include/datatypes.h"

/******************************************************************************/
/** External Data                                                            **/
/******************************************************************************/

extern c_machine *o_machine;

/******************************************************************************/
/** Installer                                                                **/
/******************************************************************************/

c_mapper_004 :: c_mapper_004 (void)
{
	__DBG_INSTALLING ("Mapper #004");

	__DBG_INSTALLED ();
}

/******************************************************************************/
/** Uninstaller                                                              **/
/******************************************************************************/

c_mapper_004 :: ~c_mapper_004 (void)
{
	__DBG_UNINSTALLING ("Mapper #004");
	__DBG_UNINSTALLED ();
}

/******************************************************************************/
/** reset ()                                                                 **/
/******************************************************************************/

void c_mapper_004 :: reset (void)
{
	bPatternSelectionRegister = 0;

	bIRQEnabled = bSingleIRQGenerated = bIRQRequested = bNeedsReload = bOutsideUpdate = FALSE;
	iIRQCounter = iIRQCounterReload = 0;
	bControl = 0;

	b_8K_PRGPages [0] = 0;
	b_8K_PRGPages [1] = 1;
	UpdatePRGPages ();

	b_1K_CHRPages [0] = b_1K_CHRPages [1] = 0;
	b_1K_CHRPages [2] = b_1K_CHRPages [3] = 2;
	b_1K_CHRPages [4] = 4;
	b_1K_CHRPages [5] = 5;
	b_1K_CHRPages [6] = 6;
	b_1K_CHRPages [7] = 7;
}

/******************************************************************************/
/** UpdatePRGPages ()                                                        **/
/******************************************************************************/

void c_mapper_004 :: UpdatePRGPages (void)
{
	if (bControl & BIT_6)
	{
		nes->o_cpu->swap_page (0x8000, _8K_prg_mask - 1, _8K_);
		nes->o_cpu->swap_page (0xc000, b_8K_PRGPages [0] & _8K_prg_mask, _8K_);
	}
	else
	{
		nes->o_cpu->swap_page (0x8000, b_8K_PRGPages [0] & _8K_prg_mask, _8K_);
		nes->o_cpu->swap_page (0xc000, _8K_prg_mask - 1, _8K_);
	}

	nes->o_cpu->swap_page (0xa000, b_8K_PRGPages [1] & _8K_prg_mask, _8K_);
	nes->o_cpu->swap_page (0xe000, _8K_prg_mask, _8K_);
}

void c_mapper_004 :: UpdateCHRPages (void)
{
	if (bControl & BIT_7)
	{
		nes->o_ppu->swap_page (0x0000, b_1K_CHRPages [4] & _1K_chr_mask, _1K_);
		nes->o_ppu->swap_page (0x0400, b_1K_CHRPages [5] & _1K_chr_mask, _1K_);
		nes->o_ppu->swap_page (0x0800, b_1K_CHRPages [6] & _1K_chr_mask, _1K_);
		nes->o_ppu->swap_page (0x0c00, b_1K_CHRPages [7] & _1K_chr_mask, _1K_);
		nes->o_ppu->swap_page (0x1000, b_1K_CHRPages [0] & _1K_chr_mask, _1K_);
		nes->o_ppu->swap_page (0x1400, (b_1K_CHRPages [0] + 1) & _1K_chr_mask, _1K_);
		nes->o_ppu->swap_page (0x1800, b_1K_CHRPages [2] & _1K_chr_mask, _1K_);
		nes->o_ppu->swap_page (0x1c00, (b_1K_CHRPages [2] + 1) & _1K_chr_mask, _1K_);
	}
	else
	{
		nes->o_ppu->swap_page (0x0000, b_1K_CHRPages [0] & _1K_chr_mask, _1K_);
		nes->o_ppu->swap_page (0x0400, (b_1K_CHRPages [0] + 1) & _1K_chr_mask, _1K_);
		nes->o_ppu->swap_page (0x0800, b_1K_CHRPages [2] & _1K_chr_mask, _1K_);
		nes->o_ppu->swap_page (0x0c00, (b_1K_CHRPages [2] + 1) & _1K_chr_mask, _1K_);
		nes->o_ppu->swap_page (0x1000, b_1K_CHRPages [4] & _1K_chr_mask, _1K_);
		nes->o_ppu->swap_page (0x1400, b_1K_CHRPages [5] & _1K_chr_mask, _1K_);
		nes->o_ppu->swap_page (0x1800, b_1K_CHRPages [6] & _1K_chr_mask, _1K_);
		nes->o_ppu->swap_page (0x1c00, b_1K_CHRPages [7] & _1K_chr_mask, _1K_);
	}
}

/******************************************************************************/
/** write_byte ()                                                             **/
/******************************************************************************/

void c_mapper_004 :: write_byte (__UINT_16 address, __UINT_8 value)
{
	if (address < 0x8000) { nes->o_sram->write_byte (address, value); return; }

	if (0x8000 == address)
	{
		bControl = value;
		UpdatePRGPages ();
		UpdateCHRPages ();
	}

	else if (0x8001 == address)
	{
		switch (bControl & 7)
		{
            case 0: b_1K_CHRPages [0] = b_1K_CHRPages [1] = value & 0xfe; UpdateCHRPages (); break;
			case 1: b_1K_CHRPages [2] = b_1K_CHRPages [3] = value & 0xfe; UpdateCHRPages (); break;
			case 2: b_1K_CHRPages [4] = value; UpdateCHRPages (); break;
			case 3: b_1K_CHRPages [5] = value; UpdateCHRPages (); break;
			case 4: b_1K_CHRPages [6] = value; UpdateCHRPages (); break;
			case 5: b_1K_CHRPages [7] = value; UpdateCHRPages (); break;
			case 6: b_8K_PRGPages [0] = value; UpdatePRGPages (); break;
			case 7: b_8K_PRGPages [1] = value; UpdatePRGPages (); break;
			default: ;
		}
	}

	else if (address == 0xa000)
	{
		if (nes->o_rom->information ().mirroring != _2C02_FOURSCREEN_MIRRORING)
		{
			if (value & BIT_0) nes->o_ppu->set_mirroring (_2C02_HORIZONTAL_MIRRORING);
			else nes->o_ppu->set_mirroring (_2C02_VERTICAL_MIRRORING);
		}
	}

	else if (0xa001 == address)
	{
	//	nes->general_log.f_write ("bs", value, " BOO\r\n");
	}

	else if (0xc000 == address)
	{
		iIRQCounterReload = value;
	}

	else if (0xc001 == address)
	{
		bNeedsReload = TRUE;
	}

	else if (0xe000 == address)
	{
        bIRQEnabled = FALSE;
		nes->o_cpu->set_irq_line (FALSE);
	}

	else if (0xe001 == address)
	{
		bIRQEnabled = TRUE;
	}
}

/******************************************************************************/
/** update ()                                                                **/
/******************************************************************************/

void c_mapper_004 :: update (void *vData)
{
	bOutsideUpdate = TRUE;
	h_blank ();
}

/******************************************************************************/
/** h_blank ()                                                                **/
/******************************************************************************/

void c_mapper_004 :: h_blank (void)
{
	if (nes->o_ppu->information ().scanline != nes->o_cpu->get_last_line () && nes->o_ppu->information ().scanline > 240) goto lblDone; 

	if (bNeedsReload) { iIRQCounter = iIRQCounterReload; bNeedsReload = FALSE; }

	//TODO: Check whether this works properly for all games or not
	else if ((iIRQCounter
		    && ((!nes->o_ppu->information ().bg_pattern_base && nes->o_ppu->information ().sp_pattern_base) || nes->o_ppu->get_flag (CTL_1, BIT_5))
		    && (nes->o_ppu->get_flag (CTL_2, BIT_3 | BIT_4) || bOutsideUpdate)
			))
	{
		iIRQCounter --;
	}
	
	if (!iIRQCounter)
	{
		bNeedsReload = TRUE;
		if (bIRQEnabled)
		{
			nes->o_cpu->request_irq ();
		}
	}

lblDone:

	bOutsideUpdate = FALSE; 
}

void c_mapper_004 :: save_state (c_tracer &o_writer)
{
	o_writer.write (&bPatternSelectionRegister, 1);
	o_writer.write (&bControl, 1);
	o_writer.write ((__UINT_8 *)(&iIRQCounter), 2);
	o_writer.write ((__UINT_8 *)(&iIRQCounterReload), 2);
	o_writer.write ((__UINT_8 *)(&bIRQEnabled), 1);
	o_writer.write ((__UINT_8 *)(&bIRQRequested), 1);
	o_writer.write ((__UINT_8 *)(&bNeedsReload), 1);
	o_writer.write ((__UINT_8 *)(b_8K_PRGPages), 8);
	o_writer.write ((__UINT_8 *)(b_1K_CHRPages), 16);
}

void c_mapper_004 :: load_state (c_tracer &o_reader)
{
	o_reader.read (&bPatternSelectionRegister, 1);
	o_reader.read (&bControl, 1);
	o_reader.read ((__UINT_8 *)(&iIRQCounter), 2);
	o_reader.read ((__UINT_8 *)(&iIRQCounterReload), 2);
	o_reader.read ((__UINT_8 *)(&bIRQEnabled), 1);
	o_reader.read ((__UINT_8 *)(&bIRQRequested), 1);
	o_reader.read ((__UINT_8 *)(&bNeedsReload), 1);
	o_reader.read ((__UINT_8 *)(b_8K_PRGPages), 8);
	o_reader.read ((__UINT_8 *)(b_1K_CHRPages), 16);
}
