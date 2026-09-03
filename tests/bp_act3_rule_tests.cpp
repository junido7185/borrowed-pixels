#include "bp_act3.h"

#include "bp_claim_ledger.h"
#include "bp_physical_ledger.h"

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace {

std::uint32_t gChecks;
std::uint32_t gFailures;

void Check(bool condition, const char* name) noexcept {
    ++gChecks;
    if (!condition) {
        ++gFailures;
        std::printf("T08B_FAIL\t%s\n", name);
    }
}

constexpr std::uint8_t Account(bp::PhysicalAccount value) noexcept {
    return static_cast<std::uint8_t>(value);
}

bp::InputFrame Input(bool confirm = false) noexcept {
    bp::InputFrame input{};
    input.focused = 1;
    input.device = bp::InputDevice::KeyboardMouse;
    input.mouseX = 300;
    input.mouseY = 90;
    input.actions[static_cast<std::uint8_t>(bp::PlatformAction::Confirm)].held =
        confirm ? 1u : 0u;
    return input;
}

void TestDefinitionsExact() noexcept {
    constexpr std::uint16_t opponent[3] = {48,72,240};
    constexpr std::uint16_t field[3] = {192,160,256};
    constexpr std::uint16_t neutral[3] = {96,64,160};
    constexpr std::uint16_t duration[3] = {3600,3600,18000};
    constexpr std::uint8_t fieldCount[3] = {9,5,8};
    constexpr std::uint8_t neutralCount[3] = {12,8,20};
    for (std::uint8_t i=0; i<3; ++i) {
        const bp::Act3EncounterDef& definition=bp::GetAct3EncounterDef(i);
        Check(bp::ValidateAct3EncounterDef(definition),"Act 3 definition validates");
        Check(definition.opponentMass==opponent[i],"opponent mass exact");
        Check(definition.fieldMass==field[i],"field mass exact");
        Check(definition.neutralMass==neutral[i],"neutral mass exact");
        Check(definition.durationTicks==duration[i],"duration exact");
        Check(definition.fieldCount==fieldCount[i],"field count exact");
        Check(definition.neutralCount==neutralCount[i],"neutral count exact");
    }
    const bp::Act3EncounterDef& index=bp::GetAct3EncounterDef(0);
    Check(index.playerX==48 && index.playerY==92 && index.opponentX==272
        && index.opponentY==90,"INDEX spawns exact");
    Check(index.field[0].centerX==104 && index.field[0].centerY==54
        && index.field[5].centerX==216 && index.field[5].centerY==126,
        "INDEX panels exact");
    for (std::uint8_t i=6;i<9;++i)
        Check(index.field[i].mass==8 && index.field[i].metric!=0,"INDEX metrics exact");
    Check(index.neutral[0].tag==bp::Act3NeutralTag::Unscored
        && index.neutral[1].tag==bp::Act3NeutralTag::Unscored,
        "INDEX UNSCORED 16 exact");
    const bp::Act3EncounterDef& witness=bp::GetAct3EncounterDef(1);
    Check(witness.field[0].centerX==96 && witness.field[4].centerX==192
        && witness.neutral[7].x==216 && witness.neutral[7].y==104,
        "WITNESS battlefield endpoints exact");
    const bp::Act3EncounterDef& current=bp::GetAct3EncounterDef(2);
    Check(current.playerX==42 && current.opponentX==264,"CURRENT spawns exact");
    Check(current.field[6].centerX==128 && current.field[6].height==30
        && current.field[7].centerX==192,"CURRENT vertical panels exact");
    Check(current.neutral[0].x==72 && current.neutral[19].x==240
        && current.neutral[19].y==116,"CURRENT neutral endpoints exact");
    bp::Act3EncounterDef invalid=current;
    ++invalid.field[0].mass;
    Check(!bp::ValidateAct3EncounterDef(invalid),"field total mismatch rejected");
    invalid=current;
    invalid.neutral[0].x=0;
    Check(!bp::ValidateAct3EncounterDef(invalid),"out of bounds neutral rejected");
    invalid=current;
    invalid.playerX=invalid.field[0].centerX;
    invalid.playerY=invalid.field[0].centerY;
    Check(!bp::ValidateAct3EncounterDef(invalid),"spawn field overlap rejected");

    constexpr const char* response[3][3][2]={
        {{"WHAT I COULD COUNT","BECAME YOUR VALUE"},
          {"THE ZERO REMAINS","UNPROVEN"},
          {"YOU CHANGED THE TEST","NOT THE SCORES"}},
        {{"THE LOSS KEEPS","ITS NAME"},
          {"THE LOSS REMAINS","ITS OWNER DOES NOT"},
          {"THE RUN CONTINUES","WITHOUT A WITNESS"}},
        {{"YOU STOPPED THE FLOW","TO NAME IT YOURS"},
          {"THE CYCLE CONTINUES","WITHOUT YOUR NAME"},
          {"NO ONE KEEPS IT","EVERYONE DEPENDS ON IT"}},
    };
    for(std::uint8_t encounter=0;encounter<3;++encounter)
        for(std::uint8_t choice=0;choice<3;++choice)
            for(std::uint8_t line=0;line<2;++line)
                Check(std::strcmp(bp::Act3ResponseText(encounter,
                    static_cast<bp::ChoiceId>(choice),line),
                    response[encounter][choice][line])==0,
                    "Act 3 exact authoritative response copy");
}

void TestIndexPriorityExhaustive() noexcept {
    for (std::uint8_t rewrite=0; rewrite<4; ++rewrite) {
        for (std::uint8_t damageHigh=0; damageHigh<2; ++damageHigh) {
            for (std::uint8_t distanceHigh=0; distanceHigh<2; ++distanceHigh) {
                for (std::uint8_t idle=0; idle<2; ++idle) {
                    for (std::uint8_t unscored=0; unscored<2; ++unscored) {
                        const std::uint16_t damage=damageHigh?32:31;
                        const std::uint32_t distance=distanceHigh?160u*bp::kQ8One:
                            160u*bp::kQ8One-1u;
                        const bool optimize=damageHigh && distanceHigh && idle==0;
                        const bp::ChoiceId expected=rewrite>=2?bp::ChoiceId::Third:
                            (unscored && !optimize?bp::ChoiceId::Second:
                                (optimize?bp::ChoiceId::First:bp::ChoiceId::Second));
                        Check(bp::ResolveIndexResult(damage,distance,idle,
                            unscored?16:8,rewrite)==expected,
                            "INDEX REWRITE REFUSE OPTIMIZE priority exhaustive");
                    }
                }
            }
        }
    }
}

void TestWitnessResults() noexcept {
    for (std::uint8_t records=0;records<=3;++records) {
        for (std::uint8_t direct=0;direct<=48;direct=static_cast<std::uint8_t>(direct+16)) {
            const bp::ChoiceId expected=direct==48?bp::ChoiceId::Third:
                (records==3?bp::ChoiceId::First:bp::ChoiceId::Second);
            Check(bp::ResolveWitnessResult(records,direct)==expected,
                "WITNESS result meaning exhaustive");
        }
    }
}

void TestCurrentCycleAndAngles() noexcept {
    std::uint16_t counts[7]{};
    for (std::uint16_t tick=0;tick<454;++tick)
        ++counts[static_cast<std::uint8_t>(bp::CurrentActionAtCycleTick(tick))];
    constexpr std::uint16_t expected[7]={48,60,1,90,60,75,120};
    for (std::uint8_t i=0;i<7;++i) Check(counts[i]==expected[i],
        "CURRENT nominal cycle duration exact");
    Check(bp::CurrentActionAtCycleTick(454)==bp::CurrentAction::Orbit,
        "CURRENT cycle repeats exactly");
    const bp::Vec2Q8 forward{bp::kInputUnit,0};
    const bp::Vec2Q8 minus24=bp::RotateCurrentDirection(forward,-24);
    const bp::Vec2Q8 minus8=bp::RotateCurrentDirection(forward,-8);
    const bp::Vec2Q8 plus8=bp::RotateCurrentDirection(forward,8);
    const bp::Vec2Q8 plus24=bp::RotateCurrentDirection(forward,24);
    Check(minus24.y<minus8.y && minus8.y<0 && plus8.y>0 && plus24.y>plus8.y,
        "CURRENT WAVE angles ordered -24 -8 +8 +24");
}

bp::Act3Runtime World(std::uint8_t encounter) noexcept {
    bp::NarrativeState narrative{};
    bp::Act3Runtime state{};
    Check(bp::InitializeAct3Runtime(state,bp::MakeInitialClaimLedger(),narrative),
        "Act 3 world initializes");
    state.encounterIndex=encounter;
    state.scene=bp::Act3Scene::Playing;
    Check(bp::PopulateAct3Encounter(state),"Act 3 world populates");
    return state;
}

void TestPopulationAndPhysicalMass() noexcept {
    bp::Act3Runtime index=World(0);
    Check(index.combat.covers[6].mass==8 && index.combat.covers[8].mass==8,
        "INDEX metric covers materialized");
    Check(index.index.probes[0].active!=0 && index.index.probes[1].active!=0
        && index.index.probes[0].targetCore!=index.index.probes[1].targetCore,
        "INDEX reuses two alternating CHECKSUM probes");
    Check(bp::PhysicalLedgerTotal(index.movement.physicalLedger)==bp::kLedgerTotal,
        "INDEX physical ledger 1440");
    bp::Act3Runtime witness=World(1);
    Check(witness.witness.records[0].position.x==132*bp::kQ8One
        && witness.witness.records[2].position.y==106*bp::kQ8One,
        "WITNESS three record positions exact");
    Check(witness.witness.records[0].stableId<witness.witness.records[1].stableId,
        "WITNESS target stable order exact");
    bp::Act3Runtime current=World(2);
    const bp::PhysicalLedger& ledger=current.movement.physicalLedger;
    Check(ledger.mass[Account(bp::PhysicalAccount::OpponentCore)]==16
        && ledger.mass[Account(bp::PhysicalAccount::OpponentShell)]==48,
        "CURRENT core16 shell48 after circulation allocation");
    Check(ledger.mass[Account(bp::PhysicalAccount::Projectile)]==128,
        "CURRENT starts sixteen owned projectiles mass8");
    Check(ledger.mass[Account(bp::PhysicalAccount::Neutral)]==208,
        "CURRENT field160 plus owned orbit neutral48");
    std::uint16_t projectiles=0;
    for (const bp::ProjectileSlot& shot:current.combat.projectiles)
        if (shot.active!=0) ++projectiles;
    Check(projectiles==16,"CURRENT projectile count exact");
    const std::uint32_t first=current.current.initialProjectileFirstStableId;
    bp::Vec2Q8 cardinals[4]{};
    for (const bp::ProjectileSlot& shot:current.combat.projectiles) {
        if (shot.active==0 || shot.stableId<first || shot.stableId>=first+16) continue;
        const std::uint8_t phase=static_cast<std::uint8_t>(shot.stableId-first);
        if ((phase&3u)==0) cardinals[phase/4u]=shot.position;
    }
    Check(cardinals[0].x==250*bp::kQ8One && cardinals[0].y==90*bp::kQ8One
        && cardinals[1].x==228*bp::kQ8One && cardinals[1].y==126*bp::kQ8One
        && cardinals[2].x==206*bp::kQ8One && cardinals[2].y==90*bp::kQ8One
        && cardinals[3].x==228*bp::kQ8One && cardinals[3].y==54*bp::kQ8One,
        "CURRENT orbitRadiusPx applies exact x22 y36 cardinal positions");
    Check(bp::PhysicalLedgerTotal(ledger)==bp::kLedgerTotal
        && bp::CombatStateValid(current.combat,ledger),
        "CURRENT initial circulation conserves 1440");
    Check(bp::TickCurrentScriptForTest(current)
        && current.current.action==bp::CurrentAction::ReturnA,
        "CURRENT skips unaffordable ORBIT to RETURN while preserving Shell41");

    bp::Act3Runtime saturated=World(2);
    Check(bp::PhysicalTransfer(saturated.movement.physicalLedger,
        bp::PhysicalAccount::LockedBands,bp::PhysicalAccount::PlayerShell,24),
        "CURRENT saturation fixture borrows test-only shell24");
    for (std::uint8_t i=0;i<9;++i)
        Check(bp::SpawnScriptedProjectile(saturated.combat,
            saturated.movement.physicalLedger,bp::CombatSide::Player,8,
            bp::Vec2Q8{40*bp::kQ8One,40*bp::kQ8One},
            bp::Vec2Q8{bp::kInputUnit,0},1),"CURRENT saturation slot prepared");
    saturated.movement.physicalLedger.mass[Account(bp::PhysicalAccount::OpponentShell)]=112;
    saturated.movement.physicalLedger.mass[Account(bp::PhysicalAccount::LockedBands)]=376;
    Check(bp::PhysicalLedgerValid(saturated.movement.physicalLedger)
        && bp::CombatStateValid(saturated.combat,saturated.movement.physicalLedger),
        "CURRENT saturated fixture conserves ledger and slot mass");
    Check(bp::TickCurrentScriptForTest(saturated)
        && saturated.current.action==bp::CurrentAction::ReturnA,
        "CURRENT skips ORBIT when eight projectile slots are unavailable");

    bp::Act3Runtime orbitDamaged=World(2);
    Check(bp::PhysicalTransfer(orbitDamaged.movement.physicalLedger,
        bp::PhysicalAccount::LockedBands,bp::PhysicalAccount::OpponentShell,57)
        && bp::TickCurrentScriptForTest(orbitDamaged)
        && orbitDamaged.current.orbitShots==1,
        "CURRENT ORBIT begins with exact mass64 plus shell41 budget");
    Check(bp::PhysicalTransfer(orbitDamaged.movement.physicalLedger,
        bp::PhysicalAccount::OpponentShell,bp::PhysicalAccount::LockedBands,56),
        "CURRENT ORBIT damage fixture reduces shell to reserve");
    bool orbitAdvanced=true;
    for (std::uint8_t tick=0;tick<6;++tick)
        orbitAdvanced=orbitAdvanced && bp::TickCurrentScriptForTest(orbitDamaged);
    Check(orbitAdvanced && orbitDamaged.current.action==bp::CurrentAction::ReturnA
        && orbitDamaged.current.orbitShots==0
        && orbitDamaged.movement.physicalLedger.mass[
            Account(bp::PhysicalAccount::OpponentShell)]==41,
        "CURRENT damaged ORBIT skips remaining shots while preserving shell41");

    bp::Act3Runtime orbitFull=World(2);
    Check(bp::PhysicalTransfer(orbitFull.movement.physicalLedger,
        bp::PhysicalAccount::LockedBands,bp::PhysicalAccount::OpponentShell,57)
        && bp::PhysicalTransfer(orbitFull.movement.physicalLedger,
            bp::PhysicalAccount::LockedBands,bp::PhysicalAccount::PlayerShell,120)
        && bp::TickCurrentScriptForTest(orbitFull),
        "CURRENT ORBIT slot saturation fixture begins");
    for (std::uint8_t i=0;i<15;++i)
        Check(bp::SpawnScriptedProjectile(orbitFull.combat,
            orbitFull.movement.physicalLedger,bp::CombatSide::Player,8,
            bp::Vec2Q8{40*bp::kQ8One,40*bp::kQ8One},
            bp::Vec2Q8{bp::kInputUnit,0},1),
            "CURRENT ORBIT fills a remaining shared projectile slot");
    bool fullAdvanced=true;
    for (std::uint8_t tick=0;tick<6;++tick)
        fullAdvanced=fullAdvanced && bp::TickCurrentScriptForTest(orbitFull);
    Check(fullAdvanced && orbitFull.current.action==bp::CurrentAction::ReturnA
        && orbitFull.current.orbitShots==0
        && bp::PhysicalLedgerTotal(orbitFull.movement.physicalLedger)==bp::kLedgerTotal,
        "CURRENT saturated ORBIT skips remaining shots without fatal state");

    bp::Act3Runtime foreclose=World(2);
    Check(bp::PhysicalTransfer(foreclose.movement.physicalLedger,
        bp::PhysicalAccount::LockedBands,bp::PhysicalAccount::OpponentShell,41),
        "CURRENT FORECLOSE fixture funds shell41 reserve");
    foreclose.current.action=bp::CurrentAction::Foreclose;
    foreclose.combat.actors[0].position.y=90*bp::kQ8One;
    const bp::Vec2Q8 lockedPlayer=foreclose.combat.actors[0].position;
    bool advanced=true;
    for (std::uint8_t tick=0;tick<75;++tick) {
        if (tick==1) foreclose.combat.actors[0].position.y+=30*bp::kQ8One;
        advanced=advanced && bp::TickCurrentScriptForTest(foreclose);
    }
    std::uint8_t forecloseShots=0;
    for (const bp::ProjectileSlot& shot:foreclose.combat.projectiles)
        if (shot.active!=0 && shot.mass==48) ++forecloseShots;
    Check(advanced && foreclose.current.action==bp::CurrentAction::ReturnB
        && foreclose.combat.charges[1].active==0 && forecloseShots==1,
        "CURRENT FORECLOSE releases exact mass48 after exact 75 ticks");
    const bp::Vec2Q8 expectedAim{lockedPlayer.x-264*bp::kQ8One,
        lockedPlayer.y-90*bp::kQ8One};
    Check(foreclose.current.forecloseAim.y==0 && expectedAim.y==0,
        "CURRENT FORECLOSE aim remains locked against later player movement");

    bp::Act3Runtime cancelled=World(2);
    Check(bp::PhysicalTransfer(cancelled.movement.physicalLedger,
        bp::PhysicalAccount::LockedBands,bp::PhysicalAccount::OpponentShell,41),
        "CURRENT cancelled FORECLOSE fixture funds shell41 reserve");
    cancelled.current.action=bp::CurrentAction::Foreclose;
    Check(bp::TickCurrentScriptForTest(cancelled)
        && cancelled.combat.charges[1].active!=0,
        "CURRENT cancelled FORECLOSE begins charge");
    cancelled.combat.actors[1].projectileImmune=1;
    cancelled.combat.charges[1].position=bp::Vec2Q8{180*bp::kQ8One,90*bp::kQ8One};
    Check(bp::SpawnScriptedProjectile(cancelled.combat,
        cancelled.movement.physicalLedger,bp::CombatSide::Player,8,
        cancelled.combat.charges[1].position,bp::Vec2Q8{bp::kInputUnit,0},1)
        && bp::StepCombatProjectiles(cancelled.combat,
            cancelled.movement.physicalLedger,cancelled.claimLedger)
        && cancelled.combat.charges[1].active==0,
        "CURRENT player projectile fully cancels FORECLOSE charge");
    Check(bp::TickCurrentScriptForTest(cancelled)
        && cancelled.current.action==bp::CurrentAction::ReturnB,
        "CURRENT cancelled FORECLOSE advances to RETURN without softlock");

    bp::Act3Runtime damaged=World(2);
    Check(bp::PhysicalTransfer(damaged.movement.physicalLedger,
        bp::PhysicalAccount::LockedBands,bp::PhysicalAccount::OpponentShell,41),
        "CURRENT damaged FORECLOSE fixture funds shell41 reserve");
    damaged.current.action=bp::CurrentAction::Foreclose;
    bool charged=true;
    for (std::uint8_t tick=0;tick<48;++tick)
        charged=charged && bp::TickCurrentScriptForTest(damaged);
    Check(charged && damaged.combat.charges[1].mass==32
        && damaged.movement.physicalLedger.mass[
            Account(bp::PhysicalAccount::OpponentShell)]==57,
        "CURRENT damaged FORECLOSE reaches mass32 with shell57");
    damaged.combat.actors[1].projectileImmune=1;
    Check(bp::SpawnScriptedProjectile(damaged.combat,
        damaged.movement.physicalLedger,bp::CombatSide::Player,8,
        damaged.combat.charges[1].position,bp::Vec2Q8{bp::kInputUnit,0},1)
        && bp::StepCombatProjectiles(damaged.combat,
            damaged.movement.physicalLedger,damaged.claimLedger)
        && damaged.combat.charges[1].active!=0
        && damaged.combat.charges[1].mass==24,
        "CURRENT FORECLOSE charge accepts partial damage");
    Check(bp::TickCurrentScriptForTest(damaged)
        && damaged.current.action==bp::CurrentAction::ReturnB
        && damaged.combat.charges[1].active==0
        && damaged.movement.physicalLedger.mass[
            Account(bp::PhysicalAccount::OpponentShell)]==81
        && bp::PhysicalLedgerTotal(damaged.movement.physicalLedger)==bp::kLedgerTotal
        && bp::CombatStateValid(damaged.combat,damaged.movement.physicalLedger),
        "CURRENT damaged FORECLOSE cancels when mass48 plus shell41 is unaffordable");

    bp::Act3Runtime ordered=World(2);
    Check(bp::PhysicalTransfer(ordered.movement.physicalLedger,
        bp::PhysicalAccount::LockedBands,bp::PhysicalAccount::OpponentShell,41),
        "CURRENT step07 fixture funds FORECLOSE");
    ordered.current.action=bp::CurrentAction::Foreclose;
    ordered.combat.actors[0].position.y=90*bp::kQ8One;
    ordered.movement.playerPosition=ordered.combat.actors[0].position;
    bp::InputFrame moving=Input();
    moving.moveY=32767;
    Check(bp::TickAct3Runtime(ordered,moving)
        && ordered.current.forecloseAim.y==0
        && ordered.combat.actors[0].position.y>90*bp::kQ8One,
        "CURRENT decision step07 observes player before same-tick movement step10");

    bp::Act3Runtime mover=World(2);
    mover.current.action=bp::CurrentAction::MoveA;
    const bp::Vec2Q8 opponentBefore=mover.combat.actors[1].position;
    Check(bp::TickCurrentScriptForTest(mover)
        && mover.current.movePending!=0
        && mover.combat.actors[1].position.x==opponentBefore.x
        && mover.combat.actors[1].position.y==opponentBefore.y,
        "CURRENT step07 selects MOVE without mutating actor position");
    Check(bp::TickAct3Runtime(mover,Input())
        && (mover.combat.actors[1].position.x!=opponentBefore.x
            || mover.combat.actors[1].position.y!=opponentBefore.y),
        "CURRENT MOVE mutates actor position in common step10");

    bool separated=true;
    for(std::uint8_t encounter=0;encounter<3;++encounter) {
        bp::Act3Runtime overlap=World(encounter);
        const bp::Vec2Q8 shared{260*bp::kQ8One,90*bp::kQ8One};
        overlap.combat.actors[0].position=shared;
        overlap.combat.actors[1].position=shared;
        overlap.movement.playerPosition=shared;
        separated=separated && bp::TickAct3Runtime(overlap,Input());
        const std::int64_t dx=static_cast<std::int64_t>(
            overlap.combat.actors[1].position.x)-overlap.combat.actors[0].position.x;
        const std::int64_t dy=static_cast<std::int64_t>(
            overlap.combat.actors[1].position.y)-overlap.combat.actors[0].position.y;
        const std::int32_t radii=bp::ActorRadiusPixels(
            overlap.movement.physicalLedger.mass[Account(bp::PhysicalAccount::PlayerShell)])
            +bp::ActorRadiusPixels(overlap.movement.physicalLedger.mass[
                Account(bp::PhysicalAccount::OpponentShell)]);
        separated=separated && static_cast<std::uint64_t>(dx*dx+dy*dy)
            >=static_cast<std::uint64_t>(radii*bp::kQ8One)
                *static_cast<std::uint64_t>(radii*bp::kQ8One);
    }
    Check(separated,"Act 3 step10 separates actor bodies in all encounters");
}

void TestChecksumAndWitnessProcesses() noexcept {
    bp::Act3Runtime index=World(0);
    const bp::Vec2Q8 probePosition{160*bp::kQ8One,70*bp::kQ8One};
    index.index.probes[0].position=probePosition;
    index.combat.covers[index.index.probes[0].physicalHandle.index].bounds=
        bp::RectPixels{156,66,8,8};
    Check(bp::SpawnScriptedProjectile(index.combat,index.movement.physicalLedger,
        bp::CombatSide::Player,8,probePosition,bp::Vec2Q8{bp::kInputUnit,0},1),
        "INDEX player shot prepared at CHECKSUM probe");
    Check(bp::TickAct3Runtime(index,Input()),"INDEX CHECKSUM collision tick");
    Check(index.index.probes[0].active==0 && index.index.probes[0].respawnTicks==120,
        "INDEX CHECKSUM hit becomes 120 tick rebuild wait");
    Check(index.movement.physicalLedger.mass[Account(bp::PhysicalAccount::ProtocolActive)]==16,
        "INDEX destroyed probe moves protocol16 out of active process");
    std::uint16_t tagged=0;
    for (const bp::NeutralSlot& neutral:index.combat.neutrals)
        if (neutral.active!=0) tagged=static_cast<std::uint16_t>(tagged+neutral.protocolTagMass);
    Check(tagged==16,"INDEX destroyed probe creates protocol-tagged neutral16");
    for (std::uint16_t tick=0;tick<120;++tick)
        Check(bp::TickAct3Runtime(index,Input()),"INDEX CHECKSUM rebuild advances");
    Check(index.index.probes[0].active!=0
        && index.movement.physicalLedger.mass[Account(bp::PhysicalAccount::ProtocolActive)]==32,
        "INDEX CHECKSUM rebuilds from nearest tagged mass after 120 ticks");

    bp::Act3Runtime retarget=World(0);
    bool retargetBoundary=true;
    for (std::uint8_t tick=0;tick<30;++tick) {
        retargetBoundary=retargetBoundary && bp::TickAct3Runtime(retarget,Input())
            && retarget.index.probes[0].targetCore==0;
    }
    retargetBoundary=retargetBoundary && bp::TickAct3Runtime(retarget,Input())
        && retarget.index.probes[0].targetCore==1;
    Check(retargetBoundary,"INDEX CHECKSUM retargets after exact 30 tracked ticks");

    bp::Act3Runtime provenance=World(0);
    const bp::SlotHandle unscored=provenance.index.unscored[0];
    const bp::SlotHandle ordinary=provenance.neutrals[2];
    provenance.combat.neutrals[ordinary.index].position=
        provenance.combat.neutrals[unscored.index].position;
    provenance.combat.neutrals[ordinary.index].velocity=bp::Vec2Q8{0,0};
    provenance.combat.neutrals[unscored.index].velocity=bp::Vec2Q8{0,0};
    Check(bp::MergeNeutralClusters(provenance.combat,provenance.movement.physicalLedger),
        "INDEX UNSCORED fixture merges ordinary mass");
    std::uint16_t unscoredTotal=0;
    bool mixedUnscored=false;
    for (const bp::NeutralSlot& neutral:provenance.combat.neutrals) {
        if (neutral.active==0) continue;
        unscoredTotal=static_cast<std::uint16_t>(unscoredTotal+neutral.unscoredMass);
        if (neutral.unscoredMass!=0 && neutral.mass>neutral.unscoredMass) mixedUnscored=true;
    }
    Check(unscoredTotal==16 && mixedUnscored,
        "INDEX UNSCORED provenance remains exact through ordinary merge");

    bp::Act3Runtime indexCore=World(0);
    Check(bp::PhysicalTransfer(indexCore.movement.physicalLedger,
        bp::PhysicalAccount::OpponentShell,bp::PhysicalAccount::Neutral,32),
        "INDEX core-safety fixture exposes shell");
    indexCore.combat.neutrals[indexCore.neutrals[0].index].mass=40;
    Check(bp::SpawnScriptedProjectile(indexCore.combat,indexCore.movement.physicalLedger,
        bp::CombatSide::Player,8,indexCore.combat.actors[1].position,
        bp::Vec2Q8{bp::kInputUnit,0},1),"INDEX core-safety shot prepared");
    Check(bp::CombatStateValid(indexCore.combat,indexCore.movement.physicalLedger)
        && bp::TickAct3Runtime(indexCore,Input()),"INDEX exposed special encounter tick survives");
    Check(indexCore.combat.actors[1].coreHit==0
        && indexCore.combat.phase==bp::CombatPhase::Active,
        "INDEX core hit is disabled until its 60-second result condition");

    bp::Act3Runtime defended=World(1);
    const bp::SlotHandle pushedHandle=defended.neutrals[0];
    defended.combat.neutrals[pushedHandle.index].position=bp::Vec2Q8{
        13*bp::kQ8One,78*bp::kQ8One};
    defended.combat.neutrals[pushedHandle.index].velocity=bp::Vec2Q8{0,0};
    Check(bp::SpawnScriptedProjectile(defended.combat,defended.movement.physicalLedger,
        bp::CombatSide::Player,8,bp::Vec2Q8{13*bp::kQ8One,78*bp::kQ8One},
        bp::Vec2Q8{bp::kInputUnit,0},1),"WITNESS defense projectile prepared");
    Check(bp::TickAct3Runtime(defended,Input()),"WITNESS first SWEEPER wave starts");
    std::uint8_t remainingPlayerMass=0;
    for (const bp::ProjectileSlot& shot:defended.combat.projectiles)
        if (shot.active!=0 && shot.owner==bp::CombatSide::Player)
            remainingPlayerMass=static_cast<std::uint8_t>(remainingPlayerMass+shot.mass);
    Check(defended.witness.sweeperActive!=0 && defended.witness.sweeperTarget==0
        && remainingPlayerMass==4
        && defended.movement.physicalLedger.mass[Account(bp::PhysicalAccount::ProtocolActive)]==28
        && defended.movement.physicalLedger.mass[Account(bp::PhysicalAccount::Transit)]==4,
        "WITNESS swept collision cancels segment4 and preserves remaining projectile4");
    Check(defended.combat.neutrals[pushedHandle.index].position.x>=19*bp::kQ8One,
        "WITNESS SWEEPER pushes unregistered Neutral inward");
    bool waveOk=true;
    for (std::uint8_t tick=1;tick<90;++tick)
        waveOk=waveOk && bp::TickAct3Runtime(defended,Input());
    Check(waveOk && defended.witness.records[0].active!=0
        && defended.witness.sweeperGap!=0 && defended.witness.sweeperReturning!=0,
        "WITNESS record survives exact 90-tick wave through physical gap");
    bool returnOk=true;
    for (std::uint8_t tick=0;tick<36;++tick)
        returnOk=returnOk && bp::TickAct3Runtime(defended,Input());
    Check(returnOk
        && defended.movement.physicalLedger.mass[Account(bp::PhysicalAccount::ProtocolActive)]==32
        && defended.witness.sweeperReturning==0,
        "WITNESS ProtocolEscrow returns over exact 36 ticks");

    bp::Act3Runtime orderedReturn=World(1);
    Check(bp::PhysicalTransfer(orderedReturn.movement.physicalLedger,
        bp::PhysicalAccount::ProtocolActive,bp::PhysicalAccount::Transit,4),
        "WITNESS step04 ordering fixture escrows protocol4");
    orderedReturn.witness.protocolEscrowMass=4;
    orderedReturn.witness.sweeperReturnTick=35;
    orderedReturn.witness.sweeperReturning=1;
    orderedReturn.encounterTick=1200;
    Check(bp::TickAct3Runtime(orderedReturn,Input())
        && orderedReturn.witness.sweeperReturning==0
        && orderedReturn.witness.sweeperActive!=0
        && orderedReturn.witness.sweeperTick==1
        && orderedReturn.movement.physicalLedger.mass[
            Account(bp::PhysicalAccount::ProtocolActive)]==32
        && orderedReturn.movement.physicalLedger.mass[
            Account(bp::PhysicalAccount::Transit)]==0,
        "WITNESS step04 escrow arrival enables same-tick step07 wave start");

    bp::Act3Runtime immune=World(1);
    const std::uint16_t witnessShell=immune.movement.physicalLedger.mass[
        Account(bp::PhysicalAccount::OpponentShell)];
    Check(bp::SpawnScriptedProjectile(immune.combat,immune.movement.physicalLedger,
        bp::CombatSide::Player,8,immune.combat.actors[1].position,
        bp::Vec2Q8{bp::kInputUnit,0},1),"WITNESS record-immunity shot prepared");
    Check(bp::TickAct3Runtime(immune,Input())
        && immune.movement.physicalLedger.mass[Account(bp::PhysicalAccount::OpponentShell)]
            ==witnessShell && immune.witness.records[0].active!=0,
        "WITNESS body and record48 are immune to player projectile damage");

    bp::Act3Runtime dismantled=World(1);
    dismantled.combat.actors[0].position=dismantled.witness.records[0].position;
    dismantled.movement.playerPosition=dismantled.witness.records[0].position;
    bp::InputFrame reclaim=Input();
    reclaim.actions[static_cast<std::uint8_t>(bp::PlatformAction::Reclaim)].held=1;
    for (std::uint8_t tick=0;tick<bp::kWitnessDismantleTicks;++tick)
        Check(bp::TickAct3Runtime(dismantled,reclaim),"WITNESS direct channel advances");
    Check(dismantled.witness.records[0].active==0
        && dismantled.witness.directRecordMass==16,
        "WITNESS exact 60 tick direct channel reclaims record16");
    Check(bp::PhysicalLedgerValid(dismantled.movement.physicalLedger),
        "WITNESS direct dismantle conserves physical1440");

    bp::Act3Runtime rollback=World(1);
    Check(bp::PhysicalTransfer(rollback.movement.physicalLedger,
        bp::PhysicalAccount::OpponentShell,bp::PhysicalAccount::LockedBands,48),
        "WITNESS rollback fixture removes record funding");
    rollback.combat.actors[0].position=rollback.witness.records[0].position;
    rollback.movement.playerPosition=rollback.witness.records[0].position;
    bool channelOk=true;
    for (std::uint8_t tick=0;tick<59;++tick)
        channelOk=channelOk && bp::TickAct3Runtime(rollback,reclaim);
    const bp::PhysicalLedger beforeFailure=rollback.movement.physicalLedger;
    const bool failedTick=bp::TickAct3Runtime(rollback,reclaim);
    bool ledgerSame=true;
    for (std::uint8_t i=0;i<bp::kPhysicalAccountCount;++i)
        ledgerSame=ledgerSame && rollback.movement.physicalLedger.mass[i]==beforeFailure.mass[i];
    Check(channelOk && !failedTick && ledgerSame
        && rollback.witness.records[0].active!=0
        && rollback.witness.records[0].channelTicks==59,
        "WITNESS failed direct transfer is exact-or-zero and keeps record active");

    bp::Act3Runtime natural=World(1);
    bool naturalOk=true;
    for (std::uint16_t tick=0;tick<bp::kWitnessDurationTicks;++tick)
        naturalOk=naturalOk && bp::TickAct3Runtime(natural,Input());
    Check(naturalOk && natural.scene==bp::Act3Scene::HitFreeze
        && natural.committedChoice==bp::ChoiceId::Second,
        "WITNESS naturally reaches COMPRESS after exact 3600 ticks");

    bp::Act3Runtime current=World(2);
    Check(bp::PhysicalTransfer(current.movement.physicalLedger,
        bp::PhysicalAccount::OpponentShell,bp::PhysicalAccount::Neutral,48),
        "CURRENT natural completion fixture exposes core");
    current.combat.neutrals[current.neutrals[0].index].mass=56;
    Check(bp::SpawnScriptedProjectile(current.combat,current.movement.physicalLedger,
        bp::CombatSide::Player,8,current.combat.actors[1].position,
        bp::Vec2Q8{bp::kInputUnit,0},1),"CURRENT natural core shot prepared");
    Check(bp::CombatStateValid(current.combat,current.movement.physicalLedger)
        && bp::TickAct3Runtime(current,Input())
        && current.scene==bp::Act3Scene::HitFreeze,
        "CURRENT naturally reaches settlement through exposed core hit");
}

void TestReachabilityPixelsAndHash() noexcept {
    bp::NarrativeState narrative{};
    bp::Act3Runtime state{};
    Check(bp::InitializeAct3Runtime(state,bp::MakeInitialClaimLedger(),narrative),
        "Act 3 reachability initializes");
    const bp::InputFrame idle=Input();
    for (std::uint16_t tick=0;tick<270;++tick)
        Check(bp::TickAct3Runtime(state,idle),"Act 3 intro objective advances");
    Check(state.scene==bp::Act3Scene::Playing,"INDEX becomes playable");
    bp::Act3Runtime naturalIndex=state;
    bool naturalIndexOk=true;
    for (std::uint16_t tick=0;tick<bp::kIndexDurationTicks;++tick)
        naturalIndexOk=naturalIndexOk && bp::TickAct3Runtime(naturalIndex,idle);
    Check(naturalIndexOk && naturalIndex.scene==bp::Act3Scene::HitFreeze
        && naturalIndex.committedChoice==bp::ChoiceId::Second,
        "INDEX naturally reaches REFUSE after exact 3600 ticks");
    Check(bp::ForceAct3EncounterSuccessForTest(state),"INDEX deterministic result freezes");
    for (std::uint16_t tick=0;tick<bp::kHitFreezeTicks+bp::kSettlementWaitTicks;++tick)
        Check(bp::TickAct3Runtime(state,idle),"INDEX result wait advances");
    Check(state.scene==bp::Act3Scene::Preview
        && state.committedChoice==bp::ChoiceId::Second,
        "INDEX automatic REFUSE preview reachable");
    for (std::uint16_t tick=0;tick<bp::kAllocationPreviewTicks+bp::kCommitTransitTicks;++tick)
        Check(bp::TickAct3Runtime(state,idle),"INDEX automatic commit advances");
    Check(state.scene==bp::Act3Scene::EntityResponse,"INDEX settlement applied");
    Check(bp::ClaimLedgerValid(state.claimLedger),"INDEX settlement keeps 1440");
    std::uint32_t pixels[bp::kRenderWidth*bp::kRenderHeight]{};
    bp::PixelBuffer buffer{pixels,bp::kRenderWidth,bp::kRenderHeight,bp::kRenderWidth};
    bp::Act3Runtime current=World(2);
    bp::RenderAct3Runtime(buffer,current);
    std::uint32_t visible=0;
    for (std::uint32_t pixel:pixels)
        if (pixel!=bp::Color(bp::PaletteColor::Background)) ++visible;
    Check(visible>100,"Act 3 emits visible exact palette pixels");
    const bp::Act3Runtime clone=current;
    Check(bp::Act3RuntimeHash(current)==bp::Act3RuntimeHash(clone),
        "Act 3 hash deterministic for identical state");
    ++current.globalTick;
    Check(bp::Act3RuntimeHash(current)!=bp::Act3RuntimeHash(clone),
        "Act 3 hash covers runtime tick");
}

struct Runner {
    Runner() noexcept {
        TestDefinitionsExact();
        TestIndexPriorityExhaustive();
        TestWitnessResults();
        TestCurrentCycleAndAngles();
        TestPopulationAndPhysicalMass();
        TestChecksumAndWitnessProcesses();
        TestReachabilityPixelsAndHash();
        if (gFailures!=0) {
            std::printf("T08B_FAIL\tchecks=%u\tfailures=%u\n",gChecks,gFailures);
            std::exit(1);
        }
        std::printf("T08B_PASS\tchecks=%u\tindex_cases=64\twitness_cases=16\tcycle_ticks=454\n",
            gChecks);
    }
};

Runner gRunner;

}  // namespace
