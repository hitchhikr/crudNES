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

#include <io.h>
#include <stdarg.h> 
#include <stdio.h>
#include <string.h>

#include "include/c_tracer.h"
#include "include/datatypes.h"

c_tracer :: c_tracer ()
{
	destination = __FILE;
	handle = NULL;
}

c_tracer :: c_tracer (const char *Path, EMode Mode, Edestination Type)
{
	set_destination (Type);
	set_output_file (Path, Mode);
}

c_tracer :: ~c_tracer ()
{
	if (destination == __FILE && handle) fclose (handle);
}

__INT_32 c_tracer :: set_output_file (const char *sPath, EMode Type)
{
	destination = __FILE;

	switch (Type)
	{
		case __NEW: handle = fopen (sPath, "wb"); break;
		case __APPEND:
			handle = fopen (sPath, "a"); break;
		case __READ:
			handle = fopen (sPath, "rb");
			if (!handle) return 1;
			break;
	}
	return 0;
}

void c_tracer :: close (void)
{
	if (handle) fclose (handle);
	handle = NULL;
}

void c_tracer :: f_write (const char *sArgDescription, ...)
{
	buffer_pos = 0;
	va_list vl;
	va_start (vl, sArgDescription);
    
	for (__UINT_16 iVar = 0; sArgDescription [iVar] != '\0'; ++iVar)
	{
		union writeUnion {
			__UINT_8 uiByte;
			__UINT_16 uiWord;
			__UINT_32 uiDWord;
			char *string;
		} write;

		switch (sArgDescription [iVar])
		{
			case 'b':
				write.uiByte = va_arg (vl, __UINT_8);
				sprintf (Buffer + buffer_pos, "$%02x", write.uiByte);
				buffer_pos += 3;
				break;

			case 'w':
				write.uiWord = va_arg (vl, __UINT_16);
				sprintf (Buffer + buffer_pos, "$%04x", write.uiWord);
				buffer_pos += 5;
				break;

			case 'n':
				write.uiWord = va_arg (vl, __UINT_16);
				sprintf (Buffer + buffer_pos, "%04x", write.uiWord);
				buffer_pos += 4;
				break;

			case 'l':
				write.uiDWord = va_arg (vl, __UINT_32);
				sprintf (Buffer + buffer_pos, "%08x", write.uiDWord);
				buffer_pos += 8;
				break;

			case 'f':
				write.uiByte = va_arg (vl, __UINT_8);
				sprintf (Buffer + buffer_pos, "%c", write.uiByte);
				buffer_pos ++;
				break;
				
			case 'd':
				write.uiDWord = va_arg (vl, __UINT_32);
				sprintf (Buffer + buffer_pos, "%03i", write.uiDWord);
				buffer_pos += 3;
				break;

			case 's':
				write.string = va_arg (vl, char *);
				sprintf (Buffer + buffer_pos, "%s", write.string);
				buffer_pos += (__UINT_32)(strlen (write.string));
				break;

			default:
				break;
		}
	}

	if(handle)
	{
        fwrite (Buffer, sizeof (__UINT_8), strlen (Buffer), handle);
    }

	va_end (vl);
}

label_dat *c_tracer :: f_read (const char *sArgDescription, ...)
{
    int pos;
	va_list vl;
	va_start (vl, sArgDescription);
        memset(label_read.string, 0, sizeof(label_read.string));
        if(handle)
        {
            pos = fscanf(handle, sArgDescription, label_read.string);
        }
    va_end(vl);
    return(&label_read);
}


void c_tracer :: read (void *buffer, __UINT_32 size)
{
	if (__WINDOW == destination) return;
	fread (buffer, 1, size, handle);
}

void c_tracer :: write (void *buffer, __UINT_32 size)
{
	if (__WINDOW == destination) return;
	fwrite (buffer, 1, size, handle);
}
