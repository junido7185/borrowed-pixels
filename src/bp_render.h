#pragma once

#include <cstddef>
#include <cstdint>

namespace bp {

inline constexpr std::int32_t kRenderWidth = 320;
inline constexpr std::int32_t kRenderHeight = 180;

enum class PaletteColor : std::uint8_t {
    Background = 0,
    LedgerGrid,
    Player,
    Opponent,
    Neutral,
    Cover,
    Core,
    Warning,
    UiInactive,
    Count,
};

inline constexpr std::uint32_t kPalette[] = {
    0x00070A0Eu,
    0x00161D24u,
    0x0034D8C4u,
    0x00F55264u,
    0x00CAD0DAu,
    0x00556373u,
    0x00FFEEB8u,
    0x00FFBE5Cu,
    0x00303B46u,
};

struct RectI {
    std::int32_t x;
    std::int32_t y;
    std::int32_t width;
    std::int32_t height;
};

struct PixelBuffer {
    std::uint32_t* pixels;
    std::int32_t width;
    std::int32_t height;
    std::int32_t stride;
};

struct PolicyAuditRow {
    const char* subject;
    const char* choice;
    const char* rule;
};

struct PolicyAuditView {
    PolicyAuditRow rows[4];
};

enum class UiReferenceScreen : std::uint8_t {
    Title = 0,
    GlobalLedger,
    Combat,
    Settlement,
    PolicyAudit,
    EndingOpenLedger,
    Count,
};

constexpr std::uint32_t Color(PaletteColor color) noexcept {
    return kPalette[static_cast<std::uint8_t>(color)];
}

bool IsPaletteColor(std::uint32_t color) noexcept;
bool IsValidBuffer(const PixelBuffer& buffer) noexcept;
void Clear(PixelBuffer buffer, PaletteColor color) noexcept;
void PutPixel(PixelBuffer buffer, std::int32_t x, std::int32_t y, PaletteColor color) noexcept;
void FillRect(PixelBuffer buffer, RectI rect, PaletteColor color) noexcept;
void StrokeRect(PixelBuffer buffer, RectI rect, PaletteColor color) noexcept;
void DrawLine(
    PixelBuffer buffer,
    std::int32_t x0,
    std::int32_t y0,
    std::int32_t x1,
    std::int32_t y1,
    PaletteColor color) noexcept;
void FillChecker(PixelBuffer buffer, RectI rect, PaletteColor color, std::uint8_t phase = 0) noexcept;
void DrawLedgerGrid(PixelBuffer buffer) noexcept;
void RenderPolicyAudit(PixelBuffer buffer, const PolicyAuditView& view) noexcept;
void RenderUiReference(PixelBuffer buffer, UiReferenceScreen screen) noexcept;
std::uint32_t PixelFnv1a32(const PixelBuffer& buffer) noexcept;

static_assert(sizeof(kPalette) / sizeof(kPalette[0]) == static_cast<std::size_t>(PaletteColor::Count));

}  // namespace bp
