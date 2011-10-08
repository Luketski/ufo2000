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
#include <math.h>

#include <sys/time.h>
#include <sys/resource.h>


#include "webm_writer2.h"


#define APP_NAME_TO_EMBED_IN_WEBM_FILES "UFO2000 | http://ufo2000.sf.net"


#define CLUSTER_TIME_LIMIT_MS 3330

#define ADD_CUES_EVERY_NTH_FRAME (30 * 1)



// reference: http://www.matroska.org/technical/specs/index.html
// reference: http://www.webmproject.org/code/specs/container/
// reference: http://www.webmfiles.org/


//#define WEBM_64BIT 1


#include "webm_writer_utils.h"

#include "webm_writer_ebml_element.h"







class WEBM_Cluster : public EBML_Element
{
  protected:
    largeval first_frame_num; // frame number of the first frame in this cluster
    largeval first_frame_time_ms; // absolute time of first frame
    largeval frame_data_size; // size of frames in this cluster sofar
    largeval cluster_total_time_ms;
    largeval debug_cluster_number;
    EBML_Element * cluster_position;
    EBML_Element * cues;
    
    int frame_time_ms;
    
  public:
    WEBM_Cluster (largeval _first_frame_num, largeval _first_frame_time_ms, EBML_Element * _cues, int _frame_time_ms, int _debug_cluster_number); // _first_frame_num is the frame number of the first frame in this cluster
    bool Is_Full (); // cluster time has reached the limit
    largeval Get_First_Frame_Num (); // frame number of the first frame in this cluster
    void Add_Frame (byte * frame_data, largeval data_size, largeval frame_num, uint16_t frametime, bool is_keyframe);
    largeval Get_Cluster_Position ();
    void Set_Cluster_Position (largeval);
    uint16_t Get_Frametime (largeval frame_num);
};








class WEBM_File
{
  protected:
    FILE * file;
    int video_width;
    int video_height;
    int framerate;
    
    EBML_Element * ebml_header;
    EBML_Element * segment;

    EBML_Element * seekhead;
    EBML_Element * info;
    EBML_Element * tracks;
    EBML_Element * cues;
    
    WEBM_Cluster * first_cluster;
    WEBM_Cluster * current_cluster;
    
    EBML_Element * end_seekhead; // this is at the end of the file and contains the positions of the clusters - which is only determinable when the clusters have been written to disk
    
    
    // FIXME: these should be floats or doubles probably, to ensure a high precision
    // a copy of frame_time_ms is also passed to WEBM_Cluster on creation
    int frame_time_ms; // the duration of 1 frame in ms
    uint64_t total_frame_time_ms; // this is the time of the current frame in the entire video. it's used for setting the cluster timecode (cluster's start time)
    
    largeval number_of_clusters;
    
    
  public:
    WEBM_File (FILE * _file, int _video_width, int _video_height, int _framerate);
    ~WEBM_File ();
    
    FILE * Get_FILE ();		
    void Write_Frame (byte * frame_data, largeval data_size, largeval frame_num, bool is_keyframe);
    WEBM_Cluster * Get_Current_Cluster ();
};







class WEBM_Frame : public EBML_Element
{
  protected:
    static const int BLOCK_HEADER_SIZE = 4;
    EBML_Element * cue_cluster_position; // position of the cluster, this will be set by the WEBM_File destructor when writing it out to disk, can be obtained by Get_CueClusterPosition ()
    
    byte * Create_Block_Header ();
    void Write_Block_Header (byte * frame_data_header_location, uint16_t frametime, bool is_keyframe);
  public:
    WEBM_Frame (byte * frame_data, largeval data_size, largeval frame_num, uint16_t frametime, EBML_Element * _cue_cluster_position, int debug_cluster_number, bool is_keyframe);
    EBML_Element * Get_CueClusterPosition ();
};


WEBM_Frame::WEBM_Frame (byte * frame_data, largeval data_size, largeval frame_num, uint16_t frametime, EBML_Element * _cue_cluster_position, int debug_cluster_number, bool is_keyframe)
  : EBML_Element (WEBM_ID_SimpleBlock)
{
  // save the reference to the cuetrackposition that refers to this frame - because the exact position will be known and written only at file close time
  cue_cluster_position = _cue_cluster_position; // NOTE: this may be 0
  
  // store the frame data, leaving room for the block header
  Set_Data_StringN ((char *)frame_data, data_size, BLOCK_HEADER_SIZE);

  //fprintf (stdout, "WEBM_Frame::WEBM_Frame: frame %d of cluster %d is %s\n", frame_num, debug_cluster_number, is_keyframe ? "*** a KEYFRAME" : "an interframe");
  
  // the block header becomes part of the data
  Write_Block_Header (Get_Data (), frametime, is_keyframe);
}


void WEBM_Frame::Write_Block_Header (byte * frame_data_header_location, uint16_t frametime, bool is_keyframe)
{
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
  
  byte * block_header = frame_data_header_location;
  block_header[0] = 0x81; // track number, as "vint",  FIXME: maybe this should be dynamic
          // "\x82" will be the audio track, when we have that. the audio SimpleBlocks are mixed together with the video blocks
  
  // store 'frametime' in big endian byte order
  block_header[1] = (byte)(frametime >> 8);
  block_header[2] = (byte)(frametime);

  // flags
  block_header[3] = 0 | (is_keyframe ? 0x80 : 0);

  // prepend the block header
  //memcpy (frame_data_header_location, block_header, BLOCK_HEADER_SIZE);
}

EBML_Element * WEBM_Frame::Get_CueClusterPosition ()
{
  return cue_cluster_position;
}







WEBM_Cluster::WEBM_Cluster (largeval _first_frame_num, largeval _first_frame_time_ms, EBML_Element * _cues, int _frame_time_ms, int _debug_cluster_number)
  : EBML_Element (WEBM_ID_Cluster)
{
  frame_data_size = 0;
  first_frame_num = _first_frame_num;
  first_frame_time_ms = _first_frame_time_ms;
  cues = _cues;
  
  debug_cluster_number = _debug_cluster_number;
  
  frame_time_ms = _frame_time_ms;
  
  Add_Child
  (
    (new EBML_Element (WEBM_ID_TimeCode))
      ->Set_Data_Plain_Uint (_first_frame_time_ms) // Absolute timecode of the cluster (based on TimecodeScale).  (the first cluster is usually at time 0 ) = value is in milliseconds
  );
  //fprintf (stdout, "webm_cluster::webm_cluster: got here 1\n");
  Add_Child
  (
    cluster_position =
    (new EBML_Element (WEBM_ID_Position))
      // FIXME: this will break for files larger than ~4gb - instead use the 8 byte version , though first sort out the 64-bits issue
      ->Set_Data_Plain_Uint (0xfffffffe) // The Position of the Cluster in the segment (0 in live broadcast streams). It might help to resynchronise offset on damaged streams.
  );
  //fprintf (stdout, "webm_cluster::webm_cluster: got here 2\n");
}

bool WEBM_Cluster::Is_Full ()
{
  //return (frame_data_size >= CLUSTER_SIZE_LIMIT);
  return (cluster_total_time_ms >= CLUSTER_TIME_LIMIT_MS);
}

largeval WEBM_Cluster::Get_First_Frame_Num ()
{
  return first_frame_num;
}

void WEBM_Cluster::Add_Frame (byte * frame_data, largeval data_size, largeval frame_num, uint16_t frametime, bool is_keyframe)
{
  EBML_Element * cue_cluster_position = 0;
  if (!(frame_num % ADD_CUES_EVERY_NTH_FRAME)) {
    largeval block_number = frame_num - Get_First_Frame_Num ();
    cues
      ->Add_Child
      (
        (new EBML_Element (WEBM_ID_CuePoint))
          ->Add_Child
          (
            (new EBML_Element (WEBM_ID_CueTime))
              ->Set_Data_Plain_Uint (frametime)
          )
          ->Add_Child
          (
            (new EBML_Element (WEBM_ID_CueTrackPositions))
              ->Add_Child
              (
                (new EBML_Element (WEBM_ID_CueTrack))
                  ->Set_Data_Plain_Uint (1) // track 1 is the video track
              )
              ->Add_Child
              (
                cue_cluster_position =
                (new EBML_Element (WEBM_ID_CueClusterPosition))
                  // FIXME: will break with files larger than 4gb
                  ->Set_Data_Plain_Uint (0xfffffffe)
              )
          )
      )
    ;
    
    if (block_number > 0 /**/|| 1/**/) {
      cues
        ->Get_Last_Child () // CuePoint
          ->Get_Last_Child () // CueTrackPositions
            ->Add_Child
            (
              (new EBML_Element (WEBM_ID_CueBlockNumber))
                ->Set_Data_Plain_Uint (block_number)
            )
      ;
    }
  }
  Add_Child
  (
    new WEBM_Frame (frame_data, data_size, frame_num, frametime, cue_cluster_position, debug_cluster_number, is_keyframe)
  );
  cluster_total_time_ms = frametime;
}

void WEBM_Cluster::Set_Cluster_Position (largeval pos)
{
  cluster_position
    ->Set_Data_Plain_Uint (pos)
  ;
}

largeval WEBM_Cluster::Get_Cluster_Position ()
{
  return cluster_position->Get_Data_Plain_Uint ();
}

uint16_t WEBM_Cluster::Get_Frametime (largeval frame_num)
{
  uint16_t frametime = (frame_num - Get_First_Frame_Num ()) * frame_time_ms; // FIXME: a bug is possible here if the 'frametime' variable overflows - this also limits the maximum size of a cluster to ~32000 ms, or 32 seconds
  return frametime;
}





WEBM_File::WEBM_File (FILE * _file, int _video_width, int _video_height, int _framerate)
{
  file = _file;
  video_width = _video_width;
  video_height = _video_height;
  framerate = _framerate;
  
  
  number_of_clusters = 0;
  
  //fprintf (stdout, "webm_got_here 1-1\n");
  //fflush (stdout);
  
  
  //
  //
  //
  // construct the ebml header
  //
  //
  //
  
  ebml_header =
    (new EBML_Element (WEBM_ID_EBML_Header))
      ->Add_Child
      (
        (new EBML_Element (WEBM_ID_EBML_Version))
          ->Set_Data_StringN ("\x03", 1) // EBML version 3
      )
      ->Add_Child
      (
        (new EBML_Element (WEBM_ID_EBML_ReadVersion))
          ->Set_Data_StringN ("\x01", 1) // parser can be EBML version 1
      )
      ->Add_Child
      (
        (new EBML_Element (WEBM_ID_EBML_MaxIDLength))
        ->Set_Data_StringN ("\x04", 1) // length of longest ID in this file (FIXME: this should be determined dynamically)
      )
      ->Add_Child
      (
        (new EBML_Element (WEBM_ID_EBML_MaxSizeLength))
        ->Set_Data_StringN ("\x08", 1) // length of longest length field in this file
      )
      ->Add_Child
      (
        (new EBML_Element (WEBM_ID_EBML_Doctype))
        ->Set_Data_StringN ("webm", 4) // doctype
      )
      ->Add_Child
      (
        (new EBML_Element (WEBM_ID_EBML_DoctypeVersion))
        ->Set_Data_StringN ("\x01", 1) // WEBM version 1
      )
      ->Add_Child
      (
        (new EBML_Element (WEBM_ID_EBML_DoctypeReadVersion))
        ->Set_Data_StringN ("\x01", 1) // parser can be WEBM version 1
      )
      ->Add_Child
      (
        (new EBML_Element (WEBM_ID_Void))
        ->Set_Data_StringN ("\x00\x00", 2) // padding to make the header 40 bytes long (though it may be unnecessary)
      )
  ;
  
  
  //fprintf (stdout, "webm_got_here 1-2\n");
  //fflush (stdout);
  
  //exit (-1);
  
  
  
  //
  //
  //
  // construct the segment
  //
  //
  //
  
  segment =
    (new EBML_Element (WEBM_ID_Segment))
      ->Add_Child
      (
        seekhead =
        (new EBML_Element (WEBM_ID_SeekHead))
      )
      ->Add_Child
      (
        info =
        (new EBML_Element (WEBM_ID_Info))
      )
      ->Add_Child
      (
        tracks =
        (new EBML_Element (WEBM_ID_Tracks))
      )
      ->Add_Child
      (
        cues =
        (new EBML_Element (WEBM_ID_Cues))
      )
  ;
  // end seekhead will be added in the destructor, below
  
  first_cluster = 0; // this will be added automatically when the first cluster is created, which will happen on the first frame-write
  current_cluster = 0;
  
  
  //fprintf (stdout, "webm_got_here 1-3\n");
  //fflush (stdout);
  
  
    // info
    info
      ->Add_Child
      (
        (new EBML_Element (WEBM_ID_TimecodeScale))
          ->Set_Data_Plain_Uint (1000000) // Timecode scale in nanoseconds (1.000.000 means all timecodes in the segment are expressed in milliseconds).
      )
      ->Add_Child
      (
        (new EBML_Element (WEBM_ID_MuxingApp))
          ->Set_Data_String ((char *)APP_NAME_TO_EMBED_IN_WEBM_FILES)
      )
      ->Add_Child
      (
        (new EBML_Element (WEBM_ID_WritingApp))
          ->Set_Data_String ((char *)APP_NAME_TO_EMBED_IN_WEBM_FILES)
      )
    ;
    
    
  //fprintf (stdout, "webm_got_here 1-4\n");
  //fflush (stdout);
    
    
    // tracks
    uint32_t frametime_nanosec = 1000000000 / framerate;
    frame_time_ms = frametime_nanosec / 1000000; // FIXME: store it as a float value to avoid precision loss
    total_frame_time_ms = 0;
    tracks->Add_Child
    (
      (new EBML_Element (WEBM_ID_TrackEntry))
      ->Add_Child
      (
        (new EBML_Element (WEBM_ID_TrackNumber))
        ->Set_Data_String ((char *)"\x01") // track 1
      )
      ->Add_Child
      (
        (new EBML_Element (WEBM_ID_TrackUID))
        ->Set_Data_String ((char *)"asdf1234") // uid // FIXME: this should be randomly generated
      )
      ->Add_Child
      (
        (new EBML_Element (WEBM_ID_TrackType))
        ->Set_Data_String ((char *)"\x01") // video
      )
      ->Add_Child
      (
        (new EBML_Element (WEBM_ID_DefaultDuration))
        ->Set_Data_Plain_Uint (frametime_nanosec) // framerate, an uint value, this sets the duration of 1 frame (in nanoseconds)
      )
      ->Add_Child
      (
        (new EBML_Element (WEBM_ID_TrackTimecodeScale))
        ->Set_Data_StringN ((char *)"\x3f\x80\x00\x00", 4)	// speed scaling, a float value, is 1.0
      )
      ->Add_Child
      (
        (new EBML_Element (WEBM_ID_CodecID))
        ->Set_Data_String ((char *)"V_VP8")
      )
      ->Add_Child
      (
        (new EBML_Element (WEBM_ID_CodecName))
        ->Set_Data_String ((char *)"VP8")
      )
      ->Add_Child
      (
        (new EBML_Element (WEBM_ID_Video))
        ->Add_Child
        (
          (new EBML_Element (WEBM_ID_PixelWidth))
          ->Set_Data_Plain_Uint (video_width) // size of the video in pixels
        )
        ->Add_Child
        (
          (new EBML_Element (WEBM_ID_PixelHeight))
          ->Set_Data_Plain_Uint (video_height) // size of the video in pixels
        )
      )
    );
    
  //fprintf (stdout, "webm_got_here 1-5\n");
  //fflush (stdout);
  
  
  //fprintf (stdout, "\n\n\n\n");
  //fprintf (stdout, "ebml_header\n");
  //fflush (stdout);
  //ebml_header->Debug_Write_Structure ();

  //fprintf (stdout, "\n\n\n\n");
  //fprintf (stdout, "segment\n");
  //fflush (stdout);
  //segment->Debug_Write_Structure ();

}



/**
*
*	This finishes writing to the file. It sets the SeekHead structures correctly, and writes everything to disk. 
*
*	The FILE* pointer needs to be closed after calling this, separately.
*
**/
WEBM_File::~WEBM_File ()
{
  // set final meta-infos
  info
    ->Add_Child
    (
      (new EBML_Element (WEBM_ID_Duration))
        ->Set_Data_Float (total_frame_time_ms)
    )
  ;
  
  
  // there are 4 element positions recorded in the first seekhead
  // seekhead meta size is
  // 4(seekhead sign) + 1(seekhead length)  == 5
  // plus
  // the size of the seek entry and seekid and seekposition structures is 
  // 4(seek sign) + 1(seek length)  == 5
  // + 4(seekID sign) + 1(seekID length) + 4(seekID value, longest signature can be 4 bytes)  == 9
  // + 4(seekPosition sign) + 1(seekPosition length) + WEBM_MAX_LENGTH_BYTES(seekPosition value, longest length field can be WEBM_MAX_LENGTH_BYTES(8) bytes)  == 13
  // == 27 at most per seek entry
  // 108 + 5 == 113 bytes maximum for the seekhead structure
  
  largeval seekhead_size_temp = 113; 
  
  largeval info_position;
  largeval info_size = info->Get_Total_Length_On_Disk ();

  largeval tracks_position;
  largeval tracks_size = tracks->Get_Total_Length_On_Disk ();

  largeval cues_position;
  largeval cues_size = cues->Get_Total_Length_On_Disk ();

  //fprintf (stdout, "~WEBM_File(): location 1\n");
  //fprintf (stdout, "\n\n\n\n");
  //fprintf (stdout, "segment\n");
  //fflush (stdout);
  //segment->Debug_Write_Structure ();
  
  
  largeval end_seekhead_offset = 0;
  WEBM_Cluster * cluster = first_cluster;
  largeval clusters_total_length = 0;
  while (cluster) {
    clusters_total_length += cluster->Get_Total_Length_On_Disk ();
    clusters_total_length += (WEBM_MAX_LENGTH_BYTES-1); // this is added because the cluster Position value is currently set to 0 but will be set to the actual position later, see below
    if (cluster->Is_Last_Child ()) {
      break;
    }
    cluster = (WEBM_Cluster *)cluster->Get_Next_Sibling ();
  }
  end_seekhead_offset = clusters_total_length;
  largeval end_seekhead_position = 0;

  //fprintf (stdout, "~WEBM_File(): location 2\n");
  //fprintf (stdout, "\n\n\n\n");
  //fprintf (stdout, "segment\n");
  //fflush (stdout);
  //segment->Debug_Write_Structure ();
  
  
  EBML_Element * seek_info;
  EBML_Element * seek_tracks;
  EBML_Element * seek_cues;
  EBML_Element * seek_end_seekhead;
  
  seekhead
    ->Add_Child
    (	// Info
      (new EBML_Element (WEBM_ID_Seek))
        ->Add_Child
        (
          (new EBML_Element (WEBM_ID_SeekID))
            ->Set_Data_String ((char *)webm_id_manager->Get_ID_Signature (WEBM_ID_Info))
        )
        ->Add_Child
        (
          seek_info =
          (new EBML_Element (WEBM_ID_SeekPosition))
            ->Set_Data_Plain_Uint (info_position = seekhead_size_temp)
        )
    )
    ->Add_Child
    (	// Tracks
      (new EBML_Element (WEBM_ID_Seek))
        ->Add_Child
        (
          (new EBML_Element (WEBM_ID_SeekID))
            ->Set_Data_String ((char *)webm_id_manager->Get_ID_Signature (WEBM_ID_Tracks))
        )
        ->Add_Child
        (
          seek_tracks =
          (new EBML_Element (WEBM_ID_SeekPosition))
            ->Set_Data_Plain_Uint (tracks_position = seekhead_size_temp + info_size)
        )
    )
    ->Add_Child
    (	// Cues
      (new EBML_Element (WEBM_ID_Seek))
        ->Add_Child
        (
          (new EBML_Element (WEBM_ID_SeekID))
            ->Set_Data_String ((char *)webm_id_manager->Get_ID_Signature (WEBM_ID_Cues))
        )
        ->Add_Child
        (
          seek_cues =
          (new EBML_Element (WEBM_ID_SeekPosition))
            ->Set_Data_Plain_Uint (cues_position = seekhead_size_temp + info_size + tracks_size)
        )
    )
    ->Add_Child
    (	// end SeekHead
      (new EBML_Element (WEBM_ID_Seek))
        ->Add_Child
        (
          (new EBML_Element (WEBM_ID_SeekID))
            ->Set_Data_String ((char *)webm_id_manager->Get_ID_Signature (WEBM_ID_SeekHead))
        )
        ->Add_Child
        (
          seek_end_seekhead =
          (new EBML_Element (WEBM_ID_SeekPosition))
            ->Set_Data_Plain_Uint (end_seekhead_position = seekhead_size_temp + info_size + tracks_size + cues_size + end_seekhead_offset)
        )
    )
  ;
  
  
  //fprintf (stdout, "~WEBM_File(): location 3\n");
  //fprintf (stdout, "\n\n\n\n");
  //fprintf (stdout, "segment\n");
  //fflush (stdout);
  //segment->Debug_Write_Structure ();
  
  // the actual values will be less than the maximum, but that will affect the values themselves
  // therefore the size of the seekhead structure will be recalculated with the results of the previous iteration, starting with the maximum until it doesn't shrink anymore, and that will be the real final size of the seekhead structure
  largeval current_seekhead_size;
  largeval shrunk_by;
  largeval clusters_length_prev;
  largeval cues_length_prev = 0;
  largeval cues_total_length = 0;
  int rounds_after_shrunk = -1;
  do {
    current_seekhead_size = seekhead->Get_Total_Length_On_Disk ();
    shrunk_by = (seekhead_size_temp - current_seekhead_size);
    clusters_length_prev = clusters_total_length;
    
    info_position -= shrunk_by;
      seek_info->Set_Data_Plain_Uint (info_position);
    tracks_position -= shrunk_by;
      seek_tracks->Set_Data_Plain_Uint (tracks_position);
    cues_position -= shrunk_by;
      seek_cues->Set_Data_Plain_Uint (cues_position);
      

    //fprintf (stdout, "webm info_position = %d, info_size = %d\n", info_position, info_size);
    //fflush (stdout);
    //fprintf (stdout, "webm tracks_position = %d, tracks_size = %d\n", tracks_position, tracks_size);
    //fflush (stdout);
    //fprintf (stdout, "webm cues_position = %d, cues_size = %d\n", cues_position, cues_total_length);
    //fflush (stdout);

    
    
    // since the position of the clusters is also recorded within the clusters themselves (as a plain uint data field of variable length (though not a vint)), we need to shrink these too at the same time
    cluster = first_cluster;
    clusters_total_length = 0;
    cues_length_prev = cues_total_length;
    int clustern = 0;
    while (cluster) {
      cues_total_length = cues->Get_Total_Length_On_Disk ();
      largeval cluster_position = cues_position + cues_total_length + clusters_total_length;
      cluster->Set_Cluster_Position (cluster_position);
      largeval clustersize = cluster->Get_Total_Length_On_Disk ();
      clusters_total_length += clustersize;
      
      //fprintf (stdout, "webm cluster %d position = %d (clusters_total_length = %d)\n", clustern++, cluster_position, clusters_total_length);
      //fflush (stdout);
      
      WEBM_Frame * frame = (WEBM_Frame *) cluster->Get_First_Child ()->Get_Next_Sibling ()->Get_Next_Sibling ();
      int framen = 0;
      while (frame) {
        EBML_Element * cueclusterpos = frame->Get_CueClusterPosition ();
        if (cueclusterpos) {
          cueclusterpos
            ->Set_Data_Plain_Uint (cluster_position)
          ;
        }
        //fprintf (stdout, "webm cluster %d, frame %d, cueclusterposition set to %d\n", clustern++, framen++, cluster_position);
        //fflush (stdout);
        if (frame->Is_Last_Child ()) {
          break;
        }
        frame = (WEBM_Frame *)frame->Get_Next_Sibling ();
      }		
      
      if (cluster->Is_Last_Child ()) {
        break;
      }
      cluster = (WEBM_Cluster *)cluster->Get_Next_Sibling ();
    }
    cues_total_length = cues->Get_Total_Length_On_Disk ();
    if (cues_length_prev == 0) {
      cues_length_prev = cues_total_length + 1;
    }
    end_seekhead_position = cues_position + cues_total_length + clusters_total_length;
    seek_end_seekhead->Set_Data_Plain_Uint (end_seekhead_position);
      
    //fprintf (stdout, "webm end_seekhead_position = %d\n", end_seekhead_position);
    //fflush (stdout);
      
    shrunk_by += (cues_length_prev - cues_total_length);
    shrunk_by += (clusters_length_prev - clusters_total_length);
    
    //fprintf (stdout, "webm shrunk_by = %d\n", shrunk_by);
    //fflush (stdout);
    
    seekhead_size_temp = current_seekhead_size;
    
    if (shrunk_by == 0) {
      //fprintf (stdout, "rounds after shrunk = %d\n", rounds_after_shrunk);
      //fflush (stdout);
      rounds_after_shrunk++;
    }
  }
  while (shrunk_by > 0 || rounds_after_shrunk < 1);
  
  
  //fprintf (stdout, "~WEBM_File(): location 4\n");
  //fprintf (stdout, "\n\n\n\n");
  //fprintf (stdout, "segment\n");
  //fflush (stdout);
  //segment->Debug_Write_Structure ();
  

  //fprintf (stdout, "segment->Last_Child () == %x (%s), nchildren = %d\n", segment->Get_Last_Child (), webm_id_manager->Get_ID_Name (segment->Get_Last_Child ()->Get_ID ()), segment->Number_Of_Children ());
  //fflush (stdout);


  // now add the cluster positions to the end seekhead
  largeval first_cluster_position = cues_position + cues_total_length;
  largeval current_cluster_position = first_cluster_position;
  
  segment->Add_Child
  (
    end_seekhead =
      (new EBML_Element (WEBM_ID_SeekHead))
  );

  //fprintf (stdout, "segment->Last_Child () == %x (%s), nchildren = %d\n", segment->Get_Last_Child (), webm_id_manager->Get_ID_Name (segment->Get_Last_Child ()->Get_ID ()), segment->Number_Of_Children ());
  //fflush (stdout);
  
  
  //fprintf (stdout, "~WEBM_File(): location 5\n");
  //fprintf (stdout, "\n\n\n\n");
  //fprintf (stdout, "segment\n");
  //fflush (stdout);
  //segment->Debug_Write_Structure ();

  //fprintf (stdout, "~WEBM_File(): cues_position = %d\n", cues_position);
  //fprintf (stdout, "~WEBM_File(): cues_size = %d\n", cues_total_length);
  //fprintf (stdout, "~WEBM_File(): current_cluster_position = %d\n", current_cluster_position);
  //fprintf (stdout, "~WEBM_File(): first_cluster_position = %d\n", first_cluster_position);
  //fprintf (stdout, "\n\n\n\n");
  
  
  cluster = first_cluster;
  int clustern = 0;
  while (cluster) {
    //fprintf (stdout, "~WEBM_File(): cluster %d position = %d\n", clustern++, current_cluster_position);
    //fflush (stdout);
    
    if (cluster->Get_ID () == WEBM_ID_SeekHead) {
      // reached the seekhead that follows the last cluster (that was added just above)
      break;
    }
    
    end_seekhead
      ->Add_Child
      (	// save cluster position
        (new EBML_Element (WEBM_ID_Seek))
          ->Add_Child
          (
            (new EBML_Element (WEBM_ID_SeekID))
              ->Set_Data_String ((char *)webm_id_manager->Get_ID_Signature (WEBM_ID_Cluster))
          )
          ->Add_Child
          (
            seek_info =
            (new EBML_Element (WEBM_ID_SeekPosition))
              ->Set_Data_Plain_Uint (current_cluster_position)
          )
      )
    ;
    
    current_cluster_position += cluster->Get_Total_Length_On_Disk ();
    cluster = (WEBM_Cluster *)cluster->Get_Next_Sibling ();
  }
  
  //fprintf (stdout, "\n\n\n\n");
  //fprintf (stdout, "ebml_header\n");
  //fflush (stdout);
  //ebml_header->Debug_Write_Structure ();

  //fprintf (stdout, "\n\n\n\n");
  //fprintf (stdout, "segment\n");
  //fflush (stdout);
  //segment->Debug_Write_Structure ();

  largeval segment_data_offset = ebml_header->Get_Total_Length_On_Disk () + segment->Get_Data_Offset_On_Disk ();

  
  
  
  // all set up, just write it out to disk
  ebml_header->Write_To_Disk (file, segment_data_offset);
  segment->Write_To_Disk (file, segment_data_offset);
  
  
  delete (ebml_header);
  ebml_header = 0;
  delete (segment);
  segment = 0;
}


FILE * WEBM_File::Get_FILE ()
{
  return file;
}


void WEBM_File::Write_Frame (byte * frame_data, largeval data_size, largeval frame_num, bool is_keyframe)
{
  bool start_new_cluster = (!first_cluster) || current_cluster->Is_Full ();
  
  // frametime is relative to the beginning of the cluster
  uint16_t frametime = ((!first_cluster) ? 0 : current_cluster->Get_Frametime (frame_num));
  // total_frame_time_ms is relative to the beginning of the video
  total_frame_time_ms = frame_num * frame_time_ms;

  //fprintf (stdout, "Add_Frame: frame_num %d, frametime %d, total_frame_time_ms %Ld\n", frame_num, frametime, total_frame_time_ms);
  
  
  if (start_new_cluster) {
    if (!first_cluster) {
      // create the first cluster
      current_cluster = first_cluster = new WEBM_Cluster (frame_num, total_frame_time_ms, cues, frame_time_ms, number_of_clusters);
      cues
        ->Add_After
        (
          first_cluster
        )
      ;
      
      //fprintf (stdout, "THE FIRST CLUSTER ADDED\n");
      //fprintf (stdout, "\n\n\n\n");
      //fprintf (stdout, "segment\n");
      //fflush (stdout);
      //segment->Debug_Write_Structure ();
      //fprintf (stdout, "\n\n\n\n");
    }
    else {
      // this is not the first cluster
      
      // TODO: write out the previous cluster to disk, so that all clusters don't have to be held in memory at the same time
      // this will allow to write videos longer than the amount of memory present
      // though it results in the data being written twice to disk (first the finished clusters are written to temporary files, then the final video file is assembled from the clusters), which is inefficient
      // this would preserve the cluster's metadata in memory, and only free its data (after writing it to disk temporarily). in the end the clusters would be loaded one by one and written to the final video file, then the temp files deleted
      //current_cluster->Save_Temp_To_Disk ()
      //current_cluster->Load_Temp_From_Disk ()
      //current_cluster->Delete_Temp_File ()
    
      EBML_Element * previous_cluster = current_cluster;
      previous_cluster
        ->Add_After
        (
          current_cluster =
            (new WEBM_Cluster (frame_num, total_frame_time_ms, cues, frame_time_ms, number_of_clusters))
        )
      ;
      number_of_clusters++;
      frametime = current_cluster->Get_Frametime (frame_num);
      
      //fprintf (stdout, "NOT THE FIRST, CLUSTER %d ADDED\n", number_of_clusters);
    }
  }
  
  current_cluster->Add_Frame (frame_data, data_size, frame_num, frametime, is_keyframe);

  if (start_new_cluster) {
    //fprintf (stdout, "start new cluster (cluster number %d, time ms %d)\n", number_of_clusters, frametime);
    //fflush (stdout);
    
    //fprintf (stdout, "FIRST FRAME ADDED\n");
    //fprintf (stdout, "\n\n\n\n");
    //fprintf (stdout, "segment\n");
    //fflush (stdout);
    //segment->Debug_Write_Structure ();
    //fflush (stdout);
    //fprintf (stdout, "\n\n\n\n");
  }
}


// this may return 0, if no frames have been written yet
WEBM_Cluster * WEBM_File::Get_Current_Cluster ()
{
  return current_cluster;
}







// time measurement - thanks!! http://web.me.com/haroldsoh/tutorials/technical-skills/microsecond-timing-in-cc-wi.html
timeval webm_startTime;
timeval webm_endTime;
rusage webm_ru;

void webm_start_time_measure ()
{
  //getrusage (RUSAGE_SELF, &webm_ru);
  //webm_startTime = webm_ru.ru_utime;
  gettimeofday (&webm_startTime, NULL);
}

void webm_end_time_measure (char * name)
{
  //getrusage (RUSAGE_SELF, &webm_ru);
  //webm_endTime = webm_ru.ru_utime;
  gettimeofday (&webm_endTime, NULL);
  
  int daytime = webm_startTime.tv_sec % 86400;
  
  double tS = daytime*1000000 + (webm_startTime.tv_usec);
  double tE = daytime*1000000 + (webm_endTime.tv_usec);	
  
  double time_elapsed = tE - tS;
  
  fprintf (stdout, "TIME OF %s: %.1f usec\n", name, time_elapsed);
}










/**
*
*	Creates a new webm_file resource
*
*	the FILE* must be a writable file resource created by the caller before calling this - it is also the caller's responsibility to close the file after it is finished
*
*	a webm_file resource is returned, which must be destroyed with webm_close_file() to free it's memory
*
**/
WEBM_FILE webm_open_file (FILE * file, int video_width, int video_height, int framerate) // framerate gives the duration of a frame in "1 second / (framerate)" , where 1 is 1 second and n is number of frames
{
  //fprintf (stdout, "webm_got_here 1\n");
  //fflush (stdout);
  WEBM_File * wf = new WEBM_File (file, video_width, video_height, framerate);

  //fprintf (stdout, "webm_got_here 2\n");
  //fflush (stdout);
  
  return (WEBM_FILE) wf;
}


/**
*
*	writes a frame to a webm_file
*
**/

void webm_write_frame (WEBM_FILE wf, byte * frame_data, largeval data_size, largeval frame_num, bool is_keyframe)
{
  //webm_start_time_measure ();
  //fprintf (stdout, "webm_got_here 3\n");
  //fflush (stdout);
  ((WEBM_File *)wf)->Write_Frame (frame_data, data_size, frame_num, is_keyframe);
  
  //fprintf (stdout, "webm_got_here 4\n");
  //fflush (stdout);
  
  //char framenumstr [100];
  //sprintf (framenumstr, "FRAME %d %s", frame_num, is_keyframe ? "K" : "|");
  //webm_end_time_measure (framenumstr);
}


/**
*
*  queries whether next frame should be forced to become a keyframe (this is necessary for the first frame of a new cluster)
*
*  the result should be passed to the vpx_encode_frame() function
*
**/
int webm_should_next_frame_be_a_keyframe (WEBM_FILE wf)
{
  WEBM_Cluster * current_cluster = ((WEBM_File *)wf)->Get_Current_Cluster ();
  if (!current_cluster) {
    // first frame in the video
    return 1;
  }
  return current_cluster->Is_Full ();
}



/**
*
*	closes the file on disk (stores the final position of the elements within the file), and frees the memory of a webm_file struct
*
*	the returned FILE* pointer should be closed by the caller
*
**/
FILE * webm_close_file (WEBM_FILE wf)
{
  //webm_start_time_measure ();
  
  //fprintf (stdout, "webm_got_here 4\n");
  //fflush (stdout);
  
  FILE * f = ((WEBM_File *)wf)->Get_FILE ();
  
  //fprintf (stdout, "webm_got_here 5\n");
  //fflush (stdout);
  
  delete ((WEBM_File *)wf);

  //fprintf (stdout, "webm_got_here 6\n");
  //fflush (stdout);

  wf = 0;
  
  //webm_end_time_measure ("CLOSE FILE");
  
  return f;
}
