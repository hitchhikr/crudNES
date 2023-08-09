/*******************************************************************************
    crudNES - A NES emulator for reverse engineering purposes
    Hardware Controller
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

#include <math.h>
#include <stdio.h>
#include <string.h>

#include "include/c_control.h"
#include "include/c_machine.h"
#include "include/c_nes.h"
#include "include/c_cpu.h"
#include "include/c_ppu.h"
#include "include/mappers/c_mapper.h"
#include "include/c_input.h"

/******************************************************************************/
/** External Data                                                            **/
/******************************************************************************/

extern c_machine *o_machine;

/******************************************************************************/
/** Installer                                                                **/
/******************************************************************************/

c_nes_control :: c_nes_control (void)
{
	__DBG_INSTALLING ("Multi-Hardware Controller");

	__NEW_MEM_BLOCK (control [APU], __UINT_8, 0x18);
	__NEW_MEM_BLOCK (control [DMA], __UINT_8, 1);

	memset (control [APU], 0, 0x18);
	memset (control [DMA], 0, 0x01);

	__DBG_INSTALLED ();
}

/******************************************************************************/
/** Uninstaller                                                              **/
/******************************************************************************/

c_nes_control :: ~c_nes_control (void)
{
	__DBG_UNINSTALLING ("Multi-Hardware Controller");

	__DELETE_MEM_BLOCK (control [APU]);
	__DELETE_MEM_BLOCK (control [DMA]);

	__DBG_UNINSTALLED ();
}

/******************************************************************************/
/** read_byte ()                                                              **/
/******************************************************************************/

__UINT_8 c_nes_control :: read_byte (__UINT_16 address)
{
	if (address > 0x7fff) return nes->o_cpu->read_byte (address);
	if (address < 0x2000) return nes->o_ram->read_byte (address);
	if (address < 0x4000) return nes->o_ppu->read_byte (address);
	if (address < 0x4020) return ApuDmaReadByte (address);
	if (address > 0x5fff) return nes->o_sram->read_byte (address);
	return nes->o_mapper->read_byte (address);
}

__UINT_16 c_nes_control :: read_word (__UINT_16 address)
{
	if (address > 0x7fff) return nes->o_cpu->read_word (address);
	if (address < 0x2000) return nes->o_ram->read_word (address);
	if (address > 0x5fff) return nes->o_sram->read_word (address);
	return 0x00;
}

/******************************************************************************/
/** write_byte ()                                                             **/
/******************************************************************************/

void c_nes_control :: write_byte (__UINT_16 address, __UINT_8 value)
{
	if (address < 0x2000) nes->o_ram->write_byte(address, value);
	else if (address < 0x4000) nes->o_ppu->write_byte (address, value);
	else if (address < 0x4020) ApuDmawrite_byte (address, value);
	else if (address > 0x4fff) nes->o_mapper->write_byte (address, value);
	else if (nes->o_cpu->is_tracer_on ()) nes->general_log.f_write ("swsbs", "Multi-Hardware Controller: WARNING: Bad write at ", address, " ", value, "\r\n");
}

/******************************************************************************/
/** APU/DMA/o_input Registers - Read                                           **/
/******************************************************************************/

__UINT_8 c_nes_control :: ApuDmaReadByte (const __UINT_16 &address)
{
	switch (address)
	{
		//Apparently some games expect data returned from joypad ports to be
		//in the format of $4x (x_offset: 0/1).
		case 0x4016: return (0x40 | nes->o_input->read_bitstream ());

		case nes->o_apu.status_addr:
			return nes->o_apu.read_status (nes->o_cpu->current_time ());
	}

	return 0x40;
}

/******************************************************************************/
/** APU/DMA/o_input Registers - write                                          **/
/******************************************************************************/

void c_nes_control :: ApuDmawrite_byte (const __UINT_16 &address, const __UINT_8 &value)
{
	switch (address)
	{
   		case 0x4014:
			if (!nes->o_ppu->information ().is_v_blank
				&& nes->o_ppu->get_flag (CTL_2, BIT_4 | BIT_3))
			{
				if (nes->o_cpu->is_tracer_on ()) nes->general_log.f_write ("s", "\r\nPPU: WARNING: Attempting to perform OAM DMA prior to VBlank without disabling rendering.");
				return;
			}

			nes->o_ppu->oam_dma (nes->o_ram, (value << 8) & 0x7ff);
			nes->o_cpu->kill_cycles (512 * 3); //Yes, I ought to be sent to death row for 
                                      //killing 512 cycles in such a brutal manner...
			return;
               
		case 0x4016:
			nes->o_input->write_strobe (value & 1);
      		return;

		default: ;
			if (address >= nes->o_apu.start_addr && address <= nes->o_apu.end_addr)
					nes->o_apu.write_register (nes->o_cpu->current_time (), address, value);
	}
}
