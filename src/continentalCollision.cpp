#include <iostream>
#include <cmath>
#include <vector>
#include <unordered_set>
#include <limits>
#include <algorithm>

#include "tectonicPhenomenon.h"
#include "planet.h"
#include "crust.h"

const float COLLISION_RADIUS = 0.3f;        // Rayon d'influence de la collision
const float MOUNTAIN_HEIGHT = 5000.0f;      // Hauteur des montagnes créées (en mètres)
const float MOUNTAIN_WIDTH = 0.05f;         // Largeur de la zone de montagne
const float SMOOTHNESS = 5.0f; 

float smoothMountainProfile(float distance, float width, float smoothness){
        if (distance >= width) return 0.0f;
        
        float normalizedDist = distance / width;
        float cosineFactor = (std::cos(normalizedDist * M_PI) + 1.0f) * 0.5f;
        float expFactor = std::exp(-smoothness * normalizedDist * normalizedDist);
        return 0.3f * cosineFactor + 0.7f * expFactor;
    };



void ContinentalCollision::triggerEvent(Planet& planet) {

    
    unsigned int collisionVertex = getVertexIndex();
    unsigned int plateAIdx = getPlateA();
    unsigned int plateBIdx = getPlateB();
    
    // Vérifications
    if (collisionVertex >= planet.vertices.size()) {
        std::cerr << "ERROR: Invalid collision vertex" << std::endl;
        return;
    }
    if (plateAIdx >= planet.plates.size() || plateBIdx >= planet.plates.size()) {
        std::cerr << "ERROR: Invalid plate indices" << std::endl;
        return;
    }
    
    Plate& plateA = planet.plates[plateAIdx];
    Plate& plateB = planet.plates[plateBIdx];
    
    // std::cout << "Collision at vertex " << collisionVertex 
    //           << " between plates " << plateAIdx << " and " << plateBIdx << std::endl;
    
    
    Vec3 collisionPoint = planet.vertices[collisionVertex];
    
    size_t terraneA_idx = 0;
    size_t terraneB_idx = 0;
    float minDistA = std::numeric_limits<float>::max();
    float minDistB = std::numeric_limits<float>::max();
    bool foundTerraneA = false;
    bool foundTerraneB = false;
    
    
    for (size_t i = 0; i < plateA.terranes.size(); ++i) {
        if (plateA.terranes[i].empty()) continue;
        float dist = (collisionPoint - plateA.terraneCentroids[i]).length();
        if (dist < minDistA) {
            minDistA = dist;
            terraneA_idx = i;
            foundTerraneA = true;
        }
    }
    
    
    for (size_t i = 0; i < plateB.terranes.size(); ++i) {
        if (plateB.terranes[i].empty()) continue;
        float dist = (collisionPoint - plateB.terraneCentroids[i]).length();
        if (dist < minDistB) {
            minDistB = dist;
            terraneB_idx = i;
            foundTerraneB = true;
        }
    }
    
    if (!foundTerraneA || !foundTerraneB) {
        // std::cout << "  No terranes found for collision. Aborting." << std::endl;
        return;
    }
    
    std::vector<unsigned int>& terraneA = plateA.terranes[terraneA_idx];
    std::vector<unsigned int>& terraneB = plateB.terranes[terraneB_idx];
    Vec3 centroidA = plateA.terraneCentroids[terraneA_idx];
    Vec3 centroidB = plateB.terraneCentroids[terraneB_idx];
    
    // std::cout << "  Terrane A: " << terraneA.size() << " vertices" << std::endl;
    // std::cout << "  Terrane B: " << terraneB.size() << " vertices" << std::endl;
    

    bool terraneA_wins = terraneA.size() >= terraneB.size();
    
    std::vector<unsigned int>& winningTerrane = terraneA_wins ? terraneA : terraneB;
    std::vector<unsigned int>& losingTerrane = terraneA_wins ? terraneB : terraneA;
    Vec3 winningCentroid = terraneA_wins ? centroidA : centroidB;
    Vec3 losingCentroid = terraneA_wins ? centroidB : centroidA;
    unsigned int winningPlateIdx = terraneA_wins ? plateAIdx : plateBIdx;
    unsigned int losingPlateIdx = terraneA_wins ? plateBIdx : plateAIdx;
    size_t winningTerraneIdx = terraneA_wins ? terraneA_idx : terraneB_idx;
    size_t losingTerraneIdx = terraneA_wins ? terraneB_idx : terraneA_idx;
    
    // std::cout << "  Winner: Plate " << winningPlateIdx << " (terrane with " 
    //           << winningTerrane.size() << " vertices)" << std::endl;
    // std::cout << "  Loser: Plate " << losingPlateIdx << " (terrane with " 
    //           << losingTerrane.size() << " vertices)" << std::endl;
    
    std::vector<unsigned int> verticesToTransfer;
    std::unordered_set<unsigned int> transferSet;
    
    for (unsigned int vIdx : losingTerrane) {
        if (vIdx >= planet.vertices.size()) continue;
        
        float distToCollision = (planet.vertices[vIdx] - collisionPoint).length();
        if (distToCollision <= COLLISION_RADIUS) {
            verticesToTransfer.push_back(vIdx);
            transferSet.insert(vIdx);
        }
    }
    
    // std::cout << "  Vertices to transfer: " << verticesToTransfer.size() 
    //           << " (within radius " << COLLISION_RADIUS << ")" << std::endl;
    
    if (verticesToTransfer.empty()) {
        // std::cout << "  No vertices close enough to transfer. Aborting." << std::endl;
        return;
    }
    
    Vec3 junctionCenter = (winningCentroid + losingCentroid) * 0.5f;
    unsigned int mountainsCreated = 0;
    
    float extendedMountainRadius = MOUNTAIN_WIDTH * 2.0f;

    for (unsigned int i = 0; i < planet.vertices.size(); ++i) {
        Vec3 p = planet.vertices[i];
        float distToJunction = (p - junctionCenter).length();
        

        if (distToJunction > extendedMountainRadius) continue;
        

        float mountainFactor = smoothMountainProfile(distToJunction, extendedMountainRadius, SMOOTHNESS);
        
        if (mountainFactor < 0.01f) continue; // Skip si contribution négligeable
        
  
        float elevationIncrease = MOUNTAIN_HEIGHT * mountainFactor;
        
        if (i < planet.crust_data.size() && planet.crust_data[i]) {

            if(planet.crust_data[i]->relief_elevation < 8000.0f) planet.crust_data[i]->relief_elevation += elevationIncrease;
            

            ContinentalCrust* cc = dynamic_cast<ContinentalCrust*>(planet.crust_data[i].get());
            if (cc) {
                Vec3 n = p;
                n.normalize();
                Vec3 toJunction = junctionCenter - p;
                float toJunctionLen = toJunction.length();
                if (toJunctionLen > 1e-6f) {
                    toJunction /= toJunctionLen;
                    
                    Vec3 foldDir = Vec3::cross(n, toJunction);
                    foldDir.normalize();
                    
                    cc->fold_dir = foldDir;
                    cc->orogeny_type = OrogenyType::Collisional;
                    cc->orogeny_age = 0.0f;
                }
                
                if (elevationIncrease > 500.0f) {
                    mountainsCreated++;
                }
            }
        }
    }
    
   
    
    for (unsigned int vIdx : verticesToTransfer) {
        planet.verticesToPlates[vIdx] = winningPlateIdx;
    }
    
    // Retirer de la plaque perdante
    Plate& losingPlate = planet.plates[losingPlateIdx];
    losingPlate.vertices_indices.erase(
        std::remove_if(
            losingPlate.vertices_indices.begin(),
            losingPlate.vertices_indices.end(),
            [&transferSet](unsigned int v) { return transferSet.find(v) != transferSet.end(); }
        ),
        losingPlate.vertices_indices.end()
    );
    
    // Ajouter à la plaque gagnante
    Plate& winningPlate = planet.plates[winningPlateIdx];
    winningPlate.vertices_indices.insert(
        winningPlate.vertices_indices.end(),
        verticesToTransfer.begin(),
        verticesToTransfer.end()
    );
    
    //ajouter les vertices transférés à la terrane gagnante
    winningTerrane.insert(
        winningTerrane.end(),
        verticesToTransfer.begin(),
        verticesToTransfer.end()
    );
    
    //recalculer le centroïde de la terrane gagnante fusionnée
    Vec3 newCentroid(0.0f, 0.0f, 0.0f);
    for (unsigned int vIdx : winningTerrane) {
        if (vIdx < planet.vertices.size()) {
            newCentroid += planet.vertices[vIdx];
        }
    }
    newCentroid /= (float)winningTerrane.size();
    newCentroid.normalize();
    newCentroid *= planet.radius;
    
    if (terraneA_wins) {
        plateA.terraneCentroids[winningTerraneIdx] = newCentroid;
    } else {
        plateB.terraneCentroids[winningTerraneIdx] = newCentroid;
    }
    
    losingTerrane.erase(
        std::remove_if(
            losingTerrane.begin(),
            losingTerrane.end(),
            [&transferSet](unsigned int v) { return transferSet.find(v) != transferSet.end(); }
        ),
        losingTerrane.end()
    );
    
    // Si la terrane perdante est maintenant vide, la supprimer
    if (losingTerrane.empty()) {
        Plate& losingPlateRef = planet.plates[losingPlateIdx];
        losingPlateRef.terranes.erase(losingPlateRef.terranes.begin() + losingTerraneIdx);
        losingPlateRef.terraneCentroids.erase(losingPlateRef.terraneCentroids.begin() + losingTerraneIdx);
        // std::cout << "  Losing terrane completely absorbed and removed." << std::endl;
    } else {
        // Recalculer le centroïde de la terrane perdante
        Vec3 newLosingCentroid(0.0f, 0.0f, 0.0f);
        for (unsigned int vIdx : losingTerrane) {
            if (vIdx < planet.vertices.size()) {
                newLosingCentroid += planet.vertices[vIdx];
            }
        }
        newLosingCentroid /= (float)losingTerrane.size();
        newLosingCentroid.normalize();
        newLosingCentroid *= planet.radius;
        
        if (terraneA_wins) {
            plateB.terraneCentroids[losingTerraneIdx] = newLosingCentroid;
        } else {
            plateA.terraneCentroids[losingTerraneIdx] = newLosingCentroid;
        }
        
        // std::cout << "  Losing terrane reduced to " << losingTerrane.size() << " vertices." << std::endl;
    }
    
    // std::cout << "  Collision completed successfully!" << std::endl;
    // std::cout << "  Merged terrane now has " << winningTerrane.size() << " vertices" << std::endl;
    // std::cout << "========================================\n" << std::endl;
}