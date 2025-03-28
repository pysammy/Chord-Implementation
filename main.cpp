#include "node.h"
#include <iostream>
#include <vector>

// Helper function to print the keys stored at each node
void printKeysDistribution(const std::vector<Node*>& nodes) {
    std::cout << "\n************* Keys Distribution *************" << std::endl;
    for (Node* node : nodes) {
        std::cout << "--------------Node id:" << static_cast<int>(node->getId()) << "------------" << std::endl;
        std::cout << "{";
        bool first = true;
        for (const auto& pair : node->getLocalKeys()) {
            if (!first) {
                std::cout << ", ";
            }
            std::cout << static_cast<int>(pair.first) << ": ";
            if (pair.second == NONE_VALUE) {
                std::cout << "None";
            } else {
                std::cout << static_cast<int>(pair.second);
            }
            first = false;
        }
        std::cout << "}" << std::endl;
    }
    std::cout << "********************************************" << std::endl;
}

int main() {
    // SECTION 1: Add nodes to the network using the join function (m = 8)
    std::cout << "1. Add nodes to the network using the join function, m = 8\n" << std::endl;
    
    // Create nodes with specified IDs as in test document
    std::vector<Node*> nodes;
    nodes.push_back(new Node(0));    // n0
    nodes.push_back(new Node(30));   // n1
    nodes.push_back(new Node(65));   // n2
    nodes.push_back(new Node(110));  // n3
    nodes.push_back(new Node(160));  // n4
    nodes.push_back(new Node(230));  // n5
    
    // First node joins (creates) the ring
    nodes[0]->join(nullptr);   
    
    // Join other nodes one by one
    for (size_t i = 1; i < nodes.size(); i++) {
        nodes[i]->join(nodes[i-1]);
    }
    
    // Test if predecessors are functionally correct
    std::cout << "\n-------- Debugging Predecessor Information --------" << std::endl;
    for (Node* node : nodes) {
        std::cout << "Node " << static_cast<int>(node->getId()) 
                << " thinks its predecessor is " << static_cast<int>(node->getPredecessor()->getId()) 
                << " and would be responsible for keys in range: (" 
                << static_cast<int>(node->getPredecessor()->getId()) << ", " 
                << static_cast<int>(node->getId()) << "]" << std::endl;
        
        // Print the full predecessor chain
        node->printPredecessorChain();
    }
    std::cout << "-------- End Predecessor Debug Info --------\n" << std::endl;
    
    std::cout << "\nRunning stabilization to establish correct predecessor relationships..." << std::endl;
    for (int round = 0; round < 10; round++) {
        for (Node* node : nodes) {
            node->stabilize();
        }
    }
    
    // SECTION 2: Print finger tables of all nodes
    std::cout << "\n2. Print finger table of all nodes (40pts)\n" << std::endl;
    for (Node* node : nodes) {
        // Display correct predecessor before printing finger table
        std::cout << "Node id:" << static_cast<int>(node->getId()) 
                  << " Predecessor: " << static_cast<int>(node->getPredecessor()->getId()) << std::endl;
        node->getFingerTable().prettyPrint();
    }
    
    // SECTION 3: Insert keys and add new node joins
    std::cout << "\n3. Insert keys and add new node joins (20pts)\n" << std::endl;
    nodes[0]->insert(3, 3);
    nodes[1]->insert(200);      // Uses NONE_VALUE as value
    nodes[2]->insert(123);      // Uses NONE_VALUE as value
    nodes[3]->insert(45, 3);
    nodes[4]->insert(99);       // Uses NONE_VALUE as value
    nodes[2]->insert(60, 10);
    nodes[0]->insert(50, 8);
    nodes[3]->insert(100, 5);
    nodes[3]->insert(101, 4);
    nodes[3]->insert(102, 6);
    nodes[5]->insert(240, 8);
    nodes[5]->insert(250, 10);
    
    // SECTION 3.1: Print keys that stored in each node
    std::cout << "\n3.1 print keys that stored in each node (10pts)\n" << std::endl;
    printKeysDistribution(nodes);
    
    // SECTION 3.2: Node 100 joins
    std::cout << "\nn6 (id: 100) joins\n" << std::endl;
    Node* newNode = new Node(100);
    nodes.push_back(newNode);
    newNode->join(nodes[3]);  // Join using node 110
    
    // Print finger tables after adding the new node
    std::cout << "\nFig.4 An updated circle after n6 joins" << std::endl;
    for (Node* node : nodes) {
        // Display correct predecessor before printing finger table
        std::cout << "Node id:" << static_cast<int>(node->getId()) 
                  << " Predecessor: " << static_cast<int>(node->getPredecessor()->getId()) << std::endl;
        node->getFingerTable().prettyPrint();
    }
    
    // SECTION 3.2: Print migrated keys
    std::cout << "\n3.2 Print migrated keys (10pts)" << std::endl;
    printKeysDistribution(nodes);
    
    // SECTION 4: Lookup keys
    std::cout << "\n4. Lookup keys (40pts)" << std::endl;
    std::cout << "Print lookup results and sequences of nodes get involved in this procedure (run lookup on node n0, n2, n6 for all keys)\n" << std::endl;
    
    // Lookup all keys from node 0
    std::cout << "---------------------node 0---------------------" << std::endl;
    for (uint8_t key : {3, 200, 123, 45, 99, 60, 50, 100, 101, 102, 240, 250}) {
        nodes[0]->find(key);
    }
    
    // Lookup all keys from node 2 (ID 65)
    std::cout << "\n---------------------node 65--------------------" << std::endl;
    for (uint8_t key : {3, 200, 123, 45, 99, 60, 50, 100, 101, 102, 240, 250}) {
        nodes[2]->find(key);
    }
    
    // Lookup all keys from node 6 (ID 100)
    std::cout << "\n---------------------node 100-------------------" << std::endl;
    for (uint8_t key : {3, 200, 123, 45, 99, 60, 50, 100, 101, 102, 240, 250}) {
        newNode->find(key);
    }
    
    // SECTION 5: Leave 
    std::cout << "\n5. Leave (20 pts)" << std::endl;
    std::cout << "Let one node n2 (ID 65) leave, print the updated finger tables of n0 and n1, and keys distribution\n" << std::endl;
    
    nodes[2]->leave();
    
    // Print updated finger tables
    std::cout << "Fig.6 Updated finger table" << std::endl;
    // Display correct predecessor before printing finger table
    std::cout << "Node id:" << static_cast<int>(nodes[0]->getId()) 
              << " Predecessor: " << static_cast<int>(nodes[0]->getPredecessor()->getId()) << std::endl;
    nodes[0]->getFingerTable().prettyPrint();
    
    // Display correct predecessor before printing finger table
    std::cout << "Node id:" << static_cast<int>(nodes[1]->getId()) 
              << " Predecessor: " << static_cast<int>(nodes[1]->getPredecessor()->getId()) << std::endl;
    nodes[1]->getFingerTable().prettyPrint();
    
    // Print key distribution after node leaves
    printKeysDistribution(nodes);
    
    // Clean up
    for (Node* node : nodes) {
        delete node;
    }
    
    return 0;
}