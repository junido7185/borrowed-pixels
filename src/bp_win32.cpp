#define _WIN32_WINNT 0x0A00

#include "bp_platform.h"
#include "bp_render.h"
#include "bp_encounter.h"
#include "bp_act2.h"
#include "bp_act3.h"
#include "bp_precedent.h"
#include "bp_narrative.h"
#include "bp_save.h"
#include "bp_font.h"
#include "bp_audio.h"
#include "bp_options.h"
#include "bp_strings.h"

#include <windows.h>
#include <windowsx.h>
#include <mmsystem.h>
#include <xinput.h>

#if defined(BP_DEBUG)
#include <cwchar>
#endif

#pragma comment(lib, "winmm.lib")

namespace {

using XInputGetStateFn = DWORD(WINAPI*)(DWORD, XINPUT_STATE*);

constexpr wchar_t kWindowClassName[] = L"BorrowedPixelsWindow_v2_1";
constexpr wchar_t kWindowTitle[] = L"BORROWED PIXELS v2.1";
constexpr std::int32_t kGamepadLeftDeadzone = XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE;
constexpr std::int32_t kGamepadRightDeadzone = 7864;
constexpr std::int32_t kGamepadAimRetainThreshold = 6553;

struct Backbuffer {
    BITMAPINFO info;
    std::uint32_t* pixels;
};

struct PendingInput {
    std::uint8_t keyboardHeld[bp::kPlatformActionCount];
    std::uint8_t gamepadHeld[bp::kPlatformActionCount];
    std::uint8_t pressed[bp::kPlatformActionCount];
    std::uint8_t released[bp::kPlatformActionCount];
    std::uint16_t mouseX;
    std::uint16_t mouseY;
    std::int16_t gamepadMoveX;
    std::int16_t gamepadMoveY;
    std::int16_t gamepadAimX;
    std::int16_t gamepadAimY;
    bp::InputDevice lastDevice;
    DWORD lastGamepadPacket;
};

struct PlatformState {
    HWND window;
    HMODULE xinputModule;
    XInputGetStateFn xinputGetState;
    HANDLE frameTimer;
    LARGE_INTEGER qpcFrequency;
    std::int64_t lastCounter;
    std::int64_t accumulator60;
    WINDOWPLACEMENT windowedPlacement;
    DWORD windowedStyle;
    DWORD windowedExStyle;
    Backbuffer backbuffer;
    PendingInput input;
    bp::InputFrame lastInputFrame;
    bp::Act1Runtime act1;
    bp::Act2Runtime act2;
    bp::Act3Runtime act3;
    bp::PrecedentRuntime precedent;
    bp::NarrativeRuntime narrativeFlow;
    bp::SaveBoundaryObserver saveObserver;
    bp::SaveData persistentSave;
    bp::AudioState audio;
    bp::OptionsRuntime optionsRuntime;
    HWAVEOUT waveOut;
    WAVEHDR audioHeaders[bp::kAudioBufferCount];
    std::int16_t audioSamples[bp::kAudioBufferCount]
        [bp::kAudioBufferFrames*bp::kAudioChannels];
    bool act2Active;
    bool act3Active;
    bool precedentActive;
    bool narrativeActive;
    bool optionsActive;
    bool savePathReady;
    wchar_t saveDirectory[MAX_PATH];
    wchar_t savePath[MAX_PATH];
    wchar_t saveTempPath[MAX_PATH];
    wchar_t saveInvalidPath[MAX_PATH];
    std::uint32_t options;
    std::uint32_t runSeed;
    std::uint32_t bestFullRunTicks;
    std::uint16_t saveWarningTicks;
    std::uint8_t tutorial;
    std::uint32_t globalTick;
    std::uint32_t renderFrame;
    bool running;
    bool focused;
    bool fullscreen;
    bool highResolutionTimer;
    bool fallbackTimerPeriod;
    bool dpiAware;
    bool audioAvailable;
    bool xinputAvailable;
#if defined(BP_DEBUG)
    bool smokeTest;
    std::uint32_t smokePausedTick;
    int smokeTestResult;
#endif
};

PlatformState* gPlatform;

std::uint32_t ActionIndex(bp::PlatformAction action) noexcept {
    return static_cast<std::uint32_t>(action);
}

bool CombinedHeld(const PendingInput& input, std::uint32_t index) noexcept {
    return input.keyboardHeld[index] != 0 || input.gamepadHeld[index] != 0;
}

void SetKeyboardAction(PendingInput& input, bp::PlatformAction action, bool held) noexcept {
    const std::uint32_t index = ActionIndex(action);
    const bool wasHeld = CombinedHeld(input, index);
    input.keyboardHeld[index] = held ? 1u : 0u;
    const bool isHeld = CombinedHeld(input, index);
    if (!wasHeld && isHeld) {
        input.pressed[index] = 1;
        input.lastDevice = bp::InputDevice::KeyboardMouse;
    } else if (wasHeld && !isHeld) {
        input.released[index] = 1;
    }
}

void SetGamepadAction(PendingInput& input, bp::PlatformAction action, bool held) noexcept {
    const std::uint32_t index = ActionIndex(action);
    const bool wasHeld = CombinedHeld(input, index);
    input.gamepadHeld[index] = held ? 1u : 0u;
    const bool isHeld = CombinedHeld(input, index);
    if (!wasHeld && isHeld) {
        input.pressed[index] = 1;
        input.lastDevice = bp::InputDevice::Gamepad;
    } else if (wasHeld && !isHeld) {
        input.released[index] = 1;
    }
}

void ClearInputForFocusChange(PendingInput& input) noexcept {
    ZeroMemory(input.keyboardHeld, sizeof(input.keyboardHeld));
    ZeroMemory(input.gamepadHeld, sizeof(input.gamepadHeld));
    ZeroMemory(input.pressed, sizeof(input.pressed));
    ZeroMemory(input.released, sizeof(input.released));
    input.gamepadMoveX = 0;
    input.gamepadMoveY = 0;
}

void SetFocused(PlatformState& state, bool focused) noexcept {
    state.focused = focused;
    state.accumulator60 = 0;
    ClearInputForFocusChange(state.input);
}

bool MapVirtualKeyToAction(UINT virtualKey, bp::PlatformAction& action) noexcept {
    switch (virtualKey) {
    case 'W': action = bp::PlatformAction::MoveUp; return true;
    case 'S': action = bp::PlatformAction::MoveDown; return true;
    case 'A': action = bp::PlatformAction::MoveLeft; return true;
    case 'D': action = bp::PlatformAction::MoveRight; return true;
    case VK_LBUTTON: action = bp::PlatformAction::Fire; return true;
    case VK_RBUTTON: action = bp::PlatformAction::Reclaim; return true;
    case VK_SPACE: action = bp::PlatformAction::Dash; return true;
    case 'Q': action = bp::PlatformAction::HudCycle; return true;
    case 'E': action = bp::PlatformAction::HudReallocate; return true;
    case '1': action = bp::PlatformAction::SelectLeft; return true;
    case '2': action = bp::PlatformAction::SelectUp; return true;
    case '3': action = bp::PlatformAction::SelectRight; return true;
    case VK_RETURN: action = bp::PlatformAction::Confirm; return true;
    case VK_ESCAPE: action = bp::PlatformAction::Pause; return true;
    case VK_LEFT: action = bp::PlatformAction::SelectLeft; return true;
    case VK_UP: action = bp::PlatformAction::SelectUp; return true;
    case VK_RIGHT: action = bp::PlatformAction::SelectRight; return true;
    default: return false;
    }
}

void HandleKeyboardMessage(PlatformState& state, UINT message, WPARAM wParam, LPARAM lParam) noexcept {
    const bool isDown = message == WM_KEYDOWN || message == WM_SYSKEYDOWN;
    const bool wasDown = (lParam & (1ll << 30)) != 0;
    if (isDown && wasDown) {
        return;
    }

    bp::PlatformAction action{};
    if (MapVirtualKeyToAction(static_cast<UINT>(wParam), action)) {
        SetKeyboardAction(state.input, action, isDown);
    }
}

void UpdateMousePosition(PlatformState& state, std::int32_t clientX, std::int32_t clientY) noexcept {
    RECT client{};
    GetClientRect(state.window, &client);
    const bp::Viewport viewport = bp::ComputeViewport(client.right - client.left, client.bottom - client.top);
    std::uint16_t canvasX = 0;
    std::uint16_t canvasY = 0;
    bp::ClientToCanvas(viewport, clientX, clientY, canvasX, canvasY);
    if (canvasX != state.input.mouseX || canvasY != state.input.mouseY) {
        state.input.lastDevice = bp::InputDevice::KeyboardMouse;
    }
    state.input.mouseX = canvasX;
    state.input.mouseY = canvasY;
}

void PresentBackbuffer(PlatformState& state, HDC destination, const RECT& client) noexcept {
    bp::SetTextLanguageEnglish(bp::EnglishLanguage(state.options));
    const bp::PixelBuffer buffer{
            state.backbuffer.pixels,
            bp::kCanvasWidth,
            bp::kCanvasHeight,
            bp::kCanvasWidth,
        };
    if (state.optionsActive) bp::RenderOptionsRuntime(buffer,state.optionsRuntime);
    else if (state.narrativeActive) bp::RenderNarrativeRuntime(buffer,state.narrativeFlow);
    else if (state.precedentActive) bp::RenderPrecedentRuntime(buffer, state.precedent);
    else if (state.act3Active) bp::RenderAct3Runtime(buffer, state.act3);
    else if (state.act2Active) bp::RenderAct2Runtime(buffer, state.act2);
    else bp::RenderAct1Runtime(buffer, state.act1);
    if (state.saveWarningTicks!=0)
        (void)bp::DrawLatin(buffer,210,168,"SAVE NOT WRITTEN",bp::PaletteColor::Warning);
    bp::ApplyAccessibilityPostprocess(buffer,state.options);
    const std::int32_t clientWidth = client.right - client.left;
    const std::int32_t clientHeight = client.bottom - client.top;
    const bp::Viewport viewport = bp::ComputeViewport(clientWidth, clientHeight);

    SetDCBrushColor(destination, RGB(7, 10, 14));
    FillRect(destination, &client, static_cast<HBRUSH>(GetStockObject(DC_BRUSH)));
    SetStretchBltMode(destination, COLORONCOLOR);
    StretchDIBits(
        destination,
        viewport.x,
        viewport.y,
        viewport.width,
        viewport.height,
        0,
        0,
        bp::kCanvasWidth,
        bp::kCanvasHeight,
        state.backbuffer.pixels,
        &state.backbuffer.info,
        DIB_RGB_COLORS,
        SRCCOPY);
}

void PresentWindow(PlatformState& state) noexcept {
    RECT client{};
    GetClientRect(state.window, &client);
    HDC dc = GetDC(state.window);
    if (dc) {
        PresentBackbuffer(state, dc, client);
        ReleaseDC(state.window, dc);
    }
}

void ToggleBorderlessFullscreen(PlatformState& state) noexcept {
    if (!state.fullscreen) {
        state.windowedStyle = static_cast<DWORD>(GetWindowLongPtrW(state.window, GWL_STYLE));
        state.windowedExStyle = static_cast<DWORD>(GetWindowLongPtrW(state.window, GWL_EXSTYLE));
        state.windowedPlacement.length = sizeof(state.windowedPlacement);
        GetWindowPlacement(state.window, &state.windowedPlacement);

        MONITORINFO monitorInfo{sizeof(monitorInfo)};
        GetMonitorInfoW(MonitorFromWindow(state.window, MONITOR_DEFAULTTONEAREST), &monitorInfo);
        SetWindowLongPtrW(state.window, GWL_STYLE, WS_POPUP | WS_VISIBLE);
        SetWindowLongPtrW(state.window, GWL_EXSTYLE, 0);
        SetWindowPos(
            state.window,
            HWND_TOP,
            monitorInfo.rcMonitor.left,
            monitorInfo.rcMonitor.top,
            monitorInfo.rcMonitor.right - monitorInfo.rcMonitor.left,
            monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top,
            SWP_FRAMECHANGED | SWP_NOOWNERZORDER);
        state.fullscreen = true;
        return;
    }

    SetWindowLongPtrW(state.window, GWL_STYLE, state.windowedStyle);
    SetWindowLongPtrW(state.window, GWL_EXSTYLE, state.windowedExStyle);
    SetWindowPlacement(state.window, &state.windowedPlacement);
    SetWindowPos(
        state.window,
        nullptr,
        0,
        0,
        0,
        0,
        SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER);
    state.fullscreen = false;
}

LRESULT CALLBACK WindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
    PlatformState* state = reinterpret_cast<PlatformState*>(GetWindowLongPtrW(window, GWLP_USERDATA));
    if (message == WM_NCCREATE) {
        const CREATESTRUCTW* create = reinterpret_cast<const CREATESTRUCTW*>(lParam);
        state = static_cast<PlatformState*>(create->lpCreateParams);
        SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(state));
        state->window = window;
    }

    if (!state) {
        return DefWindowProcW(window, message, wParam, lParam);
    }

    switch (message) {
    case WM_CLOSE:
        DestroyWindow(window);
        return 0;
    case WM_DESTROY:
        state->running = false;
        PostQuitMessage(0);
        return 0;
    case WM_ERASEBKGND:
        return 1;
    case WM_PAINT: {
        PAINTSTRUCT paint{};
        HDC dc = BeginPaint(window, &paint);
        RECT client{};
        GetClientRect(window, &client);
        PresentBackbuffer(*state, dc, client);
        EndPaint(window, &paint);
        return 0;
    }
    case WM_GETMINMAXINFO: {
        if (state->fullscreen) {
            return 0;
        }
        RECT minimum{0, 0, bp::kMinimumClientWidth, bp::kMinimumClientHeight};
        const DWORD style = static_cast<DWORD>(GetWindowLongPtrW(window, GWL_STYLE));
        const DWORD exStyle = static_cast<DWORD>(GetWindowLongPtrW(window, GWL_EXSTYLE));
        const UINT dpi = GetDpiForWindow(window);
        if (!AdjustWindowRectExForDpi(&minimum, style, FALSE, exStyle, dpi)) {
            AdjustWindowRectEx(&minimum, style, FALSE, exStyle);
        }
        MINMAXINFO* minMax = reinterpret_cast<MINMAXINFO*>(lParam);
        minMax->ptMinTrackSize.x = minimum.right - minimum.left;
        minMax->ptMinTrackSize.y = minimum.bottom - minimum.top;
        return 0;
    }
    case WM_DPICHANGED: {
        const RECT* suggested = reinterpret_cast<const RECT*>(lParam);
        SetWindowPos(
            window,
            nullptr,
            suggested->left,
            suggested->top,
            suggested->right - suggested->left,
            suggested->bottom - suggested->top,
            SWP_NOACTIVATE | SWP_NOZORDER);
        return 0;
    }
    case WM_KILLFOCUS:
        SetFocused(*state, false);
        return 0;
    case WM_SETFOCUS:
        SetFocused(*state, true);
        return 0;
    case WM_ACTIVATEAPP:
        SetFocused(*state, wParam != FALSE);
        return 0;
    case WM_KEYDOWN:
    case WM_KEYUP:
        HandleKeyboardMessage(*state, message, wParam, lParam);
        return 0;
    case WM_SYSKEYDOWN:
        if (wParam == VK_RETURN && (lParam & (1ll << 29)) != 0 && (lParam & (1ll << 30)) == 0) {
            ToggleBorderlessFullscreen(*state);
            return 0;
        }
        HandleKeyboardMessage(*state, message, wParam, lParam);
        break;
    case WM_SYSKEYUP:
        HandleKeyboardMessage(*state, message, wParam, lParam);
        break;
    case WM_MOUSEMOVE:
        UpdateMousePosition(*state, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        return 0;
    case WM_LBUTTONDOWN:
        SetCapture(window);
        SetKeyboardAction(state->input, bp::PlatformAction::Fire, true);
        SetKeyboardAction(state->input, bp::PlatformAction::Confirm, true);
        return 0;
    case WM_LBUTTONUP:
        ReleaseCapture();
        SetKeyboardAction(state->input, bp::PlatformAction::Fire, false);
        SetKeyboardAction(state->input, bp::PlatformAction::Confirm, false);
        return 0;
    case WM_RBUTTONDOWN:
        SetCapture(window);
        SetKeyboardAction(state->input, bp::PlatformAction::Reclaim, true);
        return 0;
    case WM_RBUTTONUP:
        ReleaseCapture();
        SetKeyboardAction(state->input, bp::PlatformAction::Reclaim, false);
        return 0;
    default:
        break;
    }

    return DefWindowProcW(window, message, wParam, lParam);
}

bool InitializeBackbuffer(Backbuffer& backbuffer) noexcept {
    ZeroMemory(&backbuffer.info, sizeof(backbuffer.info));
    backbuffer.info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    backbuffer.info.bmiHeader.biWidth = bp::kCanvasWidth;
    backbuffer.info.bmiHeader.biHeight = -bp::kCanvasHeight;
    backbuffer.info.bmiHeader.biPlanes = 1;
    backbuffer.info.bmiHeader.biBitCount = 32;
    backbuffer.info.bmiHeader.biCompression = BI_RGB;
    const SIZE_T bytes = static_cast<SIZE_T>(bp::kCanvasWidth) * bp::kCanvasHeight * sizeof(std::uint32_t);
    backbuffer.pixels = static_cast<std::uint32_t*>(VirtualAlloc(nullptr, bytes, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE));
    return backbuffer.pixels != nullptr;
}

bool InitializeDpiAwareness() noexcept {
    if (SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2)) {
        return true;
    }
    return SetProcessDPIAware() != FALSE;
}

bool InitializeWaveOut(PlatformState& state) noexcept {
    WAVEFORMATEX format{};
    format.wFormatTag = WAVE_FORMAT_PCM;
    format.nChannels = 2;
    format.nSamplesPerSec = 48000;
    format.wBitsPerSample = 16;
    format.nBlockAlign = static_cast<WORD>((format.nChannels * format.wBitsPerSample) / 8);
    format.nAvgBytesPerSec = format.nSamplesPerSec * format.nBlockAlign;

    if (waveOutOpen(&state.waveOut,WAVE_MAPPER,&format,0,0,CALLBACK_NULL)
        !=MMSYSERR_NOERROR) return false;
    bp::InitializeAudio(state.audio,true);
    for (std::uint8_t index=0;index<bp::kAudioBufferCount;++index) {
        WAVEHDR& header=state.audioHeaders[index];
        ZeroMemory(&header,sizeof(header));
        header.lpData=reinterpret_cast<LPSTR>(state.audioSamples[index]);
        header.dwBufferLength=sizeof(state.audioSamples[index]);
        bp::RenderAudioFrames(state.audio,state.audioSamples[index],bp::kAudioBufferFrames);
        if (waveOutPrepareHeader(state.waveOut,&header,sizeof(header))!=MMSYSERR_NOERROR
            || waveOutWrite(state.waveOut,&header,sizeof(header))!=MMSYSERR_NOERROR) {
            waveOutReset(state.waveOut);
            for (std::uint8_t prepared=0;prepared<=index;++prepared)
                if ((state.audioHeaders[prepared].dwFlags&WHDR_PREPARED)!=0)
                    waveOutUnprepareHeader(state.waveOut,&state.audioHeaders[prepared],
                        sizeof(WAVEHDR));
            waveOutClose(state.waveOut);
            state.waveOut=nullptr;
            bp::InitializeAudio(state.audio,false);
            return false;
        }
    }
    return true;
}

bp::AudioEnding CurrentAudioEnding(const PlatformState& state) noexcept {
    if (!state.narrativeActive||state.narrativeFlow.scene!=bp::NarrativeScene::Ending)
        return bp::AudioEnding::None;
    switch (state.narrativeFlow.ending) {
    case bp::EndingId::Enough: return bp::AudioEnding::Enough;
    case bp::EndingId::Rule: return bp::AudioEnding::Rule;
    case bp::EndingId::Hand: return bp::AudioEnding::Hand;
    case bp::EndingId::OpenLedger: return bp::AudioEnding::Open;
    case bp::EndingId::One: return bp::AudioEnding::One;
    }
    return bp::AudioEnding::None;
}

void ServiceWaveOut(PlatformState& state) noexcept {
    bp::SetAudioMixState(state.audio,bp::MasterVolume(state.options),
        state.optionsActive,state.focused,CurrentAudioEnding(state));
    if (!state.waveOut) return;
    for (std::uint8_t index=0;index<bp::kAudioBufferCount;++index) {
        WAVEHDR& header=state.audioHeaders[index];
        if ((header.dwFlags&WHDR_DONE)==0) continue;
        bp::RenderAudioFrames(state.audio,state.audioSamples[index],bp::kAudioBufferFrames);
        header.dwFlags&=~WHDR_DONE;
        if (waveOutWrite(state.waveOut,&header,sizeof(header))!=MMSYSERR_NOERROR) {
            state.audioAvailable=false;
            bp::InitializeAudio(state.audio,false);
            return;
        }
    }
}

void InitializeXInput(PlatformState& state) noexcept {
    state.xinputModule = LoadLibraryW(L"xinput1_4.dll");
    if (!state.xinputModule) {
        state.xinputAvailable = false;
        return;
    }
    state.xinputGetState = reinterpret_cast<XInputGetStateFn>(GetProcAddress(state.xinputModule, "XInputGetState"));
    state.xinputAvailable = state.xinputGetState != nullptr;
    if (!state.xinputAvailable) {
        FreeLibrary(state.xinputModule);
        state.xinputModule = nullptr;
    }
}

bool OutsideRadialDeadzone(SHORT x, SHORT y, std::int32_t threshold) noexcept {
    const std::int64_t x64 = x;
    const std::int64_t y64 = y;
    const std::int64_t threshold64 = threshold;
    return x64 * x64 + y64 * y64 >= threshold64 * threshold64;
}

std::int16_t InvertStickAxis(std::int16_t value) noexcept {
    return value == static_cast<std::int16_t>(-32768)
        ? static_cast<std::int16_t>(32767)
        : static_cast<std::int16_t>(-value);
}

void PollXInput(PlatformState& state) noexcept {
    std::uint8_t next[bp::kPlatformActionCount]{};
    XINPUT_STATE gamepadState{};
    const bool connected = state.xinputGetState && state.xinputGetState(0, &gamepadState) == ERROR_SUCCESS;
    if (connected) {
        const XINPUT_GAMEPAD& pad = gamepadState.Gamepad;
        next[ActionIndex(bp::PlatformAction::Fire)] = pad.bRightTrigger > XINPUT_GAMEPAD_TRIGGER_THRESHOLD;
        next[ActionIndex(bp::PlatformAction::Reclaim)] = pad.bLeftTrigger > XINPUT_GAMEPAD_TRIGGER_THRESHOLD;
        next[ActionIndex(bp::PlatformAction::Dash)] = (pad.wButtons & XINPUT_GAMEPAD_A) != 0;
        next[ActionIndex(bp::PlatformAction::Confirm)] = (pad.wButtons & XINPUT_GAMEPAD_A) != 0;
        next[ActionIndex(bp::PlatformAction::HudCycle)] = (pad.wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER) != 0;
        next[ActionIndex(bp::PlatformAction::HudReallocate)] = (pad.wButtons & XINPUT_GAMEPAD_Y) != 0;
        next[ActionIndex(bp::PlatformAction::Pause)] = (pad.wButtons & XINPUT_GAMEPAD_START) != 0;
        next[ActionIndex(bp::PlatformAction::SelectLeft)] = (pad.wButtons & XINPUT_GAMEPAD_DPAD_LEFT) != 0;
        next[ActionIndex(bp::PlatformAction::SelectUp)] = (pad.wButtons & XINPUT_GAMEPAD_DPAD_UP) != 0;
        next[ActionIndex(bp::PlatformAction::SelectRight)] = (pad.wButtons & XINPUT_GAMEPAD_DPAD_RIGHT) != 0;

        if (OutsideRadialDeadzone(pad.sThumbLX, pad.sThumbLY, kGamepadLeftDeadzone)) {
            state.input.gamepadMoveX = pad.sThumbLX;
            state.input.gamepadMoveY = pad.sThumbLY;
        } else {
            state.input.gamepadMoveX = 0;
            state.input.gamepadMoveY = 0;
        }

        if (OutsideRadialDeadzone(pad.sThumbRX, pad.sThumbRY, kGamepadRightDeadzone)
            && OutsideRadialDeadzone(pad.sThumbRX, pad.sThumbRY, kGamepadAimRetainThreshold)) {
            state.input.gamepadAimX = pad.sThumbRX;
            state.input.gamepadAimY = pad.sThumbRY;
        }

        if (gamepadState.dwPacketNumber != state.input.lastGamepadPacket &&
            (state.input.gamepadMoveX != 0 || state.input.gamepadMoveY != 0 ||
             state.input.gamepadAimX != 0 || state.input.gamepadAimY != 0)) {
            state.input.lastDevice = bp::InputDevice::Gamepad;
        }
        state.input.lastGamepadPacket = gamepadState.dwPacketNumber;
    } else {
        state.input.gamepadMoveX = 0;
        state.input.gamepadMoveY = 0;
    }

    for (std::uint32_t index = 0; index < bp::kPlatformActionCount; ++index) {
        SetGamepadAction(state.input, static_cast<bp::PlatformAction>(index), next[index] != 0);
    }
}

void BuildInputFrame(PlatformState& state) noexcept {
    bp::InputFrame frame{};
    frame.tick = state.globalTick;
    frame.mouseX = state.input.mouseX;
    frame.mouseY = state.input.mouseY;
    frame.focused = state.focused ? 1u : 0u;
    frame.device = state.input.lastDevice;

    const bool keyboardHorizontal =
        state.input.keyboardHeld[ActionIndex(bp::PlatformAction::MoveLeft)] != 0 ||
        state.input.keyboardHeld[ActionIndex(bp::PlatformAction::MoveRight)] != 0;
    const bool keyboardVertical =
        state.input.keyboardHeld[ActionIndex(bp::PlatformAction::MoveUp)] != 0 ||
        state.input.keyboardHeld[ActionIndex(bp::PlatformAction::MoveDown)] != 0;
    frame.moveX = keyboardHorizontal
        ? static_cast<std::int16_t>(
              (state.input.keyboardHeld[ActionIndex(bp::PlatformAction::MoveRight)] ? 32767 : 0) -
              (state.input.keyboardHeld[ActionIndex(bp::PlatformAction::MoveLeft)] ? 32767 : 0))
        : state.input.gamepadMoveX;
    frame.moveY = keyboardVertical
        ? static_cast<std::int16_t>(
              (state.input.keyboardHeld[ActionIndex(bp::PlatformAction::MoveDown)] ? 32767 : 0) -
              (state.input.keyboardHeld[ActionIndex(bp::PlatformAction::MoveUp)] ? 32767 : 0))
        : InvertStickAxis(state.input.gamepadMoveY);

    if (frame.device == bp::InputDevice::Gamepad) {
        frame.aimX = state.input.gamepadAimX;
        frame.aimY = InvertStickAxis(state.input.gamepadAimY);
    } else {
        frame.aimX = static_cast<std::int16_t>(static_cast<std::int32_t>(frame.mouseX) - bp::kCanvasWidth / 2);
        frame.aimY = static_cast<std::int16_t>(static_cast<std::int32_t>(frame.mouseY) - bp::kCanvasHeight / 2);
    }

    for (std::uint32_t index = 0; index < bp::kPlatformActionCount; ++index) {
        frame.actions[index].held = CombinedHeld(state.input, index) ? 1u : 0u;
        frame.actions[index].pressed = state.input.pressed[index];
        frame.actions[index].released = state.input.released[index];
        state.input.pressed[index] = 0;
        state.input.released[index] = 0;
    }

    state.lastInputFrame = frame;
    ++state.globalTick;
}

std::int64_t QueryCounter() noexcept {
    LARGE_INTEGER counter{};
    QueryPerformanceCounter(&counter);
    return counter.QuadPart;
}

void EnableFallbackTimerPeriod(PlatformState& state) noexcept {
    if (!state.fallbackTimerPeriod && timeBeginPeriod(1) == TIMERR_NOERROR) {
        state.fallbackTimerPeriod = true;
    }
}

void WaitForFrame(PlatformState& state, std::int64_t frameStart, std::uint32_t frameRate) noexcept {
    const std::int64_t deadline = frameStart + state.qpcFrequency.QuadPart / frameRate;
    std::int64_t now = QueryCounter();
    std::int64_t remaining = deadline - now;
    if (remaining <= 0) {
        return;
    }

    if (state.highResolutionTimer && state.frameTimer) {
        const std::int64_t hundredNanoseconds = (remaining * 10000000ll) / state.qpcFrequency.QuadPart;
        if (hundredNanoseconds > 2500) {
            LARGE_INTEGER due{};
            due.QuadPart = -(hundredNanoseconds - 2500);
            if (SetWaitableTimer(state.frameTimer, &due, 0, nullptr, nullptr, FALSE)) {
                WaitForSingleObject(state.frameTimer, INFINITE);
            } else {
                state.highResolutionTimer = false;
                EnableFallbackTimerPeriod(state);
            }
        }
    }

    if (!state.highResolutionTimer) {
        EnableFallbackTimerPeriod(state);
        now = QueryCounter();
        remaining = deadline - now;
        const std::int64_t halfMillisecond = state.qpcFrequency.QuadPart / 2000;
        if (remaining > halfMillisecond) {
            const DWORD sleepMilliseconds = static_cast<DWORD>((remaining * 1000) / state.qpcFrequency.QuadPart);
            if (sleepMilliseconds > 1) {
                Sleep(sleepMilliseconds - 1);
            }
        }
    }

    while (QueryCounter() < deadline) {
        YieldProcessor();
    }
}

bool InitializeFrameTimer(PlatformState& state) noexcept {
    state.frameTimer = CreateWaitableTimerExW(
        nullptr,
        nullptr,
        CREATE_WAITABLE_TIMER_HIGH_RESOLUTION,
        TIMER_ALL_ACCESS);
    state.highResolutionTimer = state.frameTimer != nullptr;
    if (!state.highResolutionTimer) {
        EnableFallbackTimerPeriod(state);
    }
    return true;
}

bool CreateMainWindow(PlatformState& state, HINSTANCE instance, int showCommand) noexcept {
    WNDCLASSEXW windowClass{};
    windowClass.cbSize = sizeof(windowClass);
    windowClass.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    windowClass.lpfnWndProc = WindowProc;
    windowClass.hInstance = instance;
    windowClass.hCursor = LoadCursorW(nullptr, MAKEINTRESOURCEW(32512));
    windowClass.lpszClassName = kWindowClassName;
    if (!RegisterClassExW(&windowClass)) {
        return false;
    }

    constexpr DWORD style = WS_OVERLAPPEDWINDOW;
    RECT windowRect{0, 0, bp::kDefaultClientWidth, bp::kDefaultClientHeight};
    const UINT systemDpi = GetDpiForSystem();
    const UINT initialDpi = systemDpi == 0 ? 96 : systemDpi;
    if (!AdjustWindowRectExForDpi(&windowRect, style, FALSE, 0, initialDpi)) {
        AdjustWindowRectEx(&windowRect, style, FALSE, 0);
    }

    state.window = CreateWindowExW(
        0,
        kWindowClassName,
        kWindowTitle,
        style,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        windowRect.right - windowRect.left,
        windowRect.bottom - windowRect.top,
        nullptr,
        nullptr,
        instance,
        &state);
    if (!state.window) {
        return false;
    }

    ShowWindow(state.window, showCommand);
    UpdateWindow(state.window);
    return true;
}

#if defined(BP_DEBUG)
int RunPlatformSelfTests() noexcept {
    const bp::Viewport full = bp::ComputeViewport(1280, 720);
    if (full.scale != 4 || full.x != 0 || full.y != 0 || full.width != 1280 || full.height != 720) {
        return 101;
    }
    const bp::Viewport letterboxed = bp::ComputeViewport(1000, 700);
    if (letterboxed.scale != 3 || letterboxed.x != 20 || letterboxed.y != 80) {
        return 102;
    }
    const bp::Viewport minimum = bp::ComputeViewport(640, 360);
    if (minimum.scale != 2 || minimum.x != 0 || minimum.y != 0) {
        return 103;
    }
    std::uint16_t x = 0;
    std::uint16_t y = 0;
    bp::ClientToCanvas(letterboxed, -200, 900, x, y);
    if (x != 0 || y != 179) {
        return 104;
    }

    PlatformState inputState{};
    inputState.focused = true;
    SetKeyboardAction(inputState.input, bp::PlatformAction::Fire, true);
    BuildInputFrame(inputState);
    const std::uint32_t fire = ActionIndex(bp::PlatformAction::Fire);
    if (inputState.lastInputFrame.actions[fire].pressed != 1
        || inputState.lastInputFrame.actions[fire].held != 1) {
        return 105;
    }
    BuildInputFrame(inputState);
    if (inputState.lastInputFrame.actions[fire].pressed != 0
        || inputState.lastInputFrame.actions[fire].held != 1) {
        return 106;
    }
    SetKeyboardAction(inputState.input, bp::PlatformAction::Fire, false);
    BuildInputFrame(inputState);
    if (inputState.lastInputFrame.actions[fire].released != 1
        || inputState.lastInputFrame.actions[fire].held != 0) {
        return 107;
    }
    BuildInputFrame(inputState);
    if (inputState.lastInputFrame.actions[fire].released != 0) {
        return 108;
    }

    HandleKeyboardMessage(inputState, WM_KEYDOWN, 'W', 0);
    BuildInputFrame(inputState);
    HandleKeyboardMessage(inputState, WM_KEYDOWN, 'W', 1ll << 30);
    BuildInputFrame(inputState);
    const std::uint32_t moveUp = ActionIndex(bp::PlatformAction::MoveUp);
    if (inputState.lastInputFrame.actions[moveUp].pressed != 0
        || inputState.lastInputFrame.actions[moveUp].held != 1) {
        return 109;
    }

    inputState.accumulator60 = 123;
    SetFocused(inputState, false);
    if (inputState.focused || inputState.accumulator60 != 0
        || CombinedHeld(inputState.input, moveUp)) {
        return 110;
    }
    if (OutsideRadialDeadzone(7863, 0, kGamepadRightDeadzone)
        || !OutsideRadialDeadzone(7864, 0, kGamepadRightDeadzone)) {
        return 111;
    }

    PlatformState axisState{};
    axisState.focused = true;
    axisState.input.lastDevice = bp::InputDevice::Gamepad;
    axisState.input.gamepadMoveY = static_cast<std::int16_t>(-32768);
    axisState.input.gamepadAimY = static_cast<std::int16_t>(-32768);
    BuildInputFrame(axisState);
    if (axisState.lastInputFrame.moveY != 32767
        || axisState.lastInputFrame.aimY != 32767) {
        return 112;
    }
    return 0;
}
#endif

bool AppendPath(wchar_t (&path)[MAX_PATH],const wchar_t* suffix) noexcept {
    DWORD length=lstrlenW(path);
    const DWORD suffixLength=lstrlenW(suffix);
    if (length+suffixLength>=MAX_PATH) return false;
    for (DWORD index=0;index<=suffixLength;++index) path[length+index]=suffix[index];
    return true;
}

bool ResolveSavePaths(PlatformState& state) noexcept {
    const DWORD length=GetEnvironmentVariableW(
        L"LOCALAPPDATA",state.saveDirectory,MAX_PATH);
    if (length==0 || length>=MAX_PATH
        || !AppendPath(state.saveDirectory,L"\\BorrowedPixels")) return false;
    if (!CreateDirectoryW(state.saveDirectory,nullptr)
        && GetLastError()!=ERROR_ALREADY_EXISTS) return false;
    lstrcpyW(state.savePath,state.saveDirectory);
    lstrcpyW(state.saveTempPath,state.saveDirectory);
    lstrcpyW(state.saveInvalidPath,state.saveDirectory);
    if (!AppendPath(state.savePath,L"\\save.bin")
        || !AppendPath(state.saveTempPath,L"\\save.tmp")
        || !AppendPath(state.saveInvalidPath,L"\\save.invalid")) return false;
    state.savePathReady=true;
    return true;
}

bool ReadExactSave(const wchar_t* path,std::uint8_t (&bytes)[bp::kSaveV2Bytes]) noexcept {
    HANDLE file=CreateFileW(path,GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,nullptr);
    if (file==INVALID_HANDLE_VALUE) return false;
    LARGE_INTEGER size{};
    DWORD read=0;
    const bool ok=GetFileSizeEx(file,&size) && size.QuadPart==bp::kSaveV2Bytes
        && ReadFile(file,bytes,bp::kSaveV2Bytes,&read,nullptr)
        && read==bp::kSaveV2Bytes;
    CloseHandle(file);
    return ok;
}

bool WriteSaveAtomically(PlatformState& state,const bp::SaveData& save) noexcept {
    if (!state.savePathReady) return false;
    std::uint8_t bytes[bp::kSaveV2Bytes]{};
    if (!bp::SerializeSaveV2(save,bytes)) return false;
    HANDLE file=CreateFileW(state.saveTempPath,GENERIC_WRITE,0,nullptr,CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL|FILE_FLAG_WRITE_THROUGH,nullptr);
    if (file==INVALID_HANDLE_VALUE) return false;
    DWORD written=0;
    const bool wrote=WriteFile(file,bytes,bp::kSaveV2Bytes,&written,nullptr)
        && written==bp::kSaveV2Bytes && FlushFileBuffers(file);
    CloseHandle(file);
    if (!wrote) {
        DeleteFileW(state.saveTempPath);
        return false;
    }
    if (GetFileAttributesW(state.savePath)!=INVALID_FILE_ATTRIBUTES) {
        if (!ReplaceFileW(state.savePath,state.saveTempPath,nullptr,
                REPLACEFILE_WRITE_THROUGH,nullptr,nullptr)) {
            DeleteFileW(state.saveTempPath);
            return false;
        }
    } else if (!MoveFileExW(state.saveTempPath,state.savePath,
            MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)) {
        DeleteFileW(state.saveTempPath);
        return false;
    }
    return true;
}

void NarrativeFromSave(const bp::SaveData& save,bp::NarrativeState& narrative) noexcept {
    narrative=bp::NarrativeState{};
    narrative.choiceBits=save.choiceBits;
    narrative.adjudicatedMass=save.adjudicatedMass;
    narrative.encounterIndex=save.encounterIndex;
    narrative.role=save.role;
    for (std::uint8_t index=0;index<bp::kSettlementEncounterCount;++index)
        narrative.entityStatus[index]=save.entityStatus[index];
    narrative.endingsSeen=static_cast<std::uint8_t>(save.endingsSeen);
}

void LoadPersistentSave(PlatformState& state) noexcept {
    if (!ResolveSavePaths(state)) return;
    if (GetFileAttributesW(state.savePath)==INVALID_FILE_ATTRIBUTES) return;
    std::uint8_t bytes[bp::kSaveV2Bytes]{};
    bp::SaveData save{};
    if (!ReadExactSave(state.savePath,bytes)
        || !bp::DeserializeSaveV2(bytes,bp::kSaveV2Bytes,save)) {
        if (GetFileAttributesW(state.saveInvalidPath)==INVALID_FILE_ATTRIBUTES)
            (void)MoveFileW(state.savePath,state.saveInvalidPath);
        return;
    }
    bp::NarrativeState narrative{};
    NarrativeFromSave(save,narrative);
    if (!bp::InitializeTitleNarrative(
            state.narrativeFlow,save.claimLedger,narrative,save.flags)) return;
    state.persistentSave=save;
    state.options=save.options;
    state.runSeed=save.runSeed;
    state.bestFullRunTicks=save.bestFullRunTicks;
    state.tutorial=save.tutorial;
    state.narrativeActive=true;
}

void CurrentStoryState(PlatformState& state,const bp::ClaimLedger*& claims,
    const bp::NarrativeState*& narrative) noexcept {
    if (state.narrativeActive) {
        claims=&state.narrativeFlow.claimLedger; narrative=&state.narrativeFlow.narrative;
    } else if (state.precedentActive) {
        claims=&state.precedent.claimLedger; narrative=&state.precedent.narrative;
    } else if (state.act3Active) {
        claims=&state.act3.claimLedger; narrative=&state.act3.narrative;
    } else if (state.act2Active) {
        claims=&state.act2.claimLedger; narrative=&state.act2.narrative;
    } else {
        claims=&state.act1.claimLedger; narrative=&state.act1.narrative;
    }
}

void WriteCurrentSave(PlatformState& state) noexcept {
    const bp::ClaimLedger* claims=nullptr;
    const bp::NarrativeState* narrative=nullptr;
    CurrentStoryState(state,claims,narrative);
    std::uint32_t flags=1u;
    if (state.narrativeActive) {
        if (state.narrativeFlow.vesselContinueAvailable!=0) flags|=1u<<1u;
        if (state.narrativeFlow.fullEndingReached!=0) flags|=1u<<2u;
        if (state.narrativeFlow.auditTraceUnlocked!=0) flags|=1u<<3u;
    } else flags|=state.persistentSave.flags&0x0eu;
    const bp::SaveData save=bp::MakeSaveData(*claims,*narrative,flags,state.options,
        state.runSeed,state.bestFullRunTicks,state.tutorial);
    if (WriteSaveAtomically(state,save)) state.persistentSave=save;
    else state.saveWarningTicks=180;
}

void MaybeWriteBoundarySave(PlatformState& state) noexcept {
    std::uint8_t stage=1;
    std::uint8_t encounter=state.act1.encounterIndex;
    std::uint8_t scene=static_cast<std::uint8_t>(state.act1.scene);
    bool boundary=state.act1.scene==bp::Act1Scene::LedgerFocus
        || state.act1.scene==bp::Act1Scene::EncounterIntro
        || state.act1.scene==bp::Act1Scene::Settlement
        || state.act1.scene==bp::Act1Scene::EntityResponse;
    bool forbidden=state.act1.scene==bp::Act1Scene::Preview
        || state.act1.scene==bp::Act1Scene::HitFreeze;
    if (state.narrativeActive) {
        stage=5; encounter=state.narrativeFlow.auditIndex;
        scene=static_cast<std::uint8_t>(state.narrativeFlow.scene);
        boundary=state.narrativeFlow.scene==bp::NarrativeScene::Ending;
        forbidden=state.narrativeFlow.scene==bp::NarrativeScene::FinalPreview;
    } else if (state.precedentActive) {
        stage=4; encounter=static_cast<std::uint8_t>(bp::EncounterId::Precedent);
        scene=static_cast<std::uint8_t>(state.precedent.scene);
        boundary=state.precedent.scene==bp::PrecedentScene::RoleAssign;
        forbidden=state.precedent.scene==bp::PrecedentScene::HitFreeze;
    } else if (state.act3Active) {
        stage=3; encounter=state.act3.encounterIndex;
        scene=static_cast<std::uint8_t>(state.act3.scene);
        boundary=state.act3.scene==bp::Act3Scene::EncounterIntro
            || state.act3.scene==bp::Act3Scene::Settlement
            || state.act3.scene==bp::Act3Scene::EntityResponse;
        forbidden=state.act3.scene==bp::Act3Scene::Preview
            || state.act3.scene==bp::Act3Scene::HitFreeze;
    } else if (state.act2Active) {
        stage=2; encounter=state.act2.encounterIndex;
        scene=static_cast<std::uint8_t>(state.act2.scene);
        boundary=state.act2.scene==bp::Act2Scene::EncounterIntro
            || state.act2.scene==bp::Act2Scene::Settlement
            || state.act2.scene==bp::Act2Scene::EntityResponse;
        forbidden=state.act2.scene==bp::Act2Scene::Preview
            || state.act2.scene==bp::Act2Scene::HitFreeze;
    }
    if (bp::ObserveSaveBoundary(state.saveObserver,stage,encounter,scene,state.options,
            boundary,forbidden)) WriteCurrentSave(state);
}

void ShutdownPlatform(PlatformState& state) noexcept {
    if (state.waveOut) {
        waveOutReset(state.waveOut);
        for (std::uint8_t index=0;index<bp::kAudioBufferCount;++index)
            if ((state.audioHeaders[index].dwFlags&WHDR_PREPARED)!=0)
                waveOutUnprepareHeader(state.waveOut,&state.audioHeaders[index],sizeof(WAVEHDR));
        waveOutClose(state.waveOut);
        state.waveOut=nullptr;
    }
    if (state.xinputModule) {
        FreeLibrary(state.xinputModule);
        state.xinputModule = nullptr;
    }
    if (state.frameTimer) {
        CloseHandle(state.frameTimer);
        state.frameTimer = nullptr;
    }
    if (state.fallbackTimerPeriod) {
        timeEndPeriod(1);
        state.fallbackTimerPeriod = false;
    }
    if (state.backbuffer.pixels) {
        VirtualFree(state.backbuffer.pixels, 0, MEM_RELEASE);
        state.backbuffer.pixels = nullptr;
    }
}

}  // namespace

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int showCommand) {
#if defined(BP_DEBUG)
    const int selfTestResult = RunPlatformSelfTests();
    if (selfTestResult != 0) {
        return selfTestResult;
    }
#endif

    PlatformState state{};
    gPlatform = &state;
    state.running = true;
    state.focused = true;
    state.options=10u;
    state.input.lastDevice = bp::InputDevice::KeyboardMouse;
    if (!bp::InitializeAct1Runtime(state.act1)) {
        return 2;
    }
    LoadPersistentSave(state);
    state.windowedPlacement.length = sizeof(state.windowedPlacement);
    state.dpiAware = InitializeDpiAwareness();
    state.audioAvailable = InitializeWaveOut(state);
    if (!state.audioAvailable) bp::InitializeAudio(state.audio,false);
    InitializeXInput(state);

#if defined(BP_DEBUG)
    state.smokeTest = std::wcsstr(GetCommandLineW(), L"--smoke-test") != nullptr;
#endif

    if (!InitializeBackbuffer(state.backbuffer) || !QueryPerformanceFrequency(&state.qpcFrequency)) {
        MessageBoxW(nullptr, L"BP ERROR 1", kWindowTitle, MB_OK | MB_ICONERROR);
        ShutdownPlatform(state);
        return 1;
    }
    InitializeFrameTimer(state);

    if (!CreateMainWindow(state, instance, showCommand)) {
        MessageBoxW(nullptr, L"BP ERROR 1", kWindowTitle, MB_OK | MB_ICONERROR);
        ShutdownPlatform(state);
        return 1;
    }

#if defined(BP_DEBUG)
    if (state.smokeTest) {
        RECT client{};
        if (!GetClientRect(state.window, &client)
            || client.right != bp::kDefaultClientWidth
            || client.bottom != bp::kDefaultClientHeight) {
            DestroyWindow(state.window);
            ShutdownPlatform(state);
            return 120;
        }
        const LONG_PTR windowedStyle = GetWindowLongPtrW(state.window, GWL_STYLE);
        SendMessageW(state.window, WM_SYSKEYDOWN, VK_RETURN, 1ll << 29);
        if (!state.fullscreen
            || (GetWindowLongPtrW(state.window, GWL_STYLE) & WS_OVERLAPPEDWINDOW) != 0) {
            DestroyWindow(state.window);
            ShutdownPlatform(state);
            return 122;
        }
        SendMessageW(state.window, WM_SYSKEYDOWN, VK_RETURN, 1ll << 29);
        if (state.fullscreen || GetWindowLongPtrW(state.window, GWL_STYLE) != windowedStyle) {
            DestroyWindow(state.window);
            ShutdownPlatform(state);
            return 123;
        }
    }
#endif

    state.lastCounter = QueryCounter();
    MSG message{};
    while (state.running) {
        while (PeekMessageW(&message, nullptr, 0, 0, PM_REMOVE)) {
            if (message.message == WM_QUIT) {
                state.running = false;
                break;
            }
            TranslateMessage(&message);
            DispatchMessageW(&message);
        }
        if (!state.running) {
            break;
        }

        const std::int64_t frameStart = QueryCounter();
        std::int64_t elapsed = frameStart - state.lastCounter;
        state.lastCounter = frameStart;
        if (elapsed < 0 || elapsed > state.qpcFrequency.QuadPart / 4) {
            elapsed = state.qpcFrequency.QuadPart / 4;
        }

        PollXInput(state);
        if (state.focused) {
            state.accumulator60 += elapsed * bp::kSimulationHz;
            std::uint32_t ticks = 0;
            while (state.accumulator60 >= state.qpcFrequency.QuadPart && ticks < bp::kMaxCatchUpTicks) {
                BuildInputFrame(state);
                bool tickOk = true;
                if (!state.optionsActive && state.narrativeActive
                    && state.narrativeFlow.scene==bp::NarrativeScene::Title
                    && state.lastInputFrame.actions[ActionIndex(bp::PlatformAction::Pause)].pressed!=0) {
                    bp::InitializeOptionsRuntime(state.optionsRuntime,state.options);
                    state.optionsActive=true;
                }
                if (state.optionsActive) {
                    tickOk=bp::TickOptionsRuntime(state.optionsRuntime,state.lastInputFrame);
                    if (tickOk&&state.optionsRuntime.changed!=0) {
                        const bool wasBorderless=bp::BorderlessEnabled(state.options);
                        state.options=state.optionsRuntime.value;
                        if (state.window&&wasBorderless!=bp::BorderlessEnabled(state.options))
                            ToggleBorderlessFullscreen(state);
                        WriteCurrentSave(state);
                        (void)bp::QueueAudioEvent(state.audio,bp::AudioEvent::MenuMove);
                    }
                    if (state.optionsRuntime.closeRequested!=0) state.optionsActive=false;
                } else if (state.narrativeActive) {
                    tickOk=bp::TickNarrativeRuntime(state.narrativeFlow,state.lastInputFrame);
                    if (tickOk && state.narrativeFlow.routeRequest
                            ==bp::NarrativeRouteRequest::NewRun) {
                        tickOk=bp::InitializeAct1Runtime(state.act1);
                        state.act2Active=false;
                        state.act3Active=false;
                        state.precedentActive=false;
                        state.narrativeActive=false;
                    } else if (tickOk && state.narrativeFlow.routeRequest
                            ==bp::NarrativeRouteRequest::ContinueVessel) {
                        tickOk=bp::InitializeAct2Runtime(state.act2,
                            state.narrativeFlow.claimLedger,state.narrativeFlow.narrative);
                        state.act2Active=tickOk;
                        state.act3Active=false;
                        state.precedentActive=false;
                        state.narrativeActive=false;
                    }
                } else if (!state.act2Active) {
                    tickOk = bp::TickAct1Runtime(state.act1, state.lastInputFrame);
                    if (tickOk && state.act1.scene==bp::Act1Scene::EnoughEnding) {
                        tickOk=bp::InitializeEnoughNarrative(state.narrativeFlow,
                            state.act1.claimLedger,state.act1.narrative);
                        state.narrativeActive=tickOk;
                    } else if (tickOk && state.act1.scene == bp::Act1Scene::ContinueReady) {
                        tickOk = bp::InitializeAct2Runtime(
                            state.act2, state.act1.claimLedger, state.act1.narrative);
                        state.act2Active = tickOk;
                    }
                } else if (!state.act3Active) {
                    tickOk = bp::TickAct2Runtime(state.act2, state.lastInputFrame);
                    if (tickOk && state.act2.scene == bp::Act2Scene::Complete) {
                        tickOk = bp::InitializeAct3Runtime(
                            state.act3, state.act2.claimLedger, state.act2.narrative);
                        state.act3Active = tickOk;
                    }
                } else if (!state.precedentActive) {
                    tickOk = bp::TickAct3Runtime(state.act3, state.lastInputFrame);
                    if (tickOk && state.act3.scene == bp::Act3Scene::Complete) {
                        tickOk = bp::InitializePrecedentRuntime(state.precedent,
                            state.act3.claimLedger,state.act3.narrative);
                        state.precedentActive = tickOk;
                    }
                } else {
                    tickOk = bp::TickPrecedentRuntime(state.precedent,state.lastInputFrame);
                    if (tickOk && state.precedent.scene==bp::PrecedentScene::Complete) {
                        tickOk=bp::InitializeFinalNarrative(state.narrativeFlow,
                            state.precedent.claimLedger,state.precedent.narrative);
                        state.narrativeActive=tickOk;
                    }
                }
                if (!tickOk) {
                    state.running = false;
                    break;
                }
                MaybeWriteBoundarySave(state);
                if (state.saveWarningTicks!=0) --state.saveWarningTicks;
                state.accumulator60 -= state.qpcFrequency.QuadPart;
                ++ticks;
            }
            if (ticks == bp::kMaxCatchUpTicks && state.accumulator60 >= state.qpcFrequency.QuadPart) {
                state.accumulator60 = state.qpcFrequency.QuadPart - 1;
            }
        } else {
            state.accumulator60 = 0;
        }

        PresentWindow(state);
        ServiceWaveOut(state);
        ++state.renderFrame;

#if defined(BP_DEBUG)
        if (state.smokeTest && state.renderFrame == 30) {
            SendMessageW(state.window, WM_KILLFOCUS, 0, 0);
            state.smokePausedTick = state.globalTick;
        } else if (state.smokeTest && state.renderFrame == 60) {
            if (state.globalTick != state.smokePausedTick) {
                state.smokeTestResult = 121;
                PostMessageW(state.window, WM_CLOSE, 0, 0);
            } else {
                SendMessageW(state.window, WM_SETFOCUS, 0, 0);
            }
        } else if (state.smokeTest && state.renderFrame >= 150) {
            PostMessageW(state.window, WM_CLOSE, 0, 0);
        }
#endif

        WaitForFrame(
            state,
            frameStart,
            state.focused ? bp::kSimulationHz : bp::kBackgroundRenderHz);
    }

    ShutdownPlatform(state);
    gPlatform = nullptr;
#if defined(BP_DEBUG)
    return state.smokeTestResult;
#else
    return 0;
#endif
}
