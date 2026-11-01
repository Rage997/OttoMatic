// Web Audio implementation of Pomme's cmixer interface. Native builds retain cmixer.cpp.
#include "SoundMixer/cmixer.h"
#include <emscripten.h>
#include <algorithm>
#include <cstring>
#include <utility>

namespace cmixer
{
static constexpr int kFixedOne = 1 << 12;
static int hardwareRate = 44100;
static double masterGain = 1;

void InitWithSDL()
{
    hardwareRate = EM_ASM_INT({ return Module.ottoAudio.init(); });
    EM_ASM({ Module.ottoAudio.masterGain($0); }, masterGain);
}

void ShutdownWithSDL() { EM_ASM({ Module.ottoAudio.shutdown(); }); }
double GetMasterGain() { return masterGain; }
void SetMasterGain(double value)
{
    masterGain = value;
    EM_ASM({ Module.ottoAudio.masterGain($0); }, value);
}

Source::Source() { ClearPrivate(); active = false; }
Source::~Source() { EM_ASM({ Module.ottoAudio.release($0); }, this); }

void Source::ClearPrivate()
{
    samplerate = length = sustainOffset = end = state = 0;
    position = 0;
    lgain = rgain = rate = nextfill = 0;
    loop = interpolate = false;
    rewind = true;
    gain = pan = 0;
    onComplete = nullptr;
}

void Source::Clear()
{
    EM_ASM({ Module.ottoAudio.release($0); }, this);
    ClearPrivate();
    active = false;
    ClearImplementation();
}

void Source::Init(int sampleRate, int frames)
{
    samplerate = sampleRate;
    length = frames;
    sustainOffset = 0;
    SetGain(1);
    SetPan(0);
    SetPitch(1);
    SetLoop(false);
    Stop();
}

void Source::RemoveFromMixer()
{
    EM_ASM({ Module.ottoAudio.pause($0); }, this);
    active = false;
}

void Source::Rewind()
{
    EM_ASM({ Module.ottoAudio.rewind($0); }, this);
    RewindImplementation();
    position = 0;
    rewind = false;
    end = length;
    nextfill = 0;
}

void Source::FillBuffer(int offset, int count) { FillBuffer(pcmbuf + offset, count); }
double Source::GetLength() const { return length ? double(length) / samplerate : 0; }
double Source::GetPosition() const
{
    return EM_ASM_DOUBLE({ return Module.ottoAudio.position($0); }, this);
}
int Source::GetState() const { return state; }

void Source::RecalcGains()
{
    const double left = gain * (pan <= 0 ? 1 : 1 - pan);
    const double right = gain * (pan >= 0 ? 1 : 1 + pan);
    lgain = int(left * kFixedOne);
    rgain = int(right * kFixedOne);
    EM_ASM({ Module.ottoAudio.gains($0, $1, $2); }, this, left, right);
}
void Source::SetGain(double value) { gain = value; RecalcGains(); }
void Source::SetPan(double value) { pan = std::clamp(value, -1.0, 1.0); RecalcGains(); }
void Source::SetPitch(double value)
{
    const double pitch = value > 0 ? value : 0.001;
    rate = int(double(samplerate) / hardwareRate * pitch * kFixedOne);
    EM_ASM({ Module.ottoAudio.pitch($0, $1); }, this, pitch);
}
void Source::SetLoop(bool value)
{
    loop = value;
    EM_ASM({ Module.ottoAudio.loop($0, $1, $2); }, this, loop, sustainOffset);
}
void Source::SetInterpolation(bool value)
{
    // Web Audio performs resampling; its interpolation algorithm is browser-defined.
    interpolate = value;
}

void Source::Play()
{
    if (!length) return;
    EM_ASM({ Module.ottoAudio.play($0, $1, $2, $3); }, this, loop, sustainOffset, rewind);
    state = CM_STATE_PLAYING;
    active = true;
    rewind = false;
}
void Source::Pause()
{
    EM_ASM({ Module.ottoAudio.pause($0); }, this);
    state = CM_STATE_PAUSED;
    active = false;
}
void Source::TogglePause()
{
    if (state == CM_STATE_PAUSED) Play();
    else if (state == CM_STATE_PLAYING) Pause();
}
void Source::Stop()
{
    EM_ASM({ Module.ottoAudio.stop($0); }, this);
    state = CM_STATE_STOPPED;
    active = false;
    rewind = true;
}

WavStream::WavStream() { ClearImplementation(); }
void WavStream::ClearImplementation()
{
    bitdepth = channels = idx = 0;
    bigEndian = false;
    span = {};
    userBuffer.clear();
}
void WavStream::Init(int sampleRate, int bits, int nChannels, bool isBigEndian, std::span<char> data)
{
    // GetBuffer/SetBuffer callers hand our own allocation back to Init.
    std::vector<char> owned;
    if (data.data() == userBuffer.data()) owned = std::move(userBuffer);
    Clear();
    if (!owned.empty()) userBuffer = std::move(owned);
    Source::Init(sampleRate, int(data.size() / (bits / 8) / nChannels));
    bitdepth = bits;
    channels = nChannels;
    bigEndian = isBigEndian;
    span = data;
    EM_ASM({ Module.ottoAudio.load($0, $1, $2, $3, $4, $5, $6); },
        this, span.data(), length, samplerate, bitdepth, channels, bigEndian);
}
std::span<char> WavStream::GetBuffer(int size)
{
    userBuffer.resize(size);
    return userBuffer;
}
std::span<char> WavStream::SetBuffer(std::vector<char>&& data)
{
    userBuffer = std::move(data);
    return userBuffer;
}
void WavStream::RewindImplementation() { idx = 0; }
void WavStream::FillBuffer(int16_t* output, int count)
{
    if (!length) { std::memset(output, 0, count * sizeof(*output)); return; }
    const auto* bytes = reinterpret_cast<const uint8_t*>(span.data());
    for (int frame = 0; frame < count / 2; frame++)
    {
        if (idx == length) idx = sustainOffset;
        for (int channel = 0; channel < 2; channel++)
        {
            const int sample = idx * channels + (channels == 1 ? 0 : channel);
            if (bitdepth == 8) output[frame * 2 + channel] = (int(bytes[sample]) - 128) * 256;
            else
            {
                const int offset = sample * 2;
                output[frame * 2 + channel] = int16_t(bigEndian
                    ? (bytes[offset] << 8) | bytes[offset + 1]
                    : (bytes[offset + 1] << 8) | bytes[offset]);
            }
        }
        idx++;
    }
}
}

extern "C" EMSCRIPTEN_KEEPALIVE void ottoAudioEnded(cmixer::Source* source)
{
    source->state = cmixer::CM_STATE_STOPPED;
    source->active = false;
    // Completion handlers may delete or reload their channel.
    auto callback = source->onComplete;
    if (callback) callback();
}
