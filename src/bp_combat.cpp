#include "bp_combat.h"

#include "bp_claim_ledger.h"
#include "bp_game.h"
#include "bp_physical_ledger.h"

namespace bp {
namespace {

constexpr std::uint8_t SideIndex(CombatSide side) noexcept {
    return static_cast<std::uint8_t>(side);
}

constexpr CombatSide Opposite(CombatSide side) noexcept {
    return side == CombatSide::Player ? CombatSide::Opponent : CombatSide::Player;
}

constexpr PhysicalAccount ShellAccount(CombatSide side) noexcept {
    return side == CombatSide::Player
        ? PhysicalAccount::PlayerShell
        : PhysicalAccount::OpponentShell;
}

constexpr std::uint8_t AccountIndex(PhysicalAccount account) noexcept {
    return static_cast<std::uint8_t>(account);
}

std::uint16_t NextGeneration(std::uint16_t generation) noexcept {
    ++generation;
    return generation == 0 ? 1u : generation;
}

bool TakeStableId(CombatState& state, std::uint32_t& stableId) noexcept {
    if (state.nextStableId == 0 || state.nextStableId == 0xFFFFFFFFu) {
        state.phase = CombatPhase::Fatal;
        state.error = CombatError::StableIdExhausted;
        return false;
    }
    stableId = state.nextStableId++;
    return true;
}

std::uint16_t FindFreeProjectile(const CombatState& state) noexcept {
    for (std::uint16_t index = 0; index < kProjectileCapacity; ++index) {
        if (state.projectiles[index].active == 0) return index;
    }
    return kInvalidSlotIndex;
}

std::uint16_t FindFreeNeutral(const CombatState& state) noexcept {
    for (std::uint16_t index = 0; index < kNeutralCapacity; ++index) {
        if (state.neutrals[index].active == 0) return index;
    }
    return kInvalidSlotIndex;
}

std::uint16_t FindFreeCover(const CombatState& state) noexcept {
    for (std::uint16_t index = 0; index < kCoverCapacity; ++index) {
        if (state.covers[index].active == 0) return index;
    }
    return kInvalidSlotIndex;
}

bool RecoverNeutralSlot(CombatState& state) noexcept {
    std::uint16_t bestFirst = kInvalidSlotIndex;
    std::uint16_t bestSecond = kInvalidSlotIndex;
    std::uint64_t bestDistanceSquared = UINT64_MAX;
    std::uint32_t bestLowId = 0xFFFFFFFFu;
    std::uint32_t bestHighId = 0xFFFFFFFFu;
    for (std::uint16_t first = 0; first < kNeutralCapacity; ++first) {
        const NeutralSlot& a = state.neutrals[first];
        if (a.active == 0) continue;
        for (std::uint16_t second = static_cast<std::uint16_t>(first + 1);
             second < kNeutralCapacity;
             ++second) {
            const NeutralSlot& b = state.neutrals[second];
            if (b.active == 0 || static_cast<std::uint32_t>(a.mass) + b.mass > 256u) continue;
            const std::int64_t dx = static_cast<std::int64_t>(a.position.x) - b.position.x;
            const std::int64_t dy = static_cast<std::int64_t>(a.position.y) - b.position.y;
            const std::uint64_t distanceSquared =
                static_cast<std::uint64_t>(dx * dx) + static_cast<std::uint64_t>(dy * dy);
            const std::uint32_t lowId = a.stableId < b.stableId ? a.stableId : b.stableId;
            const std::uint32_t highId = a.stableId < b.stableId ? b.stableId : a.stableId;
            if (bestFirst == kInvalidSlotIndex
                || distanceSquared < bestDistanceSquared
                || (distanceSquared == bestDistanceSquared
                    && (lowId < bestLowId || (lowId == bestLowId && highId < bestHighId)))) {
                bestFirst = first;
                bestSecond = second;
                bestDistanceSquared = distanceSquared;
                bestLowId = lowId;
                bestHighId = highId;
            }
        }
    }
    if (bestFirst == kInvalidSlotIndex) return false;
    std::uint16_t survivorIndex = bestFirst;
    std::uint16_t releasedIndex = bestSecond;
    if (state.neutrals[bestSecond].stableId < state.neutrals[bestFirst].stableId) {
        survivorIndex = bestSecond;
        releasedIndex = bestFirst;
    }
    NeutralSlot& survivor = state.neutrals[survivorIndex];
    NeutralSlot& released = state.neutrals[releasedIndex];
    survivor.mass = static_cast<std::uint16_t>(survivor.mass + released.mass);
    survivor.playerMissMass = static_cast<std::uint16_t>(
        survivor.playerMissMass + released.playerMissMass);
    survivor.opponentMissMass = static_cast<std::uint16_t>(
        survivor.opponentMissMass + released.opponentMissMass);
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
    released.reclaimLockTicks = 0;
    released.immunityTicks = 0;
    released.inertiaTicks = 0;
    released.active = 0;
    return true;
}

Vec2Q8 NormalizeVectorQ15(Vec2Q8 vector) noexcept {
    const std::uint64_t squared =
        static_cast<std::uint64_t>(static_cast<std::int64_t>(vector.x) * vector.x)
        + static_cast<std::uint64_t>(static_cast<std::int64_t>(vector.y) * vector.y);
    const std::uint32_t floorLength = IntegerSqrt(squared);
    if (floorLength == 0) return Vec2Q8{kInputUnit, 0};
    const std::uint32_t ceilingLength = floorLength
        + (static_cast<std::uint64_t>(floorLength) * floorLength < squared ? 1u : 0u);
    return Vec2Q8{
        RoundDivideSigned(static_cast<std::int64_t>(vector.x) * kInputUnit,
            static_cast<std::int32_t>(ceilingLength)),
        RoundDivideSigned(static_cast<std::int64_t>(vector.y) * kInputUnit,
            static_cast<std::int32_t>(ceilingLength)),
    };
}

Vec2Q8 ScaleDirection(Vec2Q8 directionQ15, std::int32_t magnitudeQ8) noexcept {
    return Vec2Q8{
        RoundDivideSigned(static_cast<std::int64_t>(directionQ15.x) * magnitudeQ8, kInputUnit),
        RoundDivideSigned(static_cast<std::int64_t>(directionQ15.y) * magnitudeQ8, kInputUnit),
    };
}

bool StableIdsUnique(const CombatState& state) noexcept {
    for (std::uint8_t side = 0; side < 2; ++side) {
        if (state.charges[side].active == 0) continue;
        const std::uint32_t id = state.charges[side].stableId;
        if (id == 0) return false;
        for (std::uint8_t other = side + 1; other < 2; ++other) {
            if (state.charges[other].active != 0 && state.charges[other].stableId == id) return false;
        }
        for (const ProjectileSlot& slot : state.projectiles) {
            if (slot.active != 0 && slot.stableId == id) return false;
        }
        for (const NeutralSlot& slot : state.neutrals) {
            if (slot.active != 0 && slot.stableId == id) return false;
        }
        for (const CoverSlot& slot : state.covers) {
            if (slot.active != 0 && slot.stableId == id) return false;
        }
    }
    for (std::uint16_t index = 0; index < kProjectileCapacity; ++index) {
        const ProjectileSlot& slot = state.projectiles[index];
        if (slot.active == 0) continue;
        if (slot.stableId == 0) return false;
        for (std::uint16_t other = index + 1; other < kProjectileCapacity; ++other) {
            if (state.projectiles[other].active != 0
                && state.projectiles[other].stableId == slot.stableId) return false;
        }
        for (const NeutralSlot& other : state.neutrals) {
            if (other.active != 0 && other.stableId == slot.stableId) return false;
        }
        for (const CoverSlot& other : state.covers) {
            if (other.active != 0 && other.stableId == slot.stableId) return false;
        }
    }
    for (std::uint16_t index = 0; index < kNeutralCapacity; ++index) {
        const NeutralSlot& slot = state.neutrals[index];
        if (slot.active == 0) continue;
        if (slot.stableId == 0) return false;
        for (std::uint16_t other = index + 1; other < kNeutralCapacity; ++other) {
            if (state.neutrals[other].active != 0
                && state.neutrals[other].stableId == slot.stableId) return false;
        }
        for (const CoverSlot& other : state.covers) {
            if (other.active != 0 && other.stableId == slot.stableId) return false;
        }
    }
    for (std::uint16_t index = 0; index < kCoverCapacity; ++index) {
        const CoverSlot& slot = state.covers[index];
        if (slot.active == 0) continue;
        if (slot.stableId == 0) return false;
        for (std::uint16_t other = index + 1; other < kCoverCapacity; ++other) {
            if (state.covers[other].active != 0
                && state.covers[other].stableId == slot.stableId) return false;
        }
    }
    return true;
}

bool AllocateNeutralSlot(
    CombatState& state,
    Vec2Q8 position,
    Vec2Q8 velocity,
    std::uint16_t mass,
    std::uint16_t playerMissMass,
    std::uint16_t opponentMissMass,
    std::uint32_t retainedStableId,
    std::uint16_t& indexOut) noexcept {
    if (mass == 0 || mass > 256
        || static_cast<std::uint32_t>(playerMissMass) + opponentMissMass > mass) return false;
    std::uint16_t index = FindFreeNeutral(state);
    if (index == kInvalidSlotIndex && RecoverNeutralSlot(state)) {
        index = FindFreeNeutral(state);
    }
    if (index == kInvalidSlotIndex) {
        state.phase = CombatPhase::Fatal;
        state.error = CombatError::NeutralCapacityExhausted;
        return false;
    }
    std::uint32_t stableId = retainedStableId;
    if (stableId == 0 && !TakeStableId(state, stableId)) return false;
    NeutralSlot& slot = state.neutrals[index];
    slot.position = position;
    slot.velocity = velocity;
    slot.stableId = stableId;
    slot.generation = NextGeneration(slot.generation);
    slot.mass = mass;
    slot.reclaimLockTicks = 0;
    slot.immunityTicks = 0;
    slot.playerMissMass = playerMissMass;
    slot.opponentMissMass = opponentMissMass;
    slot.inertiaTicks = 15;
    slot.active = 1;
    indexOut = index;
    return true;
}

std::uint16_t ChargeDesiredMass(std::uint16_t ageTicks) noexcept {
    if (ageTicks >= 75) return 48;
    if (ageTicks >= 48) return 32;
    if (ageTicks >= 21) return 16;
    return 8;
}

std::uint16_t CooldownForMass(std::uint16_t mass) noexcept {
    if (mass >= 48) return 34;
    if (mass >= 32) return 27;
    if (mass >= 16) return 19;
    return 15;
}

std::uint32_t CeilingSqrt(std::uint64_t value) noexcept {
    const std::uint32_t floor = IntegerSqrt(value);
    return floor + (static_cast<std::uint64_t>(floor) * floor < value ? 1u : 0u);
}

bool SweptCircleTimeQ16(
    Vec2Q8 start,
    Vec2Q8 delta,
    Vec2Q8 target,
    std::int32_t combinedRadiusQ8,
    std::uint16_t& timeOut) noexcept {
    const std::int64_t dx = static_cast<std::int64_t>(start.x) - target.x;
    const std::int64_t dy = static_cast<std::int64_t>(start.y) - target.y;
    const std::int64_t vx = delta.x;
    const std::int64_t vy = delta.y;
    const std::int64_t radius = combinedRadiusQ8;
    const std::int64_t c = dx * dx + dy * dy - radius * radius;
    if (c <= 0) {
        timeOut = 0;
        return true;
    }
    const std::int64_t a = vx * vx + vy * vy;
    const std::int64_t b = dx * vx + dy * vy;
    if (a == 0 || b >= 0) return false;
    const std::int64_t discriminant = b * b - a * c;
    if (discriminant < 0) return false;
    const std::uint32_t root = CeilingSqrt(static_cast<std::uint64_t>(discriminant));
    const std::int64_t numerator = -b - static_cast<std::int64_t>(root);
    if (numerator < 0 || numerator > a) return false;
    std::uint64_t time = (static_cast<std::uint64_t>(numerator) << 16) / static_cast<std::uint64_t>(a);
    if (time > 65535u) time = 65535u;
    timeOut = static_cast<std::uint16_t>(time);
    return true;
}

bool SweptPointAabbTimeQ16(
    Vec2Q8 start,
    Vec2Q8 delta,
    std::int32_t minimumX,
    std::int32_t minimumY,
    std::int32_t maximumX,
    std::int32_t maximumY,
    std::uint16_t& timeOut) noexcept {
    std::int64_t enter = 0;
    std::int64_t exit = 65536;
    const std::int32_t starts[2] = {start.x, start.y};
    const std::int32_t deltas[2] = {delta.x, delta.y};
    const std::int32_t minima[2] = {minimumX, minimumY};
    const std::int32_t maxima[2] = {maximumX, maximumY};
    for (std::uint8_t axis = 0; axis < 2; ++axis) {
        if (deltas[axis] == 0) {
            if (starts[axis] < minima[axis] || starts[axis] > maxima[axis]) return false;
            continue;
        }
        std::int64_t first =
            (static_cast<std::int64_t>(minima[axis] - starts[axis]) * 65536) / deltas[axis];
        std::int64_t second =
            (static_cast<std::int64_t>(maxima[axis] - starts[axis]) * 65536) / deltas[axis];
        if (first > second) {
            const std::int64_t swap = first;
            first = second;
            second = swap;
        }
        if (first > enter) enter = first;
        if (second < exit) exit = second;
        if (enter > exit) return false;
    }
    if (exit < 0 || enter > 65536) return false;
    if (enter < 0) enter = 0;
    if (enter > 65535) enter = 65535;
    timeOut = static_cast<std::uint16_t>(enter);
    return true;
}

Vec2Q8 PointAtTime(Vec2Q8 start, Vec2Q8 delta, std::uint16_t timeQ16) noexcept {
    if (timeQ16 == 65535u) return Vec2Q8{start.x + delta.x, start.y + delta.y};
    return Vec2Q8{
        start.x + static_cast<std::int32_t>(static_cast<std::int64_t>(delta.x) * timeQ16 / 65536),
        start.y + static_cast<std::int32_t>(static_cast<std::int64_t>(delta.y) * timeQ16 / 65536),
    };
}

bool BetterImpact(const ImpactCandidate& candidate, const ImpactCandidate& current) noexcept {
    if (candidate.kind == ImpactKind::None) return false;
    if (current.kind == ImpactKind::None) return true;
    if (candidate.timeQ16 != current.timeQ16) return candidate.timeQ16 < current.timeQ16;
    if (candidate.kind != current.kind) {
        return static_cast<std::uint8_t>(candidate.kind) < static_cast<std::uint8_t>(current.kind);
    }
    return candidate.targetStableId < current.targetStableId;
}

void OfferCircleImpact(
    ImpactCandidate& best,
    ImpactKind kind,
    std::uint16_t targetIndex,
    std::uint32_t stableId,
    Vec2Q8 start,
    Vec2Q8 delta,
    Vec2Q8 target,
    Vec2Q8 targetDelta,
    std::int32_t radiusQ8) noexcept {
    std::uint16_t time = 0;
    const Vec2Q8 relativeDelta{delta.x - targetDelta.x, delta.y - targetDelta.y};
    if (!SweptCircleTimeQ16(start, relativeDelta, target, radiusQ8, time)) return;
    ImpactCandidate candidate{kind, time, targetIndex, stableId, PointAtTime(start, delta, time)};
    if (BetterImpact(candidate, best)) best = candidate;
}

ImpactCandidate FindEarliestImpactInternal(
    const CombatState& state,
    const PhysicalLedger& ledger,
    std::uint16_t projectileIndex,
    const bool* processed) noexcept {
    ImpactCandidate best{ImpactKind::None, 65535u, kInvalidSlotIndex, 0xFFFFFFFFu, Vec2Q8{0, 0}};
    if (projectileIndex >= kProjectileCapacity || state.projectiles[projectileIndex].active == 0) return best;
    const ProjectileSlot& projectile = state.projectiles[projectileIndex];
    const Vec2Q8 start = projectile.position;
    const Vec2Q8 delta = projectile.velocity;
    const std::int32_t projectileRadiusQ8 = ProjectileRadiusPixels(projectile.mass) * kQ8One;
    const CombatSide targetSide = Opposite(projectile.owner);
    const CombatActor& actor = state.actors[SideIndex(targetSide)];
    if (actor.projectileImmune == 0) {
        const std::uint16_t shellMass = ledger.mass[AccountIndex(ShellAccount(targetSide))];
        std::uint16_t coreTime = 0;
        const bool coreCandidate = shellMass <= kCoreExposureShell
            && actor.exposureGraceTicks == 0
            && SweptCircleTimeQ16(
                start,
                delta,
                actor.position,
                projectileRadiusQ8 + kCoreRadiusPixels * kQ8One,
                coreTime);
        if (coreCandidate) {
            best = ImpactCandidate{
                ImpactKind::Core,
                coreTime,
                SideIndex(targetSide),
                static_cast<std::uint32_t>(SideIndex(targetSide) + 1u),
                PointAtTime(start, delta, coreTime),
            };
        } else {
            OfferCircleImpact(
                best,
                ImpactKind::Shell,
                SideIndex(targetSide),
                static_cast<std::uint32_t>(SideIndex(targetSide) + 1u),
                start,
                delta,
                actor.position,
                Vec2Q8{0, 0},
                projectileRadiusQ8 + ActorRadiusPixels(shellMass) * kQ8One);
        }
    }

    const ChargeSlot& charge = state.charges[SideIndex(targetSide)];
    if (charge.active != 0) {
        const std::uint16_t targetShellMass = ledger.mass[AccountIndex(ShellAccount(targetSide))];
        OfferCircleImpact(
            best,
            ImpactKind::Charge,
            SideIndex(targetSide),
            charge.stableId,
            start,
            delta,
            charge.position,
            Vec2Q8{0, 0},
            projectileRadiusQ8 + ActorRadiusPixels(targetShellMass) * kQ8One);
    }

    for (std::uint16_t index = 0; index < kProjectileCapacity; ++index) {
        const ProjectileSlot& target = state.projectiles[index];
        if (index == projectileIndex || target.active == 0 || target.owner == projectile.owner) continue;
        if (processed != nullptr && processed[index]) continue;
        OfferCircleImpact(
            best,
            ImpactKind::Projectile,
            index,
            target.stableId,
            start,
            delta,
            target.position,
            target.velocity,
            projectileRadiusQ8 + ProjectileRadiusPixels(target.mass) * kQ8One);
    }

    for (std::uint16_t index = 0; index < kCoverCapacity; ++index) {
        const CoverSlot& cover = state.covers[index];
        if (cover.active == 0) continue;
        const std::int32_t minimumX = cover.bounds.x * kQ8One - projectileRadiusQ8;
        const std::int32_t minimumY = cover.bounds.y * kQ8One - projectileRadiusQ8;
        const std::int32_t maximumX = (cover.bounds.x + cover.bounds.width) * kQ8One + projectileRadiusQ8;
        const std::int32_t maximumY = (cover.bounds.y + cover.bounds.height) * kQ8One + projectileRadiusQ8;
        std::uint16_t time = 0;
        if (SweptPointAabbTimeQ16(start, delta, minimumX, minimumY, maximumX, maximumY, time)) {
            ImpactCandidate candidate{
                ImpactKind::Cover,
                time,
                index,
                cover.stableId,
                PointAtTime(start, delta, time),
            };
            if (BetterImpact(candidate, best)) best = candidate;
        }
    }

    const std::int32_t minimumX = kPlayInterior.x * kQ8One + projectileRadiusQ8;
    const std::int32_t minimumY = kPlayInterior.y * kQ8One + projectileRadiusQ8;
    const std::int32_t maximumX = (kPlayInterior.x + kPlayInterior.width) * kQ8One - projectileRadiusQ8;
    const std::int32_t maximumY = (kPlayInterior.y + kPlayInterior.height) * kQ8One - projectileRadiusQ8;
    const Vec2Q8 end{start.x + delta.x, start.y + delta.y};
    std::uint16_t boundaryTime = 65535u;
    std::uint16_t boundaryMask = 0;
    bool hitsBoundary = false;
    if (delta.x < 0 && end.x <= minimumX) {
        boundaryTime = static_cast<std::uint16_t>(Clamp32(
            static_cast<std::int32_t>(
                (static_cast<std::int64_t>(minimumX - start.x) * 65536) / delta.x),
            0,
            65535));
        hitsBoundary = true;
        boundaryMask = 1;
    } else if (delta.x > 0 && end.x >= maximumX) {
        boundaryTime = static_cast<std::uint16_t>(Clamp32(
            static_cast<std::int32_t>(
                (static_cast<std::int64_t>(maximumX - start.x) * 65536) / delta.x),
            0,
            65535));
        hitsBoundary = true;
        boundaryMask = 1;
    }
    if (delta.y < 0 && end.y <= minimumY) {
        const std::uint16_t time = static_cast<std::uint16_t>(Clamp32(
            static_cast<std::int32_t>(
                (static_cast<std::int64_t>(minimumY - start.y) * 65536) / delta.y),
            0,
            65535));
        if (!hitsBoundary || time < boundaryTime) {
            boundaryTime = time;
            boundaryMask = 2;
        } else if (time == boundaryTime) {
            boundaryMask = static_cast<std::uint16_t>(boundaryMask | 2u);
        }
        hitsBoundary = true;
    } else if (delta.y > 0 && end.y >= maximumY) {
        const std::uint16_t time = static_cast<std::uint16_t>(Clamp32(
            static_cast<std::int32_t>(
                (static_cast<std::int64_t>(maximumY - start.y) * 65536) / delta.y),
            0,
            65535));
        if (!hitsBoundary || time < boundaryTime) {
            boundaryTime = time;
            boundaryMask = 2;
        } else if (time == boundaryTime) {
            boundaryMask = static_cast<std::uint16_t>(boundaryMask | 2u);
        }
        hitsBoundary = true;
    }
    if (hitsBoundary) {
        ImpactCandidate candidate{
            ImpactKind::Boundary,
            boundaryTime,
            boundaryMask,
            0,
            PointAtTime(start, delta, boundaryTime),
        };
        if (BetterImpact(candidate, best)) best = candidate;
    }
    return best;
}

Vec2Q8 DeflectionVelocity(const ProjectileSlot& projectile, std::int32_t reversePxS, std::int32_t tangentPxS) noexcept {
    const Vec2Q8 direction = NormalizeVectorQ15(projectile.velocity);
    const std::int32_t reverseQ8 = PixelsPerSecondToQ8PerTick(reversePxS);
    const std::int32_t tangentQ8 = PixelsPerSecondToQ8PerTick(tangentPxS);
    const std::int32_t tangentSign = (projectile.stableId & 1u) == 0 ? 1 : -1;
    return Vec2Q8{
        RoundDivideSigned(
            static_cast<std::int64_t>(-direction.x) * reverseQ8
                + static_cast<std::int64_t>(-direction.y) * tangentQ8 * tangentSign,
            kInputUnit),
        RoundDivideSigned(
            static_cast<std::int64_t>(-direction.y) * reverseQ8
                + static_cast<std::int64_t>(direction.x) * tangentQ8 * tangentSign,
            kInputUnit),
    };
}

bool ProcessImpact(
    CombatState& state,
    PhysicalLedger& ledger,
    const ClaimLedger& claimLedger,
    std::uint16_t projectileIndex,
    const ImpactCandidate& impact,
    bool* processed) noexcept {
    if (impact.kind == ImpactKind::Core) {
        state.projectiles[projectileIndex].position = impact.point;
        const CombatSide hitSide = static_cast<CombatSide>(impact.targetIndex);
        state.actors[SideIndex(hitSide)].coreHit = 1;
        state.phase = CombatPhase::HitFreeze;
        state.settlement.physicalLedger = ledger;
        state.settlement.claimLedger = claimLedger;
        state.settlement.tick = state.globalTick;
        state.settlement.hitSide = hitSide;
        state.settlement.valid = 1;
        return true;
    }

    CombatState candidate = state;
    PhysicalLedger ledgerCandidate = ledger;
    ProjectileSlot& shot = candidate.projectiles[projectileIndex];
    shot.position = impact.point;
    std::uint16_t neutralIndex = kInvalidSlotIndex;
    if (impact.kind == ImpactKind::Shell) {
        const CombatSide targetSide = static_cast<CombatSide>(impact.targetIndex);
        const PhysicalAccount targetShell = ShellAccount(targetSide);
        const std::uint16_t beforeShell = ledgerCandidate.mass[AccountIndex(targetShell)];
        const std::uint16_t damage = ProjectileShellDamage(shot.mass) < beforeShell
            ? ProjectileShellDamage(shot.mass)
            : beforeShell;
        const std::uint16_t neutralMass = static_cast<std::uint16_t>(shot.mass + damage);
        if (!AllocateNeutralSlot(
                candidate,
                impact.point,
                DeflectionVelocity(shot, 90, 24),
                neutralMass,
                0,
                0,
                shot.stableId,
                neutralIndex)
            || !PhysicalTransfer(ledgerCandidate, PhysicalAccount::Projectile, PhysicalAccount::Neutral, shot.mass)
            || !PhysicalTransfer(ledgerCandidate, targetShell, PhysicalAccount::Neutral, damage)) {
            state.phase = CombatPhase::Fatal;
            state.error = candidate.error != CombatError::None
                ? candidate.error
                : CombatError::LedgerTransactionFailed;
            return false;
        }
        NeutralSlot& neutral = candidate.neutrals[neutralIndex];
        neutral.reclaimLockTicks = static_cast<std::uint16_t>(6 + neutral.mass / 3);
        neutral.immunityTicks = 45;
        shot.active = 0;
        shot.mass = 0;
        if (beforeShell >= 41 && ledgerCandidate.mass[AccountIndex(targetShell)] <= 40) {
            candidate.actors[SideIndex(targetSide)].exposureGraceTicks = kCoreExposureGraceTicks;
        }
    } else if (impact.kind == ImpactKind::Cover) {
        CoverSlot& cover = candidate.covers[impact.targetIndex];
        const std::uint16_t damage = ProjectileCoverDamage(shot.mass) < cover.mass
            ? ProjectileCoverDamage(shot.mass)
            : cover.mass;
        const std::uint16_t neutralMass = static_cast<std::uint16_t>(shot.mass + damage);
        if (!AllocateNeutralSlot(
                candidate,
                impact.point,
                DeflectionVelocity(shot, 75, 20),
                neutralMass,
                0,
                0,
                shot.stableId,
                neutralIndex)
            || !PhysicalTransfer(ledgerCandidate, PhysicalAccount::Projectile, PhysicalAccount::Neutral, shot.mass)
            || !PhysicalTransfer(ledgerCandidate, PhysicalAccount::Cover, PhysicalAccount::Neutral, damage)) {
            state.phase = CombatPhase::Fatal;
            state.error = candidate.error != CombatError::None
                ? candidate.error
                : CombatError::LedgerTransactionFailed;
            return false;
        }
        cover.mass = static_cast<std::uint16_t>(cover.mass - damage);
        if (cover.mass == 0) cover.active = 0;
        shot.active = 0;
        shot.mass = 0;
    } else if (impact.kind == ImpactKind::Boundary) {
        Vec2Q8 neutralPosition = impact.point;
        const std::int32_t inset = 2 * kQ8One;
        neutralPosition.x = Clamp32(
            neutralPosition.x,
            kPlayInterior.x * kQ8One + inset,
            (kPlayInterior.x + kPlayInterior.width) * kQ8One - inset);
        neutralPosition.y = Clamp32(
            neutralPosition.y,
            kPlayInterior.y * kQ8One + inset,
            (kPlayInterior.y + kPlayInterior.height) * kQ8One - inset);
        const std::uint16_t playerMiss = shot.owner == CombatSide::Player ? shot.mass : 0;
        const std::uint16_t opponentMiss = shot.owner == CombatSide::Opponent ? shot.mass : 0;
        Vec2Q8 reflectedVelocity = shot.velocity;
        if ((impact.targetIndex & 1u) != 0) {
            reflectedVelocity.x = TruncMultiplyDivide(shot.velocity.x, -20, 100);
        }
        if ((impact.targetIndex & 2u) != 0) {
            reflectedVelocity.y = TruncMultiplyDivide(shot.velocity.y, -20, 100);
        }
        if (!AllocateNeutralSlot(
                candidate,
                neutralPosition,
                reflectedVelocity,
                shot.mass,
                playerMiss,
                opponentMiss,
                shot.stableId,
                neutralIndex)
            || !PhysicalTransfer(ledgerCandidate, PhysicalAccount::Projectile, PhysicalAccount::Neutral, shot.mass)) {
            state.phase = CombatPhase::Fatal;
            state.error = candidate.error != CombatError::None
                ? candidate.error
                : CombatError::LedgerTransactionFailed;
            return false;
        }
        shot.active = 0;
        shot.mass = 0;
    } else if (impact.kind == ImpactKind::Charge) {
        ChargeSlot& charge = candidate.charges[impact.targetIndex];
        const std::uint16_t common = shot.mass < charge.mass ? shot.mass : charge.mass;
        const std::uint16_t neutralMass = static_cast<std::uint16_t>(common * 2);
        const std::uint16_t playerMiss = shot.owner == CombatSide::Player ? common : 0;
        const std::uint16_t opponentMiss = shot.owner == CombatSide::Opponent ? common : 0;
        if (!AllocateNeutralSlot(
                candidate,
                impact.point,
                Vec2Q8{0, 0},
                neutralMass,
                playerMiss,
                opponentMiss,
                shot.mass == common ? shot.stableId : charge.stableId,
                neutralIndex)
            || !PhysicalTransfer(ledgerCandidate, PhysicalAccount::Projectile, PhysicalAccount::Neutral, common)
            || !PhysicalTransfer(ledgerCandidate, PhysicalAccount::Charge, PhysicalAccount::Neutral, common)) {
            state.phase = CombatPhase::Fatal;
            state.error = candidate.error != CombatError::None
                ? candidate.error
                : CombatError::LedgerTransactionFailed;
            return false;
        }
        shot.mass = static_cast<std::uint16_t>(shot.mass - common);
        charge.mass = static_cast<std::uint16_t>(charge.mass - common);
        if (shot.mass == 0) {
            shot.active = 0;
        } else {
            shot.velocity.x = TruncMultiplyDivide(shot.velocity.x, 85, 100);
            shot.velocity.y = TruncMultiplyDivide(shot.velocity.y, 85, 100);
        }
        if (charge.mass < 8) {
            if (charge.mass > 0
                && !PhysicalTransfer(
                    ledgerCandidate,
                    PhysicalAccount::Charge,
                    ShellAccount(static_cast<CombatSide>(impact.targetIndex)),
                    charge.mass)) {
                state.phase = CombatPhase::Fatal;
                state.error = CombatError::LedgerTransactionFailed;
                return false;
            }
            charge.mass = 0;
            charge.stageMass = 0;
            charge.ageTicks = 0;
            charge.active = 0;
        }
    } else if (impact.kind == ImpactKind::Projectile) {
        ProjectileSlot& target = candidate.projectiles[impact.targetIndex];
        target.position = PointAtTime(target.position, target.velocity, impact.timeQ16);
        const std::uint16_t common = shot.mass < target.mass ? shot.mass : target.mass;
        const std::uint16_t neutralMass = static_cast<std::uint16_t>(common * 2);
        const std::uint16_t playerMiss =
            (shot.owner == CombatSide::Player ? common : 0)
            + (target.owner == CombatSide::Player ? common : 0);
        const std::uint16_t opponentMiss =
            (shot.owner == CombatSide::Opponent ? common : 0)
            + (target.owner == CombatSide::Opponent ? common : 0);
        const std::uint32_t retainedId = shot.mass == common && target.mass == common
            ? (shot.stableId < target.stableId ? shot.stableId : target.stableId)
            : (shot.mass == common ? shot.stableId : target.stableId);
        if (!AllocateNeutralSlot(
                candidate,
                impact.point,
                Vec2Q8{0, 0},
                neutralMass,
                playerMiss,
                opponentMiss,
                retainedId,
                neutralIndex)
            || !PhysicalTransfer(
                ledgerCandidate,
                PhysicalAccount::Projectile,
                PhysicalAccount::Neutral,
                neutralMass)) {
            state.phase = CombatPhase::Fatal;
            state.error = candidate.error != CombatError::None
                ? candidate.error
                : CombatError::LedgerTransactionFailed;
            return false;
        }
        shot.mass = static_cast<std::uint16_t>(shot.mass - common);
        target.mass = static_cast<std::uint16_t>(target.mass - common);
        if (shot.mass == 0) shot.active = 0;
        else {
            shot.velocity.x = TruncMultiplyDivide(shot.velocity.x, 85, 100);
            shot.velocity.y = TruncMultiplyDivide(shot.velocity.y, 85, 100);
        }
        if (target.mass == 0) target.active = 0;
        else {
            target.velocity.x = TruncMultiplyDivide(target.velocity.x, 85, 100);
            target.velocity.y = TruncMultiplyDivide(target.velocity.y, 85, 100);
        }
        if (processed != nullptr) processed[impact.targetIndex] = true;
    } else {
        return false;
    }

    if (!CombatStateValid(candidate, ledgerCandidate)) {
        state.phase = CombatPhase::Fatal;
        state.error = CombatError::InvariantFailure;
        return false;
    }
    state = candidate;
    ledger = ledgerCandidate;
    return true;
}

}  // namespace

void InitializeCombatState(
    CombatState& state,
    Vec2Q8 playerPosition,
    Vec2Q8 opponentPosition) noexcept {
    state = CombatState{};
    state.actors[SideIndex(CombatSide::Player)].position = playerPosition;
    state.actors[SideIndex(CombatSide::Opponent)].position = opponentPosition;
    state.actors[SideIndex(CombatSide::Player)].aimDirectionQ15 = Vec2Q8{kInputUnit, 0};
    state.actors[SideIndex(CombatSide::Opponent)].aimDirectionQ15 = Vec2Q8{-kInputUnit, 0};
    state.nextStableId = 3;
    state.phase = CombatPhase::Active;
}

bool CombatStateValid(const CombatState& state, const PhysicalLedger& ledger) noexcept {
    if (!PhysicalLedgerValid(ledger) || state.phase == CombatPhase::Fatal || state.nextStableId == 0) return false;
    std::uint32_t chargeMass = 0;
    std::uint32_t projectileMass = 0;
    std::uint32_t neutralMass = 0;
    std::uint32_t coverMass = 0;
    for (std::uint8_t side = 0; side < 2; ++side) {
        const ChargeSlot& slot = state.charges[side];
        if (slot.active != 0) {
            const bool validStage = slot.stageMass == 8 || slot.stageMass == 16
                || slot.stageMass == 32 || slot.stageMass == 48;
            if (slot.mass < 8 || slot.mass > 48 || !validStage || slot.stageMass < slot.mass
                || slot.stableId == 0 || slot.generation == 0) return false;
            chargeMass += slot.mass;
        } else if (slot.mass != 0 || slot.stageMass != 0) return false;
    }
    for (const ProjectileSlot& slot : state.projectiles) {
        if (slot.active != 0) {
            if (slot.mass == 0 || slot.mass > 48 || slot.stableId == 0 || slot.generation == 0
                || SideIndex(slot.owner) >= 2) return false;
            projectileMass += slot.mass;
        } else if (slot.mass != 0) return false;
    }
    for (const NeutralSlot& slot : state.neutrals) {
        if (slot.active != 0) {
            if (slot.mass == 0 || slot.mass > 256 || slot.stableId == 0 || slot.generation == 0
                || static_cast<std::uint32_t>(slot.playerMissMass) + slot.opponentMissMass > slot.mass) return false;
            neutralMass += slot.mass;
        } else if (slot.mass != 0) return false;
    }
    for (const CoverSlot& slot : state.covers) {
        if (slot.active != 0) {
            if (slot.mass == 0 || slot.stableId == 0 || slot.generation == 0
                || slot.bounds.width <= 0 || slot.bounds.height <= 0) return false;
            coverMass += slot.mass;
        } else if (slot.mass != 0) return false;
    }
    return chargeMass == ledger.mass[AccountIndex(PhysicalAccount::Charge)]
        && projectileMass == ledger.mass[AccountIndex(PhysicalAccount::Projectile)]
        && neutralMass == ledger.mass[AccountIndex(PhysicalAccount::Neutral)]
        && coverMass == ledger.mass[AccountIndex(PhysicalAccount::Cover)]
        && StableIdsUnique(state);
}

bool BeginCharge(
    CombatState& state,
    PhysicalLedger& ledger,
    CombatSide side,
    Vec2Q8 directionQ15) noexcept {
    const std::uint8_t sideIndex = SideIndex(side);
    if (sideIndex >= 2 || state.phase != CombatPhase::Active || !CombatStateValid(state, ledger)
        || state.charges[sideIndex].active != 0 || state.actors[sideIndex].shotCooldownTicks != 0
        || FindFreeProjectile(state) == kInvalidSlotIndex) return false;
    CombatState candidate = state;
    PhysicalLedger ledgerCandidate = ledger;
    ChargeSlot& charge = candidate.charges[sideIndex];
    std::uint32_t stableId = 0;
    if (!TakeStableId(candidate, stableId)) {
        state.phase = candidate.phase;
        state.error = candidate.error;
        return false;
    }
    if (!PhysicalTransfer(ledgerCandidate, ShellAccount(side), PhysicalAccount::Charge, 8)) return false;
    charge.position = candidate.actors[sideIndex].position;
    charge.directionQ15 = NormalizeVectorQ15(directionQ15);
    candidate.actors[sideIndex].aimDirectionQ15 = charge.directionQ15;
    charge.stableId = stableId;
    charge.generation = NextGeneration(charge.generation);
    charge.mass = 8;
    charge.stageMass = 8;
    charge.ageTicks = 0;
    charge.active = 1;
    if (!CombatStateValid(candidate, ledgerCandidate)) return false;
    state = candidate;
    ledger = ledgerCandidate;
    return true;
}

bool AdvanceCharge(CombatState& state, PhysicalLedger& ledger, CombatSide side) noexcept {
    const std::uint8_t sideIndex = SideIndex(side);
    if (sideIndex >= 2 || state.phase != CombatPhase::Active || !CombatStateValid(state, ledger)
        || state.charges[sideIndex].active == 0) return false;
    CombatState candidate = state;
    PhysicalLedger ledgerCandidate = ledger;
    ChargeSlot& charge = candidate.charges[sideIndex];
    charge.position = candidate.actors[sideIndex].position;
    charge.directionQ15 = NormalizeVectorQ15(candidate.actors[sideIndex].aimDirectionQ15);
    if (charge.ageTicks < 65535) ++charge.ageTicks;
    const std::uint16_t desired = ChargeDesiredMass(charge.ageTicks);
    if (desired > charge.mass) {
        const std::uint16_t additional = static_cast<std::uint16_t>(desired - charge.mass);
        if (ledgerCandidate.mass[AccountIndex(ShellAccount(side))] >= additional
            && PhysicalTransfer(ledgerCandidate, ShellAccount(side), PhysicalAccount::Charge, additional)) {
            charge.mass = desired;
            charge.stageMass = desired;
        }
    }
    if (!CombatStateValid(candidate, ledgerCandidate)) return false;
    state = candidate;
    ledger = ledgerCandidate;
    return true;
}

bool RollbackCharge(CombatState& state, PhysicalLedger& ledger, CombatSide side) noexcept {
    const std::uint8_t sideIndex = SideIndex(side);
    if (sideIndex >= 2 || !CombatStateValid(state, ledger) || state.charges[sideIndex].active == 0) return false;
    CombatState candidate = state;
    PhysicalLedger ledgerCandidate = ledger;
    ChargeSlot& charge = candidate.charges[sideIndex];
    if (!PhysicalTransfer(ledgerCandidate, PhysicalAccount::Charge, ShellAccount(side), charge.mass)) return false;
    charge.mass = 0;
    charge.stageMass = 0;
    charge.ageTicks = 0;
    charge.active = 0;
    if (!CombatStateValid(candidate, ledgerCandidate)) return false;
    state = candidate;
    ledger = ledgerCandidate;
    return true;
}

bool ReleaseCharge(CombatState& state, PhysicalLedger& ledger, CombatSide side) noexcept {
    const std::uint8_t sideIndex = SideIndex(side);
    if (sideIndex >= 2 || state.phase != CombatPhase::Active || !CombatStateValid(state, ledger)
        || state.charges[sideIndex].active == 0) return false;
    const std::uint16_t projectileIndex = FindFreeProjectile(state);
    if (projectileIndex == kInvalidSlotIndex) {
        RollbackCharge(state, ledger, side);
        return false;
    }
    CombatState candidate = state;
    PhysicalLedger ledgerCandidate = ledger;
    ChargeSlot& charge = candidate.charges[sideIndex];
    ProjectileSlot& projectile = candidate.projectiles[projectileIndex];
    if (!PhysicalTransfer(ledgerCandidate, PhysicalAccount::Charge, PhysicalAccount::Projectile, charge.mass)) return false;
    projectile.position = charge.position;
    projectile.velocity = ScaleDirection(charge.directionQ15, ProjectileSpeedQ8(charge.mass));
    projectile.stableId = charge.stableId;
    projectile.generation = NextGeneration(projectile.generation);
    projectile.mass = charge.mass;
    projectile.owner = side;
    projectile.active = 1;
    const Vec2Q8 recoil = ScaleDirection(charge.directionQ15, RecoilQ8(charge.mass));
    candidate.actors[sideIndex].velocity.x -= recoil.x;
    candidate.actors[sideIndex].velocity.y -= recoil.y;
    candidate.actors[sideIndex].shotCooldownTicks = CooldownForMass(charge.stageMass);
    charge.mass = 0;
    charge.stageMass = 0;
    charge.ageTicks = 0;
    charge.active = 0;
    if (!CombatStateValid(candidate, ledgerCandidate)) return false;
    state = candidate;
    ledger = ledgerCandidate;
    return true;
}

bool AddCover(
    CombatState& state,
    const RectPixels& bounds,
    std::uint16_t mass,
    SlotHandle& handle) noexcept {
    const std::uint16_t index = FindFreeCover(state);
    if (index == kInvalidSlotIndex || mass == 0 || bounds.width <= 0 || bounds.height <= 0) return false;
    CoverSlot& slot = state.covers[index];
    std::uint32_t stableId = 0;
    if (!TakeStableId(state, stableId)) return false;
    slot.bounds = bounds;
    slot.stableId = stableId;
    slot.generation = NextGeneration(slot.generation);
    slot.mass = mass;
    slot.active = 1;
    handle = SlotHandle{index, slot.generation};
    return true;
}

bool AddNeutral(
    CombatState& state,
    Vec2Q8 position,
    std::uint16_t mass,
    std::uint16_t playerMissMass,
    std::uint16_t opponentMissMass,
    SlotHandle& handle) noexcept {
    std::uint16_t index = kInvalidSlotIndex;
    if (!AllocateNeutralSlot(
            state,
            position,
            Vec2Q8{0, 0},
            mass,
            playerMissMass,
            opponentMissMass,
            0,
            index)) return false;
    handle = SlotHandle{index, state.neutrals[index].generation};
    return true;
}

bool ProjectileHandleValid(const CombatState& state, SlotHandle handle) noexcept {
    return handle.index < kProjectileCapacity
        && state.projectiles[handle.index].active != 0
        && state.projectiles[handle.index].generation == handle.generation;
}

bool NeutralHandleValid(const CombatState& state, SlotHandle handle) noexcept {
    return handle.index < kNeutralCapacity
        && state.neutrals[handle.index].active != 0
        && state.neutrals[handle.index].generation == handle.generation;
}

bool CoverHandleValid(const CombatState& state, SlotHandle handle) noexcept {
    return handle.index < kCoverCapacity
        && state.covers[handle.index].active != 0
        && state.covers[handle.index].generation == handle.generation;
}

bool ResolveProjectileHandle(CombatState& state, SlotHandle handle, std::uint16_t& index) noexcept {
    if (!ProjectileHandleValid(state, handle)) {
        state.phase = CombatPhase::Fatal;
        state.error = CombatError::InvalidHandle;
        index = kInvalidSlotIndex;
        return false;
    }
    index = handle.index;
    return true;
}

bool ResolveNeutralHandle(CombatState& state, SlotHandle handle, std::uint16_t& index) noexcept {
    if (!NeutralHandleValid(state, handle)) {
        state.phase = CombatPhase::Fatal;
        state.error = CombatError::InvalidHandle;
        index = kInvalidSlotIndex;
        return false;
    }
    index = handle.index;
    return true;
}

bool ResolveCoverHandle(CombatState& state, SlotHandle handle, std::uint16_t& index) noexcept {
    if (!CoverHandleValid(state, handle)) {
        state.phase = CombatPhase::Fatal;
        state.error = CombatError::InvalidHandle;
        index = kInvalidSlotIndex;
        return false;
    }
    index = handle.index;
    return true;
}

std::int32_t ProjectileSpeedQ8(std::uint16_t mass) noexcept {
    const std::uint16_t clamped = mass < 8 ? 8 : (mass > 48 ? 48 : mass);
    return PixelsPerSecondToQ8PerTick(170 - static_cast<std::int32_t>(clamped - 8) * 5 / 4);
}

std::uint16_t ProjectileShellDamage(std::uint16_t mass) noexcept {
    return static_cast<std::uint16_t>(static_cast<std::uint32_t>(mass) * 3 / 2);
}

std::uint16_t ProjectileCoverDamage(std::uint16_t mass) noexcept {
    return mass;
}

std::int32_t ProjectileRadiusPixels(std::uint16_t mass) noexcept {
    return 1 + static_cast<std::int32_t>(IntegerSqrt(static_cast<std::uint64_t>(mass) * 9u) / 8u);
}

std::int32_t RecoilQ8(std::uint16_t mass) noexcept {
    return static_cast<std::int32_t>((static_cast<std::uint32_t>(mass) * 8960u + 30000u) / 60000u);
}

ImpactCandidate FindEarliestImpact(
    const CombatState& state,
    const PhysicalLedger& ledger,
    std::uint16_t projectileIndex) noexcept {
    return FindEarliestImpactInternal(state, ledger, projectileIndex, nullptr);
}

bool StepCombatProjectiles(
    CombatState& state,
    PhysicalLedger& ledger,
    const ClaimLedger& claimLedger) noexcept {
    if (state.phase != CombatPhase::Active) return false;
    if (!CombatStateValid(state, ledger) || !ClaimLedgerValid(claimLedger)) {
        state.phase = CombatPhase::Fatal;
        state.error = CombatError::InvariantFailure;
        return false;
    }
    bool processed[kProjectileCapacity]{};
    for (std::uint16_t pass = 0; pass < kProjectileCapacity; ++pass) {
        std::uint16_t selected = kInvalidSlotIndex;
        ImpactCandidate selectedImpact{
            ImpactKind::None, 65535u, kInvalidSlotIndex, 0xFFFFFFFFu, Vec2Q8{0, 0}};
        for (std::uint16_t index = 0; index < kProjectileCapacity; ++index) {
            if (processed[index] || state.projectiles[index].active == 0) continue;
            const ImpactCandidate impact = FindEarliestImpactInternal(state, ledger, index, processed);
            if (impact.kind == ImpactKind::None) continue;
            if (selected == kInvalidSlotIndex
                || BetterImpact(impact, selectedImpact)
                || (impact.timeQ16 == selectedImpact.timeQ16
                    && impact.kind == selectedImpact.kind
                    && impact.targetStableId == selectedImpact.targetStableId
                    && state.projectiles[index].stableId < state.projectiles[selected].stableId)) {
                selected = index;
                selectedImpact = impact;
            }
        }
        if (selected == kInvalidSlotIndex) break;
        processed[selected] = true;
        if (!ProcessImpact(state, ledger, claimLedger, selected, selectedImpact, processed)) return false;
        if (state.phase == CombatPhase::HitFreeze) break;
    }
    if (state.phase == CombatPhase::Active) {
        for (std::uint16_t index = 0; index < kProjectileCapacity; ++index) {
            ProjectileSlot& projectile = state.projectiles[index];
            if (projectile.active != 0 && !processed[index]) {
                projectile.position.x += projectile.velocity.x;
                projectile.position.y += projectile.velocity.y;
            }
        }
    }
    if (!CombatStateValid(state, ledger)) {
        state.phase = CombatPhase::Fatal;
        state.error = CombatError::InvariantFailure;
        return false;
    }
    ++state.globalTick;
    return true;
}

void TickCombatCountdowns(CombatState& state) noexcept {
    for (CombatActor& actor : state.actors) {
        if (actor.exposureGraceTicks > 0) --actor.exposureGraceTicks;
        if (actor.shotCooldownTicks > 0) --actor.shotCooldownTicks;
    }
    for (NeutralSlot& neutral : state.neutrals) {
        if (neutral.active == 0) continue;
        if (neutral.reclaimLockTicks > 0) --neutral.reclaimLockTicks;
        if (neutral.immunityTicks > 0) --neutral.immunityTicks;
        if (neutral.inertiaTicks > 0) --neutral.inertiaTicks;
    }
}

}  // namespace bp
