#include <core/AVLTree.hpp>

/* ====== Private Helper Methods ====== */

uint32_t AVLTree::getHight(const Node* node) {
    return node ? node->height: 0;
}

uint32_t AVLTree::getSubtreeSize(const Node* node) {
    return node ? node->subtreeSize: 0;
}

void AVLTree::updateNode(Node* node) {
    if (!node) return;
    node->height = 1 + std::max(getHight(node->left), getHight(node->right));
    node->subtreeSize = 1 + getSubtreeSize(node->left) + getSubtreeSize(node->right);
}

AVLTree::Node* AVLTree::rotateLeft(Node* old_root) {
    Node* new_root = old_root->right;
    Node* innerSubtree = new_root->left;

    new_root->parent = old_root->parent;
    old_root->parent = new_root;

    if (innerSubtree) {
        innerSubtree->parent = old_root;
    }

    new_root->left = old_root;
    old_root->right = innerSubtree;

    updateNode(old_root);
    updateNode(new_root);

    return new_root;
}

AVLTree::Node* AVLTree::rotateRight(Node* old_root) {
    Node* new_root = old_root->left;
    Node* innerSubtree = new_root->right;

    new_root->parent = old_root->parent;
    old_root->parent = new_root;

    if (innerSubtree) {
        innerSubtree->parent = old_root;
    }

    new_root->right = old_root;
    old_root->left = innerSubtree;

    updateNode(old_root);
    updateNode(new_root);

    return new_root;
}

AVLTree::Node* AVLTree::fixLeftImbalance(Node* node) {
    if (getHight(node->left->left) < getHight(node->left->right)) {
        node->left = rotateLeft(node->left);
    }
    return rotateRight(node);
}

AVLTree::Node* AVLTree::fixRightImbalance(Node* node) {
    if (getHight(node->right->left) < getHight(node->right->right)) {
        node->right = rotateRight(node->right);
    }

    return rotateLeft(node);
}

AVLTree::Node* AVLTree::balance(Node* node) {
    int balance_factor = getHight(node->left) - getHight(node->right);

    if (balance_factor > 1) {
        if (getHight(node->left->left) < getHight(node->left->right)) {
            node->left = fixLeftImbalance(node->left);
        }
        return rotateRight(node);
    }

    if (balance_factor < -1) {
        if (getHight(node->right->right) < getHight(node->right->left)) {
            node->right = fixRightImbalance(node->right);
        }
        return rotateLeft(node);
    }
    
    return node;
}

AVLTree::Node* AVLTree::removeNodeWithOneChild(Node* node) {
    assert(!node->left || !node->right);

    Node* child = node->left ? node->left : node->right;
    Node* parent = node->parent;

    if (child) {
        child->parent = parent;
    }

    if (parent) {
        if (parent->left == node) {
            parent->left = child;
        } else {
            parent->right = child;
        }

        return balance(parent);
    } else {
        return balance(child);
    }
}

void AVLTree::deleteTree(Node* root) {
    if (root) {
        deleteTree(root->left);
        deleteTree(root->right);
        delete root;
    }
}

/* ====== Public Methods ====== */

AVLTree::~AVLTree() {
    clear();
}

std::unique_ptr<AVLTree::Node> AVLTree::detach(Node* node) {
    if(!node->left || !node->right) {
        root = removeNodeWithOneChild(node);
    } else {
        Node* successor = node->right;
        while (successor->left) {
            successor = successor->left;
        }

        root = removeNodeWithOneChild(successor);

        std::swap(node->left, successor->left);
        std::swap(node->right, successor->right);
        std::swap(node->parent, successor->parent);
        std::swap(node->height, successor->height);
        std::swap(node->subtreeSize, successor->subtreeSize);

        if (successor->left) successor->left->parent = successor;
        if (successor->right) successor->right->parent = successor;

        Node* parent = node->parent;
        if (!parent) {
            root = successor;
        } else if (parent->left == node) {
            parent->left = successor;
        } else {
            parent->right = successor;
        }

        node->left = node->right = node->parent = nullptr;
        updateNode(node);
    }
    --node_count;
    return std::unique_ptr<Node>(node);
}

void AVLTree::insert(std::unique_ptr<Node> new_node, const std::function<int(Node*, Node*)>& compare) {
    if (!root) {
        root = new_node.release();
        node_count = 1;
        return;
    }

    Node* current = root;
    while (true) {
        if (compare(new_node.get(), current) < 0) {
            if (!current->left) {
                current->left = new_node.release();
                current->left->parent = current;
                break;
            }
            current = current->left;
        } else {
            if (!current->right) {
                current->right = new_node.release();
                current->right->parent = current;
                break;
            }
            current = current->right;
        }
    }

    ++node_count;

    Node* unbalanced = current;
    while (unbalanced) {
        updateNode(unbalanced);
        int balance_factor = getHight(unbalanced->left) - getHight(unbalanced->right);

        Node* parent = unbalanced->parent;
        Node** child_ptr = parent ? (parent->left == unbalanced ? &parent->left : &parent->right) : &root;

        if (balance_factor > 1) {
            *child_ptr = fixLeftImbalance(unbalanced);
        } else if (balance_factor < -1) {
            *child_ptr = fixRightImbalance(unbalanced);
        }
        unbalanced = parent;
    }
}

AVLTree::Node* AVLTree::findByRank(int32_t rank) {
    Node* current = root;

    while (current) {
        uint32_t left_size = getSubtreeSize(current->left);
        if (rank == (int32_t)left_size) {
            return current;
        } else if (rank < (int32_t)left_size) {
            current = current->left;
        } else {
            rank = rank - left_size - 1;
            current = current->right;
        }
    }

    return nullptr;
}

AVLTree::Node* AVLTree::find(Node* key, const std::function<int(Node*, Node*)>& compare) {
    Node* current = root;

    while (current) {
        int cmp = compare(key, current);
        if (cmp == 0) {
            return current;
        } else if (cmp < 0) {
            current = current->left;
        } else {
            current = current->right;
        }
    }

    return nullptr;
}

void AVLTree::clear() {
    deleteTree(root);
    root = nullptr;
    node_count = 0;
}