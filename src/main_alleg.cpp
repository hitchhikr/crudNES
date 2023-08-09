/*******************************************************************************
    crudNES - A NES emulator for reverse engineering purposes
    Main Module
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

#define ALLEGRO_USE_CONSOLE
#ifdef __CRUDNES_ALLEGRO_GUI

#include "include/allegro.h"
#include <stdio.h>

#include "include/c_machine.h"
#include "include/c_nes.h"
#include "include/c_gui.h"

/******************************************************************************/
/** Global Data                                                              **/
/******************************************************************************/

c_machine *o_machine;
NESGUIHandler NESGUI;

extern int NES_Uninstall (void);

void print_usage(void)
{
    printf("crudNES v1.0\n");
    printf("Copyright (C) 2003-2004 Sadai Sarmiento\n");
    printf("Copyright (C) 2023 Franck \"hitchhikr\" Charlet\n\n");
    printf("Usage: <P|N> <rom file>\n\n");
    printf("       <P|N> = PAL|NTSC\n\n");
    printf("       Keys: A=A S=B ENTER=START SPACE=SELECT\n");
    printf("             Arrows=Direction pad\n");
    printf("             (Joystick is supported too)\n");
    printf("       F5: Save state\n");
    printf("       F6: Load state\n");
    printf("       F7: Decrement current slot number (Total # of slots: 8)\n");
    printf("       F8: Increment current slot number\n");
    printf("       F10: Soft reset\n");
    printf("       ESC: Quit emulation and start disassembling process\n");
}

int main (int argc, char *argv[])
{
    int pos_arg = 1;
    if(argc < 3)
    {
        print_usage();
        return 0;
    }

	allegro_init ();
	install_timer ();

	install_mouse ();
	install_keyboard ();

	install_sound (DIGI_AUTODETECT, MIDI_NONE, NULL);

	set_color_depth (8);
	if (set_gfx_mode (GFX_AUTODETECT_WINDOWED, 256, 240, 0, 0) < 0)
	{
		return -1;
    }

	set_display_switch_mode(SWITCH_BACKGROUND);

	__NEW (o_machine, c_nes);

	NESGUI.Install ();

	switch(toupper(argv[pos_arg][0]))
	{
        case 'P':
	        o_machine->Open(1, argv[++pos_arg]);
            break;
        case 'N':
	        o_machine->Open(0, argv[++pos_arg]);
            break;
        default:
            print_usage();
            break;
	}
	
//	NESGUI.run ();

	NES_Uninstall ();

	NESGUI.Uninstall ();

	remove_keyboard ();
	remove_mouse ();
	allegro_exit ();

	return 0;
}
END_OF_MAIN()

#endif
