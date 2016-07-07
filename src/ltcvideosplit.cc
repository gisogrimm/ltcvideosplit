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

extern "C" {

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>

}

#define DEBUG(x) std::cerr << __FILE__ << ":" << __LINE__ << ": " << #x << "=" << x << std::endl

class decoder_t 
{
public:
  decoder_t(const std::string& filename);
  ~decoder_t();
  bool readframe();
  void process_video(AVPacket* packet);
  void process_audio(AVPacket* packet);
private:
  AVCodecContext* open_decoder(AVCodecContext*);
  AVFormatContext* pFormatCtx;
  AVCodecContext* pCodecCtxVideo;
  AVCodecContext* pCodecCtxAudio;
  AVFrame *pVideoFrame;
  AVFrame *pAudioFrame;
  int videoStream;
  int audioStream;
  uint32_t frameno;
  std::vector<uint32_t> video_frame_starts;
};

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


decoder_t::decoder_t(const std::string& filename)
  : pFormatCtx(NULL),pCodecCtxVideo(NULL),pCodecCtxAudio(NULL),
    pVideoFrame(avcodec_alloc_frame()),
    pAudioFrame(avcodec_alloc_frame()),
    videoStream(-1),
    audioStream(-1),
    frameno(0)
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
    DEBUG( pCodecCtxVideo->time_base.num );
    DEBUG( pCodecCtxVideo->time_base.den );
    DEBUG( pCodecCtxAudio->time_base.num );
    DEBUG( pCodecCtxAudio->time_base.den );
    DEBUG( pCodecCtxAudio->sample_rate );
    DEBUG( pCodecCtxAudio->channels );
  }
  catch( ... ){
    avformat_close_input(&pFormatCtx);
    throw;
  }
}

decoder_t::~decoder_t()
{
  avformat_close_input(&pFormatCtx);
}

bool decoder_t::readframe()
{
  AVPacket packet;
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

void decoder_t::process_video(AVPacket* packet)
{
  video_frame_starts.push_back( packet->pts * 
                                pCodecCtxVideo->time_base.num * 
                                pCodecCtxAudio->time_base.den /
                                pCodecCtxVideo->time_base.den / 
                                pCodecCtxAudio->time_base.num );
}

void decoder_t::process_audio(AVPacket* packet)
{
  // decode ltc:
  int got_frame(0);
  uint32_t len(avcodec_decode_audio4(pCodecCtxAudio, pAudioFrame, &got_frame, packet));
  if (len < 0) {
    fprintf(stderr, "Error while decoding\n");
    exit(1);
  }
  if (got_frame) {
    /* if a frame has been decoded, output it */
    int data_size = av_samples_get_buffer_size(NULL, pCodecCtxAudio->channels,
                                               pAudioFrame->nb_samples,
                                               pCodecCtxAudio->sample_fmt, 1);
    DEBUG(data_size);
    //fwrite(decoded_frame->data[0], 1, data_size, outfile);
  }
}

int main(int argc, char** argv)
{
  std::cerr << "ltcvideosplit version " << VERSION_MAJOR << "." << VERSION_MINOR << std::endl;
  try{
    if( argc < 2 )
      throw error_msg_t(__FILE__,__LINE__,"Invalid number of arguments %d.",argc-1);
    av_register_all();
    decoder_t dec(argv[1]);
    while( dec.readframe() );
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
