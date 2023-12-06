/*******************************************************************************
    crudNES - A NES emulator for reverse engineering purposes
    Bandai
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
#include "../include/mappers/c_mapper_016.h"
#include "../include/c_nes.h"
#include "../include/c_ppu.h"
#include "../include/c_rom.h"
#include "../include/datatypes.h"

/******************************************************************************/
/** External Data                                                            **/
/******************************************************************************/

extern c_machine *o_machine;

c_mapper_016 :: c_mapper_016 (void)
{
	__DBG_INSTALLING ("Mapper #016");

	__DBG_INSTALLED ();
}

c_mapper_016 :: ~c_mapper_016 (void)
{
	__DBG_UNINSTALLING ("Mapper #016");
	__DBG_UNINSTALLED ();
}

void c_mapper_016 :: reset (void)
{
	iIRQCounter = 342;
	bLatch = 342;
	last_prg_page = 0;

	nes->o_ppu->swap_page (0, 0, _1K_);
	nes->o_ppu->swap_page (0x400, 0, _1K_);
	nes->o_ppu->swap_page (0x800, 0, _1K_);
	nes->o_ppu->swap_page (0xc00, 0, _1K_);
	nes->o_ppu->swap_page (0x1000, 0, _1K_);
	nes->o_ppu->swap_page (0x1400, 0, _1K_);
	nes->o_ppu->swap_page (0x1800, 0, _1K_);
	nes->o_ppu->swap_page (0x1c00, 0, _1K_);
	nes->o_cpu->swap_page (0x8000, 0, _16K_);
	nes->o_cpu->swap_page (0xc000, nes->o_rom->information ().prg_pages - 1, _16K_);
}

void c_mapper_016 :: write_byte (__UINT_16 address, __UINT_8 value)
{
	switch (address)
	{
		case 0x6000: case 0x7ff0: case 0x8000: 
		case 0x6001: case 0x7ff1: case 0x8001: 
		case 0x6002: case 0x7ff2: case 0x8002: 
		case 0x6003: case 0x7ff3: case 0x8003: 
		case 0x6004: case 0x7ff4: case 0x8004: 
		case 0x6005: case 0x7ff5: case 0x8005: 
		case 0x6006: case 0x7ff6: case 0x8006: 
		case 0x6007: case 0x7ff7: case 0x8007: 
			nes->o_ppu->swap_page ((address & 7) << 10, value & _1K_chr_mask, _1K_);
			break;

		case 0x6008: case 0x7ff8: case 0x8008: 
			nes->o_cpu->swap_page (0x8000, value & _16K_prg_mask, _16K_);
			last_prg_page = value & _16K_prg_mask;
			break;

		case 0x6009: case 0x7ff9: case 0x8009:
			switch (value & 3)
			{
				case 0: nes->o_ppu->set_mirroring (_2C02_VERTICAL_MIRRORING); break;
				case 1: nes->o_ppu->set_mirroring (_2C02_HORIZONTAL_MIRRORING); break;
				case 2: nes->o_ppu->set_mirroring (_2C02_2000_MIRRORING); break;
				case 3: nes->o_ppu->set_mirroring (_2C02_2400_MIRRORING); break;
			}
			break;
		case 0x600a: case 0x7ffa:
			bIRQEnabled = value & BIT_0;
			if (bIRQEnabled)
			{
				_2A03_set_irq_line(TRUE);
				if(iIRQCounter == 0) nes->o_cpu->request_irq();
			}
			break;
		case 0x800a:
			bIRQEnabled = value & BIT_0;
			iIRQCounter = bLatch;
			if (bIRQEnabled)
			{
				_2A03_set_irq_line(TRUE);
						//if(iIRQCounter == 0)
					nes->o_cpu->request_irq();
			}
			break;
		case 0x600b: case 0x7ffb: 
			iIRQCounter &= 0xff00;
			iIRQCounter |= value;
			break;
		case 0x800b:
			bLatch &= 0xff00;
			bLatch |= value;
			break;
		case 0x600c: case 0x7ffc: 
			iIRQCounter &= 0x00ff;
			iIRQCounter |= (value << 8);
			break;
		case 0x800c: 
			bLatch &= 0x00ff;
			bLatch |= (value << 8);
			break;
	}
}

void c_mapper_016 :: h_blank (void)
{
	if (bIRQEnabled)
	{
		iIRQCounter -= 1;
		if (iIRQCounter < 0) 
		{
		//	iIRQCounter = (0xffff * 3) + (iIRQCounter - 341);
			nes->o_cpu->set_irq_line (TRUE);
		//	nes->o_cpu->request_irq();
			bIRQEnabled = 0;
			iIRQCounter = -1;
		}
	}
}
