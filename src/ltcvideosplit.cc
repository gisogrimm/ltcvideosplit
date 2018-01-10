/*
  ltcvideosplit - split video files based on audio-embedded LTC code
  Copyright (C) 2016 Giso Grimm

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/
#include <string>
#include <iostream>
#include "error.h"
#include <vector>
#include <map>
#include <ltc.h>
#include <getopt.h>
#include <set>

extern "C" {

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
  //#include <libswscale/swscale.h>

}

//#include "writevideo.h"

#define LTC_QUEUE_LENGTH 160000
#define SAMPLEBUFFERSIZE 2^17

class decoder_t 
{
public:
  decoder_t(const std::string& filename, double audiofps_, const std::set<uint32_t>& decodeframes, uint32_t channel);
  ~decoder_t();
  void scan_frame_map();
  void sort_frames();
private:
  bool readframe();
  bool readframe_sort();
  void process_video(AVPacket* packet);
  void process_audio(AVPacket* packet);
  void process_video_sort(AVPacket* packet);
  AVCodecContext* open_decoder(AVCodecContext*);
  void ff_compute_frame_duration(AVStream *st);
  std::string fname;
  AVFormatContext* pFormatCtx;
  AVCodecContext* pCodecCtxVideo;
  AVCodecContext* pCodecCtxAudio;
  AVFrame *pVideoFrame;
  AVFrame *pAudioFrame;
  int videoStream;
  int audioStream;
  uint32_t frameno;
  // list of PTS in audio samples, from video codec:
  std::vector<int64_t> video_frame_ends;
  // map of LTC frame numbers as function of audio samples:
  std::map<int64_t,uint32_t> ltc_frame_ends;
  LTCDecoder *ltcdecoder;
  int fps_den;
  int fps_num;
  uint32_t frame_duration;
  uint32_t ltc_posinfo;
  uint8_t* samplebuffer;
  uint32_t current_frame;
  uint32_t current_inframe;
  double audiofps;
  std::set<uint32_t> decodeframes_;
  uint32_t channel_;
  //writevideo_t* wrt;
public:
  bool b_list;
};

void decoder_t::scan_frame_map()
{
  while( readframe() );
}

void decoder_t::sort_frames()
{
  av_seek_frame(pFormatCtx,videoStream,0,AVSEEK_FLAG_FRAME);
  while( readframe_sort() );
}

void convert_audio_samples(ltcsnd_sample_t* outbuffer, uint8_t** inbuffer, uint32_t size, uint32_t channels, AVSampleFormat fmt, uint32_t channel)
{
  switch( fmt ){
  case AV_SAMPLE_FMT_S16P : 
    {
      int16_t* lbuf((int16_t*)(inbuffer[channel]));
      for(uint32_t k=0;k<size;++k){
        outbuffer[k] = 128+0.00387573*lbuf[k];
      }
      break;
    }
  case AV_SAMPLE_FMT_S16 : 
    {
      int16_t* lbuf((int16_t*)(inbuffer[0]));
      for(uint32_t k=0;k<size;++k){
        outbuffer[k] = 128+0.00387573*lbuf[k*channels+channel];
      }
      break;
    }
  case AV_SAMPLE_FMT_U8 : 
    {
      uint8_t* lbuf((uint8_t*)inbuffer);
      for(uint32_t k=0;k<size;++k)
        outbuffer[k] = lbuf[k*channels+channel];
      break;
    }
  case AV_SAMPLE_FMT_S32 :
    {
      int32_t* lbuf((int32_t*)(inbuffer[0]));
      for(uint32_t k=0;k<size;++k)
        outbuffer[k] = 128+5.9139e-08*lbuf[k*channels+channel];
      break;
    }
  case AV_SAMPLE_FMT_FLT :
    {
      float* lbuf((float*)(inbuffer[0]));
      for(uint32_t k=0;k<size;++k)
        outbuffer[k] = 128+127*lbuf[k*channels+channel];
      break;
    }
  case AV_SAMPLE_FMT_FLTP : 
    {
      float* lbuf((float*)(inbuffer[channel]));
      for(uint32_t k=0;k<size;++k){
        outbuffer[k] = 128+127*lbuf[k];
      }
      break;
    }
  default:
    throw error_msg_t(__FILE__,__LINE__,"Unsupported sample format \"%s\".",
                      av_get_sample_fmt_name( fmt ) );
  }
}

void decoder_t::ff_compute_frame_duration(AVStream *st)
{
  if( st->codec->codec_type != AVMEDIA_TYPE_VIDEO )
    return;
  if (st->avg_frame_rate.num) {
    fps_num = st->avg_frame_rate.den;
    fps_den = st->avg_frame_rate.num;
  } else if(st->time_base.num*1000LL > st->time_base.den) {
    fps_num = st->time_base.num;
    fps_den = st->time_base.den;
  }else if(st->codec->time_base.num*1000LL > st->codec->time_base.den){
    fps_num = st->codec->time_base.num;
    fps_den = st->codec->time_base.den;
    if (st->parser && st->parser->repeat_pict) {
      if (fps_num > INT_MAX / (1 + st->parser->repeat_pict))
        fps_den /= 1 + st->parser->repeat_pict;
      else
        fps_num *= 1 + st->parser->repeat_pict;
    }
    //If this codec can be interlaced or progressive then we need a parser to compute duration of a packet
    //Thus if we have no parser in such case leave duration undefined.
    if(st->codec->ticks_per_frame>1 && !st->parser){
      fps_num = fps_den = 0;
    }
  }
}

AVCodecContext* decoder_t::open_decoder(AVCodecContext* pCodecCtxOrig)
{
  // Find the decoder for the video stream
  AVCodec* pCodec( avcodec_find_decoder( pCodecCtxOrig->codec_id ) );
  if( !pCodec )
    throw error_msg_t(__FILE__,__LINE__,"Unsupported codec %d.", pCodecCtxOrig->codec_id);
  // Copy context
  AVCodecContext* pCodecCtx( avcodec_alloc_context3( pCodec ) );
  if( avcodec_copy_context( pCodecCtx, pCodecCtxOrig ) != 0) 
    throw error_msg_t(__FILE__,__LINE__,"Couldn't copy codec context.");
  // Open codec
  if( avcodec_open2( pCodecCtx, pCodec, NULL ) < 0 )
    throw error_msg_t(__FILE__,__LINE__,"Couldn't open codec for %s.",pCodec->long_name);
  return pCodecCtx;
}

decoder_t::decoder_t(const std::string& filename, double audiofps_, const std::set<uint32_t>& decodeframes, uint32_t channel)
  : fname(filename),
    pFormatCtx(NULL),pCodecCtxVideo(NULL),pCodecCtxAudio(NULL),
    //pVideoFrame(av_frame_alloc()),
    pVideoFrame(avcodec_alloc_frame()),
    //pAudioFrame(av_frame_alloc()),
    pAudioFrame(avcodec_alloc_frame()),
    videoStream(-1),
    audioStream(-1),
    frameno(0),
    ltcdecoder(NULL),
    fps_den(0),
    fps_num(0),
    ltc_posinfo(0),
    samplebuffer(new uint8_t[SAMPLEBUFFERSIZE]),
    current_frame(0),
    current_inframe(0),
    audiofps(audiofps_),
  decodeframes_(decodeframes),
  channel_(channel),
  b_list(false)
    //,wrt(NULL)
{
  int averr(0);
  if((averr = avformat_open_input(&pFormatCtx, filename.c_str(), NULL, NULL)) < 0){
    char averrs[1024];
    av_strerror(averr,averrs,1024);
    averrs[1023] = '\0';
    throw error_msg_t(__FILE__,__LINE__,"Unable to open video file \"%s\" (%s).",filename.c_str(),averrs);

  }
  try{
    // Retrieve stream information
    if( avformat_find_stream_info( pFormatCtx, NULL) < 0 )
      throw error_msg_t(__FILE__,__LINE__,"Unable to retrieve stream information in video file \"%s\".",filename.c_str());

    // Find the first video stream:
    for(uint32_t i=0; i<pFormatCtx->nb_streams; i++)
      if(pFormatCtx->streams[i]->codec->codec_type==AVMEDIA_TYPE_VIDEO) {
        videoStream=i;
        break;
      }
    // Find first audio stream:
    for(uint32_t i=0; i<pFormatCtx->nb_streams; i++)
      if(pFormatCtx->streams[i]->codec->codec_type==AVMEDIA_TYPE_AUDIO) {
        audioStream=i;
        break;
      }
    if(videoStream==-1)
      throw error_msg_t(__FILE__,__LINE__,"No video stream found in file \"%s\".",filename.c_str());
    if(audioStream==-1)
      throw error_msg_t(__FILE__,__LINE__,"No audio stream found in file \"%s\".",filename.c_str());
    pCodecCtxVideo = open_decoder( pFormatCtx->streams[videoStream]->codec );
    pCodecCtxAudio = open_decoder( pFormatCtx->streams[audioStream]->codec );
    ff_compute_frame_duration(pFormatCtx->streams[videoStream]);
    if( !fps_num )
      throw error_msg_t(__FILE__,__LINE__,"Invalid frame rate (0).");
    frame_duration = fps_num*pCodecCtxAudio->time_base.den/fps_den/pCodecCtxAudio->time_base.num;
    avcodec_default_get_buffer(pCodecCtxAudio, pAudioFrame );
    ltcdecoder = ltc_decoder_create(pCodecCtxAudio->sample_rate * pCodecCtxVideo->time_base.den / std::max(pCodecCtxVideo->time_base.num,1), LTC_QUEUE_LENGTH);
  }
  catch( ... ){
    avformat_close_input(&pFormatCtx);
    throw;
  }
}

decoder_t::~decoder_t()
{
  //if( wrt )
  //  delete wrt;
  delete [] samplebuffer;
  avformat_close_input(&pFormatCtx);
}

bool decoder_t::readframe()
{
  AVPacket packet;
  av_init_packet( &packet );
  if( av_read_frame( pFormatCtx, &packet ) >= 0 ){
    if( packet.stream_index == videoStream ){
      process_video( &packet );
    }else if( packet.stream_index == audioStream ) {
      process_audio( &packet );
    }
    av_free_packet( &packet );
    return true;
  }
  return false;
}

bool decoder_t::readframe_sort()
{
  AVPacket packet;
  av_init_packet( &packet );
  if( av_read_frame( pFormatCtx, &packet ) >= 0 ){
    if( packet.stream_index == videoStream ){
      process_video_sort( &packet );
    }
    av_free_packet( &packet );
    return true;
  }
  return false;
}

void decoder_t::process_video(AVPacket* packet)
{
  video_frame_ends.push_back( packet->pts * 
                              pFormatCtx->streams[videoStream]->time_base.num * 
                              pCodecCtxAudio->time_base.den /
                              pFormatCtx->streams[videoStream]->time_base.den / 
                              pCodecCtxAudio->time_base.num );
  //DEBUG(video_frame_ends.back());
}

int save_frame_as_jpeg(AVCodecContext *pCodecCtx, AVFrame *pFrame, int FrameNo) {
    AVCodec *jpegCodec = avcodec_find_encoder(AV_CODEC_ID_JPEG2000);
    if (!jpegCodec) {
        return -1;
    }
    AVCodecContext *jpegContext = avcodec_alloc_context3(jpegCodec);
    if (!jpegContext) {
        return -1;
    }

    jpegContext->pix_fmt = pCodecCtx->pix_fmt;
    jpegContext->height = pFrame->height;
    jpegContext->width = pFrame->width;

    if (avcodec_open2(jpegContext, jpegCodec, NULL) < 0) {
        return -1;
    }
    FILE *JPEGFile;
    char JPEGFName[256];

    AVPacket packet;
    memset(&packet,0,sizeof(packet));
    av_init_packet(&packet);
    int gotFrame;

    if (avcodec_encode_video2(jpegContext, &packet, pFrame, &gotFrame) < 0) {
        return -1;
    }

    sprintf(JPEGFName, "dvr-%06d.jpg", FrameNo);
    JPEGFile = fopen(JPEGFName, "wb");
    fwrite(packet.data, 1, packet.size, JPEGFile);
    fclose(JPEGFile);

    av_free_packet(&packet);
    avcodec_close(jpegContext);
    return 0;
}

void decoder_t::process_video_sort(AVPacket* packet)
{
  uint64_t aframe( packet->pts * 
                   pFormatCtx->streams[videoStream]->time_base.num * 
                   pCodecCtxAudio->time_base.den /
                   pFormatCtx->streams[videoStream]->time_base.den / 
                   pCodecCtxAudio->time_base.num );
  std::map<int64_t,uint32_t>::iterator lbound( ltc_frame_ends.lower_bound(aframe) );
  std::map<int64_t,uint32_t>::iterator ubound( ltc_frame_ends.upper_bound(aframe-frame_duration) );
  if( (lbound != ltc_frame_ends.end()) && (ubound != ltc_frame_ends.end()) && (lbound->second == ubound->second+1) ){
    if( current_frame != lbound->second ){
      current_frame = lbound->second;
      char fname_new[fname.size()+32];
      sprintf( fname_new, "%s.%05d", fname.c_str(), current_frame );
      int delta_frame((int)current_frame - (int)current_inframe);
      int delta_frame_abs(abs(delta_frame));
      int delta_sec(delta_frame_abs*fps_num/fps_den);
      char stime[32];
      memset(stime,0,32);
      sprintf( stime, "%c%02d:%02d:%02d.%02d",(delta_frame<0)?'-':'+',delta_sec/3600,(delta_sec/60)%60,delta_sec%60,(delta_frame_abs*fps_num)%fps_den );
      if( b_list ){
        std::cout << current_inframe << " " << current_frame << " " << delta_frame << std::endl;
      }else{
        std::cout << current_inframe << " -> " << current_frame << " (" << 
          delta_frame << " " << stime << ")" << std::endl;
      }
      //std::cout << "Creating new file '" << fname_new << "' for frame " << current_frame << " at sample " << aframe << "." << std::endl;
      // create new file:
      //if( wrt )
      //  delete wrt;
      //wrt = new writevideo_t( fname_new, pCodecCtxVideo, fps_den, fps_num );
    }
    // write packet to file:
    //if( wrt )
    //  wrt->add_packet(packet);
  }
  current_frame++;
  current_inframe++;
}


void decoder_t::process_audio(AVPacket* packet)
{
  // decode ltc:
  int got_frame(0);
  avcodec_get_frame_defaults( pAudioFrame );
  uint32_t len(avcodec_decode_audio4(pCodecCtxAudio, pAudioFrame, &got_frame, packet));
  if (len < 0) {
    fprintf(stderr, "Error while decoding\n");
    exit(1);
  }
  if (got_frame) {
    ltcsnd_sample_t ltcsamples[pAudioFrame->nb_samples];
    convert_audio_samples(ltcsamples, pAudioFrame->data, pAudioFrame->nb_samples, pCodecCtxAudio->channels, pCodecCtxAudio->sample_fmt,channel_);
    ltc_decoder_write(ltcdecoder, ltcsamples, pAudioFrame->nb_samples, ltc_posinfo);
    ltc_posinfo += pAudioFrame->nb_samples;
    LTCFrameExt ltcframe;
    while (ltc_decoder_read(ltcdecoder,&ltcframe)) {
      SMPTETimecode stime;
      ltc_frame_to_time(&stime, &ltcframe.ltc, false );
      uint64_t fno(stime.frame+fps_den*(stime.secs+stime.mins*60+stime.hours*3600)/fps_num);
      if( audiofps > 0 )
        fno = stime.frame*fps_den/(audiofps*fps_num)+fps_den*(stime.secs+stime.mins*60+stime.hours*3600)/fps_num;
      ltc_frame_ends[ltcframe.off_end] = fno;
    }

  }else{
    DEBUG("no frame");
  }
}

void app_usage(const std::string& app_name,struct option * opt,const std::string& app_arg = "")
{
  std::cout << "Usage:\n\n" << app_name << " [options] " << app_arg << "\n\nOptions:\n\n";
  while( opt->name ){
    std::cout << "  -" << (char)(opt->val) << " " << (opt->has_arg?"#":"") <<
      "\n  --" << opt->name << (opt->has_arg?"=#":"") << "\n\n";
    opt++;
  }
  std::cout << std::endl;
}


int main(int argc, char** argv)
{
  std::cerr << "ltcvideosplit version " << VERSION_MAJOR << "." << VERSION_MINOR << std::endl;
  try{
    if( argc < 2 )
      throw error_msg_t(__FILE__,__LINE__,"Invalid number of arguments %d.",argc-1);
    av_register_all();
    double audiofps(0);
    std::string filename("");
    std::set<uint32_t> decodeframes;
    uint32_t channel(0);
    const char *options = "hf:d:c:o";
    struct option long_options[] = { 
      { "help", 0, 0, 'h' },
      { "fps",  1, 0, 'f' },
      { "decode", 1, 0, 'd' },
      { "channel", 1, 0, 'c' },
      { "offsetlist", 0, 0, 'o' },
      { 0, 0, 0, 0 }
    };
    int opt(0);
    int option_index(0);
    bool offsetlist(false);
    while( (opt = getopt_long(argc, argv, options,
                              long_options, &option_index)) != -1){
      switch(opt){
      case 'h':
        app_usage("ltcvideosplit",long_options,"filename");
        std::cout << "-f overrides the frame rate embedded in the audio";
        return -1;
      case 'c':
        channel = atoi(optarg);
        break;
      case 'f':
        audiofps = atof(optarg);
        break;
      case 'd':
        decodeframes.insert( atoi( optarg ) );
        break;
      case 'o':
        offsetlist = true;
        break;
      }
    }
    if( optind < argc )
      filename = argv[optind++];
    
    decoder_t dec(filename,audiofps,decodeframes,channel);
    dec.b_list = offsetlist;
    dec.scan_frame_map();
    dec.sort_frames();
    return 0;
  }
  catch( const std::exception& e ){
    std::cerr << "Error: " << e.what() << std::endl;
    return 1;
  }
}

// Local Variables:
// compile-command: "make -C .."
// c-basic-offset: 2
// indent-tabs-mode: nil
// mode: c++
// End:
