#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "VideoRecorder.h"

#ifdef HAVE_H264
#include <stdexcept>

VideoRecorder::VideoRecorder()
  : fmt(nullptr), codec(nullptr), stream(nullptr),
    sws(nullptr), frame(nullptr), frameCounter(0) {}

VideoRecorder::~VideoRecorder()
{
  stop();
}

bool VideoRecorder::start(const char *filename, int width, int height)
{
  if (fmt)
    return false;

  if (avformat_alloc_output_context2(&fmt, nullptr, nullptr, filename) < 0 || !fmt)
    return false;

  AVCodec *enc = avcodec_find_encoder(AV_CODEC_ID_MPEG4);
  if (!enc)
    return false;

  stream = avformat_new_stream(fmt, enc);
  if (!stream)
    return false;

  codec = avcodec_alloc_context3(enc);
  if (!codec)
    return false;

  codec->width = width;
  codec->height = height;
  codec->time_base = {1,30};
  codec->framerate = {30,1};
  codec->pix_fmt = AV_PIX_FMT_YUV420P;
  if (fmt->oformat->flags & AVFMT_GLOBALHEADER)
    codec->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

  if (avcodec_open2(codec, enc, nullptr) < 0)
    return false;

  if (avcodec_parameters_from_context(stream->codecpar, codec) < 0)
    return false;

  if (avio_open(&fmt->pb, filename, AVIO_FLAG_WRITE) < 0)
    return false;

  if (avformat_write_header(fmt, nullptr) < 0)
    return false;

  frame = av_frame_alloc();
  if (!frame)
    return false;
  frame->format = codec->pix_fmt;
  frame->width = width;
  frame->height = height;
  if (av_frame_get_buffer(frame, 0) < 0)
    return false;

  sws = sws_getContext(width, height, AV_PIX_FMT_BGRA,
                       width, height, codec->pix_fmt,
                       SWS_BILINEAR, nullptr, nullptr, nullptr);
  if (!sws)
    return false;

  frameCounter = 0;
  return true;
}

void VideoRecorder::addFrame(const uint8_t *data, int width, int height, int stride)
{
  if (!fmt)
    return;

  const uint8_t *srcSlice[1] = { data };
  int srcStride[1] = { stride };

  sws_scale(sws, srcSlice, srcStride, 0, height, frame->data, frame->linesize);

  frame->pts = frameCounter++;

  AVPacket pkt;
  av_init_packet(&pkt);
  pkt.data = nullptr;
  pkt.size = 0;

  if (avcodec_send_frame(codec, frame) >= 0) {
    while (avcodec_receive_packet(codec, &pkt) == 0) {
      av_packet_rescale_ts(&pkt, codec->time_base, stream->time_base);
      pkt.stream_index = stream->index;
      av_interleaved_write_frame(fmt, &pkt);
      av_packet_unref(&pkt);
    }
  }
}

void VideoRecorder::stop()
{
  if (!fmt)
    return;

  AVPacket pkt;
  avcodec_send_frame(codec, nullptr);
  while (avcodec_receive_packet(codec, &pkt) == 0) {
    av_packet_rescale_ts(&pkt, codec->time_base, stream->time_base);
    pkt.stream_index = stream->index;
    av_interleaved_write_frame(fmt, &pkt);
    av_packet_unref(&pkt);
  }

  av_write_trailer(fmt);

  avcodec_free_context(&codec);
  sws_freeContext(sws);
  av_frame_free(&frame);
  avio_closep(&fmt->pb);
  avformat_free_context(fmt);

  fmt = nullptr;
  codec = nullptr;
  stream = nullptr;
  sws = nullptr;
  frame = nullptr;
  frameCounter = 0;
}

#endif // HAVE_H264
