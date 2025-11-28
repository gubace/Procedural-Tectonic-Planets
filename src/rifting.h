#pragma once

#include <vector>
#include "planet.h"

class PlateRifting {
public:

    static bool triggerRifting(Planet& planet);
    
    static bool riftPlate(Planet& planet, unsigned int plateIndex, unsigned int numFragments = 0);
    
private:
    static bool isPlateRiftable(const Plate& plate, size_t minVertices = 5000);
    
    static std::vector<Vec3> generateCentroids(const Plate& plate, 
                                               const Planet& planet, 
                                               unsigned int n);
    
    static std::vector<unsigned int> assignToVoronoiCells(
        const std::vector<unsigned int>& vertices,
        const Planet& planet,
        const std::vector<Vec3>& centroids);
    
    static void warpBoundaries(std::vector<unsigned int>& assignments,
                              const Planet& planet,
                              float warpStrength = 0.3f);
};