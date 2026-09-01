#include "bp_input.h"

namespace bp {
namespace {

std::uint32_t ActionIndex(PlatformAction action) noexcept {
    return static_cast<std::uint32_t>(action);
}

bool HeldOrPressed(const InputFrame& frame, PlatformAction action) noexcept {
    const ButtonState& button = frame.actions[ActionIndex(action)];
    return button.held != 0 || button.pressed != 0;
}

bool Pressed(const InputFrame& frame, PlatformAction action) noexcept {
    return frame.actions[ActionIndex(action)].pressed != 0;
}

}  // namespace

GameplayIntent ResolveGameplayInput(
    InputArbiter& arbiter,
    const InputFrame& frame,
    bool dashAvailable,
    bool hudReallocateAvailable) noexcept {
    if (arbiter.switchLockTicks > 0) {
        --arbiter.switchLockTicks;
    }
    GameplayIntent intent{GameplayAction::None, frame.moveX, frame.moveY, frame.focused == 0};
    if (intent.focusLost) {
        intent.action = GameplayAction::Pause;
        return intent;
    }
    if (Pressed(frame, PlatformAction::Pause)) {
        intent.action = GameplayAction::Pause;
        return intent;
    }
    if (hudReallocateAvailable && HeldOrPressed(frame, PlatformAction::HudReallocate)) {
        intent.action = GameplayAction::HudReallocate;
        return intent;
    }
    if (dashAvailable && Pressed(frame, PlatformAction::Dash)) {
        intent.action = GameplayAction::Dash;
        return intent;
    }

    const bool fire = HeldOrPressed(frame, PlatformAction::Fire);
    const bool reclaim = HeldOrPressed(frame, PlatformAction::Reclaim);
    const bool firePressed = Pressed(frame, PlatformAction::Fire);
    const bool reclaimPressed = Pressed(frame, PlatformAction::Reclaim);
    GameplayAction pressedAction = GameplayAction::None;
    if (firePressed || reclaimPressed) {
        pressedAction = firePressed ? GameplayAction::Fire : GameplayAction::Reclaim;
        arbiter.lastFireReclaim = pressedAction;
    }
    if (arbiter.switchLockTicks > 0 && arbiter.lockedFireReclaim != GameplayAction::None) {
        if (arbiter.lockedFireReclaim == GameplayAction::Fire && fire) {
            intent.action = GameplayAction::Fire;
        } else if (arbiter.lockedFireReclaim == GameplayAction::Reclaim && reclaim) {
            intent.action = GameplayAction::Reclaim;
        }
        return intent;
    }
    if (pressedAction != GameplayAction::None) {
        arbiter.lockedFireReclaim = pressedAction;
        arbiter.switchLockTicks = kActionSwitchLockTicks;
    }
    if (fire || reclaim) {
        GameplayAction selected = GameplayAction::Fire;
        if (fire && reclaim) {
            selected = arbiter.lastFireReclaim == GameplayAction::Reclaim
                ? GameplayAction::Reclaim
                : GameplayAction::Fire;
        } else {
            selected = fire ? GameplayAction::Fire : GameplayAction::Reclaim;
        }
        intent.action = selected;
        return intent;
    }
    if (frame.moveX != 0 || frame.moveY != 0) {
        intent.action = GameplayAction::Move;
    }
    return intent;
}

}  // namespace bp
