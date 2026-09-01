#pragma once

#include "bp_types.h"

#include <cstddef>
#include <cstdint>

namespace bp {

inline constexpr std::size_t kSaveV2Bytes = 96;
inline constexpr std::uint16_t kSaveV2Version = 2;

struct SaveData {
    std::uint32_t flags;
    std::uint32_t options;
    std::uint32_t endingsSeen;
    std::uint32_t bestFullRunTicks;
    ClaimLedger claimLedger;
    std::uint32_t choiceBits;
    std::uint16_t adjudicatedMass;
    std::uint32_t runSeed;
    std::uint8_t encounterIndex;
    std::uint8_t role;
    std::uint8_t entityStatus[kSettlementEncounterCount];
    std::uint8_t tutorial;
};

std::uint32_t Fnv1a32(const std::uint8_t* bytes, std::size_t count) noexcept;

bool SerializeSaveV2(
    const SaveData& save,
    std::uint8_t (&bytes)[kSaveV2Bytes]) noexcept;

bool DeserializeSaveV2(
    const std::uint8_t* bytes,
    std::size_t count,
    SaveData& save) noexcept;

}  // namespace bp
