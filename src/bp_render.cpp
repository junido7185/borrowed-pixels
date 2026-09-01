#include "bp_render.h"

#include "bp_font.h"

namespace bp {
namespace {

std::int32_t Abs(std::int32_t value) noexcept {
    return value < 0 ? -value : value;
}

void Text(
    PixelBuffer buffer,
    std::int32_t x,
    std::int32_t y,
    const char* text,
    PaletteColor color = PaletteColor::Neutral,
    std::int32_t scale = 1) noexcept {
    (void)DrawLatin(buffer, x, y, text, color, scale);
}

void BaseScreen(PixelBuffer buffer) noexcept {
    Clear(buffer, PaletteColor::Background);
    DrawLedgerGrid(buffer);
}

void DrawTitleSeal(PixelBuffer buffer) noexcept {
    for (std::int32_t tileY = 0; tileY < 8; ++tileY) {
        for (std::int32_t tileX = 0; tileX < 8; ++tileX) {
            PutPixel(buffer, 152 + tileX * 2, 50 + tileY * 2, PaletteColor::Player);
        }
    }
}

void DrawTitle(PixelBuffer buffer) noexcept {
    BaseScreen(buffer);
    Text(buffer, 112, 22, "TOTAL TILE 1440");
    DrawTitleSeal(buffer);
    StrokeRect(buffer, RectI{151, 49, 18, 18}, PaletteColor::Player);
    Text(buffer, 61, 80, "BORROWED PIXELS", PaletteColor::Neutral, 2);
    Text(buffer, 91, 100, "EVERY PIXEL IS BORROWED");
    Text(buffer, 82, 124, "PRESS ANY KEY TO BORROW", PaletteColor::Warning);
    Text(buffer, 8, 166, "KO / EN");
    Text(buffer, 272, 166, "V2.1", PaletteColor::UiInactive);
}

void DrawGlobalLedger(PixelBuffer buffer) noexcept {
    BaseScreen(buffer);
    Text(buffer, 28, 8, "GLOBAL LEDGER");
    Text(buffer, 238, 8, "TOTAL 1440");
    StrokeRect(buffer, RectI{26, 22, 268, 10}, PaletteColor::Player);

    constexpr const char* names[12] = {
        "PLAYER", "PATCH", "PAIR", "VESSEL", "KEEPER", "ADAPTER",
        "ENTITY // 240", "ENTITY // 48", "ENTITY // 72", "ENTITY // 240", "SYSTEM", "COMMON",
    };
    constexpr const char* masses[12] = {
        "144", "32", "64", "80", "112", "72", "", "", "", "", "320", "80",
    };
    constexpr const char* statuses[12] = {
        "HOLDER", "CONT.", "LINK", "RETURN", "SEALED", "SEALED",
        "SEALED", "SEALED", "SEALED", "SEALED", "", "OPEN",
    };
    constexpr std::int32_t barWidths[12] = {18, 7, 12, 16, 21, 14, 46, 9, 14, 46, 61, 15};
    for (std::int32_t row = 0; row < 12; ++row) {
        const std::int32_t y = 24 + row * 10;
        const PaletteColor rowColor = row == 0 ? PaletteColor::Player : PaletteColor::Neutral;
        Text(buffer, 30, y, names[row], rowColor);
        PaletteColor barColor = PaletteColor::LedgerGrid;
        if (row == 0) {
            barColor = PaletteColor::Player;
        } else if (row >= 1 && row <= 3) {
            barColor = PaletteColor::Opponent;
        } else if (row == 11) {
            barColor = PaletteColor::Neutral;
        }
        FillRect(buffer, RectI{112, y + 3, barWidths[row], 3}, barColor);
        if (masses[row][0] != '\0') {
            Text(buffer, 236, y, masses[row], rowColor);
        }
        if (statuses[row][0] != '\0') {
            Text(buffer, 262, y, statuses[row], rowColor);
        }
    }
    DrawLine(buffer, 28, 151, 291, 151, PaletteColor::LedgerGrid);
    Text(buffer, 28, 156, "SELECT THE NEXT CLAIM", PaletteColor::Warning);
    Text(buffer, 246, 156, "ENTER");
}

void DrawCombat(PixelBuffer buffer) noexcept {
    BaseScreen(buffer);
    StrokeRect(buffer, RectI{11, 17, 298, 150}, PaletteColor::LedgerGrid);
    StrokeRect(buffer, RectI{8, 4, 304, 8}, PaletteColor::LedgerGrid);
    FillRect(buffer, RectI{9, 5, 52, 6}, PaletteColor::Player);
    FillRect(buffer, RectI{61, 5, 71, 6}, PaletteColor::Opponent);
    FillRect(buffer, RectI{132, 5, 34, 6}, PaletteColor::Neutral);
    FillChecker(buffer, RectI{166, 5, 145, 6}, PaletteColor::LedgerGrid);

    StrokeRect(buffer, RectI{2, 22, 58, 20}, PaletteColor::Player);
    Text(buffer, 5, 25, "VITAL 144", PaletteColor::Player);
    DrawLine(buffer, 5, 36, 51, 36, PaletteColor::Player);
    DrawLine(buffer, 50, 34, 50, 39, PaletteColor::Warning);
    StrokeRect(buffer, RectI{260, 22, 58, 20}, PaletteColor::Player);
    Text(buffer, 264, 25, "IMPULSE", PaletteColor::Player);
    DrawLine(buffer, 264, 36, 310, 36, PaletteColor::Player);
    StrokeRect(buffer, RectI{78, 166, 164, 12}, PaletteColor::Player);
    Text(buffer, 83, 169, "VECTOR // 32 HIT", PaletteColor::Player);

    FillRect(buffer, RectI{103, 48, 28, 8}, PaletteColor::Cover);
    FillRect(buffer, RectI{189, 48, 28, 8}, PaletteColor::Cover);
    FillRect(buffer, RectI{103, 124, 28, 8}, PaletteColor::Cover);
    FillRect(buffer, RectI{189, 124, 28, 8}, PaletteColor::Cover);
    StrokeRect(buffer, RectI{45, 82, 16, 16}, PaletteColor::Player);
    FillRect(buffer, RectI{51, 88, 4, 4}, PaletteColor::Core);
    DrawLine(buffer, 43, 78, 43, 102, PaletteColor::Player);
    DrawLine(buffer, 63, 78, 63, 102, PaletteColor::Player);
    FillRect(buffer, RectI{252, 76, 28, 28}, PaletteColor::Opponent);
    FillRect(buffer, RectI{264, 88, 4, 4}, PaletteColor::Core);
    StrokeRect(buffer, RectI{248, 72, 36, 36}, PaletteColor::Opponent);
    FillRect(buffer, RectI{144, 68, 2, 2}, PaletteColor::Neutral);
    FillRect(buffer, RectI{176, 70, 2, 2}, PaletteColor::Neutral);
    FillRect(buffer, RectI{150, 112, 2, 2}, PaletteColor::Neutral);
    FillRect(buffer, RectI{184, 104, 2, 2}, PaletteColor::Neutral);
    DrawLine(buffer, 62, 90, 188, 90, PaletteColor::Player);
    FillRect(buffer, RectI{122, 88, 8, 4}, PaletteColor::Player);
    Text(buffer, 2, 168, "EXPOSE CORE");
    Text(buffer, 276, 168, "E HOLD");
}

void DrawSettlement(PixelBuffer buffer) noexcept {
    BaseScreen(buffer);
    StrokeRect(buffer, RectI{38, 22, 244, 136}, PaletteColor::LedgerGrid);
    Text(buffer, 48, 30, "SETTLEMENT", PaletteColor::Warning);
    Text(buffer, 48, 44, "VESSEL", PaletteColor::Neutral, 2);
    Text(buffer, 230, 46, "160", PaletteColor::Opponent);
    StrokeRect(buffer, RectI{46, 66, 228, 22}, PaletteColor::LedgerGrid);
    Text(buffer, 54, 71, "CLAIM");
    Text(buffer, 222, 71, "+64 YOU", PaletteColor::Player);
    StrokeRect(buffer, RectI{46, 90, 228, 22}, PaletteColor::Player);
    DrawLine(buffer, 43, 94, 39, 94, PaletteColor::Player);
    DrawLine(buffer, 39, 94, 39, 108, PaletteColor::Player);
    DrawLine(buffer, 39, 108, 43, 108, PaletteColor::Player);
    DrawLine(buffer, 277, 94, 281, 94, PaletteColor::Player);
    DrawLine(buffer, 281, 94, 281, 108, PaletteColor::Player);
    DrawLine(buffer, 281, 108, 277, 108, PaletteColor::Player);
    Text(buffer, 54, 95, "RETURN", PaletteColor::Player);
    Text(buffer, 207, 95, "NO TRANSFER");
    StrokeRect(buffer, RectI{46, 114, 228, 22}, PaletteColor::LedgerGrid);
    Text(buffer, 54, 119, "RELEASE");
    Text(buffer, 199, 119, "+64 COMMON");
    DrawLine(buffer, 48, 148, 154, 148, PaletteColor::Player);
    DrawLine(buffer, 154, 148, 272, 148, PaletteColor::LedgerGrid);
    Text(buffer, 48, 150, "HOLD TO COMMIT");
}

void DrawPolicyAuditFrame(PixelBuffer buffer) noexcept {
    BaseScreen(buffer);
    Text(buffer, 28, 10, "POLICY AUDIT // CASE TO RULE", PaletteColor::Warning);
    StrokeRect(buffer, RectI{28, 28, 264, 28}, PaletteColor::LedgerGrid);
    StrokeRect(buffer, RectI{28, 62, 264, 28}, PaletteColor::LedgerGrid);
    StrokeRect(buffer, RectI{28, 96, 264, 28}, PaletteColor::LedgerGrid);
    StrokeRect(buffer, RectI{28, 130, 264, 20}, PaletteColor::LedgerGrid);
    Text(buffer, 106, 160, "BUILD PRECEDENT", PaletteColor::Warning);
}

void DrawPolicyAuditRow(
    PixelBuffer buffer,
    const PolicyAuditRow& row,
    std::int32_t caseLine1Y,
    std::int32_t caseLine2Y,
    std::int32_t arrowY,
    std::int32_t ruleY,
    std::int32_t underlineY) noexcept {
    Text(buffer, 34, caseLine1Y, row.subject);
    Text(buffer, 34, caseLine2Y, "/");
    Text(buffer, 44, caseLine2Y, row.choice);
    Text(buffer, 116, arrowY, ">", PaletteColor::Warning);
    Text(buffer, 130, ruleY, row.rule, PaletteColor::Player);
    const std::int32_t ruleWidth = MeasureLatin(row.rule);
    if (ruleWidth > 0) {
        DrawLine(buffer, 130, underlineY, 130 + ruleWidth - 1, underlineY, PaletteColor::Player);
    }
}

void DrawEndingOpenLedger(PixelBuffer buffer) noexcept {
    BaseScreen(buffer);
    Text(buffer, 100, 26, "OPEN LEDGER", PaletteColor::Player, 2);
    Text(buffer, 46, 64, "EVERY CLAIM COULD BE CHALLENGED");
    Text(buffer, 46, 78, "NO SETTLEMENT WAS FINAL", PaletteColor::Warning);
    StrokeRect(buffer, RectI{40, 100, 240, 30}, PaletteColor::LedgerGrid);
    constexpr std::int32_t dividers[] = {60, 96, 140, 196, 244};
    for (std::int32_t x : dividers) {
        DrawLine(buffer, x, 100, x, 129, PaletteColor::LedgerGrid);
    }
    constexpr std::int32_t points[][2] = {{56,106},{91,116},{136,104},{192,120},{240,110}};
    constexpr std::int32_t ends[] = {88,133,189,237,266};
    for (std::int32_t index = 0; index < 5; ++index) {
        FillRect(buffer, RectI{points[index][0], points[index][1], 3, 3}, PaletteColor::Neutral);
        DrawLine(buffer, points[index][0] + 3, points[index][1] + 1, ends[index], points[index][1] + 1, PaletteColor::Neutral);
    }
    Text(buffer, 46, 144, "TOTAL 1440 // STILL MOVING");
    Text(buffer, 100, 164, "CONTINUE", PaletteColor::UiInactive);
}

}  // namespace

bool IsPaletteColor(std::uint32_t color) noexcept {
    for (std::uint32_t paletteColor : kPalette) {
        if (color == paletteColor) {
            return true;
        }
    }
    return false;
}

bool IsValidBuffer(const PixelBuffer& buffer) noexcept {
    return buffer.pixels != nullptr && buffer.width > 0 && buffer.height > 0 && buffer.stride >= buffer.width;
}

void Clear(PixelBuffer buffer, PaletteColor color) noexcept {
    if (!IsValidBuffer(buffer)) {
        return;
    }
    for (std::int32_t y = 0; y < buffer.height; ++y) {
        for (std::int32_t x = 0; x < buffer.width; ++x) {
            buffer.pixels[y * buffer.stride + x] = Color(color);
        }
    }
}

void PutPixel(PixelBuffer buffer, std::int32_t x, std::int32_t y, PaletteColor color) noexcept {
    if (IsValidBuffer(buffer) && x >= 0 && y >= 0 && x < buffer.width && y < buffer.height) {
        buffer.pixels[y * buffer.stride + x] = Color(color);
    }
}

void FillRect(PixelBuffer buffer, RectI rect, PaletteColor color) noexcept {
    if (!IsValidBuffer(buffer) || rect.width <= 0 || rect.height <= 0) {
        return;
    }
    const std::int32_t left = rect.x < 0 ? 0 : rect.x;
    const std::int32_t top = rect.y < 0 ? 0 : rect.y;
    const std::int32_t unclippedRight = rect.x + rect.width;
    const std::int32_t unclippedBottom = rect.y + rect.height;
    const std::int32_t right = unclippedRight > buffer.width ? buffer.width : unclippedRight;
    const std::int32_t bottom = unclippedBottom > buffer.height ? buffer.height : unclippedBottom;
    for (std::int32_t y = top; y < bottom; ++y) {
        for (std::int32_t x = left; x < right; ++x) {
            buffer.pixels[y * buffer.stride + x] = Color(color);
        }
    }
}

void StrokeRect(PixelBuffer buffer, RectI rect, PaletteColor color) noexcept {
    if (rect.width <= 0 || rect.height <= 0) {
        return;
    }
    DrawLine(buffer, rect.x, rect.y, rect.x + rect.width - 1, rect.y, color);
    DrawLine(buffer, rect.x, rect.y + rect.height - 1, rect.x + rect.width - 1, rect.y + rect.height - 1, color);
    DrawLine(buffer, rect.x, rect.y, rect.x, rect.y + rect.height - 1, color);
    DrawLine(buffer, rect.x + rect.width - 1, rect.y, rect.x + rect.width - 1, rect.y + rect.height - 1, color);
}

void DrawLine(
    PixelBuffer buffer,
    std::int32_t x0,
    std::int32_t y0,
    std::int32_t x1,
    std::int32_t y1,
    PaletteColor color) noexcept {
    const std::int32_t dx = Abs(x1 - x0);
    const std::int32_t sx = x0 < x1 ? 1 : -1;
    const std::int32_t dy = -Abs(y1 - y0);
    const std::int32_t sy = y0 < y1 ? 1 : -1;
    std::int32_t error = dx + dy;
    for (;;) {
        PutPixel(buffer, x0, y0, color);
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

void FillChecker(PixelBuffer buffer, RectI rect, PaletteColor color, std::uint8_t phase) noexcept {
    if (rect.width <= 0 || rect.height <= 0) {
        return;
    }
    for (std::int32_t y = rect.y; y < rect.y + rect.height; ++y) {
        for (std::int32_t x = rect.x; x < rect.x + rect.width; ++x) {
            if (((x - rect.x) + (y - rect.y) + phase) % 2 == 0) {
                PutPixel(buffer, x, y, color);
            }
        }
    }
}

void DrawLedgerGrid(PixelBuffer buffer) noexcept {
    for (std::int32_t y = 18; y < 166; ++y) {
        for (std::int32_t x = 12; x < 308; ++x) {
            if (((x - 12) % 8 == 0) || ((y - 18) % 8 == 0)) {
                PutPixel(buffer, x, y, PaletteColor::LedgerGrid);
            }
        }
    }
}

void RenderPolicyAudit(PixelBuffer buffer, const PolicyAuditView& view) noexcept {
    if (!IsValidBuffer(buffer)) {
        return;
    }
    DrawPolicyAuditFrame(buffer);
    DrawPolicyAuditRow(buffer, view.rows[0], 31, 43, 38, 31, 43);
    DrawPolicyAuditRow(buffer, view.rows[1], 65, 77, 72, 65, 77);
    DrawPolicyAuditRow(buffer, view.rows[2], 99, 111, 106, 99, 111);
    DrawPolicyAuditRow(buffer, view.rows[3], 131, 140, 136, 131, 144);
}

void RenderUiReference(PixelBuffer buffer, UiReferenceScreen screen) noexcept {
    if (!IsValidBuffer(buffer)) {
        return;
    }
    switch (screen) {
    case UiReferenceScreen::Title: DrawTitle(buffer); break;
    case UiReferenceScreen::GlobalLedger: DrawGlobalLedger(buffer); break;
    case UiReferenceScreen::Combat: DrawCombat(buffer); break;
    case UiReferenceScreen::Settlement: DrawSettlement(buffer); break;
    case UiReferenceScreen::PolicyAudit: {
        constexpr PolicyAuditView view{{
            {"PATCH", "CONTINUE", "A PROCESS MAY CONTINUE"},
            {"ADAPTER", "NEED", "THE LEAST RECEIVE FIRST"},
            {"FRAME", "OPEN", "THE AFFECTED MAY CHALLENGE"},
            {"WITNESS", "PRESERVE", "NAMES AND LOSSES REMAIN"},
        }};
        RenderPolicyAudit(buffer, view);
        break;
    }
    case UiReferenceScreen::EndingOpenLedger: DrawEndingOpenLedger(buffer); break;
    default: Clear(buffer, PaletteColor::Background); break;
    }
}

std::uint32_t PixelFnv1a32(const PixelBuffer& buffer) noexcept {
    if (!IsValidBuffer(buffer)) {
        return 0;
    }
    std::uint32_t hash = 2166136261u;
    for (std::int32_t y = 0; y < buffer.height; ++y) {
        for (std::int32_t x = 0; x < buffer.width; ++x) {
            const std::uint32_t pixel = buffer.pixels[y * buffer.stride + x];
            for (std::uint8_t shift = 0; shift < 32; shift += 8) {
                hash ^= static_cast<std::uint8_t>((pixel >> shift) & 0xffu);
                hash *= 16777619u;
            }
        }
    }
    return hash;
}

}  // namespace bp
