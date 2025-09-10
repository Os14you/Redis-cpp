#pragma once

#include "AVLTree.hpp"
#include "HashTable.hpp"
#include <string>

struct ZSetNode: public AVLTree::Node {
    double score;
    std::string member;
};

struct ZSetMemberNode : public HashTable::Node {
    std::string member;
    double score;
};

struct SortedSet {
    HashTable member_to_score_map;
    AVLTree score_sorted_tree;
};