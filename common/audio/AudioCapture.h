#ifndef TIGERVNC_AUDIOCAPTURE_H
#define TIGERVNC_AUDIOCAPTURE_H

#include <vector>
#include <cstdint>

namespace audio {

class AudioCapture {
public:
    virtual ~AudioCapture() {}

    virtual bool init() = 0;
    virtual bool readFrame(std::vector<uint8_t>& buffer) = 0;
};

}

#endif
