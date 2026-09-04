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

std::uint8_t ActiveVoices(const bp::AudioState& state) noexcept;
bp::AudioVoice FirstActive(const bp::AudioState& state) noexcept;

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

    bp::AudioState saturated{};
    bp::InitializeAudio(saturated,true);
    for (std::uint8_t index=0;index<bp::kAudioVoiceCount;++index)
        Check(bp::QueueAudioEvent(saturated,bp::AudioEvent::AmbientClick),
            "voice saturation fixture fills one low-priority slot");
    Check(bp::QueueAudioEvent(saturated,bp::AudioEvent::CoreHit),
        "priority8 CORE_HIT is accepted by a saturated mixer");
    std::uint8_t coreVoices=0;
    bool coreNoise=false;
    bool coreTriangle=false;
    for (const bp::AudioVoice& voice:saturated.voices) {
        if (voice.active!=0&&voice.priority==8u) {
            ++coreVoices;
            coreNoise=coreNoise||voice.oscillator==bp::Oscillator::Noise;
            coreTriangle=coreTriangle||voice.oscillator==bp::Oscillator::Triangle;
        }
    }
    Check(ActiveVoices(saturated)==bp::kAudioVoiceCount&&coreVoices==2
        &&coreNoise&&coreTriangle,
        "priority8 CORE_HIT replaces two lowest voices without dropping either layer");

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
    bp::AudioState delayedOne{};
    bp::InitializeAudio(delayedOne,true);
    Check(bp::QueueAudioEvent(delayedOne,bp::AudioEvent::Settlement),
        "ONE delayed-voice fixture schedules settlement notes");
    bp::SetAudioMixState(delayedOne,10,false,true,bp::AudioEnding::One);
    Check(delayedOne.voices[1].active==0&&delayedOne.voices[2].active==0,
        "ONE cancels not-yet-started notes so no new sound begins");
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
    bp::AudioState clearedCoreFlag{};
    bp::InitializeAudio(clearedCoreFlag,true);
    after=before;
    after.coreHitValid=1;
    after.coreHitTick=17;
    after.coreHitSide=1;
    bp::QueueRuntimeAudioTransitions(clearedCoreFlag,before,after,false);
    Check(ActiveVoices(clearedCoreFlag)==2,
        "combat settlement snapshot preserves CORE_HIT after encounter clears actor flag");

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

    bp::AudioState menuMove{};
    bp::InitializeAudio(menuMove,true);
    before=bp::AudioRuntimeSnapshot{};
    before.valid=1;
    before.menuKind=1;
    before.menuFocus=0;
    after=before;
    after.menuFocus=1;
    bp::QueueRuntimeAudioTransitions(menuMove,before,after,false);
    Check(ActiveVoices(menuMove)==1&&menuMove.voices[0].bus==bp::AudioBus::Menu
        &&menuMove.voices[0].priority==2,
        "actual focus change in one menu namespace queues MENU_MOVE");
    after.menuKind=2;
    bp::QueueRuntimeAudioTransitions(menuMove,before,after,false);
    Check(ActiveVoices(menuMove)==1,
        "opening another menu namespace does not invent a navigation sound");

    bp::AudioState distribution{};
    bp::InitializeAudio(distribution,true);
    before=bp::AudioRuntimeSnapshot{};
    before.valid=before.playing=1;
    before.route=3;
    before.encounter=bp::AudioEncounter::Index;
    before.playerShell=96;
    before.opponentShell=48;
    before.neutralMass=24;
    bp::QueueRuntimeAudioTransitions(distribution,before,before,false);
    Check(distribution.playerLayerGainQ15==32767
        &&distribution.opponentLayerGainQ15==16383
        &&distribution.neutralLayerGainQ15==8191,
        "PLAYER opponent and Neutral mass ratios set three exact normalized layers");
    distribution.step=0;
    distribution.stepAccumulator=bp::kAudioStepNumerator-bp::kAudioStepDenominator;
    bp::RenderAudioFrames(distribution,samples,1);
    bool playerLayer=false;
    bool neutralLayer=false;
    for (const bp::AudioVoice& voice:distribution.voices) {
        playerLayer=playerLayer||(voice.active!=0&&voice.bus==bp::AudioBus::Music
            &&voice.oscillator==bp::Oscillator::Triangle&&voice.velocityQ15==32767);
        neutralLayer=neutralLayer||(voice.active!=0&&voice.bus==bp::AudioBus::Music
            &&voice.oscillator==bp::Oscillator::Noise&&voice.velocityQ15==8191);
    }
    Check(playerLayer&&neutralLayer,
        "step scheduler applies PLAYER and Neutral ratios to base rhythm layers");
    bp::AudioState opponentLayerState{};
    bp::InitializeAudio(opponentLayerState,true);
    bp::QueueRuntimeAudioTransitions(opponentLayerState,before,before,false);
    opponentLayerState.step=2;
    opponentLayerState.stepAccumulator=bp::kAudioStepNumerator-bp::kAudioStepDenominator;
    bp::RenderAudioFrames(opponentLayerState,samples,1);
    Check(ActiveVoices(opponentLayerState)==1
        &&FirstActive(opponentLayerState).velocityQ15==16383,
        "step scheduler applies opponent ratio to the counter layer");

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

    bp::AudioState noise{};
    bp::InitializeAudio(noise,true);
    Check(bp::QueueAudioEvent(noise,bp::AudioEvent::CoverBreak),
        "frequency-clocked noise fixture queues");
    const std::uint32_t initialNoise=noise.voices[0].noiseState;
    bp::RenderAudioFrames(noise,samples,10);
    Check(noise.voices[0].noiseState==initialNoise,
        "XORSHIFT noise holds its sample before phase wrap");
    bp::RenderAudioFrames(noise,samples,20);
    Check(noise.voices[0].noiseState!=initialNoise,
        "XORSHIFT noise start-to-end frequency drives sample clock");
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

    std::uint16_t profiles=0;
    for (std::uint8_t identity=0;identity<3;++identity)
        for (std::uint8_t distribution=0;distribution<3;++distribution)
            for (std::uint8_t governance=0;governance<3;++governance)
                for (std::uint8_t memory=0;memory<3;++memory) {
                    bp::AudioState policy{};
                    bp::InitializeAudio(policy,true);
                    bp::AudioRuntimeSnapshot context{};
                    context.valid=context.playing=1;
                    context.route=4;
                    context.encounter=bp::AudioEncounter::Precedent;
                    context.contextA=identity;
                    context.contextB=distribution;
                    context.contextC=governance;
                    context.contextD=memory;
                    context.contextE=0;
                    context.contextF=1;
                    context.contextG=governance==2u?3u:0u;
                    context.echoMotif[0]=0;
                    context.echoMotif[1]=4;
                    context.echoMotif[2]=7;
                    bp::QueueRuntimeAudioTransitions(policy,context,context,false);
                    policy.step=2;
                    policy.stepAccumulator=bp::kAudioStepNumerator-bp::kAudioStepDenominator;
                    std::int16_t sample[2]{};
                    bp::RenderAudioFrames(policy,sample,1);
                    Check(ActiveVoices(policy)>0&&policy.contextA==identity
                        &&policy.contextB==distribution&&policy.contextC==governance
                        &&policy.contextD==memory,
                        "PRECEDENT audio context covers one of 81 policy profiles");
                    const bp::Oscillator expectedOscillator=identity==1u
                        ?bp::Oscillator::Triangle:bp::Oscillator::Pulse25;
                    const std::uint32_t expectedIncrement=distribution==2u
                        ?31248413u:19685267u;
                    const std::uint8_t expectedVoices=governance==2u?3u:2u;
                    Check(ActiveVoices(policy)==expectedVoices
                        &&policy.voices[0].oscillator==expectedOscillator
                        &&policy.voices[0].startIncrement==expectedIncrement,
                        "PRECEDENT profile applies exact identity distribution and governance result");
                    const std::uint16_t expectedLeft=distribution==1u?32767u:23197u;
                    const std::uint16_t expectedRight=distribution==2u?32767u:23197u;
                    Check(policy.voices[0].velocityQ15==expectedLeft
                        &&policy.voices[1].velocityQ15==expectedRight,
                        "PRECEDENT profile applies exact EQUALITY NEED or UTILITY channel gain");

                    bp::AudioState memoryLayer{};
                    bp::InitializeAudio(memoryLayer,true);
                    bp::QueueRuntimeAudioTransitions(memoryLayer,context,context,false);
                    memoryLayer.step=1;
                    memoryLayer.stepAccumulator=
                        bp::kAudioStepNumerator-bp::kAudioStepDenominator;
                    bp::RenderAudioFrames(memoryLayer,sample,1);
                    const std::uint8_t expectedMemoryVoices=memory==2u?0u:1u;
                    const std::uint32_t expectedMemoryIncrement=memory==1u
                        ?3284585u:6569170u;
                    Check(ActiveVoices(memoryLayer)==expectedMemoryVoices
                        &&(expectedMemoryVoices==0u
                            ||FirstActive(memoryLayer).startIncrement==expectedMemoryIncrement),
                        "PRECEDENT profile applies exact FULL SUMMARY or NONE memory layer");
                    ++profiles;
                }
    Check(profiles==81,"PRECEDENT audio checks all 81 policy profiles");

    bp::AudioState commonsPlayer{};
    bp::InitializeAudio(commonsPlayer,true);
    bp::AudioRuntimeSnapshot commons{};
    commons.valid=commons.playing=1;
    commons.route=4;
    commons.encounter=bp::AudioEncounter::Precedent;
    commons.contextC=2;
    commons.contextG=3;
    bp::QueueRuntimeAudioTransitions(commonsPlayer,commons,commons,false);
    commonsPlayer.step=2;
    commonsPlayer.stepAccumulator=bp::kAudioStepNumerator-bp::kAudioStepDenominator;
    std::int16_t commonsSample[2]{};
    bp::RenderAudioFrames(commonsPlayer,commonsSample,1);
    bool playerLeft=false;
    bool opponentRight=false;
    for (const bp::AudioVoice& voice:commonsPlayer.voices) {
        playerLeft=playerLeft||(voice.active!=0&&voice.panQ15<0);
        opponentRight=opponentRight||(voice.active!=0&&voice.panQ15>0);
    }
    Check(!playerLeft&&opponentRight,
        "COMMONS moves actual PLAYER-majority TAP channel to center only");

    bp::AudioState commonsOpponent{};
    bp::InitializeAudio(commonsOpponent,true);
    commons.contextG=5;
    bp::QueueRuntimeAudioTransitions(commonsOpponent,commons,commons,false);
    commonsOpponent.step=2;
    commonsOpponent.stepAccumulator=bp::kAudioStepNumerator-bp::kAudioStepDenominator;
    bp::RenderAudioFrames(commonsOpponent,commonsSample,1);
    playerLeft=false;
    opponentRight=false;
    for (const bp::AudioVoice& voice:commonsOpponent.voices) {
        playerLeft=playerLeft||(voice.active!=0&&voice.panQ15<0);
        opponentRight=opponentRight||(voice.active!=0&&voice.panQ15>0);
    }
    Check(playerLeft&&!opponentRight,
        "COMMONS moves actual OPPONENT-majority CLAIM channel to center only");

    bp::AudioState judgment{};
    bp::InitializeAudio(judgment,true);
    bp::AudioRuntimeSnapshot judgmentContext{};
    judgmentContext.valid=judgmentContext.playing=1;
    judgmentContext.route=4;
    judgmentContext.encounter=bp::AudioEncounter::Precedent;
    judgmentContext.contextC=1;
    bp::QueueRuntimeAudioTransitions(judgment,judgmentContext,judgmentContext,false);
    judgment.step=0;
    judgment.stepAccumulator=bp::kAudioStepNumerator-bp::kAudioStepDenominator;
    bp::RenderAudioFrames(judgment,commonsSample,1);
    judgment.contextG=3;
    judgment.step=1;
    judgment.stepAccumulator=bp::kAudioStepNumerator-bp::kAudioStepDenominator;
    bp::RenderAudioFrames(judgment,commonsSample,1);
    Check(judgment.judgmentRow==0,
        "JUDGMENT retains chosen row inside a four-step group");
    judgment.step=4;
    judgment.stepAccumulator=bp::kAudioStepNumerator-bp::kAudioStepDenominator;
    bp::RenderAudioFrames(judgment,commonsSample,1);
    Check(judgment.judgmentRow==3,
        "JUDGMENT selects the recent-action row only every four steps");
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
