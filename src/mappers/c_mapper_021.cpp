/*******************************************************************************
    crudNES - A NES emulator for reverse engineering purposes
    VCR4
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
// TODO: Completely broken.

#include "../include/c_cpu.h"
#include "../include/c_tracer.h"
#include "../include/mappers/c_mapper.h"
#include "../include/mappers/c_mapper_021.h"
#include "../include/c_nes.h"
#include "../include/c_ppu.h"
#include "../include/c_rom.h"
#include "../include/datatypes.h"

/******************************************************************************/
/** External Data                                                            **/
/******************************************************************************/

extern c_machine *o_machine;

c_mapper_021 :: c_mapper_021 (void)
{
	__DBG_INSTALLING ("Mapper #021");

	__DBG_INSTALLED ();
}

c_mapper_021 :: ~c_mapper_021 (void)
{
	__DBG_UNINSTALLING ("Mapper #021");
	__DBG_UNINSTALLED ();
}

void c_mapper_021 :: reset (void)
{
    int i;

	bIRQEnabled = FALSE;
	bIRQCounter = 0;
	bPRGBankSwitch = 0;

    for(i = 0; i < 8; i++)
    {
        CHRBanks[i] = i;
    }

    PRGBanks[0] = 0;
    PRGBanks[1] = 1;
    swap_banks();

	memset (bPortLow, 0x00, 8);
	memset (bPortHigh, 0x00, 8);
	memset (bPortData, 0x00, 8);

	nes->o_cpu->swap_page (0x8000, 0, _16K_);
	nes->o_cpu->swap_page (0xc000, nes->o_rom->information ().prg_pages - 1, _16K_);
}

void c_mapper_021 :: swap_banks ()
{
    nes->o_cpu->swap_page (0xa000, PRGBanks[1] & _8K_prg_mask, _8K_);

    nes->o_cpu->swap_page ((bPRGBankSwitch ? 0xc000 : 0x8000), PRGBanks[0] & _8K_prg_mask, _8K_);
}

void c_mapper_021 :: swap_chr()
{
    int i;
    
    for(i = 0; i < 8; i++)
    {
//        setchr1(x * 1024, CHRBanks[x]);
    }
}

void c_mapper_021 :: write_byte (__UINT_16 address, __UINT_8 value)
{
	if (0x8000 == address)
	{
	    PRGBanks[0] = value;
	    swap_banks();
	}
	else if (0x9000 == address)
	{
		value &= 3;
		if (0 == value) nes->o_ppu->set_mirroring (_2C02_VERTICAL_MIRRORING);
		else if (1 == value) nes->o_ppu->set_mirroring (_2C02_HORIZONTAL_MIRRORING);
		else if (2 == value) nes->o_ppu->set_mirroring (_2C02_2400_MIRRORING);
		else if (3 == value) nes->o_ppu->set_mirroring (_2C02_2000_MIRRORING);
	}
	else if (0x9002 == address ||
	         0x9004 == address ||
	         0x9006 == address)
	{
	    bPRGBankSwitch = value & BIT_1;
        swap_banks();        
	}
	else if (0xa000 == address)
	{
	    PRGBanks[1] = value;
        swap_banks();
    }

	else if (0xb000 == address) updatePPUPage (0, 0, value & 0xf);
	else if (0xb002 == address) updatePPUPage (0, 1, value & 0xf);
	else if (0xb001 == address) updatePPUPage (1, 0, value & 0xf);
	else if (0xb003 == address) updatePPUPage (1, 1, value & 0xf);
	else if (0xb004 == address) updatePPUPage (1, 0, value & 0xf);
	else if (0xb006 == address) updatePPUPage (1, 1, value & 0xf);
	else if (0xc000 == address) updatePPUPage (2, 0, value & 0xf);
	else if (0xc002 == address) updatePPUPage (2, 1, value & 0xf);
	else if (0xc001 == address) updatePPUPage (3, 0, value & 0xf);
	else if (0xc003 == address) updatePPUPage (3, 1, value & 0xf);
	else if (0xc004 == address) updatePPUPage (3, 0, value & 0xf);
	else if (0xc006 == address) updatePPUPage (3, 1, value & 0xf);
	else if (0xd000 == address) updatePPUPage (4, 0, value & 0xf);
	else if (0xd002 == address) updatePPUPage (4, 1, value & 0xf);
	else if (0xd001 == address) updatePPUPage (5, 0, value & 0xf);
	else if (0xd003 == address) updatePPUPage (5, 1, value & 0xf);
	else if (0xd004 == address) updatePPUPage (5, 0, value & 0xf);
	else if (0xd006 == address) updatePPUPage (5, 1, value & 0xf);
	else if (0xe000 == address) updatePPUPage (6, 0, value & 0xf);
	else if (0xe002 == address) updatePPUPage (6, 1, value & 0xf);
	else if (0xe001 == address) updatePPUPage (7, 0, value & 0xf);
	else if (0xe003 == address) updatePPUPage (7, 1, value & 0xf);
	else if (0xe004 == address) updatePPUPage (7, 0, value & 0xf);
	else if (0xe006 == address) updatePPUPage (7, 1, value & 0xf);
	
	switch(address &= 0xf006)
	{
	    case 0xf000:
	        bIRQReload &= 0xf0;
	        bIRQReload |= value & 0xf;
	        break;
	    case 0xf002:
	        bIRQReload &= 0xf;
	        bIRQReload |= (value << 4) & 0xf0;
	        break;

	    case 0xf004:
	        bIRQCounter = bIRQReload;
		    value &= 3;
		    if (value == 2 || value == 3) bIRQEnabled = TRUE;
		    else if (value == 0) bIRQEnabled = FALSE;
            break;

	    case 0xf006:
	        bIRQCounter = 0;
//            IRQa = K4IRQ;
            break;
	}
//	else if (0xf003 == address) bIRQCounter = 0;
}

void c_mapper_021 :: updatePPUPage (__UINT_8 bArea, __UINT_8 bHalf, __UINT_8 page)
{
	if (bHalf) { bPortHigh [bArea] = 1; bPortData [bArea] &= 0xf; bPortData [bArea] |= page << 4; }
	else { bPortLow [bArea] = 1; bPortData [bArea] &= 0xf0; bPortData [bArea] |= page; }

	if (bPortHigh [bArea] && bPortLow [bArea])
	{
		bPortHigh [bArea] = bPortLow [bArea] = 0;
		nes->o_ppu->swap_page (bArea * _1K_, bPortData [bArea] & _1K_chr_mask, _1K_);
	}
}

void c_mapper_021 :: h_blank (void)
{
	bIRQCounter ++;
	if (bIRQCounter == bIRQReload || !bIRQCounter) {
		if (bIRQEnabled) nes->o_cpu->set_irq_line (TRUE);
	}
}
