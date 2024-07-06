crudNES is a Nintendo Entertaining System console emulator made for reverse engineering purposes.
It contains a tracer which records every code and data accesses of the emulated cpu while the game is running (and being played).
It automatically generates disassembled files (and also all necessary files needed to recreate the ROM image) when exiting the emulation.

Obviously, the more the user plays a game, the more accurate the tracing & disassembling is.

crudNES is a heavily modified version of aNESe v4.0a from Sadai Sarmiento (Hyde).

Currrently handled mappers:

- NROM (0) *
- SxROM, MMC1 (1)
- UxROM (2) *
- CNROM (3 87 185) *
- TxROM, MMC3, MMC6 (4 118 119 220)
- ExROM, MMC5 (5)
- AxROM (7) *
- PxROM, MMC2 (9)
- FxROM, MMC4 (10) *
- Color Dreams (11) *
- Bandai EPROM (24C02) (16)
- VRC4a, VRC4c (21)
- BxROM, NINA-001 (34) *
- After Burner (68)
- FME-7, Sunsoft 5B (69)
- Camerica/Codemasters (71) *
- HK-SF3 (91) *

\* Handled by the builtin disassembler.

It's a work in progress so there's still a lot to do:

       - Create bitmap files with the decoded chr roms.
       - Problem with NMI triggering in emulation (DW III & DW IV).
       - Add more mapping to the disassembler (especially MMC1).

       - Need to implement these mappers:

           - 66 (GxROM, MxROM) almost the same as color dreams.
           - 19 (NAMCO106) (Erika to Satoru no Yumebouken, Final Lap,
                       King of Kings, Mappy Kids, Megami Tensei II,
                       Sangokushi 2, Youkai).
           - 23 (VRC2) (Contra (Japan), Tiny Toon Adventures (Japan),
                       Akumajou Special: Boku Dracula Kun).
           - 26 (VRC6) (Akumajou Densetsu, Madara, Esper Dream II,
                       Nigel Mansell's World Challenge,
                       Digital Devil Monogatari II, Hydlide III,
                       Rolling Thunder).
           - 85 (VRC7) (Lagrange Point).

       - Known problems with these games:

           - Akira (hangs) (04/MMC3).
           - Alien Syndrome
             (problem after selection screen (gfx trash) behaviour
              is different if the emulation is PAL or not).
             (118/MMC3).
           - Crisis force (Unknown mapper) (23/VRC2 type B).
           - Digital Devil Monogatari I (Unknown mapper) (76/Namco 109).
           - Digital Devil Monogatari II (Unknown mapper) (19/Namcot 106).
           - Dragon Warrior III (Hangs) (1/MMC1).
           - Dragon Warrior IV (Hangs) (1/MMC1).
           - Esper Dream 2 (Unknown mapper) (26/Konami VRC6 A0-A1 Inverse).
           - Fantasy Zone 2 (Unknown mapper) (67/SunSoft Mapper 4).
           - Fire Emblem (reset after title screen) (10/MMC4).
           - Fire Emblem Gaiden (hangs) (10/MMC4).
           - Getsufuu Maden (Unknown mapper) (23/VRC2 type B).
           - Gradius II (Hangs) (25/VRC4).
           - Harvest Moon (Crashes).
           - Hydlide 3 (Unknown mapper) (19/Namcot 106).
           - Jesus - Kyoufu no Bio Monster (Hangs after title screen) (1/MMC1).
           - Just Breed (Completely bugged visuals) (05/MMC5).
           - Parodius Da! (Unknown mapper) (23/VRC2 type B).
           - Rolling Thunder (Unknown mapper) (19/Namcot 106).
           - Ys III (gfx problems) (118/MMC3).
           - Dragon Ball Z II - Gekishin Freeza!! (hangs after a while) (19/Bandai).
