#pragma once 

#include <cstdint>
#include <memory>
#include <cassert>

class AVLTree {
public:
    struct Node {
        Node* parent = nullptr;
        Node* left = nullptr;
        Node* right = nullptr;

        uint32_t height = 1;
        uint32_t subtreeSize = 1;

        virtual ~Node() = default;
    };

    ~AVLTree();

    std::unique_ptr<Node> detach(Node* node);

    void clear();

private:
    Node* root = nullptr;

    static uint32_t getHight(const Node* node);
    static uint32_t getSubtreeSize(const Node* node);

    static void updateNode(Node* node);
    static Node* rotateLeft(Node* old_root);
    static Node* rotateRight(Node* old_root);

    static Node* fixLeftImbalance(Node* node);
    static Node* fixRightImbalance(Node* node);
    static Node* balance(Node* node);

    static Node* removeNodeWithOneChild(Node* node);
    void deleteTree(Node* root);
};