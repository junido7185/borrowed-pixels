#include "bp_claim_ledger.h"
#include "bp_physical_ledger.h"
#include "bp_precedent.h"

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace {

std::uint32_t gChecks;
std::uint32_t gFailures;

void Check(bool condition,const char* name) noexcept {
    ++gChecks;
    if (!condition) {
        ++gFailures;
        std::printf("T09_FAIL\t%s\n",name);
    }
}

constexpr std::uint8_t Account(bp::PhysicalAccount value) noexcept {
    return static_cast<std::uint8_t>(value);
}

bp::NarrativeState NarrativeForCode(std::uint32_t code) noexcept {
    bp::NarrativeState narrative{};
    for (std::uint8_t encounter=0;encounter<bp::kSettlementEncounterCount;++encounter) {
        const std::uint32_t choice=code%3u;
        code/=3u;
        narrative.choiceBits|=choice<<(encounter*2u);
    }
    narrative.encounterIndex=bp::kSettlementEncounterCount;
    return narrative;
}

void SetChoice(bp::NarrativeState& narrative,bp::EncounterId encounter,
    bp::ChoiceId choice) noexcept {
    const std::uint32_t shift=static_cast<std::uint8_t>(encounter)*2u;
    narrative.choiceBits=(narrative.choiceBits&~(3u<<shift))
        |(static_cast<std::uint32_t>(choice)<<shift);
}

bp::InputFrame Idle() noexcept {
    bp::InputFrame input{};
    input.focused=1;
    input.device=bp::InputDevice::KeyboardMouse;
    input.mouseX=48;
    input.mouseY=92;
    return input;
}

bp::InputFrame ActionInput(bp::PlatformAction action) noexcept {
    bp::InputFrame input=Idle();
    input.actions[static_cast<std::uint8_t>(action)].pressed=1;
    input.actions[static_cast<std::uint8_t>(action)].held=1;
    return input;
}

std::uint8_t ProfileIndex(const bp::PolicyProfile& profile) noexcept {
    return static_cast<std::uint8_t>(profile.identity*27u+profile.distribution*9u
        +profile.governance*3u+profile.memory);
}

void DamageOpponentTo(bp::PrecedentRuntime& state,std::uint16_t shell) noexcept {
    const std::uint16_t before=state.movement.physicalLedger.mass[
        Account(bp::PhysicalAccount::OpponentShell)];
    if (before<=shell) return;
    const std::uint16_t amount=static_cast<std::uint16_t>(before-shell);
    Check(bp::PhysicalTransfer(state.movement.physicalLedger,
        bp::PhysicalAccount::OpponentShell,bp::PhysicalAccount::Neutral,amount),
        "PRECEDENT phase fixture transfers opponent shell");
    bp::NeutralSlot& neutral=state.combat.neutrals[state.neutrals[0].index];
    neutral.mass=static_cast<std::uint16_t>(neutral.mass+amount);
    neutral.structureMass=static_cast<std::uint16_t>(neutral.structureMass+amount);
    Check(bp::CombatStateValid(state.combat,state.movement.physicalLedger),
        "PRECEDENT phase fixture remains valid");
}

void TestPolicyExhaustive(std::uint32_t (&representative)[81]) noexcept {
    bool seen[81]{};
    std::uint32_t unique=0;
    for (std::uint32_t code=0;code<19683u;++code) {
        const bp::NarrativeState narrative=NarrativeForCode(code);
        bp::PolicyDecision decision{};
        Check(bp::DerivePolicyDecision(narrative,decision),
            "all 3^9 choices derive a policy");
        const bool valid=decision.profile.identity<3 && decision.profile.distribution<3
            && decision.profile.governance<3 && decision.profile.memory<3;
        Check(valid,"derived policy axes are valid");
        if (!valid) continue;
        const std::uint8_t index=ProfileIndex(decision.profile);
        if (!seen[index]) { seen[index]=true; representative[index]=code; ++unique; }
        const bp::PolicyAuditView audit=bp::BuildPolicyAuditView(decision);
        Check(audit.rows[0].subject!=nullptr && audit.rows[0].choice!=nullptr
            && audit.rows[0].rule!=nullptr,"audit rows derive from same decision");
    }
    Check(unique==81,"exactly 81 unique PolicyProfiles");
    for (bool present:seen) Check(present,"every PolicyProfile is reachable");

    bp::NarrativeState tie=NarrativeForCode(0);
    SetChoice(tie,bp::EncounterId::Patch,bp::ChoiceId::First);
    SetChoice(tie,bp::EncounterId::Pair,bp::ChoiceId::Second);
    SetChoice(tie,bp::EncounterId::Keeper,bp::ChoiceId::First);
    SetChoice(tie,bp::EncounterId::Adapter,bp::ChoiceId::First);
    SetChoice(tie,bp::EncounterId::Index,bp::ChoiceId::First);
    SetChoice(tie,bp::EncounterId::Frame,bp::ChoiceId::First);
    SetChoice(tie,bp::EncounterId::Current,bp::ChoiceId::Second);
    SetChoice(tie,bp::EncounterId::Witness,bp::ChoiceId::Third);
    bp::PolicyDecision decision{};
    Check(bp::DerivePolicyDecision(tie,decision),"explicit tie fixture derives");
    Check(decision.profile.identity==static_cast<std::uint8_t>(bp::IdentityPolicy::Continuity)
        && decision.decisiveEncounter[0]==bp::EncounterId::Pair,
        "IDENTITY tie uses later PAIR");
    Check(decision.profile.distribution==static_cast<std::uint8_t>(bp::DistributionPolicy::Utility)
        && decision.decisiveEncounter[1]==bp::EncounterId::Index,
        "DISTRIBUTION tie uses later INDEX");
    Check(decision.profile.governance==static_cast<std::uint8_t>(bp::GovernancePolicy::Rule)
        && decision.decisiveEncounter[2]==bp::EncounterId::Current,
        "GOVERNANCE tie uses later CURRENT");
    const bp::PolicyAuditView audit=bp::BuildPolicyAuditView(decision);
    Check(std::strcmp(audit.rows[3].subject,"CASE // HASH")==0
        && std::strcmp(audit.rows[3].choice,"ERASE")==0
        && std::strcmp(audit.rows[3].rule,"NO CASE MAY SPEAK")==0,
        "MEMORY NONE uses exact anonymized CASE semantics");
}

void TestAllProfiles(const std::uint32_t (&representative)[81]) noexcept {
    for (std::uint8_t profile=0;profile<81;++profile) {
        const bp::NarrativeState narrative=NarrativeForCode(representative[profile]);
        bp::PrecedentRuntime state{};
        Check(bp::InitializePrecedentRuntime(state,bp::MakeInitialClaimLedger(),narrative),
            "all 81 PRECEDENT runtimes initialize");
        Check(bp::PopulatePrecedentBattlefield(state),
            "all 81 PRECEDENT battlefields populate");
        Check(ProfileIndex(state.policy.profile)==profile,
            "initialized battlefield retains exact profile");
        Check(bp::PhysicalLedgerTotal(state.movement.physicalLedger)==bp::kLedgerTotal
            && bp::ClaimLedgerTotal(state.claimLedger)==bp::kLedgerTotal,
            "all profiles retain both exact 1440 ledgers");
        DamageOpponentTo(state,80);
        bool live=true;
        for (std::uint16_t tick=0;tick<720 && live;++tick)
            live=bp::TickPrecedentRuntime(state,Idle());
        if (!live || state.scene==bp::PrecedentScene::Fatal)
            std::printf("T09_PROFILE_FAIL\tprofile=%u\ttick=%u\taction=%u\tphase=%u"
                "\terror=%u\tps=%u\tos=%u\tneutral=%u\tprojectile=%u\n",
                profile,state.encounterTick,static_cast<unsigned>(state.action),state.phase,
                static_cast<unsigned>(state.combat.error),
                state.movement.physicalLedger.mass[Account(bp::PhysicalAccount::PlayerShell)],
                state.movement.physicalLedger.mass[Account(bp::PhysicalAccount::OpponentShell)],
                state.movement.physicalLedger.mass[Account(bp::PhysicalAccount::Neutral)],
                state.movement.physicalLedger.mass[Account(bp::PhysicalAccount::Projectile)]);
        Check(live && state.scene!=bp::PrecedentScene::Fatal,
            "all 81 profiles have no deterministic idle softlock");
    }
}

void TestAuditAndDeterminism() noexcept {
    const bp::NarrativeState narrative=NarrativeForCode(1729);
    bp::PrecedentRuntime state{};
    Check(bp::InitializePrecedentRuntime(state,bp::MakeInitialClaimLedger(),narrative),
        "PRECEDENT audit runtime initializes");
    for (std::uint8_t tick=0;tick<59;++tick)
        Check(bp::TickPrecedentRuntime(state,Idle()),"RoleAssign advances safely");
    Check(state.scene==bp::PrecedentScene::RoleAssign && state.sceneTicks==59,
        "RoleAssign holds exactly through tick59");
    Check(bp::TickPrecedentRuntime(state,Idle())
        && state.scene==bp::PrecedentScene::PolicyAudit && state.auditRow==0,
        "RoleAssign transitions at exact tick60");
    bp::InputFrame confirm=Idle();
    confirm.actions[static_cast<std::uint8_t>(bp::PlatformAction::Confirm)].pressed=1;
    for (std::uint8_t tick=0;tick<35;++tick)
        Check(bp::TickPrecedentRuntime(state,Idle()),"audit reveal advances");
    Check(bp::TickPrecedentRuntime(state,confirm) && state.auditRow==1,
        "audit confirm advances at exact reveal tick36");
    for (std::uint8_t row=1;row<4;++row) {
        for (std::uint8_t tick=0;tick<90;++tick)
            Check(bp::TickPrecedentRuntime(state,Idle()),"audit row auto hold advances");
    }
    Check(state.scene==bp::PrecedentScene::Playing && state.borrowedActive!=0,
        "fourth audit row populates PRECEDENT only after hold");

    bp::PrecedentRuntime left{};
    bp::PrecedentRuntime right{};
    Check(bp::InitializePrecedentRuntime(left,bp::MakeInitialClaimLedger(),narrative)
        && bp::InitializePrecedentRuntime(right,bp::MakeInitialClaimLedger(),narrative)
        && bp::PopulatePrecedentBattlefield(left)
        && bp::PopulatePrecedentBattlefield(right),"determinism pair initializes");
    bool same=true;
    for (std::uint16_t tick=0;tick<480;++tick) {
        same=bp::TickPrecedentRuntime(left,Idle())
            && bp::TickPrecedentRuntime(right,Idle()) && same;
        same=bp::PrecedentRuntimeHash(left)==bp::PrecedentRuntimeHash(right) && same;
    }
    Check(same,"same seed profile and input produce same hash every tick");
}

void TestBoundariesModulesAndFold(const std::uint32_t (&representative)[81]) noexcept {
    Check(bp::PrecedentPhaseForShell(240)==0 && bp::PrecedentPhaseForShell(161)==0
        && bp::PrecedentPhaseForShell(160)==1 && bp::PrecedentPhaseForShell(82)==1
        && bp::PrecedentPhaseForShell(81)==2 && bp::PrecedentPhaseForShell(0)==2,
        "PRECEDENT phases use exact 67 and 34 percent boundaries");

    auto world=[&representative](std::uint8_t i) noexcept {
        bp::PrecedentRuntime state{};
        const bp::NarrativeState narrative=NarrativeForCode(representative[i]);
        Check(bp::InitializePrecedentRuntime(state,bp::MakeInitialClaimLedger(),narrative)
            && bp::PopulatePrecedentBattlefield(state),"module boundary world initializes");
        return state;
    };

    bp::PrecedentRuntime material=world(0);
    material.encounterTick=719;
    Check(bp::TickPrecedentRuntime(material,Idle()) && material.markedOriginMass==16
        && material.markedOriginTicks==90,"ORIGIN_LOCK starts exact tick720 mass16 duration90");

    const std::uint8_t continuityIndex=static_cast<std::uint8_t>(
        static_cast<std::uint8_t>(bp::IdentityPolicy::Continuity)*27u);
    bp::PrecedentRuntime continuity=world(continuityIndex);
    continuity.encounterTick=599;
    Check(bp::TickPrecedentRuntime(continuity,Idle()) && continuity.forkTicks==180,
        "FORK starts exact tick600 duration180");

    const std::uint8_t relationIndex=static_cast<std::uint8_t>(
        static_cast<std::uint8_t>(bp::IdentityPolicy::Relation)*27u);
    bp::PrecedentRuntime relation=world(relationIndex);
    std::uint8_t tetherCount=0;
    for (bp::SlotHandle handle:relation.tetherNodes)
        if (bp::CoverHandleValid(relation.combat,handle)) ++tetherCount;
    Check(tetherCount==3,"TETHER initializes exact three nodes of mass16");

    const std::uint8_t equalityIndex=0;
    bp::PrecedentRuntime equality=world(equalityIndex);
    DamageOpponentTo(equality,160);
    equality.encounterTick=599;
    const std::uint16_t equalityBefore=equality.movement.physicalLedger.mass[
        Account(bp::PhysicalAccount::OpponentShell)];
    Check(bp::TickPrecedentRuntime(equality,Idle())
        && equality.movement.physicalLedger.mass[Account(bp::PhysicalAccount::OpponentShell)]
            ==equalityBefore-32,"EQUALIZE tick600 moves floor8 capped32");

    const std::uint8_t needIndex=static_cast<std::uint8_t>(
        static_cast<std::uint8_t>(bp::DistributionPolicy::Need)*9u);
    bp::PrecedentRuntime need=world(needIndex);
    DamageOpponentTo(need,160);
    need.encounterTick=599;
    const std::uint16_t needBefore=need.movement.physicalLedger.mass[
        Account(bp::PhysicalAccount::PlayerShell)];
    Check(bp::TickPrecedentRuntime(need,Idle())
        && need.movement.physicalLedger.mass[Account(bp::PhysicalAccount::PlayerShell)]
            ==needBefore+16,"LOW_MASS_PRIORITY assigns Neutral16 at exact tick600");

    const std::uint8_t utilityIndex=static_cast<std::uint8_t>(
        static_cast<std::uint8_t>(bp::DistributionPolicy::Utility)*9u);
    bp::PrecedentRuntime utility=world(utilityIndex);
    DamageOpponentTo(utility,160);
    utility.encounterTick=479;
    utility.outputDamage[1]=100;
    const std::uint16_t playerBefore=utility.movement.physicalLedger.mass[
        Account(bp::PhysicalAccount::PlayerShell)];
    Check(bp::TickPrecedentRuntime(utility,Idle())
        && utility.movement.physicalLedger.mass[Account(bp::PhysicalAccount::PlayerShell)]
            ==playerBefore-16,"OUTPUT_SCORE applies at exact tick480");

    bp::PrecedentRuntime phaseDisplay=world(0);
    DamageOpponentTo(phaseDisplay,160);
    Check(bp::TickPrecedentRuntime(phaseDisplay,Idle())
        && phaseDisplay.phase==1 && phaseDisplay.phaseDisplayTicks==120,
        "DISTRIBUTION CASE to RULE display begins at exact phase entry");
    phaseDisplay.phaseDisplayTicks=1;
    Check(bp::TickPrecedentRuntime(phaseDisplay,Idle())
        && phaseDisplay.phaseDisplayTicks==0,
        "DISTRIBUTION policy display ends after exact 120 frames");

    bp::PrecedentRuntime adaptive=world(0);
    bool adaptiveOk=true;
    for (std::uint8_t tick=0;tick<24;++tick)
        adaptiveOk=adaptiveOk && bp::TickPrecedentRuntime(adaptive,Idle());
    Check(adaptiveOk && adaptive.action!=bp::PrecedentAction::Evade,
        "base Utility AI remains reachable before GOVERNANCE phase");

    const std::uint8_t commonsIndex=static_cast<std::uint8_t>(
        static_cast<std::uint8_t>(bp::GovernancePolicy::Commons)*3u);
    bp::PrecedentRuntime commons=world(commonsIndex);
    DamageOpponentTo(commons,80);
    commons.encounterTick=11;
    Check(bp::TickPrecedentRuntime(commons,Idle())
        && commons.action==bp::PrecedentAction::Reclaim,
        "VOTE_NODES count physical COMMON cover mass and choose low aggression on ties");

    bp::PrecedentRuntime replay=world(0);
    DamageOpponentTo(replay,80);
    replay.encounterTick=599;
    Check(bp::TickPrecedentRuntime(replay,Idle()) && replay.echoTicks==180
        && replay.echoIndex==0,"REPLAY_NAMED opens exact 180 tick echo slot");

    bp::PrecedentRuntime exceptionDenied=world(0);
    Check(bp::TickPrecedentRuntime(exceptionDenied,ActionInput(bp::PlatformAction::Fire)),
        "exception rule records initial player fire");
    for (std::uint8_t tick=0;tick<9;++tick)
        Check(bp::TickPrecedentRuntime(exceptionDenied,Idle()),"exception switch lock clears");
    Check(bp::TickPrecedentRuntime(exceptionDenied,ActionInput(bp::PlatformAction::Reclaim))
        && exceptionDenied.exceptionShown!=0 && exceptionDenied.exceptionDeniedShown!=0,
        "opposite action within 180 ticks requests and denies RULE exception once");

    const std::uint8_t judgmentIndex=static_cast<std::uint8_t>(
        static_cast<std::uint8_t>(bp::GovernancePolicy::Judgment)*3u);
    bp::PrecedentRuntime exceptionAllowed=world(judgmentIndex);
    Check(bp::TickPrecedentRuntime(exceptionAllowed,ActionInput(bp::PlatformAction::Fire)),
        "JUDGMENT exception fixture records fire");
    for (std::uint8_t tick=0;tick<9;++tick)
        Check(bp::TickPrecedentRuntime(exceptionAllowed,Idle()),"JUDGMENT switch lock clears");
    Check(bp::TickPrecedentRuntime(exceptionAllowed,ActionInput(bp::PlatformAction::Reclaim))
        && exceptionAllowed.exceptionShown!=0 && exceptionAllowed.exceptionDeniedShown==0,
        "JUDGMENT permits the first opposite action exception");

    bp::PrecedentRuntime originPriority=world(0);
    for (bp::NeutralSlot& neutral:originPriority.combat.neutrals)
        if (neutral.active!=0) neutral.reclaimLockTicks=2;
    bp::NeutralSlot& marked=originPriority.combat.neutrals[originPriority.neutrals[0].index];
    bp::NeutralSlot& ordinary=originPriority.combat.neutrals[originPriority.neutrals[1].index];
    marked.reclaimLockTicks=0; marked.position=bp::Vec2Q8{210*bp::kQ8One,90*bp::kQ8One};
    marked.playerMissMass=marked.mass;
    ordinary.reclaimLockTicks=0; ordinary.position=bp::Vec2Q8{250*bp::kQ8One,90*bp::kQ8One};
    originPriority.combat.actors[1].aimDirectionQ15=bp::Vec2Q8{-bp::kInputUnit,0};
    bp::ReclaimTickResult priorityResult{};
    Check(bp::TickReclaim(originPriority.combat,originPriority.movement.physicalLedger,
        originPriority.reclaim,false,true,priorityResult,false,true)
        && priorityResult.opponentTarget==originPriority.neutrals[0].index,
        "ORIGIN_LOCK gives player projectile residue exact +128 reclaim priority");

    bp::PrecedentRuntime folded=world(0);
    const std::uint16_t lockedBefore=folded.movement.physicalLedger.mass[
        Account(bp::PhysicalAccount::LockedBands)];
    Check(bp::FoldPrecedentBorrow(folded) && folded.borrowedActive==0
        && folded.borrowedFoldedMass==256
        && folded.movement.physicalLedger.mass[Account(bp::PhysicalAccount::LockedBands)]
            ==lockedBefore+256
        && bp::PhysicalLedgerTotal(folded.movement.physicalLedger)==bp::kLedgerTotal,
        "PRECEDENT exact borrowed256 folds atomically to LockedBands");

    bp::PrecedentRuntime unfolding=world(0);
    unfolding.scene=bp::PrecedentScene::CoreUnfold;
    const std::uint16_t unfoldLocked=unfolding.movement.physicalLedger.mass[
        Account(bp::PhysicalAccount::LockedBands)];
    bool unfoldOk=true;
    for (std::uint8_t tick=0;tick<36;++tick)
        unfoldOk=unfoldOk && bp::TickPrecedentRuntime(unfolding,Idle());
    Check(unfoldOk && unfolding.scene==bp::PrecedentScene::Complete
        && unfolding.movement.physicalLedger.mass[Account(bp::PhysicalAccount::LockedBands)]
            ==unfoldLocked+256,"core hit unfolds and folds on exact tick36");
}

void TestSaturationAndRender() noexcept {
    bp::PrecedentRuntime state{};
    Check(bp::InitializePrecedentRuntime(state,bp::MakeInitialClaimLedger(),NarrativeForCode(0))
        && bp::PopulatePrecedentBattlefield(state),"saturation world initializes");
    state.combat.nextStableId=0xFFFFFFFFu;
    const bp::PhysicalLedger before=state.movement.physicalLedger;
    const std::uint32_t hashBefore=bp::PrecedentRuntimeHash(state);
    Check(!bp::ApplyPrecedentNeutralTransferForTest(state,
        bp::PhysicalAccount::OpponentShell,16),"injected allocation failure rejects");
    Check(std::memcmp(&before,&state.movement.physicalLedger,sizeof(before))==0
        && bp::PrecedentRuntimeHash(state)==hashBefore
        && state.combat.phase==bp::CombatPhase::Active,
        "failed neutral transfer is exact-or-zero with no live mutation");

    Check(bp::InitializePrecedentRuntime(state,bp::MakeInitialClaimLedger(),NarrativeForCode(0))
        && bp::PopulatePrecedentBattlefield(state)
        && bp::PhysicalTransfer(state.movement.physicalLedger,
            bp::PhysicalAccount::LockedBands,bp::PhysicalAccount::PlayerShell,200),
        "projectile saturation world initializes");
    for (std::uint8_t i=0;i<31;++i)
        Check(bp::SpawnScriptedProjectile(state.combat,state.movement.physicalLedger,
            bp::CombatSide::Player,8,bp::Vec2Q8{32*bp::kQ8One,32*bp::kQ8One},
            bp::Vec2Q8{bp::kInputUnit,0},1),"projectile saturation fills reserved slots");
    const bp::PhysicalLedger saturatedBefore=state.movement.physicalLedger;
    Check(bp::ApplyPrecedentOpponentAttackForTest(state,8),
        "saturated opponent attack skips without runtime failure");
    std::uint8_t opponentShots=0;
    for (const bp::ProjectileSlot& shot:state.combat.projectiles)
        if (shot.active!=0 && shot.owner==bp::CombatSide::Opponent) ++opponentShots;
    Check(opponentShots==0
        && std::memcmp(&saturatedBefore,&state.movement.physicalLedger,
            sizeof(saturatedBefore))==0,
        "projectile slot saturation attack is exact-or-zero");

    std::uint32_t pixels[bp::kRenderWidth*bp::kRenderHeight]{};
    bp::PixelBuffer buffer{pixels,bp::kRenderWidth,bp::kRenderHeight,bp::kRenderWidth};
    bp::RenderPrecedentRuntime(buffer,state);
    std::uint32_t visible=0;
    for (std::uint32_t pixel:pixels) {
        Check(bp::IsPaletteColor(pixel),"PRECEDENT renderer uses frozen palette");
        if (pixel!=bp::Color(bp::PaletteColor::Background)) ++visible;
    }
    Check(visible>100,"PRECEDENT battlefield renders visible pixels");
}

struct Runner {
    Runner() noexcept {
        std::uint32_t representative[81]{};
        TestPolicyExhaustive(representative);
        TestAllProfiles(representative);
        TestAuditAndDeterminism();
        TestBoundariesModulesAndFold(representative);
        TestSaturationAndRender();
        if (gFailures!=0) {
            std::printf("T09_FAIL\tchecks=%u\tfailures=%u\n",gChecks,gFailures);
            std::exit(1);
        }
        std::printf("T09_PASS\tchecks=%u\tcombinations=19683\tprofiles=81\tsoftlocks=0\n",
            gChecks);
    }
};

Runner gRunner;

}  // namespace
