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

#include "webm_writer_utils.h"


#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>


//TODO: make it possible to store the special value 0xFF (all bits set, no matter the length of the vint), which is not the same as the number 0xff , which can be stored currently and is stored as 0x40ff


static WEBM_ID_Manager _webm_id_manager;
WEBM_ID_Manager * webm_id_manager = &_webm_id_manager;




WEBM_ID::WEBM_ID (WEBM_Element_ID _id, const char * _signature, const char * _name)
{
  id = _id;
  signature = _signature;
  siglen = strlen (_signature);
  name = _name;
}


WEBM_ID_Manager::WEBM_ID_Manager ()
{
  nids = 0;
  ids = (WEBM_ID **)realloc (0, 0);
  
  // level 0
  Set_ID (WEBM_ID_Void, "\xec", "Void");
  Set_ID (WEBM_ID_EBML_Header, "\x1a\x45\xdf\xa3", "EBML Header");
  Set_ID (WEBM_ID_Segment, "\x18\x53\x80\x67", "Segment");

  // level 1 (EBML Header)
  Set_ID (WEBM_ID_EBML_Version, "\x42\x86", "EBML Version");
  Set_ID (WEBM_ID_EBML_ReadVersion, "\x42\xf7", "EBML Read Version");
  Set_ID (WEBM_ID_EBML_MaxIDLength, "\x42\xf2", "EBML Max ID Length");
  Set_ID (WEBM_ID_EBML_MaxSizeLength, "\x42\xf3", "EBML Max Size Length");
  Set_ID (WEBM_ID_EBML_Doctype, "\x42\x82", "EBML Doctype");
  Set_ID (WEBM_ID_EBML_DoctypeVersion, "\x42\x87", "EBML Doctype Version");
  Set_ID (WEBM_ID_EBML_DoctypeReadVersion, "\x42\x85", "EBML Doctype Read Version");

  // level 1 (Segment)
  Set_ID (WEBM_ID_SeekHead, "\x11\x4d\x9b\x74", "Seek Head");
  Set_ID (WEBM_ID_Info, "\x15\x49\xa9\x66", "Info");
  Set_ID (WEBM_ID_Tracks, "\x16\x54\xae\x6b", "Tracks");
  Set_ID (WEBM_ID_Cues, "\x1c\x53\xbb\x6b", "Cues");
  Set_ID (WEBM_ID_Cluster, "\x1f\x43\xb6\x75", "Cluster");

  // level 2 (Info)
  Set_ID (WEBM_ID_SegmentUID, "\x73\xa4", "Segment UID");
  Set_ID (WEBM_ID_TimecodeScale, "\x2a\xd7\xb1", "Timecode Scale");
  Set_ID (WEBM_ID_Duration, "\x44\x89", "Duration");
  Set_ID (WEBM_ID_DateUTC, "\x44\x61", "Date UTC");
  Set_ID (WEBM_ID_MuxingApp, "\x4d\x80", "Muxing App");
  Set_ID (WEBM_ID_WritingApp, "\x57\x41", "Writing App");

  // level 2 (SeekHead)
  Set_ID (WEBM_ID_Seek, "\x4d\xbb", "Seek");

  // level 3 (Seek)
  Set_ID (WEBM_ID_SeekID, "\x53\xab", "Seek ID");
  Set_ID (WEBM_ID_SeekPosition, "\x53\xac", "Seek Position");

  // level 2 (Tracks)
  Set_ID (WEBM_ID_TrackEntry, "\xae", "Track Entry");

  // level 3 (TrackEntry)
  Set_ID (WEBM_ID_TrackNumber, "\xd7", "Track Number");
  Set_ID (WEBM_ID_TrackUID, "\x73\xc5", "Track UID");
  Set_ID (WEBM_ID_TrackType, "\x83", "Track Type");
  Set_ID (WEBM_ID_DefaultDuration, "\x23\xe3\x83", "Default Duration");
  Set_ID (WEBM_ID_TrackTimecodeScale, "\x23\x31\x4f", "Track Timecode Scale");
  Set_ID (WEBM_ID_CodecID, "\x86", "Codec ID");
  Set_ID (WEBM_ID_CodecName, "\x25\x86\x88", "Codec Name");
  Set_ID (WEBM_ID_Video, "\xe0", "Video");
  Set_ID (WEBM_ID_FlagEnabled, "\xb9", "Flag Enabled");

  // level 4 (Video)
  Set_ID (WEBM_ID_PixelWidth, "\xb0", "Pixel Width");
  Set_ID (WEBM_ID_PixelHeight, "\xba", "Pixel Height");

  // level 2 (Cues)
  Set_ID (WEBM_ID_CuePoint, "\xbb", "Cue Point");

  // level 3 (CuePoint)
  Set_ID (WEBM_ID_CueTime, "\xb3", "Cue Time");
  Set_ID (WEBM_ID_CueTrackPositions, "\xb7", "Cue Track Positions");

  // level 4 (CueTrackPositions)
  Set_ID (WEBM_ID_CueTrack, "\xf7", "Cue Track");
  Set_ID (WEBM_ID_CueClusterPosition, "\xf1", "Cue Cluster Position");
  Set_ID (WEBM_ID_CueBlockNumber, "\x53\x78", "Cue Block Number");

  // level 2 (Cluster)
  Set_ID (WEBM_ID_TimeCode, "\xe7", "Time Code");
  Set_ID (WEBM_ID_Position, "\xa7", "Position");
  Set_ID (WEBM_ID_SimpleBlock, "\xa3", "Simple Block");
  
}

void WEBM_ID_Manager::Set_ID (WEBM_Element_ID id, const char * signature, const char * name)
{
  int last_id = nids;
  
  ids = (WEBM_ID **)realloc (ids, (last_id + 1) * sizeof (WEBM_ID *));
  
  ids[last_id] = new WEBM_ID (id, signature, name);
  
  nids++;
}

WEBM_ID_Manager::~WEBM_ID_Manager ()
{
  for (int i = 0, imax = nids; i < imax; i++) {
    if (ids[i]) {
      delete (ids[i]);
      ids[i] = 0;
    }
  }
  free (ids);
}


const char * WEBM_ID_Manager::Get_ID_Signature (WEBM_Element_ID id)
{
  for (int i = 0, imax = nids; i < imax; i++) {
    if (ids[i]) {
      if (ids[i]->id == id) {
        return ids[i]->signature;
      }
    }
  }
  // if not found, returns WEBM_ID_Void	
  return "\xec";
}


const char * WEBM_ID_Manager::Get_ID_Name (WEBM_Element_ID id)
{
  for (int i = 0, imax = nids; i < imax; i++) {
    if (ids[i]) {
      if (ids[i]->id == id) {
        return ids[i]->name;
      }
    }
  }
  // if not found, returns WEBM_ID_Void	
  return "Void";
}


/**
*	WARNING: FIXME: this may read up to WEBM_MAX_ID_BYTES from the source pointer. needs to be changed to do that only when the ID is unknown
**/
WEBM_Element_ID WEBM_ID_Manager::Signature_2_ID (char * signature)
{
  for (int i = 0, imax = nids; i < imax; i++) {
    if (ids[i]) {
      if (!strncmp (signature, ids[i]->signature, ids[i]->siglen)) {
        return ids[i]->id;
      }
    }
  }
  // if not found, returns WEBM_ID_Void
  return WEBM_ID_Void;
}


int WEBM_ID_Manager::Get_Signature_Length (WEBM_Element_ID id)
{
  for (int i = 0, imax = nids; i < imax; i++) {
    if (ids[i]) {
      if (ids[i]->id == id) {
        return ids[i]->siglen;
      }
    }
  }
  // if not found, returns WEBM_ID_Void	
  return Get_Signature_Length (WEBM_ID_Void);
}



















/**
*	Packs an unsigned number into EBML style 'vint' representation
*
*	returns the length of the resulting 'vint' value (maximum is 8 bytes on 64 bit systems, 5 bytes on 32 bit systems)
**/
int webm_pack_u (byte * vint, largeval num, int depth /*= 0*/)
{
  #ifdef DEBUG
    int i, imax;
    for (i = 0, imax = depth; i < imax; i++) {
      // prefix line with "depth" number of tabs, for nice indentation
      printf ("\t");
    }
    #define PRINTDEBUG(Length)\
    printf ("webm_pack_u: length = %d, num = \\x%02x, (", Length, num);\
    for (i = 0, imax = Length; i < imax; i++)\
    {\
        printf ("\\x%02x", vint[i]);\
    }\
    printf ("), value = %d\n", num);
  #else
      #define PRINTDEBUG(x)
  #endif
    
    
  if (num < (0x80 - 1)) {
    vint[0] = num;
    
    vint[0] &= 0x7f;
    vint[0] |= 0x80;
    
    PRINTDEBUG(1);
    
    return 1;
  }
  else if (num < (0x4000 - 1)) {
    vint[1] = (num) & 0xff;
    vint[0] = (num >> 8) & 0xff;
    
    vint[0] &= 0x3f;
    vint[0] |= 0x40;
    
    PRINTDEBUG(2);
    
    return 2;
  }
  else if (num < (0x200000 - 1)) {
    vint[2] = (num) & 0xff;
    vint[1] = (num >> 8) & 0xff;
    vint[0] = (num >> 16) & 0xff;
    
    vint[0] &= 0x1f;
    vint[0] |= 0x20;
    
    PRINTDEBUG(3);
    
    return 3;
  }
  else if (num < (0x10000000 - 1)) {
    vint[3] = (num) & 0xff;
    vint[2] = (num >> 8) & 0xff;
    vint[1] = (num >> 16) & 0xff;
    vint[0] = (num >> 24) & 0xff;
    
    vint[0] &= 0x0f;
    vint[0] |= 0x10;
    
    PRINTDEBUG(4);
    
    return 4;
  }
  #ifndef WEBM_64BIT
  // store values larger than 0x10000000 in 5 bytes, even on 32 bit systems
  else if (num < (0xFFFFFFFF)) {
    vint[4] = (num) & 0xff;
    vint[3] = (num >> 8) & 0xff;
    vint[2] = (num >> 16) & 0xff;
    vint[1] = (num >> 24) & 0xff;
    vint[0] = 0;
    
    //vint[0] &= 0x07;
    vint[0] |= 0x08;
    
    PRINTDEBUG(5);
    
    return 5;
  }
  #else
  else if (num < (0x0800000000 - 1)) {
    vint[4] = (num) & 0xff;
    vint[3] = (num >> 8) & 0xff;
    vint[2] = (num >> 16) & 0xff;
    vint[1] = (num >> 24) & 0xff;
    vint[0] = (num >> 32) & 0xff;
    
    vint[0] &= 0x07;
    vint[0] |= 0x08;
    
    PRINTDEBUG(5);
    
    return 5;
  }
  else if (num < (0x040000000000 - 1)) {
    vint[5] = (num) & 0xff;
    vint[4] = (num >> 8) & 0xff;
    vint[3] = (num >> 16) & 0xff;
    vint[2] = (num >> 24) & 0xff;
    vint[1] = (num >> 32) & 0xff;
    vint[0] = (num >> 40) & 0xff;
    
    vint[0] &= 0x03;
    vint[0] |= 0x04;
    
    PRINTDEBUG(6);
    
    return 6;
  }
  else if (num < (0x02000000000000 - 1)) {
    vint[6] = (num) & 0xff;
    vint[5] = (num >> 8) & 0xff;
    vint[4] = (num >> 16) & 0xff;
    vint[3] = (num >> 24) & 0xff;
    vint[2] = (num >> 32) & 0xff;
    vint[1] = (num >> 40) & 0xff;
    vint[0] = (num >> 48) & 0xff;
    
    vint[0] &= 0x01;
    vint[0] |= 0x02;
    
    PRINTDEBUG(7);
    
    return 7;
  }
  else if (num < (0x0100000000000000 - 1)) {
    vint[7] = (num) & 0xff;
    vint[6] = (num >> 8) & 0xff;
    vint[5] = (num >> 16) & 0xff;
    vint[4] = (num >> 24) & 0xff;
    vint[3] = (num >> 32) & 0xff;
    vint[2] = (num >> 40) & 0xff;
    vint[1] = (num >> 48) & 0xff;
    vint[0] = (num >> 56) & 0xff;
    
    vint[0] &= 0x00;
    vint[0] |= 0x01;
    
    PRINTDEBUG(8);
    
    return 8;
  }
  #endif

  #undef PRINTDEBUG

  printf ("value %d too large for webm_pack_u() to handle...\n", num);
  exit (-1);
  return -1;
}



/**
*
*	Packs an unsigned number into a big-endian, smallest possible number of bytes used representation
*
*	returns the length of the resulting value (maximum is 8 bytes on 64 bit systems, 4 bytes on 32 bit systems)
*
**/
int webm_pack_bigendian_uint (byte * uint_bigendian, largeval num)
{
  if (num < (0xFF)) {
    uint_bigendian[0] = num;
    
    return 1;
  }
  else if (num < (0xFFFF)) {
    uint_bigendian[1] = (num) & 0xff;
    uint_bigendian[0] = (num >> 8) & 0xff;
    
    return 2;
  }
  else if (num < (0xFFFFFF)) {
    uint_bigendian[2] = (num) & 0xff;
    uint_bigendian[1] = (num >> 8) & 0xff;
    uint_bigendian[0] = (num >> 16) & 0xff;
    
    return 3;
  }
  else if (num < (0xFFFFFFFF)) {
    uint_bigendian[3] = (num) & 0xff;
    uint_bigendian[2] = (num >> 8) & 0xff;
    uint_bigendian[1] = (num >> 16) & 0xff;
    uint_bigendian[0] = (num >> 24) & 0xff;
    
    return 4;
  }
  #ifdef WEBM_64BIT
  else if (num < (0xFFFFFFFFFF)) {
    uint_bigendian[4] = (num) & 0xff;
    uint_bigendian[3] = (num >> 8) & 0xff;
    uint_bigendian[2] = (num >> 16) & 0xff;
    uint_bigendian[1] = (num >> 24) & 0xff;
    uint_bigendian[0] = (num >> 32) & 0xff;

    return 5;
  }
  else if (num < (0xFFFFFFFFFFFF)) {
    uint_bigendian[5] = (num) & 0xff;
    uint_bigendian[4] = (num >> 8) & 0xff;
    uint_bigendian[3] = (num >> 16) & 0xff;
    uint_bigendian[2] = (num >> 24) & 0xff;
    uint_bigendian[1] = (num >> 32) & 0xff;
    uint_bigendian[0] = (num >> 40) & 0xff;

    return 6;
  }
  else if (num < (0xFFFFFFFFFFFFFF)) {
    uint_bigendian[6] = (num) & 0xff;
    uint_bigendian[5] = (num >> 8) & 0xff;
    uint_bigendian[4] = (num >> 16) & 0xff;
    uint_bigendian[3] = (num >> 24) & 0xff;
    uint_bigendian[2] = (num >> 32) & 0xff;
    uint_bigendian[1] = (num >> 40) & 0xff;
    uint_bigendian[0] = (num >> 48) & 0xff;
    
    return 7;
  }
  else if (num < (0xFFFFFFFFFFFFFFFF)) {
    uint_bigendian[7] = (num) & 0xff;
    uint_bigendian[6] = (num >> 8) & 0xff;
    uint_bigendian[5] = (num >> 16) & 0xff;
    uint_bigendian[4] = (num >> 24) & 0xff;
    uint_bigendian[3] = (num >> 32) & 0xff;
    uint_bigendian[2] = (num >> 40) & 0xff;
    uint_bigendian[1] = (num >> 48) & 0xff;
    uint_bigendian[0] = (num >> 56) & 0xff;
    
    return 8;
  }
  #endif

  printf ("value %d too large for webm_pack_bigendian_uint() to handle...\n", num);
  exit (-1);
  return -1;
}




/**
*
*	Packs a 32 bit float number into a big-endian representation
*
*	returns the length of the resulting value in bytes (4)
*
**/
int webm_pack_bigendian_float (byte * float_bigendian, float num)
{
  float_bigendian[0] = ((byte *)&num)[3];
  float_bigendian[1] = ((byte *)&num)[2];
  float_bigendian[2] = ((byte *)&num)[1];
  float_bigendian[3] = ((byte *)&num)[0];
  return 4;
}




/**
*
*	returns the length of the value if it were converted to 'vint' (maximum is 8 bytes)
*
**/
int webm_numsize_u (largeval num)
{
  if (num < (0x80 - 1)) {
    return 1;
  }
  else if (num < (0x4000 - 1)) {
    return 2;
  }
  else if (num < (0x200000 - 1)) {
    return 3;
  }
  else if (num < (0x10000000 - 1)) {
    return 4;
  }
  #ifndef WEBM_64BIT
  // store values larger than 0x10000000, but less than 0x0100000000 in 5 bytes, even on 32 bit systems
  else if (num < (0xFFFFFFFF)) {
    return 5;
  }
  #else
  else if (num < (0x0800000000 - 1)) {
    return 5;
  }
  else if (num < (0x040000000000 - 1)) {
    return 6;
  }
  else if (num < (0x02000000000000 - 1)) {
    return 7;
  }
  else if (num < (0x0100000000000000 - 1)) {
    return 8;
  }
  #endif

  printf ("value %d too large for webm_numsize_u() to handle...\n", num);
  exit (-1);
  return -1;
}





/**
*
*	returns the minimum length required to store the value in memory as a plain 'uint' (maximum is 8 bytes) (these are stored in big-endian byte order in the file)
*
*   it is _not_ an EBML style 'vint', the size of the value is _not_ stored in the value. most likely you are looking for webm_numsize_u() instead
*
**/
int webm_numsize_plain_uint (largeval num)
{
  if (num <= (0xFF)) {
    return 1;
  }
  else if (num <= (0xFFFF)) {
    return 2;
  }
  else if (num <= (0xFFFFFF)) {
    return 3;
  }
  else if (num <= (0xFFFFFFFF)) {
    return 4;
  }
  #ifdef WEBM_64BIT
  else if (num < (0xFFFFFFFFFF)) {
    return 5;
  }
  else if (num < (0xFFFFFFFFFFFF)) {
    return 6;
  }
  else if (num < (0xFFFFFFFFFFFFFF)) {
    return 7;
  }
  else if (num < (0xFFFFFFFFFFFFFFFF)) {
    return 8;
  }
  #endif

  printf ("value %d too large for webm_numsize_plain_uint() to handle...\n", num);
  exit (-1);
  return -1;
}




int webm_numsize_float (float num)
{
  return 4;
}



/**
*
*	returns the length of a 'vint' value, as determined by its first byte (maximum is 8 bytes)
*
**/
int webm_vint_size (byte * vint)
{
  byte num = *vint;

  if (num & 0x80 /* == 0x80*/) {
    return 1;
  }
  else if (num & 0x40) {
    return 2;
  }
  else if (num & 0x20) {
    return 3;
  }
  else if (num & 0x10) {
    return 4;
  }
  else if (num & 0x08) {
    return 5;
  }
  else if (num & 0x04) {
    return 6;
  }
  else if (num & 0x02) {
    return 7;
  }
  else if (num & 0x01) {
    return 8;
  }

  printf ("value is too large for webm_vint_size() to handle...\n");
  exit (-1);
  return -1;
}


/**
*
*	returns the value of a 'vint' (stored as a big endian ebml-style variable width number)
*
**/
largeval webm_vint_val (byte * vint, int depth /*= 0*/)
{
  #ifdef DEBUG
  int i, imax;
  for (i = 0, imax = depth; i < imax; i++)
  {	// prefix line with "depth" number of tabs, for nice indentation
    printf ("\t");
  }
    #define PRINTDEBUG(Length)\
    printf ("webm_vint_val: length = %d, num = \\x%02x, (", Length, num);\
    for (i = 0, imax = Length; i < imax; i++)\
    {\
        printf ("\\x%02x", vint[i]);\
    }\
    printf ("), value = %d\n", val);
  #else
    #define PRINTDEBUG(x)
  #endif

  largeval val = 0;
  byte num = *vint;

  if (num & 0x80) {
    // length 1
    val = vint[0] & 0x7f;
    
    PRINTDEBUG(1);
    
    return val;
  }
  else if (num & 0x40) {
    // length 2
    val =
        (vint[1] )
        | ((vint[0] & 0x3f) << 8)
    ;
    
    PRINTDEBUG(2);
    
    return val;
  }
  else if (num & 0x20) {
    // length 3
    val =
        (vint[2] )
        | ((vint[1]       ) << 8)
        | ((vint[0] & 0x1f) << 16)
    ;
    
    PRINTDEBUG(3);

    
    return val;
  }
  else if (num & 0x10) {
    // length 4
    val =
        (vint[3] )
        | ((vint[2]       ) << 8)
        | ((vint[1]       ) << 16)
        | ((vint[0] & 0x0f) << 24)
    ;
    
    PRINTDEBUG(4);

    
    return val;
  }
  else if (num & 0x08) {
    // length 5
    #ifndef WEBM_64BIT
    // the number must be smaller than 0x0100000000
    if (!(num & 0x07)) {
    #endif
        val =
            (vint[4] )
            | ((vint[3]       ) << 8)
            | ((vint[2]       ) << 16)
            | ((vint[1]       ) << 24)
            #ifdef WEBM_64BIT
            | ((vint[0] & 0x07) << 32)
            #endif
        ;
    
        PRINTDEBUG(5);

    
        return val;
    #ifndef WEBM_64BIT
    }
    #endif
  }
  #ifdef WEBM_64BIT
  else if (num & 0x04) {
    // length 6
    val =
        (vint[5] )
        | ((vint[4]       ) << 8)
        | ((vint[3]       ) << 16)
        | ((vint[2]       ) << 24)
        | ((vint[1]       ) << 32)
        | ((vint[0] & 0x03) << 40)
    ;
    
    PRINTDEBUG(6);

    
    return val;
  }
  else if (num & 0x02) {
    // length 7
    val =
        (vint[6] )
        | ((vint[5]       ) << 8)
        | ((vint[4]       ) << 16)
        | ((vint[3]       ) << 24)
        | ((vint[2]       ) << 32)
        | ((vint[1]       ) << 40)
        | ((vint[0] & 0x01) << 48)
    ;
    
    PRINTDEBUG(7);

    
    return val;
  }
  else if (num & 0x01) {
    // length 8
    val =
        (vint[7] )
        | ((vint[6]       ) << 8)
        | ((vint[5]       ) << 16)
        | ((vint[4]       ) << 24)
        | ((vint[3]       ) << 32)
        | ((vint[2]       ) << 40)
        | ((vint[1]       ) << 48)
        // the first byte is used up completely to store the length of the 'vint', and doesn't carry any of the value itself
    ;
    
    PRINTDEBUG(8);

    
    return val;
  }
  #endif

  #undef PRINTDEBUG

  printf ("webm_vint_val: ");

  for (int i = 0, imax = WEBM_MAX_LENGTH_BYTES; i < imax; i++) {
    printf ("\\x%02x", (unsigned char)vint[i]);
  }

  printf (" (num = \\x%02x) value is too large to handle...\n", num);
  exit (-1);
  return -1;
}


int webm_length_field_size_for_data_size (largeval data_size)
{
  if (data_size < (0x80 - 1 - 1)) {
    return 1;
  }
  else if (data_size < (0x4000 - 1 - 2)) {
    return 2;
  }
  else if (data_size < (0x200000 - 1 - 3)) {
    return 3;
  }
  else if (data_size < (0x10000000 - 1 - 4)) {
    return 4;
  }
  #ifndef WEBM_64BIT
  // store values larger than 0x10000000, but less than 0x0100000000 in 5 bytes, even on 32 bit systems
  else if (data_size < (0xFFFFFFFF - 5)) {
    return 5;
  }
  #else
  else if (data_size < (0x0800000000 - 1 - 5)) {
    return 5;
  }
  else if (data_size < (0x040000000000 - 1 - 6)) {
    return 6;
  }
  else if (data_size < (0x02000000000000 - 1 - 7)) {
    return 7;
  }
  else if (data_size < (0x0100000000000000 - 1 - 8)) {
    return 8;
  }
  #endif

  printf ("value %d too large for size_of_data_with_length_field() to handle...\n", data_size);
  exit (-1);
  return -1;
}