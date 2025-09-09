#include <core/HashTable.hpp>

/**
 * @file HashTable.cpp
 * @brief Implements the HashTable class, providing functionality for a hash map
 * with incremental rehashing.
 */

/// @brief Defines the maximum number of nodes to migrate in a single `helpRehashing` call.
const size_t REHASHING_WORK_LIMIT = 128;
/// @brief The load factor threshold that triggers a new rehashing cycle.
const size_t MAX_LOAD_FACTOR = 8;

/* ====== Private methods ====== */

void HashTable::initializeTable(Table& table, size_t size) {
    // Assert that size is a power of 2. This is a common bit-twiddling hack
    // where (n > 0) && ((n & (n - 1)) == 0).
    assert(size > 0 && ((size - 1) & size) == 0);
    table.slots.resize(size);
    table.mask = size - 1;
    table.elementCount = 0;
}

void HashTable::insertIntoTable(Table& table, std::unique_ptr<Node> node) {
    size_t pos = node->hashCode & table.mask;
    node->next = std::move(table.slots[pos]);
    table.slots[pos] = std::move(node);
    table.elementCount++;
}

std::unique_ptr<HashTable::Node>* HashTable::findNodePtr(Table& table, Node* key, const std::function<bool(Node*, Node*)>& equals) {
    if (table.slots.empty()) {
        return nullptr;
    }

    size_t pos = key->hashCode & table.mask;
    std::unique_ptr<Node>* currentOwnerPtr = &table.slots[pos];

    // Traverse the linked list (collision chain).
    while (*currentOwnerPtr) {
        // Fast path: check hash codes first.
        // Slow path: if hashes match, use the provided equality function.
        if ((*currentOwnerPtr)->hashCode == key->hashCode && equals(currentOwnerPtr->get(), key)) {
            return currentOwnerPtr;
        }
        currentOwnerPtr = &(*currentOwnerPtr)->next;
    }

    return nullptr;
}

std::unique_ptr<HashTable::Node> HashTable::detachNode(Table& table, std::unique_ptr<Node>* nodeOwnerPtr) {
    std::unique_ptr<Node> targetNode = std::move(*nodeOwnerPtr);
    *nodeOwnerPtr = std::move(targetNode->next);
    --table.elementCount;
    return targetNode;
}


void HashTable::startRehashing() {
    assert(olderTable.slots.empty());
    olderTable = std::move(newerTable);
    initializeTable(newerTable, (olderTable.mask + 1) * 2);
    migrateIndex = 0;
}

void HashTable::helpRehashing() {
    if (olderTable.slots.empty()) {
        return;
    }

    size_t workDone = 0;
    while (workDone < REHASHING_WORK_LIMIT && olderTable.elementCount > 0) {
        // Find a non-empty slot to migrate from
        while (migrateIndex < olderTable.slots.size() && !olderTable.slots[migrateIndex]) {
            migrateIndex++;
        }

        if (migrateIndex >= olderTable.slots.size()) {
            // This should ideally not be hit if elementCount is tracked correctly.
            // It indicates we've scanned all slots but elements remain.
            // For robustness, we can break or reset the migration.
            break;
        }

        std::unique_ptr<Node>* slotOwnerPtr = &olderTable.slots[migrateIndex];
        // Move the node from the older table to the newer one.
        insertIntoTable(newerTable, detachNode(olderTable, slotOwnerPtr));
        workDone++;
    }

    // If migration is complete, clear the old table.
    if (olderTable.elementCount == 0) {
        olderTable.slots.clear();
        olderTable.mask = 0;
    }
}

void HashTable::forEachInTable(Table& table, const std::function<void(Node*)>& callback) {
    for(auto &slot: table.slots) {
        for(Node* current = slot.get(); current; current = current->next.get()) {
            callback(current);
        }
    }
}

/* ====== Public methods ====== */

HashTable::HashTable() = default;
HashTable::~HashTable() = default; // unique_ptr handles memory cleanup

HashTable::Node* HashTable::lookup(Node* key, const std::function<bool(Node*, Node*)>& equals) {
    helpRehashing();
    
    if (auto* nodeOwnerPtr = findNodePtr(newerTable, key, equals)) {
        return nodeOwnerPtr->get();
    }
    
    if (auto* nodeOwnerPtr = findNodePtr(olderTable, key, equals)) {
        return nodeOwnerPtr->get();
    }

    return nullptr;
}

void HashTable::insert(std::unique_ptr<Node> node) {
    if (newerTable.slots.empty()) {
        initializeTable(newerTable, 4);
    }
    insertIntoTable(newerTable, std::move(node));

    // Trigger rehashing if the load factor is exceeded and we are not already rehashing.
    if (olderTable.slots.empty()) {
        size_t threshold = (newerTable.mask + 1) * MAX_LOAD_FACTOR;
        if (newerTable.elementCount >= threshold) {
            startRehashing();
        }
    }
    
    // Always contribute to any ongoing rehashing effort.
    helpRehashing();
}

std::unique_ptr<HashTable::Node> HashTable::remove(Node* key, const std::function<bool(Node*, Node*)>& equals) {
    helpRehashing();

    if (auto* nodeOwnerPtr = findNodePtr(newerTable, key, equals)) {
        return detachNode(newerTable, nodeOwnerPtr);
    }
    
    if (auto* nodeOwnerPtr = findNodePtr(olderTable, key, equals)) {
        return detachNode(olderTable, nodeOwnerPtr);
    }

    return nullptr;
}

size_t HashTable::size() const {
    return newerTable.elementCount + olderTable.elementCount;
}

void HashTable::forEach(const std::function<void(Node*)>& callback) {
    forEachInTable(newerTable, callback);
    forEachInTable(olderTable, callback);
}

void HashTable::clear() {
    newerTable = {};
    olderTable = {};
    migrateIndex = 0;
}