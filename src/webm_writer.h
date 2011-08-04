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



#ifndef _WEBM_WRITER_H
#define _WEBM_WRITER_H


#include <stdint.h>


//#define WEBM_64BIT 1


// reference: http://matroska.org/technical/diagram/index.html
// reference: http://www.webmproject.org/code/specs/container/


#define WEBM_VAR(tttype,vvvarname) tttype vvvarname __attribute__((packed, aligned(1)));


#define WEBM_MAX_ID_BYTES 4


typedef uint8_t byte; // 8 bits
#ifdef WEBM_64BIT
typedef uint64_t largeval; // 64 bits
#define WEBM_MAX_LENGTH_BYTES 8
#else
typedef uint32_t largeval; // 32 bits
#define WEBM_MAX_LENGTH_BYTES 5
#endif


typedef struct
{
	byte EBML_HEADER[4];
	byte EBML_HEADER_length[1];
	struct
	{
		byte EBML_Version[2];
		byte EBML_Version_length[1];
		byte EBML_Version_data[1];

		byte EBML_ReadVersion[2];
		byte EBML_ReadVersion_length[1];
		byte EBML_ReadVersion_data[1];

		byte EBML_MaxIDLength[2];
		byte EBML_MaxIDLength_length[1];
		byte EBML_MaxIDLength_data[1];

		byte EBML_MaxSizeLength[2];
		byte EBML_MaxSizeLength_length[1];
		byte EBML_MaxSizeLength_data[1];

		byte EBML_DocType[2];
		byte EBML_DocType_length[1];
		byte EBML_DocType_data[4];

		byte EBML_DocTypeVersion[2];
		byte EBML_DocTypeVersion_length[1];
		byte EBML_DocTypeVersion_data[1];

		byte EBML_DocTypeReadVersion[2];
		byte EBML_DocTypeReadVersion_length[1];
		byte EBML_DocTypeReadVersion_data[1];
		
		byte Void[1];
		byte Void_length[1];
		byte Void_data[2];
	} EBML_HEADER_data;
} webm_header;




typedef struct ebml_element
{
	byte id [WEBM_MAX_ID_BYTES];
	int id_size;

	byte length [WEBM_MAX_LENGTH_BYTES];	// actual length of this field depends on the value. this is the maximum allowed by the spec
						// when writing to file, the actual length will be known and this field will have the corresponding size
	int length_size;
						
	void * data;
	largeval data_size;

	struct ebml_element * parent;
	struct ebml_element * children;
	
	struct ebml_element * next;
	struct ebml_element * prev;
} pebml_element;



typedef struct
{
	FILE * file;
	
	webm_header * h;
	struct ebml_element * s;
	
	struct ebml_element * current_cluster;
    int frame_time_ms;    
} webm_file;





webm_file * webm_open_file (FILE * file, int video_width, int video_height, int framerate);
FILE * webm_close_file (webm_file * wf);

void webm_write_frame (webm_file * wf, byte * frame_data, largeval data_size, largeval frame_num);


#endif