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

#ifndef VPX_ENCODING_H
#define VPX_ENCODING_H


#include "vpx/vpx_encoder.h"
#include "vpx/vp8cx.h"


typedef struct
{
  vpx_codec_ctx_t      codec;
  vpx_codec_enc_cfg_t  cfg;
  int                  frame_cnt;
  vpx_image_t          raw_yv12;
  //vpx_codec_err_t      res;
  long                 width;
  long                 height;
  int                  frame_avail;
  int                  got_data;
  int                  flags;
  
  //char *               raw_yv12; // pointer to YV12 pixel data (YUV 4:2:0 planar)
} vpx_config;



//extern vpx_config vpx;



vpx_config * init_vpx_encoder (int width, int height);
void shutdown_vpx_encoder ();
unsigned char * vpx_encode_frame (vpx_image_t * raw_yv12, unsigned long * data_length, bool * is_keyframe);




#endif
