#ifndef VIDEO_CONVERSION_H
#define VIDEO_CONVERSION_H


void rgb_to_yuv(float r[8][8],float g[8][8],float b[8][8],float y[8][8],float u[8][8],float v[8][8]);
void yuv_to_rgb(float y[8][8],float u[8][8],float v[8][8],float r[8][8],float g[8][8],float b[8][8]);
static void rgb__to__yv12 (/*char * raw_rgb,*/ unsigned char * raw_yv12);




#endif
