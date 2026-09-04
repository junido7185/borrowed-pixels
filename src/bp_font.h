#pragma once

#include "bp_render.h"

#include <cstddef>
#include <cstdint>

namespace bp {

inline constexpr std::int32_t kLatinGlyphWidth = 5;
inline constexpr std::int32_t kLatinGlyphHeight = 7;
inline constexpr std::int32_t kLatinAdvance = 6;
inline constexpr std::int32_t kLatinSpaceAdvance = 4;
inline constexpr std::int32_t kLatinLineHeight = 8;
inline constexpr std::int32_t kHangulGlyphWidth = 8;
inline constexpr std::int32_t kHangulGlyphHeight = 12;
inline constexpr std::int32_t kHangulAdvance = 9;
inline constexpr std::int32_t kHangulLineHeight = 13;
inline constexpr std::size_t kLatinGlyphCount = 59;

bool LatinGlyphRows(char codepoint, std::uint8_t (&rows)[kLatinGlyphHeight]) noexcept;
bool IsSupportedLatin(char codepoint) noexcept;
std::int32_t MeasureLatin(const char* text, std::int32_t scale = 1) noexcept;
bool DrawLatin(
    PixelBuffer buffer,
    std::int32_t x,
    std::int32_t y,
    const char* text,
    PaletteColor color,
    std::int32_t scale = 1) noexcept;

bool ComposeHangulGlyph(
    std::uint32_t codepoint,
    std::uint8_t (&rows)[kHangulGlyphHeight]) noexcept;
bool DrawHangulSyllable(
    PixelBuffer buffer,
    std::int32_t x,
    std::int32_t y,
    std::uint32_t codepoint,
    PaletteColor color) noexcept;

std::int32_t MeasureUtf8(const char* text,std::int32_t scale=1) noexcept;
bool DrawUtf8(PixelBuffer buffer,std::int32_t x,std::int32_t y,
    const char* text,PaletteColor color,std::int32_t scale=1) noexcept;

}  // namespace bp
