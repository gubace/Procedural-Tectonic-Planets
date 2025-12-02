#include <iostream>
#include <cmath>

#include "tectonicPhenomenon.h"
#include "planet.h"
#include "crust.h"

void crustGeneration::triggerEvent(Planet& planet) {

    if (q.length() == 0) {
        return; // not clean
    }

    unsigned int vertexIndex = getVertexIndex();
    
    //std::cout << "crustGeneration event triggered at vertex " << vertexIndex 
    //          << " between plates " << getPlateA() << " and " << getPlateB() << std::endl;
    
    Vec3 p = planet.vertices[vertexIndex];
    Vec3 ridgePosition = q;
    
    //distance to ridge (dΓ)
    float dGamma = (p - ridgePosition).length();
    
    //find distance to closest plate boundary (dP)
    float dP = (p - closestPlateBoundary).length();
    
 
    //α = dΓ(p) / (dΓ(p) + dP(p))
    float alpha = dGamma / (dGamma + dP + 1e-6f);
    

    float maxElevation = 0.0f;
    float width = 0.05f;
    float zGamma = maxElevation * std::exp(-(dGamma * dGamma) / (2.0f * width * width));
    
    
    float baseDepth = -5000.0f;
    float coefficient = 350.0f;
    float zOceanic = baseDepth;
    
    
    float zBar = 0.0f;
    if (vertexIndex < planet.crust_data.size() && planet.crust_data[vertexIndex]) {
        zBar = planet.crust_data[vertexIndex]->relief_elevation;
    }
    
    
    float newElevation = (1.0f - alpha) * zGamma + alpha * zBar;

    
    float riftInfluenceRadius = 0.08f;
    float blendFactor = std::min(1.0f, dGamma / (riftInfluenceRadius * 0.5f));
    newElevation = (1.0f - blendFactor) * newElevation + blendFactor * zOceanic;
    
    newElevation = std::min(newElevation, -10.0f);
    
    Vec3 pNorm = p;
    pNorm.normalize();
    Vec3 ridgeDir = Vec3::cross(p - ridgePosition, pNorm);
    ridgeDir.normalize();

    
    float age = dGamma / (divergence + 1e-6f);
    

    if (vertexIndex < planet.crust_data.size()) {
        OceanicCrust* existingOceanic = dynamic_cast<OceanicCrust*>(planet.crust_data[vertexIndex].get());
        
        if (existingOceanic) {

            existingOceanic->relief_elevation = newElevation;
            existingOceanic->age = std::min(existingOceanic->age, age);
            existingOceanic->ridge_dir = ridgeDir;
            
        } else {
            float thickness = 1000.0f + (newElevation > 0 ? newElevation * 0.5f : 0.0f);
            
            planet.crust_data[vertexIndex] = std::make_unique<OceanicCrust>(
                thickness,
                newElevation,
                age,
                ridgeDir,
                true
            );
            

        }
    }
}