/*******************************************************************************
    crudNES - A NES emulator for reverse engineering purposes
    MMC5
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
#include "../include/mappers/c_mapper_005.h"
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

c_mapper_005 :: c_mapper_005 (void)
{
	__DBG_INSTALLING ("Mapper #005");

	ExRam.resize (_1K_);
	Fillnametable.resize (_1K_);

	__DBG_INSTALLED ();
}

/******************************************************************************/
/** Uninstaller                                                              **/
/******************************************************************************/

c_mapper_005 :: ~c_mapper_005 (void)
{
	__DBG_UNINSTALLING ("Mapper #005");
	__DBG_UNINSTALLED ();
}

/******************************************************************************/
/** reset ()                                                                 **/
/******************************************************************************/

void c_mapper_005 :: reset (void)
{
	nes->o_cpu->swap_page (0xc000, _16K_prg_mask, _16K_);

	for (__UINT_32 uiIndex = 0; uiIndex < 4; uiIndex ++)
	{
		ExtraBackground [uiIndex] = NULL;
    }
}

/******************************************************************************/
/** write_byte ()                                                             **/
/******************************************************************************/

void c_mapper_005 :: write_byte (__UINT_16 address, __UINT_8 value)
{
	//if (address > 0x7fff) { nes->general_log.f_write ("swsbs", "Writing data at: ", address, " Data: ", value, ".\r\n"); return; }
	if (address > 0x5fff && bIsSRamEnabled) { nes->o_sram->write_byte (address, value); return; }
	else if (address > 0x5bff && bExRamUsage != 3)
	{
		if (bExRamUsage & 2) ExRam.write_byte (address, value);
		return;
	}

	switch (address)
	{
		//This register controls the PRG bank-switch mode
		case 0x5100: bBankSwitch = value & 3; break;
		case 0x5101: bChrBankSwitch = value & 3; break;

		case 0x5102:
		case 0x5103:
			bSRamProtected [address & 1] = value & 3;
			if ((2 == bSRamProtected [0]) && (1 == bSRamProtected [1])) bIsSRamEnabled = TRUE;
			break;

		case 0x5104:
			bExRamUsage = value & 3;
			break;

		case 0x5105:
			bMirroring [0] = value & 3;
			bMirroring [1] = (value >> 2) & 3;
			bMirroring [2] = (value >> 4) & 3;
			bMirroring [3] = (value >> 6) & 3;

			set_mirroring ();
			break;

		case 0x5106:
			Fillnametable.clear_to (0x000, value, 0x3c0);
			break;

		case 0x5107:
			value &= 3;
			value |= value << 2;
			value |= value << 4;
			Fillnametable.clear_to (0x3c0, value, 0x40);
			break;

		case 0x5113:
			//nes->general_log.f_write ("s", "Changing RAM banks.\r\n");
			break;

		case 0x5114:
			//if (!(value & BIT_7)) nes->general_log.f_write ("sws", "RAM: ", address, "\r\n");
			switch (bBankSwitch)
			{
				case 3: nes->o_cpu->swap_page (0x8000, (value & 0x7f) & _8K_prg_mask, _8K_); break;				
			}
			break;

		case 0x5115:
			//if (!(value & BIT_7)) nes->general_log.f_write ("sws", "RAM: ", address, "\r\n");
			switch (bBankSwitch)
			{
				case 1:
				case 2: nes->o_cpu->swap_page (0x8000, ((value >> 1) & 0x3f) & _16K_prg_mask, _16K_); break;
				case 3: nes->o_cpu->swap_page (0xa000, (value & 0x7f) & _8K_prg_mask, _8K_); break;				
			}
			break;

		case 0x5116:
			//if (!(value & BIT_7)) nes->general_log.f_write ("sws", "RAM: ", address, "\r\n");
			switch (bBankSwitch)
			{
				case 2:
				case 3: nes->o_cpu->swap_page (0xc000, (value & 0x7f) & _8K_prg_mask, _8K_); break;				
			}
			break;

		case 0x5117:
			if (!(value & BIT_7)) nes->general_log.f_write ("sws", "RAM: ", address, "\r\n");
			switch (bBankSwitch)
			{
				case 0: if (nes->o_rom->information ().prg_pages > 1) nes->o_cpu->swap_page (0x8000, ((value >> 2) & 0x1f) & _32K_prg_mask, _32K_); break;
				case 1: nes->o_cpu->swap_page (0xc000, ((value >> 1) & 0x3f) & _16K_prg_mask, _16K_); break;
				case 2: case 3: nes->o_cpu->swap_page (0xe000, (value & 0x7f) & _8K_prg_mask, _8K_); break;				
			}
			break;

		case 0x5120:
			if (3 == bChrBankSwitch) nes->o_ppu->swap_page (0x0000, value & _1K_chr_mask, _1K_);
			break;

		case 0x5121:
			switch (bChrBankSwitch)
			{
				case 2: nes->o_ppu->swap_page (0x0000, value & _2K_chr_mask, _2K_); break;
				case 3: nes->o_ppu->swap_page (0x0400, value & _1K_chr_mask, _1K_); break;
			}
			break;

		case 0x5122:
			if (3 == bChrBankSwitch) nes->o_ppu->swap_page (0x0800, value & _1K_chr_mask, _1K_);
			break;

		case 0x5123:
			switch (bChrBankSwitch)
			{
				case 1: nes->o_ppu->swap_page (0x0000, value & _4K_chr_mask, _4K_); break;
				case 2: nes->o_ppu->swap_page (0x0800, value & _2K_chr_mask, _2K_); break;
				case 3: nes->o_ppu->swap_page (0x0c00, value & _1K_chr_mask, _1K_); break;
			}
			break;

		case 0x5124:
			if (3 == bChrBankSwitch) nes->o_ppu->swap_page (0x1000, value & _1K_chr_mask, _1K_);
			break;

		case 0x5125:
			switch (bChrBankSwitch)
			{
				case 2: nes->o_ppu->swap_page (0x1000, value & _2K_chr_mask, _2K_); break;
				case 3: nes->o_ppu->swap_page (0x1400, value & _1K_chr_mask, _1K_); break;
			}
			break;

		case 0x5126:
			if (3 == bChrBankSwitch) nes->o_ppu->swap_page (0x1800, value & _1K_chr_mask, _1K_);
			break;

		case 0x5127:
			switch (bChrBankSwitch)
			{
				case 0: nes->o_ppu->swap_page (0x0000, value & _8K_chr_mask, _8K_); break;
				case 1: nes->o_ppu->swap_page (0x1000, value & _4K_chr_mask, _4K_); break;
				case 2: nes->o_ppu->swap_page (0x1800, value & _2K_chr_mask, _2K_); break;
				case 3: nes->o_ppu->swap_page (0x1c00, value & _1K_chr_mask, _1K_); break;
			}
			break;

		case 0x5130:
			//nes->general_log.f_write ("sbs", "Upper bits: ", value & 3, "\r\n");
			break;

		case 0x5203:
			iIrqCounter = value - 1;
			break;

		case 0x5204:
			bIsIrqEnabled = value & BIT_7;
			break;

		case 0x5205: 
		case 0x5206:
			iMultipliers [address & 1] = value;
			iProduct = iMultipliers [0] * iMultipliers [1];
			break;

		default:
			if (nes->o_ppu->get_flag (CTL_1, BIT_5))
			{
				switch (address)
				{
					case 0x5128:
						if (3 == bChrBankSwitch) nes->o_ppu->swap_page (ExtraBackground, 0x000, value & _1K_chr_mask, _1K_); 
						break;
					case 0x5129:
						switch (bChrBankSwitch)
						{
							case 2: nes->o_ppu->swap_page (ExtraBackground, 0x000, value & _2K_chr_mask, _2K_); break;
							case 3: nes->o_ppu->swap_page (ExtraBackground, 0x400, value & _1K_chr_mask, _1K_); break;
						}
						break;
					case 0x512a:
						if (3 == bChrBankSwitch) nes->o_ppu->swap_page (ExtraBackground, 0x800, value & _1K_chr_mask, _1K_); 
						break;
					case 0x512b:
						switch (bChrBankSwitch)
						{
							case 0: nes->o_ppu->swap_page (ExtraBackground, 0x000, value & _8K_chr_mask, _8K_); break;
							case 1: nes->o_ppu->swap_page (ExtraBackground, 0x000, value & _4K_chr_mask, _4K_); break;
							case 2: nes->o_ppu->swap_page (ExtraBackground, 0x800, value & _2K_chr_mask, _2K_); break;
							case 3: nes->o_ppu->swap_page (ExtraBackground, 0xc00, value & _1K_chr_mask, _1K_); break;
						}
						break;	
				}
			}
			if (address != 0x5128 && address != 0x5129 && address != 0x512a && address != 0x512b) nes->general_log.f_write ("swsbs", "Address: ", address, " Data: ", value, "\r\n");
	}
}

__UINT_8 c_mapper_005 :: read_byte (__UINT_16 address)
{
	if (address > 0x5c00 && bExRamUsage && bExRamUsage != 1) return ExRam [address];

	switch (address)
	{
		case 0x5204:
		{
			__UINT_8 value;
			if (bIsIrqGenerated) value = BIT_7;
			if (nes->o_ppu->get_flag (CTL_2, BIT_3 | BIT_4) && nes->o_ppu->information ().scanline && nes->o_ppu->information ().scanline < 240) value |= BIT_6;
			bIsIrqGenerated = FALSE;
			nes->o_cpu->set_irq_line (FALSE);
			return value;
		}

		case 0x5205: return (__UINT_8) (iProduct);
		case 0x5206: return (__UINT_8) (iProduct >> 8);
		default: return 0x00;
	}
}

void c_mapper_005 :: h_blank (void)
{
	if (nes->o_ppu->information ().scanline > 240) return;
	if (!nes->o_ppu->information ().scanline) nes->o_cpu->set_irq_line (FALSE);

	if (iIrqCounter == nes->o_ppu->information ().scanline)
	{
		if (bIsIrqEnabled && nes->o_ppu->get_flag (CTL_2, BIT_3 | BIT_4)) nes->o_cpu->set_irq_line (TRUE);
		bIsIrqGenerated = TRUE;
	}
}

void c_mapper_005 :: set_mirroring (void)
{
	for (__UINT_32 uiIndex = 0; uiIndex < 4; uiIndex ++)
	{
		switch (bMirroring [uiIndex])
		{
			case 0: nes->o_ppu->set_mirroring (uiIndex, nes->o_ppu->get_vram (0x2000)); break;
			case 1: nes->o_ppu->set_mirroring (uiIndex, nes->o_ppu->get_vram (0x2400)); break;
			case 2:
				if (bExRamUsage & 2) nes->o_ppu->set_mirroring (uiIndex, nes->o_ppu->get_vram (0x2400));
				else nes->o_ppu->set_mirroring (uiIndex, &ExRam [0x000]);
				break;
			case 3: nes->o_ppu->set_mirroring (uiIndex, &Fillnametable [0x000]); break;
		}
	}
}

void c_mapper_005 :: save_state (c_tracer &o_writer)
{
	ExRam.dump_to (o_writer, NULL, 0, _1K_, BINARY);
	Fillnametable.dump_to (o_writer, NULL, 0, _1K_, BINARY);
	o_writer.write ((__UINT_8 *)(&bIsIrqEnabled), 1);
	o_writer.write ((__UINT_8 *)(&bIsIrqGenerated), 1);
	o_writer.write ((__UINT_8 *)(&bIsSRamEnabled), 1);
	o_writer.write ((__UINT_8 *)(&iIrqCounter), 4);
	o_writer.write ((__UINT_8 *)(iMultipliers), 8);
	o_writer.write ((__UINT_8 *)(&iProduct), 4);
	o_writer.write (&bBankSwitch, 1);
	o_writer.write (&bChrBankSwitch, 1);
	o_writer.write (bSRamProtected, 2);
	o_writer.write (&bExRamUsage, 1);
}

void c_mapper_005 :: load_state (c_tracer &o_reader)
{
	o_reader.read (&ExRam [0], _1K_);
	o_reader.read (&Fillnametable [0], _1K_);
	o_reader.read ((__UINT_8 *)(&bIsIrqEnabled), 1);
	o_reader.read ((__UINT_8 *)(&bIsIrqGenerated), 1);
	o_reader.read ((__UINT_8 *)(&bIsSRamEnabled), 1);
	o_reader.read ((__UINT_8 *)(&iIrqCounter), 4);
	o_reader.read ((__UINT_8 *)(iMultipliers), 8);
	o_reader.read ((__UINT_8 *)(&iProduct), 4);
	o_reader.read (&bBankSwitch, 1);
	o_reader.read (&bChrBankSwitch, 1);
	o_reader.read (bSRamProtected, 2);
	o_reader.read (&bExRamUsage, 1);
}
