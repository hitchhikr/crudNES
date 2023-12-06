/*******************************************************************************
    crudNES - A NES emulator for reverse engineering purposes
	o_state Saver/Loader
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

#include <fcntl.h>
#include <io.h>
#include <stdio.h>

#include "include/c_control.h"
#include "include/c_cpu.h"
#include "include/c_input.h"
#include "include/c_ppu.h"
#include "include/c_rom.h"
#include "include/c_save_state.h"
#include "blargg/Nes_Apu_Reflector.h"

/***********************************************************************/
/** External Data													  **/
/***********************************************************************/

extern c_machine *o_machine;

/***********************************************************************/
/** reset ()                                                          **/
/***********************************************************************/

void c_save_state :: reset (void)
{
	iSlot = 0;
}

/***********************************************************************/
/** get_filename ()                                                    **/
/***********************************************************************/

const char * c_save_state :: get_filename (void)
{
	char *filename = strrchr (nes->o_rom->information ().filename, '\\');
	if(filename == NULL) filename = nes->o_rom->information ().filename;
	filename++;
	__INT_32 length = (__INT_32)(strlen (filename));
	strcpy (filename + length - 3, "as");
	_itoa (iSlot, filename + length - 1, 10);

	return filename;
}

const char * c_save_state :: get_filename (const char *extension)
{
	char *filename = strrchr (nes->o_rom->information ().filename, '\\');
	if(filename == NULL)
	{
		filename = nes->o_rom->information ().filename;
	}
	else
	{
		filename++;
	}
	__INT_32 length = (__INT_32)(strlen (filename));
	strcpy (filename + length - 3, extension );
	return filename;
}

char * c_save_state :: get_filename(char *filepath, const char *extension, int strip)
{
    char *filename = filepath;
    int i;
    __INT_32 length;

    if(strip)
    {
		// Remove the filepath
        length = strlen(filepath);
        while(length > 0 && filename[length - 1] != '\\')
        {
            length--;
        }
		if(length > 0)
		{
			for(i = 0; i < (int) strlen(filename) - length; i++)
			{
				filename[i] = filename[i + length];
			}
			filename[i] = '\0';
		}
	}
    else
    {
	    filename = strrchr (filepath, '\\');
	    if(filename == NULL)
		{
			filename = filepath;
		}
		else
		{
			filename++;
		}
    }    
	length = (__INT_32)(strlen (filename));
	if(strlen(extension))
	{
    	strcpy (filename + length - 3, extension );
	}
	else
	{
        filename[length - 4] = '\0';
	}
	return filename;
}

/***********************************************************************/
/** save ()                                                           **/
/***********************************************************************/

void c_save_state :: save (void)
{
	Nes_Apu_Reflector Saver;

	c_tracer o_writer (get_filename (), __NEW, __FILE);
	nes->o_sram->dump_to (o_writer, NULL, 0x0000, _8K_, BINARY);
	nes->o_ram->dump_to (o_writer, NULL, 0x000, _2K_, BINARY);
	nes->o_cpu->save_state (o_writer, MAIN);
	nes->o_ppu->save_state (o_writer, MAIN);
	nes->o_cpu->save_state (o_writer, OTHER);
	nes->o_ppu->save_state (o_writer, OTHER);
	Saver.save (nes->o_apu, o_writer.get_handle ());
	o_writer.close ();

	rest (100);
}

/***********************************************************************/
/** load ()                                                           **/
/***********************************************************************/

void c_save_state :: load (void)
{
	Nes_Apu_Reflector Loader;
	c_tracer o_loader;

	if (o_loader.set_output_file (get_filename (), __READ)) return;
	o_loader.read (&(*(nes->o_sram)) [0], _8K_);
	o_loader.read (&(*(nes->o_ram)) [0], _2K_);
	nes->o_cpu->load_state (o_loader, MAIN);
	nes->o_ppu->load_state (o_loader, MAIN);
	nes->o_cpu->load_state (o_loader, OTHER);
	nes->o_ppu->load_state (o_loader, OTHER);
	Loader.load (o_loader.get_handle (), nes->o_apu);
	o_loader.close ();

	rest (100);
}
