# Chord-Implementation

Chord DHT Implementation

This project implements the Chord Distributed Hash Table (DHT) protocol as described in the paper "Chord: A Scalable Peer-to-peer Lookup Service for Internet Applications" by Ion Stoica et al.

## Project Overview

The implementation provides a functional Chord DHT with the following features:

1. Node join and leave operations
2. Finger table construction and maintenance
3. Key-value pair storage and retrieval
4. Stabilization protocol for network consistency
5. Space Shuffle optimization for load balancing 

## Files

1. node.h - Header file containing the Node and FingerTable class definitions
2. node.cpp - Implementation of the Node and FingerTable classes
3. main.cpp - Test program that demonstrates the Chord DHT functionality

## Compilation Instructions

Important Note: Before compiling, make sure you navigate to the directory containing all the project files (node.h, node.cpp, main.cpp) using the terminal or command prompt.

Windows
To compile the project on Windows, use the following command:

g++ main.cpp node.cpp -o chord_dht

macOS

To compile the project on macOS, use the following command:

g++ -std=c++11 main.cpp node.cpp -o chord_dht

If you don't have g++ installed, you can use clang++ instead:

clang++ -std=c++11 main.cpp node.cpp -o chord_dht

Linux

To compile the project on Linux, use the following command:

g++ -std=c++11 main.cpp node.cpp -o chord_dht

## Running the Program
Make sure you're still in the directory containing the compiled executable before running the following commands.

Windows
After compilation, run the program using:

chord_dht

macOS/Linux

After compilation, run the program using:

./chord_dht

## Implementation Details

Chord Features

1. Node Join: When a node joins the Chord network, it:
   - Initializes its finger table
   - Updates finger tables of existing nodes
   - Takes responsibility for keys from its successor

2. Key Lookup: The lookup algorithm efficiently finds the node responsible for a key by:
   - Traversing the finger table to find the closest preceding node
   - Following successor pointers to locate the exact node

3. Stabilization: The implementation includes a stabilization protocol to maintain correct successor pointers when nodes join or leave.

4. Node Leave: When a node leaves, it:
   - Transfers its keys to its successor
   - Updates finger tables of affected nodes

5. Space Shuffle Optimization: This feature balances key distribution across nodes.

Key Functions

- join(Node* node): Adds a node to the Chord network
- find(uint8_t key): Locates the value associated with a key
- insert(uint8_t key, uint8_t value): Stores a key-value pair
- remove(uint8_t key): Removes a key from the DHT
- leave(): Removes a node from the network

## Testing

The main.cpp file contains a comprehensive test suite that:

1. Creates a Chord network with multiple nodes
2. Inserts keys into the network
3. Demonstrates lookups from different nodes
4. Shows key migration when nodes join/leave
5. Tests the optional node leave functionality
