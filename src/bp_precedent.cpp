#include "bp_precedent.h"

#include "bp_claim_ledger.h"
#include "bp_font.h"
#include "bp_physical_ledger.h"

#include <cstdint>
#include <cstring>

namespace bp {
namespace {

constexpr std::uint8_t Account(PhysicalAccount value) noexcept {
    return static_cast<std::uint8_t>(value);
}

constexpr std::uint8_t Action(PlatformAction value) noexcept {
    return static_cast<std::uint8_t>(value);
}

constexpr std::uint8_t PolicyIndex(PrecedentAction value) noexcept {
    return static_cast<std::uint8_t>(value);
}

constexpr bool Pressed(const InputFrame& input,PlatformAction action) noexcept {
    return input.actions[Action(action)].pressed!=0;
}

constexpr bool Held(const InputFrame& input,PlatformAction action) noexcept {
    return input.actions[Action(action)].held!=0;
}

constexpr RectPixels kCovers[kPrecedentCoverCount]={
    {76,42,24,8},{124,42,24,8},{172,42,24,8},{220,42,24,8},
    {76,130,24,8},{124,130,24,8},{172,130,24,8},{220,130,24,8},
    {140,73,8,34},{188,73,8,34},
};

constexpr Vec2Q8 kNeutrals[kPrecedentNeutralCount]={
    {80*kQ8One,70*kQ8One},{104*kQ8One,70*kQ8One},
    {128*kQ8One,70*kQ8One},{152*kQ8One,70*kQ8One},
    {176*kQ8One,70*kQ8One},{200*kQ8One,70*kQ8One},
    {224*kQ8One,70*kQ8One},{248*kQ8One,70*kQ8One},
    {80*kQ8One,110*kQ8One},{104*kQ8One,110*kQ8One},
    {128*kQ8One,110*kQ8One},{152*kQ8One,110*kQ8One},
    {176*kQ8One,110*kQ8One},{200*kQ8One,110*kQ8One},
    {224*kQ8One,110*kQ8One},{248*kQ8One,110*kQ8One},
};

constexpr RectPixels kTethers[3]={
    {216,66,6,6},{238,87,6,6},{216,108,6,6},
};

constexpr Vec2Q8 kVotePositions[3]={
    {112*kQ8One,90*kQ8One},{160*kQ8One,90*kQ8One},{208*kQ8One,90*kQ8One},
};

constexpr const char* kSubjects[9]={
    "PATCH","PAIR","VESSEL","KEEPER","ADAPTER","FRAME","INDEX","WITNESS","CURRENT",
};

constexpr const char* kChoices[9][3]={
    {"RECLAIM","CONTINUE","UNMARK"},
    {"SELECT","SEPARATE","LINK"},
    {"CLAIM","RETURN","RELEASE"},
    {"PRIVATE","MAINTAIN","COMMON"},
    {"EQUAL","NEED","OUTPUT"},
    {"SELF","RESTORE","OPEN"},
    {"OPTIMIZE","REFUSE","REWRITE"},
    {"PRESERVE","COMPRESS","ERASE"},
    {"HOLD","RETURN","CIRCULATE"},
};

constexpr const char* kRules[4][3]={
    {"ORIGIN HAS PRIORITY","A PROCESS MAY CONTINUE","A SELF REQUIRES OTHERS"},
    {"EACH SHARE IS EQUAL","THE LEAST RECEIVE FIRST","OUTPUT EARNS PRIORITY"},
    {"SAME CASE, SAME ANSWER","THE AUDITOR MAY OVERRIDE","THE AFFECTED MAY CHALLENGE"},
    {"NAMES AND LOSSES REMAIN","ONLY COUNTS REMAIN","NO CASE MAY SPEAK"},
};

struct ScoreBucket {
    std::int32_t score;
    EncounterId latest;
    ChoiceId choice;
};

void AddScore(ScoreBucket (&buckets)[3],std::uint8_t policy,std::int32_t score,
    EncounterId encounter,ChoiceId choice) noexcept {
    buckets[policy].score+=score;
    if (static_cast<std::uint8_t>(encounter)>=static_cast<std::uint8_t>(buckets[policy].latest)) {
        buckets[policy].latest=encounter;
        buckets[policy].choice=choice;
    }
}

std::uint8_t Winner(const ScoreBucket (&buckets)[3]) noexcept {
    std::uint8_t winner=0;
    for (std::uint8_t i=1;i<3;++i) {
        if (buckets[i].score>buckets[winner].score
            || (buckets[i].score==buckets[winner].score
                && static_cast<std::uint8_t>(buckets[i].latest)
                    >static_cast<std::uint8_t>(buckets[winner].latest))) winner=i;
    }
    return winner;
}

std::uint16_t ProjectileCount(const CombatState& combat) noexcept {
    std::uint16_t count=0;
    for (const ProjectileSlot& shot:combat.projectiles) if (shot.active!=0) ++count;
    return count;
}

Vec2Q8 DirectionTo(Vec2Q8 from,Vec2Q8 to) noexcept {
    return NormalizeInput(static_cast<std::int16_t>(
        (to.x-from.x)/kQ8One),static_cast<std::int16_t>((to.y-from.y)/kQ8One));
}

Vec2Q8 AimDirection(const PrecedentRuntime& state,const InputFrame& input) noexcept {
    Vec2Q8 raw{};
    if (input.device==InputDevice::KeyboardMouse) {
        raw=Vec2Q8{static_cast<std::int32_t>(input.mouseX)*kQ8One
                -state.combat.actors[0].position.x,
            static_cast<std::int32_t>(input.mouseY)*kQ8One
                -state.combat.actors[0].position.y};
    } else raw=Vec2Q8{input.aimX,input.aimY};
    if (raw.x==0 && raw.y==0) return state.combat.actors[0].aimDirectionQ15;
    return DirectionTo(Vec2Q8{0,0},raw);
}

void ClearAction(InputFrame& input,PlatformAction action) noexcept {
    input.actions[Action(action)]=ButtonState{};
}

bool CanStartDash(const PrecedentRuntime& state) noexcept {
    return state.movement.impulseInstalled!=0 && state.movement.dashCooldown==0
        && state.movement.dashTicksRemaining==0 && state.movement.chargeActive==0
        && state.movement.reclaimActive==0 && state.movement.hudChannelActive==0;
}

void ClearInterrupted(PrecedentRuntime& state) noexcept {
    state.movement.chargeActive=0;
    state.movement.reclaimActive=0;
    state.movement.hudChannelActive=0;
    if (state.combat.charges[0].active!=0)
        RollbackCharge(state.combat,state.movement.physicalLedger,CombatSide::Player);
}

bool ResolveActorCovers(CombatActor& actor,std::int32_t radius,
    const CombatState& combat) noexcept {
    bool collided=false;
    for (const CoverSlot& cover:combat.covers) {
        if (cover.active==0) continue;
        const CollisionResolution hit=ResolveCircleAgainstAabb(actor.position,radius,cover.bounds);
        if (!hit.collided) continue;
        collided=true;
        actor.position.x+=hit.shiftXQ8;
        actor.position.y+=hit.shiftYQ8;
        if (hit.shiftXQ8!=0) actor.velocity.x=0;
        if (hit.shiftYQ8!=0) actor.velocity.y=0;
    }
    return collided;
}

std::int32_t Abs32(std::int32_t value) noexcept { return value<0?-value:value; }

std::int32_t Q8Term(std::int32_t coefficient,std::int32_t observation) noexcept {
    return static_cast<std::int32_t>(
        static_cast<std::int64_t>(coefficient)*observation/256);
}

std::uint32_t DistancePixels(Vec2Q8 a,Vec2Q8 b) noexcept {
    const std::int64_t dx=a.x-b.x;
    const std::int64_t dy=a.y-b.y;
    return IntegerSqrt(static_cast<std::uint64_t>(dx*dx+dy*dy))/kQ8One;
}

bool HasNeutralNear(const PrecedentRuntime& state,Vec2Q8 point,std::uint32_t radius) noexcept {
    for (const NeutralSlot& neutral:state.combat.neutrals)
        if (neutral.active!=0 && DistancePixels(neutral.position,point)<=radius) return true;
    return false;
}

bool CoverBlocksMidpoint(const PrecedentRuntime& state,Vec2Q8 a,Vec2Q8 b) noexcept {
    const std::int32_t x=(a.x+b.x)/(2*kQ8One);
    const std::int32_t y=(a.y+b.y)/(2*kQ8One);
    for (const CoverSlot& cover:state.combat.covers)
        if (cover.active!=0 && x>=cover.bounds.x && x<cover.bounds.x+cover.bounds.width
            && y>=cover.bounds.y && y<cover.bounds.y+cover.bounds.height) return true;
    return false;
}

void SetAction(PrecedentRuntime& state,PrecedentAction action) noexcept {
    if (state.action==action) return;
    if (state.combat.charges[1].active!=0)
        RollbackCharge(state.combat,state.movement.physicalLedger,CombatSide::Opponent);
    state.action=action;
    state.phaseTicks=0;
}

bool RemoveNeutralMass(NeutralSlot& neutral,std::uint16_t amount) noexcept {
    if (amount>neutral.mass) return false;
    auto consume=[&amount](std::uint16_t& value) noexcept {
        const std::uint16_t take=value<amount?value:amount;
        value=static_cast<std::uint16_t>(value-take);
        amount=static_cast<std::uint16_t>(amount-take);
    };
    const std::uint16_t tagged=static_cast<std::uint16_t>(neutral.playerMissMass
        +neutral.opponentMissMass+neutral.protocolTagMass+neutral.patchFeedMass
        +neutral.adapterEscrowMass+neutral.structureMass+neutral.unscoredMass);
    std::uint16_t general=static_cast<std::uint16_t>(neutral.mass-tagged);
    const std::uint16_t generalTake=general<amount?general:amount;
    amount=static_cast<std::uint16_t>(amount-generalTake);
    consume(neutral.structureMass); consume(neutral.opponentMissMass);
    consume(neutral.playerMissMass); consume(neutral.protocolTagMass);
    consume(neutral.patchFeedMass); consume(neutral.adapterEscrowMass);
    consume(neutral.unscoredMass);
    const std::uint16_t removed=static_cast<std::uint16_t>(generalTake
        +(tagged-(neutral.playerMissMass+neutral.opponentMissMass
            +neutral.protocolTagMass+neutral.patchFeedMass
            +neutral.adapterEscrowMass+neutral.structureMass+neutral.unscoredMass)));
    neutral.mass=static_cast<std::uint16_t>(neutral.mass-removed);
    if (neutral.mass==0) neutral=NeutralSlot{};
    return amount==0;
}

bool SpawnNeutralFrom(PrecedentRuntime& state,PhysicalAccount from,std::uint16_t mass,
    Vec2Q8 position) noexcept {
    if (mass==0 || state.movement.physicalLedger.mass[Account(from)]<mass) return false;
    PrecedentRuntime candidate=state;
    SlotHandle handle{};
    if (!AddNeutral(candidate.combat,position,mass,0,0,handle)
        || !PhysicalTransfer(candidate.movement.physicalLedger,from,
            PhysicalAccount::Neutral,mass)) return false;
    if (from==PhysicalAccount::OpponentShell)
        candidate.combat.neutrals[handle.index].structureMass=mass;
    if (!CombatStateValid(candidate.combat,candidate.movement.physicalLedger)) return false;
    state=candidate;
    return true;
}

bool MoveNeutralToShell(PrecedentRuntime& state,CombatSide side,std::uint16_t mass) noexcept {
    PrecedentRuntime candidate=state;
    std::uint32_t available=0;
    for (const NeutralSlot& neutral:candidate.combat.neutrals)
        if (neutral.active!=0) available+=neutral.mass;
    if (available<mass) return false;
    std::uint16_t remaining=mass;
    while (remaining>0) {
        std::uint16_t selected=kInvalidSlotIndex;
        std::uint32_t stable=0xFFFFFFFFu;
        for (std::uint16_t i=0;i<kNeutralCapacity;++i) {
            const NeutralSlot& neutral=candidate.combat.neutrals[i];
            if (neutral.active!=0 && neutral.stableId<stable) {
                stable=neutral.stableId; selected=i;
            }
        }
        if (selected==kInvalidSlotIndex) return false;
        NeutralSlot& neutral=candidate.combat.neutrals[selected];
        const std::uint16_t amount=neutral.mass<remaining?neutral.mass:remaining;
        if (!RemoveNeutralMass(neutral,amount)) return false;
        remaining=static_cast<std::uint16_t>(remaining-amount);
    }
    const PhysicalAccount shell=side==CombatSide::Player
        ?PhysicalAccount::PlayerShell:PhysicalAccount::OpponentShell;
    if (!PhysicalTransfer(candidate.movement.physicalLedger,
            PhysicalAccount::Neutral,shell,mass)
        || !CombatStateValid(candidate.combat,candidate.movement.physicalLedger)) return false;
    state=candidate;
    return true;
}

std::uint8_t ActiveTethers(const PrecedentRuntime& state) noexcept {
    std::uint8_t count=0;
    for (SlotHandle handle:state.tetherNodes) if (CoverHandleValid(state.combat,handle)) ++count;
    return count;
}

PrecedentAction VoteAction(const PrecedentRuntime& state) noexcept {
    std::uint16_t votes[3]{}; // COMMON, PLAYER, OPPONENT
    constexpr std::uint32_t radius=24;
    for (Vec2Q8 node:kVotePositions) {
        std::uint32_t mass[3]{};
        if (DistancePixels(state.combat.actors[0].position,node)<=radius)
            mass[1]+=state.movement.physicalLedger.mass[Account(PhysicalAccount::PlayerShell)]+16;
        if (DistancePixels(state.combat.actors[1].position,node)<=radius)
            mass[2]+=state.movement.physicalLedger.mass[Account(PhysicalAccount::OpponentShell)]+16;
        for (const ProjectileSlot& shot:state.combat.projectiles) {
            if (shot.active==0 || DistancePixels(shot.position,node)>radius) continue;
            mass[shot.owner==CombatSide::Player?1:2]+=shot.mass;
        }
        for (const NeutralSlot& neutral:state.combat.neutrals)
            if (neutral.active!=0 && DistancePixels(neutral.position,node)<=radius)
                mass[0]+=neutral.mass;
        for (const CoverSlot& cover:state.combat.covers) {
            if (cover.active==0) continue;
            const Vec2Q8 center{
                (cover.bounds.x+cover.bounds.width/2)*kQ8One,
                (cover.bounds.y+cover.bounds.height/2)*kQ8One};
            if (DistancePixels(center,node)<=radius) mass[0]+=cover.mass;
        }
        std::uint8_t winner=0;
        if (mass[1]>mass[winner]) winner=1;
        if (mass[2]>mass[winner]) winner=2;
        ++votes[winner];
    }
    std::uint8_t winner=0;
    if (votes[1]>votes[winner]) winner=1;
    if (votes[2]>votes[winner]) winner=2;
    return winner==0?PrecedentAction::Reclaim:
        winner==1?PrecedentAction::Tap:PrecedentAction::Claim;
}

bool ApplyDistribution(PrecedentRuntime& state) noexcept {
    if (state.phase<1) return true;
    const DistributionPolicy policy=static_cast<DistributionPolicy>(
        state.policy.profile.distribution);
    PhysicalLedger& ledger=state.movement.physicalLedger;
    if (policy==DistributionPolicy::Equality && state.encounterTick%600u==0) {
        const std::uint16_t player=ledger.mass[Account(PhysicalAccount::PlayerShell)];
        const std::uint16_t opponent=ledger.mass[Account(PhysicalAccount::OpponentShell)];
        const std::uint16_t difference=player>opponent?player-opponent:opponent-player;
        std::uint16_t amount=static_cast<std::uint16_t>((difference/2u)/8u*8u);
        if (amount>32) amount=32;
        if (amount==0) return true;
        const PhysicalAccount from=player>opponent?PhysicalAccount::PlayerShell:
            PhysicalAccount::OpponentShell;
        return SpawnNeutralFrom(state,from,amount,Vec2Q8{160*kQ8One,90*kQ8One});
    }
    if (policy==DistributionPolicy::Need && state.encounterTick%600u==0) {
        const std::uint16_t player=ledger.mass[Account(PhysicalAccount::PlayerShell)];
        const std::uint16_t opponent=ledger.mass[Account(PhysicalAccount::OpponentShell)];
        const CombatSide lower=player<=opponent?CombatSide::Player:CombatSide::Opponent;
        if (MoveNeutralToShell(state,lower,16)) return true;
        PrecedentRuntime candidate=state;
        const PhysicalAccount from=lower==CombatSide::Player?PhysicalAccount::OpponentShell:
            PhysicalAccount::PlayerShell;
        const PhysicalAccount to=lower==CombatSide::Player?PhysicalAccount::PlayerShell:
            PhysicalAccount::OpponentShell;
        if (candidate.movement.physicalLedger.mass[Account(from)]<16
            || !PhysicalTransfer(candidate.movement.physicalLedger,from,to,16)
            || !CombatStateValid(candidate.combat,candidate.movement.physicalLedger)) return false;
        state=candidate;
    }
    if (policy==DistributionPolicy::Utility && state.encounterTick%480u==0) {
        const std::uint32_t player=state.outputDamage[0]+state.outputReclaim[0]
            +state.outputMovementQ8[0]/(8u*kQ8One);
        const std::uint32_t opponent=state.outputDamage[1]+state.outputReclaim[1]
            +state.outputMovementQ8[1]/(8u*kQ8One);
        const CombatSide lower=player<=opponent?CombatSide::Player:CombatSide::Opponent;
        if (lower==CombatSide::Player && OpenHudLoanCount(state.hudLoan)>0) {
            if (!ShortenOpenHudMaturity(state.hudLoan,120)) return false;
        } else {
            const PhysicalAccount from=lower==CombatSide::Player?PhysicalAccount::PlayerShell:
                PhysicalAccount::OpponentShell;
            if (!SpawnNeutralFrom(state,from,16,
                    lower==CombatSide::Player?state.combat.actors[0].position:
                    state.combat.actors[1].position)) return false;
        }
        std::memset(state.outputDamage,0,sizeof(state.outputDamage));
        std::memset(state.outputReclaim,0,sizeof(state.outputReclaim));
        std::memset(state.outputMovementQ8,0,sizeof(state.outputMovementQ8));
    }
    return true;
}

void ApplyIdentity(PrecedentRuntime& state) noexcept {
    const IdentityPolicy policy=static_cast<IdentityPolicy>(state.policy.profile.identity);
    if (policy==IdentityPolicy::Material) {
        if (state.encounterTick%720u==0) {
            const std::uint16_t shell=state.movement.physicalLedger.mass[
                Account(PhysicalAccount::PlayerShell)];
            state.markedOriginMass=shell<16?shell:16;
            state.markedOriginTicks=90;
        } else if (state.markedOriginTicks>0 && --state.markedOriginTicks==0)
            state.markedOriginMass=0;
    } else if (policy==IdentityPolicy::Continuity) {
        if (state.encounterTick%600u==0) state.forkTicks=180;
        else if (state.forkTicks>0) --state.forkTicks;
    }
}

void ApplyMemory(PrecedentRuntime& state) noexcept {
    if (state.phase<2) { state.echoTicks=0; return; }
    const MemoryPolicy policy=static_cast<MemoryPolicy>(state.policy.profile.memory);
    if (policy==MemoryPolicy::None) { state.echoTicks=0; return; }
    if (state.echoTicks==0 && state.encounterTick%600u==0) {
        state.echoTicks=180;
        state.echoIndex=static_cast<std::uint8_t>((state.encounterTick/600u-1u)%9u);
    }
    else if (state.echoTicks>0) --state.echoTicks;
}

void UpdateJudgment(PrecedentRuntime& state) noexcept {
    if (state.encounterTick%12u!=0) return;
    if (state.pendingValid!=0) SetAction(state,state.pendingAction);
    const PrecedentActionScores scores=EvaluatePrecedentActions(state);
    std::uint8_t best=0;
    for (std::uint8_t i=1;i<PolicyIndex(PrecedentAction::Count);++i)
        if (scores.value[i]>scores.value[best]) best=i;
    const std::int32_t current=scores.value[PolicyIndex(state.action)];
    if (scores.value[best]>=current+38) {
        state.pendingAction=static_cast<PrecedentAction>(best);
        state.pendingValid=1;
    } else state.pendingValid=0;
}

void UpdateGovernance(PrecedentRuntime& state) noexcept {
    if (state.phase<2) { UpdateJudgment(state); return; }
    const GovernancePolicy policy=static_cast<GovernancePolicy>(state.policy.profile.governance);
    if (policy==GovernancePolicy::Rule) {
        const std::uint16_t duration=state.action==PrecedentAction::Foreclose?75u:60u;
        if (state.phaseTicks>=duration) {
            const std::uint8_t next=static_cast<std::uint8_t>(
                (PolicyIndex(state.action)+1u)%PolicyIndex(PrecedentAction::Count));
            SetAction(state,static_cast<PrecedentAction>(next));
        }
    } else if (policy==GovernancePolicy::Judgment) {
        UpdateJudgment(state);
    } else if (policy==GovernancePolicy::Commons && state.encounterTick%12u==0) {
        SetAction(state,VoteAction(state));
    }
}

bool AttackAffordable(const PrecedentRuntime& state,std::uint16_t mass) noexcept {
    const std::uint16_t shell=state.movement.physicalLedger.mass[
        Account(PhysicalAccount::OpponentShell)];
    std::uint16_t backedCoverMass=0;
    for (const CoverSlot& cover:state.combat.covers)
        if (cover.active!=0 && cover.backingAccount==PhysicalAccount::OpponentShell)
            backedCoverMass=static_cast<std::uint16_t>(backedCoverMass+cover.mass);
    const bool predictedCoreHit=state.movement.physicalLedger.mass[
        Account(PhysicalAccount::PlayerShell)]<=40
        && !CoverBlocksMidpoint(state,state.combat.actors[0].position,
            state.combat.actors[1].position);
    return ProjectileCount(state.combat)<kProjectileCapacity-1u
        && shell>=static_cast<std::uint16_t>(mass+backedCoverMass)
        && (shell>=mass+41u || predictedCoreHit);
}

bool SpawnOpponentAttack(PrecedentRuntime& state,std::uint16_t mass) noexcept {
    if (!AttackAffordable(state,mass)) { SetAction(state,PrecedentAction::Move); return true; }
    return SpawnScriptedProjectile(state.combat,state.movement.physicalLedger,
        CombatSide::Opponent,mass,state.combat.actors[1].position,
        DirectionTo(state.combat.actors[1].position,state.combat.actors[0].position),
        ProjectileSpeedQ8(mass));
}

bool ExecuteAction(PrecedentRuntime& state) noexcept {
    ++state.phaseTicks;
    CombatActor& opponent=state.combat.actors[1];
    if (state.action==PrecedentAction::Evade || state.action==PrecedentAction::Move) {
        Vec2Q8 direction=DirectionTo(opponent.position,state.combat.actors[0].position);
        if (state.action==PrecedentAction::Evade) { direction.x=-direction.x; direction.y=-direction.y; }
        const std::int32_t speed=LargeOpponentMaximumSpeedQ8(
            state.movement.physicalLedger.mass[Account(PhysicalAccount::OpponentShell)]);
        opponent.velocity=Vec2Q8{
            RoundDivideSigned(static_cast<std::int64_t>(direction.x)*speed,kInputUnit),
            RoundDivideSigned(static_cast<std::int64_t>(direction.y)*speed,kInputUnit)};
    } else opponent.velocity=Vec2Q8{};
    if (state.phaseTicks!=1) {
        if (state.action==PrecedentAction::Foreclose && state.combat.charges[1].active!=0) {
            const std::uint16_t available=static_cast<std::uint16_t>(
                state.movement.physicalLedger.mass[Account(PhysicalAccount::OpponentShell)]
                +state.combat.charges[1].mass);
            if (available<89 || ProjectileCount(state.combat)>=kProjectileCapacity-1u) {
                if (!RollbackCharge(state.combat,state.movement.physicalLedger,
                        CombatSide::Opponent)) return false;
                SetAction(state,PrecedentAction::Move);
                return true;
            }
            if (!AdvanceCharge(state.combat,state.movement.physicalLedger,
                    CombatSide::Opponent)) return false;
            if (state.phaseTicks>=75) {
                if (state.combat.charges[1].mass!=48
                    || !ReleaseCharge(state.combat,state.movement.physicalLedger,
                        CombatSide::Opponent)) return false;
                SetAction(state,PrecedentAction::Move);
            }
        }
        return true;
    }
    switch (state.action) {
    case PrecedentAction::Tap: return SpawnOpponentAttack(state,8);
    case PrecedentAction::Cut: return SpawnOpponentAttack(state,16);
    case PrecedentAction::Claim: return SpawnOpponentAttack(state,32);
    case PrecedentAction::Foreclose:
        if (!AttackAffordable(state,48)) { SetAction(state,PrecedentAction::Move); return true; }
        opponent.aimDirectionQ15=DirectionTo(opponent.position,state.combat.actors[0].position);
        return BeginCharge(state.combat,state.movement.physicalLedger,
            CombatSide::Opponent,opponent.aimDirectionQ15);
    default: return true;
    }
}

void MoveOpponent(PrecedentRuntime& state) noexcept {
    CombatActor& opponent=state.combat.actors[1];
    opponent.position.x+=opponent.velocity.x;
    opponent.position.y+=opponent.velocity.y;
    const std::int32_t radius=ActorRadiusPixels(state.movement.physicalLedger.mass[
        Account(PhysicalAccount::OpponentShell)]);
    ClampCircleToRect(opponent.position,radius,kPlayInterior);
    ResolveActorCovers(opponent,radius,state.combat);
}

void Centered(PixelBuffer buffer,std::int32_t y,const char* text,PaletteColor color) noexcept {
    DrawLatin(buffer,(kRenderWidth-MeasureLatin(text))/2,y,text,color);
}

void RenderBattle(PixelBuffer buffer,const PrecedentRuntime& state) noexcept {
    Clear(buffer,PaletteColor::Background);
    DrawLedgerGrid(buffer);
    for (const CoverSlot& cover:state.combat.covers)
        if (cover.active!=0) FillRect(buffer,RectI{cover.bounds.x,cover.bounds.y,
            cover.bounds.width,cover.bounds.height},PaletteColor::Cover);
    for (const NeutralSlot& neutral:state.combat.neutrals)
        if (neutral.active!=0) FillRect(buffer,RectI{neutral.position.x/kQ8One-1,
            neutral.position.y/kQ8One-1,3,3},PaletteColor::Neutral);
    for (const ProjectileSlot& shot:state.combat.projectiles)
        if (shot.active!=0) FillRect(buffer,RectI{shot.position.x/kQ8One-1,
            shot.position.y/kQ8One-1,3,3},shot.owner==CombatSide::Player
                ?PaletteColor::Player:PaletteColor::Opponent);
    FillRect(buffer,RectI{state.combat.actors[0].position.x/kQ8One-2,
        state.combat.actors[0].position.y/kQ8One-2,5,5},PaletteColor::Core);
    FillRect(buffer,RectI{state.combat.actors[1].position.x/kQ8One-3,
        state.combat.actors[1].position.y/kQ8One-3,7,7},PaletteColor::Opponent);
    if (state.forkTicks>0) StrokeRect(buffer,RectI{
        state.combat.actors[1].position.x/kQ8One+10,
        state.combat.actors[1].position.y/kQ8One-3,7,7},PaletteColor::UiInactive);
    if (state.forkTicks>0 && state.globalTick%30u<6u)
        DrawLine(buffer,state.combat.actors[1].position.x/kQ8One+10,
            state.combat.actors[1].position.y/kQ8One,
            state.combat.actors[1].position.x/kQ8One+3,
            state.combat.actors[1].position.y/kQ8One,PaletteColor::Warning);
    if (state.markedOriginTicks>0)
        StrokeRect(buffer,RectI{state.combat.actors[0].position.x/kQ8One-5,
            state.combat.actors[0].position.y/kQ8One-5,11,11},PaletteColor::Warning);
    DrawLatin(buffer,14,6,"PRECEDENT",PaletteColor::Warning);
    constexpr const char* phaseName[3]={"IDENTITY","DISTRIBUTION","GOVERNANCE / MEMORY"};
    DrawLatin(buffer,86,6,phaseName[state.phase],PaletteColor::Neutral);
    if (state.phase==1 && state.phaseDisplayTicks>0)
        DrawLatin(buffer,84,158,kRules[1][state.policy.profile.distribution],PaletteColor::Warning);
    if (state.echoTicks>0) {
        constexpr const char* actTotals[3]={"ACT 1 TOTAL","ACT 2 TOTAL","ACT 3 TOTAL"};
        const char* echo=state.policy.profile.memory==static_cast<std::uint8_t>(MemoryPolicy::Full)
            ?kSubjects[state.echoIndex]:actTotals[state.echoIndex/3u];
        DrawLatin(buffer,14,158,echo,PaletteColor::UiInactive);
    }
    if (state.exceptionTicks>0) {
        Centered(buffer,142,"EXCEPTION REQUESTED",PaletteColor::Warning);
        if (state.exceptionDeniedShown!=0)
            Centered(buffer,152,"EXCEPTIONS WERE NOT IN YOUR DATA",PaletteColor::UiInactive);
    }
}

}  // namespace

ChoiceId ChoiceForEncounter(const NarrativeState& narrative,EncounterId encounter) noexcept {
    const std::uint8_t index=static_cast<std::uint8_t>(encounter);
    if (index>=kSettlementEncounterCount) return ChoiceId::Count;
    const std::uint8_t choice=static_cast<std::uint8_t>((narrative.choiceBits>>(index*2u))&3u);
    return choice<3?static_cast<ChoiceId>(choice):ChoiceId::Count;
}

bool DerivePolicyDecision(const NarrativeState& narrative,PolicyDecision& out) noexcept {
    for (std::uint8_t i=0;i<kSettlementEncounterCount;++i)
        if (ChoiceForEncounter(narrative,static_cast<EncounterId>(i))==ChoiceId::Count) return false;
    ScoreBucket identity[3]{};
    ScoreBucket distribution[3]{};
    ScoreBucket governance[3]{};
    for (ScoreBucket& bucket:identity) bucket.latest=EncounterId::Patch;
    for (ScoreBucket& bucket:distribution) bucket.latest=EncounterId::Patch;
    for (ScoreBucket& bucket:governance) bucket.latest=EncounterId::Patch;
    const ChoiceId patch=ChoiceForEncounter(narrative,EncounterId::Patch);
    const ChoiceId pair=ChoiceForEncounter(narrative,EncounterId::Pair);
    AddScore(identity,static_cast<std::uint8_t>(patch),2,EncounterId::Patch,patch);
    AddScore(identity,static_cast<std::uint8_t>(pair),2,EncounterId::Pair,pair);
    constexpr std::uint8_t keeperMap[3]={2,1,0};
    constexpr std::uint8_t adapterMap[3]={0,1,2};
    constexpr std::uint8_t indexMap[3]={2,1,0};
    const ChoiceId keeper=ChoiceForEncounter(narrative,EncounterId::Keeper);
    const ChoiceId adapter=ChoiceForEncounter(narrative,EncounterId::Adapter);
    const ChoiceId index=ChoiceForEncounter(narrative,EncounterId::Index);
    AddScore(distribution,keeperMap[static_cast<std::uint8_t>(keeper)],1,
        EncounterId::Keeper,keeper);
    AddScore(distribution,adapterMap[static_cast<std::uint8_t>(adapter)],3,
        EncounterId::Adapter,adapter);
    AddScore(distribution,indexMap[static_cast<std::uint8_t>(index)],2,
        EncounterId::Index,index);
    constexpr std::uint8_t governanceMap[3]={1,0,2};
    const ChoiceId frame=ChoiceForEncounter(narrative,EncounterId::Frame);
    const ChoiceId current=ChoiceForEncounter(narrative,EncounterId::Current);
    AddScore(governance,governanceMap[static_cast<std::uint8_t>(frame)],2,
        EncounterId::Frame,frame);
    AddScore(governance,governanceMap[static_cast<std::uint8_t>(current)],2,
        EncounterId::Current,current);
    const std::uint8_t identityWinner=Winner(identity);
    const std::uint8_t distributionWinner=Winner(distribution);
    const std::uint8_t governanceWinner=Winner(governance);
    const ChoiceId witness=ChoiceForEncounter(narrative,EncounterId::Witness);
    out=PolicyDecision{};
    out.profile=PolicyProfile{identityWinner,distributionWinner,governanceWinner,
        static_cast<std::uint8_t>(witness)};
    out.decisiveEncounter[0]=identity[identityWinner].latest;
    out.decisiveEncounter[1]=distribution[distributionWinner].latest;
    out.decisiveEncounter[2]=governance[governanceWinner].latest;
    out.decisiveEncounter[3]=EncounterId::Witness;
    out.decisiveChoice[0]=identity[identityWinner].choice;
    out.decisiveChoice[1]=distribution[distributionWinner].choice;
    out.decisiveChoice[2]=governance[governanceWinner].choice;
    out.decisiveChoice[3]=witness;
    return true;
}

PolicyAuditView BuildPolicyAuditView(const PolicyDecision& decision) noexcept {
    PolicyAuditView view{};
    for (std::uint8_t axis=0;axis<4;++axis) {
        const std::uint8_t encounter=static_cast<std::uint8_t>(decision.decisiveEncounter[axis]);
        const std::uint8_t choice=static_cast<std::uint8_t>(decision.decisiveChoice[axis]);
        const std::uint8_t policy=axis==0?decision.profile.identity:
            axis==1?decision.profile.distribution:
            axis==2?decision.profile.governance:decision.profile.memory;
        view.rows[axis].subject=axis==3 && policy==static_cast<std::uint8_t>(MemoryPolicy::None)
            ?"CASE // HASH":kSubjects[encounter];
        view.rows[axis].choice=kChoices[encounter][choice];
        view.rows[axis].rule=kRules[axis][policy];
    }
    return view;
}

std::uint8_t PrecedentPhaseForShell(std::uint16_t shell) noexcept {
    if (static_cast<std::uint32_t>(shell)*100u
        >=static_cast<std::uint32_t>(kPrecedentInitialShell)*67u) return 0;
    if (static_cast<std::uint32_t>(shell)*100u
        >=static_cast<std::uint32_t>(kPrecedentInitialShell)*34u) return 1;
    return 2;
}

PrecedentActionScores EvaluatePrecedentActions(const PrecedentRuntime& state) noexcept {
    PrecedentActionScores scores{};
    const std::uint16_t opponentShell=state.movement.physicalLedger.mass[
        Account(PhysicalAccount::OpponentShell)];
    const std::uint16_t playerShell=state.movement.physicalLedger.mass[
        Account(PhysicalAccount::PlayerShell)];
    std::uint32_t threatMass=0;
    for (const ProjectileSlot& shot:state.combat.projectiles)
        if (shot.active!=0 && shot.owner==CombatSide::Player) threatMass+=shot.mass;
    const std::int32_t threat=threatMass>=64?256:static_cast<std::int32_t>(threatMass*4);
    const std::int32_t exposed=opponentShell<=40?256:0;
    const std::int32_t need=opponentShell>=128?0:static_cast<std::int32_t>(
        (128u-opponentShell)*2u);
    const std::int32_t neutralNear=HasNeutralNear(state,state.combat.actors[1].position,72)?256:0;
    const std::int32_t distance=static_cast<std::int32_t>(DistancePixels(
        state.combat.actors[0].position,state.combat.actors[1].position));
    const std::int32_t rangeFit=256-(Abs32(distance-96)*4>256?256:Abs32(distance-96)*4);
    const std::int32_t coverBlocks=CoverBlocksMidpoint(state,state.combat.actors[0].position,
        state.combat.actors[1].position)?256:0;
    const std::int32_t lineOfSight=256-coverBlocks;
    const std::int32_t playerExposed=playerShell<=40?256:0;
    const std::int32_t playerChannel=(state.combat.charges[0].active!=0
        || state.movement.reclaimActive!=0 || state.movement.hudChannelActive!=0)?256:0;
    const std::int32_t oneMinusRangeFit=256-rangeFit;
    scores.value[0]=Q8Term(410,threat)+Q8Term(205,exposed)-Q8Term(51,need);
    scores.value[2]=51+Q8Term(307,need)+Q8Term(256,neutralNear)-Q8Term(230,threat);
    scores.value[1]=64+Q8Term(230,oneMinusRangeFit)+Q8Term(154,coverBlocks);
    scores.value[3]=64+Q8Term(179,lineOfSight)+Q8Term(230,playerExposed)-Q8Term(90,threat);
    scores.value[4]=26+Q8Term(192,lineOfSight)+Q8Term(115,rangeFit)
        +Q8Term(90,playerChannel)-Q8Term(154,threat);
    scores.value[5]=Q8Term(218,lineOfSight)+Q8Term(192,playerChannel)
        -Q8Term(179,need)-Q8Term(205,threat);
    scores.value[6]=-26+Q8Term(320,playerChannel)+Q8Term(230,playerExposed)
        -Q8Term(256,need)-Q8Term(230,threat);
    return scores;
}

bool PopulatePrecedentBattlefield(PrecedentRuntime& state) noexcept {
    PrecedentRuntime candidate=state;
    if (!InitializeGameState(candidate.movement,0x50ECED09u,candidate.narrative.choiceBits))
        return false;
    candidate.movement.claimLedger=candidate.claimLedger;
    candidate.movement.narrative=candidate.narrative;
    if (!InitializePhysicalLedger(candidate.movement.physicalLedger,64,
            kPrecedentBorrowMass,320,128)) return false;
    InitializeCombatState(candidate.combat,Vec2Q8{48*kQ8One,92*kQ8One},
        Vec2Q8{264*kQ8One,90*kQ8One});
    InitializeReclaimState(candidate.reclaim);
    for (std::uint8_t i=0;i<kPrecedentCoverCount;++i)
        if (!AddCover(candidate.combat,kCovers[i],32,candidate.covers[i])) return false;
    for (std::uint8_t i=0;i<kPrecedentNeutralCount;++i)
        if (!AddNeutral(candidate.combat,kNeutrals[i],8,0,0,candidate.neutrals[i])) return false;
    for (SlotHandle& handle:candidate.tetherNodes) handle=SlotHandle{kInvalidSlotIndex,0};
    if (candidate.policy.profile.identity==static_cast<std::uint8_t>(IdentityPolicy::Relation)) {
        for (std::uint8_t i=0;i<3;++i)
            if (!AddBackedCover(candidate.combat,kTethers[i],16,
                    PhysicalAccount::OpponentShell,CoverBehavior::OwnedStructure,
                    candidate.tetherNodes[i])) return false;
    }
    candidate.voteNodes[0]=candidate.covers[1];
    candidate.voteNodes[1]=candidate.covers[8];
    candidate.voteNodes[2]=candidate.covers[6];
    candidate.scene=PrecedentScene::Playing;
    candidate.action=PrecedentAction::Evade;
    candidate.pendingAction=PrecedentAction::Evade;
    candidate.encounterTick=0;
    candidate.sceneTicks=0;
    candidate.phaseTicks=0;
    candidate.phaseDisplayTicks=0;
    candidate.exceptionTicks=0;
    candidate.phase=0;
    candidate.borrowedActive=1;
    candidate.borrowedFoldedMass=0;
    candidate.pendingValid=0;
    candidate.exceptionShown=0;
    candidate.exceptionDeniedShown=0;
    candidate.lastPlayerPolicyAction=0;
    candidate.lastPlayerPolicyTick=0;
    candidate.movement.playerPosition=candidate.combat.actors[0].position;
    candidate.movement.playerVelocity=Vec2Q8{};
    if (!CombatStateValid(candidate.combat,candidate.movement.physicalLedger)
        || !ClaimLedgerValid(candidate.claimLedger)) return false;
    state=candidate;
    return true;
}

bool FoldPrecedentBorrow(PrecedentRuntime& state) noexcept {
    if (state.borrowedActive==0) return state.borrowedFoldedMass==kPrecedentBorrowMass;
    PrecedentRuntime candidate=state;
    PhysicalLedger& ledger=candidate.movement.physicalLedger;
    CombatState& combat=candidate.combat;
    std::uint16_t remaining=kPrecedentBorrowMass;
    auto transfer=[&](PhysicalAccount from,std::uint16_t wanted) noexcept {
        const std::uint16_t amount=wanted<remaining?wanted:remaining;
        if (amount==0) return true;
        if (!PhysicalTransfer(ledger,from,PhysicalAccount::LockedBands,amount)) return false;
        remaining=static_cast<std::uint16_t>(remaining-amount);
        return true;
    };
    if (!transfer(PhysicalAccount::OpponentCore,ledger.mass[Account(PhysicalAccount::OpponentCore)]))
        return false;
    if (!transfer(PhysicalAccount::OpponentShell,ledger.mass[Account(PhysicalAccount::OpponentShell)]))
        return false;
    if (combat.charges[1].active!=0 && remaining>=combat.charges[1].mass) {
        const std::uint16_t mass=combat.charges[1].mass;
        if (!transfer(PhysicalAccount::Charge,mass)) return false;
        combat.charges[1]=ChargeSlot{};
    }
    for (ProjectileSlot& shot:combat.projectiles) {
        if (remaining==0) break;
        if (shot.active==0 || shot.owner!=CombatSide::Opponent || shot.mass>remaining) continue;
        const std::uint16_t mass=shot.mass;
        if (!transfer(PhysicalAccount::Projectile,mass)) return false;
        shot=ProjectileSlot{};
    }
    for (NeutralSlot& neutral:combat.neutrals) {
        if (remaining==0) break;
        if (neutral.active==0) continue;
        const std::uint16_t amount=neutral.mass<remaining?neutral.mass:remaining;
        if (!RemoveNeutralMass(neutral,amount)
            || !transfer(PhysicalAccount::Neutral,amount)) return false;
    }
    if (!transfer(PhysicalAccount::PlayerShell,ledger.mass[Account(PhysicalAccount::PlayerShell)]))
        return false;
    if (combat.charges[0].active!=0 && remaining>=combat.charges[0].mass) {
        const std::uint16_t mass=combat.charges[0].mass;
        if (!transfer(PhysicalAccount::Charge,mass)) return false;
        combat.charges[0]=ChargeSlot{};
    }
    for (ProjectileSlot& shot:combat.projectiles) {
        if (remaining==0) break;
        if (shot.active==0 || shot.mass>remaining) continue;
        const std::uint16_t mass=shot.mass;
        if (!transfer(PhysicalAccount::Projectile,mass)) return false;
        shot=ProjectileSlot{};
    }
    if (remaining!=0) return false;
    for (SlotHandle handle:candidate.tetherNodes) {
        if (!CoverHandleValid(combat,handle)) continue;
        combat.covers[handle.index]=CoverSlot{};
    }
    combat.phase=CombatPhase::Active;
    combat.error=CombatError::None;
    combat.actors[1].coreHit=0;
    candidate.borrowedActive=0;
    candidate.borrowedFoldedMass=kPrecedentBorrowMass;
    if (!PhysicalLedgerValid(ledger) || !CombatStateValid(combat,ledger)) return false;
    state=candidate;
    return true;
}

bool InitializePrecedentRuntime(PrecedentRuntime& state,const ClaimLedger& claims,
    const NarrativeState& narrative) noexcept {
    if (!ClaimLedgerValid(claims)) return false;
    PrecedentRuntime candidate{};
    candidate.claimLedger=claims;
    candidate.narrative=narrative;
    if (!DerivePolicyDecision(narrative,candidate.policy)
        || !InitializeGameState(candidate.movement,0x50ECED09u,narrative.choiceBits)) return false;
    candidate.movement.claimLedger=claims;
    candidate.movement.narrative=narrative;
    InitializeHudLoanSystem(candidate.hudLoan);
    candidate.scene=PrecedentScene::RoleAssign;
    candidate.action=PrecedentAction::Evade;
    candidate.pendingAction=PrecedentAction::Evade;
    state=candidate;
    return true;
}

bool TickPrecedentRuntime(PrecedentRuntime& state,const InputFrame& input) noexcept {
    if (state.scene==PrecedentScene::Fatal || state.scene==PrecedentScene::Complete)
        return state.scene!=PrecedentScene::Fatal;
    ++state.globalTick;
    if (state.scene==PrecedentScene::RoleAssign) {
        if (++state.sceneTicks>=60) {
            state.scene=PrecedentScene::PolicyAudit;
            state.sceneTicks=0;
            state.auditRow=0;
        }
        return true;
    }
    if (state.scene==PrecedentScene::PolicyAudit) {
        ++state.sceneTicks;
        if (state.sceneTicks>=90 || (state.sceneTicks>=36
                && Pressed(input,PlatformAction::Confirm))) {
            state.sceneTicks=0;
            if (++state.auditRow>=4 && !PopulatePrecedentBattlefield(state)) {
                state.scene=PrecedentScene::Fatal; return false;
            }
        }
        return true;
    }
    if (state.scene==PrecedentScene::HitFreeze) {
        if (++state.sceneTicks>=12) { state.scene=PrecedentScene::Retry; state.sceneTicks=0; }
        return true;
    }
    if (state.scene==PrecedentScene::Retry) {
        if (!PopulatePrecedentBattlefield(state)) { state.scene=PrecedentScene::Fatal; return false; }
        return true;
    }
    if (state.scene==PrecedentScene::CoreUnfold) {
        if (++state.sceneTicks>=36) {
            if (!FoldPrecedentBorrow(state)) { state.scene=PrecedentScene::Fatal; return false; }
            state.scene=PrecedentScene::Complete;
        }
        return true;
    }

    PhysicalLedger& ledger=state.movement.physicalLedger;
    if (state.movement.paused!=0) {
        if (input.focused!=0 && Pressed(input,PlatformAction::Pause)) {
            state.movement.paused=0; ClearInterrupted(state);
        }
        return true;
    }
    if (state.movement.dashCooldown>0) --state.movement.dashCooldown;
    if (state.movement.dashInvulnerableRemaining>0) --state.movement.dashInvulnerableRemaining;
    TickCombatCountdowns(state.combat);
    if (input.focused==0 || Pressed(input,PlatformAction::Pause)) {
        state.movement.paused=1; ClearInterrupted(state); return true;
    }
    InputFrame effective=input;
    if (state.movement.dashTicksRemaining>0) {
        ClearAction(effective,PlatformAction::HudReallocate);
        ClearAction(effective,PlatformAction::Dash);
        ClearAction(effective,PlatformAction::Fire);
        ClearAction(effective,PlatformAction::Reclaim);
    }
    state.movement.claimLedger=state.claimLedger;
    state.movement.narrative=state.narrative;
    state.movement.impulseInstalled=HudModuleFullyInstalled(state.hudLoan,HudModuleId::Impulse)?1u:0u;
    const GameplayIntent intent=ResolveGameplayInput(state.movement.inputArbiter,effective,
        CanStartDash(state),true);
    if (!AdvanceHudLoanEarly(state.hudLoan,ledger,
            intent.action==GameplayAction::HudReallocate,state.previousPlayerReclaim)) return false;
    state.previousPlayerReclaim=0;
    ++state.encounterTick;
    if (state.exceptionTicks>0) --state.exceptionTicks;
    std::uint8_t playerPolicyAction=0;
    if (intent.action==GameplayAction::Fire) playerPolicyAction=1;
    else if (intent.action==GameplayAction::Reclaim) playerPolicyAction=2;
    if (playerPolicyAction!=0) {
        if (state.exceptionShown==0 && state.lastPlayerPolicyAction!=0
            && playerPolicyAction!=state.lastPlayerPolicyAction
            && state.encounterTick-state.lastPlayerPolicyTick<=180u) {
            state.exceptionShown=1;
            state.exceptionTicks=180;
            if (state.policy.profile.governance
                    !=static_cast<std::uint8_t>(GovernancePolicy::Judgment))
                state.exceptionDeniedShown=1;
        }
        state.lastPlayerPolicyAction=playerPolicyAction;
        state.lastPlayerPolicyTick=state.encounterTick;
    }
    const Vec2Q8 playerBefore=state.combat.actors[0].position;
    const std::uint8_t nextPhase=PrecedentPhaseForShell(
        ledger.mass[Account(PhysicalAccount::OpponentShell)]);
    if (nextPhase!=state.phase) {
        state.phase=nextPhase;
        state.phaseDisplayTicks=nextPhase==1?120u:0u;
        state.pendingValid=0;
    } else if (state.phaseDisplayTicks>0) --state.phaseDisplayTicks;
    ApplyIdentity(state);
    ApplyMemory(state);
    UpdateGovernance(state);
    if (!ApplyDistribution(state) || !ExecuteAction(state)) return false;

    const Vec2Q8 aim=AimDirection(state,input);
    state.combat.actors[0].aimDirectionQ15=aim;
    if (state.reclaimPriorityTicks>0) --state.reclaimPriorityTicks;
    ChargeSlot& playerCharge=state.combat.charges[0];
    if (intent.action==GameplayAction::HudReallocate && playerCharge.active!=0) {
        if (!RollbackCharge(state.combat,ledger,CombatSide::Player)) return false;
    } else if (intent.action==GameplayAction::Fire && !HudFireLocked(state.hudLoan)) {
        if (playerCharge.active==0) {
            if (state.combat.actors[0].shotCooldownTicks==0
                && ledger.mass[Account(PhysicalAccount::PlayerShell)]>=8
                && ProjectileCount(state.combat)<kProjectileCapacity
                && !BeginCharge(state.combat,ledger,CombatSide::Player,aim)) return false;
        } else if (!AdvanceCharge(state.combat,ledger,CombatSide::Player)) return false;
    } else if (playerCharge.active!=0) {
        if (ProjectileCount(state.combat)>=kProjectileCapacity) {
            if (!RollbackCharge(state.combat,ledger,CombatSide::Player)) return false;
        } else if (Held(input,PlatformAction::Fire)
            || input.actions[Action(PlatformAction::Fire)].released!=0) {
            if (!ReleaseCharge(state.combat,ledger,CombatSide::Player)) return false;
            if (state.markedOriginTicks>0) state.reclaimPriorityTicks=90;
        } else if (!RollbackCharge(state.combat,ledger,CombatSide::Player)) return false;
    }
    state.movement.chargeActive=playerCharge.active!=0?1u:0u;
    if (!AdvanceGameIntent(state.movement,intent)) return false;
    state.combat.actors[0].position=state.movement.playerPosition;
    state.combat.actors[0].velocity=state.movement.playerVelocity;
    const std::int32_t playerRadius=ActorRadiusPixels(ledger.mass[
        Account(PhysicalAccount::PlayerShell)]);
    if (ResolveActorCovers(state.combat.actors[0],playerRadius,state.combat)
        && state.movement.dashTicksRemaining>0) {
        state.movement.dashTicksRemaining=0;
        state.movement.dashInvulnerableRemaining=0;
    }
    MoveOpponent(state);
    SeparateCombatActorsByMass(state.combat,ledger,kPlayInterior);
    state.movement.playerPosition=state.combat.actors[0].position;

    ReclaimTickResult reclaimed{};
    if (!TickReclaim(state.combat,ledger,state.reclaim,
            intent.action==GameplayAction::Reclaim,
            state.action==PrecedentAction::Reclaim,reclaimed,false,
            state.reclaimPriorityTicks>0)) return false;
    state.previousPlayerReclaim=reclaimed.playerMass;
    state.outputReclaim[0]=static_cast<std::uint16_t>(state.outputReclaim[0]+reclaimed.playerMass);
    state.outputReclaim[1]=static_cast<std::uint16_t>(state.outputReclaim[1]+reclaimed.opponentMass);
    if (state.policy.profile.identity==static_cast<std::uint8_t>(IdentityPolicy::Relation)
        && ActiveTethers(state)>0) state.combat.actors[1].exposureGraceTicks=1;
    if (!StepCombatProjectiles(state.combat,ledger,state.claimLedger)) return false;
    state.outputDamage[0]=static_cast<std::uint16_t>(state.outputDamage[0]
        +state.combat.shellDamageThisTick[1]);
    state.outputDamage[1]=static_cast<std::uint16_t>(state.outputDamage[1]
        +state.combat.shellDamageThisTick[0]);
    state.outputMovementQ8[0]+=static_cast<std::uint32_t>(Abs32(
        state.combat.actors[0].position.x-playerBefore.x)+Abs32(
        state.combat.actors[0].position.y-playerBefore.y));
    state.outputMovementQ8[1]+=static_cast<std::uint32_t>(Abs32(
        state.combat.actors[1].velocity.x)+Abs32(state.combat.actors[1].velocity.y));
    if (!TickNeutralDynamics(state.combat,ledger)
        || !MergeNeutralClusters(state.combat,ledger)
        || !ResolvePendingHudCommit(state.hudLoan,ledger,
            state.combat.shellDamageThisTick[0]!=0)) return false;
    if (state.combat.actors[0].coreHit!=0) {
        state.scene=PrecedentScene::HitFreeze; state.sceneTicks=0; state.retryPending=1;
    } else if (state.combat.actors[1].coreHit!=0) {
        state.scene=PrecedentScene::CoreUnfold; state.sceneTicks=0;
        state.combat.phase=CombatPhase::Active;
    }
    ++state.combat.globalTick;
    ++state.movement.globalTick;
    return PhysicalLedgerValid(ledger) && ClaimLedgerValid(state.claimLedger)
        && CombatStateValid(state.combat,ledger) && HudLoanSystemValid(state.hudLoan,ledger);
}

void RenderPrecedentRuntime(PixelBuffer buffer,const PrecedentRuntime& state) noexcept {
    if (state.scene==PrecedentScene::RoleAssign) {
        Clear(buffer,PaletteColor::Background);
        Centered(buffer,52,"ROLE ASSIGNED: AUDITOR",PaletteColor::Warning);
        std::uint16_t player=state.claimLedger.mass[static_cast<std::uint8_t>(ClaimOwner::Player)];
        std::uint16_t greatest=0;
        for (std::uint16_t mass:state.claimLedger.mass) if (mass>greatest) greatest=mass;
        Centered(buffer,76,player>=greatest?"YOU HOLD THE MOST":"YOU DO NOT HOLD THE MOST",
            PaletteColor::Neutral);
        if (player<greatest) Centered(buffer,88,"YOU HAVE DECIDED THE MOST",PaletteColor::Neutral);
        Centered(buffer,112,"YOU PROVIDED THE CASES.",PaletteColor::UiInactive);
        Centered(buffer,124,"I REMOVED THEIR NAMES.",PaletteColor::UiInactive);
        Centered(buffer,136,"THE RULE REMAINED.",PaletteColor::UiInactive);
        return;
    }
    if (state.scene==PrecedentScene::PolicyAudit) {
        PolicyAuditView view=BuildPolicyAuditView(state.policy);
        for (std::uint8_t row=static_cast<std::uint8_t>(state.auditRow+1u);row<4;++row)
            view.rows[row]=PolicyAuditRow{"","",""};
        RenderPolicyAudit(buffer,view);
        return;
    }
    if (state.scene==PrecedentScene::CoreUnfold) {
        Clear(buffer,PaletteColor::Background);
        DrawLedgerGrid(buffer);
        const std::int32_t width=static_cast<std::int32_t>(state.sceneTicks)*256/36;
        FillRect(buffer,RectI{32,88,width,4},PaletteColor::Opponent);
        Centered(buffer,104,"THE RULE RETURNS TO THE LEDGER",PaletteColor::Neutral);
        return;
    }
    RenderBattle(buffer,state);
}

std::uint32_t PrecedentRuntimeHash(const PrecedentRuntime& state) noexcept {
    std::uint32_t hash=2166136261u;
    const auto add=[&hash](std::uint32_t value) noexcept {
        for (std::uint8_t i=0;i<4;++i) { hash^=(value>>(i*8u))&0xFFu; hash*=16777619u; }
    };
    for (std::uint16_t mass:state.claimLedger.mass) add(mass);
    for (std::uint16_t mass:state.movement.physicalLedger.mass) add(mass);
    add(state.narrative.choiceBits);
    add(state.policy.profile.identity); add(state.policy.profile.distribution);
    add(state.policy.profile.governance); add(state.policy.profile.memory);
    for (std::uint8_t i=0;i<4;++i) {
        add(static_cast<std::uint8_t>(state.policy.decisiveEncounter[i]));
        add(static_cast<std::uint8_t>(state.policy.decisiveChoice[i]));
    }
    add(static_cast<std::uint8_t>(state.scene)); add(static_cast<std::uint8_t>(state.action));
    add(static_cast<std::uint8_t>(state.pendingAction)); add(state.globalTick); add(state.encounterTick);
    add(state.sceneTicks); add(state.phaseTicks); add(state.phaseDisplayTicks);
    add(state.exceptionTicks); add(state.auditRow); add(state.phase);
    add(state.markedOriginMass); add(state.markedOriginTicks); add(state.reclaimPriorityTicks);
    add(state.forkTicks); add(state.echoTicks); add(state.echoIndex); add(state.borrowedFoldedMass);
    add(state.borrowedActive); add(state.pendingValid); add(state.exceptionShown);
    add(state.exceptionDeniedShown); add(state.lastPlayerPolicyAction);
    add(state.lastPlayerPolicyTick); add(GameStateHash(state.movement));
    for (const ProjectileSlot& shot:state.combat.projectiles) {
        add(shot.active); add(shot.stableId); add(shot.mass); add(static_cast<std::uint8_t>(shot.owner));
        add(shot.position.x); add(shot.position.y);
    }
    for (const NeutralSlot& neutral:state.combat.neutrals) {
        add(neutral.active); add(neutral.stableId); add(neutral.mass);
        add(neutral.position.x); add(neutral.position.y);
    }
    return hash;
}

#if defined(BP_TEST)
bool ApplyPrecedentNeutralTransferForTest(PrecedentRuntime& state,
    PhysicalAccount from,std::uint16_t mass) noexcept {
    return SpawnNeutralFrom(state,from,mass,Vec2Q8{160*kQ8One,90*kQ8One});
}

bool ApplyPrecedentOpponentAttackForTest(PrecedentRuntime& state,
    std::uint16_t mass) noexcept {
    return SpawnOpponentAttack(state,mass);
}
#endif

}  // namespace bp
