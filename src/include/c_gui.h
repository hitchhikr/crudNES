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

#ifdef __CRUDNES_ALLEGRO_GUI
#ifndef _GUI_H
#define _GUI_H

#pragma warning (disable : 4200)
#include "allegro.h"

#include "datatypes.h"

struct NESGUIColors {

	__INT_32 fgColor,
		   bgColor,
           disabledColor,
           mnuBarFgColor,
		   mnuBarBgColor,
           mnuHBColor,
           btnFgColor,
           btnBgColor,
           shadowBoxColor;

};

extern NESGUIColors colors;

class NESGUIHandler {

	public:

		void Install (void);
		void Uninstall (void);
		
		void setPalette (void);

		void run (void);

		NESGUIColors *color (void) { return &colors; }

		void drawMenu		(int, int, int, int);
		void drawMenuItem	(MENU *, int, int, int, int, int, int);
		int	 drawShadowBox	(int, struct DIALOG *, int);
		int	 drawText		(int, struct DIALOG *, int);
		int	 drawButton		(int, struct DIALOG *, int);
		int  drawList       (int, struct DIALOG *, int);

	private:

		void drawListBox (struct DIALOG *);
		void draw_frame (DIALOG *d, int listsize, int offset, int height, int fg_color, int bg);

		//DATAFILE *dData;

};

#endif
#endif