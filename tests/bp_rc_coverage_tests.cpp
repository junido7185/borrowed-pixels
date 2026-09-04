#include "bp_act1.h"
#include "bp_act2.h"
#include "bp_act3.h"
#include "bp_game.h"
#include "bp_narrative.h"
#include "bp_precedent.h"

#include <cstdint>
#include <cstdio>
#include <cstdlib>

namespace {

std::uint32_t gChecks;
std::uint32_t gFailures;

void Check(bool condition,const char* name) noexcept {
    ++gChecks;
    if (!condition) { ++gFailures; std::printf("T13_FAIL\t%s\n",name); }
}

constexpr std::uint8_t Owner(bp::ClaimOwner owner) noexcept {
    return static_cast<std::uint8_t>(owner);
}

bp::InputFrame Input(bp::PlatformAction action,bool held=true) noexcept {
    bp::InputFrame input{};
    input.focused=1;
    input.device=bp::InputDevice::KeyboardMouse;
    input.mouseX=160;
    input.mouseY=90;
    input.actions[static_cast<std::uint8_t>(action)].pressed=1;
    input.actions[static_cast<std::uint8_t>(action)].held=held?1u:0u;
    return input;
}

bp::InputFrame ReplayInput(std::uint32_t seed,std::uint32_t tick) noexcept {
    bp::InputFrame input{};
    input.focused=1;
    input.device=bp::InputDevice::KeyboardMouse;
    input.tick=tick;
    const std::uint32_t value=seed*747796405u+tick*2891336453u+277803737u;
    input.moveX=(value&1u)!=0?bp::kInputUnit:-bp::kInputUnit;
    input.moveY=(value&2u)!=0?bp::kInputUnit:-bp::kInputUnit;
    input.aimX=(value&4u)!=0?bp::kInputUnit:-bp::kInputUnit;
    input.aimY=(value&8u)!=0?bp::kInputUnit:-bp::kInputUnit;
    return input;
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

bool TickChecked(bp::NarrativeRuntime& runtime,const bp::InputFrame& input) noexcept {
    return bp::TickNarrativeRuntime(runtime,input)
        &&bp::ClaimLedgerValid(runtime.claimLedger)
        &&bp::ClaimLedgerTotal(runtime.claimLedger)==bp::kLedgerTotal;
}

void TestThousandSeedReplay() noexcept {
    for (std::uint32_t seed=1;seed<=1000u;++seed) {
        bp::GameState first{};
        bp::GameState second{};
        bool same=bp::InitializeGameState(first,seed)&&bp::InitializeGameState(second,seed);
        for (std::uint8_t tick=0;tick<64&&same;++tick) {
            const bp::InputFrame input=ReplayInput(seed,tick);
            same=bp::TickGame(first,input)&&bp::TickGame(second,input)
                &&bp::GameStateHash(first)==bp::GameStateHash(second)
                &&bp::ClaimLedgerTotal(first.claimLedger)==bp::kLedgerTotal
                &&bp::PhysicalLedgerTotal(first.physicalLedger)==bp::kLedgerTotal;
        }
        Check(same,"1000-seed deterministic replay preserves both ledgers");
    }
}

void TestBossBounds() noexcept {
    for (std::uint8_t encounter=0;encounter<bp::kAct1EncounterCount;++encounter)
        Check(bp::ValidateAct1EncounterDef(bp::GetAct1EncounterDef(encounter)),
            "Act1 boss definition remains within frozen bounds");
    for (std::uint8_t encounter=0;encounter<bp::kAct2EncounterCount;++encounter)
        Check(bp::ValidateAct2EncounterDef(bp::GetAct2EncounterDef(encounter)),
            "Act2 boss definition remains within frozen bounds");
    for (std::uint8_t encounter=0;encounter<bp::kAct3EncounterCount;++encounter)
        Check(bp::ValidateAct3EncounterDef(bp::GetAct3EncounterDef(encounter)),
            "Act3 boss definition remains within frozen bounds");
}

bool ReachFullEnding(bp::FinalChoice choice,bp::EndingId ending) noexcept {
    bp::ClaimLedger claims=bp::MakeInitialClaimLedger();
    bp::NarrativeState narrative=CompleteNarrative();
    if (choice==bp::FinalChoice::Open)
        SetChoice(narrative,bp::EncounterId::Frame,bp::ChoiceId::Third);
    if (choice==bp::FinalChoice::TotalClaim) {
        constexpr bp::ChoiceId exact[9]={bp::ChoiceId::First,bp::ChoiceId::First,
            bp::ChoiceId::First,bp::ChoiceId::First,bp::ChoiceId::Third,
            bp::ChoiceId::First,bp::ChoiceId::First,bp::ChoiceId::Third,
            bp::ChoiceId::First};
        for (std::uint8_t index=0;index<9;++index)
            SetChoice(narrative,static_cast<bp::EncounterId>(index),exact[index]);
    }
    bp::NarrativeRuntime runtime{};
    if (!bp::InitializeFinalNarrative(runtime,claims,narrative)) return false;
    runtime.oneRevealTicks=bp::kOneRevealTicks;
    bp::PlatformAction selection=bp::PlatformAction::Confirm;
    if (choice==bp::FinalChoice::Override) selection=bp::PlatformAction::SelectUp;
    else if (choice==bp::FinalChoice::Open) selection=bp::PlatformAction::SelectRight;
    else if (choice==bp::FinalChoice::TotalClaim) selection=bp::PlatformAction::MoveDown;
    if (selection!=bp::PlatformAction::Confirm&&!TickChecked(runtime,Input(selection))) return false;
    if (!TickChecked(runtime,Input(bp::PlatformAction::Confirm))) return false;
    for (std::uint16_t tick=0;tick<bp::kFinalPreviewTicks;++tick)
        if (!TickChecked(runtime,bp::InputFrame{})) return false;
    for (std::uint16_t tick=0;tick<bp::kFinalConfirmTicks;++tick)
        if (!TickChecked(runtime,Input(bp::PlatformAction::Confirm))) return false;
    for (std::uint16_t tick=0;tick<1000&&runtime.scene!=bp::NarrativeScene::Ending;++tick)
        if (!TickChecked(runtime,bp::InputFrame{})) return false;
    if (runtime.scene!=bp::NarrativeScene::Ending||runtime.ending!=ending) return false;
    for (std::uint16_t tick=0;tick<bp::kEndingAutomaticTicks;++tick)
        if (!TickChecked(runtime,bp::InputFrame{})) return false;
    for (std::uint16_t tick=0;tick<bp::kCreditsTicks;++tick)
        if (!TickChecked(runtime,bp::InputFrame{})) return false;
    if (runtime.scene!=bp::NarrativeScene::Title) return false;
    return TickChecked(runtime,Input(bp::PlatformAction::SelectUp))
        &&runtime.scene==bp::NarrativeScene::AuditTrace;
}

void TestTitleToEndingReachability() noexcept {
    bp::ClaimLedger enoughClaims=bp::MakeInitialClaimLedger();
    enoughClaims.mass[Owner(bp::ClaimOwner::System)]-=144;
    enoughClaims.mass[Owner(bp::ClaimOwner::Player)]=144;
    bp::NarrativeRuntime enough{};
    bool enoughReachable=bp::InitializeEnoughNarrative(enough,enoughClaims,CompleteNarrative());
    for (std::uint16_t tick=0;tick<bp::kEnoughConfirmTicks&&enoughReachable;++tick)
        enoughReachable=TickChecked(enough,Input(bp::PlatformAction::Confirm));
    Check(enoughReachable&&enough.scene==bp::NarrativeScene::Ending
        &&enough.ending==bp::EndingId::Enough,"TITLE route reaches ENOUGH without ledger loss");
    Check(ReachFullEnding(bp::FinalChoice::Ratify,bp::EndingId::Rule),
        "TITLE route reaches RULE and Audit Trace");
    Check(ReachFullEnding(bp::FinalChoice::Override,bp::EndingId::Hand),
        "TITLE route reaches HAND and Audit Trace");
    Check(ReachFullEnding(bp::FinalChoice::Open,bp::EndingId::OpenLedger),
        "TITLE route reaches OPEN LEDGER and Audit Trace");
    Check(ReachFullEnding(bp::FinalChoice::TotalClaim,bp::EndingId::One),
        "TITLE route reaches ONE and Audit Trace");
}

struct Runner {
    Runner() noexcept {
        TestThousandSeedReplay();
        TestBossBounds();
        TestTitleToEndingReachability();
        if (gFailures!=0) {
            std::printf("T13_FAIL\tchecks=%u\tfailures=%u\n",gChecks,gFailures);
            std::exit(1);
        }
        std::printf("T13_PASS\tchecks=%u\tseeds=1000\tendings=5\ttuning=NONE\n",gChecks);
    }
};

Runner gRunner;

}  // namespace
