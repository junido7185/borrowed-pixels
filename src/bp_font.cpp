#include "bp_font.h"

#include <cstring>

namespace bp {
namespace {

struct LatinGlyph {
    char codepoint;
    std::uint8_t rows[kLatinGlyphHeight];
};

constexpr LatinGlyph kLatinGlyphs[kLatinGlyphCount] = {
    {' ', {0, 0, 0, 0, 0, 0, 0}},
    {'A', {14, 17, 17, 31, 17, 17, 17}}, {'B', {30, 17, 17, 30, 17, 17, 30}},
    {'C', {15, 16, 16, 16, 16, 16, 15}}, {'D', {30, 17, 17, 17, 17, 17, 30}},
    {'E', {31, 16, 16, 30, 16, 16, 31}}, {'F', {31, 16, 16, 30, 16, 16, 16}},
    {'G', {15, 16, 16, 23, 17, 17, 15}}, {'H', {17, 17, 17, 31, 17, 17, 17}},
    {'I', {31, 4, 4, 4, 4, 4, 31}}, {'J', {7, 2, 2, 2, 2, 18, 12}},
    {'K', {17, 18, 20, 24, 20, 18, 17}}, {'L', {16, 16, 16, 16, 16, 16, 31}},
    {'M', {17, 27, 21, 21, 17, 17, 17}}, {'N', {17, 25, 21, 19, 17, 17, 17}},
    {'O', {14, 17, 17, 17, 17, 17, 14}}, {'P', {30, 17, 17, 30, 16, 16, 16}},
    {'Q', {14, 17, 17, 17, 21, 18, 13}}, {'R', {30, 17, 17, 30, 20, 18, 17}},
    {'S', {15, 16, 16, 14, 1, 1, 30}}, {'T', {31, 4, 4, 4, 4, 4, 4}},
    {'U', {17, 17, 17, 17, 17, 17, 14}}, {'V', {17, 17, 17, 17, 17, 10, 4}},
    {'W', {17, 17, 17, 21, 21, 27, 17}}, {'X', {17, 17, 10, 4, 10, 17, 17}},
    {'Y', {17, 17, 10, 4, 4, 4, 4}}, {'Z', {31, 1, 2, 4, 8, 16, 31}},
    {'0', {14, 17, 19, 21, 25, 17, 14}}, {'1', {4, 12, 4, 4, 4, 4, 14}},
    {'2', {14, 17, 1, 2, 4, 8, 31}}, {'3', {30, 1, 1, 14, 1, 1, 30}},
    {'4', {2, 6, 10, 18, 31, 2, 2}}, {'5', {31, 16, 16, 30, 1, 1, 30}},
    {'6', {14, 16, 16, 30, 17, 17, 14}}, {'7', {31, 1, 2, 4, 8, 8, 8}},
    {'8', {14, 17, 17, 14, 17, 17, 14}}, {'9', {14, 17, 17, 15, 1, 1, 14}},
    {'.', {0, 0, 0, 0, 0, 6, 6}}, {',', {0, 0, 0, 0, 6, 4, 8}},
    {':', {0, 6, 6, 0, 6, 6, 0}}, {';', {0, 6, 6, 0, 6, 4, 8}},
    {'!', {4, 4, 4, 4, 4, 0, 4}}, {'?', {14, 17, 1, 2, 4, 0, 4}},
    {'-', {0, 0, 0, 31, 0, 0, 0}}, {'+', {0, 4, 4, 31, 4, 4, 0}},
    {'/', {1, 2, 2, 4, 8, 8, 16}}, {'\\', {16, 8, 8, 4, 2, 2, 1}},
    {'>', {16, 8, 4, 2, 4, 8, 16}}, {'<', {1, 2, 4, 8, 4, 2, 1}},
    {'=', {0, 0, 31, 0, 31, 0, 0}}, {'[', {14, 8, 8, 8, 8, 8, 14}},
    {']', {14, 2, 2, 2, 2, 2, 14}}, {'(', {2, 4, 8, 8, 8, 4, 2}},
    {')', {8, 4, 2, 2, 2, 4, 8}}, {'\'', {4, 4, 0, 0, 0, 0, 0}},
    {'"', {10, 10, 0, 0, 0, 0, 0}}, {'_', {0, 0, 0, 0, 0, 0, 31}},
    {'|', {4, 4, 4, 4, 4, 4, 4}}, {'%', {25, 26, 4, 8, 22, 6, 0}},
};

struct Segment {
    std::uint8_t x0;
    std::uint8_t y0;
    std::uint8_t x1;
    std::uint8_t y1;
};

struct SegmentSet {
    const Segment* segments;
    std::uint8_t count;
};

constexpr Segment kGiyeok[] = {{0,0,4,0},{4,0,4,4}};
constexpr Segment kNieun[] = {{0,0,0,4},{0,4,4,4}};
constexpr Segment kDigeut[] = {{0,0,4,0},{0,0,0,4},{0,4,4,4}};
constexpr Segment kRieul[] = {{0,0,4,0},{4,0,4,2},{4,2,0,2},{0,2,0,4},{0,4,4,4}};
constexpr Segment kMieum[] = {{0,0,4,0},{4,0,4,4},{4,4,0,4},{0,4,0,0}};
constexpr Segment kBieup[] = {{0,0,4,0},{4,0,4,4},{4,4,0,4},{0,4,0,0},{0,2,4,2}};
constexpr Segment kSiot[] = {{2,0,0,4},{2,0,4,4}};
constexpr Segment kIeung[] = {{1,0,3,0},{3,0,4,1},{4,1,4,3},{4,3,3,4},{3,4,1,4},{1,4,0,3},{0,3,0,1},{0,1,1,0}};
constexpr Segment kJieut[] = {{0,0,4,0},{2,1,0,4},{2,1,4,4}};
constexpr Segment kChieut[] = {{1,0,3,0},{0,2,4,2},{2,2,0,4},{2,2,4,4}};
constexpr Segment kKieuk[] = {{0,0,4,0},{4,0,4,4},{1,2,4,2}};
constexpr Segment kTieut[] = {{0,0,4,0},{0,0,0,4},{0,2,4,2},{0,4,4,4}};
constexpr Segment kPieup[] = {{0,0,4,0},{1,0,1,4},{3,0,3,4},{0,4,4,4}};
constexpr Segment kHieut[] = {{1,0,3,0},{0,1,4,1},{1,2,3,2},{3,2,4,3},{4,3,3,4},{3,4,1,4},{1,4,0,3},{0,3,1,2}};

constexpr Segment kA[] = {{2,0,2,4},{2,2,4,2}};
constexpr Segment kYa[] = {{2,0,2,4},{2,1,4,1},{2,3,4,3}};
constexpr Segment kEo[] = {{2,0,2,4},{0,2,2,2}};
constexpr Segment kYeo[] = {{2,0,2,4},{0,1,2,1},{0,3,2,3}};
constexpr Segment kO[] = {{0,3,4,3},{2,0,2,3}};
constexpr Segment kYo[] = {{0,3,4,3},{1,0,1,3},{3,0,3,3}};
constexpr Segment kU[] = {{0,1,4,1},{2,1,2,4}};
constexpr Segment kYu[] = {{0,1,4,1},{1,1,1,4},{3,1,3,4}};
constexpr Segment kEu[] = {{0,2,4,2}};
constexpr Segment kI[] = {{2,0,2,4}};
constexpr Segment kAeExtra[] = {{4,0,4,4}};

constexpr SegmentSet kConsonants[] = {
    {kGiyeok,2},{kNieun,2},{kDigeut,3},{kRieul,5},{kMieum,4},{kBieup,5},{kSiot,2},
    {kIeung,8},{kJieut,3},{kChieut,4},{kKieuk,3},{kTieut,4},{kPieup,4},{kHieut,8},
};

struct Box {
    std::int32_t x;
    std::int32_t y;
    std::int32_t width;
    std::int32_t height;
};

constexpr std::uint8_t kInitialBase[19] = {0,0,1,2,2,3,4,5,5,6,6,7,8,8,9,10,11,12,13};
constexpr bool kInitialDouble[19] = {false,true,false,false,true,false,false,false,true,false,true,false,false,true,false,false,false,false,false};
constexpr std::uint8_t kFinalFirst[28] = {255,0,0,0,1,1,1,2,3,3,3,3,3,3,3,3,4,5,5,6,6,7,8,9,10,11,12,13};
constexpr std::uint8_t kFinalSecond[28] = {255,255,255,6,255,8,13,255,255,0,4,5,6,11,12,13,255,255,6,255,255,255,255,255,255,255,255,255};
constexpr bool kFinalDouble[28] = {false,false,true,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,true,false,false,false,false,false,false,false};

std::int32_t RoundNormalized(std::int32_t value, std::int32_t size) noexcept {
    return (value * (size - 1) + 2) / 4;
}

void SetGlyphPixel(std::uint8_t (&rows)[kHangulGlyphHeight], std::int32_t x, std::int32_t y) noexcept {
    if (x >= 0 && x < kHangulGlyphWidth && y >= 0 && y < kHangulGlyphHeight) {
        rows[y] = static_cast<std::uint8_t>(rows[y] | (1u << (kHangulGlyphWidth - 1 - x)));
    }
}

void DrawGlyphLine(
    std::uint8_t (&rows)[kHangulGlyphHeight],
    std::int32_t x0,
    std::int32_t y0,
    std::int32_t x1,
    std::int32_t y1) noexcept {
    const std::int32_t dx = x1 > x0 ? x1 - x0 : x0 - x1;
    const std::int32_t sx = x0 < x1 ? 1 : -1;
    const std::int32_t dyAbs = y1 > y0 ? y1 - y0 : y0 - y1;
    const std::int32_t dy = -dyAbs;
    const std::int32_t sy = y0 < y1 ? 1 : -1;
    std::int32_t error = dx + dy;
    for (;;) {
        SetGlyphPixel(rows, x0, y0);
        if (x0 == x1 && y0 == y1) {
            break;
        }
        const std::int32_t twice = error * 2;
        if (twice >= dy) {
            error += dy;
            x0 += sx;
        }
        if (twice <= dx) {
            error += dx;
            y0 += sy;
        }
    }
}

void DrawSegmentSet(
    std::uint8_t (&rows)[kHangulGlyphHeight],
    const SegmentSet& set,
    Box box) noexcept {
    if (box.width <= 0 || box.height <= 0) {
        return;
    }
    for (std::uint8_t index = 0; index < set.count; ++index) {
        const Segment segment = set.segments[index];
        DrawGlyphLine(
            rows,
            box.x + RoundNormalized(segment.x0, box.width),
            box.y + RoundNormalized(segment.y0, box.height),
            box.x + RoundNormalized(segment.x1, box.width),
            box.y + RoundNormalized(segment.y1, box.height));
    }
}

void DrawConsonant(
    std::uint8_t (&rows)[kHangulGlyphHeight],
    std::uint8_t base,
    Box box,
    bool doubled) noexcept {
    if (doubled) {
        const std::int32_t leftWidth = box.width / 2;
        DrawSegmentSet(rows, kConsonants[base], Box{box.x, box.y, leftWidth, box.height});
        DrawSegmentSet(rows, kConsonants[base], Box{box.x + leftWidth, box.y, box.width - leftWidth, box.height});
        return;
    }
    DrawSegmentSet(rows, kConsonants[base], box);
}

void DrawVowelBase(std::uint8_t (&rows)[kHangulGlyphHeight], std::uint8_t index, Box box) noexcept {
    switch (index) {
    case 0: DrawSegmentSet(rows, {kA,2}, box); break;
    case 1: DrawSegmentSet(rows, {kAeExtra,1}, box); break;
    case 2: DrawSegmentSet(rows, {kYa,3}, box); break;
    case 3: DrawSegmentSet(rows, {kEo,2}, box); break;
    case 4: DrawSegmentSet(rows, {kYeo,3}, box); break;
    case 5: DrawSegmentSet(rows, {kO,2}, box); break;
    case 6: DrawSegmentSet(rows, {kYo,3}, box); break;
    case 7: DrawSegmentSet(rows, {kU,2}, box); break;
    case 8: DrawSegmentSet(rows, {kYu,3}, box); break;
    case 9: DrawSegmentSet(rows, {kEu,1}, box); break;
    case 10: DrawSegmentSet(rows, {kI,1}, box); break;
    default: break;
    }
}

void DrawVowel(std::uint8_t (&rows)[kHangulGlyphHeight], std::uint8_t vowel, Box box) noexcept {
    switch (vowel) {
    case 0: DrawVowelBase(rows, 0, box); break;
    case 1: DrawVowelBase(rows, 0, box); DrawVowelBase(rows, 1, box); break;
    case 2: DrawVowelBase(rows, 2, box); break;
    case 3: DrawVowelBase(rows, 2, box); DrawVowelBase(rows, 1, box); break;
    case 4: DrawVowelBase(rows, 3, box); break;
    case 5: DrawVowelBase(rows, 3, box); DrawVowelBase(rows, 1, box); break;
    case 6: DrawVowelBase(rows, 4, box); break;
    case 7: DrawVowelBase(rows, 4, box); DrawVowelBase(rows, 1, box); break;
    case 8: DrawVowelBase(rows, 5, box); break;
    case 9: DrawVowelBase(rows, 5, box); DrawVowelBase(rows, 0, box); break;
    case 10: DrawVowelBase(rows, 5, box); DrawVowelBase(rows, 0, box); DrawVowelBase(rows, 1, box); break;
    case 11: DrawVowelBase(rows, 5, box); DrawVowelBase(rows, 10, box); break;
    case 12: DrawVowelBase(rows, 6, box); break;
    case 13: DrawVowelBase(rows, 7, box); break;
    case 14: DrawVowelBase(rows, 7, box); DrawVowelBase(rows, 3, box); break;
    case 15: DrawVowelBase(rows, 7, box); DrawVowelBase(rows, 3, box); DrawVowelBase(rows, 1, box); break;
    case 16: DrawVowelBase(rows, 7, box); DrawVowelBase(rows, 10, box); break;
    case 17: DrawVowelBase(rows, 8, box); break;
    case 18: DrawVowelBase(rows, 9, box); break;
    case 19: DrawVowelBase(rows, 9, box); DrawVowelBase(rows, 10, box); break;
    case 20: DrawVowelBase(rows, 10, box); break;
    default: break;
    }
}

}  // namespace

bool LatinGlyphRows(char codepoint, std::uint8_t (&rows)[kLatinGlyphHeight]) noexcept {
    for (const LatinGlyph& glyph : kLatinGlyphs) {
        if (glyph.codepoint == codepoint) {
            std::memcpy(rows, glyph.rows, sizeof(rows));
            return true;
        }
    }
    std::memset(rows, 0, sizeof(rows));
    return false;
}

bool IsSupportedLatin(char codepoint) noexcept {
    std::uint8_t rows[kLatinGlyphHeight];
    return LatinGlyphRows(codepoint, rows);
}

std::int32_t MeasureLatin(const char* text, std::int32_t scale) noexcept {
    if (!text || (scale != 1 && scale != 2)) {
        return -1;
    }
    std::int32_t width = 0;
    for (const char* at = text; *at; ++at) {
        if (!IsSupportedLatin(*at)) {
            return -1;
        }
        width += (*at == ' ' ? kLatinSpaceAdvance : kLatinAdvance) * scale;
    }
    return width;
}

bool DrawLatin(
    PixelBuffer buffer,
    std::int32_t x,
    std::int32_t y,
    const char* text,
    PaletteColor color,
    std::int32_t scale) noexcept {
    if (!IsValidBuffer(buffer) || MeasureLatin(text, scale) < 0) {
        return false;
    }
    std::int32_t penX = x;
    for (const char* at = text; *at; ++at) {
        std::uint8_t rows[kLatinGlyphHeight];
        LatinGlyphRows(*at, rows);
        for (std::int32_t row = 0; row < kLatinGlyphHeight; ++row) {
            for (std::int32_t column = 0; column < kLatinGlyphWidth; ++column) {
                if ((rows[row] & (1u << (kLatinGlyphWidth - 1 - column))) != 0) {
                    FillRect(buffer, RectI{penX + column * scale, y + row * scale, scale, scale}, color);
                }
            }
        }
        penX += (*at == ' ' ? kLatinSpaceAdvance : kLatinAdvance) * scale;
    }
    return true;
}

bool ComposeHangulGlyph(
    std::uint32_t codepoint,
    std::uint8_t (&rows)[kHangulGlyphHeight]) noexcept {
    std::memset(rows, 0, sizeof(rows));
    if (codepoint < 0xAC00u || codepoint > 0xD7A3u) {
        return false;
    }
    const std::uint32_t syllable = codepoint - 0xAC00u;
    const std::uint8_t initial = static_cast<std::uint8_t>(syllable / 588u);
    const std::uint8_t vowel = static_cast<std::uint8_t>((syllable % 588u) / 28u);
    const std::uint8_t final = static_cast<std::uint8_t>(syllable % 28u);

    const bool horizontal = vowel == 8 || vowel == 12 || vowel == 13 || vowel == 17 || vowel == 18;
    const bool mixed = vowel == 9 || vowel == 10 || vowel == 11
        || vowel == 14 || vowel == 15 || vowel == 16 || vowel == 19;
    const Box initialBox = horizontal ? Box{1,0,6,6} : (mixed ? Box{0,0,4,7} : Box{0,0,4,8});
    const Box vowelBox = horizontal ? Box{1,6,6,2} : Box{4,0,4,8};
    const Box finalBox{1,8,6,4};

    DrawConsonant(rows, kInitialBase[initial], initialBox, kInitialDouble[initial]);
    DrawVowel(rows, vowel, vowelBox);
    if (final != 0) {
        const std::uint8_t first = kFinalFirst[final];
        const std::uint8_t second = kFinalSecond[final];
        if (second == 255) {
            DrawConsonant(rows, first, finalBox, kFinalDouble[final]);
        } else {
            DrawConsonant(rows, first, Box{finalBox.x, finalBox.y, 3, finalBox.height}, false);
            DrawConsonant(rows, second, Box{finalBox.x + 3, finalBox.y, 3, finalBox.height}, false);
        }
    }
    return true;
}

bool DrawHangulSyllable(
    PixelBuffer buffer,
    std::int32_t x,
    std::int32_t y,
    std::uint32_t codepoint,
    PaletteColor color) noexcept {
    if (!IsValidBuffer(buffer)) {
        return false;
    }
    std::uint8_t rows[kHangulGlyphHeight];
    if (!ComposeHangulGlyph(codepoint, rows)) {
        return false;
    }
    for (std::int32_t row = 0; row < kHangulGlyphHeight; ++row) {
        for (std::int32_t column = 0; column < kHangulGlyphWidth; ++column) {
            if ((rows[row] & (1u << (kHangulGlyphWidth - 1 - column))) != 0) {
                PutPixel(buffer, x + column, y + row, color);
            }
        }
    }
    return true;
}

static_assert(sizeof(kLatinGlyphs) / sizeof(kLatinGlyphs[0]) == kLatinGlyphCount);

}  // namespace bp
