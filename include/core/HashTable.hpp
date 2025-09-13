#pragma once

#include <vector>
#include <memory>
#include <functional>
#include <cassert>

/**
 * @file HashTable.hpp
 * @brief Defines the HashTable class, a hash map implementation that uses
 * incremental rehashing to avoid long pauses during table resizing.
 */

/**
 * @class HashTable
 * @brief A hash table that supports insertion, lookup, and deletion.
 * @details This implementation uses two internal tables (`newerTable` and `olderTable`)
 * to perform incremental rehashing. When the load factor of the `newerTable`
 * exceeds a threshold, a new, larger table is created. Elements are then
 * gradually migrated from the `olderTable` to the `newerTable` with each
 * subsequent operation (insert, lookup, remove), ensuring that resizing
 * overhead is amortized over time.
 */
class HashTable {
public:
    /**
     * @struct Node
     * @brief The basic building block for elements stored in the hash table.
     * @details Users of the HashTable must extend this struct to include their own
     * data. The `hashCode` should be pre-computed before insertion for
     * efficiency. Nodes form a singly linked list within each hash slot
     * to handle collisions.
     */
    struct Node {
        virtual ~Node() = default;
        
        std::unique_ptr<Node> next = nullptr;
        uint64_t hashCode = 0;
    };

    /**
     * @brief Constructs an empty HashTable.
     */
    HashTable();

    /**
     * @brief Destroys the HashTable.
     * @details Memory is managed automatically by the `unique_ptr` members,
     * so the default destructor is sufficient.
     */
    ~HashTable();

    /**
     * @brief Searches for a node in the hash table.
     * @details This operation first contributes to any ongoing rehashing effort. It then
     * searches for the key in the `newerTable`. If not found, it proceeds
     * to search the `olderTable`.
     * @param key A raw pointer to a node-like object containing the hash code to search for.
     * @param equals A function that compares two nodes for equality. It is only called
     * if their hash codes match.
     * @return A raw pointer to the found node, or `nullptr` if the key is not present.
     */
    Node* lookup(Node* key, const std::function<bool(Node*, Node*)>& equals);

    /**
     * @brief Inserts a new node into the hash table.
     * @details The node is always inserted into the `newerTable`. If the insertion causes
     * the load factor to exceed a defined maximum and no rehashing is in
     * progress, a new rehashing cycle is initiated. Every insertion also
     * contributes to any ongoing rehashing work.
     * @param node A `unique_ptr` to the node to be inserted. The HashTable takes ownership.
     */
    void insert(std::unique_ptr<Node> node);

    /**
     * @brief Removes a node from the hash table.
     * @details This operation first contributes to any ongoing rehashing effort. It then
     * searches for and removes the key from either the `newerTable` or the
     * `olderTable`.
     * @param key A raw pointer to a node-like object containing the key to remove.
     * @param equals A function to compare nodes for equality.
     * @return A `unique_ptr` to the removed node if found, or `nullptr` otherwise.
     */
    std::unique_ptr<Node> remove(Node* key, const std::function<bool(Node*, Node*)>& equals);

    /**
     * @brief Returns the total number of elements in the hash table.
     * @return The combined element count from both the newer and older tables.
     */
    size_t size() const;

    /**
     * @brief Removes all elements from the hash table.
     * @details Resets both internal tables and stops any ongoing rehashing.
     */
    void clear();

    /**
     * @brief Applies a function to every node in the hash table.
     * @details This is used to implement commands like KEYS that need to iterate over all data.
     * @param callback The function to execute for each node.
     */
    void forEach(const std::function<void(Node*)>& callback);

    HashTable(const HashTable&) = delete;
    HashTable& operator=(const HashTable&) = delete;

    HashTable(HashTable&&) = default;
    HashTable& operator=(HashTable&&) = default;

private:
    /**
     * @struct Table
     * @brief Represents one of the two internal hash tables.
     * @details `slots` is a vector of unique pointers, each representing the head of a
     * linked list for collision handling. `mask` is used for efficient index
     * calculation (`hashCode & mask`) and requires the table size to be a
     * power of two. `elementCount` tracks the number of items in this table.
     */
    struct Table {
        std::vector<std::unique_ptr<Node>> slots;
        size_t mask = 0;
        size_t elementCount = 0;
    };

    /// @brief The primary table for new insertions and lookups. During rehashing, this is the destination table.
    Table newerTable;
    /// @brief The secondary table that holds old data during rehashing. It is read-only for lookups and removals.
    Table olderTable;

    /// @brief Tracks the current slot index being migrated from `olderTable` during rehashing.
    size_t migrateIndex = 0;

    /**
     * @brief Initializes a Table with a specified size.
     * @param table The `Table` struct to initialize.
     * @param size The number of slots for the table.
     * @note The size must be a power of two to ensure the bitwise mask works correctly for index calculation.
     */
    void initializeTable(Table& table, size_t size);

    /**
     * @brief A helper function to insert a node into a specific table.
     * @param table The table to insert the node into.
     * @param node The node to insert. The function takes ownership.
     */
    void insertIntoTable(Table& table, std::unique_ptr<Node> node);

    /**
     * @brief Finds the owning `unique_ptr` of a node that matches a given key.
     * @details This is a crucial helper for modification operations (like `remove`).
     * Instead of returning the node itself, it returns a pointer to the
     * `unique_ptr` that owns it, allowing the caller to modify the linked
     * list (e.g., by detaching the node).
     * @param table The table to search within.
     * @param key The key to look for.
     * @param equals The equality comparison function.
     * @return A raw pointer to the owning `unique_ptr` of the matching node, or `nullptr` if not found.
     */
    std::unique_ptr<Node>* findNodePtr(Table& table, Node* key, const std::function<bool(Node*, Node*)>& equals);

    /**
     * @brief Unlinks and returns a node from a collision chain.
     * @param table The table from which to detach the node.
     * @param nodeOwnerPtr A pointer to the `unique_ptr` that owns the node to be detached.
     * This is typically obtained from `findNodePtr`.
     * @return A `unique_ptr` containing the detached node.
     */
    std::unique_ptr<Node> detachNode(Table& table, std::unique_ptr<Node>* nodeOwnerPtr);

    /**
     * @brief Begins the incremental rehashing process.
     * @details The current `newerTable` becomes the `olderTable`, and a new `newerTable`
     * is created with double the capacity. The migration index is reset.
     */
    void startRehashing();

    /**
     * @brief Performs a small, fixed amount of rehashing work.
     * @details Moves a limited number of nodes from one slot in the `olderTable` to the
     * `newerTable`. This method is called by public-facing operations to
     * distribute the cost of rehashing over time. Once all elements are
     * migrated, the `olderTable` is cleared.
     */
    void helpRehashing();

    void forEachInTable(Table& table, const std::function<void(Node*)>& callback);
};