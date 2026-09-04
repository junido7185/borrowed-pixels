#include "bp_font.h"
#include "bp_game.h"
#include "bp_narrative.h"
#include "bp_options.h"
#include "bp_strings.h"

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace {

std::uint32_t gChecks;
std::uint32_t gFailures;

void Check(bool condition,const char* name) noexcept {
    ++gChecks;
    if (!condition) { ++gFailures; std::printf("T12_FAIL\t%s\n",name); }
}

bool HasLowercaseAscii(const char* text) noexcept {
    if (!text) return false;
    for (const unsigned char* at=reinterpret_cast<const unsigned char*>(text);*at;++at)
        if (*at>='a'&&*at<='z') return true;
    return false;
}

std::uint32_t LitPixels(const std::uint32_t* pixels,std::uint32_t background) noexcept {
    std::uint32_t result=0;
    for (std::int32_t i=0;i<bp::kRenderWidth*bp::kRenderHeight;++i)
        result+=pixels[i]!=background?1u:0u;
    return result;
}

void TestFrozenCopyAndGlyphs() noexcept {
    std::uint16_t count=0;
    const bp::LocalizedString* strings=bp::RuntimeStrings(count);
    Check(strings!=nullptr&&count>=100,"frozen KO/EN runtime table present");
    std::uint32_t pixels[bp::kRenderWidth*bp::kRenderHeight]{};
    bp::PixelBuffer buffer{pixels,bp::kRenderWidth,bp::kRenderHeight,bp::kRenderWidth};
    for (std::uint16_t index=0;index<count;++index) {
        const bp::LocalizedString& entry=strings[index];
        Check(entry.id&&*entry.id&&entry.english&&*entry.english
            &&entry.koreanUtf8&&*entry.koreanUtf8,"localized row fields nonempty");
        Check(!HasLowercaseAscii(entry.english),"runtime English contains no lowercase");
        const std::int32_t englishWidth=bp::MeasureUtf8(entry.english);
        const std::int32_t koreanWidth=bp::MeasureUtf8(entry.koreanUtf8);
        Check(englishWidth>0&&englishWidth<=bp::kRenderWidth,
            "English frozen line fits 320px canvas");
        Check(koreanWidth>0&&koreanWidth<=bp::kRenderWidth,
            "Korean frozen line composes and fits 320px canvas");
        bp::Clear(buffer,bp::PaletteColor::Background);
        Check(bp::DrawUtf8(buffer,0,0,entry.koreanUtf8,bp::PaletteColor::Neutral),
            "Korean frozen line renders with built-in glyph composition");
    }
}

void TestLanguageIsPresentationOnly() noexcept {
    bp::GameState game{};
    Check(bp::InitializeGameState(game,0x12A11CEu),"language hash fixture initializes");
    const std::uint32_t before=bp::GameStateHash(game);
    bp::SetTextLanguageEnglish(false);
    Check(std::strcmp(bp::LocalizedText("ENOUGH"),"ENOUGH")!=0,
        "KO selects frozen localized copy");
    bp::SetTextLanguageEnglish(true);
    Check(std::strcmp(bp::LocalizedText("ENOUGH"),"ENOUGH")==0,
        "EN selects frozen English copy");
    Check(bp::GameStateHash(game)==before,"language selection never changes game hash");
}

void TestOptionsAndCreditsBothLanguages() noexcept {
    std::uint32_t pixels[bp::kRenderWidth*bp::kRenderHeight]{};
    bp::PixelBuffer buffer{pixels,bp::kRenderWidth,bp::kRenderHeight,bp::kRenderWidth};
    bp::OptionsRuntime options{};
    bp::NarrativeRuntime credits{};
    credits.scene=bp::NarrativeScene::Credits;
    for (std::uint8_t english=0;english<2;++english) {
        bp::SetTextLanguageEnglish(english!=0);
        bp::InitializeOptionsRuntime(options,english!=0?(10u|(1u<<4u)):10u);
        bp::RenderOptionsRuntime(buffer,options);
        Check(LitPixels(pixels,bp::Color(bp::PaletteColor::Background))>500,
            "six-row options panel renders in selected language");
        bp::RenderNarrativeRuntime(buffer,credits);
        Check(LitPixels(pixels,bp::Color(bp::PaletteColor::Background))>500,
            "credits provenance panel renders in selected language");
    }
    bp::SetTextLanguageEnglish(true);
}

struct Runner {
    Runner() noexcept {
        bp::SetTextLanguageEnglish(true);
        TestFrozenCopyAndGlyphs();
        TestLanguageIsPresentationOnly();
        TestOptionsAndCreditsBothLanguages();
        bp::SetTextLanguageEnglish(true);
        if (gFailures!=0) {
            std::printf("T12_FAIL\tchecks=%u\tfailures=%u\n",gChecks,gFailures);
            std::exit(1);
        }
        std::printf("T12_PASS\tchecks=%u\tcopy=KO_EN\texternal_assets=0\n",gChecks);
    }
};

Runner gRunner;

}  // namespace
