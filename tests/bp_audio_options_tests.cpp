#include "bp_audio.h"
#include "bp_game.h"
#include "bp_options.h"

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace {

std::uint32_t gChecks;
std::uint32_t gFailures;

void Check(bool condition,const char* name) noexcept {
    ++gChecks;
    if (!condition) { ++gFailures; std::printf("T11_FAIL\t%s\n",name); }
}

bp::InputFrame Press(bp::PlatformAction action) noexcept {
    bp::InputFrame input{};
    input.focused=1;
    input.actions[static_cast<std::uint8_t>(action)].pressed=1;
    return input;
}

void TestOptions() noexcept {
    bp::OptionsRuntime options{};
    bp::InitializeOptionsRuntime(options,10);
    Check(bp::MasterVolume(options.value)==10&&!bp::EnglishLanguage(options.value)
        &&bp::ColorAssist(options.value)==bp::ColorAssistMode::Off,
        "options expose frozen initial fields");
    Check(bp::TickOptionsRuntime(options,Press(bp::PlatformAction::Confirm))
        &&bp::EnglishLanguage(options.value)&&options.changed!=0,
        "LANGUAGE toggles KO to EN");
    for (std::uint8_t row=0;row<4;++row)
        Check(bp::TickOptionsRuntime(options,Press(bp::PlatformAction::MoveDown)),
            "options move through exact row pitch order");
    Check(options.focused==bp::OptionRow::ColorAssist
        &&bp::TickOptionsRuntime(options,Press(bp::PlatformAction::SelectRight))
        &&bp::ColorAssist(options.value)==bp::ColorAssistMode::Shapes,
        "COLOR ASSIST cycles OFF to SHAPES");
    Check(bp::TickOptionsRuntime(options,Press(bp::PlatformAction::SelectRight))
        &&bp::ColorAssist(options.value)==bp::ColorAssistMode::HighContrast,
        "COLOR ASSIST cycles SHAPES to HIGH CONTRAST");
    std::uint32_t pixels[bp::kRenderWidth*bp::kRenderHeight]{};
    bp::PixelBuffer buffer{pixels,bp::kRenderWidth,bp::kRenderHeight,bp::kRenderWidth};
    bp::RenderOptionsRuntime(buffer,options);
    bp::ApplyAccessibilityPostprocess(buffer,options.value);
    bool black=false;
    bool highGrid=false;
    for (std::uint32_t pixel:pixels) {
        black=black||pixel==0;
        highGrid=highGrid||pixel==0x0027313bu;
    }
    Check(black&&highGrid,"HIGH CONTRAST maps BG and GRID exactly");

    bp::PauseRuntime pause{};
    bp::InitializePauseRuntime(pause);
    Check(pause.focused==bp::PauseRow::Resume,"pause opens on RESUME");
    for (std::uint8_t row=0;row<2;++row)
        Check(bp::TickPauseRuntime(pause,Press(bp::PlatformAction::MoveDown)),
            "pause moves through frozen row order");
    Check(pause.focused==bp::PauseRow::RestartEncounter,
        "pause exposes RESTART ENCOUNTER as row3");
    bp::InputFrame hold=Press(bp::PlatformAction::Confirm);
    hold.actions[static_cast<std::uint8_t>(bp::PlatformAction::Confirm)].held=1;
    for (std::uint8_t tick=0;tick<bp::kPauseDestructiveHoldTicks-1u;++tick) {
        Check(bp::TickPauseRuntime(pause,hold),"restart hold advances");
        Check(pause.action==bp::PauseAction::None,"restart cannot fire before tick30");
    }
    Check(bp::TickPauseRuntime(pause,hold)
        &&pause.action==bp::PauseAction::RestartEncounter,
        "restart fires at exact hold tick30");
}

void TestSynthAndSilentFallback() noexcept {
    bp::AudioState first{};
    bp::AudioState second{};
    bp::InitializeAudio(first,true);
    bp::InitializeAudio(second,true);
    Check(bp::QueueAudioEvent(first,bp::AudioEvent::CoreHit)
        &&bp::QueueAudioEvent(second,bp::AudioEvent::CoreHit),
        "priority8 CORE_HIT queues in both deterministic mixers");
    std::int16_t a[bp::kAudioBufferFrames*bp::kAudioChannels]{};
    std::int16_t b[bp::kAudioBufferFrames*bp::kAudioChannels]{};
    bp::RenderAudioFrames(first,a,bp::kAudioBufferFrames);
    bp::RenderAudioFrames(second,b,bp::kAudioBufferFrames);
    Check(std::memcmp(a,b,sizeof(a))==0,"synth output is deterministic and asset-free");
    bool audible=false;
    for (std::int16_t sample:a) audible=audible||sample!=0;
    Check(audible,"available synthesized mixer produces nonzero samples");

    bp::AudioState silent{};
    bp::InitializeAudio(silent,false);
    Check(bp::QueueAudioEvent(silent,bp::AudioEvent::Settlement),
        "silent fallback accepts gameplay events");
    std::int16_t silence[bp::kAudioBufferFrames*bp::kAudioChannels]{};
    bp::RenderAudioFrames(silent,silence,bp::kAudioBufferFrames);
    bool allZero=true;
    for (std::int16_t sample:silence) allZero=allZero&&sample==0;
    Check(allZero,"unavailable waveOut falls back to exact silence");

    bp::GameState game{};
    Check(bp::InitializeGameState(game,1234u),"audio hash fixture game initializes");
    const std::uint32_t before=bp::GameStateHash(game);
    bp::RenderAudioFrames(first,a,bp::kAudioBufferFrames);
    Check(bp::GameStateHash(game)==before,"audio RNG/mixer never changes game hash");
    bp::SetAudioMixState(first,10,false,true,bp::AudioEnding::One);
    for (std::uint8_t index=0;index<bp::kAudioVoiceCount;++index)
        Check(first.voices[index].active==0
            || first.voices[index].releaseFrames==8640,
            "ONE releases every live voice over exact180ms");
    Check(bp::QueueAudioEvent(first,bp::AudioEvent::Tap),
        "ONE accepts but suppresses future events");
}

std::uint8_t ActiveVoices(const bp::AudioState& state) noexcept {
    std::uint8_t count=0;
    for (const bp::AudioVoice& voice:state.voices) count+=voice.active!=0?1u:0u;
    return count;
}

void TestEndingPoliciesAndRuntimeRouting() noexcept {
    std::int16_t samples[6000*bp::kAudioChannels]{};
    bp::AudioState enough{};
    bp::InitializeAudio(enough,true);
    bp::SetAudioMixState(enough,10,false,true,bp::AudioEnding::Enough);
    enough.stepAccumulator=bp::kAudioStepNumerator-bp::kAudioStepDenominator;
    bp::RenderAudioFrames(enough,samples,1);
    Check(ActiveVoices(enough)==1&&enough.voices[0].oscillator==bp::Oscillator::Triangle,
        "ENOUGH retains bass and removes counter");

    bp::AudioState rule{};
    bp::InitializeAudio(rule,true);
    bp::SetAudioMixState(rule,10,false,true,bp::AudioEnding::Rule);
    for (std::uint8_t step=0;step<4;++step) {
        rule.stepAccumulator=bp::kAudioStepNumerator-bp::kAudioStepDenominator;
        bp::RenderAudioFrames(rule,samples,1);
    }
    Check(ActiveVoices(rule)==4,"RULE schedules every step at identical velocity");

    bp::AudioState hand{};
    bp::InitializeAudio(hand,true);
    bp::SetAudioMixState(hand,10,false,true,bp::AudioEnding::Hand);
    bp::AudioRuntimeSnapshot empty{};
    bp::QueueRuntimeAudioTransitions(hand,empty,empty,false);
    Check(ActiveVoices(hand)==0,"HAND adds no cycle-external note without player input");
    bp::QueueRuntimeAudioTransitions(hand,empty,empty,true);
    Check(ActiveVoices(hand)==1,"HAND adds one cycle-external note on player input");

    bp::AudioState open{};
    bp::InitializeAudio(open,true);
    bp::SetAudioMixState(open,10,false,true,bp::AudioEnding::Open);
    open.stepAccumulator=bp::kAudioStepNumerator-bp::kAudioStepDenominator;
    bp::RenderAudioFrames(open,samples,1);
    Check(ActiveVoices(open)==3&&open.voices[0].panQ15<0
        &&open.voices[1].panQ15==0&&open.voices[2].panQ15>0,
        "OPEN joins three offset channels at bar start");

    bp::AudioState routed{};
    bp::InitializeAudio(routed,true);
    bp::AudioRuntimeSnapshot before{};
    before.valid=before.playing=1;
    before.route=2;
    before.playerShell=80;
    before.opponentShell=48;
    before.coverMass=32;
    before.chargeMass=16;
    bp::AudioRuntimeSnapshot after=before;
    after.playerShell=84;
    after.playerReclaimedMass=4;
    after.opponentShell=36;
    after.coverMass=24;
    after.chargeMass=0;
    after.projectileMass=16;
    after.opponentCoreHit=1;
    bp::QueueRuntimeAudioTransitions(routed,before,after,true);
    Check(ActiveVoices(routed)>=5,
        "runtime deltas make reclaim fire cover shell exposure and core SFX reachable");

    bp::AudioState coreTransition{};
    bp::InitializeAudio(coreTransition,true);
    after=before;
    after.playing=0;
    after.opponentCoreHit=1;
    bp::QueueRuntimeAudioTransitions(coreTransition,before,after,false);
    Check(ActiveVoices(coreTransition)==2,
        "Playing-to-HitFreeze transition queues both CORE_HIT voices");

    bp::AudioState accumulated{};
    bp::InitializeAudio(accumulated,true);
    after=before;
    after.playerReclaimedMass=1;
    for (std::uint8_t tick=0;tick<3;++tick)
        bp::QueueRuntimeAudioTransitions(accumulated,before,after,false);
    Check(ActiveVoices(accumulated)==0&&accumulated.reclaimAccumulator==3,
        "RECLAIM_4 waits across three one-TILE ticks");
    bp::QueueRuntimeAudioTransitions(accumulated,before,after,false);
    Check(ActiveVoices(accumulated)==1&&accumulated.reclaimAccumulator==0,
        "fourth one-TILE reclaim tick queues RECLAIM_4");

    bp::AudioState settlement{};
    bp::InitializeAudio(settlement,true);
    Check(bp::QueueAudioEvent(settlement,bp::AudioEvent::Settlement)
        &&ActiveVoices(settlement)==3
        &&settlement.voices[0].delayFrames==0
        &&settlement.voices[1].delayFrames==5280
        &&settlement.voices[2].delayFrames==10560,
        "SETTLEMENT schedules exact sequential 220 330 440Hz note slots");
    Check(settlement.voices[0].bus==bp::AudioBus::Settlement,
        "SETTLEMENT uses dedicated minus8dB bus");
    bp::AudioState buses{};
    bp::InitializeAudio(buses,true);
    Check(bp::QueueAudioEvent(buses,bp::AudioEvent::Tap)
        &&bp::QueueAudioEvent(buses,bp::AudioEvent::MenuMove)
        &&buses.voices[0].bus==bp::AudioBus::Sfx
        &&buses.voices[1].bus==bp::AudioBus::Menu,
        "music SFX settlement and menu gain classes remain distinct");
    bp::SetAudioMixState(buses,10,true,true,bp::AudioEnding::None);
    bp::RenderAudioFrames(buses,samples,1);
    Check(buses.musicPauseGainQ15<32767&&buses.musicPauseGainQ15>8231
        &&buses.musicPauseRampFrames==4799,
        "pause starts a 4800-frame music-only ramp");
    bp::RenderAudioFrames(buses,samples,4799);
    Check(buses.musicPauseGainQ15==8231&&buses.musicPauseRampFrames==0,
        "pause reaches exact additional minus12dB after 100ms");
}

bp::AudioVoice FirstActive(const bp::AudioState& state) noexcept {
    for (const bp::AudioVoice& voice:state.voices) if (voice.active!=0) return voice;
    return bp::AudioVoice{};
}

void ScheduleEncounter(bp::AudioState& state,bp::AudioEncounter encounter,
    std::uint8_t step,std::uint8_t a=0,std::uint8_t b=0,std::uint8_t c=0,
    std::uint8_t d=0,std::uint8_t e=0) noexcept {
    bp::InitializeAudio(state,true);
    bp::AudioRuntimeSnapshot context{};
    context.valid=context.playing=1;
    context.route=1;
    context.encounter=encounter;
    context.contextA=a; context.contextB=b; context.contextC=c;
    context.contextD=d; context.contextE=e;
    bp::QueueRuntimeAudioTransitions(state,context,context,false);
    state.step=step;
    state.stepAccumulator=bp::kAudioStepNumerator-bp::kAudioStepDenominator;
    std::int16_t sample[2]{};
    bp::RenderAudioFrames(state,sample,1);
}

void TestEncounterModifiers() noexcept {
    bp::AudioState patch{};
    ScheduleEncounter(patch,bp::AudioEncounter::Patch,4);
    Check(FirstActive(patch).oscillator==bp::Oscillator::Pulse25,
        "PATCH step4 switches motif waveform to PULSE");

    bp::AudioState pair{};
    ScheduleEncounter(pair,bp::AudioEncounter::Pair,2);
    Check(ActiveVoices(pair)==1&&FirstActive(pair).panQ15<0
        &&pair.pairCounterPendingNote==57,"PAIR emits left counter and retains one-step delay");
    pair.stepAccumulator=bp::kAudioStepNumerator-bp::kAudioStepDenominator;
    std::int16_t pairSample[2]{};
    bp::RenderAudioFrames(pair,pairSample,1);
    bool right=false;
    for (const bp::AudioVoice& voice:pair.voices) right=right||(voice.active!=0&&voice.panQ15>0);
    Check(right,"PAIR emits right counter exactly on the next scheduler step");

    bp::AudioState vessel{};
    ScheduleEncounter(vessel,bp::AudioEncounter::Vessel,4);
    Check(FirstActive(vessel).durationFrames==19287,
        "VESSEL lowers bass octave and gates it for three steps");

    bp::AudioState keeper{};
    ScheduleEncounter(keeper,bp::AudioEncounter::Keeper,0,1);
    Check(ActiveVoices(keeper)>=3,"KEEPER retained-cover click enables step0");

    bp::AudioState adapter{};
    ScheduleEncounter(adapter,bp::AudioEncounter::Adapter,2,0,0);
    Check(ActiveVoices(adapter)==0,"ADAPTER delays base counter by one step below escrow32");

    bp::AudioState frame{};
    ScheduleEncounter(frame,bp::AudioEncounter::Frame,8);
    Check(FirstActive(frame).startIncrement==4384395u,
        "FRAME uses frozen 38 33 31 36 bass order");

    bp::AudioState index{};
    ScheduleEncounter(index,bp::AudioEncounter::Index,2);
    Check(FirstActive(index).startIncrement==19685267u,
        "INDEX retains exact base pattern without timing variation");

    bp::AudioState witness{};
    ScheduleEncounter(witness,bp::AudioEncounter::Witness,14,0,0,0,1);
    Check(ActiveVoices(witness)==1&&FirstActive(witness).oscillator==bp::Oscillator::Noise,
        "WITNESS first lost record removes counter step14");

    bp::AudioState current{};
    ScheduleEncounter(current,bp::AudioEncounter::Current,1,0,0,0,0,1);
    Check(ActiveVoices(current)==1&&FirstActive(current).oscillator==bp::Oscillator::Noise,
        "CURRENT one projectile group fills first empty mechanical step");

    bp::AudioState precedent{};
    ScheduleEncounter(precedent,bp::AudioEncounter::Precedent,2,0,0,0,0,2);
    Check(ActiveVoices(precedent)>=2,
        "PRECEDENT identity distribution governance and memory select mixer layers");
}

struct Runner {
    Runner() noexcept {
        TestOptions();
        TestSynthAndSilentFallback();
        TestEndingPoliciesAndRuntimeRouting();
        TestEncounterModifiers();
        if (gFailures!=0) {
            std::printf("T11_FAIL\tchecks=%u\tfailures=%u\n",gChecks,gFailures);
            std::exit(1);
        }
        std::printf("T11_PASS\tchecks=%u\tbuffers=4\tvoices=8\n",gChecks);
    }
};

Runner gRunner;

}  // namespace
