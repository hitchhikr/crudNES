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

#include "include/datatypes.h"
#include "include/c_nes.h"
#include "include/c_cpu.h"
#include "include/c_control.h"
#include "include/c_tracer.h"
#include "include/c_input.h"
#include "include/c_ppu.h"
#include "include/c_rom.h"
#include "include/c_save_state.h"
#include "include/c_graphics.h"
#include "include/c_input.h"

#include "include/mappers/c_mapper_000.h"
#include "include/mappers/c_mapper_001.h"
#include "include/mappers/c_mapper_002.h"
#include "include/mappers/c_mapper_003.h"
#include "include/mappers/c_mapper_004.h"
#include "include/mappers/c_mapper_005.h"
#include "include/mappers/c_mapper_007.h"
#include "include/mappers/c_mapper_009.h"
#include "include/mappers/c_mapper_010.h"
#include "include/mappers/c_mapper_011.h"
#include "include/mappers/c_mapper_016.h"
#include "include/mappers/c_mapper_021.h"
#include "include/mappers/c_mapper_034.h"
#include "include/mappers/c_mapper_068.h"
#include "include/mappers/c_mapper_069.h"
#include "include/mappers/c_mapper_071.h"
#include "include/mappers/c_mapper_091.h"

c_nes :: c_nes()
{
    prg_pages = NULL;
    chr_pages = NULL;
}

void c_nes :: Open (int PAL, const char *FilePath)
{
	close (); 

    general_log.set_output_file ("crudNES.log", __NEW);

	__NEW (o_rom, c_nes_rom (FilePath));
	bIsPowerOff = o_rom->check_header (PAL);

	if (bIsPowerOff)
	{
		#ifdef GUIPLUS
			alert ("ERROR: File not found!",
				   " ",
			       NULL,
			       "&OK", NULL, NULL, NULL);
		#endif

		__DELETE (o_rom);
		general_log.close ();
		return; 
	}

	__NEW (o_ram, c_mem_block (0x800));
	__NEW (o_sram, c_mem_block (0x2000));

	__NEW (o_state, c_save_state);
	__NEW (o_control, c_nes_control);
	__NEW (o_input, c_input);
	__NEW (o_cpu, c_nes_cpu);
	__NEW (o_gfx, c_graphics);
	__NEW (o_ppu, c_nes_ppu);

	switch (o_rom->information ().mapper)
	{
		case 0:
		case 185:
		    __NEW (o_mapper, c_mapper_000);
		    break;
		case 1: __NEW (o_mapper, c_mapper_001); break;
		case 2: __NEW (o_mapper, c_mapper_002); break;

		case 3:
		case 87:
		    __NEW (o_mapper, c_mapper_003);
		    break;

		case 4:
		case 118:
		case 119:
		case 220:
		    __NEW (o_mapper, c_mapper_004);
		    break;
		
		case 5: __NEW (o_mapper, c_mapper_005); break;
		case 7: __NEW (o_mapper, c_mapper_007); break;
		case 9: __NEW (o_mapper, c_mapper_009); break;
		case 10: __NEW (o_mapper, c_mapper_010); break;
		case 11: __NEW (o_mapper, c_mapper_011); break;
		case 16: __NEW (o_mapper, c_mapper_016); break;
		case 21: __NEW (o_mapper, c_mapper_021); break;
		case 25: __NEW (o_mapper, c_mapper_021); break;
		case 34: __NEW (o_mapper, c_mapper_034); break;
		case 68: __NEW (o_mapper, c_mapper_068); break;
		case 69: __NEW (o_mapper, c_mapper_069); break;
		case 71: __NEW (o_mapper, c_mapper_071); break;
		case 91: __NEW (o_mapper, c_mapper_091); break;

		default:
			alert ("WARNING: Unsupported o_mapper!", 
				   "crudNES will now attempt to run the selected",
				   "program under the default memory mapping scheme.",
				   "Proceed...", NULL, NULL, NULL);
			__NEW (o_mapper, c_mapper);
	}

    strcpy(Labels_Name, FilePath);
    strcpy(Game_FileName, FilePath);
    strcpy(Game_Name, FilePath);
    o_state->get_filename(Labels_Name, "txt", 0);
    o_state->get_filename(Game_FileName, "", 0);
    o_state->get_filename(Game_Name, "", 1);

	__NEW (BankJMPList, c_label_holder);

//	Rom->close ();

	bis_running = TRUE;
	bis_paused = FALSE;

	show_mouse (0);
	clear_keybuf ();
	
	o_sram->load_from ((char *)(o_state->get_filename ("sav")), 0, 0, _8K_);
	o_state->reset ();
	o_cpu->reset ();
	o_cpu->run_accurate ();

#ifdef GUIPLUS
	show_mouse (screen);
	clear_keybuf ();
#endif
}

void c_nes :: close (void)
{
	bis_running = FALSE;

	if (!bIsPowerOff)
	{
		bIsPowerOff = TRUE;

        BankJMPList->dump_rom();

		if ((nes->o_rom->information ().mapper != 0 &&
			nes->o_rom->information ().mapper != 2) ||
			nes->o_rom->information ().o_sram)
	    {
			o_sram->dump_to((char *) (o_state->get_filename ("sav")),
			                NULL, 0, _8K_, BINARY);
        }

		__DELETE (o_cpu);		
		__DELETE (o_control);
		__DELETE (o_gfx);
		__DELETE (o_input);
		__DELETE (o_mapper);
		__DELETE (o_ppu);
		__DELETE (o_rom);
		__DELETE (o_ram);
		__DELETE (o_sram);
		__DELETE (o_state);
		__DELETE (BankJMPList);
		general_log.close ();
	}
}

void c_nes :: reset (void)
{
	o_cpu->reset ();
}

void c_nes :: pause ()
{
	bis_paused = !bis_paused;
}

void c_nes :: stop ()
{
	bis_running = FALSE;
}

void c_nes :: set_slot (__INT_32 Slot)
{
	o_state->set_slot (Slot);
}

void c_nes :: save_state (void)
{
	o_state->save ();
}

void c_nes :: load_state (void)
{
	o_state->load ();
}

void c_nes :: set_instruction_dumper (__BOOL o_state)
{
	o_cpu->toggle_tracer ();
/*	if (o_cpu->is_tracer_on ()) writeText->Hide ();
	else writeText->Show ();*/
}

void c_nes :: set_label_holder (__BOOL o_state)
{
	o_cpu->toggle_label_holder ();
}

void c_nes :: set_sram (const char *Path)
{
	SRamPath = (char *)(Path);
	o_sram->load_from (Path, 0, 0, _8K_);
	reset ();
}

void c_nes :: dump_header (const char *Path)
{
	__UINT_8 bROMHeader [0x10];
    o_rom->transfer_block (bROMHeader, 0x00, 0x10);
	c_tracer THeaderDumper ((char *)(Path));
	THeaderDumper.write (bROMHeader, 0x10);
	THeaderDumper.close ();
}

void c_nes :: load_config (void)
{
	o_cpu->request_config_load ();
	o_gfx->request_config_load ();
}
