#ifndef VPX_ENCODING_H
#define VPX_ENCODING_H


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



void init_vpx_encoder (int width, int height);
void shutdown_vpx_encoder ();
void vpx_encode_frame ();




#endif
