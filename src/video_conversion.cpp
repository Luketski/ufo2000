


#include "video_conversion.h"

#include "vpx_encoding.h"











/*================================== RGB-YUV =================================*/
void rgb_to_yuv (float r[8][8], float g[8][8], float b[8][8], float y[8][8], float u[8][8], float v[8][8])
{
    int i,j;

    for ( i=0 ; i<8 ; i++ ){
        for ( j=0 ; j<8 ; j++ ){
            y[i][j]= 0.299 *r[i][j]+0.587 *g[i][j]+0.114 *b[i][j]      ;
            u[i][j]=-0.1687*r[i][j]-0.3313*g[i][j]+0.5   *b[i][j]+128;
            v[i][j]= 0.5   *r[i][j]-0.4187*g[i][j]-0.0813*b[i][j]+128;
        }
    }
}



/*================================== YUV-RGB =================================*/
void yuv_to_rgb (float y[8][8], float u[8][8], float v[8][8], float r[8][8], float g[8][8], float b[8][8])
{
    int i,j;

    for ( i=0 ; i<8 ; i++ ){
        for ( j=0 ; j<8 ; j++ ){
            r[i][j]=y[i][j]			 +1.402  *(v[i][j]-128);
            g[i][j]=y[i][j]-0.34414*(u[i][j]-128)-0.71414*(v[i][j]-128);
            b[i][j]=y[i][j]+1.772  *(u[i][j]-128)		       ;
        }
    }
}



/**
*
*  saves the pixels from the screen to the 'raw_yv12' buffer as yv12 pixels
*  it uses the Allegro getpixel() function, and acquire_screen () must be called before using this function
*
*/
void rgb__to__yv12 (BITMAP * screen, unsigned char * raw_yv12, unsigned long image_width, unsigned long image_height)
{
	//unsigned long image_height = vpx->height; // vpx = global vpx_config struct defined in vpx_encoding.h, and initialized with init_vpx_encoder()
	//unsigned long image_width = vpx->width;
	unsigned long rowbytes = image_width * 3; // FIXME: don't assume this
	
	
	
	#define BLOCK_SIZE 8
	
	float r_pixels [BLOCK_SIZE][BLOCK_SIZE];
	float g_pixels [BLOCK_SIZE][BLOCK_SIZE];
	float b_pixels [BLOCK_SIZE][BLOCK_SIZE];
	
	float y_pixels [BLOCK_SIZE][BLOCK_SIZE];
	float u_pixels [BLOCK_SIZE][BLOCK_SIZE];
	float v_pixels [BLOCK_SIZE][BLOCK_SIZE];
	
	
	// create output buffer
	unsigned long image_bytes = image_height * rowbytes / 2; // YV12 format  w * h * 3 / 2, planar. all Ys first, then all Us, then all Vs.  (Us and Vs have half resolution (only 1 value for each 2x2 pixel block), Ys have full resolution)
	unsigned char * output_buffer = (unsigned char *)raw_yv12;
	
		
	if (!output_buffer)
	{
		return;
	}
	
		
	unsigned long blocks_x = image_width / BLOCK_SIZE + ((image_width % BLOCK_SIZE) ? 1 : 0);
	unsigned long blocks_y = image_height / BLOCK_SIZE + ((image_height % BLOCK_SIZE) ? 1 : 0);
	
	unsigned long bx, by, bx0, by0;
	unsigned int i, j, ip, jp;
	
	unsigned char rgb[3];
	
	unsigned long u_start = image_width * image_height;
	unsigned long v_start = u_start + (image_width * image_height) / 4;
	
	unsigned char u_val, v_val;

	int pixel;
	
	// using allegro
	//acquire_screen ();
	
	for (bx = 0; bx < blocks_x; bx++)
	{
		for (by = 0; by < blocks_y; by++)
		{
			bx0 = bx * BLOCK_SIZE; // position of block in image pixel coordinates
			by0 = by * BLOCK_SIZE; // position of block in image pixel coordinates
			
			// read source image 
			for (i = 0; i < BLOCK_SIZE; i++)
			{
				for (j = 0; j < BLOCK_SIZE; j++)
				{
					ip = bx0 + i;  // position of this pixel in image pixel coordinates
					jp = by0 + j;  // position of this pixel in image pixel coordinates
					
					if (ip < image_width && jp < image_height)
					{	// get that pixel's bytes
					
						// using allegro
						pixel = getpixel (screen, ip, jp);
					
							/*
						rgb[0] = raw_rgb[(jp * rowbytes) + (ip * 3 + 0)];
						rgb[1] = raw_rgb[(jp * rowbytes) + (ip * 3 + 1)];
						rgb[2] = raw_rgb[(jp * rowbytes) + (ip * 3 + 2)];
							*/
						rgb[0] = getr (pixel);
						rgb[1] = getg (pixel);
						rgb[2] = getb (pixel);
					}
					else
					{	// this pixel is outside of the image
						rgb[0] = rgb[1] = rgb[2] = 0;
					}
					
					// copy to the conversion buffer
					r_pixels [i][j] = rgb[0];
					g_pixels [i][j] = rgb[1];
					b_pixels [i][j] = rgb[2];
				}
			}
			
			// convert block
			rgb_to_yuv (r_pixels, g_pixels, b_pixels, y_pixels, u_pixels, v_pixels);
			
			// write target image 
			for (i = 0; i < BLOCK_SIZE; i++)
			{
				for (j = 0; j < BLOCK_SIZE; j++)
				{
					ip = bx0 + i;  // position of this pixel in image pixel coordinates
					jp = by0 + j;  // position of this pixel in image pixel coordinates
					
					if (ip < image_width && jp < image_height)
					{	// write that pixel's bytes
					
						// full resolution
						output_buffer [jp * (image_width) + ip] = y_pixels [i][j];
						
						if (!(i % 2) && !(j % 2))
						{	// half resolution
							u_val = (u_pixels [i][j] + u_pixels [i + 1][j] + u_pixels [i][j + 1] + u_pixels [i + 1][j + 1]) / 4;
							v_val = (v_pixels [i][j] + v_pixels [i + 1][j] + v_pixels [i][j + 1] + v_pixels [i + 1][j + 1]) / 4;
							
							output_buffer [u_start + (jp / 2) * (image_width / 2) + ip / 2] = u_val;
							output_buffer [v_start + (jp / 2) * (image_width / 2) + ip / 2] = v_val;
						}
						
					}
					else
					{	// this pixel is outside of the image
					}
				}
			}
		}
	}
	
	// using allegro
	//release_screen ();

	#undef BLOCK_SIZE
}

