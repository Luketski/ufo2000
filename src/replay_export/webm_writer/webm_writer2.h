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



#ifndef _WEBM_WRITER2_H
#define _WEBM_WRITER2_H


#include "webm_writer_common.h"



WEBM_FILE webm_open_file (FILE * file, int video_width, int video_height, int framerate);
FILE * webm_close_file (WEBM_FILE wf);
void webm_write_frame (WEBM_FILE wf, byte * frame_data, largeval data_size, largeval frame_num, bool is_keyframe);
int webm_should_next_frame_be_a_keyframe (WEBM_FILE wf);


#endif