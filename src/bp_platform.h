#pragma once

#include <cstdint>

namespace bp {

inline constexpr std::int32_t kCanvasWidth = 320;
inline constexpr std::int32_t kCanvasHeight = 180;
inline constexpr std::int32_t kDefaultClientWidth = 1280;
inline constexpr std::int32_t kDefaultClientHeight = 720;
inline constexpr std::int32_t kMinimumClientWidth = 640;
inline constexpr std::int32_t kMinimumClientHeight = 360;
inline constexpr std::uint32_t kSimulationHz = 60;
inline constexpr std::uint32_t kBackgroundRenderHz = 15;
inline constexpr std::uint32_t kMaxCatchUpTicks = 5;

enum class InputDevice : std::uint8_t {
    KeyboardMouse = 0,
    Gamepad = 1,
};

enum class PlatformAction : std::uint8_t {
    MoveUp = 0,
    MoveDown,
    MoveLeft,
    MoveRight,
    Fire,
    Reclaim,
    Dash,
    HudCycle,
    HudReallocate,
    SelectLeft,
    SelectUp,
    SelectRight,
    Confirm,
    Pause,
    Count,
};

inline constexpr std::uint32_t kPlatformActionCount =
    static_cast<std::uint32_t>(PlatformAction::Count);

struct ButtonState {
    std::uint8_t held;
    std::uint8_t pressed;
    std::uint8_t released;
    std::uint8_t reserved;
};

struct InputFrame {
    std::uint32_t tick;
    std::int16_t moveX;
    std::int16_t moveY;
    std::int16_t aimX;
    std::int16_t aimY;
    std::uint16_t mouseX;
    std::uint16_t mouseY;
    std::uint8_t focused;
    InputDevice device;
    ButtonState actions[kPlatformActionCount];
};

struct Viewport {
    std::int32_t x;
    std::int32_t y;
    std::int32_t width;
    std::int32_t height;
    std::int32_t scale;
};

constexpr std::int32_t ClampInt(
    std::int32_t value,
    std::int32_t minimum,
    std::int32_t maximum) noexcept {
    return value < minimum ? minimum : (value > maximum ? maximum : value);
}

constexpr Viewport ComputeViewport(std::int32_t clientWidth, std::int32_t clientHeight) noexcept {
    const std::int32_t safeWidth = clientWidth > 0 ? clientWidth : 1;
    const std::int32_t safeHeight = clientHeight > 0 ? clientHeight : 1;
    const std::int32_t scaleX = safeWidth / kCanvasWidth;
    const std::int32_t scaleY = safeHeight / kCanvasHeight;
    std::int32_t scale = scaleX < scaleY ? scaleX : scaleY;
    if (scale < 1) {
        scale = 1;
    }

    const std::int32_t width = kCanvasWidth * scale;
    const std::int32_t height = kCanvasHeight * scale;
    return Viewport{
        (safeWidth - width) / 2,
        (safeHeight - height) / 2,
        width,
        height,
        scale,
    };
}

constexpr void ClientToCanvas(
    const Viewport& viewport,
    std::int32_t clientX,
    std::int32_t clientY,
    std::uint16_t& canvasX,
    std::uint16_t& canvasY) noexcept {
    const std::int32_t localX = (clientX - viewport.x) / viewport.scale;
    const std::int32_t localY = (clientY - viewport.y) / viewport.scale;
    canvasX = static_cast<std::uint16_t>(ClampInt(localX, 0, kCanvasWidth - 1));
    canvasY = static_cast<std::uint16_t>(ClampInt(localY, 0, kCanvasHeight - 1));
}

static_assert(kCanvasWidth == 320 && kCanvasHeight == 180);
static_assert(kSimulationHz == 60 && kMaxCatchUpTicks == 5);
static_assert(kPlatformActionCount == 14);

}  // namespace bp
