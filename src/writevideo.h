#ifndef WRITEVIDEO_H
#define WRITEVIDEO_H

#include <string>

extern "C" {

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>

}

class writevideo_t {
public:
  writevideo_t( const std::string& filename, const AVCodecContext* srcCodec, uint64_t fps_den, uint64_t fps_num );
  int add_frame( AVFrame* pkt );
  ~writevideo_t();
private:
  AVFormatContext* oc;
  AVStream* st;
  uint64_t frameno;
  uint8_t *video_outbuf;
};

#endif

// Local Variables:
// compile-command: "make -C .."
// c-basic-offset: 2
// indent-tabs-mode: nil
// mode: c++
// End:
