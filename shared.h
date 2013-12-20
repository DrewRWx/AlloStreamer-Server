#ifndef SHARED_H
#define SHARED_H
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/opt.h>
#include <libavutil/imgutils.h>
#include <libavutil/time.h>
#include <libswscale/swscale.h>
#include <x264.h>
}

#include <pthread.h>
#include "ConcurrentQueue.h"

#include "../shared2.h"

extern ConcurrentQueue<x264_nal_t> m_queue;
extern struct SwsContext* convertCtx;
extern x264_param_t param;
extern x264_t* encoder;
extern x264_picture_t pic_in, pic_out;
extern unsigned char image[image_width*image_height*3];
extern pthread_mutex_t mutex;
extern AVCodec *codec;
extern AVCodecContext *c;
extern AVFrame *frame;
extern AVFrame *iframe;
extern AVPacket pkt;
#endif
