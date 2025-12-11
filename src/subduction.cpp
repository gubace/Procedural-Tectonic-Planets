#include <iostream>
#include <cmath>
#include <cstdio>
#include <limits>
#include <vector>

#include "tectonicPhenomenon.h"
#include "planet.h"

float subductionUplift = 1000.0f;

void Subduction::triggerEvent(Planet& planet) {

    float minZ = planet.min_elevation; 
    float maxZ = planet.max_elevation; 
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
        float d = distanceToInteractionFront(vertex, subductionVertex);
        float v = planet.relativeVelocity(plateUnder, plateOver);
        float z = elevationImpact(planet.crust_data[phenomenonVertexIndex]->relief_elevation, minZ, maxZ); // TODO: this should not be the elevation on the contact point. It should be the one of the plate that is under the current vertex

        

        float newElevation = subductionUplift * f(d) * g(v) * h(z);

        if(planet.crust_data[vertexIndex]->relief_elevation >= 6000.0f){
            newElevation *= 0.2f; 
        }else if(planet.crust_data[vertexIndex]->relief_elevation >= 4000.0f){
            newElevation *= 0.5f; 
        }
        else if(planet.crust_data[vertexIndex]->relief_elevation >= 2000.0f){
            newElevation *= 0.8f; 
        }

        planet.crust_data[vertexIndex]->relief_elevation += newElevation;
        
        //std::cout << " - Vertex " << vertexIndex << " elevated by " << newElevation << " to " << planet.crust_data[vertexIndex]->relief_elevation << std::endl;
    }
    
}