/*******************************************************************************
    crudNES - A NES emulator for reverse engineering purposes
    SUNSOFT FME-7
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
#include "../include/mappers/c_mapper_069.h"
#include "../include/c_nes.h"
#include "../include/c_ppu.h"
#include "../include/c_rom.h"
#include "../include/datatypes.h"

/******************************************************************************/
/** External Data                                                            **/
/******************************************************************************/

extern c_machine *o_machine;

c_mapper_069 :: c_mapper_069 (void)
{
	__DBG_INSTALLING ("Mapper #069");
	__DBG_INSTALLED ();
}

c_mapper_069 :: ~c_mapper_069 (void)
{
	__DBG_UNINSTALLING ("Mapper #069");
	__DBG_UNINSTALLED ();
}

void c_mapper_069 :: reset (void)
{
	bIRQEnabled = FALSE;
	bRAMEnabled = FALSE;
	iIRQCounter = 0;
	bControl = 0;

	last_page_switched = 0x00;
	nes->o_cpu->swap_page (0x8000, 0, _16K_);
	nes->o_cpu->swap_page (0xc000, nes->o_rom->information ().prg_pages - 1, _16K_);
}

void c_mapper_069 :: write_byte (__UINT_16 address, __UINT_8 value)
{
	if (address < 0x8000 && bRAMEnabled)
		nes->o_sram->write_byte (address, value);
	else if (0x8000 == address) bControl = value & 0xf;
	else if (0xa000 == address)
	{
		switch (bControl)
		{
			case 0:	case 1:	case 2: case 3:	case 4:	case 5:	case 6:	case 7:
				nes->o_ppu->swap_page (bControl << 10, value & _1K_chr_mask, _1K_);
				break;
			case 8:
				if (!(value & BIT_6))
                    nes->o_cpu->swap_page (nes->o_sram, 0x0000, (value & 0x3f) & _8K_prg_mask, _8K_);

				bRAMEnabled = value & BIT_7;
				break;
			case 9:
				nes->o_cpu->swap_page (0x8000, value & _8K_prg_mask, _8K_);
				break;
			case 0xa:
				nes->o_cpu->swap_page (0x2000, value & _8K_prg_mask, _8K_);
				break;
			case 0xb:
		        nes->o_cpu->swap_page (0xc000, value & _8K_prg_mask, _8K_);
				break;
			case 0xc:
                value &= 3;
				if (!value) nes->o_ppu->set_mirroring (_2C02_VERTICAL_MIRRORING);
				else if (1 == value) nes->o_ppu->set_mirroring (_2C02_HORIZONTAL_MIRRORING);
				else if (2 == value) nes->o_ppu->set_mirroring (_2C02_2000_MIRRORING);
				else if (3 == value) nes->o_ppu->set_mirroring (_2C02_2400_MIRRORING);
				break;				
			case 0xd:
				bCounterEnabled = value & BIT_0;
				bIRQEnabled = value & BIT_7;

				if (!bCounterEnabled || !bIRQEnabled)
				{
					bIRQEnabled = FALSE;
					nes->o_cpu->set_irq_line (FALSE);
				}

				if (bCounterEnabled) iIRQCounter *= 3;
				break;
			case 0xe:
				iIRQCounter &= 0xff00;
				iIRQCounter |= value;
				break;
			case 0xf:
				iIRQCounter &= 0x00ff;
				iIRQCounter |= (value << 8);
			default: ;
		}
	}
}

void c_mapper_069 :: h_blank (void)
{
    if (bCounterEnabled)
	{
		if (iIRQCounter <= 341) 
		{
			iIRQCounter = (0xffff * 3) + (iIRQCounter - 341);
			if (bIRQEnabled) nes->o_cpu->set_irq_line (TRUE);
		}
		else iIRQCounter -= 341;		
	}
}