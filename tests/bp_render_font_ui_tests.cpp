#include "bp_font.h"
#include "bp_render.h"

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace {

std::uint32_t gT03Checks;
std::uint32_t gT03Failures;

void Check(bool condition, const char* name) noexcept {
    ++gT03Checks;
    if (!condition) {
        ++gT03Failures;
        std::printf("T03_FAIL\t%s\n", name);
    }
}

std::uint32_t PixelAt(const bp::PixelBuffer& buffer, std::int32_t x, std::int32_t y) noexcept {
    return buffer.pixels[y * buffer.stride + x];
}

std::uint32_t RowsFnv(const std::uint8_t* rows, std::size_t count, std::uint32_t hash) noexcept {
    for (std::size_t index = 0; index < count; ++index) {
        hash ^= rows[index];
        hash *= 16777619u;
    }
    return hash;
}

void TestPaletteAndPrimitives() noexcept {
    constexpr std::uint32_t expected[] = {
        0x00070A0Eu, 0x00161D24u, 0x0034D8C4u, 0x00F55264u, 0x00CAD0DAu,
        0x00556373u, 0x00FFEEB8u, 0x00FFBE5Cu, 0x00303B46u,
    };
    for (std::uint32_t color : expected) {
        Check(bp::IsPaletteColor(color), "palette required color");
    }
    Check(!bp::IsPaletteColor(0), "palette rejects black");
    Check(!bp::IsPaletteColor(0x00ffffffu), "palette rejects white");

    std::uint32_t guarded[126];
    for (std::uint32_t& pixel : guarded) {
        pixel = 0xdeadbeefu;
    }
    bp::PixelBuffer buffer{guarded + 13, 10, 10, 10};
    bp::Clear(buffer, bp::PaletteColor::Background);
    bp::FillRect(buffer, bp::RectI{-4, -3, 20, 18}, bp::PaletteColor::Player);
    bp::DrawLine(buffer, -20, -20, 30, 30, bp::PaletteColor::Warning);
    bp::StrokeRect(buffer, bp::RectI{-2, -2, 14, 14}, bp::PaletteColor::Neutral);
    for (std::size_t index = 0; index < 13; ++index) {
        Check(guarded[index] == 0xdeadbeefu, "primitive leading canary");
    }
    for (std::size_t index = 113; index < 126; ++index) {
        Check(guarded[index] == 0xdeadbeefu, "primitive trailing canary");
    }
    for (std::int32_t y = 0; y < 10; ++y) {
        for (std::int32_t x = 0; x < 10; ++x) {
            Check(bp::IsPaletteColor(PixelAt(buffer, x, y)), "primitive palette membership");
        }
    }
}

void TestLatinFont() noexcept {
    constexpr char required[] = " ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.,:;!?-+/\\><=[]()'\"_|%";
    std::uint32_t glyphCount = 0;
    std::uint32_t authorityHash = 2166136261u;
    for (const char* at = required; *at; ++at) {
        std::uint8_t rows[bp::kLatinGlyphHeight];
        Check(bp::LatinGlyphRows(*at, rows), "latin required glyph exists");
        authorityHash = RowsFnv(reinterpret_cast<const std::uint8_t*>(at), 1, authorityHash);
        authorityHash = RowsFnv(rows, sizeof(rows), authorityHash);
        bool any = false;
        for (std::uint8_t row : rows) {
            Check(row <= 31, "latin row range");
            any = any || row != 0;
        }
        Check(*at == ' ' || any, "latin glyph readable pixels");
        ++glyphCount;
    }
    Check(glyphCount == bp::kLatinGlyphCount, "latin required glyph count");
    Check(authorityHash == 0xE3AAAAB5u, "latin authority table golden");
    Check(!bp::IsSupportedLatin('a'), "latin lowercase rejected");
    Check(!bp::IsSupportedLatin('@'), "latin unspecified glyph rejected");
    Check(bp::MeasureLatin("A A", 1) == 16, "latin space advance");
    Check(bp::MeasureLatin("AB", 2) == 24, "latin scale2 advance");
    Check(bp::MeasureLatin("a", 1) == -1, "latin invalid measure");

    std::uint32_t pixels[32 * 12];
    bp::PixelBuffer buffer{pixels, 32, 12, 32};
    bp::Clear(buffer, bp::PaletteColor::Background);
    const std::uint32_t before = bp::PixelFnv1a32(buffer);
    Check(!bp::DrawLatin(buffer, 0, 0, "Aa", bp::PaletteColor::Neutral), "latin invalid draw rejected");
    Check(bp::PixelFnv1a32(buffer) == before, "latin invalid draw nonmutating");
    Check(bp::DrawLatin(buffer, 0, 0, "A A", bp::PaletteColor::Neutral), "latin valid draw");
}

void TestHangulFont() noexcept {
    std::uint32_t aggregate = 2166136261u;
    for (std::uint32_t codepoint = 0xAC00u; codepoint <= 0xD7A3u; ++codepoint) {
        std::uint8_t rows[bp::kHangulGlyphHeight];
        Check(bp::ComposeHangulGlyph(codepoint, rows), "hangul modern syllable supported");
        bool any = false;
        for (std::uint8_t row : rows) {
            any = any || row != 0;
        }
        Check(any, "hangul composed glyph nonempty");
        aggregate = RowsFnv(rows, sizeof(rows), aggregate);
    }
    std::uint8_t invalid[bp::kHangulGlyphHeight];
    Check(!bp::ComposeHangulGlyph(0xABFFu, invalid), "hangul low invalid rejected");
    Check(!bp::ComposeHangulGlyph(0xD7A4u, invalid), "hangul high invalid rejected");
    std::printf("T03_INFO\thangul=%08X\n", aggregate);
    constexpr std::uint32_t kExpectedAggregate = 0x160B0D5Bu;
    Check(aggregate == kExpectedAggregate, "hangul aggregate golden");
}

struct PolicyCaseToken {
    const char* subject;
    const char* choice;
};

constexpr PolicyCaseToken kPolicyCases[] = {
    {"PATCH", "RECLAIM"}, {"PATCH", "CONTINUE"}, {"PATCH", "UNMARK"},
    {"PAIR", "SELECT"}, {"PAIR", "SEPARATE"}, {"PAIR", "LINK"},
    {"VESSEL", "CLAIM"}, {"VESSEL", "RETURN"}, {"VESSEL", "RELEASE"},
    {"KEEPER", "PRIVATE"}, {"KEEPER", "MAINTAIN"}, {"KEEPER", "COMMON"},
    {"ADAPTER", "EQUAL"}, {"ADAPTER", "NEED"}, {"ADAPTER", "OUTPUT"},
    {"FRAME", "SELF"}, {"FRAME", "RESTORE"}, {"FRAME", "OPEN"},
    {"INDEX", "OPTIMIZE"}, {"INDEX", "REFUSE"}, {"INDEX", "REWRITE"},
    {"WITNESS", "PRESERVE"}, {"WITNESS", "COMPRESS"}, {"WITNESS", "ERASE"},
    {"CURRENT", "HOLD"}, {"CURRENT", "RETURN"}, {"CURRENT", "CIRCULATE"},
    {"CASE // HASH", "ERASE"},
};

constexpr const char* kPolicyRules[4][3] = {
    {"ORIGIN HAS PRIORITY", "A PROCESS MAY CONTINUE", "A SELF REQUIRES OTHERS"},
    {"EACH SHARE IS EQUAL", "THE LEAST RECEIVE FIRST", "OUTPUT EARNS PRIORITY"},
    {"SAME CASE, SAME ANSWER", "THE AUDITOR MAY OVERRIDE", "THE AFFECTED MAY CHALLENGE"},
    {"NAMES AND LOSSES REMAIN", "ONLY COUNTS REMAIN", "NO CASE MAY SPEAK"},
};

struct PolicyRowGeometry {
    std::int32_t line1Y;
    std::int32_t line2Y;
    std::int32_t arrowY;
    std::int32_t ruleY;
    std::int32_t underlineY;
    std::int32_t rowBottom;
};

constexpr PolicyRowGeometry kPolicyGeometry[4] = {
    {31,43,38,31,43,55},
    {65,77,72,65,77,89},
    {99,111,106,99,111,123},
    {131,140,136,131,144,149},
};

void DrawPolicyCaseLayer(
    bp::PixelBuffer buffer,
    const PolicyCaseToken& token,
    const PolicyRowGeometry& geometry) noexcept {
    bp::DrawLatin(buffer, 34, geometry.line1Y, token.subject, bp::PaletteColor::Neutral);
    bp::DrawLatin(buffer, 34, geometry.line2Y, "/", bp::PaletteColor::Neutral);
    bp::DrawLatin(buffer, 44, geometry.line2Y, token.choice, bp::PaletteColor::Neutral);
}

void TestPolicyAuditAmendment() noexcept {
    constexpr std::int32_t kCaseLineBudget = 76;
    for (const PolicyCaseToken& token : kPolicyCases) {
        Check(bp::MeasureLatin(token.subject) <= kCaseLineBudget, "policy subject fits 76px");
        char choiceLine[32];
        const int written = std::snprintf(choiceLine, sizeof(choiceLine), "/ %s", token.choice);
        Check(written > 0 && written < static_cast<int>(sizeof(choiceLine)), "policy choice line built");
        Check(bp::MeasureLatin(choiceLine) <= kCaseLineBudget, "policy choice fits 76px");
    }
    for (const auto& axisRules : kPolicyRules) {
        for (const char* rule : axisRules) {
            const std::int32_t width = bp::MeasureLatin(rule);
            Check(width > 0 && 130 + width - 1 <= 291, "policy rule fits row");
        }
    }

    static std::uint32_t casePixels[bp::kRenderWidth * bp::kRenderHeight];
    static std::uint32_t arrowPixels[bp::kRenderWidth * bp::kRenderHeight];
    static std::uint32_t rulePixels[bp::kRenderWidth * bp::kRenderHeight];
    bp::PixelBuffer caseLayer{casePixels, bp::kRenderWidth, bp::kRenderHeight, bp::kRenderWidth};
    bp::PixelBuffer arrowLayer{arrowPixels, bp::kRenderWidth, bp::kRenderHeight, bp::kRenderWidth};
    bp::PixelBuffer ruleLayer{rulePixels, bp::kRenderWidth, bp::kRenderHeight, bp::kRenderWidth};
    const std::uint32_t background = bp::Color(bp::PaletteColor::Background);
    std::uint32_t overlapCount = 0;
    for (const PolicyCaseToken& token : kPolicyCases) {
        for (const PolicyRowGeometry& geometry : kPolicyGeometry) {
            bp::Clear(caseLayer, bp::PaletteColor::Background);
            bp::Clear(arrowLayer, bp::PaletteColor::Background);
            bp::Clear(ruleLayer, bp::PaletteColor::Background);
            DrawPolicyCaseLayer(caseLayer, token, geometry);
            bp::DrawLatin(arrowLayer, 116, geometry.arrowY, ">", bp::PaletteColor::Warning);
            const char* longestRule = "THE AFFECTED MAY CHALLENGE";
            bp::DrawLatin(ruleLayer, 130, geometry.ruleY, longestRule, bp::PaletteColor::Player);
            const std::int32_t ruleWidth = bp::MeasureLatin(longestRule);
            bp::DrawLine(ruleLayer, 130, geometry.underlineY, 130 + ruleWidth - 1, geometry.underlineY, bp::PaletteColor::Player);
            std::uint32_t caseArrowOverlap = 0;
            std::uint32_t caseRuleOverlap = 0;
            for (std::int32_t pixel = 0; pixel < bp::kRenderWidth * bp::kRenderHeight; ++pixel) {
                const bool caseLit = casePixels[pixel] != background;
                caseArrowOverlap += caseLit && arrowPixels[pixel] != background ? 1u : 0u;
                caseRuleOverlap += caseLit && rulePixels[pixel] != background ? 1u : 0u;
            }
            overlapCount += caseArrowOverlap + caseRuleOverlap;
            Check(caseArrowOverlap == 0, "policy CASE/arrow pixel overlap zero");
            Check(caseRuleOverlap == 0, "policy CASE/rule pixel overlap zero");
        }
    }

    constexpr bp::PolicyAuditRow identity[3] = {
        {"PATCH", "RECLAIM", "ORIGIN HAS PRIORITY"},
        {"PATCH", "CONTINUE", "A PROCESS MAY CONTINUE"},
        {"PATCH", "UNMARK", "A SELF REQUIRES OTHERS"},
    };
    constexpr bp::PolicyAuditRow distribution[3] = {
        {"ADAPTER", "EQUAL", "EACH SHARE IS EQUAL"},
        {"ADAPTER", "NEED", "THE LEAST RECEIVE FIRST"},
        {"ADAPTER", "OUTPUT", "OUTPUT EARNS PRIORITY"},
    };
    constexpr bp::PolicyAuditRow governance[3] = {
        {"FRAME", "RESTORE", "SAME CASE, SAME ANSWER"},
        {"FRAME", "SELF", "THE AUDITOR MAY OVERRIDE"},
        {"FRAME", "OPEN", "THE AFFECTED MAY CHALLENGE"},
    };
    constexpr bp::PolicyAuditRow memory[3] = {
        {"WITNESS", "PRESERVE", "NAMES AND LOSSES REMAIN"},
        {"WITNESS", "COMPRESS", "ONLY COUNTS REMAIN"},
        {"CASE // HASH", "ERASE", "NO CASE MAY SPEAK"},
    };
    static std::uint32_t profilePixels[bp::kRenderWidth * bp::kRenderHeight];
    bp::PixelBuffer profileBuffer{profilePixels, bp::kRenderWidth, bp::kRenderHeight, bp::kRenderWidth};
    std::uint32_t profileCount = 0;
    for (const bp::PolicyAuditRow& identityRow : identity) {
        for (const bp::PolicyAuditRow& distributionRow : distribution) {
            for (const bp::PolicyAuditRow& governanceRow : governance) {
                for (const bp::PolicyAuditRow& memoryRow : memory) {
                    const bp::PolicyAuditView view{{identityRow, distributionRow, governanceRow, memoryRow}};
                    bp::RenderPolicyAudit(profileBuffer, view);
                    for (std::uint32_t pixel : profilePixels) {
                        Check(bp::IsPaletteColor(pixel), "policy profile palette membership");
                    }
                    const bp::PolicyAuditRow profileRows[4] = {
                        identityRow, distributionRow, governanceRow, memoryRow,
                    };
                    for (std::int32_t row = 0; row < 4; ++row) {
                        bp::Clear(caseLayer, bp::PaletteColor::Background);
                        bp::Clear(arrowLayer, bp::PaletteColor::Background);
                        bp::Clear(ruleLayer, bp::PaletteColor::Background);
                        DrawPolicyCaseLayer(
                            caseLayer,
                            PolicyCaseToken{profileRows[row].subject, profileRows[row].choice},
                            kPolicyGeometry[row]);
                        bp::DrawLatin(
                            arrowLayer, 116, kPolicyGeometry[row].arrowY,
                            ">", bp::PaletteColor::Warning);
                        bp::DrawLatin(
                            ruleLayer, 130, kPolicyGeometry[row].ruleY,
                            profileRows[row].rule, bp::PaletteColor::Player);
                        const std::int32_t profileRuleWidth = bp::MeasureLatin(profileRows[row].rule);
                        bp::DrawLine(
                            ruleLayer,
                            130, kPolicyGeometry[row].underlineY,
                            130 + profileRuleWidth - 1, kPolicyGeometry[row].underlineY,
                            bp::PaletteColor::Player);
                        std::uint32_t profileCaseArrowOverlap = 0;
                        std::uint32_t profileCaseRuleOverlap = 0;
                        for (std::int32_t pixel = 0; pixel < bp::kRenderWidth * bp::kRenderHeight; ++pixel) {
                            const bool caseLit = casePixels[pixel] != background;
                            profileCaseArrowOverlap += caseLit && arrowPixels[pixel] != background ? 1u : 0u;
                            profileCaseRuleOverlap += caseLit && rulePixels[pixel] != background ? 1u : 0u;
                        }
                        overlapCount += profileCaseArrowOverlap + profileCaseRuleOverlap;
                        Check(profileCaseArrowOverlap == 0, "policy profile CASE/arrow overlap zero");
                        Check(profileCaseRuleOverlap == 0, "policy profile CASE/rule overlap zero");
                    }
                    ++profileCount;
                }
            }
        }
    }
    Check(profileCount == 81, "policy 81 profiles rendered");
    Check(overlapCount == 0, "policy exhaustive lit-pixel overlap zero");
    std::printf("T03_INFO\tpolicy_profiles=%u\tlanguage_modes=1\toverlap_pixels=%u\n", profileCount, overlapCount);
}

void CheckScreenStructure(
    const bp::PixelBuffer& buffer,
    bp::UiReferenceScreen screen) noexcept {
    using bp::Color;
    using bp::PaletteColor;
    switch (screen) {
    case bp::UiReferenceScreen::Title:
        Check(PixelAt(buffer, 151, 49) == Color(PaletteColor::Player), "title seal outer rect");
        Check(PixelAt(buffer, 152, 50) == Color(PaletteColor::Player), "title seal tile origin");
        Check(PixelAt(buffer, 168, 66) == Color(PaletteColor::Player), "title seal outer end");
        {
            std::uint32_t materialPixels = 0;
            for (std::int32_t y = 50; y < 66; ++y) {
                for (std::int32_t x = 152; x < 168; ++x) {
                    materialPixels += PixelAt(buffer, x, y) == Color(PaletteColor::Player) ? 1u : 0u;
                }
            }
            Check(materialPixels == 64, "title seal exact 64 material pixels");
        }
        break;
    case bp::UiReferenceScreen::GlobalLedger:
        Check(PixelAt(buffer, 26, 22) == Color(PaletteColor::Player), "ledger focus rect");
        Check(PixelAt(buffer, 293, 31) == Color(PaletteColor::Player), "ledger focus rect end");
        Check(PixelAt(buffer, 28, 151) == Color(PaletteColor::LedgerGrid), "ledger footer divider");
        break;
    case bp::UiReferenceScreen::Combat:
        Check(PixelAt(buffer, 11, 17) == Color(PaletteColor::LedgerGrid), "combat arena rect");
        Check(PixelAt(buffer, 8, 4) == Color(PaletteColor::LedgerGrid), "combat ledger rect");
        Check(PixelAt(buffer, 103, 48) == Color(PaletteColor::Cover), "combat cover rect");
        Check(PixelAt(buffer, 51, 88) == Color(PaletteColor::Core), "combat core rect");
        break;
    case bp::UiReferenceScreen::Settlement:
        Check(PixelAt(buffer, 38, 22) == Color(PaletteColor::LedgerGrid), "settlement frame rect");
        Check(PixelAt(buffer, 46, 90) == Color(PaletteColor::Player), "settlement focused row rect");
        Check(PixelAt(buffer, 48, 148) == Color(PaletteColor::Player), "settlement hold bar");
        break;
    case bp::UiReferenceScreen::PolicyAudit:
        Check(PixelAt(buffer, 28, 28) == Color(PaletteColor::LedgerGrid), "audit axis row one");
        Check(PixelAt(buffer, 28, 62) == Color(PaletteColor::LedgerGrid), "audit axis row two");
        Check(PixelAt(buffer, 28, 96) == Color(PaletteColor::LedgerGrid), "audit axis row three");
        Check(PixelAt(buffer, 28, 130) == Color(PaletteColor::LedgerGrid), "audit memory row");
        Check(PixelAt(buffer, 116, 136) == Color(PaletteColor::Warning), "audit memory arrow");
        Check(PixelAt(buffer, 130, 144) == Color(PaletteColor::Player), "audit memory underline");
        break;
    case bp::UiReferenceScreen::EndingOpenLedger:
        Check(PixelAt(buffer, 40, 100) == Color(PaletteColor::LedgerGrid), "ending ledger visual rect");
        Check(PixelAt(buffer, 279, 129) == Color(PaletteColor::LedgerGrid), "ending ledger visual end");
        Check(PixelAt(buffer, 60, 100) == Color(PaletteColor::LedgerGrid), "ending divider");
        break;
    default: Check(false, "unknown screen structure"); break;
    }
}

void TestUiCaptures() noexcept {
    constexpr std::uint32_t expectedHashes[] = {
        0x1A039B19u, 0x9472891Eu, 0x1B9B76AAu,
        0x7DEB6BACu, 0x0F5C1C21u, 0xBCB22556u,
    };
    std::uint32_t pixels[bp::kRenderWidth * bp::kRenderHeight];
    bp::PixelBuffer buffer{pixels, bp::kRenderWidth, bp::kRenderHeight, bp::kRenderWidth};
    for (std::uint8_t index = 0; index < static_cast<std::uint8_t>(bp::UiReferenceScreen::Count); ++index) {
        const auto screen = static_cast<bp::UiReferenceScreen>(index);
        bp::RenderUiReference(buffer, screen);
        for (std::uint32_t pixel : pixels) {
            Check(bp::IsPaletteColor(pixel), "capture palette membership");
        }
        CheckScreenStructure(buffer, screen);
        const std::uint32_t hash = bp::PixelFnv1a32(buffer);
        if (hash != expectedHashes[index]) {
            std::printf("T03_INFO\tscreen=%u\tactual_hash=%08X\texpected_hash=%08X\n",
                index, hash, expectedHashes[index]);
        }
        Check(hash == expectedHashes[index], "capture frozen hash");
    }
}

void WriteU16(std::FILE* file, std::uint16_t value) noexcept {
    std::fputc(value & 0xffu, file);
    std::fputc((value >> 8) & 0xffu, file);
}

void WriteU32(std::FILE* file, std::uint32_t value) noexcept {
    std::fputc(value & 0xffu, file);
    std::fputc((value >> 8) & 0xffu, file);
    std::fputc((value >> 16) & 0xffu, file);
    std::fputc((value >> 24) & 0xffu, file);
}

bool WriteBmp(const char* path, const bp::PixelBuffer& buffer) noexcept {
    std::FILE* file = nullptr;
    if (fopen_s(&file, path, "wb") != 0 || !file) {
        return false;
    }
    const std::uint32_t rowBytes = static_cast<std::uint32_t>((buffer.width * 3 + 3) & ~3);
    const std::uint32_t pixelBytes = rowBytes * static_cast<std::uint32_t>(buffer.height);
    WriteU16(file, 0x4d42u);
    WriteU32(file, 54u + pixelBytes);
    WriteU16(file, 0); WriteU16(file, 0); WriteU32(file, 54);
    WriteU32(file, 40); WriteU32(file, static_cast<std::uint32_t>(buffer.width));
    WriteU32(file, static_cast<std::uint32_t>(buffer.height));
    WriteU16(file, 1); WriteU16(file, 24); WriteU32(file, 0); WriteU32(file, pixelBytes);
    WriteU32(file, 2835); WriteU32(file, 2835); WriteU32(file, 0); WriteU32(file, 0);
    for (std::int32_t y = buffer.height - 1; y >= 0; --y) {
        for (std::int32_t x = 0; x < buffer.width; ++x) {
            const std::uint32_t pixel = PixelAt(buffer, x, y);
            std::fputc(pixel & 0xffu, file);
            std::fputc((pixel >> 8) & 0xffu, file);
            std::fputc((pixel >> 16) & 0xffu, file);
        }
        for (std::uint32_t pad = static_cast<std::uint32_t>(buffer.width * 3); pad < rowBytes; ++pad) {
            std::fputc(0, file);
        }
    }
    return std::fclose(file) == 0;
}

void DrawLatinAtlas(bp::PixelBuffer buffer) noexcept {
    constexpr char glyphs[] = " ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.,:;!?-+/\\><=[]()'\"_|%";
    bp::Clear(buffer, bp::PaletteColor::Background);
    for (std::size_t index = 0; glyphs[index]; ++index) {
        const std::int32_t cellX = static_cast<std::int32_t>(index % 10) * 24;
        const std::int32_t cellY = static_cast<std::int32_t>(index / 10) * 32;
        std::uint8_t rows[bp::kLatinGlyphHeight];
        bp::LatinGlyphRows(glyphs[index], rows);
        for (std::int32_t y = 0; y < bp::kLatinGlyphHeight; ++y) {
            for (std::int32_t x = 0; x < bp::kLatinGlyphWidth; ++x) {
                if ((rows[y] & (1u << (bp::kLatinGlyphWidth - 1 - x))) != 0) {
                    bp::FillRect(buffer, bp::RectI{cellX + x * 4, cellY + y * 4, 4, 4}, bp::PaletteColor::Neutral);
                }
            }
        }
    }
}

void DrawHangulAtlas(bp::PixelBuffer buffer) noexcept {
    constexpr std::uint32_t glyphs[] = {
        0xAC00u, 0xAC01u, 0xB098u, 0xB2E4u, 0xB77Cu, 0xB9C8u, 0xBC14u, 0xC0ACu,
        0xC544u, 0xC790u, 0xCC28u, 0xCE74u, 0xD0C0u, 0xD30Cu, 0xD558u, 0xD7A3u,
    };
    bp::Clear(buffer, bp::PaletteColor::Background);
    for (std::size_t index = 0; index < sizeof(glyphs) / sizeof(glyphs[0]); ++index) {
        const std::int32_t cellX = static_cast<std::int32_t>(index % 8) * 36;
        const std::int32_t cellY = static_cast<std::int32_t>(index / 8) * 52;
        std::uint8_t rows[bp::kHangulGlyphHeight];
        bp::ComposeHangulGlyph(glyphs[index], rows);
        for (std::int32_t y = 0; y < bp::kHangulGlyphHeight; ++y) {
            for (std::int32_t x = 0; x < bp::kHangulGlyphWidth; ++x) {
                if ((rows[y] & (1u << (bp::kHangulGlyphWidth - 1 - x))) != 0) {
                    bp::FillRect(buffer, bp::RectI{cellX + x * 4, cellY + y * 4, 4, 4}, bp::PaletteColor::Neutral);
                }
            }
        }
    }
}

void WriteEvidenceIfRequested() noexcept {
    char* directory = nullptr;
    std::size_t length = 0;
    if (_dupenv_s(&directory, &length, "BP_T03_EVIDENCE_DIR") != 0 || !directory || length <= 1) {
        std::free(directory);
        return;
    }
    constexpr const char* names[] = {
        "title.bmp", "global-ledger.bmp", "combat.bmp", "settlement.bmp", "policy-audit.bmp", "ending-open-ledger.bmp",
    };
    std::uint32_t pixels[bp::kRenderWidth * bp::kRenderHeight];
    bp::PixelBuffer buffer{pixels, bp::kRenderWidth, bp::kRenderHeight, bp::kRenderWidth};
    char path[1024];
    for (std::uint8_t index = 0; index < static_cast<std::uint8_t>(bp::UiReferenceScreen::Count); ++index) {
        bp::RenderUiReference(buffer, static_cast<bp::UiReferenceScreen>(index));
        std::snprintf(path, sizeof(path), "%s\\%s", directory, names[index]);
        Check(WriteBmp(path, buffer), "write ui evidence bmp");
    }

    std::uint32_t latinPixels[240 * 192];
    bp::PixelBuffer latin{latinPixels, 240, 192, 240};
    DrawLatinAtlas(latin);
    std::snprintf(path, sizeof(path), "%s\\font-atlas-latin-4x.bmp", directory);
    Check(WriteBmp(path, latin), "write latin atlas bmp");

    std::uint32_t hangulPixels[288 * 104];
    bp::PixelBuffer hangul{hangulPixels, 288, 104, 288};
    DrawHangulAtlas(hangul);
    std::snprintf(path, sizeof(path), "%s\\font-atlas-hangul-4x.bmp", directory);
    Check(WriteBmp(path, hangul), "write hangul atlas bmp");
    std::free(directory);
}

struct T03TestRunner {
    T03TestRunner() noexcept {
        TestPaletteAndPrimitives();
        TestLatinFont();
        TestHangulFont();
        TestPolicyAuditAmendment();
        TestUiCaptures();
        WriteEvidenceIfRequested();
        if (gT03Failures != 0) {
            std::printf("T03_FAIL\tchecks=%u\tfailures=%u\n", gT03Checks, gT03Failures);
            std::exit(1);
        }
        std::printf("T03_PASS\tchecks=%u\tlatin_glyphs=%zu\thangul_syllables=11172\tcaptures=6\n",
            gT03Checks, bp::kLatinGlyphCount);
    }
};

T03TestRunner gT03TestRunner;

}  // namespace
