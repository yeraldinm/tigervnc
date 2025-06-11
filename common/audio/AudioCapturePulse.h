#ifndef TIGERVNC_AUDIOCAPTURE_PULSE_H
#define TIGERVNC_AUDIOCAPTURE_PULSE_H

#include "AudioCapture.h"

#ifdef HAVE_PULSEAUDIO
#include <pulse/simple.h>
#endif

namespace audio {
class AudioCapturePulse : public AudioCapture {
public:
    AudioCapturePulse();
    ~AudioCapturePulse();

    bool init() override;
    bool readFrame(std::vector<uint8_t>& buffer) override;

private:
#ifdef HAVE_PULSEAUDIO
    pa_simple* handle;
#endif
};
}

#endif
