#include "bp_audio.h"

#include <cstdint>

namespace bp {
namespace {

struct EventDef {
    Oscillator oscillator;
    std::uint16_t startHz;
    std::uint16_t endHz;
    std::uint16_t durationMs;
    std::uint16_t attackMs;
    std::uint16_t releaseMs;
    std::int16_t panQ15;
    std::uint8_t priority;
};

constexpr EventDef kEvents[static_cast<std::uint8_t>(AudioEvent::Count)]={
    {Oscillator::Pulse25,880,760,18,1,8,0,1},
    {Oscillator::Pulse25,880,760,18,1,8,0,2},
    {Oscillator::Triangle,440,659,22,1,8,0,3},
    {Oscillator::Triangle,220,125,35,1,12,0,4},
    {Oscillator::Triangle,180,90,55,2,18,0,4},
    {Oscillator::Triangle,520,440,30,1,12,0,4},
    {Oscillator::Pulse25,145,72,90,2,24,0,5},
    {Oscillator::Noise,2500,900,45,1,30,0,5},
    {Oscillator::Triangle,440,659,45,2,18,0,5},
    {Oscillator::Pulse25,700,260,45,1,18,0,6},
    {Oscillator::Triangle,110,48,140,4,40,0,6},
    {Oscillator::Pulse25,960,480,45,1,15,0,7},
    {Oscillator::Pulse25,330,220,120,2,40,0,7},
    {Oscillator::Triangle,880,660,500,4,80,0,8},
    {Oscillator::Noise,1200,120,180,2,80,0,8},
    {Oscillator::Triangle,220,440,330,3,40,0,8},
    {Oscillator::Triangle,440,660,45,2,18,0,4},
    {Oscillator::Triangle,659,440,360,4,60,0,5},
    {Oscillator::Triangle,440,659,360,4,60,0,5},
};

constexpr std::int8_t kBassPattern[16]={38,-1,-1,-1,33,-1,-1,-1,
    36,-1,-1,-1,31,-1,-1,-1};
constexpr std::int8_t kCounterPattern[16]={-1,-1,57,-1,-1,-1,60,-1,
    -1,-1,62,-1,-1,-1,65,-1};
constexpr std::uint8_t kMechanicalPattern[16]={1,0,0,1,0,0,1,0,1,0,0,1,0,0,1,0};

struct MidiPhase { std::uint8_t note; std::uint32_t increment; };
constexpr MidiPhase kMidiPhase[]={
    {26,3284585u},{31,4384395u},{33,4921317u},{36,5852465u},{38,6569170u},
    {57,19685267u},{60,23409859u},{62,26276679u},{65,31248413u},
};

std::uint32_t HertzIncrement(std::uint32_t hertz) noexcept {
    return static_cast<std::uint32_t>((static_cast<std::uint64_t>(hertz)<<32u)
        /kAudioSampleRate);
}

std::uint32_t MidiIncrement(std::uint8_t note) noexcept {
    for (const MidiPhase& entry:kMidiPhase) if (entry.note==note) return entry.increment;
    return 0;
}

std::uint32_t MillisecondsToFrames(std::uint16_t milliseconds) noexcept {
    return static_cast<std::uint32_t>(milliseconds)*kAudioSampleRate/1000u;
}

std::int32_t ClampSample(std::int32_t value) noexcept {
    return value<-32768?-32768:(value>32767?32767:value);
}

std::uint32_t XorShift(std::uint32_t& state) noexcept {
    state^=state<<13u; state^=state>>17u; state^=state<<5u;
    return state;
}

std::int32_t OscillatorSample(AudioVoice& voice) noexcept {
    const std::uint32_t phase=voice.phase;
    switch (voice.oscillator) {
    case Oscillator::Triangle: {
        const std::uint32_t ramp=phase>>16u;
        const std::int32_t triangle=ramp<32768u?static_cast<std::int32_t>(ramp*2u)-32768:
            98303-static_cast<std::int32_t>(ramp*2u);
        return triangle;
    }
    case Oscillator::Pulse25: return phase<0x40000000u?24576:-8192;
    case Oscillator::Noise: return static_cast<std::int16_t>(XorShift(voice.noiseState)>>16u);
    }
    return 0;
}

std::uint32_t EnvelopeQ15(const AudioVoice& voice) noexcept {
    if (voice.attackFrames!=0&&voice.ageFrames<voice.attackFrames)
        return voice.ageFrames*32767u/voice.attackFrames;
    const std::uint32_t remaining=voice.durationFrames-voice.ageFrames;
    if (voice.releaseFrames!=0&&remaining<voice.releaseFrames)
        return remaining*32767u/voice.releaseFrames;
    return 32767u;
}

std::uint8_t ReplacementVoice(const AudioState& state,std::uint8_t incoming) noexcept {
    for (std::uint8_t index=0;index<kAudioVoiceCount;++index)
        if (state.voices[index].active==0) return index;
    std::uint8_t selected=kAudioVoiceCount;
    for (std::uint8_t index=0;index<kAudioVoiceCount;++index) {
        const AudioVoice& voice=state.voices[index];
        const bool eligible=incoming==8u || voice.priority<incoming;
        if (!eligible) continue;
        if (selected==kAudioVoiceCount || voice.priority<state.voices[selected].priority
            || (voice.priority==state.voices[selected].priority
                && voice.durationFrames-voice.ageFrames
                    <state.voices[selected].durationFrames-state.voices[selected].ageFrames))
            selected=index;
    }
    return selected;
}

void QueueTone(AudioState& state,Oscillator oscillator,std::uint32_t startIncrement,
    std::uint32_t endIncrement,std::uint32_t durationFrames,std::uint8_t priority,
    std::int16_t panQ15=0) noexcept {
    const std::uint8_t index=ReplacementVoice(state,priority);
    if (index>=kAudioVoiceCount) return;
    AudioVoice& voice=state.voices[index];
    voice=AudioVoice{};
    voice.startIncrement=startIncrement;
    voice.endIncrement=endIncrement;
    voice.durationFrames=durationFrames;
    voice.attackFrames=48;
    voice.releaseFrames=durationFrames/4u;
    voice.noiseState=state.noiseState^static_cast<std::uint32_t>(index+1u)*0x9e3779b9u;
    voice.panQ15=panQ15;
    voice.oscillator=oscillator;
    voice.priority=priority;
    voice.active=1;
}

void ScheduleStep(AudioState& state) noexcept {
    if (state.ending==AudioEnding::One) return;
    const std::uint8_t step=state.step&15u;
    if (kBassPattern[step]>=0 && state.ending!=AudioEnding::Enough)
        QueueTone(state,Oscillator::Triangle,MidiIncrement(kBassPattern[step]),
            MidiIncrement(kBassPattern[step]),kAudioSampleRate/10u,1);
    if (kCounterPattern[step]>=0 && state.ending!=AudioEnding::Enough)
        QueueTone(state,Oscillator::Pulse25,MidiIncrement(kCounterPattern[step]),
            MidiIncrement(kCounterPattern[step]),kAudioSampleRate/14u,1);
    if (kMechanicalPattern[step]!=0 && state.ending==AudioEnding::None)
        QueueTone(state,Oscillator::Noise,HertzIncrement(1800),HertzIncrement(900),
            kAudioSampleRate/100u,1);
    state.step=static_cast<std::uint8_t>((step+1u)&15u);
}

}  // namespace

void InitializeAudio(AudioState& state,bool available) noexcept {
    state=AudioState{};
    state.noiseState=0x42505831u;
    state.masterVolume=10;
    state.focused=1;
    state.available=available?1u:0u;
}

void SetAudioMixState(AudioState& state,std::uint8_t masterVolume,bool paused,
    bool focused,AudioEnding ending) noexcept {
    state.masterVolume=masterVolume<=10?masterVolume:10;
    state.paused=paused?1u:0u;
    state.focused=focused?1u:0u;
    if (ending==AudioEnding::One&&state.ending!=AudioEnding::One)
        for (AudioVoice& voice:state.voices)
            if (voice.active!=0) {
                voice.durationFrames=voice.ageFrames+MillisecondsToFrames(180);
                voice.releaseFrames=MillisecondsToFrames(180);
            }
    state.ending=ending;
}

bool QueueAudioEvent(AudioState& state,AudioEvent event) noexcept {
    const std::uint8_t eventIndex=static_cast<std::uint8_t>(event);
    if (eventIndex>=static_cast<std::uint8_t>(AudioEvent::Count)) return false;
    if (state.ending==AudioEnding::One) return true;
    EventDef definition=kEvents[eventIndex];
    if (event==AudioEvent::Reclaim4) {
        constexpr std::uint16_t notes[4]={440,494,554,659};
        definition.startHz=notes[state.reclaimCycle&3u];
        definition.endHz=definition.startHz;
        ++state.reclaimCycle;
    }
    const std::uint8_t voiceIndex=ReplacementVoice(state,definition.priority);
    if (voiceIndex>=kAudioVoiceCount) return true;
    AudioVoice& voice=state.voices[voiceIndex];
    voice=AudioVoice{};
    voice.startIncrement=HertzIncrement(definition.startHz);
    voice.endIncrement=HertzIncrement(definition.endHz);
    voice.durationFrames=MillisecondsToFrames(definition.durationMs);
    voice.attackFrames=MillisecondsToFrames(definition.attackMs);
    voice.releaseFrames=MillisecondsToFrames(definition.releaseMs);
    voice.noiseState=state.noiseState^static_cast<std::uint32_t>(voiceIndex+1u)*0x85ebca6bu;
    voice.panQ15=definition.panQ15;
    voice.oscillator=definition.oscillator;
    voice.priority=definition.priority;
    voice.active=1;
    return true;
}

void RenderAudioFrames(AudioState& state,std::int16_t* interleaved,
    std::uint32_t frames) noexcept {
    if (!interleaved) return;
    for (std::uint32_t frame=0;frame<frames;++frame) {
        state.stepAccumulator+=kAudioStepDenominator;
        if (state.stepAccumulator>=kAudioStepNumerator) {
            state.stepAccumulator-=kAudioStepNumerator;
            ScheduleStep(state);
        }
        std::int32_t left=0;
        std::int32_t right=0;
        for (AudioVoice& voice:state.voices) {
            if (voice.active==0) continue;
            if (voice.ageFrames>=voice.durationFrames) { voice.active=0; continue; }
            const std::uint32_t increment=voice.startIncrement
                +static_cast<std::uint32_t>((static_cast<std::int64_t>(
                    static_cast<std::int64_t>(voice.endIncrement)-voice.startIncrement)
                    *voice.ageFrames)/voice.durationFrames);
            const std::int32_t sample=OscillatorSample(voice);
            const std::int32_t enveloped=static_cast<std::int32_t>(
                static_cast<std::int64_t>(sample)*EnvelopeQ15(voice)/32767);
            const std::int32_t leftGain=32767-voice.panQ15;
            const std::int32_t rightGain=32767+voice.panQ15;
            left+=static_cast<std::int32_t>(static_cast<std::int64_t>(enveloped)*leftGain/65534);
            right+=static_cast<std::int32_t>(static_cast<std::int64_t>(enveloped)*rightGain/65534);
            voice.phase+=increment;
            ++voice.ageFrames;
        }
        std::uint32_t gain=static_cast<std::uint32_t>(state.masterVolume)*state.masterVolume*32767u/100u;
        if (state.paused!=0) gain=gain/4u;
        if (state.focused==0) gain=gain/8u;
        if (state.available==0) gain=0;
        interleaved[frame*2u]=static_cast<std::int16_t>(ClampSample(
            static_cast<std::int32_t>(static_cast<std::int64_t>(left)*gain/32767)));
        interleaved[frame*2u+1u]=static_cast<std::int16_t>(ClampSample(
            static_cast<std::int32_t>(static_cast<std::int64_t>(right)*gain/32767)));
    }
    state.noiseState=XorShift(state.noiseState);
}

}  // namespace bp
