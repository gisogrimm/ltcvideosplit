#include <iostream>

#include "writevideo.h"
#include "error.h"

writevideo_t::writevideo_t(const std::string& filename, const AVCodecContext* srcCodec, uint64_t fps_den, uint64_t fps_num )
  : oc(avformat_alloc_context()),
    frameno(0)
{
  if( !oc )
    throw error_msg_t(__FILE__,__LINE__,"Memory error");
  if (avio_open(&oc->pb, filename.c_str(), AVIO_FLAG_WRITE) < 0) 
    throw error_msg_t(__FILE__,__LINE__,"Could not create output file '%s'.", filename.c_str());
  AVCodec* codec(avcodec_find_encoder( srcCodec->codec_id ));
  if( !codec )
    throw error_msg_t(__FILE__,__LINE__,"Codec %d not found.",srcCodec->codec_id);
  st = avformat_new_stream(oc, codec);
  if (!st) {
    fprintf(stderr, "Could not alloc stream\n");
    exit(1);
  }
  
  

  DEBUG(srcCodec->time_base.den);
  DEBUG(srcCodec->time_base.num);
  DEBUG(st->codec->time_base.den);
  DEBUG(st->codec->time_base.num);
  //AVCodecContext *c(st->codec);
  //c->bit_rate = 400000;
  //c->codec_id = srcCodec->codec_id;
  //c->codec_type = srcCodec->codec_type;
  //c->time_base.num = srcCodec->time_base.num;
  //c->time_base.den = srcCodec->time_base.den;
  //fprintf(stderr, "time_base.num = %d time_base.den = %d\n", c->time_base.num, c->time_base.den);
  //c->width = srcCodec->width;
  //c->height = srcCodec->height;
  //c->pix_fmt = srcCodec->pix_fmt;
  //printf("%d %d %d", c->width, c->height, c->pix_fmt);
  //c->flags = srcCodec->flags;
  //c->flags |= CODEC_FLAG_GLOBAL_HEADER;
  //c->me_range = srcCodec->me_range;
  //c->max_qdiff = srcCodec->max_qdiff;
  //
  //c->qmin = srcCodec->qmin;
  //c->qmax = srcCodec->qmax;
  //
  //c->qcompress = srcCodec->qcompress;
  //
  //c->extradata = srcCodec->extradata;
  //c->extradata_size = srcCodec->extradata_size;

  avcodec_copy_context( st->codec, srcCodec );
  DEBUG(st->codec->time_base.den);
  DEBUG(st->codec->time_base.num);
  //st->codec->time_base.den = fps_den;
  //st->codec->time_base.num = fps_num;
  DEBUG(st->codec->time_base.den);
  DEBUG(st->codec->time_base.num);
  //st->time_base = st->codec->time_base;
  /* open the codec */
  AVDictionary* d(NULL); // "create" an empty dictionary
  av_dict_set(&d, "vpre", "fast", 0); // add an entry
  av_dict_set(&d, "me_range", "16", 0); // add an entry
  av_dict_set(&d, "qdiff", "4", 0); // add an entry
  av_dict_set(&d, "qmin", "10", 0); // add an entry
  av_dict_set(&d, "qmax", "51", 0); // add an entry
  if (avcodec_open2( st->codec, codec, &d ) < 0)
    throw error_msg_t(__FILE__,__LINE__, "could not open codec.");
  /* Write the stream header, if any. */
  DEBUG(1);
  avformat_write_header(oc, NULL);
  DEBUG(1);
}

int writevideo_t::add_frame(AVFrame* frame)
{
  /* encode the image */
  uint32_t out_size = avcodec_encode_video( st->codec, video_outbuf,
                                            video_outbuf_size, frame );
    /* If size is zero, it means the image was buffered. */
    if (out_size > 0) {
      AVPacket pkt;
      av_init_packet(&pkt);
      if (c->coded_frame->pts != AV_NOPTS_VALUE)
	pkt.pts = av_rescale_q(c->coded_frame->pts,
			       c->time_base, st->time_base);
      if (c->coded_frame->key_frame)
	pkt.flags |= AV_PKT_FLAG_KEY;
      pkt.stream_index = st->index;
      pkt.data = video_outbuf;
      pkt.size = out_size;
      /* Write the compressed frame to the media file. */
      ret = av_interleaved_write_frame(oc, &pkt);
    } else {
      ret = 0;
    }
  }
  //st->codec->time_base.den = fps_den;
  //st->codec->time_base.num = fps_num;
  //pkt->pts = frameno*st->codec->time_base.den/st->codec->time_base.num;
  //pkt->dts = pkt->pts;
  pkt->pts = AV_NOPTS_VALUE;
  pkt->dts = AV_NOPTS_VALUE;
  pkt->stream_index = 0;
  pkt->flags |= AV_PKT_FLAG_KEY;
  ++frameno;
  return av_interleaved_write_frame(oc, pkt);
  //return 0;
}

writevideo_t::~writevideo_t()
{
  av_write_trailer(oc);
//  //close_video(oc, video_st);
//  for (uint32_t i = 0; i < oc->nb_streams; i++) {
//    av_freep(&oc->streams[i]->codec);
//    av_freep(&oc->streams[i]);
//  }
  /* Close the output file. */
  avio_close(oc->pb);
  /* free the stream */
  av_free(oc);
}

// Local Variables:
// compile-command: "make -C .."
// c-basic-offset: 2
// indent-tabs-mode: nil
// mode: c++
// End:
