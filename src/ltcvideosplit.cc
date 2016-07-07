#include <string>
#include <iostream>

extern "C" {

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>

}

#define DEBUG(x) std::cerr << __FILE__ << ":" << __LINE__ << ": " << #x << "=" << x << std::endl

class error_msg_t : public std::exception {
public:
  error_msg_t(const char* file,int line,const char* fmt,...);
  error_msg_t(const error_msg_t&);
  error_msg_t& operator=(const error_msg_t&);
  ~error_msg_t() throw ();
  const char* what() const throw () { return msg; };
private:
  char* msg;
};

error_msg_t::error_msg_t(const char* s_file,int l,const char* fmt,...)
{
  int len(0);
  va_list ap;
  va_start(ap,fmt);
  char * dummy_str = 0;
  len = vsnprintf(dummy_str,0,fmt,ap);
  len += strlen(s_file)+10;
  msg = new char[len+1];
  snprintf(msg,len+1,"%s:%d: ",s_file,l);
  uint32_t flen(strlen(msg));
  dummy_str = &(msg[flen]);
  va_end(ap);
  va_start(ap,fmt);    
  vsnprintf(dummy_str,len+1-flen,fmt,ap);
  msg[len] = '\0';
  va_end(ap);
}

error_msg_t::~error_msg_t() throw ()
{
  delete [] msg;
}

error_msg_t& error_msg_t::operator=(const error_msg_t& p)
{
  if( msg )
    delete [] msg;
  int len;
  len = strlen(p.msg);
  msg = new char[len+1];
  memset(msg,0,len+1);
  strncpy(msg,p.msg,len);
  return *this;
}

error_msg_t::error_msg_t(const error_msg_t& p)
{
  int len;
  len = strlen(p.msg);
  msg = new char[len+1];
  memset(msg,0,len+1);
  strncpy(msg,p.msg,len);
}


class decoder_t 
{
public:
  decoder_t(const std::string& filename);
  ~decoder_t();
  bool readframe();
private:
  AVFormatContext* pFormatCtx;
  AVCodecContext* pCodecCtx;
  AVCodecContext* pCodecCtxOrig;
  AVCodec *pCodec;
  AVFrame *pFrameSrc;
  int videoStream;
  int audioStream;
  uint32_t frameno;
};

decoder_t::decoder_t(const std::string& filename)
  : pFormatCtx(NULL),pCodecCtx(NULL),pCodecCtxOrig(NULL),
    pCodec(NULL),pFrameSrc(avcodec_alloc_frame()),
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
    if(avformat_find_stream_info(pFormatCtx, NULL)<0)
      throw error_msg_t(__FILE__,__LINE__,"Unable to retrieve stream information in video file \"%s\".",filename.c_str());
    //av_dump_format(pFormatCtx, 0, filename.c_str(), 0);
    // Find the first video stream
    for(uint32_t i=0; i<pFormatCtx->nb_streams; i++)
      if(pFormatCtx->streams[i]->codec->codec_type==AVMEDIA_TYPE_VIDEO) {
        videoStream=i;
        break;
      }
    for(uint32_t i=0; i<pFormatCtx->nb_streams; i++)
      if(pFormatCtx->streams[i]->codec->codec_type==AVMEDIA_TYPE_AUDIO) {
        audioStream=i;
        break;
      }
    if(videoStream==-1)
      throw error_msg_t(__FILE__,__LINE__,"No video stream found in file \"%s\".",filename.c_str());
            
    if(audioStream==-1)
      throw error_msg_t(__FILE__,__LINE__,"No audio stream found in file \"%s\".",filename.c_str());
            
    // Get a pointer to the codec context for the video stream
    pCodecCtxOrig=pFormatCtx->streams[videoStream]->codec;
    // Find the decoder for the video stream
    pCodec=avcodec_find_decoder(pCodecCtxOrig->codec_id);
    if(pCodec==NULL) 
      throw error_msg_t(__FILE__,__LINE__,"Unsupported codec %d in file \"%s\".",pCodecCtx->codec_id,filename.c_str());
    // Copy context
    pCodecCtx = avcodec_alloc_context3(pCodec);
    if(avcodec_copy_context(pCodecCtx, pCodecCtxOrig) != 0) 
      throw error_msg_t(__FILE__,__LINE__,"Couldn't copy codec context. (\"%s\")",filename.c_str());
    // Open codec
    if(avcodec_open2(pCodecCtx, pCodec, NULL)<0)
      throw error_msg_t(__FILE__,__LINE__,"Couldn't open codec. (\"%s\").",filename.c_str());
    // Determine required buffer size and allocate buffer
    //numBytes=avpicture_get_size(PIX_FMT_RGB24, o_width, o_height );

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
  int frameFinished(0);
  bool has_data(true);
  while(!frameFinished){
    AVPacket packet;
    if(av_read_frame(pFormatCtx, &packet)>=0){
      // Is this a packet from the video stream?
      if(packet.stream_index == videoStream) {
        // Decode video frame
        avcodec_decode_video2(pCodecCtx, pFrameSrc, &frameFinished, &packet);
      }else if (packet.stream_index == audioStream) {
        DEBUG("audio");
      }
    }else{
      has_data = false;
      frameFinished = 1;
    }
    av_free_packet(&packet);
  }
  if( has_data ){
    DEBUG(frameno);
    frameno++;
  }
  return has_data;
}

int main(int argc, char** argv)
{
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
