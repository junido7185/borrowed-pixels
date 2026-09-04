#include "bp_options.h"

#include "bp_font.h"

namespace bp {
namespace {

constexpr std::uint8_t Action(PlatformAction action) noexcept {
    return static_cast<std::uint8_t>(action);
}

bool Pressed(const InputFrame& input,PlatformAction action) noexcept {
    return input.actions[Action(action)].pressed!=0;
}

void Centered(PixelBuffer buffer,std::int32_t y,const char* text,
    PaletteColor color=PaletteColor::Neutral) noexcept {
    (void)DrawLatin(buffer,(kRenderWidth-MeasureLatin(text))/2,y,text,color);
}

std::uint32_t SetField(std::uint32_t value,std::uint32_t mask,std::uint32_t bits) noexcept {
    return (value&~mask)|(bits&mask);
}

}  // namespace

std::uint8_t MasterVolume(std::uint32_t options) noexcept {
    const std::uint8_t value=static_cast<std::uint8_t>(options&0x0fu);
    return value<=10?value:10;
}

bool EnglishLanguage(std::uint32_t options) noexcept { return (options&(1u<<4u))!=0; }
bool ScreenShakeEnabled(std::uint32_t options) noexcept { return (options&(1u<<5u))!=0; }
bool ReduceFlashEnabled(std::uint32_t options) noexcept { return (options&(1u<<6u))!=0; }
ColorAssistMode ColorAssist(std::uint32_t options) noexcept {
    const std::uint8_t value=static_cast<std::uint8_t>((options>>7u)&3u);
    return value<=2?static_cast<ColorAssistMode>(value):ColorAssistMode::Off;
}
bool BorderlessEnabled(std::uint32_t options) noexcept { return (options&(1u<<9u))!=0; }

void InitializeOptionsRuntime(OptionsRuntime& state,std::uint32_t options) noexcept {
    state=OptionsRuntime{};
    state.value=options&0x3ffu;
    if ((state.value&0x0fu)>10u) state.value=SetField(state.value,0x0fu,10u);
    if (((state.value>>7u)&3u)>2u)
        state.value=SetField(state.value,3u<<7u,0u);
    state.previousValue=state.value;
    state.focused=OptionRow::Language;
}

bool TickOptionsRuntime(OptionsRuntime& state,const InputFrame& input) noexcept {
    state.changed=0;
    if (Pressed(input,PlatformAction::Pause)) { state.closeRequested=1; return true; }
    std::uint8_t row=static_cast<std::uint8_t>(state.focused);
    if (Pressed(input,PlatformAction::MoveUp)||Pressed(input,PlatformAction::SelectUp))
        row=row==0?static_cast<std::uint8_t>(OptionRow::Count)-1u:row-1u;
    if (Pressed(input,PlatformAction::MoveDown))
        row=static_cast<std::uint8_t>((row+1u)%static_cast<std::uint8_t>(OptionRow::Count));
    state.focused=static_cast<OptionRow>(row);
    const bool decrease=Pressed(input,PlatformAction::SelectLeft);
    const bool increase=Pressed(input,PlatformAction::SelectRight)
        || Pressed(input,PlatformAction::Confirm);
    if (!decrease&&!increase) return true;
    state.previousValue=state.value;
    switch (state.focused) {
    case OptionRow::Language: state.value^=1u<<4u; break;
    case OptionRow::MasterVolume: {
        std::uint8_t volume=MasterVolume(state.value);
        if (decrease&&volume>0) --volume;
        if (increase&&volume<10) ++volume;
        state.value=SetField(state.value,0x0fu,volume);
        break;
    }
    case OptionRow::ScreenShake: state.value^=1u<<5u; break;
    case OptionRow::ReduceFlash: state.value^=1u<<6u; break;
    case OptionRow::ColorAssist: {
        std::uint8_t mode=static_cast<std::uint8_t>(ColorAssist(state.value));
        mode=decrease?static_cast<std::uint8_t>((mode+2u)%3u):
            static_cast<std::uint8_t>((mode+1u)%3u);
        state.value=SetField(state.value,3u<<7u,static_cast<std::uint32_t>(mode)<<7u);
        break;
    }
    case OptionRow::Fullscreen: state.value^=1u<<9u; break;
    case OptionRow::Count: return false;
    }
    state.changed=state.value!=state.previousValue?1u:0u;
    return true;
}

void RenderOptionsRuntime(PixelBuffer buffer,const OptionsRuntime& state) noexcept {
    Clear(buffer,PaletteColor::Background);
    DrawLedgerGrid(buffer);
    StrokeRect(buffer,RectI{54,22,212,136},PaletteColor::LedgerGrid);
    Centered(buffer,30,"OPTIONS",PaletteColor::Warning);
    constexpr const char* names[6]={"LANGUAGE","MASTER VOLUME","SCREEN SHAKE",
        "REDUCE FLASH","COLOR ASSIST","FULLSCREEN"};
    const char* values[6]{};
    values[0]=EnglishLanguage(state.value)?"EN":"KO";
    constexpr const char* volumes[11]={"0","1","2","3","4","5","6","7","8","9","10"};
    values[1]=volumes[MasterVolume(state.value)];
    values[2]=ScreenShakeEnabled(state.value)?"ON":"OFF";
    values[3]=ReduceFlashEnabled(state.value)?"ON":"OFF";
    constexpr const char* assist[3]={"OFF","SHAPES","HIGH CONTRAST"};
    values[4]=assist[static_cast<std::uint8_t>(ColorAssist(state.value))];
    values[5]=BorderlessEnabled(state.value)?"BORDERLESS":"WINDOW";
    for (std::uint8_t row=0;row<6;++row) {
        const std::int32_t y=50+row*16;
        const PaletteColor color=row==static_cast<std::uint8_t>(state.focused)
            ?PaletteColor::Player:PaletteColor::Neutral;
        if (row==static_cast<std::uint8_t>(state.focused))
            StrokeRect(buffer,RectI{60,y-3,200,13},PaletteColor::Player);
        (void)DrawLatin(buffer,66,y,names[row],color);
        const std::int32_t width=MeasureLatin(values[row]);
        (void)DrawLatin(buffer,252-width,y,values[row],color);
    }
    Centered(buffer,148,"ESC // BACK",PaletteColor::UiInactive);
}

void ApplyAccessibilityPostprocess(PixelBuffer buffer,std::uint32_t options) noexcept {
    if (!IsValidBuffer(buffer)||ColorAssist(options)!=ColorAssistMode::HighContrast) return;
    for (std::int32_t y=0;y<buffer.height;++y)
        for (std::int32_t x=0;x<buffer.width;++x) {
            std::uint32_t& pixel=buffer.pixels[y*buffer.stride+x];
            if (pixel==Color(PaletteColor::Background)) pixel=0x00000000u;
            else if (pixel==Color(PaletteColor::LedgerGrid)) pixel=0x0027313Bu;
        }
}

}  // namespace bp
