#include "RandomItemTable.h"
#include "port/oracle/OracleStatePut.h" // MK64 Online FASE 1.4b: oracle state hash
#include "defines.h"
#include "port/Game.h"

extern "C" {
#include "racing/math_util.h"
}

RandomItemTable::RandomItemTable() {
    mDistanceEnabled = false;
}

bool RandomItemTable::Add(const std::string& resourceName, uint32_t rank, uint32_t weight, float distance) {
    const ItemInfo* info = gItemRegistry.GetInfo(resourceName);
    if (!info) {
        printf("[ItemTables] Failed to add %s to table,\n  item name passed into resourceName does not exist!\n", resourceName.c_str());
        return false;
    }

    if (rank >= mTable.size()) {
        if (rank >= NUM_PLAYERS) {
            printf("[ItemTables] Failed to add item table, rank value too high!\n");
            return false;
        }
        mTable.resize(rank + 1);
    }

    // Check if already exists
    for (const ItemEntry& entry : mTable[rank]) {
        if (entry.Id == info->Id) {
            printf("[ItemTables] %s already exists for this rank, skipping...\n", resourceName.c_str());
            return false;
        }
    }

    mTable[rank].push_back(ItemEntry{info->Id, weight, distance});

    return true;
}

uint8_t RandomItemTable::Roll(uint32_t rank) const {
    if (rank >= mTable.size()) {
        printf("[ItemTables] [Roll] Invalid rank input %d. Giving player a none item\n", rank);
        return ITEM_NONE;
    }

    // Items that could be selected for this rank
    const std::vector<ItemEntry>& itemList = mTable.at(rank);
    if (itemList.empty()) {
        printf("[ItemTables] [Roll] itemList empty. Giving player a none item\n");
        return ITEM_NONE;
    }

    // Calculate total weight
    uint32_t totalWeight = 0;
    for (const ItemEntry& entry : itemList) {
        if (this->IsBlacklisted(entry.Id)) {
            continue;
        }
        totalWeight += entry.Weight;
    }

    // Avoid division by zero
    if (totalWeight == 0) {
        printf("[ItemTables] [Roll] No items to give out for this rank. Giving player a none item\n");
        return ITEM_NONE;
    }

    // Pick a random number from zero to (totalWeight - 1)
    uint32_t rand = random_int(totalWeight - 1);

    // Cumulative sum over weights
    uint32_t accumulated = 0;
    for (const ItemEntry& entry : itemList) {
        if (this->IsBlacklisted(entry.Id)) {
            continue;
        }
        accumulated += entry.Weight;
        if (rand < accumulated) {
            return static_cast<uint8_t>(entry.Id);
        }
    }

    // Fallback (should not happen)
    printf("[ItemTables] [Roll] Reached unreachable code\n");
    [[unlikely]] return ITEM_NONE;
}

void RandomItemTable::Blacklist(const std::string& resourceName) {
    const ItemInfo* info = gItemRegistry.GetInfo(resourceName);
    if (!info) {
        return;
    }

    mBlacklist.insert(info->Id);
}

bool RandomItemTable::IsBlacklisted(uint32_t itemId) const {
    //return mBlacklist.contains(itemId);
    return mBlacklist.find(itemId) != mBlacklist.end();
}

void RandomItemTable::ClearBlacklist() {
    mBlacklist.clear();
}

// MK64 Online FASE 1.4b: the oracle state hash, member by member (OracleCoverage.h).
void RandomItemTable::HashState(oracle::Hasher& h) const {
    using oracle::put;
    put(h, "mDistanceEnabled", mDistanceEnabled);
    {
        oracle::Hasher::Scope s(h, "mTable");
        put(h, "size", (uint64_t) mTable.size());
        for (size_t rank = 0; rank < mTable.size(); rank++) {
            oracle::Hasher::Scope r(h, rank);
            put(h, "size", (uint64_t) mTable[rank].size());
            for (size_t i = 0; i < mTable[rank].size(); i++) {
                oracle::Hasher::Scope e(h, i);
                put(h, "Id", mTable[rank][i].Id);
                put(h, "Weight", mTable[rank][i].Weight);
                put(h, "Distance", mTable[rank][i].Distance);
            }
        }
    }
    put(h, "mBlacklist", mBlacklist); // sorted (an unordered_set's order is not state)
}
