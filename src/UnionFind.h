#pragma once

#include <vector>
#include <map>
#include <chrono>
#include <iostream>

// Forward declaration
class Planet;

// Union-Find (Disjoint Set Union) pour une détection rapide des composantes connexes
class UnionFind {
private:
    std::vector<unsigned int> parent;
    std::vector<unsigned int> rank;
    
public:
    UnionFind(size_t n) : parent(n), rank(n, 0) {
        for (size_t i = 0; i < n; ++i) {
            parent[i] = static_cast<unsigned int>(i);
        }
    }
    
    unsigned int find(unsigned int x) {
        if (parent[x] != x) {
            parent[x] = find(parent[x]); // Path compression
        }
        return parent[x];
    }
    
    void unite(unsigned int x, unsigned int y) {
        unsigned int rootX = find(x);
        unsigned int rootY = find(y);
        
        if (rootX == rootY) return;
        
        // Union by rank
        if (rank[rootX] < rank[rootY]) {
            parent[rootX] = rootY;
        } else if (rank[rootX] > rank[rootY]) {
            parent[rootY] = rootX;
        } else {
            parent[rootY] = rootX;
            rank[rootX]++;
        }
    }
};

// Déclarations des fonctions (implémentation dans reSampling.cpp)
unsigned int findSurroundingMajorityPlate(
    Planet& planet,
    const std::vector<unsigned int>& islandVertices,
    unsigned int currentPlateId);

void cleanPlatesFast(Planet& planet, size_t minIslandSize = 50);