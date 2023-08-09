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
#pragma warning (disable : 4200)
#include "include/allegro.h"
#include "include/c_machine.h"
#include "include/c_nes.h"
#include "include/c_graphics.h"
#include "include/c_gui.h"

/******************************************************************************/
/** External Data                                                            **/
/******************************************************************************/

extern c_machine *o_machine;
extern NESGUIHandler NESGUI;
extern __UINT_16 nes_palette [512];

NESGUIColors colors;

/******************************************************************************/
/** NES_Install (), NES_Uninstall (), NES_continue (), NES_reset () are		 **/
/** events triggered by the either the GUI or special keys.                  **/
/******************************************************************************/

int NES_Install (void);
int NES_Uninstall (void);

int NES_continue (void);
int NES_reset (void);

/******************************************************************************/
/** NES_Uninstall ()														 **/
/**                                                                          **/
/** Uninstalls the machine.													 **/
/******************************************************************************/

int NES_Uninstall (void)
{
	o_machine->close ();

	return D_CLOSE;
}

/******************************************************************************/
/** NES_continue ()                                                          **/
/**                                                                          **/
/** Resumes machine's activity after either a system reset or a GUI event has**/
/** been properly handled.                                                   **/
/******************************************************************************/

int NES_continue (void)
{
	show_mouse (0);
	clear_keybuf ();
	clear_to_color (screen, gui_bg_color);

    o_machine->run ();

	show_mouse (screen);
	clear_keybuf ();

	return D_O_K;
}

int NES_reset (void)
{
	o_machine->reset ();

	return D_O_K;
}

int NES_screenshot (void)
{
	return D_O_K;
}

int NES_saveState (void)
{
	o_machine->save_state ();
	return D_O_K;
}

int NES_loadState (void)
{
	o_machine->load_state ();
	return D_O_K;
}

void AllegDrawMenu (int x_offset, int y, int w, int h)
{
	NESGUI.drawMenu (x_offset, y, w, h);
}

void AllegDrawMenuItem (MENU *m, int x_offset, int y, int w, int h, int bar, int sel)
{
	NESGUI.drawMenuItem (m, x_offset, y, w, h, bar, sel);
}

int AllegDrawShadowBox (int msg, struct DIALOG *d, int c)
{
	return NESGUI.drawShadowBox (msg, d, c);
}

int AllegDrawText (int msg, struct DIALOG *d, int c)
{
	return NESGUI.drawText (msg, d, c);
}

int AllegDrawButton (int msg, DIALOG *d, int c)
{
	return NESGUI.drawButton (msg, d, c);
}

int AllegDrawList (int msg, DIALOG *d, int c)
{
	return NESGUI.drawList (msg, d, c);
}

void NESGUIHandler :: Install (void)
{
	gui_menu_draw_menu_item = AllegDrawMenuItem;
	gui_ctext_proc			= AllegDrawText;
	gui_button_proc			= AllegDrawButton;
	gui_menu_draw_menu		= AllegDrawMenu;
	gui_shadow_box_proc		= AllegDrawShadowBox;
	gui_list_proc = AllegDrawList;

	colors.fgColor = nes_palette [0x10];
	colors.bgColor = nes_palette [0x0c];
	colors.disabledColor = nes_palette [0x10];
	colors.mnuBarFgColor = nes_palette [0x0e];
	colors.mnuBarBgColor = nes_palette [0x20];
	colors.mnuHBColor = nes_palette [0x10];
	colors.btnFgColor = nes_palette [0x10];
	colors.btnBgColor = nes_palette [0x0e];
	colors.shadowBoxColor = nes_palette [0x10];

	gui_fg_color = nes_palette [0x1a];
	gui_bg_color = nes_palette [0x0e];

	setPalette ();

	text_mode (gui_bg_color);
	set_window_title ("aNESe - Another NES Emulator");
	set_window_close_button (FALSE);
	clear_to_color (screen, gui_bg_color);
	show_mouse (screen); 	
}

void NESGUIHandler :: Uninstall (void)
{
	set_gfx_mode (GFX_TEXT, 0, 0, 0, 0);
}

void NESGUIHandler :: setPalette (void)
{
	nes->o_gfx->compute_palette ();
}

void NESGUIHandler :: run (void)
{

}

void NESGUIHandler :: drawMenu (int x_offset, int y, int w, int h)
{
	rectfill(screen, x_offset, y, x_offset+w-1, y+h-1, colors.bgColor);
}

int NESGUIHandler :: drawShadowBox (int msg, struct DIALOG *d, int c)
{
   if (msg==MSG_DRAW) {
      int fg = (d->flags & D_DISABLED) ? colors.fgColor : colors.disabledColor;
      rectfill(screen, d->x+1, d->y+1, d->x+d->w-3, d->y+d->h-3, colors.bgColor);
      rect(screen, d->x, d->y, d->x+d->w-2, d->y+d->h-2, gui_bg_color);
      vline(screen, d->x+d->w-1, d->y+1, d->y+d->h-1, gui_bg_color);
      hline(screen, d->x+1, d->y+d->h-1, d->x+d->w-1, gui_bg_color);
   }

   return D_O_K;
}

int NESGUIHandler :: drawText (int msg, struct DIALOG *d, int c)
{
	if (msg==MSG_DRAW) {
		int rtm;
		int fg = (d->flags & D_DISABLED) ? colors.fgColor : colors.disabledColor;

		rtm = text_mode (colors.bgColor);
		gui_textout(screen, (char *)d->dp, d->x, d->y, fg, TRUE);
		text_mode (rtm);
	}

	return D_O_K;
}

static char* split_around_tab(const char *s, char **tok1, char **tok2)
{
	char *buf, *last;
	char tmp[16];

	buf = ustrdup((char *)s);
	*tok1 = ustrtok_r(buf, uconvert_ascii("\t", tmp), &last);
	*tok2 = ustrtok_r(NULL, empty_string, &last);

	return buf;
}

void NESGUIHandler :: drawMenuItem (MENU *m, int x_offset, int y, int w, int h, int bar, int sel)
{
	int fg, bg;
	char *buf, *tok1, *tok2;
	int my;
	int rtm;

	if (m->flags & D_DISABLED) {
		if (sel) {
			fg = gui_mg_color;
			bg = gui_fg_color;
		}
		else {
			fg = gui_mg_color;
			bg = gui_bg_color;
		} 
	}
	else {
		if (sel) {
			fg = colors.mnuBarFgColor;
			bg = colors.mnuBarBgColor; 
		}
		else {
			fg = colors.fgColor;
			bg = colors.bgColor;
		} 
	}
	
	rectfill(screen, x_offset, y, x_offset+w-1, y+text_height(font)+2, bg);
	rtm = text_mode(bg);

	if (ugetc(m->text)) {

		buf = split_around_tab(m->text, &tok1, &tok2);
		gui_textout(screen, tok1, x_offset+8, y+1, fg, FALSE);

		if (tok2)
			gui_textout(screen, tok2, x_offset+w-gui_strlen(tok2)-10, y+1, fg, FALSE);

		if ((m->child) && (!bar)) {
			my = y + text_height(font)/2;
			hline(screen, x_offset+w-8, my+1, x_offset+w-4, fg);
			hline(screen, x_offset+w-8, my+0, x_offset+w-5, fg);
			hline(screen, x_offset+w-8, my-1, x_offset+w-6, fg);
			hline(screen, x_offset+w-8, my-2, x_offset+w-7, fg);
			putpixel(screen, x_offset+w-8, my-3, fg);
			hline(screen, x_offset+w-8, my+2, x_offset+w-5, fg);
			hline(screen, x_offset+w-8, my+3, x_offset+w-6, fg);
			hline(screen, x_offset+w-8, my+4, x_offset+w-7, fg);
			putpixel(screen, x_offset+w-8, my+5, fg);
		}

		free(buf);
	}
	else
	{
		hline(screen, x_offset, y+text_height(font)/2+2, x_offset+w, colors.shadowBoxColor);
	}

	if (m->flags & D_SELECTED) {
		line(screen, x_offset+1, y+text_height(font)/2+1, x_offset+3, y+text_height(font)+1, fg);
		line(screen, x_offset+3, y+text_height(font)+1, x_offset+6, y+2, fg);
	}

	text_mode(rtm);
}

int NESGUIHandler :: drawButton (int msg, DIALOG *d, int c)
{
   int state1, state2;
   int black;
   int swap;
   int g;
   int rtm;

   switch (msg) {

      case MSG_DRAW:
	 if (d->flags & D_SELECTED) {
	    g = 1;
	    state1 = colors.btnFgColor;
	    state2 = (d->flags & D_DISABLED) ? gui_mg_color : colors.btnBgColor;
	 }
	 else {
	    g = 0; 
	    state1 = (d->flags & D_DISABLED) ? gui_mg_color : colors.btnBgColor;
	    state2 = colors.btnFgColor;
	 }

	 rectfill(screen, d->x+1+g, d->y+1+g, d->x+d->w-3+g, d->y+d->h-3+g, state2);
	 rect(screen, d->x+g, d->y+g, d->x+d->w-2+g, d->y+d->h-2+g, state1);
	 rtm = text_mode(-1);
	 gui_textout(screen, (char *)d->dp, d->x+d->w/2+g, d->y+d->h/2-text_height(font)/2+g, state1, TRUE);
	 text_mode(rtm);

	 if (d->flags & D_SELECTED) {
	    vline(screen, d->x, d->y, d->y+d->h-2, d->bg);
	    hline(screen, d->x, d->y, d->x+d->w-2, d->bg);
	 }
	 else {
	    black = makecol(0,0,0);
	    vline(screen, d->x+d->w-1, d->y+1, d->y+d->h-2, black);
	    hline(screen, d->x+1, d->y+d->h-1, d->x+d->w-1, black);
	 }
	 break;

      case MSG_WANTFOCUS:
	 return D_WANTFOCUS;

      case MSG_KEY:
	 /* close dialog? */
	 if (d->flags & D_EXIT) {
	    return D_CLOSE;
	 }

	 /* or just toggle */
	 d->flags ^= D_SELECTED;
	 scare_mouse();
	 object_message(d, MSG_DRAW, 0);
	 unscare_mouse();
	 break;

      case MSG_CLICK:
	 /* what state was the button originally in? */
	 state1 = d->flags & D_SELECTED;
	 if (d->flags & D_EXIT)
	    swap = FALSE;
	 else
	    swap = state1;

	 /* track the mouse until it is released */
	 while (gui_mouse_b()) {
	    state2 = ((gui_mouse_x() >= d->x) && (gui_mouse_y() >= d->y) &&
		      (gui_mouse_x() < d->x + d->w) && (gui_mouse_y() < d->y + d->h));
	    if (swap)
	       state2 = !state2;

	    /* redraw? */
	    if (((state1) && (!state2)) || ((state2) && (!state1))) {
	       d->flags ^= D_SELECTED;
	       state1 = d->flags & D_SELECTED;
	       scare_mouse();
	       object_message(d, MSG_DRAW, 0);
	       unscare_mouse();
	    }

	    /* let other objects continue to animate */
	    broadcast_dialog_message(MSG_IDLE, 0);
	 }

	 /* should we close the dialog? */
	 if ((d->flags & D_SELECTED) && (d->flags & D_EXIT)) {
	    d->flags ^= D_SELECTED;
	    return D_CLOSE;
	 }
	 break; 
   }

   return D_O_K;
}

void _handle_scrollable_scroll_click(DIALOG *d, int listsize, int *offset, int height)
{
   int xx, yy;
   int hh = d->h - 5;

   while (gui_mouse_b()) {
      int i = (hh * height + listsize/2) / listsize;
      int len = (hh * (*offset) + listsize/2) / listsize + 2;

      if ((gui_mouse_y() >= d->y+len) && (gui_mouse_y() <= d->y+len+i)) {
	 xx = gui_mouse_y() - len + 2;
	 while (gui_mouse_b()) {
	    yy = (listsize * (gui_mouse_y() - xx) + hh/2) / hh;
	    if (yy > listsize-height) 
	       yy = listsize-height;

	    if (yy < 0) 
	       yy = 0;

	    if (yy != *offset) {
	       *offset = yy;
	       scare_mouse();
	       object_message(d, MSG_DRAW, 0);
	       unscare_mouse();
	    }

	    /* let other objects continue to animate */
	    broadcast_dialog_message(MSG_IDLE, 0);
	 }
      }
      else {
	 if (gui_mouse_y() <= d->y+len) 
	    yy = *offset - height;
	 else 
	    yy = *offset + height;

	 if (yy > listsize-height) 
	    yy = listsize-height;

	 if (yy < 0) 
	    yy = 0;

	 if (yy != *offset) {
	    *offset = yy;
	    scare_mouse();
	    object_message(d, MSG_DRAW, 0);
	    unscare_mouse();
	 }
      }

      /* let other objects continue to animate */
      broadcast_dialog_message(MSG_IDLE, 0);
   }
}

/* _handle_scrollable_scroll:
 *  Helper function to scroll through a scrollable object.
 */
void _handle_scrollable_scroll(DIALOG *d, int listsize, int *index, int *offset)
{
   int height = (d->h-4) / text_height(font);

   if (listsize <= 0) {
      *index = *offset = 0;
      return;
   }

   /* check selected item */
   if (*index < 0) 
      *index = 0;
   else if (*index >= listsize)
      *index = listsize - 1;

   /* check scroll position */
   while ((*offset > 0) && (*offset + height > listsize))
      (*offset)--;

   if (*offset >= *index) {
      if (*index < 0) 
	 *offset = 0;
      else
	 *offset = *index;
   }
   else {
      while ((*offset + height - 1) < *index)
	 (*offset)++;
   }
}

typedef char *(*getfuncptr)(int, int *);

void NESGUIHandler :: drawListBox (DIALOG * d)
{
   int height, listsize, i, len, bar, x_offset, y, w;
   int fg_color, fg, bg;
   char *sel = (char *)(d->dp2);
   char s[1024];
   int rtm;

   (*(getfuncptr)(d->dp))(-1, &listsize);
   height = (d->h-4) / text_height(font);
   bar = (listsize > height);
   w = (bar ? d->w-15 : d->w-3);
   fg_color = (d->flags & D_DISABLED) ? gui_mg_color : d->fg;

   /* draw box contents */
   for (i=0; i<height; i++) {
      if (d->d2+i < listsize) {
	 if (d->d2+i == d->d1) {
	    fg = d->bg;
	    bg = fg_color;
	 } 
	 else if ((sel) && (sel[d->d2+i])) { 
	    fg = d->bg;
	    bg = gui_mg_color;
	 }
	 else {
	    fg = fg_color;
	    bg = d->bg;
	 }
	 ustrzcpy(s, sizeof(s), (*(getfuncptr)d->dp)(i+d->d2, NULL));
	 x_offset = d->x + 2;
	 y = d->y + 2 + i*text_height(font);
	 rtm = text_mode(bg);
	 rectfill(screen, x_offset, y, x_offset+7, y+text_height(font)-1, bg); 
	 x_offset += 8;
	 len = ustrlen(s);
	 while (text_length(font, s) >= MAX(d->w - 1 - (bar ? 22 : 10), 1)) {
	    len--;
	    usetat(s, len, 0);
	 }
	 textout(screen, font, s, x_offset, y, fg);
	 text_mode(rtm);
	 x_offset += text_length(font, s);
	 if (x_offset <= d->x+w) 
	    rectfill(screen, x_offset, y, d->x+w, y+text_height(font)-1, bg);
      }
      else {
	 rectfill(screen, d->x+2,  d->y+2+i*text_height(font), 
		  d->x+w, d->y+1+(i+1)*text_height(font), d->bg);
      }
   }

   if (d->y+2+i*text_height(font) <= d->y+d->h-3)
      rectfill(screen, d->x+2, d->y+2+i*text_height(font), 
				       d->x+w, d->y+d->h-3, d->bg);

   /* draw frame, maybe with scrollbar */
   draw_frame (d, listsize, d->d2, height, fg_color, d->bg);
}

void NESGUIHandler :: draw_frame (DIALOG *d, int listsize, int offset, int height, int fg_color, int bg)
{
	   int i, len;
   BITMAP *pattern;
   int xx, yy;


   /* possibly draw scrollbar */
   if (listsize > height) {
//      vline(screen, d->x_offset+d->w-13, d->y+1, d->y+d->h-2, fg_color);

      /* scrollbar with focus */ 
//   if (d->flags & D_GOTFOCUS) {
//	 dotted_rect(d->x_offset+1, d->y+1, d->x_offset+d->w-14, d->y+d->h-2, fg_color, bg);
//	 dotted_rect(d->x_offset+d->w-12, d->y+1, d->x_offset+d->w-2, d->y+d->h-2, fg_color, bg);
//      }
//    else {
	 rect(screen, d->x+1, d->y+1, d->x+d->w-14, d->y+d->h-2, 0x13);//colors.fgColor);
	 rect(screen, d->x+d->w-12, d->y+1, d->x+d->w-2, d->y+d->h-2, colors.fgColor);
//      }

      /* create and draw the scrollbar */
      pattern = create_bitmap(2, 2);
      i = ((d->h-5) * height + listsize/2) / listsize;
      xx = d->x+d->w-11;
      yy = d->y+2;

      putpixel(pattern, 0, 1, colors.fgColor);
      putpixel(pattern, 1, 0, colors.fgColor);
      putpixel(pattern, 0, 0, colors.fgColor);
      putpixel(pattern, 1, 1, colors.fgColor);

      if (offset > 0) {
	 len = (((d->h-5) * offset) + listsize/2) / listsize;
	 rectfill(screen, xx, yy, xx+8, yy+len, bg);
	 yy += len;
      }
      if (yy+i < d->y+d->h-3) {
	 drawing_mode(DRAW_MODE_COPY_PATTERN, pattern, 0, 0);
	 rectfill(screen, xx, yy, xx+8, yy+i, 0);
	 solid_mode();
	 yy += i+1;
	 rectfill(screen, xx, yy, xx+8, d->y+d->h-3, bg);
      }
      else {
	 drawing_mode(DRAW_MODE_COPY_PATTERN, pattern, 0, 0);
	 rectfill(screen, xx, yy, xx+8, d->y+d->h-3, 0);
	 solid_mode();
      }
      destroy_bitmap(pattern);
   }
   else {
      /* no scrollbar necessary */
      if (d->flags & D_GOTFOCUS);
      else
	 rect(screen, d->x+1, d->y+1, d->x+d->w-2, d->y+d->h-2, bg);
   }
}

static void idle_cb(void)
{
   broadcast_dialog_message(MSG_IDLE, 0);
}

void _handle_listbox_click(DIALOG *d)
{
   char *sel = (char *)(d->dp2);
   int listsize, height;
   int i, j;

   (*(getfuncptr)d->dp)(-1, &listsize);
   if (!listsize)
      return;

   height = (d->h-4) / text_height(font);

   i = MID(0, ((gui_mouse_y() - d->y - 2) / text_height(font)), 
	      ((d->h-4) / text_height(font) - 1));
   i += d->d2;
   if (i < d->d2)
      i = d->d2;
   else {
      if (i > d->d2 + height-1)
	 i = d->d2 + height-1;
      if (i >= listsize)
	 i = listsize-1;
   }

   if (gui_mouse_y() <= d->y)
      i = MAX(i-1, 0);
   else if (gui_mouse_y() >= d->y+d->h-1)
      i = MIN(i+1, listsize-1);

   if (i != d->d1) {
      if (sel) {
	 if (key_shifts & (KB_SHIFT_FLAG | KB_CTRL_FLAG)) {
	    if ((key_shifts & KB_SHIFT_FLAG) || (d->flags & D_INTERNAL)) {
	       for (j=MIN(i, d->d1); j<=MAX(i, d->d1); j++)
		  sel[j] = TRUE;
	    }
	    else
	       sel[i] = !sel[i];
	 }
      }

      d->d1 = i;
      i = d->d2;
      _handle_scrollable_scroll(d, listsize, &d->d1, &d->d2);

      d->flags |= D_DIRTY;

      if (i != d->d2)
	 rest_callback(MID(10, text_height(font)*16-d->h-1, 100), idle_cb);
   }
}

int NESGUIHandler :: drawList (int msg, DIALOG *d, int c)
{
   int listsize, i, bottom, height, bar, orig;
   char *sel = (char *)(d->dp2);
   int redraw = FALSE;

   switch (msg) {

      case MSG_START:
	 (*(getfuncptr)d->dp)(-1, &listsize);
	 _handle_scrollable_scroll(d, listsize, &d->d1, &d->d2);
	 break;

      case MSG_DRAW:
	 drawListBox(d);
	 break;

      case MSG_CLICK:
	 (*(getfuncptr)d->dp)(-1, &listsize);
	 height = (d->h-4) / text_height(font);
	 bar = (listsize > height);
	 if ((!bar) || (gui_mouse_x() < d->x+d->w-13)) {
	    if ((sel) && (!(key_shifts & KB_CTRL_FLAG))) {
	       for (i=0; i<listsize; i++) {
		  if (sel[i]) {
		     redraw = TRUE;
		     sel[i] = FALSE;
		  }
	       }
	       if (redraw) {
		  scare_mouse();
		  object_message(d, MSG_DRAW, 0);
		  unscare_mouse();
	       }
	    }
	    _handle_listbox_click(d);
	    while (gui_mouse_b()) {
	       broadcast_dialog_message(MSG_IDLE, 0);
	       d->flags |= D_INTERNAL;
	       _handle_listbox_click(d);
	       d->flags &= ~D_INTERNAL;
	    }
	 }
	 else {
	    _handle_scrollable_scroll_click(d, listsize, &d->d2, height);
	 }
	 break;

      case MSG_DCLICK:
	 (*(getfuncptr)d->dp)(-1, &listsize);
	 height = (d->h-4) / text_height(font);
	 bar = (listsize > height);
	 if ((!bar) || (gui_mouse_x() < d->x+d->w-13)) {
	    if (d->flags & D_EXIT) {
	       if (listsize) {
		  i = d->d1;
		  object_message(d, MSG_CLICK, 0);
		  if (i == d->d1) 
		     return D_CLOSE;
	       }
	    }
	 }
	 break;

      case MSG_WHEEL:
	 (*(getfuncptr)d->dp)(-1, &listsize);
	 height = (d->h-4) / text_height(font);
	 if (height < listsize) {
	    int delta = (height > 3) ? 3 : 1;
	    if (c > 0) 
	       i = MAX(0, d->d2-delta);
	    else
	       i = MIN(listsize-height, d->d2+delta);
	    if (i != d->d2) {
	       d->d2 = i;
	       scare_mouse();
	       object_message(d, MSG_DRAW, 0);
	       unscare_mouse(); 
	    }
	 }
	 break;

      case MSG_KEY:
	 (*(getfuncptr)d->dp)(-1, &listsize);
	 if ((listsize) && (d->flags & D_EXIT))
	    return D_CLOSE;
	 break;

      case MSG_WANTFOCUS:
	 return D_WANTFOCUS;

      case MSG_CHAR:
	 (*(getfuncptr)d->dp)(-1, &listsize);

	 if (listsize) {
	    c >>= 8;

	    bottom = d->d2 + (d->h-4)/text_height(font) - 1;
	    if (bottom >= listsize-1)
	       bottom = listsize-1;

	    orig = d->d1;

	    if (c == KEY_UP)
	       d->d1--;
	    else if (c == KEY_DOWN)
	       d->d1++;
	    else if (c == KEY_HOME)
	       d->d1 = 0;
	    else if (c == KEY_END)
	       d->d1 = listsize-1;
	    else if (c == KEY_PGUP) {
	       if (d->d1 > d->d2)
		  d->d1 = d->d2;
	       else
		  d->d1 -= (bottom - d->d2) ? bottom - d->d2 : 1;
	    }
	    else if (c == KEY_PGDN) {
	       if (d->d1 < bottom)
		  d->d1 = bottom;
	       else
		  d->d1 += (bottom - d->d2) ? bottom - d->d2 : 1;
	    } 
	    else 
	       return D_O_K;

	    if (sel) {
	       if (!(key_shifts & (KB_SHIFT_FLAG | KB_CTRL_FLAG))) {
		  for (i=0; i<listsize; i++)
		     sel[i] = FALSE;
	       }
	       else if (key_shifts & KB_SHIFT_FLAG) {
		  for (i=MIN(orig, d->d1); i<=MAX(orig, d->d1); i++) {
		     if (key_shifts & KB_CTRL_FLAG)
			sel[i] = (i != d->d1);
		     else
			sel[i] = TRUE;
		  }
	       }
	    }

	    /* if we changed something, better redraw... */ 
	    _handle_scrollable_scroll(d, listsize, &d->d1, &d->d2);
	    d->flags |= D_DIRTY;
	    return D_USED_CHAR;
	 }
	 break;
   }

   return D_O_K;
}
#endif
