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
/*

Some of the code in this file was copied from here: http://www.webmproject.org/tools/vp8-sdk/example__simple__encoder.html

*/


#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "vpx_encoding.h"





#define VPX_CODEC_DISABLE_COMPAT 1
#define VPX_INTERFACE (vpx_codec_vp8_cx())
                                                                                      //
                                                                                      //





static vpx_config vpx;








static void die(const char *fmt, ...) {
  va_list ap;

  va_start(ap, fmt);
  vprintf(fmt, ap);
  if(fmt[strlen(fmt)-1] != '\n')
    printf("\n");
  exit(EXIT_FAILURE);
}

static void die_codec(vpx_codec_ctx_t *ctx, const char *s) {                  //
  const char *detail = vpx_codec_error_detail(ctx);                         //
                                                                            //
  printf("%s: %s\n", s, vpx_codec_error(ctx));                              //
  if(detail)                                                                //
    printf("    %s\n",detail);                                            //
  exit(EXIT_FAILURE);                                                       //
}                                                                             //







vpx_config * init_vpx_encoder (int width, int height)
{
  vpx_codec_err_t res;

  vpx.frame_cnt = 0;
  vpx.flags = 0;

  vpx.width = width;
  vpx.height = height;

  

  if(!vpx_img_alloc(&vpx.raw_yv12, VPX_IMG_FMT_I420, vpx.width, vpx.height, 1)) {
    die("vpx: Failed to allocate image (%d x %d)", vpx.width, vpx.height);
  }



  printf ("vpx: Using %s\n", vpx_codec_iface_name (VPX_INTERFACE));

  /* Populate encoder configuration */                                      //
  res = vpx_codec_enc_config_default (VPX_INTERFACE, &vpx.cfg, 0);          //
  if(res) {                                                                 //
    die("vpx: Failed to get config: %s\n", vpx_codec_err_to_string(res));   //
  }                                                                         //

  /* Update the default configuration with our settings */                  //
  vpx.cfg.rc_target_bitrate = vpx.width * vpx.height * vpx.cfg.rc_target_bitrate            //
                          / vpx.cfg.g_w / vpx.cfg.g_h;                              //
  vpx.cfg.g_w = vpx.width;                                                          //
  vpx.cfg.g_h = vpx.height;                                                         //


  
  /* Initialize codec */                                                //
  if (vpx_codec_enc_init (&vpx.codec, VPX_INTERFACE, &vpx.cfg, 0)) {
      die_codec (&vpx.codec, "vpx: Failed to initialize encoder");                //
  }
  
  
  return &vpx;
}





void shutdown_vpx_encoder ()
{
  printf ("vpx: Processed %d frames.\n", vpx.frame_cnt-1);
  if (vpx_codec_destroy (&vpx.codec)) {
    die_codec (&vpx.codec, "vpx: Failed to destroy codec");
  }
}





/**
*
* 'raw_yv12->planes[0]' must be filled with the raw yv12 pixels of the frame before calling this
* returns the frame data, and in the other params the data length and the keyframe flag
*
*/
unsigned char * vpx_encode_frame (vpx_image_t * raw_yv12, unsigned long * data_length, bool * is_keyframe, bool force_keyframe)
{
      // as a temporary solution, it will read from allegro's screen buffer directly
  //rgb__to__yv12 (/*raw_rgb,*/ vpx.raw_yv12.planes[0]);
  
  unsigned char * encoded_frame = 0;
  
  
  vpx_codec_iter_t iter = NULL;
  const vpx_codec_cx_pkt_t *pkt;

  if(vpx_codec_encode(&vpx.codec, raw_yv12, vpx.frame_cnt, 1, vpx.flags | (force_keyframe ? VPX_EFLAG_FORCE_KF : 0), VPX_DL_REALTIME)) {
    die_codec(&vpx.codec, "Failed to encode frame");
  }
  
  //int got_data = 0;
  while ( (pkt = vpx_codec_get_cx_data(&vpx.codec, &iter)) ) {
    //got_data = 1;
    switch(pkt->kind) {
      case VPX_CODEC_CX_FRAME_PKT: {
        encoded_frame = (unsigned char *)pkt->data.frame.buf;
        *data_length = pkt->data.frame.sz;
      }
      break;                                                    //
      default: {
      }
      break;
    }
    //printf(pkt->kind == VPX_CODEC_CX_FRAME_PKT && (pkt->data.frame.flags & VPX_FRAME_IS_KEY)? "K":".");
    //fflush(stdout);
    *is_keyframe = (pkt->kind == VPX_CODEC_CX_FRAME_PKT && (pkt->data.frame.flags & VPX_FRAME_IS_KEY));
  }
  vpx.frame_cnt++;
  
  return encoded_frame;
}
