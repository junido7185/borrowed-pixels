#pragma once

#include "bp_platform.h"

#include <cstdint>

namespace bp {

inline constexpr std::uint8_t kActionSwitchLockTicks = 9;

enum class GameplayAction : std::uint8_t {
    None = 0,
    Pause,
    HudReallocate,
    Dash,
    Fire,
    Reclaim,
    Move,
};

struct InputArbiter {
    GameplayAction lastFireReclaim;
    GameplayAction lockedFireReclaim;
    std::uint8_t switchLockTicks;
};

struct GameplayIntent {
    GameplayAction action;
    std::int16_t moveX;
    std::int16_t moveY;
    bool focusLost;
};

GameplayIntent ResolveGameplayInput(
    InputArbiter& arbiter,
    const InputFrame& frame,
    bool dashAvailable,
    bool hudReallocateAvailable) noexcept;

}  // namespace bp
