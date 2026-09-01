#include "bp_claim_ledger.h"
#include "bp_physical_ledger.h"
#include "bp_save.h"
#include "bp_types.h"

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>

namespace {

std::uint32_t gChecks;
std::uint32_t gFailures;

void Check(bool condition, const char* name) noexcept {
    ++gChecks;
    if (!condition) {
        ++gFailures;
        std::printf("FAIL\t%s\n", name);
    }
}

std::uint8_t OwnerIndex(bp::ClaimOwner owner) noexcept {
    return static_cast<std::uint8_t>(owner);
}

std::uint8_t AccountIndex(bp::PhysicalAccount account) noexcept {
    return static_cast<std::uint8_t>(account);
}

bool SameSaveData(const bp::SaveData& left, const bp::SaveData& right) noexcept {
    if (left.flags != right.flags
        || left.options != right.options
        || left.endingsSeen != right.endingsSeen
        || left.bestFullRunTicks != right.bestFullRunTicks
        || left.choiceBits != right.choiceBits
        || left.adjudicatedMass != right.adjudicatedMass
        || left.runSeed != right.runSeed
        || left.encounterIndex != right.encounterIndex
        || left.role != right.role
        || left.tutorial != right.tutorial) {
        return false;
    }
    for (std::uint8_t index = 0; index < bp::kClaimOwnerCount; ++index) {
        if (left.claimLedger.mass[index] != right.claimLedger.mass[index]) {
            return false;
        }
    }
    for (std::uint8_t index = 0; index < bp::kSettlementEncounterCount; ++index) {
        if (left.entityStatus[index] != right.entityStatus[index]) {
            return false;
        }
    }
    return true;
}

void TestInitialClaimLedger() noexcept {
    constexpr std::uint16_t expected[bp::kClaimOwnerCount] = {
        0, 320, 48, 64, 224, 112, 72, 240, 48, 72, 240, 0,
    };
    const bp::ClaimLedger ledger = bp::MakeInitialClaimLedger();
    Check(bp::ClaimLedgerTotal(ledger) == bp::kLedgerTotal, "claim initial total");
    Check(bp::ClaimLedgerValid(ledger), "claim initial valid");
    for (std::uint8_t owner = 0; owner < bp::kClaimOwnerCount; ++owner) {
        Check(ledger.mass[owner] == expected[owner], "claim initial owner mass");
    }
}

void TestClaimTransfers() noexcept {
    for (std::uint8_t from = 0; from < bp::kClaimOwnerCount; ++from) {
        for (std::uint8_t to = 0; to < bp::kClaimOwnerCount; ++to) {
            const bp::ClaimLedger initial = bp::MakeInitialClaimLedger();
            const std::uint16_t available = initial.mass[from];
            const std::uint16_t amounts[4] = {
                0,
                1,
                available,
                static_cast<std::uint16_t>(available + 1),
            };
            for (std::uint8_t amountIndex = 0; amountIndex < 4; ++amountIndex) {
                bp::ClaimLedger ledger = initial;
                const bp::ClaimLedger before = ledger;
                const std::uint16_t amount = amounts[amountIndex];
                const bool expected = from == to ? amount == 0 : amount <= available;
                const bool result = bp::ClaimTransfer(
                    ledger,
                    static_cast<bp::ClaimOwner>(from),
                    static_cast<bp::ClaimOwner>(to),
                    amount);
                Check(result == expected, "claim transfer result");
                if (result) {
                    Check(bp::ClaimLedgerTotal(ledger) == bp::kLedgerTotal, "claim transfer total");
                } else {
                    Check(std::memcmp(&ledger, &before, sizeof(ledger)) == 0, "claim transfer rollback");
                }
            }
        }
    }

    bp::ClaimLedger invalid = bp::MakeInitialClaimLedger();
    invalid.mass[OwnerIndex(bp::ClaimOwner::System)] -= 1;
    const bp::ClaimLedger invalidBefore = invalid;
    Check(!bp::ClaimTransfer(invalid, bp::ClaimOwner::System, bp::ClaimOwner::Player, 1),
        "claim invalid total rejected");
    Check(std::memcmp(&invalid, &invalidBefore, sizeof(invalid)) == 0,
        "claim invalid total rollback");

    bp::ClaimLedger invalidOwner = bp::MakeInitialClaimLedger();
    const bp::ClaimLedger invalidOwnerBefore = invalidOwner;
    Check(!bp::ClaimTransfer(
              invalidOwner,
              bp::ClaimOwner::System,
              static_cast<bp::ClaimOwner>(bp::kClaimOwnerCount),
              1),
        "claim invalid owner rejected");
    Check(std::memcmp(&invalidOwner, &invalidOwnerBefore, sizeof(invalidOwner)) == 0,
        "claim invalid owner rollback");
}

void TestPhysicalInitialization() noexcept {
    struct EncounterMasses {
        std::uint16_t opponent;
        std::uint16_t field;
        std::uint16_t neutral;
        std::uint16_t locked;
    };
    constexpr EncounterMasses encounters[] = {
        {48, 160, 64, 672},
        {64, 160, 64, 656},
        {224, 320, 128, 272},
        {112, 192, 64, 576},
        {72, 192, 48, 632},
        {192, 384, 96, 272},
        {48, 192, 96, 608},
        {72, 160, 64, 648},
        {240, 256, 160, 288},
        {256, 320, 128, 240},
    };

    for (const EncounterMasses& encounter : encounters) {
        bp::PhysicalLedger ledger{};
        Check(bp::InitializePhysicalLedger(
                  ledger, 240, encounter.opponent, encounter.field, encounter.neutral),
            "physical encounter initialize");
        Check(bp::PhysicalLedgerTotal(ledger) == bp::kLedgerTotal, "physical encounter total");
        Check(ledger.mass[AccountIndex(bp::PhysicalAccount::PlayerCore)] == 16,
            "physical player core");
        Check(ledger.mass[AccountIndex(bp::PhysicalAccount::PlayerShell)] == 224,
            "physical player shell");
        Check(ledger.mass[AccountIndex(bp::PhysicalAccount::OpponentCore)] == 16,
            "physical opponent core");
        Check(ledger.mass[AccountIndex(bp::PhysicalAccount::OpponentShell)] == encounter.opponent - 16,
            "physical opponent shell");
        Check(ledger.mass[AccountIndex(bp::PhysicalAccount::InstalledHud)] == 224,
            "physical installed hud");
        Check(ledger.mass[AccountIndex(bp::PhysicalAccount::Cover)] == encounter.field,
            "physical field mass");
        Check(ledger.mass[AccountIndex(bp::PhysicalAccount::Neutral)] == encounter.neutral,
            "physical neutral mass");
        Check(ledger.mass[AccountIndex(bp::PhysicalAccount::ProtocolActive)] == 32,
            "physical protocol mass");
        Check(ledger.mass[AccountIndex(bp::PhysicalAccount::LockedBands)] == encounter.locked,
            "physical locked bands");
    }

    bp::PhysicalLedger sentinel{};
    std::memset(&sentinel, 0xa5, sizeof(sentinel));
    const bp::PhysicalLedger before = sentinel;
    Check(!bp::InitializePhysicalLedger(sentinel, 63, 48, 160, 64),
        "physical player minimum rejected");
    Check(std::memcmp(&sentinel, &before, sizeof(sentinel)) == 0,
        "physical initialize rollback");
    Check(!bp::InitializePhysicalLedger(sentinel, 240, 1000, 1000, 1000),
        "physical overflow rejected");
    Check(std::memcmp(&sentinel, &before, sizeof(sentinel)) == 0,
        "physical overflow rollback");
}

void TestPhysicalTransfers() noexcept {
    bp::PhysicalLedger base{};
    Check(bp::InitializePhysicalLedger(base, 240, 224, 320, 128),
        "physical transfer base initialize");
    for (std::uint8_t from = 0; from < bp::kPhysicalAccountCount; ++from) {
        for (std::uint8_t to = 0; to < bp::kPhysicalAccountCount; ++to) {
            const std::uint16_t available = base.mass[from];
            const std::uint16_t amounts[4] = {
                0,
                1,
                available,
                static_cast<std::uint16_t>(available + 1),
            };
            for (std::uint8_t amountIndex = 0; amountIndex < 4; ++amountIndex) {
                bp::PhysicalLedger ledger = base;
                const bp::PhysicalLedger before = ledger;
                const std::uint16_t amount = amounts[amountIndex];
                const bool expected = from == to ? amount == 0 : amount <= available;
                const bool result = bp::PhysicalTransfer(
                    ledger,
                    static_cast<bp::PhysicalAccount>(from),
                    static_cast<bp::PhysicalAccount>(to),
                    amount);
                Check(result == expected, "physical transfer result");
                if (result) {
                    Check(bp::PhysicalLedgerTotal(ledger) == bp::kLedgerTotal,
                        "physical transfer total");
                } else {
                    Check(std::memcmp(&ledger, &before, sizeof(ledger)) == 0,
                        "physical transfer rollback");
                }
            }
        }
    }
}

struct ClaimDelta {
    bp::ClaimOwner owner;
    std::int16_t amount;
};

struct SettlementCase {
    bp::EncounterId encounter;
    bp::ChoiceId choice;
    ClaimDelta deltas[5];
    std::uint8_t deltaCount;
};

void TestSettlementTable() noexcept {
    constexpr SettlementCase cases[] = {
        {bp::EncounterId::Patch, bp::ChoiceId::First, {{bp::ClaimOwner::Patch, -16}, {bp::ClaimOwner::Player, 16}}, 2},
        {bp::EncounterId::Patch, bp::ChoiceId::Second, {}, 0},
        {bp::EncounterId::Patch, bp::ChoiceId::Third, {{bp::ClaimOwner::Patch, -16}, {bp::ClaimOwner::Common, 16}}, 2},
        {bp::EncounterId::Pair, bp::ChoiceId::First, {{bp::ClaimOwner::Pair, -16}, {bp::ClaimOwner::Player, 16}}, 2},
        {bp::EncounterId::Pair, bp::ChoiceId::Second, {}, 0},
        {bp::EncounterId::Pair, bp::ChoiceId::Third, {}, 0},
        {bp::EncounterId::Vessel, bp::ChoiceId::First, {{bp::ClaimOwner::System, -64}, {bp::ClaimOwner::Vessel, -144}, {bp::ClaimOwner::Player, 208}}, 3},
        {bp::EncounterId::Vessel, bp::ChoiceId::Second, {{bp::ClaimOwner::System, -64}, {bp::ClaimOwner::Vessel, -80}, {bp::ClaimOwner::Player, 144}}, 3},
        {bp::EncounterId::Vessel, bp::ChoiceId::Third, {{bp::ClaimOwner::System, -64}, {bp::ClaimOwner::Vessel, -144}, {bp::ClaimOwner::Player, 144}, {bp::ClaimOwner::Common, 64}}, 4},
        {bp::EncounterId::Keeper, bp::ChoiceId::First, {{bp::ClaimOwner::Keeper, -64}, {bp::ClaimOwner::Player, 64}}, 2},
        {bp::EncounterId::Keeper, bp::ChoiceId::Second, {}, 0},
        {bp::EncounterId::Keeper, bp::ChoiceId::Third, {{bp::ClaimOwner::Keeper, -64}, {bp::ClaimOwner::Common, 64}}, 2},
        {bp::EncounterId::Adapter, bp::ChoiceId::First, {{bp::ClaimOwner::Frame, -48}, {bp::ClaimOwner::Player, 24}, {bp::ClaimOwner::Adapter, 24}}, 3},
        {bp::EncounterId::Adapter, bp::ChoiceId::Second, {{bp::ClaimOwner::Frame, -48}, {bp::ClaimOwner::Adapter, 48}}, 2},
        {bp::EncounterId::Adapter, bp::ChoiceId::Third, {{bp::ClaimOwner::Frame, -48}, {bp::ClaimOwner::Player, 32}, {bp::ClaimOwner::Adapter, 16}}, 3},
        {bp::EncounterId::Frame, bp::ChoiceId::First, {{bp::ClaimOwner::Frame, -64}, {bp::ClaimOwner::Player, 64}}, 2},
        {bp::EncounterId::Frame, bp::ChoiceId::Second, {}, 0},
        {bp::EncounterId::Frame, bp::ChoiceId::Third, {{bp::ClaimOwner::Frame, -64}, {bp::ClaimOwner::Common, 64}}, 2},
        {bp::EncounterId::Index, bp::ChoiceId::First, {{bp::ClaimOwner::Index, -32}, {bp::ClaimOwner::Player, 32}}, 2},
        {bp::EncounterId::Index, bp::ChoiceId::Second, {}, 0},
        {bp::EncounterId::Index, bp::ChoiceId::Third, {{bp::ClaimOwner::Index, -32}, {bp::ClaimOwner::Adapter, 16}, {bp::ClaimOwner::Common, 16}}, 3},
        {bp::EncounterId::Witness, bp::ChoiceId::First, {}, 0},
        {bp::EncounterId::Witness, bp::ChoiceId::Second, {{bp::ClaimOwner::Witness, -32}, {bp::ClaimOwner::Common, 32}}, 2},
        {bp::EncounterId::Witness, bp::ChoiceId::Third, {{bp::ClaimOwner::Witness, -48}, {bp::ClaimOwner::Player, 48}}, 2},
        {bp::EncounterId::Current, bp::ChoiceId::First, {{bp::ClaimOwner::Current, -64}, {bp::ClaimOwner::Player, 64}}, 2},
        {bp::EncounterId::Current, bp::ChoiceId::Second, {}, 0},
        {bp::EncounterId::Current, bp::ChoiceId::Third, {{bp::ClaimOwner::Current, -64}, {bp::ClaimOwner::Patch, 16}, {bp::ClaimOwner::Pair, 16}, {bp::ClaimOwner::Adapter, 16}, {bp::ClaimOwner::Common, 16}}, 5},
    };

    for (const SettlementCase& settlement : cases) {
        bp::ClaimLedger ledger = bp::MakeInitialClaimLedger();
        const bp::ClaimLedger initial = ledger;
        bp::NarrativeState narrative{};
        Check(bp::ApplySettlement(ledger, narrative, settlement.encounter, settlement.choice),
            "settlement table apply");
        std::int32_t expected[bp::kClaimOwnerCount]{};
        for (std::uint8_t owner = 0; owner < bp::kClaimOwnerCount; ++owner) {
            expected[owner] = initial.mass[owner];
        }
        for (std::uint8_t index = 0; index < settlement.deltaCount; ++index) {
            expected[OwnerIndex(settlement.deltas[index].owner)] += settlement.deltas[index].amount;
        }
        for (std::uint8_t owner = 0; owner < bp::kClaimOwnerCount; ++owner) {
            Check(ledger.mass[owner] == expected[owner], "settlement table owner result");
        }
        Check(bp::ClaimLedgerTotal(ledger) == bp::kLedgerTotal, "settlement table total");
        const std::uint32_t shift = static_cast<std::uint32_t>(settlement.encounter) * 2u;
        Check(((narrative.choiceBits >> shift) & 3u) == static_cast<std::uint32_t>(settlement.choice),
            "settlement choice bits");
    }
}

void TestVesselMinimumSequence() noexcept {
    for (std::uint8_t patchChoice = 0; patchChoice < 3; ++patchChoice) {
        for (std::uint8_t pairChoice = 0; pairChoice < 3; ++pairChoice) {
            bp::ClaimLedger ledger = bp::MakeInitialClaimLedger();
            bp::NarrativeState narrative{};
            Check(bp::ApplySettlement(ledger, narrative, bp::EncounterId::Patch,
                      static_cast<bp::ChoiceId>(patchChoice)),
                "vessel sequence patch");
            Check(bp::ApplySettlement(ledger, narrative, bp::EncounterId::Pair,
                      static_cast<bp::ChoiceId>(pairChoice)),
                "vessel sequence pair");
            Check(bp::ApplySettlement(ledger, narrative, bp::EncounterId::Vessel,
                      bp::ChoiceId::Second),
                "vessel sequence settlement");
            Check(ledger.mass[OwnerIndex(bp::ClaimOwner::Player)] == 144,
                "vessel minimum player 144");
            Check(ledger.mass[OwnerIndex(bp::ClaimOwner::System)] == 256,
                "vessel title transfer 64");
            Check(bp::ClaimLedgerTotal(ledger) == bp::kLedgerTotal,
                "vessel minimum total");
        }
    }
}

void TestAllChoiceCombinations() noexcept {
    constexpr std::uint32_t combinationCount = 19683;
    for (std::uint32_t combination = 0; combination < combinationCount; ++combination) {
        bp::ClaimLedger ledger = bp::MakeInitialClaimLedger();
        bp::NarrativeState narrative{};
        std::uint32_t digits = combination;
        std::uint32_t expectedChoiceBits = 0;
        bool applied = true;
        for (std::uint8_t encounter = 0; encounter < bp::kSettlementEncounterCount; ++encounter) {
            const std::uint8_t choice = static_cast<std::uint8_t>(digits % 3u);
            digits /= 3u;
            expectedChoiceBits |= static_cast<std::uint32_t>(choice) << (encounter * 2u);
            applied = applied && bp::ApplySettlement(
                ledger,
                narrative,
                static_cast<bp::EncounterId>(encounter),
                static_cast<bp::ChoiceId>(choice));
            if (!applied) {
                break;
            }
            Check(bp::ClaimLedgerTotal(ledger) == bp::kLedgerTotal,
                "combination intermediate total");
            for (std::uint8_t owner = 0; owner < bp::kClaimOwnerCount; ++owner) {
                Check(ledger.mass[owner] <= bp::kLedgerTotal, "combination nonnegative bounded mass");
            }
        }
        Check(applied, "combination applies");
        Check(bp::ClaimLedgerValid(ledger), "combination final total");
        Check(narrative.choiceBits == expectedChoiceBits, "combination choice bits");
    }
}

void TestSettlementRollback() noexcept {
    bp::ClaimLedger ledger = bp::MakeInitialClaimLedger();
    bp::NarrativeState narrative{};
    narrative.choiceBits = 0x2aaaau;
    const bp::ClaimLedger beforeLedger = ledger;
    const bp::NarrativeState beforeNarrative = narrative;
    Check(!bp::ApplySettlement(
              ledger, narrative, bp::EncounterId::Precedent, bp::ChoiceId::First),
        "settlement invalid encounter rejected");
    Check(std::memcmp(&ledger, &beforeLedger, sizeof(ledger)) == 0,
        "settlement invalid encounter ledger rollback");
    Check(std::memcmp(&narrative, &beforeNarrative, sizeof(narrative)) == 0,
        "settlement invalid encounter narrative rollback");

    Check(!bp::ApplySettlement(
              ledger, narrative, bp::EncounterId::Patch, bp::ChoiceId::Count),
        "settlement invalid choice rejected");
    Check(std::memcmp(&ledger, &beforeLedger, sizeof(ledger)) == 0,
        "settlement invalid choice ledger rollback");
    Check(std::memcmp(&narrative, &beforeNarrative, sizeof(narrative)) == 0,
        "settlement invalid choice narrative rollback");

    ledger = bp::MakeInitialClaimLedger();
    ledger.mass[OwnerIndex(bp::ClaimOwner::System)] += ledger.mass[OwnerIndex(bp::ClaimOwner::Patch)];
    ledger.mass[OwnerIndex(bp::ClaimOwner::Patch)] = 0;
    const bp::ClaimLedger insufficientLedger = ledger;
    const bp::NarrativeState insufficientNarrative = narrative;
    Check(!bp::ApplySettlement(
              ledger, narrative, bp::EncounterId::Patch, bp::ChoiceId::First),
        "settlement insufficient source rejected");
    Check(std::memcmp(&ledger, &insufficientLedger, sizeof(ledger)) == 0,
        "settlement insufficient ledger rollback");
    Check(std::memcmp(&narrative, &insufficientNarrative, sizeof(narrative)) == 0,
        "settlement insufficient narrative rollback");
}

bp::SaveData MakeGoldenSave() noexcept {
    bp::SaveData save{};
    save.flags = 0x0fu;
    save.options = 0x37au;
    save.endingsSeen = 0x1fu;
    save.bestFullRunTicks = 0x01020304u;
    save.claimLedger = bp::MakeInitialClaimLedger();
    save.choiceBits = 0x2aaaau;
    save.adjudicatedMass = 1440;
    save.runSeed = 0x89abcdefu;
    save.encounterIndex = 8;
    save.role = 4;
    for (std::uint8_t index = 0; index < bp::kSettlementEncounterCount; ++index) {
        save.entityStatus[index] = static_cast<std::uint8_t>(index + 1);
    }
    save.tutorial = 1;
    return save;
}

void RefreshChecksum(std::uint8_t (&bytes)[bp::kSaveV2Bytes]) noexcept {
    const std::uint32_t checksum = bp::Fnv1a32(bytes, 92);
    bytes[92] = static_cast<std::uint8_t>(checksum & 0xffu);
    bytes[93] = static_cast<std::uint8_t>((checksum >> 8) & 0xffu);
    bytes[94] = static_cast<std::uint8_t>((checksum >> 16) & 0xffu);
    bytes[95] = static_cast<std::uint8_t>((checksum >> 24) & 0xffu);
}

void TestSaveGolden() noexcept {
    constexpr std::uint8_t golden[bp::kSaveV2Bytes] = {
        0x42,0x50,0x31,0x34,0x02,0x00,0x60,0x00,0x0f,0x00,0x00,0x00,0x7a,0x03,0x00,0x00,
        0x1f,0x00,0x00,0x00,0x04,0x03,0x02,0x01,0x00,0x00,0x40,0x01,0x30,0x00,0x40,0x00,
        0xe0,0x00,0x70,0x00,0x48,0x00,0xf0,0x00,0x30,0x00,0x48,0x00,0xf0,0x00,0x00,0x00,
        0xaa,0xaa,0x02,0x00,0xa0,0x05,0xef,0xcd,0xab,0x89,0x08,0x04,0x01,0x02,0x03,0x04,
        0x05,0x06,0x07,0x08,0x09,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x33,0x3c,0x54,0x61,
    };

    const bp::SaveData source = MakeGoldenSave();
    std::uint8_t bytes[bp::kSaveV2Bytes]{};
    Check(bp::SerializeSaveV2(source, bytes), "save golden serialize");
    Check(std::memcmp(bytes, golden, sizeof(golden)) == 0, "save 96 byte golden");
    Check(bp::Fnv1a32(bytes, 92) == 0x61543c33u, "save golden fnv1a32");
    for (std::size_t index = 70; index < 92; ++index) {
        Check(bytes[index] == 0, "save reserved zero");
    }

    bp::SaveData decoded{};
    Check(bp::DeserializeSaveV2(bytes, sizeof(bytes), decoded), "save golden deserialize");
    Check(SameSaveData(source, decoded), "save golden round trip");

    for (std::size_t changed = 0; changed < bp::kSaveV2Bytes; ++changed) {
        std::uint8_t corrupt[bp::kSaveV2Bytes];
        std::memcpy(corrupt, golden, sizeof(corrupt));
        corrupt[changed] ^= 1u;
        bp::SaveData output{};
        std::memset(&output, 0xa5, sizeof(output));
        unsigned char before[sizeof(output)];
        std::memcpy(before, &output, sizeof(output));
        Check(!bp::DeserializeSaveV2(corrupt, sizeof(corrupt), output),
            "save corruption rejected");
        Check(std::memcmp(&output, before, sizeof(output)) == 0,
            "save corruption output rollback");
    }

    for (std::size_t partial = 0; partial < bp::kSaveV2Bytes; ++partial) {
        bp::SaveData output{};
        Check(!bp::DeserializeSaveV2(golden, partial, output), "save partial rejected");
    }

    std::uint8_t invalid[bp::kSaveV2Bytes];
    std::memcpy(invalid, golden, sizeof(invalid));
    invalid[4] = 1;
    RefreshChecksum(invalid);
    bp::SaveData output{};
    Check(!bp::DeserializeSaveV2(invalid, sizeof(invalid), output), "save old version rejected");

    std::memcpy(invalid, golden, sizeof(invalid));
    invalid[70] = 1;
    RefreshChecksum(invalid);
    Check(!bp::DeserializeSaveV2(invalid, sizeof(invalid), output), "save reserved rejected");

    std::memcpy(invalid, golden, sizeof(invalid));
    invalid[8] |= 0x10u;
    RefreshChecksum(invalid);
    Check(!bp::DeserializeSaveV2(invalid, sizeof(invalid), output), "save preview flag rejected");

    std::memcpy(invalid, golden, sizeof(invalid));
    invalid[12] = 11;
    RefreshChecksum(invalid);
    Check(!bp::DeserializeSaveV2(invalid, sizeof(invalid), output), "save volume rejected");

    std::memcpy(invalid, golden, sizeof(invalid));
    invalid[48] = 3;
    RefreshChecksum(invalid);
    Check(!bp::DeserializeSaveV2(invalid, sizeof(invalid), output), "save choice value rejected");

    std::memcpy(invalid, golden, sizeof(invalid));
    invalid[26] -= 1;
    RefreshChecksum(invalid);
    Check(!bp::DeserializeSaveV2(invalid, sizeof(invalid), output), "save claim total rejected");
}

void TestSaveSerializeRollback() noexcept {
    const bp::SaveData valid = MakeGoldenSave();
    bp::SaveData invalid = valid;
    std::uint8_t bytes[bp::kSaveV2Bytes];

    invalid.flags |= 0x10u;
    std::memset(bytes, 0xa5, sizeof(bytes));
    std::uint8_t before[bp::kSaveV2Bytes];
    std::memcpy(before, bytes, sizeof(before));
    Check(!bp::SerializeSaveV2(invalid, bytes), "save serialize invalid flags");
    Check(std::memcmp(bytes, before, sizeof(bytes)) == 0, "save serialize flags rollback");

    invalid = valid;
    invalid.options = 11;
    Check(!bp::SerializeSaveV2(invalid, bytes), "save serialize invalid options");
    Check(std::memcmp(bytes, before, sizeof(bytes)) == 0, "save serialize options rollback");

    invalid = valid;
    invalid.choiceBits = 3;
    Check(!bp::SerializeSaveV2(invalid, bytes), "save serialize invalid choice");
    Check(std::memcmp(bytes, before, sizeof(bytes)) == 0, "save serialize choice rollback");

    invalid = valid;
    invalid.claimLedger.mass[OwnerIndex(bp::ClaimOwner::System)] -= 1;
    Check(!bp::SerializeSaveV2(invalid, bytes), "save serialize invalid claim ledger");
    Check(std::memcmp(bytes, before, sizeof(bytes)) == 0, "save serialize claim rollback");
}

}  // namespace

int main() {
    TestInitialClaimLedger();
    TestClaimTransfers();
    TestPhysicalInitialization();
    TestPhysicalTransfers();
    TestSettlementTable();
    TestVesselMinimumSequence();
    TestAllChoiceCombinations();
    TestSettlementRollback();
    TestSaveGolden();
    TestSaveSerializeRollback();

    if (gFailures != 0) {
        std::printf("T02_FAIL\tchecks=%u\tfailures=%u\n", gChecks, gFailures);
        return 1;
    }
    std::printf("T02_PASS\tchecks=%u\tcombinations=19683\tsave_bytes=96\n", gChecks);
    return 0;
}
