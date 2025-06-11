#ifndef TIGERVNC_AUDIOCAPTURE_WIN_H
#define TIGERVNC_AUDIOCAPTURE_WIN_H

#include "AudioCapture.h"
#ifdef _WIN32
#include <windows.h>
#include <mmdeviceapi.h>
#include <audioclient.h>
#endif

namespace audio {
class AudioCaptureWin : public AudioCapture {
public:
    AudioCaptureWin();
    ~AudioCaptureWin();

    bool init() override;
    bool readFrame(std::vector<uint8_t>& buffer) override;

private:
#ifdef _WIN32
    IAudioClient* client;
    IAudioCaptureClient* capture;
    WAVEFORMATEX* format;
#endif
};
}

#endif
