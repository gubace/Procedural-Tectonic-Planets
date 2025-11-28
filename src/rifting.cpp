#include "rifting.h"

#include <random>
#include <algorithm>
#include <unordered_set>
#include <iostream>
#include <cmath>

bool PlateRifting::triggerRifting(Planet& planet) {
    if (planet.plates.empty()) {
        std::cout << "No plates to rift." << std::endl;
        return false;
    }
    
    std::vector<unsigned int> riftablePlates;
    for (size_t i = 0; i < planet.plates.size(); ++i) {
        if (isPlateRiftable(planet.plates[i])) {
            riftablePlates.push_back(i);
        }
    }
    
    if (riftablePlates.empty()) {
        std::cout << "No plates large enough for rifting." << std::endl;
        return false;
    }
    

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<size_t> dist(0, riftablePlates.size() - 1);
    unsigned int selectedPlate = riftablePlates[dist(gen)];
    
    std::cout << "Selected plate " << selectedPlate << " for rifting." << std::endl;
    

    std::uniform_int_distribution<unsigned int> fragDist(2, 3);
    unsigned int numFragments = fragDist(gen);
    
    return riftPlate(planet, selectedPlate, numFragments);
}

bool PlateRifting::isPlateRiftable(const Plate& plate, size_t minVertices) {
    return plate.vertices_indices.size() >= minVertices;
}

std::vector<Vec3> PlateRifting::generateCentroids(const Plate& plate, 
                                                   const Planet& planet, 
                                                   unsigned int n) {
    std::vector<Vec3> centroids;
    
    if (plate.vertices_indices.empty() || n == 0) {
        return centroids;
    }
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<size_t> dist(0, plate.vertices_indices.size() - 1);
    
    std::unordered_set<unsigned int> selectedIndices;
    
    while (centroids.size() < n && selectedIndices.size() < plate.vertices_indices.size()) {
        size_t randomIdx = dist(gen);
        unsigned int vertexIdx = plate.vertices_indices[randomIdx];
        
        if (selectedIndices.find(vertexIdx) == selectedIndices.end()) {
            selectedIndices.insert(vertexIdx);
            

            Vec3 candidate = planet.vertices[vertexIdx];
            bool tooClose = false;
            
            for (const Vec3& existing : centroids) {
                float dist = (candidate - existing).length();
                if (dist < 0.3f) { 
                    tooClose = true;
                    break;
                }
            }
            
            if (!tooClose) {
                centroids.push_back(candidate);
                std::cout << "  Centroid " << centroids.size() << " at vertex " << vertexIdx << std::endl;
            }
        }
    }
    
    return centroids;
}

std::vector<unsigned int> PlateRifting::assignToVoronoiCells(
    const std::vector<unsigned int>& vertices,
    const Planet& planet,
    const std::vector<Vec3>& centroids) {
    
    std::vector<unsigned int> assignments(planet.vertices.size(), 0);
    
    for (unsigned int vIdx : vertices) {
        if (vIdx >= planet.vertices.size()) continue;
        
        const Vec3& v = planet.vertices[vIdx];
        float minDist = std::numeric_limits<float>::max();
        unsigned int closestCentroid = 0;
        
        for (size_t i = 0; i < centroids.size(); ++i) {
            float dist = (v - centroids[i]).squareLength();
            if (dist < minDist) {
                minDist = dist;
                closestCentroid = i;
            }
        }
        
        assignments[vIdx] = closestCentroid;
    }
    
    return assignments;
}

void PlateRifting::warpBoundaries(std::vector<unsigned int>& assignments,
                                  const Planet& planet,
                                  float warpStrength) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
    

    std::vector<bool> isBoundary(planet.vertices.size(), false);
    
    for (size_t i = 0; i < planet.vertices.size(); ++i) {
        if (i >= planet.neighbors.size()) continue;
        
        unsigned int myCell = assignments[i];
        
        for (unsigned int neighbor : planet.neighbors[i]) {
            if (neighbor < assignments.size() && assignments[neighbor] != myCell) {
                isBoundary[i] = true;
                break;
            }
        }
    }
    

    for (size_t i = 0; i < planet.vertices.size(); ++i) {
        if (!isBoundary[i]) continue;
        

        if (dist(gen) < warpStrength && i < planet.neighbors.size()) {
            std::vector<unsigned int> neighborCells;
            
            for (unsigned int neighbor : planet.neighbors[i]) {
                if (neighbor < assignments.size()) {
                    neighborCells.push_back(assignments[neighbor]);
                }
            }
            
            if (!neighborCells.empty()) {
                std::uniform_int_distribution<size_t> cellDist(0, neighborCells.size() - 1);
                assignments[i] = neighborCells[cellDist(gen)];
            }
        }
    }
}

bool PlateRifting::riftPlate(Planet& planet, unsigned int plateIndex, unsigned int numFragments) {
    if (plateIndex >= planet.plates.size()) {
        std::cerr << "Invalid plate index: " << plateIndex << std::endl;
        return false;
    }
    
    Plate& originalPlate = planet.plates[plateIndex];
    
    if (!isPlateRiftable(originalPlate)) {
        std::cout << "Plate " << plateIndex << " is too small to rift." << std::endl;
        return false;
    }
    
    // Si numFragments = 0, choisir aléatoirement entre 2 et 3
    if (numFragments == 0) {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<unsigned int> dist(2, 3);
        numFragments = dist(gen);
    }
    
    std::cout << "\n========================================" << std::endl;
    std::cout << "Plate Rifting Event!" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "Rifting plate " << plateIndex << " into " << numFragments << " fragments" << std::endl;
    std::cout << "Original plate size: " << originalPlate.vertices_indices.size() << " vertices" << std::endl;
    
    
    std::vector<Vec3> centroids = generateCentroids(originalPlate, planet, numFragments);
    
    if (centroids.size() < numFragments) {
        std::cerr << "Could not generate enough centroids. Aborting rifting." << std::endl;
        return false;
    }
    
    std::vector<unsigned int> assignments = assignToVoronoiCells(
        originalPlate.vertices_indices, 
        planet, 
        centroids
    );
    
    
    warpBoundaries(assignments, planet, 0.3f);
    
    
    std::vector<std::vector<unsigned int>> newPlatesVertices(numFragments);
    
    for (unsigned int vIdx : originalPlate.vertices_indices) {
        if (vIdx < assignments.size()) {
            unsigned int cellIdx = assignments[vIdx];
            if (cellIdx < numFragments) {
                newPlatesVertices[cellIdx].push_back(vIdx);
            }
        }
    }
    
    
    for (size_t i = 0; i < newPlatesVertices.size(); ++i) {
        std::cout << "  Fragment " << i << ": " << newPlatesVertices[i].size() << " vertices" << std::endl;
        
        if (newPlatesVertices[i].empty()) {
            std::cerr << "Fragment " << i << " is empty! Aborting rifting." << std::endl;
            return false;
        }
    }
    
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> angleDist(0.0f, 2.0f * M_PI);
    std::uniform_real_distribution<float> velocityDist(0.1f, 0.9f);
    
    originalPlate.vertices_indices = newPlatesVertices[0];
    

    float theta = angleDist(gen);
    float phi = angleDist(gen);
    originalPlate.rotation_axis = Vec3(
        std::sin(phi) * std::cos(theta),
        std::sin(phi) * std::sin(theta),
        std::cos(phi)
    );
    originalPlate.rotation_axis.normalize();
    originalPlate.plate_velocity = velocityDist(gen);
    

    for (unsigned int vIdx : originalPlate.vertices_indices) {
        if (vIdx < planet.verticesToPlates.size()) {
            planet.verticesToPlates[vIdx] = plateIndex;
        }
    }
    

    originalPlate.terranes.clear();
    originalPlate.terraneCentroids.clear();
    originalPlate.fillTerranes(planet);
    

    for (size_t i = 1; i < numFragments; ++i) {
        Plate newPlate;
        newPlate.vertices_indices = newPlatesVertices[i];
        

        theta = angleDist(gen);
        phi = angleDist(gen);
        newPlate.rotation_axis = Vec3(
            std::sin(phi) * std::cos(theta),
            std::sin(phi) * std::sin(theta),
            std::cos(phi)
        );
        newPlate.rotation_axis.normalize();
        newPlate.plate_velocity = velocityDist(gen);
        
        unsigned int newPlateIndex = planet.plates.size();
        planet.plates.push_back(newPlate);
        

        for (unsigned int vIdx : newPlate.vertices_indices) {
            if (vIdx < planet.verticesToPlates.size()) {
                planet.verticesToPlates[vIdx] = newPlateIndex;
            }
        }
        
        planet.plates[newPlateIndex].fillTerranes(planet);
        
    }
    
    std::cout << "Rifting completed! Total plates: " << planet.plates.size() << std::endl;
    std::cout << "========================================\n" << std::endl;
    
    return true;
}