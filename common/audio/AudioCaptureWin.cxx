#include "AudioCaptureWin.h"

namespace audio {

AudioCaptureWin::AudioCaptureWin()
#ifdef _WIN32
    : client(nullptr), capture(nullptr), format(nullptr)
#endif
{}

AudioCaptureWin::~AudioCaptureWin()
{
#ifdef _WIN32
    if (capture)
        capture->Release();
    if (client)
        client->Release();
    if (format)
        CoTaskMemFree(format);
#endif
}

bool AudioCaptureWin::init()
{
#ifdef _WIN32
    HRESULT hr;
    IMMDeviceEnumerator* enumerator = nullptr;
    IMMDevice* device = nullptr;

    hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (FAILED(hr))
        return false;

    hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL,
                          IID_PPV_ARGS(&enumerator));
    if (FAILED(hr))
        return false;

    hr = enumerator->GetDefaultAudioEndpoint(eCapture, eConsole, &device);
    enumerator->Release();
    if (FAILED(hr))
        return false;

    hr = device->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr,
                          (void**)&client);
    device->Release();
    if (FAILED(hr))
        return false;

    hr = client->GetMixFormat(&format);
    if (FAILED(hr))
        return false;

    hr = client->Initialize(AUDCLNT_SHAREMODE_SHARED, 0, 10000000, 0, format,
                            nullptr);
    if (FAILED(hr))
        return false;

    hr = client->GetService(IID_PPV_ARGS(&capture));
    if (FAILED(hr))
        return false;

    hr = client->Start();
    return SUCCEEDED(hr);
#else
    return false;
#endif
}

bool AudioCaptureWin::readFrame(std::vector<uint8_t>& buffer)
{
#ifdef _WIN32
    if (!capture)
        return false;

    UINT32 packetLength = 0;
    HRESULT hr = capture->GetNextPacketSize(&packetLength);
    if (FAILED(hr) || packetLength == 0)
        return false;

    BYTE* data = nullptr;
    UINT32 frames = 0;
    DWORD flags;

    hr = capture->GetBuffer(&data, &frames, &flags, nullptr, nullptr);
    if (FAILED(hr))
        return false;

    size_t bytes = frames * format->nBlockAlign;
    buffer.assign(data, data + bytes);
    capture->ReleaseBuffer(frames);
    return true;
#else
    return false;
#endif
}

} // namespace audio
