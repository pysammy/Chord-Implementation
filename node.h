#ifndef NODE_H
#define NODE_H

#include <stdint.h>
#include <map>
#include <set>
#include <vector>
#include <iostream>

#define BITLENGTH 8
#define NONE_VALUE 0  // Use 0 as sentinel value for "None"

// Forward declaration
class Node;

// The FingerTable class with compatibility functions for both interfaces
class FingerTable {
public:
    /**
     * @param nodeId: the id of node hosting the finger table.
     */
    FingerTable(uint8_t nodeId): nodeId_(nodeId) {
        fingerTable_.resize(BITLENGTH + 1);
    }
    
    void set(size_t index, Node* successor) {
        fingerTable_[index] = successor;
    }
    
    
    uint8_t get(size_t index);
    
    // Internal method for implementation that returns Node pointer
    Node* getNodePtr(size_t index) {
        return fingerTable_[index];
    }
    
    void prettyPrint();
    
private:
    uint8_t nodeId_;
    std::vector<Node*> fingerTable_;
};

class Node {
public:
    Node(uint8_t id);

    void join(Node* node);
    uint8_t find(uint8_t key);
    void insert(uint8_t key);
    void remove(uint8_t key);
    
    // Additional methods needed for implementation
    void insert(uint8_t key, uint8_t value);  // Overloaded version
    void leave();  // Optional method
    void stabilize();
    void fixFingers();
    void spaceShuffleOptimization();
    
    // Getters and setters needed for implementation
    uint8_t getId() const {
        return static_cast<uint8_t>(id_);
    }
    
    Node* getPredecessor() const {
        return predecessor_;
    }
    
    void setPredecessor(Node* pred) {
        predecessor_ = pred;
    }
    
    FingerTable& getFingerTable() {
        return fingerTable_;
    }
    
    const std::map<uint8_t, uint8_t>& getLocalKeys() const {
        return localKeys_;
    }
    
    // Debug helper
    void printPredecessorChain();
    
private:
    uint64_t id_;                     
    FingerTable fingerTable_;
    std::map<uint8_t, uint8_t> localKeys_;  
    
    // Additional members for implementation
    Node* predecessor_;
    int next_finger_;
    
    // Helper methods
    Node* findSuccessor(uint8_t id);
    Node* findPredecessor(uint8_t id);
    Node* closestPrecedingFinger(uint8_t id);
    bool inRange(uint8_t id, uint8_t start, uint8_t end) const;
    void updateOthers();
    void updateFingerTable(Node* s, int i);
    void moveKeys(Node* successor);
    void notify(Node* n);
    bool isResponsibleForKey(uint8_t key) const;
    void transferKey(uint8_t key, Node* toNode);
    void checkAllNodesForKeys();
    double computeVariance(const std::vector<int>& keyDistribution);
};


inline uint8_t FingerTable::get(size_t index) {
    if (fingerTable_[index] == nullptr) return 0;
    return fingerTable_[index]->getId();
}

#endif