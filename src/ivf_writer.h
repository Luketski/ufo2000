#ifndef IVF_WRITER_H
#define IVF_WRITER_H



#include "vpx_encoding.h"


void init_ivf_writer (file * filename, vpx_config * vpx);
void shutdown_ivf_writer ();

#endif
