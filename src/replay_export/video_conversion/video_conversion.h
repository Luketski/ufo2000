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

#ifndef VIDEO_CONVERSION_H
#define VIDEO_CONVERSION_H


#include <allegro.h>


void rgb_to_yuv(float r[8][8],float g[8][8],float b[8][8],float y[8][8],float u[8][8],float v[8][8]);
void yuv_to_rgb(float y[8][8],float u[8][8],float v[8][8],float r[8][8],float g[8][8],float b[8][8]);
void rgb__to__yv12 (BITMAP * screen, unsigned char * raw_yv12, unsigned long image_width, unsigned long image_height);




#endif
