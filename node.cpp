#include "node.h"
#include <iostream>
#include <cmath>
#include <random>
#include <algorithm>
#include <numeric>

// Constructor
Node::Node(uint8_t id) 
    : id_(id), 
      fingerTable_(id), 
      predecessor_(nullptr), 
      next_finger_(1) {
}

// Print the finger table in a nice format
void FingerTable::prettyPrint() {
    std::cout << "----------Node id:" << static_cast<int>(nodeId_) << "----------" << std::endl;
    std::cout << "Successor: " << static_cast<int>(getNodePtr(1)->getId()) << std::endl;
    
    std::cout << "FingerTables:" << std::endl;
    for (size_t i = 1; i <= BITLENGTH; i++) {
        uint8_t start = (nodeId_ + (1 << (i - 1))) % (1 << BITLENGTH);
        uint8_t end = (nodeId_ + (1 << i)) % (1 << BITLENGTH);
        std::cout << "| k = " << i << " [" << static_cast<int>(start) << " , " 
                  << static_cast<int>(end) << ") \tsucc. = " 
                  << static_cast<int>(getNodePtr(i)->getId()) << " |" << std::endl;
    }
    std::cout << "-----------------------------" << std::endl;
}

// Helper function to check if id is in the range (start, end]
bool Node::inRange(uint8_t id, uint8_t start, uint8_t end) const {
    if (start == end) {
        return true;
    } else if (start < end) {
        return (id > start && id <= end);
    } else { // start > end, wrapping around the circle
        return (id > start || id <= end);
    }
}

// Find the closest preceding finger node for id
Node* Node::closestPrecedingFinger(uint8_t id) {
    for (int i = BITLENGTH; i >= 1; i--) {
        if (inRange(fingerTable_.getNodePtr(i)->getId(), id_, id)) {
            return fingerTable_.getNodePtr(i);
        }
    }
    return this;
}

// Find the predecessor node of id
Node* Node::findPredecessor(uint8_t id) {
    Node* n = this;
    while (!inRange(id, n->id_, n->fingerTable_.getNodePtr(1)->getId())) {
        n = n->closestPrecedingFinger(id);
        
        // Prevent infinite loop if the network is not properly formed
        if (n == this) {
            break;
        }
    }
    return n;
}

// Find the successor node of id
Node* Node::findSuccessor(uint8_t id) {
    // If this is the only node in the network, it's responsible for all keys
    if (fingerTable_.getNodePtr(1) == this) {
        return this;
    }
    
    // If id is in range (n, successor], then successor is responsible for id
    if (inRange(id, id_, fingerTable_.getNodePtr(1)->getId())) {
        return fingerTable_.getNodePtr(1);
    }
    
    // Otherwise find the predecessor and return its successor
    Node* predecessor = findPredecessor(id);
    return predecessor->fingerTable_.getNodePtr(1);
}

// Notify method - called by a node thinking it might be our predecessor
void Node::notify(Node* n) {
    // If predecessor is null or n is in (predecessor, this)
    if (predecessor_ == nullptr || inRange(n->getId(), predecessor_->getId(), id_)) {
        predecessor_ = n;
    }
}

// Stabilize the ring by verifying immediate successor and notifying it
void Node::stabilize() {
    Node* successor = fingerTable_.getNodePtr(1);
    Node* x = successor->getPredecessor();
    
    if (x != nullptr && inRange(x->getId(), id_, successor->getId())) {
        fingerTable_.set(1, x);
        successor = x;
    }
    
    successor->notify(this);
}

// Fix finger table entries
void Node::fixFingers() {
    next_finger_ = next_finger_ + 1;
    if (next_finger_ > BITLENGTH) {
        next_finger_ = 1;
    }
    
    uint8_t start = (id_ + (1 << (next_finger_ - 1))) % (1 << BITLENGTH);
    Node* nextSuccessor = findSuccessor(start);
    
    // Only update if different to avoid unnecessary network traffic
    if (fingerTable_.getNodePtr(next_finger_) != nextSuccessor) {
        fingerTable_.set(next_finger_, nextSuccessor);
    }
}

// Check if this node is responsible for a key based on Chord's rules
bool Node::isResponsibleForKey(uint8_t key) const {
    // If we're the only node in the network
    if (predecessor_ == this) {
        return true;
    }
    
    // Normal case: key is in (predecessor, this]
    return inRange(key, predecessor_->getId(), id_);
}

// Transfer a key to another node
void Node::transferKey(uint8_t key, Node* toNode) {
    if (localKeys_.find(key) != localKeys_.end()) {
        // Transfer the key and value
        uint8_t value = localKeys_[key];
        toNode->localKeys_[key] = value;
        
        // Log the transfer
        std::cout << "Migrate key " << static_cast<int>(key)
                  << " from node " << static_cast<int>(id_)
                  << " to node " << static_cast<int>(toNode->getId()) << std::endl;
        
        // Remove from this node
        localKeys_.erase(key);
    }
}

// Implementation of the join function
void Node::join(Node* node) {
    if (node == nullptr) {
        // This is the first node in the network
        for (int i = 1; i <= BITLENGTH; i++) {
            fingerTable_.set(i, this);
        }
        predecessor_ = this;
        std::cout << "Node " << static_cast<int>(id_) << " is the first node to join the Chord network." << std::endl;
    } else {
        // Initialize finger table
        fingerTable_.set(1, node->findSuccessor(id_));
        
        std::cout << "Node " << static_cast<int>(id_) << " joined with successor " 
                  << static_cast<int>(fingerTable_.getNodePtr(1)->getId()) << std::endl;
        
        // Initialize finger table entries
        for (int i = 1; i < BITLENGTH; i++) {
            uint8_t start = (id_ + (1 << i)) % (1 << BITLENGTH);
            
            // Check if finger i+1 is in the same interval as finger i
            if (inRange(start, id_, fingerTable_.getNodePtr(i)->getId())) {
                fingerTable_.set(i + 1, fingerTable_.getNodePtr(i));
            } else {
                fingerTable_.set(i + 1, node->findSuccessor(start));
            }
        }
        
        // Update predecessor of successor
        predecessor_ = fingerTable_.getNodePtr(1)->getPredecessor();
        fingerTable_.getNodePtr(1)->setPredecessor(this);
        
        // Update other nodes' finger tables
        updateOthers();
        
        // Move keys from successor
        moveKeys(fingerTable_.getNodePtr(1));
        
        // Check all nodes for keys that belong to this node
        checkAllNodesForKeys();
        
        // Print the finger table
        fingerTable_.prettyPrint();
    }
}

// Leave the Chord network
void Node::leave() {
    std::cout << "Node " << static_cast<int>(id_) << " is leaving the network." << std::endl;
    
    if (predecessor_ == this && fingerTable_.getNodePtr(1) == this) {
        // This is the only node in the network
        std::cout << "Node " << static_cast<int>(id_) << " was the only node in the network." << std::endl;
        return;
    }
    
    // Move keys to successor
    Node* successor = fingerTable_.getNodePtr(1);
    
    for (const auto& pair : localKeys_) {
        successor->localKeys_[pair.first] = pair.second;
        std::cout << "Migrate key " << static_cast<int>(pair.first)
                  << " from node " << static_cast<int>(id_)
                  << " to node " << static_cast<int>(successor->getId()) << std::endl;
    }
    
    // Clear local keys
    localKeys_.clear();
    
    // Update predecessor of successor
    successor->setPredecessor(predecessor_);
    
    // Update finger tables of other nodes
    for (int i = 1; i <= BITLENGTH; i++) {
        uint8_t p_id = (id_ - (1 << (i - 1)) + (1 << BITLENGTH)) % (1 << BITLENGTH);
        Node* p = findPredecessor(p_id);
        
        if (p != this && p->fingerTable_.getNodePtr(i) == this) {
            p->fingerTable_.set(i, successor);
        }
    }
    
    // Notify predecessor about the change
    if (predecessor_ != this) {
        predecessor_->fingerTable_.set(1, successor);
        predecessor_->fixFingers();
    }
    
    std::cout << "Node " << static_cast<int>(id_) << " has left the network." << std::endl;
    
    // Print updated finger tables of affected nodes
    if (predecessor_ != this) {
        std::cout << "Updated finger table of predecessor:" << std::endl;
        // Display correct predecessor
        std::cout << "Node id:" << static_cast<int>(predecessor_->getId()) 
                  << " Predecessor: " << static_cast<int>(predecessor_->getPredecessor()->getId()) << std::endl;
        predecessor_->fingerTable_.prettyPrint();
    }
    
    std::cout << "Updated finger table of successor:" << std::endl;
    // Display correct predecessor
    std::cout << "Node id:" << static_cast<int>(successor->getId()) 
              << " Predecessor: " << static_cast<int>(successor->getPredecessor()->getId()) << std::endl;
    successor->fingerTable_.prettyPrint();
}

// Check all nodes in the network for keys that should belong to this node
void Node::checkAllNodesForKeys() {
    if (predecessor_ == nullptr) return;
    
    // Start from our successor and go around the ring
    Node* current = fingerTable_.getNodePtr(1);
    std::set<Node*> visited;
    visited.insert(this); // Don't check ourselves
    
    while (current != this && visited.find(current) == visited.end()) {
        visited.insert(current);
        
        // Check if any keys in this node should belong to us
        std::vector<uint8_t> keysToMove;
        
        for (const auto& pair : current->localKeys_) {
            uint8_t key = pair.first;
            if (isResponsibleForKey(key)) {
                keysToMove.push_back(key);
            }
        }
        
        // Move the identified keys
        for (uint8_t key : keysToMove) {
            current->transferKey(key, this);
        }
        
        // Move to the next node
        current = current->fingerTable_.getNodePtr(1);
    }
}

// Update all nodes that should have this node in their finger tables
void Node::updateOthers() {
    for (int i = 1; i <= BITLENGTH; i++) {
        // Find the last node p whose i-th finger might be this node
        uint8_t p_id = (id_ - (1 << (i - 1)) + (1 << BITLENGTH)) % (1 << BITLENGTH);
        Node* p = findPredecessor(p_id);
        
        // Skip if p is this node
        if (p != this) {
            // Update p's finger table with this node
            p->updateFingerTable(this, i);
        }
    }
}

// Update finger table with s at position i
void Node::updateFingerTable(Node* s, int i) {
    // Check if s should be the i-th finger
    if (fingerTable_.getNodePtr(i) == nullptr || 
        inRange(s->getId(), id_, fingerTable_.getNodePtr(i)->getId())) {
        
        fingerTable_.set(i, s);
        
        // Propagate to predecessor if needed
        if (predecessor_ != nullptr && predecessor_ != this && predecessor_ != s) {
            predecessor_->updateFingerTable(s, i);
        }
    }
}

// Move keys from successor to this node
void Node::moveKeys(Node* successor) {
    // Find keys that should be moved to this node
    std::vector<uint8_t> keysToMove;
    
    for (const auto& pair : successor->localKeys_) {
        uint8_t key = pair.first;
        // Check if this node is responsible for the key
        if (isResponsibleForKey(key)) {
            keysToMove.push_back(key);
        }
    }
    
    // Move the keys
    for (uint8_t key : keysToMove) {
        successor->transferKey(key, this);
    }
}

// Find the value associated with key (API compatible version)
uint8_t Node::find(uint8_t key) {
    std::cout << "Look-up result of key " << static_cast<int>(key) 
              << " from node " << static_cast<int>(id_) << " with path [";
    
    // Local search first
    if (localKeys_.find(key) != localKeys_.end()) {
        std::cout << static_cast<int>(id_) << "] value is ";
        uint8_t value = localKeys_[key];
        if (value == NONE_VALUE) {
            std::cout << "None" << std::endl;
            return NONE_VALUE;
        } else {
            std::cout << static_cast<int>(value) << std::endl;
            return value;
        }
    }
    
    // Forward search through the Chord ring
    std::vector<uint8_t> path;
    path.push_back(id_);
    
    Node* current = this;
    Node* responsibleNode = nullptr;
    
    while (true) {
        Node* next = current->closestPrecedingFinger(key);
        
        // If we can't make progress, find the successor
        if (next == current) {
            responsibleNode = current->fingerTable_.getNodePtr(1);
            path.push_back(responsibleNode->getId());
            break;
        }
        
        // If we've found the predecessor, get its successor
        if (inRange(key, next->getId(), next->fingerTable_.getNodePtr(1)->getId())) {
            responsibleNode = next->fingerTable_.getNodePtr(1);
            path.push_back(next->getId());
            path.push_back(responsibleNode->getId());
            break;
        }
        
        // Continue with the next node
        current = next;
        path.push_back(current->getId());
        
        // Check for loop
        if (path.size() > (1 << BITLENGTH)) {
            std::cout << "Loop detected in lookup!" << std::endl;
            break;
        }
    }
    
    // Print the path
    for (size_t i = 0; i < path.size(); i++) {
        std::cout << static_cast<int>(path[i]);
        if (i < path.size() - 1) {
            std::cout << ",";
        }
    }
    
    std::cout << "] value is ";
    
    // Check if the responsible node has the key
    if (responsibleNode && responsibleNode->localKeys_.find(key) != responsibleNode->localKeys_.end()) {
        uint8_t value = responsibleNode->localKeys_[key];
        if (value == NONE_VALUE) {
            std::cout << "None" << std::endl;
            return NONE_VALUE;
        } else {
            std::cout << static_cast<int>(value) << std::endl;
            return value;
        }
    } else {
        std::cout << "None" << std::endl;
        return NONE_VALUE;
    }
}

// Insert a key-value pair (API compatible version)
void Node::insert(uint8_t key, uint8_t value) {
    // Find the node responsible for the key
    Node* responsibleNode = findSuccessor(key);
    
    // Insert the key-value pair
    responsibleNode->localKeys_[key] = value;
    
    std::cout << "Key " << static_cast<int>(key) << " with value ";
    if (value == NONE_VALUE) {
        std::cout << "None";
    } else {
        std::cout << static_cast<int>(value);
    }
    std::cout << " inserted at node " 
              << static_cast<int>(responsibleNode->getId()) << std::endl;
}

// Overloaded insert method that uses None as the value
void Node::insert(uint8_t key) {
    // Call the main insert method with NONE_VALUE
    insert(key, NONE_VALUE);
}

// Remove a key
void Node::remove(uint8_t key) {
    // Find the node responsible for the key
    Node* responsibleNode = findSuccessor(key);
    
    // Remove the key if it exists
    if (responsibleNode->localKeys_.find(key) != responsibleNode->localKeys_.end()) {
        responsibleNode->localKeys_.erase(key);
        std::cout << "Key " << static_cast<int>(key) << " removed from node " 
                  << static_cast<int>(responsibleNode->getId()) << std::endl;
    } else {
        std::cout << "Key " << static_cast<int>(key) << " not found" << std::endl;
    }
}

// Helper function to compute variance of key distribution
double Node::computeVariance(const std::vector<int>& keyDistribution) {
    double mean = std::accumulate(keyDistribution.begin(), keyDistribution.end(), 0.0) / keyDistribution.size();
    double variance = 0.0;
    
    for (int count : keyDistribution) {
        variance += std::pow(count - mean, 2);
    }
    
    return variance / keyDistribution.size();
}

// Space Shuffle Optimization
void Node::spaceShuffleOptimization() {
    std::cout << "Performing Space Shuffle optimization for node " << static_cast<int>(id_) << std::endl;
    
    // 1. Gather information about key distribution
    std::vector<Node*> allNodes;
    std::map<Node*, int> keyDistribution;
    
    // Collect all nodes in the network
    Node* current = this;
    do {
        allNodes.push_back(current);
        current = current->fingerTable_.getNodePtr(1); // Move to successor
    } while (current != this);
    
    // Count keys per node
    for (Node* node : allNodes) {
        keyDistribution[node] = node->localKeys_.size();
    }
    
    // 2. Compute variance before optimization
    std::vector<int> keyCountsBeforeOpt;
    for (const auto& pair : keyDistribution) {
        keyCountsBeforeOpt.push_back(pair.second);
    }
    
    double varianceBefore = computeVariance(keyCountsBeforeOpt);
    std::cout << "Variance before optimization: " << varianceBefore << std::endl;
    
    // 3. Identify heavily loaded and lightly loaded nodes
    double mean = std::accumulate(keyCountsBeforeOpt.begin(), keyCountsBeforeOpt.end(), 0.0) / keyCountsBeforeOpt.size();
    
    std::vector<Node*> heavyNodes;
    std::vector<Node*> lightNodes;
    
    for (const auto& pair : keyDistribution) {
        if (pair.second > 1.2 * mean) {
            heavyNodes.push_back(pair.first);
        } else if (pair.second < 0.8 * mean) {
            lightNodes.push_back(pair.first);
        }
    }
    
    // 4. Perform space shuffle
    if (!heavyNodes.empty() && !lightNodes.empty()) {
        std::random_device rd;
        std::mt19937 g(rd());
        
        // Shuffle lists for randomness
        std::shuffle(heavyNodes.begin(), heavyNodes.end(), g);
        std::shuffle(lightNodes.begin(), lightNodes.end(), g);
        
        std::cout << "Starting Space Shuffle transfers:" << std::endl;
        
        // Transfer keys from heavy to light nodes
        for (size_t i = 0; i < std::min(heavyNodes.size(), lightNodes.size()); i++) {
            Node* heavyNode = heavyNodes[i];
            Node* lightNode = lightNodes[i];
            
            int keysToTransfer = static_cast<int>((keyDistribution[heavyNode] - keyDistribution[lightNode]) / 2);
            
            if (keysToTransfer > 0) {
                int transferred = 0;
                
                for (auto it = heavyNode->localKeys_.begin(); it != heavyNode->localKeys_.end() && transferred < keysToTransfer;) {
                    // Transfer key
                    lightNode->localKeys_[it->first] = it->second;
                    
                    std::cout << "Space Shuffle: Migrated key " << static_cast<int>(it->first) << " with value ";
                    if (it->second == NONE_VALUE) {
                        std::cout << "None";
                    } else {
                        std::cout << static_cast<int>(it->second);
                    }
                    std::cout << " from node " << static_cast<int>(heavyNode->getId())
                              << " to node " << static_cast<int>(lightNode->getId()) << std::endl;
                    
                    // Erase from heavy node
                    auto toErase = it;
                    ++it;
                    heavyNode->localKeys_.erase(toErase);
                    
                    transferred++;
                }
                
                // Update key distribution
                keyDistribution[heavyNode] -= transferred;
                keyDistribution[lightNode] += transferred;
            }
        }
    }
    
    // 5. Compute variance after optimization
    std::vector<int> keyCountsAfterOpt;
    for (const auto& pair : keyDistribution) {
        keyCountsAfterOpt.push_back(pair.second);
    }
    
    double varianceAfter = computeVariance(keyCountsAfterOpt);
    std::cout << "Variance after optimization: " << varianceAfter << std::endl;
    
    double improvementPercent = ((varianceBefore - varianceAfter) / varianceBefore) * 100.0;
    std::cout << "Improvement: " << improvementPercent << "%" << std::endl;
}

void Node::printPredecessorChain() {
    std::cout << "Predecessor chain starting from Node " << static_cast<int>(id_) << ": ";
    Node* current = this;
    for (int i = 0; i < 10; i++) { // Limit to 10 hops to avoid infinite loops
        std::cout << static_cast<int>(current->getId()) << " <- ";
        current = current->getPredecessor();
        if (current == this || current == nullptr) break;
    }
    std::cout << "..." << std::endl;
}