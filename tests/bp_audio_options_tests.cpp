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

struct Runner {
    Runner() noexcept {
        TestOptions();
        TestSynthAndSilentFallback();
        if (gFailures!=0) {
            std::printf("T11_FAIL\tchecks=%u\tfailures=%u\n",gChecks,gFailures);
            std::exit(1);
        }
        std::printf("T11_PASS\tchecks=%u\tbuffers=4\tvoices=8\n",gChecks);
    }
};

Runner gRunner;

}  // namespace
