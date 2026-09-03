#pragma once

#include "bp_platform.h"
#include "bp_render.h"

#include <cstdint>

namespace bp {

enum class OptionRow : std::uint8_t {
    Language=0,
    MasterVolume,
    ScreenShake,
    ReduceFlash,
    ColorAssist,
    Fullscreen,
    Count,
};

enum class ColorAssistMode : std::uint8_t { Off=0,Shapes,HighContrast };

struct OptionsRuntime {
    std::uint32_t value;
    std::uint32_t previousValue;
    OptionRow focused;
    std::uint8_t changed;
    std::uint8_t closeRequested;
};

void InitializeOptionsRuntime(OptionsRuntime& state,std::uint32_t options) noexcept;
bool TickOptionsRuntime(OptionsRuntime& state,const InputFrame& input) noexcept;
void RenderOptionsRuntime(PixelBuffer buffer,const OptionsRuntime& state) noexcept;
std::uint8_t MasterVolume(std::uint32_t options) noexcept;
bool EnglishLanguage(std::uint32_t options) noexcept;
bool ScreenShakeEnabled(std::uint32_t options) noexcept;
bool ReduceFlashEnabled(std::uint32_t options) noexcept;
ColorAssistMode ColorAssist(std::uint32_t options) noexcept;
bool BorderlessEnabled(std::uint32_t options) noexcept;
void ApplyAccessibilityPostprocess(PixelBuffer buffer,std::uint32_t options) noexcept;

}  // namespace bp
