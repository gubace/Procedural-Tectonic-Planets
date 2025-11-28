#include <iostream>
#include <cmath>

#include "tectonicPhenomenon.h"
#include "planet.h"
#include "crust.h"

void crustGeneration::triggerEvent(Planet& planet) {

    if (q.length() == 0) {
        //std::cerr << "Error: Ridge position 'q' is not defined for crustGeneration event." << std::endl;
        return;
    }

    unsigned int vertexIndex = getVertexIndex();
    
    std::cout << "crustGeneration event triggered at vertex " << vertexIndex 
              << " between plates " << getPlateA() << " and " << getPlateB() << std::endl;
    
    Vec3 p = planet.vertices[vertexIndex];
    Vec3 ridgePosition = q; // Position sur la ridge
    
    // Distance to ridge (dΓ)
    float dGamma = (p - ridgePosition).length();
    
    // Find distance to closest plate boundary (dP)
    float dP = (p - closestPlateBoundary).length();
    
 
    // Interpolating factor: α = dΓ(p) / (dΓ(p) + dP(p))
    float alpha = dGamma / (dGamma + dP + 1e-6f);
    
    // Template ridge elevation profile (Gaussian-like)
    float maxElevation = 2500.0f; // Elevation maximale au centre de la ridge
    float width = 0.05f;
    float zGamma = maxElevation * std::exp(-(dGamma * dGamma) / (2.0f * width * width));
    
    // Oceanic crust age model: elevation decreases with distance from ridge
    float baseDepth = -4000.0f;
    float coefficient = 350.0f;
    float zOceanic = baseDepth - coefficient * std::sqrt(dGamma);
    
    // Linearly interpolated elevation between plates (if existing crust data)
    float zBar = 0.0f;
    if (vertexIndex < planet.crust_data.size() && planet.crust_data[vertexIndex]) {
        zBar = planet.crust_data[vertexIndex]->relief_elevation;
    }
    
    // z(p,t) = (1-α) * zΓ + α * z̄
    float newElevation = (1.0f - alpha) * zGamma + alpha * zBar;
    
    float riftInfluenceRadius = 0.15f;
    float blendFactor = std::min(1.0f, dGamma / (riftInfluenceRadius * 0.5f));
    newElevation = (1.0f - blendFactor) * newElevation + blendFactor * zOceanic;
    
    // Compute ridge direction for this point: r(p) = (p - q) × p
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
            
            std::cout << "  Updated oceanic crust at vertex " << vertexIndex 
                      << " | elevation: " << newElevation 
                      << " | age: " << age << " Ma" << std::endl;
        } else {
            float thickness = 6000.0f + (newElevation > 0 ? newElevation * 0.5f : 0.0f);
            
            planet.crust_data[vertexIndex] = std::make_unique<OceanicCrust>(
                thickness,
                newElevation,
                age,
                ridgeDir
            );
            
            std::cout << "  Created oceanic crust at vertex " << vertexIndex 
                      << " | elevation: " << newElevation 
                      << " | thickness: " << thickness
                      << " | age: " << age << " Ma" << std::endl;
        }
    }
}