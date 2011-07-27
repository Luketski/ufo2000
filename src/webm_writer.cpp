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

#include <stdio.h>
#include <string>
#include <string.h>
#include <stdlib.h>


#include "webm_writer.h"


//#define DEBUG 1


// reference: http://matroska.org/technical/diagram/index.html
// reference: http://www.webmproject.org/code/specs/container/


// level 0
static const char *  WEBM_ID_Segment =			"\x18\x53\x80\x67";
static const char *  WEBM_ID_Void =			"\xec";

// level 1 (Segment)
static const char *  WEBM_ID_SeekHead = 		"\x11\x4d\x9b\x74";
static const char *  WEBM_ID_Info = 			"\x15\x49\xa9\x66";
static const char *  WEBM_ID_Tracks = 			"\x16\x54\xae\x6b";
static const char *  WEBM_ID_Cues = 			"\x1c\x53\xbb\x6b";
static const char *  WEBM_ID_Cluster = 			"\x1f\x43\xb6\x75";

// level 2 (Info)
static const char *  WEBM_ID_SegmentUID =		"\x73\xa4"; // not in the current webm spec
static const char *  WEBM_ID_TimecodeScale =		"\x2a\xd7\xb1";
static const char *  WEBM_ID_Duration =			"\x44\x89";
static const char *  WEBM_ID_DateUTC =			"\x44\x61";
static const char *  WEBM_ID_MuxingApp =		"\x4d\x80";
static const char *  WEBM_ID_WritingApp =		"\x57\x41";

// level 2 (SeekHead)
static const char *  WEBM_ID_Seek =			"\x4d\xbb";

// level 3 (Seek)
static const char *  WEBM_ID_SeekID =			"\x53\xab";
static const char *  WEBM_ID_SeekPosition =		"\x53\xac";

// level 2 (Tracks)
static const char *  WEBM_ID_TrackEntry = 		"\xae";

// level 3 (TrackEntry)
static const char *  WEBM_ID_TrackNumber = 		"\xd7";
static const char *  WEBM_ID_TrackUID = 		"\x73\xc5";
static const char *  WEBM_ID_TrackType = 		"\x83";
static const char *  WEBM_ID_DefaultDuration = 		"\x23\xe3\x83";
static const char *  WEBM_ID_TrackTimecodeScale = 	"\x23\x31\x4f";
static const char *  WEBM_ID_CodecID = 			"\x86";
static const char *  WEBM_ID_CodecName = 		"\x25\x86\x88";
static const char *  WEBM_ID_Video = 			"\xe0";
static const char *  WEBM_ID_FlagEnabled = 		"\xb9";

// level 4 (Video)
static const char *  WEBM_ID_PixelWidth = 		"\xb0";
static const char *  WEBM_ID_PixelHeight = 		"\xba";

// level 2 (Cues)
static const char *  WEBM_ID_CuePoint = 		"\xbb";

// level 3 (CuePoint)
static const char *  WEBM_ID_CueTime = 			"\xb3";
static const char *  WEBM_ID_CueTrackPositions = 	"\xb7";

// level 4 (CueTrackPositions)
static const char *  WEBM_ID_CueTrack = 		"\xf7";
static const char *  WEBM_ID_CueClusterPosition = 	"\xf1";
static const char *  WEBM_ID_CueBlockNumber = 		"\x53\x78";

// level 2 (Cluster)
static const char *  WEBM_ID_TimeCode = 		"\xe7";
static const char *  WEBM_ID_Position = 		"\xa7";
static const char *  WEBM_ID_SimpleBlock = 		"\xa3";





#define WEBM_ID_LENGTH(id) (strlen ((char *)id))



/**
*
*	returns the human readable name of a WEBM / EBML element id, if known
*	WARNING: FIXME: this may read up to WEBM_MAX_LENGTH_BYTES from the source pointer. needs to be changed to do that only when the ID is unknown
*
**/
const char * webm_get_id_name (byte * id)
{
    int i, imax;
    int idlength = 0;
    char id2 [WEBM_MAX_LENGTH_BYTES + 1];
    for (i = 0, imax = WEBM_MAX_LENGTH_BYTES; i < imax; i++)
    {
        id2[i] = id[i];
        if (!((unsigned char)id[i]))
        {
            break;
        }
    }
    idlength = i;
    id2 [WEBM_MAX_LENGTH_BYTES] = 0;
    
    //printf ("got here, idlength = %d\n", idlength);
    for (i = 1, imax = idlength; i <= imax; i++)
    {
        /*
        if (i == 1)
        {
            printf ("comparing \\x%02x with \\x%02x, the result is %d\n", (unsigned char)id2[0], (unsigned char)WEBM_ID_SimpleBlock[0], strncmp (id2, WEBM_ID_SimpleBlock, i));
        }
        */
        
        if (!strncmp (id2, WEBM_ID_Segment, i)) return "Segment";
        if (!strncmp (id2, WEBM_ID_Void, i)) return "Void";
        if (!strncmp (id2, WEBM_ID_SeekHead, i)) return "SeekHead";
        if (!strncmp (id2, WEBM_ID_Seek, i)) return "Seek";
        if (!strncmp (id2, WEBM_ID_SeekID, i)) return "SeekID";
        if (!strncmp (id2, WEBM_ID_SeekPosition, i)) return "SeekPosition";
        if (!strncmp (id2, WEBM_ID_Info, i)) return "Info";
        if (!strncmp (id2, WEBM_ID_Tracks, i)) return "Tracks";
        if (!strncmp (id2, WEBM_ID_Cues, i)) return "Cues";
        if (!strncmp (id2, WEBM_ID_Cluster, i)) return "Cluster";
        if (!strncmp (id2, WEBM_ID_TrackEntry, i)) return "TrackEntry";
        if (!strncmp (id2, WEBM_ID_TrackNumber, i)) return "TrackNumber";
        if (!strncmp (id2, WEBM_ID_TrackUID, i)) return "TrackUID";
        if (!strncmp (id2, WEBM_ID_TrackType, i)) return "TrackType";
        if (!strncmp (id2, WEBM_ID_DefaultDuration, i)) return "DefaultDuration";
        if (!strncmp (id2, WEBM_ID_TrackTimecodeScale, i)) return "TrackTimecodeScale";
        if (!strncmp (id2, WEBM_ID_CodecID, i)) return "CodecID";
        if (!strncmp (id2, WEBM_ID_CodecName, i)) return "CodecName";
        if (!strncmp (id2, WEBM_ID_Video, i)) return "Video";
        if (!strncmp (id2, WEBM_ID_PixelWidth, i)) return "PixelWidth";
        if (!strncmp (id2, WEBM_ID_PixelHeight, i)) return "PixelHeight";
        if (!strncmp (id2, WEBM_ID_CuePoint, i)) return "CuePoint";
        if (!strncmp (id2, WEBM_ID_CueTime, i)) return "CueTime";
        if (!strncmp (id2, WEBM_ID_CueTrackPositions, i)) return "CueTrackPositions";
        if (!strncmp (id2, WEBM_ID_CueTrack, i)) return "CueTrack";
        if (!strncmp (id2, WEBM_ID_CueClusterPosition, i)) return "CueClusterPosition";
        if (!strncmp (id2, WEBM_ID_CueBlockNumber, i)) return "CueBlockNumber";
        if (!strncmp (id2, WEBM_ID_TimeCode, i)) return "TimeCode";
        if (!strncmp (id2, WEBM_ID_Position, i)) return "Position";
        if (!strncmp (id2, WEBM_ID_SimpleBlock, i)) return "SimpleBlock";
    }
    
    //"UNKNOWN ID ()" = length 13
    static char printval [13 + (WEBM_MAX_LENGTH_BYTES * 4) + 1];
    sprintf (printval, "UNKNOWN ID (");
    for (i = 0, imax = WEBM_MAX_LENGTH_BYTES; i < imax; i++)
    {
        sprintf (printval + 12 + (i * 4), "\\x%02x", (unsigned char)id[i]);
        printval [12 + (i * 4) + 4] = ')';
        printval [12 + (i * 4) + 5] = 0;
        if (!((unsigned char)id[i]))
        {
            break;
        }
    }
    printval [13 + (WEBM_MAX_LENGTH_BYTES * 4)] = 0;
    
    return printval;
}






/**
*	Packs an unsigned number into EBML style 'vint' representation
*
*	returns the length of the resulting 'vint' value (maximum is 8 bytes on 64 bit systems, 5 bytes on 32 bit systems)
**/
int webm_pack_u (byte * vint, largeval num, int depth = 0)
{
    
    #ifdef DEBUG
    int i, imax;
    for (i = 0, imax = depth; i < imax; i++)
    {	// prefix line with "depth" number of tabs, for nice indentation
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
    
    
    if (num < (0x80 - 1))
    {
        vint[0] = num;
        
        vint[0] &= 0x7f;
        vint[0] |= 0x80;
        
        PRINTDEBUG(1);
        
        return 1;
    }
    else if (num < (0x4000 - 1))
    {
        vint[1] = (num) & 0xff;
        vint[0] = (num >> 8) & 0xff;
        
        vint[0] &= 0x3f;
        vint[0] |= 0x40;
        
        PRINTDEBUG(2);
        
        return 2;
    }
    else if (num < (0x200000 - 1))
    {
        vint[2] = (num) & 0xff;
        vint[1] = (num >> 8) & 0xff;
        vint[0] = (num >> 16) & 0xff;
        
        vint[0] &= 0x1f;
        vint[0] |= 0x20;
        
        PRINTDEBUG(3);
        
        return 3;
    }
    else if (num < (0x10000000 - 1))
    {
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
    else if (num < (0xFFFFFFFF))
    {
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
    else if (num < (0x0800000000 - 1))
    {
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
    else if (num < (0x040000000000 - 1))
    {
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
    else if (num < (0x02000000000000 - 1))
    {
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
    else if (num < (0x0100000000000000 - 1))
    {
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
*	returns the length of the value if it were converted to 'vint' (maximum is 8 bytes)
*
**/
int webm_numsize_u (largeval num)
{
    if (num < (0x80 - 1))
    {
        return 1;
    }
    else if (num < (0x4000 - 1))
    {
        return 2;
    }
    else if (num < (0x200000 - 1))
    {
        return 3;
    }
    else if (num < (0x10000000 - 1))
    {
        return 4;
    }
    #ifndef WEBM_64BIT
    // store values larger than 0x10000000, but less than 0x0100000000 in 5 bytes, even on 32 bit systems
    else if (num < (0xFFFFFFFF))
    {
        return 5;
    }
    #else
    else if (num < (0x0800000000 - 1))
    {
        return 5;
    }
    else if (num < (0x040000000000 - 1))
    {
        return 6;
    }
    else if (num < (0x02000000000000 - 1))
    {
        return 7;
    }
    else if (num < (0x0100000000000000 - 1))
    {
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
**/
int webm_numsize_plain_uint (largeval num)
{
    if (num <= (0xFF))
    {
        return 1;
    }
    else if (num <= (0xFFFF))
    {
        return 2;
    }
    else if (num <= (0xFFFFFF))
    {
        return 3;
    }
    else if (num <= (0xFFFFFFFF))
    {
        return 4;
    }
    #ifdef WEBM_64BIT
    else if (num < (0xFFFFFFFFFF))
    {
        return 5;
    }
    else if (num < (0xFFFFFFFFFFFF))
    {
        return 6;
    }
    else if (num < (0xFFFFFFFFFFFFFF))
    {
        return 7;
    }
    else if (num < (0xFFFFFFFFFFFFFFFF))
    {
        return 8;
    }
    #endif
    
    printf ("value %d too large for webm_numsize_plain_uint() to handle...\n", num);
    exit (-1);
    return -1;
}




/**
*
*	returns the length of a 'vint' value, as determined by its first byte (maximum is 8 bytes)
*
**/
int webm_vint_size (byte * vint)
{
    byte num = *vint;
    
    if (num & 0x80 /* == 0x80*/)
    {
        return 1;
    }
    else if (num & 0x40)
    {
        return 2;
    }
    else if (num & 0x20)
    {
        return 3;
    }
    else if (num & 0x10)
    {
        return 4;
    }
    else if (num & 0x08)
    {
        return 5;
    }
    else if (num & 0x04)
    {
        return 6;
    }
    else if (num & 0x02)
    {
        return 7;
    }
    else if (num & 0x01)
    {
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
largeval webm_vint_val (byte * vint, int depth = 0)
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
    
    if (num & 0x80)
    {	// length 1
        val = vint[0] & 0x7f;
        
        PRINTDEBUG(1);
        
        return val;
    }
    else if (num & 0x40)
    {	// length 2
        val =
            (vint[1] )
            | ((vint[0] & 0x3f) << 8)
        ;
        
        PRINTDEBUG(2);
        
        return val;
    }
    else if (num & 0x20)
    {	// length 3
        val =
            (vint[2] )
            | ((vint[1]       ) << 8)
            | ((vint[0] & 0x1f) << 16)
        ;
        
        PRINTDEBUG(3);

        
        return val;
    }
    else if (num & 0x10)
    {	// length 4
        val =
            (vint[3] )
            | ((vint[2]       ) << 8)
            | ((vint[1]       ) << 16)
            | ((vint[0] & 0x0f) << 24)
        ;
        
        PRINTDEBUG(4);

        
        return val;
    }
    else if (num & 0x08)
    {	// length 5
        #ifndef WEBM_64BIT
        // the number must be smaller than 0x0100000000
        if (!(num & 0x07))
        {
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
    else if (num & 0x04)
    {	// length 6
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
    else if (num & 0x02)
    {	// length 7
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
    else if (num & 0x01)
    {	// length 8
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

    for (int i = 0, imax = WEBM_MAX_LENGTH_BYTES; i < imax; i++)
    {
        printf ("\\x%02x", (unsigned char)vint[i]);
    }
    
    printf (" (num = \\x%02x) value is too large to handle...\n", num);
    exit (-1);
    return -1;
}




ebml_element * ebml_get_last_child (ebml_element * parent, int * pnkids = 0, int depth = 0)
{
    #ifdef DEBUG
    int i, imax;
    #endif
    
    if (!parent->children)
    {
        #ifdef DEBUG
        for (i = 0, imax = depth; i < imax; i++)
        {	// prefix line with "depth" number of tabs, for nice indentation
            printf ("\t");
        }
        printf ("ebml_get_last_child: element %s has no kids\n", webm_get_id_name (parent->id));
        #endif
        
        return 0;
    }
    
    ebml_element * last = parent->children;
    
    int nkids = 1;
    while (last->next)
    {
        last = last->next;
        nkids++;
    }
    
    #ifdef DEBUG
    for (i = 0, imax = depth; i < imax; i++)
    {	// prefix line with "depth" number of tabs, for nice indentation
        printf ("\t");
    }
    printf ("ebml_get_last_child: element %s has %d kids\n", webm_get_id_name (parent->id), nkids);
    #endif
    
    if (pnkids)
    {
        *pnkids = nkids;
    }
    
    return last;
}

void ebml_setid (ebml_element * elem, byte * id)
{
    int id_size = WEBM_ID_LENGTH (id);
    memcpy (elem->id, id, id_size);
    elem->id_size = id_size;
}


void ebml_setdata_stringn (ebml_element * elem, byte * string, largeval length, largeval padding = 0)
{
    largeval memsize = length + padding;
    if (!elem->data)
    {	// data is not set
        elem->data = (byte *)malloc (memsize);
        if (!elem->data)
        {
            printf ("ebml_setdata_stringn: was unable to allocate data for ebml_element (%d) bytes\n", memsize);
            return;
        }
    }
    else
    {	// data already set, need realloc
        void * newp = realloc (elem->data, memsize);
        if (!newp)
        {
            printf ("ebml_setdata_stringn: was unable to re-allocate data for ebml_element (%d) bytes\n", memsize);
            return;
        }
        elem->data = newp;
    }
    memcpy (((byte*)elem->data) + padding, string, length);
    elem->data_size = memsize;
}

void ebml_setdata_string (ebml_element * elem, byte * string)
{
    largeval length = strlen ((char *)string);
    ebml_setdata_stringn (elem, string, length);
}


/**
*
*	stores a number in big endian byte order as the element's data
*
**/
void ebml_setdata_plain_uint (ebml_element * elem, largeval num)
{
    int length = webm_numsize_plain_uint (num);	
    if (!elem->data)
    {	// data is not set
        elem->data = (byte *)malloc (length);
        if (!elem->data)
        {
            printf ("ebml_setdata_plain_uint: was unable to allocate data for ebml_element (%d) bytes\n", length);
            return;
        }
    }
    else
    {	// data already set, need realloc
        void * newp = realloc (elem->data, length);
        if (!newp)
        {
            printf ("ebml_setdata_plain_uint: was unable to re-allocate data for ebml_element (%d) bytes\n", length);
            return;
        }
        elem->data = newp;
    }
    for (int i = 0, imax = length; i < imax; i++)
    {
        *(((unsigned char *)elem->data) + i) = (unsigned char)(num >> ((length - i - 1) * 8));
    }
    elem->data_size = length;
}


/**
*
*	this function may never be required
*
**/
void ebml_setdata_vint (ebml_element * elem, largeval num)
{
    int length = webm_numsize_u (num);	
    if (!elem->data)
    {	// data is not set
        elem->data = (byte *)malloc (length);
        if (!elem->data)
        {
            printf ("ebml_setdata_vint: was unable to allocate data for ebml_element (%d) bytes\n", length);
            return;
        }
    }
    else
    {	// data already set, need realloc
        void * newp = realloc (elem->data, length);
        if (!newp)
        {
            printf ("ebml_setdata_vint: was unable to re-allocate data for ebml_element (%d) bytes\n", length);
            return;
        }
        elem->data = newp;
    }
    webm_pack_u ((byte *)elem->data, num);
    elem->data_size = length;
}


void ebml_unsetdata (ebml_element * elem)
{
    if (elem->data)
    {
        free (elem->data);
        elem->data = 0;
        elem->data_size = 0;
    }
}



ebml_element * ebml_add_child (ebml_element * parent, byte * id = 0)
{
    ebml_element * newelem = (ebml_element *) malloc (sizeof (ebml_element));
    if (!newelem)
    {
        printf ("error: was unable to allocate ebml_element struct (%d) bytes\n", sizeof (ebml_element));
        return 0;
    }
    memset (newelem, 0, sizeof (ebml_element));
    
    ebml_element * kids = parent->children;
    ebml_element * last = 0;

    newelem->parent = parent;

    if (!kids)
    {	// this is the first child of parent
        parent->children = newelem;
        
        #ifdef DEBUG
        printf ("added the first child to %s: %s\n", webm_get_id_name (parent->id), webm_get_id_name (id));
        #endif
    }
    else
    {	// parent has kids, find last one
        #ifdef DEBUG
        int nkids = 0;
        #endif
        
        last = ebml_get_last_child
        (
            parent
            #ifdef DEBUG
            , &nkids
            #endif
        );
        
        // add this to the end
        last->next = newelem;
        newelem->prev = last;
        
        #ifdef DEBUG
        printf ("added %d-th child to %s: %s\n", (nkids + 1), webm_get_id_name (parent->id), webm_get_id_name (id));
        #endif
    }
    
    if (id)
    {
        ebml_setid (newelem, id);
    }
    
    return newelem;
}



// length of element when written to disk, use this when "data_size" is already calculated, or when it doesn't have children
largeval ebml_length (ebml_element * elem)
{
    return elem->id_size + webm_numsize_u (elem->data_size) + elem->data_size;
}

// length of element when written to disk, calculates "data_size" by adding up all its descendants recursively
largeval ebml_tree_length (ebml_element * elem, int depth = 0, largeval * subtree_size_only = 0)
{
    largeval total = 0;
    ebml_element * kid = elem->children; //ebml_get_last_child (elem, 0, depth);

    #ifdef DEBUG
    char datasizestr[128];
    memset (datasizestr, 0, 128);
    #endif
    
    if (!kid)
    {
        total = ebml_length (elem);
        
        #ifdef DEBUG
        sprintf (datasizestr, "(data_size = %d)", total);
        #endif
    }
    
    #ifdef DEBUG
    for (int i = 0, imax = depth; i < imax; i++)
    {	// prefix line with "depth" number of tabs, for nice indentation
        printf ("\t");
    }
    printf ("ebml_tree_length: element %s has %skids %s\n", webm_get_id_name (elem->id), (kid ? "" : "no "), (!kid ? datasizestr : ""));
    #endif
    
    if (kid)
    {	// has kids
        for (; kid; kid = kid->next)
        {
            total += ebml_tree_length (kid, depth + 1);
        }
        
        if (subtree_size_only)
        {
            *subtree_size_only = total;
        }
        return elem->id_size + webm_numsize_u (total) + total;
    }
    
    // has data (no kids)
    if (subtree_size_only)
    {
        *subtree_size_only = elem->data_size;
    }
    return ebml_length (elem);
}






/**
*	Writes a webm_file resource to a file
**/

largeval ebml_write_tree (FILE * file, ebml_element * elem, int depth = 0)
{
    #ifdef DEBUG
    int i, imax;
    byte element_name[WEBM_MAX_ID_BYTES + 1];
    memcpy (element_name, elem->id, elem->id_size);
    element_name[elem->id_size] = 0;
    for (i = 0, imax = depth; i < imax; i++)
    {	// prefix line with "depth" number of tabs, for nice indentation
        printf ("\t");
    }
    printf ("writing %s...\n", webm_get_id_name (elem->id));
    /*
    for (i = 0, imax = elem->id_size; i < imax; i++)
    {
        printf ("\\x%02x", (unsigned char)element_name[i]);
    }
    */
    #endif
    
    fwrite (elem->id, 1, elem->id_size, file);
    
    largeval size_of_data = 0;
    ebml_element * kid = elem->children;
    if (kid)
    {	// has kids (write them out recursively)
        #ifdef DEBUG
        for (i = 0, imax = depth; i < imax; i++)
        {	// prefix line with "depth" number of tabs, for nice indentation
            printf ("\t");
        }
        printf ("%s has_kids, measuring tree length...\n", webm_get_id_name (elem->id));
        #endif
        
        largeval subtree_size;
        largeval tree_length = ebml_tree_length (elem, depth, &subtree_size);
        elem->length_size = webm_pack_u (elem->length, subtree_size, depth);
        fwrite (elem->length, 1, elem->length_size, file);

        #ifdef DEBUG
        for (i = 0, imax = depth; i < imax; i++)
        {	// prefix line with "depth" number of tabs, for nice indentation
            printf ("\t");
        }
        printf ("%s tree length = %d (%d), length_size = %d\n", webm_get_id_name (elem->id), webm_vint_val (elem->length, depth), tree_length, elem->length_size);
        #endif

        #ifdef DEBUG
        int nthkid = 0;
        #endif
        
        for (; kid; kid = kid->next)
        {
            #ifdef DEBUG
            nthkid++;
            for (i = 0, imax = depth; i < imax; i++)
            {	// prefix line with "depth" number of tabs, for nice indentation
                printf ("\t");
            }
            printf ("descending into %d-th kid of %s...\n", nthkid, webm_get_id_name (elem->id));
            #endif
            
            size_of_data += ebml_write_tree (file, kid, depth + 1);
        }
    }
    else
    {	// has data
        elem->length_size = webm_pack_u (elem->length, elem->data_size);
        fwrite (elem->length, 1, elem->length_size, file);

        #ifdef DEBUG
        for (i = 0, imax = depth; i < imax; i++)
        {	// prefix line with "depth" number of tabs, for nice indentation
            printf ("\t");
        }
        printf ("%s has data only, length_size = %d, data_size = %d\n", webm_get_id_name (elem->id), elem->length_size, elem->data_size);
        #endif
        
        fwrite (elem->data, 1, elem->data_size, file);
        size_of_data += elem->data_size;
    }
    
    return elem->id_size + elem->length_size + size_of_data;
}

void webm_write_file (FILE * file, webm_file * wf)
{
    fwrite (wf->h, 1, sizeof (webm_header), file);
    ebml_write_tree (file, wf->s);
}




/**
*
*	Creates a new webm_file resource
*
*	a webm_file resource is returned, which must be destroyed with webm_destroy_webmfile() to free it's memory
**/
webm_file * webm_open_file (FILE * file, int video_width, int video_height, int framerate) // framerate gives the duration of a frame in "1 second / (framerate)" , where 1 is 1 second and n is number of frames
{								// FIXME: actually use the framerate param
    webm_file * wf = (webm_file*)malloc (sizeof (webm_file));
    if (!wf)
    {
        printf ("unable to allocate webm_file struct\n");
        exit (-2);
    }
    
    wf->h = (webm_header*)malloc (sizeof (webm_header));
    if (!wf->h)
    {
        free (wf);
        printf ("unable to allocate webm_file->webm_header struct\n");
        exit (-2);
    }

    wf->s = (ebml_element*)malloc (sizeof (ebml_element));
    if (!wf->s)
    {
        free (wf->h);
        free (wf);
        printf ("unable to allocate webm_file->webm_segment struct\n");
        exit (-2);
    }
    
    
    wf->file = file;
    

    
    // Header
    memcpy (wf->h->EBML_HEADER, "\x1a\x45\xdf\xa3", 4);
    memcpy (wf->h->EBML_HEADER_length, "\xa3", 1); // 31
        memcpy (wf->h->EBML_HEADER_data.EBML_Version, "\x42\x86", 2);
        memcpy (wf->h->EBML_HEADER_data.EBML_Version_length, "\x81", 1); // 1
        memcpy (wf->h->EBML_HEADER_data.EBML_Version_data, "\x03", 1); // EBML version 3

        memcpy (wf->h->EBML_HEADER_data.EBML_ReadVersion, "\x42\xf7", 2);
        memcpy (wf->h->EBML_HEADER_data.EBML_ReadVersion_length, "\x81", 1); // 1
        memcpy (wf->h->EBML_HEADER_data.EBML_ReadVersion_data, "\x01", 1); // parser can be EBML version 1

        memcpy (wf->h->EBML_HEADER_data.EBML_MaxIDLength, "\x42\xf2", 2);
        memcpy (wf->h->EBML_HEADER_data.EBML_MaxIDLength_length, "\x81", 1); // 1
        memcpy (wf->h->EBML_HEADER_data.EBML_MaxIDLength_data, "\x04", 1); // length of longest ID in this file (FIXME: this should be determined dynamically)

        memcpy (wf->h->EBML_HEADER_data.EBML_MaxSizeLength, "\x42\xf3", 2);
        memcpy (wf->h->EBML_HEADER_data.EBML_MaxSizeLength_length, "\x81", 1); // 1
        memcpy (wf->h->EBML_HEADER_data.EBML_MaxSizeLength_data, "\x08", 1); // length of longest length field in this file

        memcpy (wf->h->EBML_HEADER_data.EBML_DocType, "\x42\x82", 2);
        memcpy (wf->h->EBML_HEADER_data.EBML_DocType_length, "\x84", 1); // 4
        memcpy (wf->h->EBML_HEADER_data.EBML_DocType_data, "webm", 4); // doctype
        
        memcpy (wf->h->EBML_HEADER_data.EBML_DocTypeVersion, "\x42\x87", 2);
        memcpy (wf->h->EBML_HEADER_data.EBML_DocTypeVersion_length, "\x81", 1); // 1
        memcpy (wf->h->EBML_HEADER_data.EBML_DocTypeVersion_data, "\x01", 1); // WEBM version 1

        memcpy (wf->h->EBML_HEADER_data.EBML_DocTypeReadVersion, "\x42\x85", 2);
        memcpy (wf->h->EBML_HEADER_data.EBML_DocTypeReadVersion_length, "\x81", 1); // 1
        memcpy (wf->h->EBML_HEADER_data.EBML_DocTypeReadVersion_data, "\x01", 1); // parser can be WEBM version 1
        
        memcpy (wf->h->EBML_HEADER_data.Void, "\xec", 1);
        memcpy (wf->h->EBML_HEADER_data.Void_length, "\x82", 1); // 2
        memcpy (wf->h->EBML_HEADER_data.Void_data, "\x00\x00", 2);
        
    
    
    memset (wf->s, 0, sizeof (ebml_element));
    ebml_setid (wf->s, (byte *)WEBM_ID_Segment);
    
    ebml_element * segment = wf->s;
    
    ebml_element * seekhead = ebml_add_child (segment, (byte *)WEBM_ID_SeekHead);
    ebml_element * info = ebml_add_child (segment, (byte *)WEBM_ID_Info);
    ebml_element * tracks = ebml_add_child (segment, (byte *)WEBM_ID_Tracks);

        // 1 trackentry for video
        // 1 trackentry for audio (TBD)
        ebml_element * trackentry = ebml_add_child (tracks, (byte *)WEBM_ID_TrackEntry);
        
            ebml_element * tracknum = ebml_add_child (trackentry, (byte *)WEBM_ID_TrackNumber);
                ebml_setdata_string (tracknum, (byte *)"\x01");	// track 1
            ebml_element * trackuid = ebml_add_child (trackentry, (byte *)WEBM_ID_TrackUID);
                ebml_setdata_string (trackuid, (byte *)"asdf1234"); // uid // FIXME: this should be randomly generated
            ebml_element * tracktype = ebml_add_child (trackentry, (byte *)WEBM_ID_TrackType);
                ebml_setdata_string (tracktype, (byte *)"\x01"); // video
            ebml_element * defaultdur = ebml_add_child (trackentry, (byte *)WEBM_ID_DefaultDuration);
                ebml_setdata_string (defaultdur, (byte *)"\x02\x7c\x6b\x5a");	// framerate, a float value, this sets the duration of 1 frame (in nanoseconds)
                                    // the value is 1.8544844412968288e-37
            ebml_element * tracktcs = ebml_add_child (trackentry, (byte *)WEBM_ID_TrackTimecodeScale);
                ebml_setdata_stringn (tracktcs, (byte *)"\x3f\x80\x00\x00", 4);	// speed scaling, a float value, is 1.0
            ebml_element * codecid = ebml_add_child (trackentry, (byte *)WEBM_ID_CodecID);
                ebml_setdata_string (codecid, (byte *)"V_VP8");
            ebml_element * codecname = ebml_add_child (trackentry, (byte *)WEBM_ID_CodecName);
                ebml_setdata_string (codecname, (byte *)"VP8");
            ebml_element * video = ebml_add_child (trackentry, (byte *)WEBM_ID_Video);

                ebml_element * pixelwidth = ebml_add_child (video, (byte *)WEBM_ID_PixelWidth);
                    ebml_setdata_plain_uint (pixelwidth, video_width); // size of the video in pixels
                ebml_element * pixelheight = ebml_add_child (video, (byte *)WEBM_ID_PixelHeight);
                    ebml_setdata_plain_uint (pixelheight, video_height); // size of the video in pixels
            
            //ebml_element * flagenabled = ebml_add_child (trackentry, (byte *)WEBM_ID_FlagEnabled);
            //	ebml_setdata_string (flagenabled, (byte *)"\x01"); // true
                    

    ebml_element * cues = ebml_add_child (segment, (byte *)WEBM_ID_Cues);

        // there can be many of these, these allow seeking in the video
        // (?) at least one of them must be present
        ebml_element * cuepoint0 = ebml_add_child (cues, (byte *)WEBM_ID_CuePoint);

            ebml_element * cuetime0 = ebml_add_child (cuepoint0, (byte *)WEBM_ID_CueTime);
                ebml_setdata_plain_uint (cuetime0, 0);
            ebml_element * cuetrackpos0 = ebml_add_child (cuepoint0, (byte *)WEBM_ID_CueTrackPositions);

                ebml_element * ctp0_cuetrack = ebml_add_child (cuetrackpos0, (byte *)WEBM_ID_CueTrack);
                    ebml_setdata_plain_uint (ctp0_cuetrack, 1);
                ebml_element * ctp0_cueclusterposition = ebml_add_child (cuetrackpos0, (byte *)WEBM_ID_CueClusterPosition);
                    // FIXME: don't assume the length of this value. this is only a temporary solution
                    ebml_setdata_stringn (ctp0_cueclusterposition, (byte *)"\x00\x00", 2); // its real value will be set a bit later below, and we assume here that it will fit in 2 bytes
                //ebml_element * ctp0_cueblocknumber = ebml_add_child (cuetrackpos0, (byte *)WEBM_ID_CueBlockNumber);
                    // FIXME: this needs a value (though the first cue may omit this field)

        ebml_element * cuepoint1 = ebml_add_child (cues, (byte *)WEBM_ID_CuePoint);

            ebml_element * cuetime1 = ebml_add_child (cuepoint1, (byte *)WEBM_ID_CueTime);
                ebml_setdata_stringn (cuetime1, (byte *)"\x07\x42", 2);
            ebml_element * cuetrackpos1 = ebml_add_child (cuepoint1, (byte *)WEBM_ID_CueTrackPositions);

                ebml_element * ctp1_cuetrack = ebml_add_child (cuetrackpos1, (byte *)WEBM_ID_CueTrack);
                    ebml_setdata_plain_uint (ctp1_cuetrack, 1);
                ebml_element * ctp1_cueclusterposition = ebml_add_child (cuetrackpos1, (byte *)WEBM_ID_CueClusterPosition);
                    // FIXME: don't assume the length of this value. this is only a temporary solution
                    ebml_setdata_stringn (ctp1_cueclusterposition, (byte *)"\x00\x00", 2); // its real value will be set a bit later below, and we assume here that it will fit in 2 bytes
                ebml_element * ctp1_cueblocknumber = ebml_add_child (cuetrackpos1, (byte *)WEBM_ID_CueBlockNumber);
                    ebml_setdata_stringn (ctp1_cueblocknumber, (byte *)"\x01\x80", 2);


        
    // there can be many clusters. a cluster is usually as long as to hold as many frames as can be encoded in memory at the same time, and then written to disk all at once, which is usually set to around 2 MB
    // clusters hold the video and audio frames
    ebml_element * cluster0 = ebml_add_child (wf->s, (byte *)WEBM_ID_Cluster);
        
        ebml_element * cluster0_timecode = ebml_add_child (cluster0, (byte *)WEBM_ID_TimeCode); // Absolute timecode of the cluster (based on TimecodeScale).  (the first cluster is usually at time 0 )
            ebml_setdata_plain_uint (cluster0_timecode, 0);
        ebml_element * cluster0_position = ebml_add_child (cluster0, (byte *)WEBM_ID_Position); // The Position of the Cluster in the segment (0 in live broadcast streams). It might help to resynchronise offset on damaged streams.
            ebml_setdata_plain_uint
            (
                cluster0_position
                ,
                0xffff // FIXME: don't assume it will be 2 bytes long
            ); // position of the cluster, relative to Segment _data_ start. this will be set below
    

            
    // set SeekHead values
    
    // FIXME: a nicer solution is expected
        // 6(?) Level 1 elements
    #define SEEKHEAD_SIZE (40 * 6)
    // 40 bytes for each SeekHead entry (each Level 1 element)

    // position is relative to SeekHead's start (actually the beginning of Segment's _data_)

    largeval SEEKHEAD_INFO_RELATIVE_POSITION =	(SEEKHEAD_SIZE);
    largeval SEEKHEAD_TRACKS_RELATIVE_POSITION =	(SEEKHEAD_INFO_RELATIVE_POSITION + ebml_tree_length (info));
    largeval SEEKHEAD_CUES_RELATIVE_POSITION =	(SEEKHEAD_TRACKS_RELATIVE_POSITION + ebml_tree_length (tracks));
    largeval SEEKHEAD_CLUSTER_RELATIVE_POSITION =	(SEEKHEAD_CUES_RELATIVE_POSITION + ebml_tree_length (cues));
    
    // 7 = 4 bytes SeekHead ID + 3 bytes Length field
    largeval SEEKHEAD_INFO_POSITION	= 7 + SEEKHEAD_INFO_RELATIVE_POSITION;
    largeval SEEKHEAD_TRACKS_POSITION = 7 + SEEKHEAD_TRACKS_RELATIVE_POSITION;
    largeval SEEKHEAD_CUES_POSITION = 7 + SEEKHEAD_CUES_RELATIVE_POSITION;
    largeval SEEKHEAD_CLUSTER_POSITION = 7 + SEEKHEAD_CLUSTER_RELATIVE_POSITION;

    #ifdef DEBUG
    printf ("SeekHead: SEEKHEAD_SIZE: %d\n", SEEKHEAD_SIZE);
    #endif


    ebml_element * seek_info = ebml_add_child (seekhead, (byte *)WEBM_ID_Seek);
    ebml_element * seek_tracks = ebml_add_child (seekhead, (byte *)WEBM_ID_Seek);
    ebml_element * seek_cues = ebml_add_child (seekhead, (byte *)WEBM_ID_Seek);
    ebml_element * seek_cluster0 = ebml_add_child (seekhead, (byte *)WEBM_ID_Seek);
    
    ebml_element * seekid_info = ebml_add_child (seek_info, (byte *)WEBM_ID_SeekID);
        ebml_setdata_stringn (seekid_info, (byte *)WEBM_ID_Info, 4); // this must be the level 1 element's name
    ebml_element * seekposition_info = ebml_add_child (seek_info, (byte *)WEBM_ID_SeekPosition);
        ebml_setdata_plain_uint (seekposition_info, SEEKHEAD_INFO_POSITION); // this must be its position
    
    #ifdef DEBUG
    printf ("SeekHead: Info position: %d\n", SEEKHEAD_INFO_POSITION);
    #endif

    ebml_element * seekid_tracks = ebml_add_child (seek_tracks, (byte *)WEBM_ID_SeekID);
        ebml_setdata_stringn (seekid_tracks, (byte *)WEBM_ID_Tracks, 4); // this must be the level 1 element's name
    ebml_element * seekposition_tracks = ebml_add_child (seek_tracks, (byte *)WEBM_ID_SeekPosition);
        ebml_setdata_plain_uint (seekposition_tracks, SEEKHEAD_TRACKS_POSITION); // this must be its position

    #ifdef DEBUG
    printf ("SeekHead: Tracks position: %d\n", SEEKHEAD_TRACKS_POSITION);
    #endif
        
    ebml_element * seekid_cues = ebml_add_child (seek_cues, (byte *)WEBM_ID_SeekID);
        ebml_setdata_stringn (seekid_cues, (byte *)WEBM_ID_Cues, 4); // this must be the level 1 element's name
    ebml_element * seekposition_cues = ebml_add_child (seek_cues, (byte *)WEBM_ID_SeekPosition);
        ebml_setdata_plain_uint (seekposition_cues, SEEKHEAD_CUES_POSITION); // this must be its position

    #ifdef DEBUG
    printf ("SeekHead: Cues position: %d\n", SEEKHEAD_CUES_POSITION);
    #endif
        
    ebml_element * seekid_cluster0 = ebml_add_child (seek_cluster0, (byte *)WEBM_ID_SeekID);
        ebml_setdata_stringn (seekid_cluster0, (byte *)WEBM_ID_Cluster, 4); // this must be the level 1 element's name
    ebml_element * seekposition_cluster0 = ebml_add_child (seek_cluster0, (byte *)WEBM_ID_SeekPosition);
        ebml_setdata_plain_uint (seekposition_cluster0, SEEKHEAD_CLUSTER_POSITION); // this must be its position

    #ifdef DEBUG
    printf ("SeekHead: Cluster position: %d\n", SEEKHEAD_CLUSTER_POSITION);
    #endif
        
        
    int head_voidsize = SEEKHEAD_SIZE - (ebml_tree_length (seekhead) - 4 - 1); // data_size == 0, length field is all zeros, which is 1 byte, +4 bytes for the id
    int vs_len_size = webm_numsize_u (head_voidsize);
    while (vs_len_size != webm_numsize_u (head_voidsize - vs_len_size))
    {
        vs_len_size--;
    }
    head_voidsize -= vs_len_size;

    unsigned char * thevoid = (unsigned char *)malloc (head_voidsize);
    memset (thevoid, 0, head_voidsize);
    
    #ifdef DEBUG
    printf ("head voidsize = %d, SEEKHEAD_SIZE = %d\n", head_voidsize, SEEKHEAD_SIZE);
    #endif
    
    ebml_element * seekhead_void = ebml_add_child (seekhead, (byte *)WEBM_ID_Void);
        ebml_setdata_stringn (seekhead_void, (byte *)thevoid, head_voidsize);
    
    free (thevoid);
        
    

    #ifdef DEBUG
    printf ("SeekHead: CueClusterPosition: %d\n", SEEKHEAD_CLUSTER_POSITION);
    #endif


    // set the first cue's cueclusterposition to the proper value		
    ebml_setdata_plain_uint (ctp0_cueclusterposition, SEEKHEAD_CLUSTER_POSITION); // the containing Cluster's offset from the beginning of the Segment's _data_
    ebml_setdata_plain_uint (ctp1_cueclusterposition, SEEKHEAD_CLUSTER_POSITION); // the containing Cluster's offset from the beginning of the Segment's _data_
        
    // set cluster0 position
    ebml_setdata_plain_uint (cluster0_position, SEEKHEAD_CLUSTER_POSITION);
    

    
    wf->current_cluster = cluster0;
            
    return wf;
}


/**
*
*	recursively frees an ebml element tree, starting from "parent"
*
**/
void ebml_free_tree (ebml_element * parent)
{
    ebml_element * kid;
    while (kid = ebml_get_last_child (parent))
    {
        if (kid->prev)
        {
            kid->prev->next = 0;
        }
        else
        {
            parent->children = 0;
        }
        ebml_unsetdata (kid); // child elements are not stored there, only this node's data
        ebml_free_tree (kid);
    }
    ebml_unsetdata (parent); // child elements are not stored there, only this node's data
    free (parent);
}


/**
*
*	frees the memory of a webm_file struct
*
**/
FILE * webm_close_file (webm_file * wf)
{
    FILE * file = wf->file;
    
    #ifdef DEBUG
    printf ("about to webm_write_file()...\n");
    #endif
    
    webm_write_file (wf->file, wf);
    
    #ifdef DEBUG
    printf ("about to ebml_free_tree()...\n");
    #endif
    
    ebml_free_tree (wf->s);	
    free (wf->h);
    free (wf);
    
    return file;
}






/**
*
*	create a new cluster (frames can be added to a cluster until it becomes large enough to not fit in memory. then it's written to disk and a new cluster is created)
*
**/

void webm_add_cluster (webm_file * wf, largeval cluster_timepos, largeval cluster_bytepos)
{
    
    // there can be many clusters. a cluster is usually as long as to hold as many frames as can be encoded in memory at the same time, and then written to disk all at once, which is usually set to around 2 MB
    // clusters hold the video and audio frames (as a set of SimpleBlocks or Blocks)
    ebml_element * cluster = ebml_add_child (wf->s, (byte *)WEBM_ID_Cluster);
        
        ebml_element * timecode = ebml_add_child (cluster, (byte *)WEBM_ID_TimeCode); // Absolute timecode of the cluster (based on TimecodeScale).  (the first cluster is usually at time 0 )
            ebml_setdata_plain_uint (timecode, cluster_timepos);
        ebml_element * position = ebml_add_child (cluster, (byte *)WEBM_ID_Position); // The Position of the Cluster in the segment (0 in live broadcast streams). It might help to resynchronise offset on damaged streams.
            ebml_setdata_plain_uint
            (
                position
                ,
                cluster_bytepos
            ); // this should be set to the length of all the preceding level 1 elements in the segment
        ebml_element * simpleblock0 = ebml_add_child (cluster, (byte *)WEBM_ID_SimpleBlock); // Similar to Block but without all the extra information, mostly used to reduced overhead when no extra feature is needed. (see SimpleBlock Structure)

}
        
        

/**
*
*	writes a frame to a webm_file
*
**/

void webm_write_frame (webm_file * wf, byte * frame_data, largeval data_size, largeval frame_num)
{
    ebml_element * cluster = wf->current_cluster;
    
    // TODO: check if the current cluster is full, and begin a new cluster if so (writes the full cluster to disk)

    // add block header (4 bytes) (1 byte = track number (vint), 2 bytes = block timecode (signed int16), 1 byte = flags)
    /*
        Block Header
        Offset	Player	Description
        0x00+	must	Track Number (Track Entry). It is coded in EBML like form (1 octet if the value is < 0x80, 2 if < 0x4000, etc) (most significant bits set to increase the range).
        0x01+	must	Timecode (relative to Cluster timecode, signed int16)
        0x03+	-	
            Flags
            Bit	Player	Description
            0-3	-	Reserved, set to 0
            4	-	Invisible, the codec should decode this frame but not display it
            5-6	must	Lacing

                00 : no lacing
                01 : Xiph lacing
                11 : EBML lacing
                10 : fixed-size lacing

            7	-	not used
    */
    
    #define BLOCK_HEADER_SIZE 4
    
    byte block_header [BLOCK_HEADER_SIZE];
    block_header[0] = 0x81; // track number, as "vint",  FIXME: maybe this should be dynamic
            // "\x82" will be the audio track, when we have that. the audio SimpleBlocks are mixed together with the video blocks
    
    uint16_t frametime = frame_num * 42; // 42 is some unit, maybe milliseconds

    block_header[1] = (byte)(frametime >> 8);
    block_header[2] = (byte)(frametime);

    block_header[3] = 0; // flags. maybe this holds the "keyframe" flag too, but I'm not sure

    ebml_element * simpleblock = ebml_add_child (cluster, (byte *)WEBM_ID_SimpleBlock); // Similar to Block but without all the extra information, mostly used to reduced overhead when no extra feature is needed. (see SimpleBlock Structure)
        ebml_setdata_stringn (simpleblock, frame_data, data_size, BLOCK_HEADER_SIZE);
    
    // prepend the block header
    memcpy (simpleblock->data, block_header, BLOCK_HEADER_SIZE);
}
