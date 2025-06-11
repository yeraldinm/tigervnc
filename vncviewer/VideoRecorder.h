#ifndef __VIDEORECORDER_H__
#define __VIDEORECORDER_H__

#include <cstdint>

#if defined(HAVE_H264) && defined(H264_LIBAV)
extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
}

class VideoRecorder {
public:
  VideoRecorder();
  ~VideoRecorder();

  bool start(const char *filename, int width, int height);
  void addFrame(const uint8_t *data, int width, int height, int stride);
  void stop();

private:
  AVFormatContext *fmt;
  AVCodecContext *codec;
  AVStream *stream;
  SwsContext *sws;
  AVFrame *frame;
  int frameCounter;
};
#else
class VideoRecorder {
public:
  bool start(const char *, int, int) { return false; }
  void addFrame(const uint8_t *, int, int, int) {}
  void stop() {}
};

#endif

#endif
