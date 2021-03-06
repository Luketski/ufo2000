/*
This file is part of UFO2000 (http://ufo2000.sourceforge.net)

Copyright (C) 2000-2001  Alexander Ivanov aka Sanami
Copyright (C) 2002       ufo2000 development team

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
Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#ifndef MAINMENU_H
#define MAINMENU_H

#ifdef DJGPP
#error DJGPP is now not supported
#endif

/**
 * Main menu items (shown in reverse order)
 */
enum MAINMENU_ITEMS
{
    MAINMENU_BACKGROUND = 0,
    MAINMENU_YIELD,
    MAINMENU_QUIT,
    MAINMENU_ABOUT,  // simple about-box, as alert()
    MAINMENU_TIP_OF_DAY,
    MAINMENU_OPTIONS,
    MAINMENU_SHOW_REPLAY,
    MAINMENU_EXPORT_REPLAY,
    MAINMENU_LOADGAME,
    MAINMENU_GEOSCAPE,
    MAINMENU_HOTSEAT,
    MAINMENU_INTERNET,
    MAINMENU_COUNT,
    // Currently disabled menu items come next
    MAINMENU_EDITOR, // does not support all terrains
    MAINMENU_DEMO,
    MAINMENU_CONFIG,
    MAINMENU_TOTAL_COUNT
};

void initmainmenu();
int do_mainmenu();

#endif
