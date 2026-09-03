#pragma once

#include "bp_math.h"
#include "bp_types.h"

#include <cstdint>

namespace bp {

inline constexpr std::uint16_t kProjectileCapacity = 32;
inline constexpr std::uint16_t kNeutralCapacity = 64;
inline constexpr std::uint16_t kCoverCapacity = 32;
inline constexpr std::uint16_t kInvalidSlotIndex = 0xFFFFu;
inline constexpr std::uint16_t kCoreExposureShell = 40;
inline constexpr std::uint16_t kCoreExposureGraceTicks = 15;
inline constexpr std::uint8_t kCoreRadiusPixels = 3;

enum class CombatSide : std::uint8_t {
    Player = 0,
    Opponent = 1,
    Count = 2,
};

enum class CombatPhase : std::uint8_t {
    Active = 0,
    HitFreeze,
    Fatal,
};

enum class CombatError : std::uint8_t {
    None = 0,
    InvalidHandle,
    StableIdExhausted,
    NeutralCapacityExhausted,
    LedgerTransactionFailed,
    InvariantFailure,
};

enum class ImpactKind : std::uint8_t {
    None = 0,
    Core,
    Charge,
    Projectile,
    Shell,
    Cover,
    Boundary,
};

struct SlotHandle {
    std::uint16_t index;
    std::uint16_t generation;
};

struct ChargeSlot {
    Vec2Q8 position;
    Vec2Q8 directionQ15;
    std::uint32_t stableId;
    std::uint16_t generation;
    std::uint16_t mass;
    std::uint16_t stageMass;
    std::uint16_t ageTicks;
    std::uint8_t active;
};

struct ProjectileSlot {
    Vec2Q8 position;
    Vec2Q8 velocity;
    std::uint32_t stableId;
    std::uint16_t generation;
    std::uint16_t mass;
    std::uint8_t widthPixels;
    std::uint8_t heightPixels;
    CombatSide owner;
    std::uint8_t active;
};

struct NeutralSlot {
    Vec2Q8 position;
    Vec2Q8 velocity;
    std::uint32_t stableId;
    std::uint16_t generation;
    std::uint16_t mass;
    std::uint16_t reclaimLockTicks;
    std::uint16_t immunityTicks;
    std::uint16_t playerMissMass;
    std::uint16_t opponentMissMass;
    std::uint16_t protocolTagMass;
    std::uint16_t patchFeedMass;
    std::uint16_t adapterEscrowMass;
    std::uint16_t structureMass;
    std::uint8_t inertiaTicks;
    std::uint8_t active;
};

enum class CoverBehavior : std::uint8_t {
    Standard = 0,
    OwnedStructure,
    Compactor,
    Sweeper,
};

struct CoverSlot {
    RectPixels bounds;
    std::uint32_t stableId;
    std::uint16_t generation;
    std::uint16_t mass;
    PhysicalAccount backingAccount;
    CoverBehavior behavior;
    std::uint8_t active;
};

struct CombatActor {
    Vec2Q8 position;
    Vec2Q8 velocity;
    Vec2Q8 aimDirectionQ15;
    std::uint16_t exposureGraceTicks;
    std::uint16_t shotCooldownTicks;
    std::uint8_t projectileImmune;
    std::uint8_t coreHit;
};

struct SettlementSnapshot {
    PhysicalLedger physicalLedger;
    ClaimLedger claimLedger;
    std::uint32_t tick;
    CombatSide hitSide;
    std::uint8_t valid;
};

struct ImpactCandidate {
    ImpactKind kind;
    std::uint16_t timeQ16;
    std::uint16_t targetIndex;
    std::uint32_t targetStableId;
    Vec2Q8 point;
};

struct CombatState {
    ChargeSlot charges[2];
    ProjectileSlot projectiles[kProjectileCapacity];
    NeutralSlot neutrals[kNeutralCapacity];
    CoverSlot covers[kCoverCapacity];
    CombatActor actors[2];
    SettlementSnapshot settlement;
    std::uint16_t shellDamageThisTick[2];
    std::uint32_t nextStableId;
    std::uint32_t globalTick;
    CombatPhase phase;
    CombatError error;
};

void InitializeCombatState(
    CombatState& state,
    Vec2Q8 playerPosition,
    Vec2Q8 opponentPosition) noexcept;

bool CombatStateValid(const CombatState& state, const PhysicalLedger& ledger) noexcept;

bool BeginCharge(
    CombatState& state,
    PhysicalLedger& ledger,
    CombatSide side,
    Vec2Q8 directionQ15) noexcept;
bool AdvanceCharge(CombatState& state, PhysicalLedger& ledger, CombatSide side) noexcept;
bool ReleaseCharge(CombatState& state, PhysicalLedger& ledger, CombatSide side) noexcept;
bool RollbackCharge(CombatState& state, PhysicalLedger& ledger, CombatSide side) noexcept;

// CROSS_TICKET_GLUE: scripted encounters use the same projectile slots and
// ledger transaction as a released player charge, but supply their frozen
// mass, origin, direction, and speed directly.
bool SpawnScriptedProjectile(
    CombatState& state,
    PhysicalLedger& ledger,
    CombatSide side,
    std::uint16_t mass,
    Vec2Q8 position,
    Vec2Q8 directionQ15,
    std::int32_t speedQ8,
    std::uint8_t widthPixels = 0,
    std::uint8_t heightPixels = 0) noexcept;

bool AddCover(
    CombatState& state,
    const RectPixels& bounds,
    std::uint16_t mass,
    SlotHandle& handle) noexcept;
// CROSS_TICKET_GLUE: Act 2 processes and owned structures participate in the
// deterministic collision sweep while retaining their authoritative account.
bool AddBackedCover(
    CombatState& state,
    const RectPixels& bounds,
    std::uint16_t mass,
    PhysicalAccount backingAccount,
    CoverBehavior behavior,
    SlotHandle& handle) noexcept;
bool AddNeutral(
    CombatState& state,
    Vec2Q8 position,
    std::uint16_t mass,
    std::uint16_t playerMissMass,
    std::uint16_t opponentMissMass,
    SlotHandle& handle) noexcept;

bool ProjectileHandleValid(const CombatState& state, SlotHandle handle) noexcept;
bool NeutralHandleValid(const CombatState& state, SlotHandle handle) noexcept;
bool CoverHandleValid(const CombatState& state, SlotHandle handle) noexcept;
bool ResolveProjectileHandle(CombatState& state, SlotHandle handle, std::uint16_t& index) noexcept;
bool ResolveNeutralHandle(CombatState& state, SlotHandle handle, std::uint16_t& index) noexcept;
bool ResolveCoverHandle(CombatState& state, SlotHandle handle, std::uint16_t& index) noexcept;

std::int32_t ProjectileSpeedQ8(std::uint16_t mass) noexcept;
std::uint16_t ProjectileShellDamage(std::uint16_t mass) noexcept;
std::uint16_t ProjectileCoverDamage(std::uint16_t mass) noexcept;
std::int32_t ProjectileRadiusPixels(std::uint16_t mass) noexcept;
std::int32_t RecoilQ8(std::uint16_t mass) noexcept;

ImpactCandidate FindEarliestImpact(
    const CombatState& state,
    const PhysicalLedger& ledger,
    std::uint16_t projectileIndex) noexcept;
bool StepCombatProjectiles(
    CombatState& state,
    PhysicalLedger& ledger,
    const ClaimLedger& claimLedger) noexcept;
void TickCombatCountdowns(CombatState& state) noexcept;

}  // namespace bp
