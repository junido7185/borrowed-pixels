#pragma once

#include <cstdint>

namespace bp {

inline constexpr std::uint32_t kAudioSampleRate=48000;
inline constexpr std::uint8_t kAudioChannels=2;
inline constexpr std::uint16_t kAudioBufferFrames=512;
inline constexpr std::uint8_t kAudioBufferCount=4;
inline constexpr std::uint8_t kAudioVoiceCount=8;
inline constexpr std::uint32_t kAudioStepNumerator=2880000;
inline constexpr std::uint16_t kAudioStepDenominator=448;

enum class Oscillator : std::uint8_t { Triangle=0,Pulse25,Noise };
enum class AudioEvent : std::uint8_t {
    AmbientClick=0,MenuMove,Reclaim4,Tap,Cut,HudTransitTick,Claim,CoverBreak,
    HudTransitEnd,ShellHit,Foreclose,ChargeIntercept,HudDue,CoreExposed,
    CoreHit,Settlement,MenuConfirm,HudDismantle,HudRestore,Count,
};
enum class AudioEnding : std::uint8_t { None=0,Enough,Rule,Hand,Open,One };

struct AudioVoice {
    std::uint32_t phase;
    std::uint32_t startIncrement;
    std::uint32_t endIncrement;
    std::uint32_t ageFrames;
    std::uint32_t durationFrames;
    std::uint32_t attackFrames;
    std::uint32_t releaseFrames;
    std::uint32_t noiseState;
    std::int16_t panQ15;
    Oscillator oscillator;
    std::uint8_t priority;
    std::uint8_t active;
};

struct AudioState {
    AudioVoice voices[kAudioVoiceCount];
    std::uint32_t stepAccumulator;
    std::uint32_t noiseState;
    std::uint8_t step;
    std::uint8_t reclaimCycle;
    std::uint8_t masterVolume;
    std::uint8_t paused;
    std::uint8_t focused;
    std::uint8_t available;
    AudioEnding ending;
};

void InitializeAudio(AudioState& state,bool available) noexcept;
void SetAudioMixState(AudioState& state,std::uint8_t masterVolume,bool paused,
    bool focused,AudioEnding ending) noexcept;
bool QueueAudioEvent(AudioState& state,AudioEvent event) noexcept;
void RenderAudioFrames(AudioState& state,std::int16_t* interleaved,
    std::uint32_t frames) noexcept;

}  // namespace bp
