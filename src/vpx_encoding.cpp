

#include "vpx_encoding.h"



#include "vpx/vpx_encoder.h"
#include "vpx/vp8cx.h"



#define VPX_CODEC_DISABLE_COMPAT 1
#define VPX_INTERFACE (vpx_codec_vp8_cx())
                                                                                        //
                                                                                        //





static vpx_config vpx;



void init_vpx_encoder (int width, int height)
{
    vpx_codec_err_t res;

    vpx.frame_cnt = 0;
    vpx.flags = 0;

    vpx.width = width;
    vpx.height = height;

    

    if(!vpx_img_alloc(&vpx.raw_yv12, VPX_IMG_FMT_I420, vpx.width, vpx.height, 1))
    {
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
    if (vpx_codec_enc_init (&vpx.codec, VPX_INTERFACE, &vpx.cfg, 0))                //
    {
        die_codec (&vpx.codec, "vpx: Failed to initialize encoder");                //
    }
    
    
}





void shutdown_vpx_encoder ()
{
    printf ("vpx: Processed %d frames.\n", vpx.frame_cnt-1);
    if (vpx_codec_destroy (&vpx.codec)) {
        die_codec (&vpx.codec, "vpx: Failed to destroy codec");
    }
}





void vpx_encode_frame (/*char * raw_rgb*/)
{
        // as a temporary solution, it will read from allegro's screen buffer directly
    rgb__to__yv12 (/*raw_rgb,*/ vpx.raw_yv12.planes[0]);
    
    vpx_codec_iter_t iter = NULL;
    const vpx_codec_cx_pkt_t *pkt;

    if(vpx_codec_encode(&vpx.codec, &vpx.raw_yv12, vpx.frame_cnt, 1, vpx.flags, VPX_DL_REALTIME))
    {
        die_codec(&vpx.codec, "Failed to encode frame");
    }
    
    //int got_data = 0;
    while ( (pkt = vpx_codec_get_cx_data(&vpx.codec, &iter)) )
    {
        //got_data = 1;
        switch(pkt->kind)
        {
            case VPX_CODEC_CX_FRAME_PKT:                                  //
            {
                   // FIXME: this needs rewriting, its broken here right now
                write_ivf_frame_header(vpx.outfile, pkt);                     //
                if(fwrite(pkt->data.frame.buf, 1, pkt->data.frame.sz, vpx.outfile));
            }
            break;                                                    //
            default:
            {
            }
            break;
        }
        printf(pkt->kind == VPX_CODEC_CX_FRAME_PKT && (pkt->data.frame.flags & VPX_FRAME_IS_KEY)? "K":".");
        fflush(stdout);
    }
    vpx.frame_cnt++;
}
