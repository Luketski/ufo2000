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

#ifndef _WEBM_WRITER_UTILS_H_
#define _WEBM_WRITER_UTILS_H_



#include <stdint.h>


#include "webm_writer_common.h"






enum WEBM_Element_ID // the constructor function WEBM_ID_Manager() must be synched with this enum
{
	// level 0
	WEBM_ID_Void
	, WEBM_ID_EBML_Header
	, WEBM_ID_Segment
	
	// level 1 (EBML Header)
	, WEBM_ID_EBML_Version
	, WEBM_ID_EBML_ReadVersion
	, WEBM_ID_EBML_MaxIDLength
	, WEBM_ID_EBML_MaxSizeLength
	, WEBM_ID_EBML_Doctype
	, WEBM_ID_EBML_DoctypeVersion
	, WEBM_ID_EBML_DoctypeReadVersion
	
	// level 1 (Segment)
	, WEBM_ID_SeekHead
	, WEBM_ID_Info
	, WEBM_ID_Tracks
	, WEBM_ID_Cues
	, WEBM_ID_Cluster

	// level 2 (Info)
	, WEBM_ID_SegmentUID // this element is not present in the current webm spec (version 3)
	, WEBM_ID_TimecodeScale
	, WEBM_ID_Duration
	, WEBM_ID_DateUTC
	, WEBM_ID_MuxingApp
	, WEBM_ID_WritingApp

	// level 2 (SeekHead)
	, WEBM_ID_Seek

	// level 3 (Seek)
	, WEBM_ID_SeekID
	, WEBM_ID_SeekPosition

	// level 2 (Tracks)
	, WEBM_ID_TrackEntry

	// level 3 (TrackEntry)
	, WEBM_ID_TrackNumber
	, WEBM_ID_TrackUID
	, WEBM_ID_TrackType
	, WEBM_ID_DefaultDuration
	, WEBM_ID_TrackTimecodeScale
	, WEBM_ID_CodecID
	, WEBM_ID_CodecName
	, WEBM_ID_Video
	, WEBM_ID_FlagEnabled

	// level 4 (Video)
	, WEBM_ID_PixelWidth
	, WEBM_ID_PixelHeight

	// level 2 (Cues)
	, WEBM_ID_CuePoint

	// level 3 (CuePoint)
	, WEBM_ID_CueTime
	, WEBM_ID_CueTrackPositions

	// level 4 (CueTrackPositions)
	, WEBM_ID_CueTrack
	, WEBM_ID_CueClusterPosition
	, WEBM_ID_CueBlockNumber

	// level 2 (Cluster)
	, WEBM_ID_TimeCode
	, WEBM_ID_Position
	, WEBM_ID_SimpleBlock
};


class WEBM_ID
{
	public:
		WEBM_Element_ID id;
		const char * signature;
		int siglen;
		const char * name;
		
		WEBM_ID (WEBM_Element_ID _id, const char * _signature, const char * _name);
};

class WEBM_ID_Manager
{
	protected:
		WEBM_ID ** ids;
		int nids;

		void Set_ID (WEBM_Element_ID id, const char * signature, const char * name);

	public:
		
		WEBM_ID_Manager ();
		const char * Get_ID_Signature (WEBM_Element_ID id);
		const char * Get_ID_Name (WEBM_Element_ID id);
		WEBM_Element_ID Signature_2_ID (char * signature);
		int Get_Signature_Length (WEBM_Element_ID id);
		~WEBM_ID_Manager ();
};













int webm_pack_u (byte * vint, largeval num, int depth = 0);
int webm_pack_bigendian_uint (byte * uint_bigendian, largeval num);
int webm_pack_bigendian_float (byte * float_bigendian, float num);
int webm_numsize_u (largeval num);
int webm_numsize_plain_uint (largeval num);
int webm_numsize_float (float num);
int webm_vint_size (byte * vint);
largeval webm_vint_val (byte * vint, int depth = 0);
int webm_length_field_size_for_data_size (largeval data_size);


extern WEBM_ID_Manager * webm_id_manager;


#endif