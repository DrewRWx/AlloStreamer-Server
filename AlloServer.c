/*
 * Copyright (C) 1999-2001  Brian Paul   All Rights Reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/*
 * This is a port of the infamous "gears" demo to straight GLX (i.e. no GLUT)
 * Port by Brian Paul  23 March 2001

 * See usage() below for command line options.
 */

#ifndef INT64_C
#define INT64_C(c) (c ## LL)
#define UINT64_C(c) (c ## ULL)
#endif

#include "MyDeviceSource.hh"
void startRTSP();
void signalNewFrameData();
#include "FrameData.h"
#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <cstring>
#include <cstdlib>
#include <stdio.h>
#include "shared.h"
extern "C" {
#include <math.h>
#include <stdlib.h>

#include <string.h>

#include <pthread.h>
//#define FORSTREAMING 1
//#ifdef FORSTREAMING
//#include <libavcodec/avcodec.h>
//#include <libavutil/opt.h>
//#include <libavutil/imgutils.h>
//#include <libavutil/time.h>
//#include <libswscale/swscale.h>
}

/* disable printf */
//#define printf(...)

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
//#include "shared.h"

extern MyDeviceSource *fileSource;

AVCodec *codec;
AVCodecContext *c= NULL;
AVFrame *frame;
AVFrame *iframe;
AVPacket pkt;

ConcurrentQueue<x264_nal_t> m_queue;
SwsContext* convertCtx = NULL;
x264_param_t param;
x264_t* encoder;
x264_picture_t pic_in, pic_out;
    
//Encoder param
//extern const int image_width = 1920;
//extern const int image_height = 1080;
//extern const int image_width = 960;
//extern const int image_height = 540;

extern const int image_width = 1280;
extern const int image_height = 720;
    
//extern const int image_width = 480;
//extern const int image_height = 270;
//extern const int image_width = 720;
//extern const int image_height = 400;
//extern const int image_width = 64;
//extern const int image_height = 64;
const int bit_rate = 8000000;
const char * preset_val = "ultrafast";
const char * tune_val = "zerolatency:fastdecode";
const int FPS = 60;

//unsigned char *image;
unsigned char image[image_width*image_height*3];


#include <sys/time.h>
#include <unistd.h>


AVFrame *frame1;

using namespace boost::interprocess;
unsigned char randomPixels[image_width*image_height*3];
mapped_region region;
FILE * logz;
static void initEncoder()
{

  int ret;
  //uint8_t endcode[] = { 0, 0, 1, 0xb7 };
  //printf("Encode video file %s\n", filename);
  /* find the mpeg1 video encoder */
  codec = avcodec_find_encoder(AV_CODEC_ID_H264);
  if (!codec) {
    fprintf(stderr, "Codec not found\n");
    exit(1);
  }
  c = avcodec_alloc_context3(codec);
  if (!c) {
    fprintf(stderr, "Could not allocate video codec context\n");
    exit(1);
  }
  /* put sample parameters */
  c->bit_rate = bit_rate;
  /* resolution must be a multiple of two */
  c->width = image_width;
  c->height = image_height;
  /* frames per second */
  c->time_base= (AVRational){1,FPS};
  c->gop_size = 20; /* emit one intra frame every ten frames */
  c->max_b_frames=0;
  c->pix_fmt = AV_PIX_FMT_YUV420P;
 

  //if(codec_id == AV_CODEC_ID_H264){
    av_opt_set(c->priv_data, "preset", preset_val, 0);
    av_opt_set(c->priv_data, "tune", tune_val, 0);
    //av_opt_set(c->priv_data, "x264opts", x264_val, 0);
      
      
    //av_opt_set(c->priv_data, "profile", "high", 0);
    //av_opt_set(c->priv_data, "sliced-threads", "", 0);
    

  //}
  /* open it */
  if (avcodec_open2(c, codec, NULL) < 0) {
    fprintf(stderr, "Could not open codec\n");
    exit(1);
  }
  frame = avcodec_alloc_frame();
  iframe = avcodec_alloc_frame();
  if (!frame|| !iframe) {
    fprintf(stderr, "Could not allocate video frame\n");
    exit(1);
  }
    frame->format = AV_PIX_FMT_YUV420P;//c->pix_fmt;
    frame->width  = image_width;//c->width;
    frame->height = image_height;//c->height;
//  frame->time_base= (AVRational){1,10};

    iframe->format = AV_PIX_FMT_RGB24;
    iframe->width  = image_width;//c->width;
    iframe->height = image_height;//c->height;
  /* the image can be allocated by any means and av_image_alloc() is
   * just the most convenient way if av_malloc() is to be used */
  ret = av_image_alloc(frame->data, frame->linesize, frame->width, frame->height,
    AV_PIX_FMT_YUV420P, 32);
  ret = av_image_alloc(iframe->data, iframe->linesize, iframe->width, iframe->height, AV_PIX_FMT_RGB24, 32);
    
  if (ret < 0) {
    fprintf(stderr, "Could not allocate raw picture buffer\n");
    exit(1);
  }

  convertCtx = sws_getContext(image_width, image_height, PIX_FMT_RGB24, image_width, image_height, PIX_FMT_YUV420P, SWS_FAST_BILINEAR, NULL, NULL, NULL);
  
}


int count=0;
void *addr;
void uploadFrames()
{
  
  while(sharedData->shutdownServer==false/* true*/)
  {
count++;
/*if(count%100==0){*/
/* for(int i=0; i<image_width*image_height*3; i++)*/
/*{*/
/*    randomPixels[i] = rand() % 255;*/
/*}*/
/*}*/

//avpicture_fill((AVPicture *) frame1, /*randomPixels*/sharedData->pixels, AV_PIX_FMT_RGB24,image_width,image_height);
//	count++;
  avpicture_fill((AVPicture *) iframe, sharedData->pixels/*randomPixels*/, AV_PIX_FMT_RGB24,image_width,image_height);

/*    if(frame1->linesize[0] > 0)*/
/*    {*/
/*    	frame1->data[0] += frame1->linesize[0]*(image_height -1);*/
/*    	frame1->linesize[0] = -frame1->linesize[0];*/
/*    }*/

    if(iframe->linesize[0] > 0)
    {
    	iframe->data[0] += iframe->linesize[0]*(image_height -1);
    	iframe->linesize[0] = -iframe->linesize[0];
    }

/*    sws_scale(convertCtx, frame1->data, frame1->linesize,0, image_height, (u_int8_t *const *)pic_in.img.plane, (const int*)pic_in.img.i_stride);*/


    sws_scale(convertCtx, iframe->data, iframe->linesize,0, image_height, frame->data, frame->linesize);
    

/*    x264_nal_t* nals = NULL;*/
/*    int i_nals = 0;*/
/*    int frame_size = -1;*/


/*    frame_size = x264_encoder_encode(encoder, &nals, &i_nals, &pic_in, &pic_out);*/
    
/*    static bool finished = false;*/
/*    */
/*    if (frame_size >= 0)*/
/*    {*/
/*      static bool alreadydone = false;*/
/*      if(!alreadydone)*/
/*      {*/
/*        //printf("adding headers?... \n");*/
/*        */
/*        //x264_encoder_headers(encoder, &nals, &i_nals);*/
/*        alreadydone = true;*/
/*      }*/
/*      */
/*    }*/
/*    fprintf(logz, "\nNals size: %i", i_nals);*/
/*    fflush(logz);*/
/*	for(int i=0; i<i_nals; i++)*/
/*	{*/
/*		fprintf(logz, "Nal type: %i, ", nals[i].p_payload[4]);*/
/*	}*/

/*/*	fprintf(logz, "Queue size: %i\n", m_queue.get_size());*/
/*/*    fflush(logz);*/
/*    for(int i = 0; i < i_nals; ++i)*/
/*    {*/
/*      //printf("adding to queue... \n");*/

/*      m_queue.push(nals[i]);*/
/*    }*/
/*    //signalNewFrameData();*/
/*    */
    usleep(16666);
/*  }*/
}
}


int
main(int argc, char *argv[])
{
    logz = fopen("/home/mathieu/Desktop/Logs/AlloServer.log", "w");


  avcodec_register_all();
  initEncoder();

  

  //Open already created shared memory object.
  shared_memory_object shm (open_only, "MySharedMemory", read_write);
  
  fprintf(logz,"mapping region... \n");
  fflush(logz);
  
  //Map the whole shared memory in this process
  region = mapped_region(shm, read_write);
  
  //Check that memory was initialized to 1
  addr = region.get_address();
  fprintf(logz,"constructing object in memory \n");
  fflush(logz);
  
  sharedData = static_cast<FrameData*>(addr);
  
  startRTSP();

  fprintf(logz, "Uploading frames to libav... \n");
  fflush(logz);

 for(int i=0; i<image_width*image_height*3; i++)
{
    randomPixels[i] = rand() % 255;

/*	fprintf(logz, "%i, ", sharedData->pixels[i]);*/
/*	fflush(logz);*/
/*	if(i%40 == 0)*/
/*	{*/
/*		fprintf(logz,"\n");*/
/*		fflush(logz);*/
/*	}*/
} 



/*  frame1 = avcodec_alloc_frame();*/
/*  frame1->format = AV_PIX_FMT_YUV420P;*/
/*  frame1->width  = image_width;//c->width;*/
/*  frame1->height = image_height;//c->height;*/
/*  int ret2 = av_image_alloc(frame1->data, frame1->linesize, frame1->width, frame1->height, AV_PIX_FMT_YUV420P, 32);*/
  
  //avpicture_fill((AVPicture *) frame1, randomPixels/*sharedData->pixels*/, AV_PIX_FMT_RGB24,image_width,image_height);
  //avpicture_fill((AVPicture *) iframe, /*sharedData->pixels*/randomPixels, AV_PIX_FMT_RGB24,image_width,image_height);

  uploadFrames();//Does not return
  
  unsigned int winWidth = image_width, winHeight = image_height;
  int x = 0, y = 0;


  return 0;
}

