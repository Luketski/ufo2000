#ifndef VIDEO_CONVERSION_H
#define VIDEO_CONVERSION_H


#include <allegro.h>


void rgb_to_yuv(float r[8][8],float g[8][8],float b[8][8],float y[8][8],float u[8][8],float v[8][8]);
void yuv_to_rgb(float y[8][8],float u[8][8],float v[8][8],float r[8][8],float g[8][8],float b[8][8]);
void rgb__to__yv12 (BITMAP * screen, unsigned char * raw_yv12, unsigned long image_width, unsigned long image_height);




#endif
