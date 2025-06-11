#include "AudioCapturePulse.h"

#ifdef HAVE_PULSEAUDIO
#include <pulse/error.h>
#endif

namespace audio {

AudioCapturePulse::AudioCapturePulse()
#ifdef HAVE_PULSEAUDIO
    : handle(nullptr)
#endif
{}

AudioCapturePulse::~AudioCapturePulse()
{
#ifdef HAVE_PULSEAUDIO
    if (handle)
        pa_simple_free(handle);
#endif
}

bool AudioCapturePulse::init()
{
#ifdef HAVE_PULSEAUDIO
    pa_sample_spec ss;
    ss.format = PA_SAMPLE_S16LE;
    ss.channels = 2;
    ss.rate = 44100;
    int error;
    handle = pa_simple_new(nullptr, "TigerVNC", PA_STREAM_RECORD, nullptr,
                           "record", &ss, nullptr, nullptr, &error);
    return handle != nullptr;
#else
    return false;
#endif
}

bool AudioCapturePulse::readFrame(std::vector<uint8_t>& buffer)
{
#ifdef HAVE_PULSEAUDIO
    if (!handle)
        return false;
    buffer.resize(4096);
    int error;
    if (pa_simple_read(handle, buffer.data(), buffer.size(), &error) < 0)
        return false;
    return true;
#else
    return false;
#endif
}

} // namespace audio
