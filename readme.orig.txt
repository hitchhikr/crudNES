******************************************************************************   
** aNESe                                                                    **
** Author: Sadai Sarmiento                                                  **
** E-Mail: _hyde_@programmer.net                                            **
** Website: http://hydesprojects.cjb.net/                                   ** 
******************************************************************************

==============================================================================
What's New?
==============================================================================

See Changes.txt for a full list of new additions and fixes.

==============================================================================
Introduction
==============================================================================

Thank you for showing some interest in this project. aNESe is yet another
portable NES emulator (hence its name) coded in C++ that can run many commercial
games, mainly those released in America. My main goal with this project was not to
make the best NES emulator, but to provide those who are thinking about coding
their own NES emulators with a decent reference point. The code has not
yet been fully optimized, but it can run games at playable speeds (see the
requirements section for more details). 

==============================================================================
Capabilities/Features
==============================================================================

- Accurate emulation;
- Speed;
- Uses Shay Green's excellent sound library for high quality sound synthesis;
- NTSC / PAL Emulation;
- Standard controller emulation;
- Mappers supported (fully/partially): 0, 1, 2, 3, 4, 5, 7, 9, 11, 34, 69, 71, 91,
  and maybe others;

There are other features that may not be seen in the latest release (i.e.
they may be disabled for some reason). To that I must say "Wait for future
releases..." 

==============================================================================
Requirements
==============================================================================

I'm not 100% sure of what you would really need to run games at full speed, so
I'm going to have to guess. Here's my PC configuration:

eTower 500 Mhz Celeron 192 MB RAM 128KB of L2 Cache
3D AGP 4MB Video Memory
SoundFusion Audio Card 
Windows XP Professional Edition

I have been able to run all games/demos under my possession at full speed with
the following configuration:

Scanline-based core
8-bit color depth
VSync enabled (Windowed mode)
44100 KHz sound sampling

You will need a much faster computer in order to get games running under the accurate
emulation mode.

Here is my guess for what you would need to run games at full speed with
settings similar to what I mentioned above (no VSync though):

Pentium 386 150 MHz with Windows 95/98/2000/XP
16 MB RAM (a little more would help)
Crappy Video Card
Crappy Sound Card


==============================================================================
Controls
==============================================================================

There is support for only one joypad. This ought to be changed soon though...

A - A Button
S - B Button
Arrows - Direction Pad
Space bar - Select
Enter - Start

F5 - Save state
F6 - Load state
F7 - Decrement current slot number (Total # of slots: 8)
F8 - Increment current slot number
F10 - Soft reset
F12 - Toggle instruction dumper

==============================================================================
Issues / TODO
==============================================================================

- Mapper issues:
  * Some mappers don't work so well under the accurate mode (MMC5);

- TODO:
  * Add input configuration dialog;
  * Fix MMC5 savestate format;
  * I'd kill a cow to get Micro Machines working flawlessly on my emulator;
  * Finish the MMX-optimized PPU core;
  * Release the source code;

==============================================================================
Acknowledgements
==============================================================================

I'd like to thank the following people for their help:

Koitsu, Memblers, Blargg, BT, Jsr, Quietust, Disch, GBAGuy, Max2k3, Tepples,
Loopy, Bananmos, Marty, Drillian, McMartin, TimW, Siloh, TRAC, Kevtris, CCovell,
WhoaMan, Ex3, Jamethiel, ReaperSMS, Mucheserres, Zooka, Bbitmaster, Xodnizel, 
Lenophis, Gavin, Marat, Nessie (if you would like to see your name on this list let me know
about it)...

Special thanks go out there to:

- Authors of wxWindows and Allegro;

- Norix for making VirtuaNES open-source and for writing thousands of lines
  worth of mapper code (I used some of such lines);

- GbaGuy and Max2k3 for being cool doods.

And all others who are constantly posting questions/findings on the NESDev
messageboards. 	