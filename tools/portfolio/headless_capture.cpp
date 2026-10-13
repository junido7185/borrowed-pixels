// Headless gameplay capture for the README.
//
// Drives the real Act 1 runtime (and Act 2 if reached) (the same Tick*/Render* calls that
// src/bp_win32.cpp makes each 60 Hz tick) with a scripted input bot, and writes
// every Nth rendered 320x180 frame as a PPM. No window, audio or OS input is
// involved, so this builds with any C++20 compiler:
//
//   g++ -std=c++20 -O2 -Isrc tools/portfolio/headless_capture.cpp $(ls src/*.cpp | grep -v bp_win32) -o headless_capture
//   mkdir -p cap && ./headless_capture 2700 3 cap
//   python3 tools/portfolio/make_gifs.py cap docs/portfolio
//
// The bot is not a player model: it aims at the opponent, fires in fixed bursts,
// strafes vertically, and holds Confirm on every non-combat screen.
#include "bp_act2.h"
#include "bp_encounter.h"
#include "bp_platform.h"
#include "bp_render.h"

#include <cstdio>
#include <cstdlib>

using namespace bp;

namespace {

std::uint32_t gPixels[kRenderWidth * kRenderHeight];
Act1Runtime gAct1;
Act2Runtime gAct2;

bool WritePpm(const char* directory, int index) {
    char path[512];
    std::snprintf(path, sizeof(path), "%s/f%05d.ppm", directory, index);
    FILE* file = std::fopen(path, "wb");
    if (!file) return false;
    std::fprintf(file, "P6 %d %d 255\n", kRenderWidth, kRenderHeight);
    for (std::uint32_t color : gPixels) {
        const unsigned char rgb[3] = {
            static_cast<unsigned char>(color >> 16),
            static_cast<unsigned char>(color >> 8),
            static_cast<unsigned char>(color),
        };
        std::fwrite(rgb, 1, 3, file);
    }
    return std::fclose(file) == 0;
}

void Hold(InputFrame& input, PlatformAction action) { input.actions[static_cast<int>(action)].held = 1; }
void Press(InputFrame& input, PlatformAction action) {
    input.actions[static_cast<int>(action)].held = 1;
    input.actions[static_cast<int>(action)].pressed = 1;
}

InputFrame BotInput(int tick, bool playing, const CombatState& combat) {
    InputFrame input{};
    input.tick = static_cast<std::uint32_t>(tick);
    input.focused = 1;
    input.device = InputDevice::KeyboardMouse;
    if (playing) {
        // Mouse aim is relative to the canvas centre (see BuildInputFrame in bp_win32.cpp).
        const int dx = (combat.actors[1].position.x - combat.actors[0].position.x) >> 8;
        const int dy = (combat.actors[1].position.y - combat.actors[0].position.y) >> 8;
        input.mouseX = static_cast<std::uint16_t>(ClampInt(kCanvasWidth / 2 + dx, 0, kCanvasWidth - 1));
        input.mouseY = static_cast<std::uint16_t>(ClampInt(kCanvasHeight / 2 + dy, 0, kCanvasHeight - 1));
        input.aimX = static_cast<std::int16_t>(input.mouseX - kCanvasWidth / 2);
        input.aimY = static_cast<std::int16_t>(input.mouseY - kCanvasHeight / 2);
        input.moveY = (tick % 240) < 120 ? -20000 : 20000;
        if (tick % 90 < 45) Hold(input, PlatformAction::Fire);
        else if (tick % 90 == 45) input.actions[static_cast<int>(PlatformAction::Fire)].released = 1;
        if (tick % 400 > 330) Hold(input, PlatformAction::Reclaim);
    } else {
        const int phase = tick % 150;
        if (phase == 0) Press(input, PlatformAction::Confirm);
        else if (phase < 140) Hold(input, PlatformAction::Confirm);
        else if (phase == 140) input.actions[static_cast<int>(PlatformAction::Confirm)].released = 1;
        else if (phase == 145) Press(input, PlatformAction::SelectRight);
    }
    return input;
}

}  // namespace

int main(int argc, char** argv) {
    const int maxTicks = argc > 1 ? std::atoi(argv[1]) : 2700;
    const int every = argc > 2 ? std::atoi(argv[2]) : 3;
    const char* directory = argc > 3 ? argv[3] : "cap";
    if (!InitializeAct1Runtime(gAct1)) return 1;

    bool inAct2 = false;
    int frames = 0;
    for (int tick = 0; tick < maxTicks; ++tick) {
        const bool playing = inAct2 ? gAct2.scene == Act2Scene::Playing : gAct1.scene == Act1Scene::Playing;
        const InputFrame input = BotInput(tick, playing, inAct2 ? gAct2.combat : gAct1.combat);
        if (!(inAct2 ? TickAct2Runtime(gAct2, input) : TickAct1Runtime(gAct1, input))) {
            std::printf("tick failed at %d\n", tick);
            return 1;
        }
        if (!inAct2 && gAct1.scene == Act1Scene::ContinueReady) {
            if (!InitializeAct2Runtime(gAct2, gAct1.claimLedger, gAct1.narrative)) return 1;
            inAct2 = true;
        }
        if (tick % every == 0) {
            const PixelBuffer buffer{gPixels, kRenderWidth, kRenderHeight, kRenderWidth};
            if (inAct2) RenderAct2Runtime(buffer, gAct2);
            else RenderAct1Runtime(buffer, gAct1);
            if (!WritePpm(directory, frames++)) return 1;
        }
    }
    std::printf("frames=%d act=%d\n", frames, inAct2 ? 2 : 1);
    return 0;
}
