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

#ifndef _WEBM_WRITER_COMMON_H
#define _WEBM_WRITER_COMMON_H

#include <stdint.h>


typedef void * WEBM_FILE;

#define WEBM_MAX_ID_BYTES 4

typedef uint8_t byte; // 8 bits
#ifdef WEBM_64BIT
typedef uint64_t largeval; // 64 bits
#define WEBM_MAX_LENGTH_BYTES 8
#else
typedef uint32_t largeval; // 32 bits
#define WEBM_MAX_LENGTH_BYTES 5
#endif


#endif
