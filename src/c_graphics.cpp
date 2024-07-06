/*******************************************************************************
    crudNES - A NES emulator for reverse engineering purposes

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

#pragma warning (disable : 4267 4311)

#ifdef __CRUDNES_ALLEGRO
	#pragma warning (disable : 4200)
	#include "include/allegro.h"
#elif defined CRUDNES_SDL
	#include <sdl.h>
#endif

#include <math.h>
#include <stdio.h>
#include <time.h>

#include "include/c_tracer.h"
#include "include/c_graphics.h"
#include "include/datatypes.h"
#include "include/c_ppu.h"
#include "include/c_nes.h"

__UINT_8 bPalette [64][3];
__UINT_16 nes_palette [512];

/******************************************************************************/
/** External Data                                                            **/
/******************************************************************************/

extern c_machine *o_machine;
extern c_nes_cpu *o_cpu;

/******************************************************************************/
/** Installer                                                                **/
/******************************************************************************/

c_graphics :: c_graphics (void)
{
	__DBG_INSTALLING ("Graphics");

	active_page = NULL;
	dummy = NULL;

	load_config ();

	clear_to_color (screen, nes_palette [0x0e]);
	gui_bg_color = nes_palette [0x0e];
	gui_fg_color = nes_palette [0x1a];

	__DBG_INSTALLED ();
}

/******************************************************************************/
/** Uninstaller                                                              **/
/******************************************************************************/

c_graphics :: ~c_graphics (void)
{
	__DBG_UNINSTALLING ("Graphics");

	__DBG_UNINSTALLED ();
}

/******************************************************************************/
/** draw_frame ()                                                             **/
/******************************************************************************/

void c_graphics :: draw_frame (void)
{
	static __INT_32 ulEllapsedTime = (__INT_32)(time (0)), ulFPS = 0;

	ulFPS ++;

    if (ulEllapsedTime != time (0))
	{
		char Buffer [10];
		sprintf (Buffer, "%i FPS", ulFPS);
		
		set_window_title (Buffer);
    	ulEllapsedTime = (__INT_32)(time (0));
		ulFPS = 0;
	}

	stretch_blit (active_page, screen, 0, 8, 256, nes->o_cpu->Height, 0, 0, display_w, display_h);
}

#define PDX_MIN(x_offset,y) ((x_offset) < (y) ? (x_offset) : (y))
#define PDX_MAX(x_offset,y) ((x_offset) < (y) ? (y) : (x_offset))

#define PDX_CLAMP(x_offset,min_,max_) PDX_MAX(PDX_MIN(x_offset,max_),min_)

static const double emphasis[8][3] =
{
	{1.000, 1.000, 1.000},
	{1.239, 0.915, 0.743},
	{0.794, 1.086, 0.882},
	{1.019, 0.980, 0.653},
	{0.905, 1.026, 1.277},
	{1.023, 0.908, 0.979},
	{0.741, 0.987, 1.001},
	{0.750, 0.750, 0.750}
};

void ToHSV(double r,double g,double b,double& h,double& s,double& v)
{
	const double min = PDX_MIN( r, PDX_MIN( g, b ));
	const double max = PDX_MAX( r, PDX_MAX( g, b ));

	v = max;

	if (max != 0)
	{
		const double delta = max - min;

		s = delta / max;

		     if (r == max) h = 0 + (g - b) / delta;
		else if (g == max) h = 2 + (b - r) / delta;
		else			   h = 4 + (r - g) / delta;

		h *= 60;

		if (h < 0)
			h += 360;
	}
	else
	{
		s =  0;
		h = -1;
	}
}

void ToRGB(double h,double s,double v,double& r,double& g,double& b)
{
	if (s == 0)
	{
		r = g = b = v;
	}
	else
	{
		h /= 60;
		
		const int i = (int)(floor (h));

		const double f = h - i;
		const double p = v * ( 1 - s );
		const double q = v * ( 1 - s * f );
		const double t = v * ( 1 - s * ( 1 - f ) );

		switch (i) 
		{
     		case 0:  r = v; g = t; b = p; return;
			case 1:  r = q; g = v; b = p; return;
			case 2:  r = p; g = v; b = t; return;
			case 3:  r = p; g = q; b = v; return;
			case 4:  r = t; g = p; b = v; return;
			default: r = v; g = p; b = q; return;
		}
	}
}

void c_graphics :: compute_palette (void)
{
	unsigned char brightness = 128,
				  saturation = 128,
				  hue = 128;

	const double bri = (brightness - 128) / 255.0;
	const double sat = (((saturation - 128) / 255.0) * 2) + 1;
	const int hof = (hue - 128) / 4;

	const unsigned char (*const from)[3] = bPalette;

	for (unsigned short int i=0; i < 8; ++i)
	{
		for (unsigned short int j=0; j < 64; ++j)
		{
			double r = from [j][0] / 255.0;
			double g = from [j][1] / 255.0;
			double b = from [j][2] / 255.0;

			double h,s,v;

			ToHSV (r,g,b,h,s,v);

			s *= sat;
			v += bri;
			h -= hof;

			if (h >= 360)
				h -= 360;
			else if (h < 0)
				h += 360;

			ToRGB (h,s,v,r,g,b);

			r *= emphasis[i][0];
			g *= emphasis[i][1]; 
			b *= emphasis[i][2]; 

			__UINT_8 rr = __UINT_8 (PDX_CLAMP(r * 255,0,255));
			__UINT_8 gg = __UINT_8 (PDX_CLAMP(g * 255,0,255));
			__UINT_8 bb = __UINT_8 (PDX_CLAMP(b * 255,0,255));

            nes_palette [(i * 64) + j] = makecol (rr, gg, bb);
		}
	}
}

void c_graphics :: load_config (void)
{
	config_requested = FALSE;

	color_depth = 16;
    v_sync_enabled = 1;
	display_w = 512;
	display_h = nes->o_cpu->Height * 2;

	set_color_depth (color_depth);
	if (set_gfx_mode (GFX_AUTODETECT_WINDOWED, display_w, display_h, 0, 0) < 0)
	{
		exit(-1);
	}

	active_page = create_bitmap (256 + 8, nes->o_cpu->Height + 16);

	set_display_switch_mode(SWITCH_BACKGROUND);

	gui_fg_color = nes_palette [0x1a];
	gui_bg_color = nes_palette [0x0e];

	nes->o_gfx->compute_palette ();

	text_mode (gui_bg_color);
	set_window_title (APPNAME" "APPVERSION);
	set_window_close_button (FALSE);
	clear_to_color (screen, gui_bg_color);
	show_mouse (screen); 	

	c_tracer o_reader;
    // either Matrixz.pal or Fce.pal
	if (o_reader.set_output_file("Matrixz.pal", __READ))
	{
	    o_reader.set_output_file ("Fce.pal", __READ);
	}
	o_reader.read ((__UINT_8 *) bPalette, 64 * 3);
	o_reader.close ();

	compute_palette ();

	install_keyboard ();
}
