#include "bp_claim_ledger.h"
#include "bp_narrative.h"
#include "bp_save.h"

#include <cstdint>
#include <cstdio>
#include <cstdlib>

namespace {

std::uint32_t gChecks;
std::uint32_t gFailures;

void Check(bool condition,const char* name) noexcept {
    ++gChecks;
    if (!condition) { ++gFailures; std::printf("T10_FAIL\t%s\n",name); }
}

constexpr std::uint8_t Owner(bp::ClaimOwner owner) noexcept {
    return static_cast<std::uint8_t>(owner);
}

void SetChoice(bp::NarrativeState& narrative,bp::EncounterId encounter,
    bp::ChoiceId choice) noexcept {
    const std::uint32_t shift=static_cast<std::uint8_t>(encounter)*2u;
    narrative.choiceBits=(narrative.choiceBits&~(3u<<shift))
        |(static_cast<std::uint32_t>(choice)<<shift);
}

bp::NarrativeState CompleteNarrative(bp::ChoiceId choice=bp::ChoiceId::Second) noexcept {
    bp::NarrativeState narrative{};
    for (std::uint8_t encounter=0;encounter<bp::kSettlementEncounterCount;++encounter)
        SetChoice(narrative,static_cast<bp::EncounterId>(encounter),choice);
    narrative.encounterIndex=bp::kSettlementEncounterCount;
    return narrative;
}

bp::InputFrame Confirm(bool pressed=true) noexcept {
    bp::InputFrame input{};
    input.focused=1;
    input.actions[static_cast<std::uint8_t>(bp::PlatformAction::Confirm)].held=1;
    input.actions[static_cast<std::uint8_t>(bp::PlatformAction::Confirm)].pressed=pressed?1u:0u;
    return input;
}

void TestPredicatesAndEndings() noexcept {
    bp::ClaimLedger claims=bp::MakeInitialClaimLedger();
    bp::NarrativeState ordinary=CompleteNarrative();
    Check(!bp::OpenLedgerUnlocked(claims,ordinary),"OPEN locked without Common64 or FRAME OPEN");
    claims.mass[Owner(bp::ClaimOwner::Common)]=64;
    claims.mass[Owner(bp::ClaimOwner::System)]=256;
    Check(bp::OpenLedgerUnlocked(claims,ordinary),"OPEN unlocks at exact Common64");
    claims=bp::MakeInitialClaimLedger();
    SetChoice(ordinary,bp::EncounterId::Frame,bp::ChoiceId::Third);
    Check(bp::OpenLedgerUnlocked(claims,ordinary),"FRAME third independently unlocks OPEN");

    bp::NarrativeState one=CompleteNarrative();
    constexpr bp::ChoiceId exact[9]={bp::ChoiceId::First,bp::ChoiceId::First,
        bp::ChoiceId::First,bp::ChoiceId::First,bp::ChoiceId::Third,
        bp::ChoiceId::First,bp::ChoiceId::First,bp::ChoiceId::Third,
        bp::ChoiceId::First};
    for (std::uint8_t encounter=0;encounter<9;++encounter)
        SetChoice(one,static_cast<bp::EncounterId>(encounter),exact[encounter]);
    Check(bp::TotalClaimUnlocked(one),"ONE exact nine-choice predicate unlocks");
    for (std::uint8_t encounter=0;encounter<9;++encounter) {
        bp::NarrativeState wrong=one;
        SetChoice(wrong,static_cast<bp::EncounterId>(encounter),
            exact[encounter]==bp::ChoiceId::First?bp::ChoiceId::Second:bp::ChoiceId::First);
        Check(!bp::TotalClaimUnlocked(wrong),"ONE rejects every single-choice deviation");
    }

    bp::NarrativeRuntime final{};
    Check(bp::InitializeFinalNarrative(final,claims,one)
        && final.oneUnlocked!=0,"final narrative exposes exact ONE row");
    final.oneRevealTicks=bp::kOneRevealTicks;
    Check(bp::ApplyFinalChoice(final,bp::FinalChoice::TotalClaim)
        && final.ending==bp::EndingId::One
        && final.claimLedger.mass[Owner(bp::ClaimOwner::Player)]==1440
        && bp::ClaimLedgerTotal(final.claimLedger)==1440,
        "ONE final claim is exactly Player1440 while total stays1440");
    for (std::uint8_t owner=1;owner<bp::kClaimOwnerCount;++owner)
        Check(final.claimLedger.mass[owner]==0,"ONE clears every non-player claim row");
    Check((final.narrative.endingsSeen&(1u<<4u))!=0
        && final.fullEndingReached!=0&&final.auditTraceUnlocked!=0,
        "ONE fixes exact ending bit and unlocks Audit Trace");

    const bp::FinalChoice choices[3]={bp::FinalChoice::Ratify,
        bp::FinalChoice::Override,bp::FinalChoice::Open};
    const bp::EndingId endings[3]={bp::EndingId::Rule,bp::EndingId::Hand,
        bp::EndingId::OpenLedger};
    for (std::uint8_t index=0;index<3;++index) {
        bp::NarrativeRuntime route{};
        bp::NarrativeState routeNarrative=one;
        if (choices[index]==bp::FinalChoice::Open)
            SetChoice(routeNarrative,bp::EncounterId::Frame,bp::ChoiceId::Third);
        Check(bp::InitializeFinalNarrative(route,claims,routeNarrative)
            && bp::ApplyFinalChoice(route,choices[index])
            && route.ending==endings[index]
            && (route.narrative.endingsSeen&(1u<<static_cast<std::uint8_t>(endings[index])))!=0,
            "full ending choice fixes exact ending id/bit");
    }
}

void TestEnoughTimingAndRoutes() noexcept {
    bp::ClaimLedger claims=bp::MakeInitialClaimLedger();
    claims.mass[Owner(bp::ClaimOwner::Player)]=144;
    bp::NarrativeState narrative{};
    bp::NarrativeRuntime enough{};
    Check(bp::InitializeEnoughNarrative(enough,claims,narrative),
        "ENOUGH initializes only at Player144");
    for (std::uint16_t tick=0;tick<59;++tick)
        Check(bp::TickNarrativeRuntime(enough,Confirm(tick==0)),"ENOUGH confirmation advances");
    Check(enough.scene==bp::NarrativeScene::EnoughConfirm,
        "ENOUGH cannot fix before exact 60 held ticks");
    Check(bp::TickNarrativeRuntime(enough,Confirm(false))
        && enough.scene==bp::NarrativeScene::Ending
        && enough.ending==bp::EndingId::Enough
        && enough.vesselContinueAvailable!=0
        && enough.auditTraceUnlocked==0
        && (enough.narrative.endingsSeen&1u)!=0,
        "ENOUGH exact tick60 fixes bit and only Vessel continue");
    for (std::uint16_t tick=0;tick<359;++tick)
        Check(bp::TickNarrativeRuntime(enough,bp::InputFrame{}),"ending minimum advances");
    Check(enough.scene==bp::NarrativeScene::Ending,"ending skip disabled before 360");
    Check(bp::TickNarrativeRuntime(enough,Confirm())
        && enough.scene==bp::NarrativeScene::Credits,"ending permits continue at tick360");
    for (std::uint16_t tick=0;tick<bp::kCreditsTicks;++tick)
        Check(bp::TickNarrativeRuntime(enough,bp::InputFrame{}),"credits deterministic duration");
    Check(enough.scene==bp::NarrativeScene::Title,"credits return to title at tick720");
}

void TestSaveHelpersObserverAndRender() noexcept {
    const bp::ClaimLedger claims=bp::MakeInitialClaimLedger();
    const bp::NarrativeState narrative=CompleteNarrative();
    const bp::SaveData save=bp::MakeSaveData(claims,narrative,0x1fu,0x3ffu,
        0x12345678u,4567u,1u);
    Check((save.flags&(1u<<4u))==0&&save.claimLedger.mass[0]==claims.mass[0]
        &&save.choiceBits==narrative.choiceBits,"MakeSaveData strips preview and copies frozen fields");
    std::uint8_t bytes[bp::kSaveV2Bytes]{};
    Check(bp::SerializeSaveV2(save,bytes),"T10 SaveV2 remains exactly serializable96B");

    bp::SaveBoundaryObserver observer{};
    Check(!bp::ObserveSaveBoundary(observer,1,0,1,0,false,false),
        "non-boundary does not write");
    Check(bp::ObserveSaveBoundary(observer,1,0,2,0,true,false),
        "new authoritative boundary writes once");
    Check(!bp::ObserveSaveBoundary(observer,1,0,2,0,true,false),
        "unchanged boundary never writes twice");
    Check(!bp::ObserveSaveBoundary(observer,1,0,3,0,true,true),
        "preview or hit-freeze never writes");
    Check(bp::ObserveSaveBoundary(observer,1,0,3,1,true,false),
        "option change is a distinct save boundary");

    bp::NarrativeRuntime state{};
    Check(bp::InitializeFinalNarrative(state,claims,narrative),"render fixture initializes");
    std::uint32_t pixels[bp::kRenderWidth*bp::kRenderHeight]{};
    bp::PixelBuffer buffer{pixels,bp::kRenderWidth,bp::kRenderHeight,bp::kRenderWidth};
    bp::RenderNarrativeRuntime(buffer,state);
    std::uint32_t visible=0;
    for (std::uint32_t pixel:pixels) {
        Check(bp::IsPaletteColor(pixel),"T10 render uses frozen palette only");
        if (pixel!=bp::Color(bp::PaletteColor::Background)) ++visible;
    }
    Check(visible>100,"final allocation renders visible authoritative rows");
    const std::uint32_t hash=bp::NarrativeRuntimeHash(state);
    ++state.sceneTicks;
    Check(bp::NarrativeRuntimeHash(state)!=hash,"narrative hash includes future timing state");
}

struct Runner {
    Runner() noexcept {
        TestPredicatesAndEndings();
        TestEnoughTimingAndRoutes();
        TestSaveHelpersObserverAndRender();
        if (gFailures!=0) {
            std::printf("T10_FAIL\tchecks=%u\tfailures=%u\n",gChecks,gFailures);
            std::exit(1);
        }
        std::printf("T10_PASS\tchecks=%u\tendings=5\tsave_bytes=96\n",gChecks);
    }
};

Runner gRunner;

}  // namespace
