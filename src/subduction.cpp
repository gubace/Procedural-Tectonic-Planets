#include <iostream>
#include <cmath>
#include <cstdio>
#include <limits>
#include <vector>

#include "tectonicPhenomenon.h"
#include "planet.h"

float r_s = 0.1f; // Distance that impacts uplift effect
float max_velocity = 1.0f; // TODO: idk, Timothée knows -> In fact Timothée doesn't know either
float subductionUplift = 500.0f;
float minZ = -8000; 
float maxZ = 8000; 

float f(float d) {
    if (d <= 0.0f) return 1.0f;
    if (d >= r_s) return 0.0f;

    float x = d / r_s;

    return 1.0f - 3.0f * x * x + 2.0f * x * x * x; // TODO: maybe this is not accurate, need to see the function shape
}

float g(float v) {
    return v/max_velocity;
}

float h(float z) {
    return z * z;
}

float distanceToSubductionFront(Vec3 p, Vec3 subductionFront) {
    Vec3 distance = p - subductionFront;
    return distance.length();
}

float relativeVelocity(Plate & plateUnder, Plate & plateOver) {
    float v = std::abs(plateUnder.plate_velocity - plateOver.plate_velocity);
    if (v < 0.0f) v = 0.0f;
    // clamp to max_velocity to keep g() in [0,1]
    if (v > max_velocity) v = max_velocity;
    return v;
}

float elevationImpact(float z) {
    if (maxZ <= minZ) return 0.0f;
    float normalizedZ = (z - minZ) / (maxZ - minZ);
    if (normalizedZ < 0.0f) normalizedZ = 0.0f;
    if (normalizedZ > 1.0f) normalizedZ = 1.0f;
    return normalizedZ;
}

void Subduction::triggerEvent(Planet& planet) {
    //std::cout << "Subduction event triggered at vertex " << getVertexIndex() << " between plates " << getPlateA() << " and " << getPlateB() << std::endl;
    Plate& plateUnder = planet.plates[plate_under];
    Plate& plateOver = planet.plates[plate_over];

    unsigned int phenomenonVertexIndex = getVertexIndex();
    

    std::vector<unsigned int> verticesClosestToPhenomenon = plateOver.closestFrontierVertices[phenomenonVertexIndex];
    for (unsigned int vertexIndex : verticesClosestToPhenomenon) {
        unsigned int vertex_plate = planet.verticesToPlates[vertexIndex];

        if (plate_under == vertex_plate) {
            planet.crust_data[vertexIndex]->is_under_subduction = true;
            continue; // We don't care about the plate that is under
        }

        Vec3 vertex = planet.vertices[vertexIndex];
        Vec3 subductionVertex = planet.vertices[phenomenonVertexIndex];
        float d = distanceToSubductionFront(vertex, subductionVertex);
        float v = relativeVelocity(plateUnder, plateOver);
        float z = elevationImpact(planet.crust_data[phenomenonVertexIndex]->relief_elevation); // TODO: this should not be the elevation on the contact point. It should be the one of the plate that is under the current vertex

        float newElevation = subductionUplift * f(d) * g(v) * h(z);
        planet.crust_data[vertexIndex]->relief_elevation += newElevation;
        
        //std::cout << " - Vertex " << vertexIndex << " elevated by " << newElevation << " to " << planet.crust_data[vertexIndex]->relief_elevation << std::endl;
    }
    
}