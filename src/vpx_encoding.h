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
