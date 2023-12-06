/*******************************************************************************
    crudNES - A NES emulator for reverse engineering purposes
    MMC1
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
#include "../include/mappers/c_mapper_001.h"
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

c_mapper_001 :: c_mapper_001 (void)
{
	__DBG_INSTALLING ("Mapper #001");

	__DBG_INSTALLED ();
}

/******************************************************************************/
/** Uninstaller                                                              **/
/******************************************************************************/

c_mapper_001 :: ~c_mapper_001 (void)
{
	__DBG_UNINSTALLING ("Mapper #001");
	__DBG_UNINSTALLED ();
}

/******************************************************************************/
/** reset                                                                    **/
/******************************************************************************/

void c_mapper_001 :: reset (void)
{
	memset (registers, 0x00, 4);

	bit_shifter = 0;
	bBitBuffer = 0;
	registers [0] |= (BIT_2 | BIT_3);

	nes->o_ppu->swap_page (0x0000, 0, _8K_);

	nes->o_cpu->swap_page (0x8000, 0, _16K_);
	nes->o_cpu->swap_page (0xc000, nes->o_rom->information ().prg_pages - 1, _16K_);
}

/******************************************************************************/
/** write_byte ()                                                             **/
/******************************************************************************/

void c_mapper_001 :: write_byte (__UINT_16 address, __UINT_8 value)
{
    __UINT_16 uiReadAddr;

	if (address < 0x8000)
	{
		nes->o_sram->write_byte (address, value);
		return;
	}
	
	uiReadAddr = (address & 0x7fff) >> 13;
	// "Writing a value with bit 7 set ($80 through $FF) to any address in $8000-$FFFF
	//  clears the shift register to its initial state."
	if (value & BIT_7)
	{
		bit_shifter = 0;
		bBitBuffer = 0;
		registers [0] |= (BIT_3 | BIT_2);
	}
	else if (bit_shifter == 4)
	{
	    // We got 5 bits
		bBitBuffer |= ((value & 1) << bit_shifter);

		registers [uiReadAddr] = bBitBuffer;

		switch (uiReadAddr)
		{
			case 0:
				if (registers [0] & BIT_1)
				{
					if (bBitBuffer & BIT_0)
					{
					    nes->o_ppu->set_mirroring (_2C02_HORIZONTAL_MIRRORING);
					}
					else
					{
					    nes->o_ppu->set_mirroring (_2C02_VERTICAL_MIRRORING);
				    }
				}
				else
				{
					if (bBitBuffer & BIT_0)
					{
					    nes->o_ppu->set_mirroring (_2C02_2400_MIRRORING);
					}
					else
					{
					    nes->o_ppu->set_mirroring (_2C02_2000_MIRRORING);
				    }
				}
				break;

			case 1:
				if (registers [0] & BIT_4)
				{
					nes->o_ppu->swap_page (0x0000, registers [1] & _4K_chr_mask, _4K_);
                }
				else
				{
				    nes->o_ppu->swap_page (0x0000, (registers [1] >> 1) & _8K_chr_mask, _8K_);
				}
				break;

			case 2:
				if (registers [0] & BIT_4)
				{
					nes->o_ppu->swap_page (0x1000, registers [2] & _4K_chr_mask, _4K_);
                }
				break;

			case 3:
				if (!(registers [0] & BIT_3))
				{
					nes->o_cpu->swap_page (0x8000, ((registers [3] & 0xf) >> 1) & _32K_prg_mask, _32K_);
                }
				else
				{
					if (registers [0] & BIT_2)
					{
						last_page_switched = bBitBuffer & _16K_prg_mask;

						nes->o_cpu->swap_page (0x8000, (registers [3] /*& 0xf*/)/* & _16K_prg_mask*/, _16K_);
						nes->o_cpu->swap_page (0xc000, nes->o_rom->information ().prg_pages - 1, _16K_);
					}
					else
					{
						nes->o_cpu->swap_page (0xc000, (registers [3] /*& 0xf*/)/* & _16K_prg_mask*/, _16K_);
						nes->o_cpu->swap_page (0x8000, 0, _16K_);
					}
				}
				break;
		}

		bBitBuffer = 0;
		bit_shifter = 0;
	}
	else
	{
	    // Only store the bits
	    bBitBuffer |= ((value & 1) << bit_shifter++);
    }
}

/******************************************************************************/
/** save_state ()                                                             **/
/******************************************************************************/

void c_mapper_001 :: save_state (c_tracer &o_writer)
{
	o_writer.write (&bit_shifter, 1);
	o_writer.write (&bBitBuffer, 1);
	o_writer.write (registers, 4);
}

/******************************************************************************/
/** load_state ()                                                             **/
/******************************************************************************/

void c_mapper_001 :: load_state (c_tracer &o_reader)
{
	o_reader.read (&bit_shifter, 1);
	o_reader.read (&bBitBuffer, 1);
	o_reader.read (registers, 4);
}
