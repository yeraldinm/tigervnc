#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <x265.h>
#include <rfb/PixelBuffer.h>
#include <rfb/H265X265DecoderContext.h>

using namespace rfb;

H265X265DecoderContext::H265X265DecoderContext(const core::Rect& r)
  : H265DecoderContext(r)
{
  x265_param_default(&param);
}

H265X265DecoderContext::~H265X265DecoderContext()
{
}

void H265X265DecoderContext::decode(const uint8_t* /*h265_buffer*/, uint32_t /*len*/,
                                    ModifiablePixelBuffer* /*pb*/)
{
  // TODO: Implement decoding using libx265
}
