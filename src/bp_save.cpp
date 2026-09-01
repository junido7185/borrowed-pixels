#include "bp_save.h"

#include "bp_claim_ledger.h"

#include <cstddef>
#include <cstdint>

namespace bp {
namespace {

constexpr std::uint32_t kAllowedSaveFlags = 0x0000000fu;
constexpr std::uint32_t kAllowedOptions = 0x000003ffu;
constexpr std::uint32_t kAllowedEndings = 0x0000001fu;
constexpr std::uint32_t kAllowedChoiceBits = 0x0003ffffu;

void WriteU16(std::uint8_t* bytes, std::size_t offset, std::uint16_t value) noexcept {
    bytes[offset] = static_cast<std::uint8_t>(value & 0xffu);
    bytes[offset + 1] = static_cast<std::uint8_t>((value >> 8) & 0xffu);
}

void WriteU32(std::uint8_t* bytes, std::size_t offset, std::uint32_t value) noexcept {
    bytes[offset] = static_cast<std::uint8_t>(value & 0xffu);
    bytes[offset + 1] = static_cast<std::uint8_t>((value >> 8) & 0xffu);
    bytes[offset + 2] = static_cast<std::uint8_t>((value >> 16) & 0xffu);
    bytes[offset + 3] = static_cast<std::uint8_t>((value >> 24) & 0xffu);
}

std::uint16_t ReadU16(const std::uint8_t* bytes, std::size_t offset) noexcept {
    return static_cast<std::uint16_t>(
        static_cast<std::uint16_t>(bytes[offset])
        | (static_cast<std::uint16_t>(bytes[offset + 1]) << 8));
}

std::uint32_t ReadU32(const std::uint8_t* bytes, std::size_t offset) noexcept {
    return static_cast<std::uint32_t>(bytes[offset])
        | (static_cast<std::uint32_t>(bytes[offset + 1]) << 8)
        | (static_cast<std::uint32_t>(bytes[offset + 2]) << 16)
        | (static_cast<std::uint32_t>(bytes[offset + 3]) << 24);
}

bool ChoiceBitsValid(std::uint32_t choiceBits) noexcept {
    if ((choiceBits & ~kAllowedChoiceBits) != 0) {
        return false;
    }
    for (std::uint8_t encounter = 0; encounter < kSettlementEncounterCount; ++encounter) {
        if (((choiceBits >> (encounter * 2u)) & 3u) == 3u) {
            return false;
        }
    }
    return true;
}

bool SaveFieldsValid(const SaveData& save) noexcept {
    if ((save.flags & ~kAllowedSaveFlags) != 0
        || (save.options & ~kAllowedOptions) != 0
        || (save.endingsSeen & ~kAllowedEndings) != 0
        || !ChoiceBitsValid(save.choiceBits)
        || !ClaimLedgerValid(save.claimLedger)) {
        return false;
    }

    const std::uint32_t masterVolume = save.options & 0x0fu;
    const std::uint32_t colorAssist = (save.options >> 7) & 3u;
    return masterVolume <= 10 && colorAssist <= 2;
}

bool ReservedBytesAreZero(const std::uint8_t* bytes) noexcept {
    for (std::size_t index = 70; index < 92; ++index) {
        if (bytes[index] != 0) {
            return false;
        }
    }
    return true;
}

}  // namespace

std::uint32_t Fnv1a32(const std::uint8_t* bytes, std::size_t count) noexcept {
    std::uint32_t hash = 2166136261u;
    if (!bytes && count != 0) {
        return 0;
    }
    for (std::size_t index = 0; index < count; ++index) {
        hash ^= bytes[index];
        hash *= 16777619u;
    }
    return hash;
}

bool SerializeSaveV2(
    const SaveData& save,
    std::uint8_t (&bytes)[kSaveV2Bytes]) noexcept {
    if (!SaveFieldsValid(save)) {
        return false;
    }

    std::uint8_t candidate[kSaveV2Bytes]{};
    candidate[0] = 'B';
    candidate[1] = 'P';
    candidate[2] = '1';
    candidate[3] = '4';
    WriteU16(candidate, 4, kSaveV2Version);
    WriteU16(candidate, 6, static_cast<std::uint16_t>(kSaveV2Bytes));
    WriteU32(candidate, 8, save.flags);
    WriteU32(candidate, 12, save.options);
    WriteU32(candidate, 16, save.endingsSeen);
    WriteU32(candidate, 20, save.bestFullRunTicks);
    for (std::uint8_t owner = 0; owner < kClaimOwnerCount; ++owner) {
        WriteU16(candidate, 24 + static_cast<std::size_t>(owner) * 2, save.claimLedger.mass[owner]);
    }
    WriteU32(candidate, 48, save.choiceBits);
    WriteU16(candidate, 52, save.adjudicatedMass);
    WriteU32(candidate, 54, save.runSeed);
    candidate[58] = save.encounterIndex;
    candidate[59] = save.role;
    for (std::uint8_t index = 0; index < kSettlementEncounterCount; ++index) {
        candidate[60 + index] = save.entityStatus[index];
    }
    candidate[69] = save.tutorial;
    WriteU32(candidate, 92, Fnv1a32(candidate, 92));

    for (std::size_t index = 0; index < kSaveV2Bytes; ++index) {
        bytes[index] = candidate[index];
    }
    return true;
}

bool DeserializeSaveV2(
    const std::uint8_t* bytes,
    std::size_t count,
    SaveData& save) noexcept {
    if (!bytes || count != kSaveV2Bytes
        || bytes[0] != 'B' || bytes[1] != 'P' || bytes[2] != '1' || bytes[3] != '4'
        || ReadU16(bytes, 4) != kSaveV2Version
        || ReadU16(bytes, 6) != kSaveV2Bytes
        || !ReservedBytesAreZero(bytes)
        || ReadU32(bytes, 92) != Fnv1a32(bytes, 92)) {
        return false;
    }

    SaveData candidate{};
    candidate.flags = ReadU32(bytes, 8);
    candidate.options = ReadU32(bytes, 12);
    candidate.endingsSeen = ReadU32(bytes, 16);
    candidate.bestFullRunTicks = ReadU32(bytes, 20);
    for (std::uint8_t owner = 0; owner < kClaimOwnerCount; ++owner) {
        candidate.claimLedger.mass[owner] = ReadU16(bytes, 24 + static_cast<std::size_t>(owner) * 2);
    }
    candidate.choiceBits = ReadU32(bytes, 48);
    candidate.adjudicatedMass = ReadU16(bytes, 52);
    candidate.runSeed = ReadU32(bytes, 54);
    candidate.encounterIndex = bytes[58];
    candidate.role = bytes[59];
    for (std::uint8_t index = 0; index < kSettlementEncounterCount; ++index) {
        candidate.entityStatus[index] = bytes[60 + index];
    }
    candidate.tutorial = bytes[69];

    if (!SaveFieldsValid(candidate)) {
        return false;
    }
    save = candidate;
    return true;
}

}  // namespace bp
