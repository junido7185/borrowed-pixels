#include "bp_narrative.h"

#include "bp_claim_ledger.h"
#include "bp_font.h"

#include <cstdint>

namespace bp {
namespace {

constexpr std::uint8_t Index(ClaimOwner owner) noexcept {
    return static_cast<std::uint8_t>(owner);
}

constexpr std::uint8_t Action(PlatformAction action) noexcept {
    return static_cast<std::uint8_t>(action);
}

bool Pressed(const InputFrame& input,PlatformAction action) noexcept {
    return input.actions[Action(action)].pressed!=0;
}

bool Held(const InputFrame& input,PlatformAction action) noexcept {
    return input.actions[Action(action)].held!=0;
}

ChoiceId ChoiceAt(const NarrativeState& narrative,EncounterId encounter) noexcept {
    return static_cast<ChoiceId>((narrative.choiceBits
        >>(static_cast<std::uint8_t>(encounter)*2u))&3u);
}

bool NarrativeChoicesValid(const NarrativeState& narrative) noexcept {
    for (std::uint8_t encounter=0;encounter<kSettlementEncounterCount;++encounter)
        if (((narrative.choiceBits>>(encounter*2u))&3u)>=3u) return false;
    return narrative.encounterIndex>=kSettlementEncounterCount;
}

void Centered(PixelBuffer buffer,std::int32_t y,const char* text,
    PaletteColor color=PaletteColor::Neutral,std::int32_t scale=1) noexcept {
    const std::int32_t width=MeasureLatin(text,scale);
    (void)DrawLatin(buffer,(kRenderWidth-width)/2,y,text,color,scale);
}

void SetScene(NarrativeRuntime& state,NarrativeScene scene) noexcept {
    state.scene=scene;
    state.sceneTicks=0;
    state.holdTicks=0;
}

constexpr std::uint8_t EndingBit(EndingId ending) noexcept {
    return static_cast<std::uint8_t>(1u<<static_cast<std::uint8_t>(ending));
}

void FixEnding(NarrativeRuntime& state,EndingId ending) noexcept {
    state.ending=ending;
    state.narrative.endingsSeen=static_cast<std::uint8_t>(
        state.narrative.endingsSeen|EndingBit(ending));
    if (ending==EndingId::Enough) state.vesselContinueAvailable=1;
    else {
        state.fullEndingReached=1;
        state.auditTraceUnlocked=1;
    }
    SetScene(state,NarrativeScene::Ending);
}

EndingId EndingForChoice(FinalChoice choice) noexcept {
    switch (choice) {
    case FinalChoice::Ratify: return EndingId::Rule;
    case FinalChoice::Override: return EndingId::Hand;
    case FinalChoice::Open: return EndingId::OpenLedger;
    case FinalChoice::TotalClaim: return EndingId::One;
    default: return EndingId::Rule;
    }
}

bool ChoiceAllowed(const NarrativeRuntime& state,FinalChoice choice) noexcept {
    if (choice==FinalChoice::Open) return state.openUnlocked!=0;
    if (choice==FinalChoice::TotalClaim) return state.oneUnlocked!=0
        && state.oneRevealTicks>=kOneRevealTicks;
    return static_cast<std::uint8_t>(choice)<static_cast<std::uint8_t>(FinalChoice::Count);
}

void CollapseNextClaimRow(NarrativeRuntime& state) noexcept {
    while (state.commitOwner<kClaimOwnerCount
        && state.commitOwner==Index(ClaimOwner::Player)) ++state.commitOwner;
    if (state.commitOwner>=kClaimOwnerCount) return;
    const ClaimOwner owner=static_cast<ClaimOwner>(state.commitOwner);
    const std::uint16_t mass=state.claimLedger.mass[state.commitOwner];
    if (mass!=0) (void)ClaimTransfer(state.claimLedger,owner,ClaimOwner::Player,mass);
    ++state.commitOwner;
}

void DrawFinalRows(PixelBuffer buffer,const NarrativeRuntime& state) noexcept {
    constexpr const char* titles[4]={"RATIFY THE RULE","KEEP THE HAND",
        "OPEN THE LEDGER","TOTAL CLAIM"};
    constexpr const char* gains[4]={"ONE RULE FOR EVERY CASE",
        "AN EXCEPTION MAY BE GRANTED","EVERY CLAIM MAY BE CHALLENGED",
        "EVERY TILE BECOMES YOUR CLAIM"};
    constexpr const char* costs[4]={"NO EXCEPTION ANSWERS BACK",
        "NO ONE MAY OVERRIDE YOU","NO SETTLEMENT STAYS FINAL",
        "NOTHING REMAINS TO MOVE THROUGH"};
    for (std::uint8_t row=0;row<4;++row) {
        if (row==3 && (state.oneUnlocked==0 || state.oneRevealTicks<kOneRevealTicks)) continue;
        const std::int32_t y=60+row*28;
        const bool selected=state.focused==static_cast<FinalChoice>(row);
        const bool enabled=ChoiceAllowed(state,static_cast<FinalChoice>(row));
        StrokeRect(buffer,RectI{46,y,228,24},selected?PaletteColor::Player:
            PaletteColor::LedgerGrid);
        (void)DrawLatin(buffer,52,y+3,titles[row],enabled?PaletteColor::Neutral:
            PaletteColor::UiInactive);
        const char* detail=!enabled&&row==2?"NO COMMON CLAIM EXISTS":
            ((state.sceneTicks/45u)&1u)==0?gains[row]:costs[row];
        (void)DrawLatin(buffer,52,y+13,detail,enabled?PaletteColor::UiInactive:
            PaletteColor::Warning);
    }
}

void DrawEnding(PixelBuffer buffer,const NarrativeRuntime& state) noexcept {
    constexpr const char* titles[5]={"ENOUGH","RULE","HAND","OPEN LEDGER","ONE"};
    constexpr const char* gains[5]={"YOU KEPT ENOUGH TO REMAIN",
        "EVERY CASE RECEIVED ONE ANSWER","YOU KEPT THE RIGHT TO OVERRIDE",
        "EVERY CLAIM COULD BE CHALLENGED","YOU OWNED EVERY TILE"};
    constexpr const char* costs[5]={"THE OLD LEDGER REMAINED",
        "NO ONE RECEIVED AN EXCEPTION","NO ONE COULD OVERRIDE YOU",
        "NO SETTLEMENT WAS FINAL","NOTHING REMAINED TO MOVE THROUGH"};
    const std::uint8_t ending=static_cast<std::uint8_t>(state.ending);
    Clear(buffer,PaletteColor::Background);
    DrawLedgerGrid(buffer);
    Centered(buffer,26,titles[ending],PaletteColor::Player,2);
    Centered(buffer,64,gains[ending]);
    Centered(buffer,78,costs[ending],PaletteColor::Warning);
    StrokeRect(buffer,RectI{40,100,240,30},PaletteColor::LedgerGrid);
    if (state.ending==EndingId::Enough) {
        StrokeRect(buffer,RectI{154,107,12,12},PaletteColor::Player);
        if (state.sceneTicks>=180) StrokeRect(buffer,RectI{152,105,16,16},PaletteColor::UiInactive);
    } else if (state.ending==EndingId::Rule) {
        for (std::int32_t x=48;x<276;x+=16) DrawLine(buffer,x,102,x,127,PaletteColor::LedgerGrid);
    } else if (state.ending==EndingId::Hand) {
        DrawLine(buffer,70,104,62,104,PaletteColor::Player);
        DrawLine(buffer,62,104,62,126,PaletteColor::Player);
        DrawLine(buffer,250,104,258,104,PaletteColor::Player);
        DrawLine(buffer,258,104,258,126,PaletteColor::Player);
    } else if (state.ending==EndingId::OpenLedger) {
        for (std::int32_t x=56;x<268;x+=42)
            FillRect(buffer,RectI{x+((state.sceneTicks/30u)%8u),112,3,3},PaletteColor::Neutral);
    } else {
        FillRect(buffer,RectI{40,100,240,30},PaletteColor::Player);
    }
    Centered(buffer,144,state.sceneTicks<kEndingMinimumTicks?"THE LEDGER IS FIXING":
        "CONTINUE",state.sceneTicks<kEndingMinimumTicks?PaletteColor::UiInactive:
        PaletteColor::Neutral);
}

}  // namespace

bool OpenLedgerUnlocked(const ClaimLedger& claims,const NarrativeState& narrative) noexcept {
    return ClaimLedgerValid(claims)
        && (claims.mass[Index(ClaimOwner::Common)]>=64
            || ChoiceAt(narrative,EncounterId::Frame)==ChoiceId::Third);
}

bool TotalClaimUnlocked(const NarrativeState& narrative) noexcept {
    constexpr ChoiceId required[kSettlementEncounterCount]={
        ChoiceId::First,ChoiceId::First,ChoiceId::First,ChoiceId::First,
        ChoiceId::Third,ChoiceId::First,ChoiceId::First,ChoiceId::Third,
        ChoiceId::First};
    if (!NarrativeChoicesValid(narrative)) return false;
    for (std::uint8_t encounter=0;encounter<kSettlementEncounterCount;++encounter)
        if (ChoiceAt(narrative,static_cast<EncounterId>(encounter))!=required[encounter])
            return false;
    return true;
}

bool InitializeFinalNarrative(NarrativeRuntime& state,const ClaimLedger& claims,
    const NarrativeState& narrative) noexcept {
    if (!ClaimLedgerValid(claims) || !NarrativeChoicesValid(narrative)) return false;
    state=NarrativeRuntime{};
    state.claimLedger=claims;
    state.narrative=narrative;
    state.scene=NarrativeScene::FinalAllocation;
    state.ending=EndingId::Rule;
    state.focused=FinalChoice::Ratify;
    state.committed=FinalChoice::Ratify;
    state.openUnlocked=OpenLedgerUnlocked(claims,narrative)?1u:0u;
    state.oneUnlocked=TotalClaimUnlocked(narrative)?1u:0u;
    state.vesselContinueAvailable=(narrative.endingsSeen&EndingBit(EndingId::Enough))!=0?1u:0u;
    state.fullEndingReached=(narrative.endingsSeen&0x1eu)!=0?1u:0u;
    state.auditTraceUnlocked=state.fullEndingReached;
    return true;
}

bool InitializeEnoughNarrative(NarrativeRuntime& state,const ClaimLedger& claims,
    const NarrativeState& narrative) noexcept {
    if (!ClaimLedgerValid(claims) || claims.mass[Index(ClaimOwner::Player)]!=144) return false;
    state=NarrativeRuntime{};
    state.claimLedger=claims;
    state.narrative=narrative;
    state.narrative.endingsSeen=static_cast<std::uint8_t>(
        state.narrative.endingsSeen&~EndingBit(EndingId::Enough));
    state.scene=NarrativeScene::EnoughConfirm;
    state.ending=EndingId::Enough;
    state.focused=FinalChoice::Ratify;
    state.committed=FinalChoice::Ratify;
    return true;
}

bool InitializeTitleNarrative(NarrativeRuntime& state,const ClaimLedger& claims,
    const NarrativeState& narrative,std::uint32_t saveFlags) noexcept {
    if (!ClaimLedgerValid(claims)) return false;
    state=NarrativeRuntime{};
    state.claimLedger=claims;
    state.narrative=narrative;
    state.scene=NarrativeScene::Title;
    state.ending=EndingId::Enough;
    state.focused=FinalChoice::Ratify;
    state.committed=FinalChoice::Ratify;
    state.vesselContinueAvailable=(saveFlags&(1u<<1u))!=0?1u:0u;
    state.fullEndingReached=(saveFlags&(1u<<2u))!=0?1u:0u;
    state.auditTraceUnlocked=(saveFlags&(1u<<3u))!=0?1u:0u;
    return true;
}

bool ApplyFinalChoice(NarrativeRuntime& state,FinalChoice choice) noexcept {
    if (!ClaimLedgerValid(state.claimLedger) || !ChoiceAllowed(state,choice)) return false;
    NarrativeRuntime candidate=state;
    if (choice==FinalChoice::TotalClaim) {
        for (std::uint8_t owner=0;owner<kClaimOwnerCount;++owner) {
            if (owner==Index(ClaimOwner::Player)) continue;
            const std::uint16_t mass=candidate.claimLedger.mass[owner];
            if (mass!=0 && !ClaimTransfer(candidate.claimLedger,
                    static_cast<ClaimOwner>(owner),ClaimOwner::Player,mass)) return false;
        }
        if (candidate.claimLedger.mass[Index(ClaimOwner::Player)]!=kLedgerTotal) return false;
    }
    candidate.committed=choice;
    FixEnding(candidate,EndingForChoice(choice));
    state=candidate;
    return true;
}

bool TickNarrativeRuntime(NarrativeRuntime& state,const InputFrame& input) noexcept {
    if (!ClaimLedgerValid(state.claimLedger) || state.scene==NarrativeScene::Fatal) return false;
    ++state.globalTick;
    switch (state.scene) {
    case NarrativeScene::EnoughConfirm:
        ++state.sceneTicks;
        if (Held(input,PlatformAction::Confirm)) {
            if (++state.holdTicks>=kEnoughConfirmTicks) FixEnding(state,EndingId::Enough);
        } else state.holdTicks=0;
        break;
    case NarrativeScene::FinalAllocation: {
        ++state.sceneTicks;
        if (state.oneUnlocked!=0 && state.oneRevealTicks<kOneRevealTicks) ++state.oneRevealTicks;
        FinalChoice requested=state.focused;
        if (Pressed(input,PlatformAction::SelectLeft)) requested=FinalChoice::Ratify;
        else if (Pressed(input,PlatformAction::SelectUp)) requested=FinalChoice::Override;
        else if (Pressed(input,PlatformAction::SelectRight)) requested=FinalChoice::Open;
        else if (Pressed(input,PlatformAction::MoveDown)) requested=FinalChoice::TotalClaim;
        if (requested!=state.focused && ChoiceAllowed(state,requested)) {
            state.focused=requested;
            state.sceneTicks=0;
        }
        if (Pressed(input,PlatformAction::Confirm) && ChoiceAllowed(state,state.focused)) {
            state.committed=state.focused;
            SetScene(state,NarrativeScene::FinalPreview);
        }
        break;
    }
    case NarrativeScene::FinalPreview:
        if (++state.sceneTicks>=kFinalPreviewTicks) SetScene(state,NarrativeScene::FinalConfirm);
        break;
    case NarrativeScene::FinalConfirm:
        if (!Held(input,PlatformAction::Confirm)) {
            state.holdTicks=0;
        } else if (++state.holdTicks>=kFinalConfirmTicks) {
            state.commitOwner=0;
            SetScene(state,NarrativeScene::FinalCommit);
        }
        if (Pressed(input,PlatformAction::Pause)) SetScene(state,NarrativeScene::FinalAllocation);
        break;
    case NarrativeScene::FinalCommit:
        ++state.sceneTicks;
        if (state.committed==FinalChoice::TotalClaim) {
            if (state.sceneTicks%kFinalCommitTicks==0) CollapseNextClaimRow(state);
            if (state.commitOwner>=kClaimOwnerCount) {
                if (state.claimLedger.mass[Index(ClaimOwner::Player)]!=kLedgerTotal) return false;
                FixEnding(state,EndingId::One);
            }
        } else if (state.sceneTicks>=kFinalCommitTicks) {
            FixEnding(state,EndingForChoice(state.committed));
        }
        break;
    case NarrativeScene::Ending:
        ++state.sceneTicks;
        if (state.sceneTicks>=kEndingAutomaticTicks
            || (state.sceneTicks>=kEndingMinimumTicks&&Pressed(input,PlatformAction::Confirm)))
            SetScene(state,NarrativeScene::Credits);
        break;
    case NarrativeScene::Credits:
        if (++state.sceneTicks>=kCreditsTicks || Pressed(input,PlatformAction::Confirm))
            SetScene(state,NarrativeScene::Title);
        break;
    case NarrativeScene::Title:
        ++state.sceneTicks;
        if (state.auditTraceUnlocked!=0 && Pressed(input,PlatformAction::SelectUp))
            SetScene(state,NarrativeScene::AuditTrace);
        else if (state.vesselContinueAvailable!=0 && Pressed(input,PlatformAction::SelectRight))
            state.routeRequest=NarrativeRouteRequest::ContinueVessel;
        else if (Pressed(input,PlatformAction::Confirm))
            state.routeRequest=NarrativeRouteRequest::NewRun;
        break;
    case NarrativeScene::AuditTrace:
        if (++state.sceneTicks>=kAuditSummaryTicks) {
            state.sceneTicks=0;
            state.auditIndex=static_cast<std::uint8_t>((state.auditIndex+1u)%kSettlementEncounterCount);
        }
        if (Pressed(input,PlatformAction::Pause)) SetScene(state,NarrativeScene::Title);
        break;
    case NarrativeScene::Fatal: return false;
    }
    return ClaimLedgerValid(state.claimLedger);
}

void RenderNarrativeRuntime(PixelBuffer buffer,const NarrativeRuntime& state) noexcept {
    Clear(buffer,PaletteColor::Background);
    DrawLedgerGrid(buffer);
    switch (state.scene) {
    case NarrativeScene::EnoughConfirm:
        Centered(buffer,28,"ENOUGH",PaletteColor::Warning,2);
        Centered(buffer,64,"YOU WILL REMAIN");
        Centered(buffer,78,"THE LEDGER WILL NOT CHANGE");
        Centered(buffer,92,"END THIS RUN?",PaletteColor::Warning);
        StrokeRect(buffer,RectI{154,116,12,12},PaletteColor::Player);
        break;
    case NarrativeScene::FinalAllocation:
    case NarrativeScene::FinalPreview:
    case NarrativeScene::FinalConfirm:
    case NarrativeScene::FinalCommit:
        Centered(buffer,28,"FINAL ALLOCATION",PaletteColor::Warning);
        DrawFinalRows(buffer,state);
        if (state.scene==NarrativeScene::FinalConfirm)
            Centered(buffer,166,"HOLD TO COMMIT",PaletteColor::Player);
        break;
    case NarrativeScene::Ending: DrawEnding(buffer,state); break;
    case NarrativeScene::Credits:
        Centered(buffer,30,"BORROWED PIXELS",PaletteColor::Player,2);
        Centered(buffer,56,"CREATOR: DEV_SLIME");
        Centered(buffer,69,"AI-ASSISTED DESIGN AND IMPLEMENTATION");
        Centered(buffer,82,"HUMAN-DIRECTED AND REVIEWED");
        Centered(buffer,95,"FONT: BUILT-IN 5X7 / COMPOSED HANGUL");
        Centered(buffer,108,"AUDIO: RUNTIME SYNTHESIS");
        Centered(buffer,121,"EXTERNAL ASSETS: 0");
        Centered(buffer,134,"LICENSES: DOCS / RIGHTS_LOG / PROVENANCE");
        Centered(buffer,152,"TOTAL TILE: 1440",PaletteColor::Warning);
        break;
    case NarrativeScene::Title:
        Centered(buffer,34,"BORROWED PIXELS",PaletteColor::Neutral,2);
        Centered(buffer,88,"PRESS ENTER TO BORROW",PaletteColor::Warning);
        if (state.vesselContinueAvailable!=0)
            Centered(buffer,112,"RIGHT // CONTINUE FROM VESSEL",PaletteColor::Player);
        if (state.auditTraceUnlocked!=0)
            Centered(buffer,128,"UP // AUDIT TRACE",PaletteColor::Neutral);
        break;
    case NarrativeScene::AuditTrace: {
        constexpr const char* cases[9]={"PATCH","PAIR","VESSEL","KEEPER","ADAPTER",
            "FRAME","INDEX","WITNESS","CURRENT"};
        constexpr const char* rules[9]={"ORIGIN PRIORITY","LINK","MINIMUM","BOUNDARY",
            "OUTPUT","FIELD","METRIC","RECORD","RETURN"};
        Centered(buffer,18,"CASE | TRANSFER | RULE USED",PaletteColor::Warning);
        (void)DrawLatin(buffer,28,64,cases[state.auditIndex]);
        (void)DrawLatin(buffer,112,64,"CLAIM MOVE",PaletteColor::Player);
        (void)DrawLatin(buffer,206,64,rules[state.auditIndex]);
        const std::int32_t x=40+(state.sceneTicks*220/kAuditSummaryTicks);
        StrokeRect(buffer,RectI{x,100,8,8},PaletteColor::Neutral);
        Centered(buffer,152,"ESC // TITLE",PaletteColor::UiInactive);
        break;
    }
    case NarrativeScene::Fatal:
        Centered(buffer,86,"LEDGER ERROR",PaletteColor::Warning);
        break;
    }
}

std::uint32_t NarrativeRuntimeHash(const NarrativeRuntime& state) noexcept {
    std::uint32_t hash=2166136261u;
    const auto add=[&hash](std::uint32_t value) noexcept {
        for (std::uint8_t i=0;i<4;++i) { hash^=(value>>(i*8u))&0xffu; hash*=16777619u; }
    };
    for (std::uint16_t mass:state.claimLedger.mass) add(mass);
    add(state.narrative.choiceBits); add(state.narrative.adjudicatedMass);
    add(state.narrative.encounterIndex); add(state.narrative.role);
    for (std::uint8_t status:state.narrative.entityStatus) add(status);
    add(state.narrative.endingsSeen); add(static_cast<std::uint8_t>(state.scene));
    add(static_cast<std::uint8_t>(state.ending)); add(static_cast<std::uint8_t>(state.focused));
    add(static_cast<std::uint8_t>(state.committed)); add(state.globalTick); add(state.sceneTicks);
    add(state.holdTicks); add(state.oneRevealTicks); add(state.commitOwner); add(state.auditIndex);
    add(state.oneUnlocked); add(state.openUnlocked); add(state.vesselContinueAvailable);
    add(state.auditTraceUnlocked); add(state.fullEndingReached);
    add(static_cast<std::uint8_t>(state.routeRequest));
    return hash;
}

bool ObserveSaveBoundary(SaveBoundaryObserver& observer,std::uint8_t runtimeStage,
    std::uint8_t encounterIndex,std::uint8_t scene,std::uint32_t options,
    bool authoritativeBoundary,bool forbiddenState) noexcept {
    const bool changed=observer.initialized==0 || observer.lastRuntimeStage!=runtimeStage
        || observer.lastEncounterIndex!=encounterIndex || observer.lastScene!=scene
        || observer.lastOptions!=options;
    observer.lastRuntimeStage=runtimeStage;
    observer.lastEncounterIndex=encounterIndex;
    observer.lastScene=scene;
    observer.lastOptions=options;
    observer.initialized=1;
    return changed&&authoritativeBoundary&&!forbiddenState;
}

}  // namespace bp
