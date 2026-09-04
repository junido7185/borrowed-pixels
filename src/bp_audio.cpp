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
constexpr std::uint8_t kEmptyMechanicalSteps[10]={1,2,4,5,7,9,10,12,13,15};
constexpr std::uint16_t kFullGainQ15=32767;
constexpr std::uint16_t kMusicGainQ15=8231;       // -12dBFS
constexpr std::uint16_t kSfxGainQ15=16422;        // -6dBFS
constexpr std::uint16_t kSettlementGainQ15=13045; // -8dBFS
constexpr std::uint16_t kMenuGainQ15=4125;        // -18dBFS
constexpr std::uint16_t kPauseRampFrames=4800;
constexpr std::uint32_t kStepFrames=(kAudioStepNumerator+kAudioStepDenominator/2u)
    /kAudioStepDenominator;

struct MidiPhase { std::uint8_t note; std::uint32_t increment; };
constexpr MidiPhase kMidiPhase[]={
    {19,2192197u},{21,2460658u},{24,2926232u},{26,3284585u},
    {31,4384395u},{33,4921317u},{36,5852465u},{38,6569170u},
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
    case Oscillator::Noise: return static_cast<std::int16_t>(voice.noiseState>>16u);
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
        const std::uint32_t remaining=voice.delayFrames
            +(voice.durationFrames>voice.ageFrames?voice.durationFrames-voice.ageFrames:0u);
        const AudioVoice& current=state.voices[selected<kAudioVoiceCount?selected:index];
        const std::uint32_t currentRemaining=current.delayFrames
            +(current.durationFrames>current.ageFrames?current.durationFrames-current.ageFrames:0u);
        if (selected==kAudioVoiceCount || voice.priority<state.voices[selected].priority
            || (voice.priority==state.voices[selected].priority
                && remaining<currentRemaining))
            selected=index;
    }
    return selected;
}

void QueueTone(AudioState& state,Oscillator oscillator,std::uint32_t startIncrement,
    std::uint32_t endIncrement,std::uint32_t durationFrames,std::uint8_t priority,
    std::int16_t panQ15=0,AudioBus bus=AudioBus::Music,
    std::uint16_t velocityQ15=kFullGainQ15,std::uint32_t delayFrames=0,
    std::uint32_t attackFrames=48,std::uint32_t releaseFrames=0) noexcept {
    const std::uint8_t index=ReplacementVoice(state,priority);
    if (index>=kAudioVoiceCount) return;
    AudioVoice& voice=state.voices[index];
    voice=AudioVoice{};
    voice.startIncrement=startIncrement;
    voice.endIncrement=endIncrement;
    voice.durationFrames=durationFrames;
    voice.delayFrames=delayFrames;
    voice.attackFrames=attackFrames;
    voice.releaseFrames=releaseFrames!=0?releaseFrames:durationFrames/4u;
    voice.noiseState=state.noiseState^static_cast<std::uint32_t>(index+1u)*0x9e3779b9u;
    voice.panQ15=panQ15;
    voice.velocityQ15=velocityQ15;
    voice.oscillator=oscillator;
    voice.bus=bus;
    voice.priority=priority;
    voice.active=1;
}

AudioBus EventBus(AudioEvent event) noexcept {
    if (event==AudioEvent::Settlement) return AudioBus::Settlement;
    if (event==AudioEvent::MenuMove) return AudioBus::Menu;
    return AudioBus::Sfx;
}

void QueueEventTone(AudioState& state,AudioEvent event,const EventDef& definition,
    std::uint16_t startHz,std::uint16_t endHz,std::uint16_t durationMs,
    std::uint32_t delayFrames=0) noexcept {
    QueueTone(state,definition.oscillator,HertzIncrement(startHz),HertzIncrement(endHz),
        MillisecondsToFrames(durationMs),definition.priority,definition.panQ15,
        EventBus(event),kFullGainQ15,delayFrames,
        MillisecondsToFrames(definition.attackMs),MillisecondsToFrames(definition.releaseMs));
}

Oscillator PrecedentOscillator(const AudioState& state,std::uint8_t step) noexcept {
    if (state.contextA==0u) return Oscillator::Pulse25;
    if (state.contextA==1u) return Oscillator::Triangle;
    return (step&1u)==0u?Oscillator::Pulse25:Oscillator::Triangle;
}

void QueueMidi(AudioState& state,Oscillator oscillator,std::uint8_t note,
    std::uint32_t frames,std::int16_t pan=0,std::uint16_t velocity=kFullGainQ15) noexcept {
    QueueTone(state,oscillator,MidiIncrement(note),MidiIncrement(note),frames,1,
        pan,AudioBus::Music,velocity);
}

void QueuePrecedentCounter(AudioState& state,std::uint8_t note) noexcept {
    if (state.contextC==2u) {
        std::int16_t pans[3]={-18022,18022,0}; // PLAYER, OPPONENT, COMMON
        const std::uint8_t majority=state.contextG==3u?0u:
            (state.contextG==5u?1u:2u); // TAP, CLAIM, RECLAIM
        pans[majority]=0;
        for (std::uint8_t channel=0;channel<3;++channel)
            QueueMidi(state,PrecedentOscillator(state,state.step),note,
                kAudioSampleRate/14u,pans[channel],23197);
        return;
    }
    std::uint16_t left=23197;
    std::uint16_t right=23197;
    const std::uint8_t favored=state.contextB==1u?state.contextE:
        (state.contextB==2u?state.contextF:2u);
    if (favored==0u) left=kFullGainQ15;
    else if (favored==1u) right=kFullGainQ15;
    QueueMidi(state,PrecedentOscillator(state,state.step),note,
        kAudioSampleRate/14u,-18022,left);
    QueueMidi(state,PrecedentOscillator(state,state.step),note,
        kAudioSampleRate/14u,18022,right);
}

void QueuePrecedentMemory(AudioState& state,std::uint8_t step) noexcept {
    if (state.contextD==2u||!(step==1u||step==5u||step==9u)) return;
    if (state.contextD==1u) {
        QueueMidi(state,Oscillator::Triangle,26,kAudioSampleRate/20u);
        return;
    }
    constexpr std::uint8_t motifNotes[10]={38,33,36,31,38,33,36,31,38,33};
    const std::uint8_t index=step==1u?0u:(step==5u?1u:2u);
    const std::uint8_t motif=state.echoMotif[index]<10u?state.echoMotif[index]:0u;
    QueueMidi(state,PrecedentOscillator(state,step),motifNotes[motif],kAudioSampleRate/16u);
}

bool CurrentMechanical(const AudioState& state,std::uint8_t step) noexcept {
    if (kMechanicalPattern[step]!=0) return true;
    const std::uint8_t additions=state.contextE>10u?10u:state.contextE;
    for (std::uint8_t index=0;index<additions;++index)
        if (kEmptyMechanicalSteps[index]==step) return true;
    return false;
}

void ScheduleStep(AudioState& state) noexcept {
    if (state.ending==AudioEnding::One) return;
    const std::uint8_t step=state.step&15u;
    if (state.ending==AudioEnding::Open) {
        const std::uint8_t offsets[3]={step,static_cast<std::uint8_t>((step+5u)&15u),
            static_cast<std::uint8_t>((step+10u)&15u)};
        const std::int16_t pans[3]={-18022,0,18022};
        for (std::uint8_t channel=0;channel<3;++channel) {
            const std::int8_t note=step==0u?38:kBassPattern[offsets[channel]];
            if (note>=0) QueueMidi(state,Oscillator::Triangle,
                static_cast<std::uint8_t>(note),kAudioSampleRate/10u,pans[channel]);
        }
        state.step=static_cast<std::uint8_t>((step+1u)&15u);
        return;
    }
    if (state.ending==AudioEnding::Rule) {
        QueueMidi(state,Oscillator::Triangle,38,kAudioSampleRate/14u);
        state.step=static_cast<std::uint8_t>((step+1u)&15u);
        return;
    }

    std::uint8_t patternStep=step;
    if (state.encounter==AudioEncounter::Precedent&&state.contextC==1u) {
        if ((step&3u)==0u) state.judgmentRow=static_cast<std::uint8_t>(state.contextG&3u);
        patternStep=static_cast<std::uint8_t>((step+(state.judgmentRow*4u))&15u);
    }
    Oscillator bassOscillator=Oscillator::Triangle;
    Oscillator counterOscillator=Oscillator::Pulse25;
    std::int8_t bass=kBassPattern[patternStep];
    std::int8_t counter=kCounterPattern[patternStep];
    std::uint32_t bassFrames=kAudioSampleRate/10u;
    bool mechanical=kMechanicalPattern[patternStep]!=0;

    switch (state.encounter) {
    case AudioEncounter::Patch:
        bassOscillator=counterOscillator=(step<4u||(step>=8u&&step<12u))
            ?Oscillator::Triangle:Oscillator::Pulse25;
        break;
    case AudioEncounter::Pair:
        if (state.pairCounterPendingNote!=0xffu) {
            QueueMidi(state,Oscillator::Pulse25,state.pairCounterPendingNote,
                kAudioSampleRate/14u,18022);
            state.pairCounterPendingNote=0xffu;
        }
        if (counter>=0) {
            QueueMidi(state,Oscillator::Pulse25,static_cast<std::uint8_t>(counter),
                kAudioSampleRate/14u,-18022);
            state.pairCounterPendingNote=static_cast<std::uint8_t>(counter);
            counter=-1;
        }
        break;
    case AudioEncounter::Vessel:
        if (bass>=0) bass=static_cast<std::int8_t>(bass-12);
        bassFrames=kStepFrames*3u;
        counter=step==6u?60:(step==14u?65:-1);
        break;
    case AudioEncounter::Keeper: {
        const std::uint8_t clicks[5]={0,3,6,9,12};
        const std::uint8_t retained=state.contextA>5u?5u:state.contextA;
        for (std::uint8_t index=0;index<retained;++index)
            if (clicks[index]==step) QueueTone(state,Oscillator::Noise,
                HertzIncrement(1800),HertzIncrement(900),kAudioSampleRate/100u,1);
        break;
    }
    case AudioEncounter::Adapter:
        if (state.contextB<32u) counter=kCounterPattern[(step+15u)&15u];
        break;
    case AudioEncounter::Frame: {
        constexpr std::int8_t frameBass[4]={38,33,31,36};
        bass=(step&3u)==0u?frameBass[step/4u]:-1;
        break;
    }
    case AudioEncounter::Witness:
        if ((state.contextD>=1u&&step==14u)||(state.contextD>=2u&&step==10u)
            ||(state.contextD>=3u&&step==6u)) counter=-1;
        break;
    case AudioEncounter::Current:
        mechanical=CurrentMechanical(state,step);
        break;
    case AudioEncounter::Precedent: {
        bassOscillator=counterOscillator=PrecedentOscillator(state,step);
        constexpr std::uint8_t equality[4]={57,60,62,65};
        constexpr std::uint8_t need[4]={57,62,60,65};
        constexpr std::uint8_t utility[4]={65,62,60,57};
        if ((patternStep&3u)==2u) {
            const std::uint8_t index=patternStep/4u;
            const std::uint8_t note=state.contextB==0u?equality[index]:
                (state.contextB==1u?need[index]:utility[index]);
            QueuePrecedentCounter(state,note);
        }
        counter=-1;
        QueuePrecedentMemory(state,step);
        break;
    }
    default: break;
    }

    if (bass>=0) QueueMidi(state,bassOscillator,static_cast<std::uint8_t>(bass),bassFrames);
    if (counter>=0&&state.ending!=AudioEnding::Enough)
        QueueMidi(state,counterOscillator,static_cast<std::uint8_t>(counter),
            kAudioSampleRate/14u);
    if (mechanical&&state.ending==AudioEnding::None)
        QueueTone(state,Oscillator::Noise,HertzIncrement(1800),HertzIncrement(900),
            kAudioSampleRate/100u,1);
    state.step=static_cast<std::uint8_t>((step+1u)&15u);
}

}  // namespace

void InitializeAudio(AudioState& state,bool available) noexcept {
    state=AudioState{};
    state.noiseState=0x42505831u;
    state.musicPauseGainQ15=kFullGainQ15;
    state.musicPauseRampStartQ15=kFullGainQ15;
    state.pairCounterPendingNote=0xffu;
    state.masterVolume=10;
    state.focused=1;
    state.available=available?1u:0u;
}

void SetAudioMixState(AudioState& state,std::uint8_t masterVolume,bool paused,
    bool focused,AudioEnding ending) noexcept {
    state.masterVolume=masterVolume<=10?masterVolume:10;
    const std::uint8_t nextPaused=paused?1u:0u;
    if (nextPaused!=state.paused) {
        state.musicPauseRampStartQ15=state.musicPauseGainQ15;
        state.musicPauseRampFrames=kPauseRampFrames;
    }
    state.paused=nextPaused;
    state.focused=focused?1u:0u;
    if (ending==AudioEnding::One&&state.ending!=AudioEnding::One)
        for (AudioVoice& voice:state.voices)
            if (voice.active!=0) {
                if (voice.delayFrames!=0) voice.active=0;
                else {
                    voice.durationFrames=voice.ageFrames+MillisecondsToFrames(180);
                    voice.releaseFrames=MillisecondsToFrames(180);
                }
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
    if (event==AudioEvent::Settlement) {
        constexpr std::uint16_t notes[3]={220,330,440};
        for (std::uint8_t note=0;note<3;++note)
            QueueEventTone(state,event,definition,notes[note],notes[note],110,
                MillisecondsToFrames(static_cast<std::uint16_t>(note*110u)));
        return true;
    }
    if (event==AudioEvent::CoreExposed) {
        QueueEventTone(state,event,definition,880,880,250);
        QueueEventTone(state,event,definition,660,660,250,MillisecondsToFrames(250));
        return true;
    }
    if (event==AudioEvent::CoreHit) {
        QueueEventTone(state,event,definition,1200,120,180);
        EventDef tone=definition;
        tone.oscillator=Oscillator::Triangle;
        QueueEventTone(state,event,tone,55,55,180);
        return true;
    }
    if (event==AudioEvent::HudDismantle||event==AudioEvent::HudRestore) {
        const std::uint16_t first=event==AudioEvent::HudDismantle?659u:440u;
        const std::uint16_t last=event==AudioEvent::HudDismantle?440u:659u;
        for (std::uint8_t segment=0;segment<4;++segment) {
            const std::uint16_t start=static_cast<std::uint16_t>(first
                +(static_cast<std::int32_t>(last)-first)*segment/4);
            const std::uint16_t end=static_cast<std::uint16_t>(first
                +(static_cast<std::int32_t>(last)-first)*(segment+1u)/4);
            QueueEventTone(state,event,definition,start,end,90,
                MillisecondsToFrames(static_cast<std::uint16_t>(segment*90u)));
        }
        return true;
    }
    if (event==AudioEvent::HudDue) {
        QueueEventTone(state,event,definition,330,330,60);
        QueueEventTone(state,event,definition,220,220,60,MillisecondsToFrames(60));
        return true;
    }
    QueueEventTone(state,event,definition,definition.startHz,definition.endHz,
        definition.durationMs);
    return true;
}

void QueueRuntimeAudioTransitions(AudioState& state,
    const AudioRuntimeSnapshot& before,const AudioRuntimeSnapshot& after,
    bool playerInput) noexcept {
    if (state.ending==AudioEnding::Hand&&playerInput)
        QueueTone(state,Oscillator::Triangle,MidiIncrement(65),MidiIncrement(65),
            kAudioSampleRate/12u,2,0,AudioBus::Music);
    if (after.valid==0) return;
    if (state.encounter!=after.encounter) state.pairCounterPendingNote=0xffu;
    state.encounter=after.encounter;
    state.contextA=after.contextA;
    state.contextB=after.contextB;
    state.contextC=after.contextC;
    state.contextD=after.contextD;
    state.contextE=after.contextE;
    state.contextF=after.contextF;
    state.contextG=after.contextG;
    for (std::uint8_t index=0;index<3;++index) state.echoMotif[index]=after.echoMotif[index];
    if (after.settlement!=0&&before.settlement==0)
        (void)QueueAudioEvent(state,AudioEvent::Settlement);
    if (after.commit!=0&&before.commit==0)
        (void)QueueAudioEvent(state,AudioEvent::MenuConfirm);
    if (after.duePulse!=0)
        (void)QueueAudioEvent(state,AudioEvent::HudDue);
    if (before.valid==0||before.route!=after.route||before.playing==0) return;

    state.reclaimAccumulator=static_cast<std::uint16_t>(
        state.reclaimAccumulator+after.playerReclaimedMass);
    while (state.reclaimAccumulator>=4u) {
            (void)QueueAudioEvent(state,AudioEvent::Reclaim4);
        state.reclaimAccumulator=static_cast<std::uint16_t>(state.reclaimAccumulator-4u);
    }
    if (after.projectileMass>before.projectileMass) {
        const std::uint16_t mass=static_cast<std::uint16_t>(
            after.projectileMass-before.projectileMass);
        const AudioEvent event=mass>=48?AudioEvent::Foreclose:
            (mass>=32?AudioEvent::Claim:(mass>=16?AudioEvent::Cut:AudioEvent::Tap));
        (void)QueueAudioEvent(state,event);
    }
    if (after.coverMass<before.coverMass)
        (void)QueueAudioEvent(state,AudioEvent::CoverBreak);
    if (after.opponentShell<before.opponentShell||after.playerShell<before.playerShell)
        (void)QueueAudioEvent(state,AudioEvent::ShellHit);
    if (before.opponentShell>40u&&after.opponentShell<=40u)
        (void)QueueAudioEvent(state,AudioEvent::CoreExposed);
    const bool newCoreSnapshot=after.coreHitValid!=0
        &&(before.coreHitValid==0||after.coreHitTick!=before.coreHitTick
            ||after.coreHitSide!=before.coreHitSide);
    if (after.playerCoreHit!=0||after.opponentCoreHit!=0||newCoreSnapshot)
        (void)QueueAudioEvent(state,AudioEvent::CoreHit);
    if (after.chargeMass<before.chargeMass&&after.projectileMass<=before.projectileMass)
        (void)QueueAudioEvent(state,AudioEvent::ChargeIntercept);
    for (std::uint8_t module=0;module<kAudioChannels+1u;++module) {
        if (after.hudState[module]!=before.hudState[module]) {
            if (after.hudState[module]==1u)
                (void)QueueAudioEvent(state,AudioEvent::HudDismantle);
            else if (after.hudState[module]==4u)
                (void)QueueAudioEvent(state,AudioEvent::HudRestore);
            else if (after.hudState[module]==0u&&before.hudState[module]!=0u)
                (void)QueueAudioEvent(state,AudioEvent::HudTransitEnd);
        }
        if (after.transitMass[module]!=before.transitMass[module])
            (void)QueueAudioEvent(state,AudioEvent::HudTransitTick);
    }
    if (after.encounter==AudioEncounter::Frame&&before.contextC!=after.contextC)
        QueueTone(state,Oscillator::Noise,HertzIncrement(1800),HertzIncrement(900),
            MillisecondsToFrames(35),1,0,AudioBus::Music);
}

void RenderAudioFrames(AudioState& state,std::int16_t* interleaved,
    std::uint32_t frames) noexcept {
    if (!interleaved) return;
    for (std::uint32_t frame=0;frame<frames;++frame) {
        if (state.musicPauseRampFrames!=0) {
            const std::uint16_t target=state.paused!=0?kMusicGainQ15:kFullGainQ15;
            const std::uint32_t elapsed=kPauseRampFrames-state.musicPauseRampFrames+1u;
            const std::int32_t delta=static_cast<std::int32_t>(target)
                -state.musicPauseRampStartQ15;
            state.musicPauseGainQ15=static_cast<std::uint16_t>(
                static_cast<std::int32_t>(state.musicPauseRampStartQ15)
                +delta*static_cast<std::int32_t>(elapsed)/kPauseRampFrames);
            --state.musicPauseRampFrames;
        }
        state.stepAccumulator+=kAudioStepDenominator;
        if (state.stepAccumulator>=kAudioStepNumerator) {
            state.stepAccumulator-=kAudioStepNumerator;
            ScheduleStep(state);
        }
        std::int32_t left=0;
        std::int32_t right=0;
        for (AudioVoice& voice:state.voices) {
            if (voice.active==0) continue;
            if (voice.delayFrames!=0) { --voice.delayFrames; continue; }
            if (voice.ageFrames>=voice.durationFrames) { voice.active=0; continue; }
            const std::uint32_t increment=voice.startIncrement
                +static_cast<std::uint32_t>((static_cast<std::int64_t>(
                    static_cast<std::int64_t>(voice.endIncrement)-voice.startIncrement)
                    *voice.ageFrames)/voice.durationFrames);
            const std::int32_t sample=OscillatorSample(voice);
            std::int32_t enveloped=static_cast<std::int32_t>(
                static_cast<std::int64_t>(sample)*EnvelopeQ15(voice)/32767);
            enveloped=static_cast<std::int32_t>(
                static_cast<std::int64_t>(enveloped)*voice.velocityQ15/32767);
            std::uint16_t busGain=kSfxGainQ15;
            if (voice.bus==AudioBus::Music) busGain=kMusicGainQ15;
            else if (voice.bus==AudioBus::Settlement) busGain=kSettlementGainQ15;
            else if (voice.bus==AudioBus::Menu) busGain=kMenuGainQ15;
            if (voice.bus==AudioBus::Music)
                busGain=static_cast<std::uint16_t>(
                    static_cast<std::uint32_t>(busGain)*state.musicPauseGainQ15/32767u);
            enveloped=static_cast<std::int32_t>(
                static_cast<std::int64_t>(enveloped)*busGain/32767);
            const std::int32_t leftGain=32767-voice.panQ15;
            const std::int32_t rightGain=32767+voice.panQ15;
            left+=static_cast<std::int32_t>(static_cast<std::int64_t>(enveloped)*leftGain/65534);
            right+=static_cast<std::int32_t>(static_cast<std::int64_t>(enveloped)*rightGain/65534);
            const std::uint32_t previousPhase=voice.phase;
            voice.phase+=increment;
            if (voice.oscillator==Oscillator::Noise&&voice.phase<previousPhase)
                (void)XorShift(voice.noiseState);
            ++voice.ageFrames;
        }
        std::uint32_t gain=static_cast<std::uint32_t>(state.masterVolume)*state.masterVolume*32767u/100u;
        if (state.focused==0) gain=static_cast<std::uint32_t>(gain)*kMenuGainQ15/32767u;
        if (state.available==0) gain=0;
        interleaved[frame*2u]=static_cast<std::int16_t>(ClampSample(
            static_cast<std::int32_t>(static_cast<std::int64_t>(left)*gain/32767)));
        interleaved[frame*2u+1u]=static_cast<std::int16_t>(ClampSample(
            static_cast<std::int32_t>(static_cast<std::int64_t>(right)*gain/32767)));
    }
    state.noiseState=XorShift(state.noiseState);
}

}  // namespace bp
