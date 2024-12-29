/*******************************************************************************
    crudNES - A NES emulator for reverse engineering purposes
    Main Module
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

#define ALLEGRO_USE_CONSOLE

#include "include/allegro.h"
#include <stdio.h>
#include <ctype.h>

#include "include/c_machine.h"
#include "include/c_nes.h"

/******************************************************************************/
/** Global Data                                                              **/
/******************************************************************************/

c_machine *o_machine;

char *ASCII = "APZLGITYEOXUKSVN";
int nbr_genies_6 = 0;
GENIE_6 genies_6[1024];
int nbr_genies_8 = 0;
GENIE_8 genies_8[1024];

void print_usage(void)
{
    printf(APPNAME" "APPVERSION"\n");
    printf("Copyright (C) 2003-2004 Sadai Sarmiento\n");
    printf("Copyright (C) 2023-2024 Franck \"hitchhikr\" Charlet\n\n");
    printf("Usage: [L] [J] [-XXXXXX] [-XXXXXXXX] <P|N> <rom file>\n\n");
    printf("       [L] = Turn instructions logger on at startup\n");
    printf("       [J] = Use 2 pads\n");
    printf("       [-XXXXXX] | [-XXXXXXXX] = Specify 6 or 8-char Game Genies\n");
    printf("                                 (Any number of Genies can be used)\n");
    printf("       <P|N> = PAL|NTSC\n\n");
    printf("       Keys: A=A S=B ENTER=START SPACE=SELECT\n");
    printf("             Arrows=Direction pad\n");
    printf("             (Joystick is supported too)\n");
    printf("       P: Turn emulation on/off\n");
    printf("       F5: Save state\n");
    printf("       F6: Load state\n");
    printf("       F7: Decrement current slot number (Total # of slots: 8)\n");
    printf("       F8: Increment current slot number\n");
    printf("       F10: Soft reset\n");
    printf("       F12: Toggle instructions logger (generates *HUGE* .log).\n");
    printf("       ESC: Quit emulation and start disassembling process\n");
}

void free_everything()
{
	o_machine->close ();
	remove_keyboard ();
	remove_mouse ();
	allegro_exit ();
}

int get_letter_position(char Letter, int len)
{
    int i;

    for(i = 0; i < (int) strlen(ASCII); i++)
    {
        if(toupper(Letter) == ASCII[i])
        {
            return(i);
        }
    }
    return(-1);
}

int get_genie(char *string)
{
    int address;
    int compare;
    int data;
    int n0;
    int n1;
    int n2;
    int n3;
    int n4;
    int n5;
    int n6;
    int n7;
    int len;

    len = strlen(string);
    if(len != 6 && len != 8)
    {
        return -1;
    }
    n0 = get_letter_position(string[0], len);
    n1 = get_letter_position(string[1], len);
    n2 = get_letter_position(string[2], len);
    n3 = get_letter_position(string[3], len);
    n4 = get_letter_position(string[4], len);
    n5 = get_letter_position(string[5], len);
    if(n0 == -1 ||
       n1 == -1 ||
       n2 == -1 ||
       n3 == -1 ||
       n4 == -1 ||
       n5 == -1)
    {
        return -1;
    }
    address = 0x8000 + 
              ((n3 & 7) << 12) |
              ((n5 & 7) << 8)  | ((n4 & 8) << 8) |
              ((n2 & 7) << 4)  | ((n1 & 8) << 4) |
               (n4 & 7)        |  (n3 & 8);
    if(len == 6)
    {
        // 6 letters genie
        data =   ((n1 & 7) << 4) | ((n0 & 8) << 4) |
                  (n0 & 7)       |  (n5 & 8);
        genies_6[nbr_genies_6].address.W = address;
        genies_6[nbr_genies_6].data = data;
        if(!nbr_genies_6 && !nbr_genies_8)
        {
		    printf("---------------------------------------------------------------------------------\n");
        }
        nbr_genies_6++;
        printf("Adding 6-Characters Genie: Address: 0x%04x - Set to: 0x%02x\n", address, data);
    }
    else
    {
        n6 = get_letter_position(string[6], len);
        n7 = get_letter_position(string[7], len);
        if(n6 == -1 ||
           n7 == -1)
        {
            return -1;
        }
        // 8 letters genie
        data =   ((n1 & 7) << 4) | ((n0 & 8) << 4) |
                  (n0 & 7)       |  (n7 & 8);
        compare = ((n7 & 7) << 4) | ((n6 & 8) << 4) |
                   (n6 & 7)       |  (n5 & 8);
        genies_8[nbr_genies_8].address.W = address;
        genies_8[nbr_genies_8].compare = compare;
        genies_8[nbr_genies_8].data = data;
        if(!nbr_genies_6 && !nbr_genies_8)
        {
		    printf("---------------------------------------------------------------------------------\n");
        }
        nbr_genies_8++;
        printf("Adding 8-Characters Genie: Address: 0x%04x - Compare: 0x%02x - Set to: 0x%02x\n", address, compare, data);
    }
    return 0;
}

int main (int argc, char *argv[])
{
    int pos_arg = 1;
    if(argc < 3)
    {
        print_usage();
        return 0;
    }

    printf(APPNAME" "APPVERSION"\n");

	atexit(&free_everything);

	allegro_init ();
	install_timer ();

	install_mouse ();
	install_keyboard ();

	install_sound (DIGI_AUTODETECT, MIDI_NONE, NULL);

	__NEW (o_machine, c_nes);

	nes->set_log_tracer(FALSE);
	if(toupper(argv[pos_arg][0]) == 'L')
	{
		nes->set_log_tracer(TRUE);
		pos_arg++;
	}

    o_machine->read_from_second_pad = 0;
	if(toupper(argv[pos_arg][0]) == 'J')
	{
        o_machine->read_from_second_pad = 1;
		pos_arg++;
	}

    // Store any eventual genies
    while(argv[pos_arg][0] == '-')
    {
        if(get_genie(&argv[pos_arg][1]) == -1)
        {
            printf("'%s' is not a valid Game Genie.\n", &argv[pos_arg][1]);
		    exit(-1);
        }
		pos_arg++;
    }
    if(nbr_genies_6 || nbr_genies_8)
    {
        printf("---------------------------------------------------------------------------------\n");
    }

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

	exit(0);
	return 0;
}
END_OF_MAIN()
