#include "bp_reclaim.h"

#include "bp_game.h"
#include "bp_physical_ledger.h"

#include <cstdint>

namespace bp {
namespace {

constexpr std::uint8_t SideIndex(CombatSide side) noexcept {
    return static_cast<std::uint8_t>(side);
}

constexpr PhysicalAccount ShellAccount(CombatSide side) noexcept {
    return side == CombatSide::Player
        ? PhysicalAccount::PlayerShell
        : PhysicalAccount::OpponentShell;
}

constexpr std::uint8_t AccountIndex(PhysicalAccount account) noexcept {
    return static_cast<std::uint8_t>(account);
}

std::uint64_t DistanceSquared(Vec2Q8 a, Vec2Q8 b) noexcept {
    const std::int64_t dx = static_cast<std::int64_t>(a.x) - b.x;
    const std::int64_t dy = static_cast<std::int64_t>(a.y) - b.y;
    return static_cast<std::uint64_t>(dx * dx) + static_cast<std::uint64_t>(dy * dy);
}

std::uint32_t CeilingLength(Vec2Q8 vector) noexcept {
    const std::uint64_t squared =
        static_cast<std::uint64_t>(static_cast<std::int64_t>(vector.x) * vector.x)
        + static_cast<std::uint64_t>(static_cast<std::int64_t>(vector.y) * vector.y);
    const std::uint32_t floor = IntegerSqrt(squared);
    return floor + (static_cast<std::uint64_t>(floor) * floor < squared ? 1u : 0u);
}

Vec2Q8 NormalizeQ15(Vec2Q8 vector) noexcept {
    const std::uint32_t length = CeilingLength(vector);
    if (length == 0) return Vec2Q8{kInputUnit, 0};
    return Vec2Q8{
        RoundDivideSigned(static_cast<std::int64_t>(vector.x) * kInputUnit,
            static_cast<std::int32_t>(length)),
        RoundDivideSigned(static_cast<std::int64_t>(vector.y) * kInputUnit,
            static_cast<std::int32_t>(length)),
    };
}

bool PointInsideCover(std::int32_t x, std::int32_t y, const RectPixels& bounds) noexcept {
    return x >= bounds.x && x < bounds.x + bounds.width
        && y >= bounds.y && y < bounds.y + bounds.height;
}

bool CoverBlocksLine(const CombatState& state, Vec2Q8 from, Vec2Q8 to) noexcept {
    std::int32_t x = from.x / kQ8One;
    std::int32_t y = from.y / kQ8One;
    const std::int32_t endX = to.x / kQ8One;
    const std::int32_t endY = to.y / kQ8One;
    const std::int32_t dx = endX > x ? endX - x : x - endX;
    const std::int32_t stepX = x < endX ? 1 : -1;
    const std::int32_t dyMagnitude = endY > y ? endY - y : y - endY;
    const std::int32_t dy = -dyMagnitude;
    const std::int32_t stepY = y < endY ? 1 : -1;
    std::int32_t error = dx + dy;
    for (;;) {
        for (const CoverSlot& cover : state.covers) {
            if (cover.active != 0 && PointInsideCover(x, y, cover.bounds)) return true;
        }
        if (x == endX && y == endY) return false;
        const std::int32_t doubled = error * 2;
        if (doubled >= dy) {
            error += dy;
            x += stepX;
        }
        if (doubled <= dx) {
            error += dx;
            y += stepY;
        }
    }
}

bool InsideReclaimCone(Vec2Q8 delta, Vec2Q8 aimDirectionQ15) noexcept {
    // cos(18 degrees), rounded to Q1.15. Comparing squares avoids float use.
    constexpr std::int64_t kCos18Q15 = 31164;
    if (delta.x == 0 && delta.y == 0) return true;
    const Vec2Q8 aim = NormalizeQ15(aimDirectionQ15);
    const std::int64_t dot = static_cast<std::int64_t>(delta.x) * aim.x
        + static_cast<std::int64_t>(delta.y) * aim.y;
    if (dot <= 0) return false;
    const std::uint64_t distanceSquared =
        static_cast<std::uint64_t>(static_cast<std::int64_t>(delta.x) * delta.x)
        + static_cast<std::uint64_t>(static_cast<std::int64_t>(delta.y) * delta.y);
    const std::uint64_t dotSquared = static_cast<std::uint64_t>(dot * dot);
    const std::uint64_t cosineSquared = static_cast<std::uint64_t>(kCos18Q15 * kCos18Q15);
    return dotSquared >= distanceSquared * cosineSquared;
}

void AccelerateToward(NeutralSlot& neutral, Vec2Q8 target, std::int32_t accelerationQ8) noexcept {
    const Vec2Q8 direction = NormalizeQ15(Vec2Q8{
        target.x - neutral.position.x,
        target.y - neutral.position.y,
    });
    neutral.velocity.x += RoundDivideSigned(
        static_cast<std::int64_t>(direction.x) * accelerationQ8, kInputUnit);
    neutral.velocity.y += RoundDivideSigned(
        static_cast<std::int64_t>(direction.y) * accelerationQ8, kInputUnit);
}

bool InActorBody(const CombatState& state, const PhysicalLedger& ledger, CombatSide side,
    const NeutralSlot& neutral) noexcept {
    const std::int32_t radiusQ8 = ActorRadiusPixels(
        ledger.mass[AccountIndex(ShellAccount(side))]) * kQ8One;
    return DistanceSquared(state.actors[SideIndex(side)].position, neutral.position)
        <= static_cast<std::uint64_t>(radiusQ8) * radiusQ8;
}

void ConsumeProvenance(NeutralSlot& neutral, CombatSide side) noexcept {
    std::uint16_t& ownMiss = side == CombatSide::Player
        ? neutral.playerMissMass
        : neutral.opponentMissMass;
    std::uint16_t& otherMiss = side == CombatSide::Player
        ? neutral.opponentMissMass
        : neutral.playerMissMass;
    const std::uint16_t general = static_cast<std::uint16_t>(
        neutral.mass - neutral.playerMissMass - neutral.opponentMissMass
        - neutral.protocolTagMass - neutral.patchFeedMass
        - neutral.adapterEscrowMass - neutral.structureMass
        - neutral.unscoredMass);
    if (ownMiss > 0) --ownMiss;
    else if (general == 0 && otherMiss > 0) --otherMiss;
}

bool AbsorbOne(CombatState& state, PhysicalLedger& ledger, ReclaimState& reclaim,
    CombatSide side, std::uint16_t neutralIndex) noexcept {
    NeutralSlot& neutral = state.neutrals[neutralIndex];
    if (neutral.active == 0 || neutral.mass == 0 || neutral.reclaimLockTicks != 0) return false;
    if (!PhysicalTransfer(ledger, PhysicalAccount::Neutral, ShellAccount(side), 1)) return false;
    if (neutral.unscoredMass > 0) --neutral.unscoredMass;
    else if (neutral.protocolTagMass > 0) --neutral.protocolTagMass;
    else if (neutral.patchFeedMass > 0) --neutral.patchFeedMass;
    else if (neutral.adapterEscrowMass > 0) --neutral.adapterEscrowMass;
    else if (neutral.structureMass > 0) --neutral.structureMass;
    else ConsumeProvenance(neutral, side);
    --neutral.mass;
    reclaim.accumulator[SideIndex(side)] = static_cast<std::uint16_t>(
        reclaim.accumulator[SideIndex(side)] - kReclaimRateDenominator);
    if (neutral.mass == 0) {
        neutral.playerMissMass = 0;
        neutral.opponentMissMass = 0;
        neutral.protocolTagMass = 0;
        neutral.patchFeedMass = 0;
        neutral.adapterEscrowMass = 0;
        neutral.structureMass = 0;
        neutral.unscoredMass = 0;
        neutral.reclaimLockTicks = 0;
        neutral.immunityTicks = 0;
        neutral.inertiaTicks = 0;
        neutral.active = 0;
    }
    return true;
}

void MergePair(NeutralSlot& survivor, NeutralSlot& released) noexcept {
    survivor.mass = static_cast<std::uint16_t>(survivor.mass + released.mass);
    survivor.playerMissMass = static_cast<std::uint16_t>(
        survivor.playerMissMass + released.playerMissMass);
    survivor.opponentMissMass = static_cast<std::uint16_t>(
        survivor.opponentMissMass + released.opponentMissMass);
    survivor.protocolTagMass = static_cast<std::uint16_t>(
        survivor.protocolTagMass + released.protocolTagMass);
    survivor.patchFeedMass = static_cast<std::uint16_t>(
        survivor.patchFeedMass + released.patchFeedMass);
    survivor.adapterEscrowMass = static_cast<std::uint16_t>(
        survivor.adapterEscrowMass + released.adapterEscrowMass);
    survivor.structureMass = static_cast<std::uint16_t>(
        survivor.structureMass + released.structureMass);
    survivor.unscoredMass = static_cast<std::uint16_t>(
        survivor.unscoredMass + released.unscoredMass);
    if (released.reclaimLockTicks > survivor.reclaimLockTicks) {
        survivor.reclaimLockTicks = released.reclaimLockTicks;
    }
    if (released.immunityTicks > survivor.immunityTicks) {
        survivor.immunityTicks = released.immunityTicks;
    }
    if (released.inertiaTicks > survivor.inertiaTicks) {
        survivor.inertiaTicks = released.inertiaTicks;
    }
    released.mass = 0;
    released.playerMissMass = 0;
    released.opponentMissMass = 0;
    released.protocolTagMass = 0;
    released.patchFeedMass = 0;
    released.adapterEscrowMass = 0;
    released.structureMass = 0;
    released.unscoredMass = 0;
    released.reclaimLockTicks = 0;
    released.immunityTicks = 0;
    released.inertiaTicks = 0;
    released.active = 0;
}

}  // namespace

void InitializeReclaimState(ReclaimState& state) noexcept {
    state = ReclaimState{};
    state.targetIndex[0] = kInvalidSlotIndex;
    state.targetIndex[1] = kInvalidSlotIndex;
}

bool TickNeutralDynamics(CombatState& state, const PhysicalLedger& ledger) noexcept {
    if (!CombatStateValid(state, ledger)) return false;
    CombatState candidate = state;
    const std::int32_t minimumX = kPlayInterior.x * kQ8One;
    const std::int32_t minimumY = kPlayInterior.y * kQ8One;
    const std::int32_t maximumX = (kPlayInterior.x + kPlayInterior.width) * kQ8One;
    const std::int32_t maximumY = (kPlayInterior.y + kPlayInterior.height) * kQ8One;
    for (NeutralSlot& neutral : candidate.neutrals) {
        if (neutral.active == 0) continue;
        neutral.position.x += neutral.velocity.x;
        neutral.position.y += neutral.velocity.y;
        if (neutral.position.x < minimumX) {
            neutral.position.x = minimumX;
            if (neutral.velocity.x < 0) neutral.velocity.x = -neutral.velocity.x * 20 / 100;
        } else if (neutral.position.x > maximumX) {
            neutral.position.x = maximumX;
            if (neutral.velocity.x > 0) neutral.velocity.x = -neutral.velocity.x * 20 / 100;
        }
        if (neutral.position.y < minimumY) {
            neutral.position.y = minimumY;
            if (neutral.velocity.y < 0) neutral.velocity.y = -neutral.velocity.y * 20 / 100;
        } else if (neutral.position.y > maximumY) {
            neutral.position.y = maximumY;
            if (neutral.velocity.y > 0) neutral.velocity.y = -neutral.velocity.y * 20 / 100;
        }
        if (neutral.inertiaTicks == 0) {
            neutral.velocity.x = TruncMultiplyDivide(neutral.velocity.x, 65067, 65536);
            neutral.velocity.y = TruncMultiplyDivide(neutral.velocity.y, 65067, 65536);
        }
    }
    if (!CombatStateValid(candidate, ledger)) return false;
    state = candidate;
    return true;
}

bool MergeNeutralClusters(CombatState& state, const PhysicalLedger& ledger) noexcept {
    if (!CombatStateValid(state, ledger)) return false;
    CombatState candidate = state;
    const std::uint64_t maximumDistanceSquared =
        static_cast<std::uint64_t>(kNeutralMergeDistancePixels * kQ8One)
        * (kNeutralMergeDistancePixels * kQ8One);
    const std::int32_t maximumRelativeQ8 =
        PixelsPerSecondToQ8PerTick(kNeutralMergeRelativeSpeedPixelsPerSecond);
    const std::uint64_t maximumRelativeSquared =
        static_cast<std::uint64_t>(maximumRelativeQ8) * maximumRelativeQ8;
    for (;;) {
        std::uint16_t bestA = kInvalidSlotIndex;
        std::uint16_t bestB = kInvalidSlotIndex;
        std::uint64_t bestDistance = UINT64_MAX;
        std::uint32_t bestLowId = UINT32_MAX;
        std::uint32_t bestHighId = UINT32_MAX;
        for (std::uint16_t a = 0; a < kNeutralCapacity; ++a) {
            if (candidate.neutrals[a].active == 0) continue;
            for (std::uint16_t b = static_cast<std::uint16_t>(a + 1); b < kNeutralCapacity; ++b) {
                const NeutralSlot& first = candidate.neutrals[a];
                const NeutralSlot& second = candidate.neutrals[b];
                if (second.active == 0 || static_cast<std::uint32_t>(first.mass) + second.mass > 256u) continue;
                const std::uint64_t distance = DistanceSquared(first.position, second.position);
                if (distance > maximumDistanceSquared) continue;
                const Vec2Q8 relative{
                    first.velocity.x - second.velocity.x,
                    first.velocity.y - second.velocity.y,
                };
                const std::uint64_t relativeSquared =
                    static_cast<std::uint64_t>(static_cast<std::int64_t>(relative.x) * relative.x)
                    + static_cast<std::uint64_t>(static_cast<std::int64_t>(relative.y) * relative.y);
                if (relativeSquared > maximumRelativeSquared) continue;
                const std::uint32_t lowId = first.stableId < second.stableId
                    ? first.stableId : second.stableId;
                const std::uint32_t highId = first.stableId < second.stableId
                    ? second.stableId : first.stableId;
                if (bestA == kInvalidSlotIndex || distance < bestDistance
                    || (distance == bestDistance && (lowId < bestLowId
                        || (lowId == bestLowId && highId < bestHighId)))) {
                    bestA = a;
                    bestB = b;
                    bestDistance = distance;
                    bestLowId = lowId;
                    bestHighId = highId;
                }
            }
        }
        if (bestA == kInvalidSlotIndex) break;
        std::uint16_t survivor = bestA;
        std::uint16_t released = bestB;
        if (candidate.neutrals[released].stableId < candidate.neutrals[survivor].stableId) {
            survivor = bestB;
            released = bestA;
        }
        MergePair(candidate.neutrals[survivor], candidate.neutrals[released]);
    }
    if (!CombatStateValid(candidate, ledger)) return false;
    state = candidate;
    return true;
}

std::uint16_t FindReclaimTargetInternal(
    const CombatState& state,
    CombatSide side,
    Vec2Q8 aimDirectionQ15,
    bool structureOnly,
    std::uint32_t playerMissPriorityStableId,
    std::uint32_t playerMissSecondaryStableId = 0) noexcept {
    const std::uint8_t sideIndex = SideIndex(side);
    if (sideIndex >= 2) return kInvalidSlotIndex;
    const Vec2Q8 actorPosition = state.actors[sideIndex].position;
    const std::int32_t rangePixels = side == CombatSide::Player
        ? kPlayerReclaimRangePixels : kOpponentReclaimRangePixels;
    const std::uint64_t rangeSquared = static_cast<std::uint64_t>(rangePixels * kQ8One)
        * (rangePixels * kQ8One);
    std::uint16_t best = kInvalidSlotIndex;
    std::uint32_t bestScore = 0;
    std::uint32_t bestStableId = UINT32_MAX;
    for (std::uint16_t index = 0; index < kNeutralCapacity; ++index) {
        const NeutralSlot& neutral = state.neutrals[index];
        if (neutral.active == 0 || neutral.reclaimLockTicks != 0) continue;
        if (structureOnly && neutral.structureMass == 0) continue;
        const Vec2Q8 delta{
            neutral.position.x - actorPosition.x,
            neutral.position.y - actorPosition.y,
        };
        const std::uint64_t distanceSquared = DistanceSquared(actorPosition, neutral.position);
        if (distanceSquared > rangeSquared || !InsideReclaimCone(delta, aimDirectionQ15)
            || CoverBlocksLine(state, actorPosition, neutral.position)) continue;
        const std::uint32_t distancePixels = IntegerSqrt(distanceSquared) / kQ8One;
        std::uint32_t score = static_cast<std::uint32_t>(neutral.mass) * 256u
            / (distancePixels + 16u);
        if ((playerMissPriorityStableId!=0
                && neutral.stableId==playerMissPriorityStableId
            || playerMissSecondaryStableId!=0
                && neutral.stableId==playerMissSecondaryStableId)
            && neutral.playerMissMass!=0) score+=128u;
        if (best == kInvalidSlotIndex || score > bestScore
            || (score == bestScore && neutral.stableId < bestStableId)) {
            best = index;
            bestScore = score;
            bestStableId = neutral.stableId;
        }
    }
    return best;
}

std::uint16_t FindReclaimTarget(
    const CombatState& state,
    CombatSide side,
    Vec2Q8 aimDirectionQ15) noexcept {
    return FindReclaimTargetInternal(state, side, aimDirectionQ15, false, false);
}

bool TickReclaim(
    CombatState& state,
    PhysicalLedger& ledger,
    ReclaimState& reclaim,
    bool playerActive,
    bool opponentActive,
    ReclaimTickResult& result,
    bool opponentStructurePriority,
    std::uint32_t opponentPlayerMissPriorityStableId,
    std::uint32_t opponentPlayerMissSecondaryStableId) noexcept {
    result = ReclaimTickResult{
        0, 0, 0, 0, 0, 0, kInvalidSlotIndex, kInvalidSlotIndex};
    if (!CombatStateValid(state, ledger)) return false;
    CombatState candidate = state;
    PhysicalLedger ledgerCandidate = ledger;
    ReclaimState reclaimCandidate = reclaim;
    const bool active[2] = {playerActive, opponentActive};
    std::uint16_t targets[2] = {kInvalidSlotIndex, kInvalidSlotIndex};
    bool wantsTile[2] = {false, false};
    for (std::uint8_t sideIndex = 0; sideIndex < 2; ++sideIndex) {
        reclaimCandidate.targetIndex[sideIndex] = kInvalidSlotIndex;
        if (!active[sideIndex]) continue;
        const CombatSide side = static_cast<CombatSide>(sideIndex);
        targets[sideIndex] = FindReclaimTargetInternal(
            candidate,
            side,
            candidate.actors[sideIndex].aimDirectionQ15,
            side == CombatSide::Opponent && opponentStructurePriority,
            side == CombatSide::Opponent?opponentPlayerMissPriorityStableId:0u,
            side == CombatSide::Opponent?opponentPlayerMissSecondaryStableId:0u);
        if (targets[sideIndex] == kInvalidSlotIndex
            && side == CombatSide::Opponent && opponentStructurePriority) {
            targets[sideIndex] = FindReclaimTargetInternal(
                candidate, side, candidate.actors[sideIndex].aimDirectionQ15,
                false, opponentPlayerMissPriorityStableId,
                opponentPlayerMissSecondaryStableId);
        }
        reclaimCandidate.targetIndex[sideIndex] = targets[sideIndex];
        if (targets[sideIndex] == kInvalidSlotIndex) continue;
        NeutralSlot& neutral = candidate.neutrals[targets[sideIndex]];
        const std::int32_t acceleration = PixelsPerSecondSquaredToQ8PerTickSquared(
            side == CombatSide::Player ? 180 : 165);
        AccelerateToward(neutral, candidate.actors[sideIndex].position, acceleration);
        if (InActorBody(candidate, ledgerCandidate, side, neutral)) {
            reclaimCandidate.accumulator[sideIndex] = static_cast<std::uint16_t>(
                reclaimCandidate.accumulator[sideIndex] + kReclaimRateNumerator);
            wantsTile[sideIndex] = reclaimCandidate.accumulator[sideIndex]
                >= kReclaimRateDenominator;
        }
    }

    if (wantsTile[0] && wantsTile[1] && targets[0] == targets[1]
        && candidate.neutrals[targets[0]].mass == 1) {
        const std::uint64_t playerDistance = DistanceSquared(
            candidate.actors[0].position, candidate.neutrals[targets[0]].position);
        const std::uint64_t opponentDistance = DistanceSquared(
            candidate.actors[1].position, candidate.neutrals[targets[1]].position);
        if (opponentDistance < playerDistance) wantsTile[0] = false;
        else wantsTile[1] = false;
    }

    for (std::uint8_t sideIndex = 0; sideIndex < 2; ++sideIndex) {
        if (!wantsTile[sideIndex]) continue;
        const CombatSide side = static_cast<CombatSide>(sideIndex);
        const NeutralSlot& selected = candidate.neutrals[targets[sideIndex]];
        const bool escrow = selected.adapterEscrowMass > 0;
        const bool structure = selected.structureMass > 0;
        if (!AbsorbOne(candidate, ledgerCandidate, reclaimCandidate, side, targets[sideIndex])) {
            state.phase = CombatPhase::Fatal;
            state.error = CombatError::LedgerTransactionFailed;
            return false;
        }
        if (side == CombatSide::Player) {
            result.playerMass = 1;
            result.playerAdapterEscrowMass = escrow ? 1 : 0;
            result.playerStructureMass = structure ? 1 : 0;
        } else {
            result.opponentMass = 1;
            result.opponentAdapterEscrowMass = escrow ? 1 : 0;
            result.opponentStructureMass = structure ? 1 : 0;
        }
    }
    result.playerTarget = targets[0];
    result.opponentTarget = targets[1];
    if (!CombatStateValid(candidate, ledgerCandidate)) {
        state.phase = CombatPhase::Fatal;
        state.error = CombatError::InvariantFailure;
        return false;
    }
    state = candidate;
    ledger = ledgerCandidate;
    reclaim = reclaimCandidate;
    return true;
}

}  // namespace bp
