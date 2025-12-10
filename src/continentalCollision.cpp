#include <iostream>
#include <cmath>
#include <vector>
#include <unordered_set>
#include <limits>
#include <algorithm>

#include "tectonicPhenomenon.h"
#include "planet.h"
#include "crust.h"

const float COLLISION_RADIUS = 0.2f;        // Rayon d'influence de la collision
const float MOUNTAIN_HEIGHT = 6000.0f;      // Hauteur des montagnes créées (en mètres)
const float MOUNTAIN_WIDTH = 0.01f;         // Largeur de la zone de montagne
const float SMOOTHNESS = 5.0f;

float continentalCollisionUplift = 5000.0f;   // por ejemplo


float smoothMountainProfile(float distance, float width, float smoothness){
    if (distance >= width) return 0.0f;

    float normalizedDist = distance / width;
    float cosineFactor = (std::cos(normalizedDist * M_PI) + 1.0f) * 0.5f;
    float expFactor = std::exp(-smoothness * normalizedDist * normalizedDist);
    return 0.3f * cosineFactor + 0.7f * expFactor;
};


void ContinentalCollision::triggerEvent(Planet& planet) {
    float minZ = planet.min_elevation;
    float maxZ = planet.max_elevation;

    Plate& plateA = planet.plates[plate_a];
    Plate& plateB = planet.plates[plate_b];

    unsigned int phenomenonVertexIndex = getVertexIndex();
    Vec3 collisionVertex = planet.vertices[phenomenonVertexIndex];

    std::vector<unsigned int> verticesA =
        plateA.closestFrontierVertices[phenomenonVertexIndex];

    std::vector<unsigned int> verticesB =
        plateB.closestFrontierVertices[phenomenonVertexIndex];

    float v = planet.relativeVelocity(plateA, plateB);

    //
    // --- PROCESAR VÉRTICES DE LA PLACA A ---
    //
    for (unsigned int vertexIndex : verticesA) {

        Vec3 vertex = planet.vertices[vertexIndex];
        float d = distanceToInteractionFront(vertex, collisionVertex);

        float z = elevationImpact(
            planet.crust_data[vertexIndex]->relief_elevation,
            minZ, maxZ
        );

        float newElevation = continentalCollisionUplift * f(d) * g(v) * h(z);

        if (planet.crust_data[vertexIndex]->relief_elevation >= 6000.0f)
            newElevation *= 0.1f;
        else if (planet.crust_data[vertexIndex]->relief_elevation >= 4000.0f)
            newElevation *= 0.5f;
        else if (planet.crust_data[vertexIndex]->relief_elevation >= 2000.0f)
            newElevation *= 0.4f;

        planet.crust_data[vertexIndex]->relief_elevation += newElevation;
    }


    //
    // --- PROCESAR VÉRTICES DE LA PLACA B ---
    //
    for (unsigned int vertexIndex : verticesB) {

        Vec3 vertex = planet.vertices[vertexIndex];
        float d = distanceToInteractionFront(vertex, collisionVertex);

        float z = elevationImpact(
            planet.crust_data[vertexIndex]->relief_elevation,
            minZ, maxZ
        );

        float newElevation = continentalCollisionUplift * f(d) * g(v) * h(z);

        if (planet.crust_data[vertexIndex]->relief_elevation >= 6000.0f)
            newElevation *= 0.1f;
        else if (planet.crust_data[vertexIndex]->relief_elevation >= 4000.0f)
            newElevation *= 0.2f;
        else if (planet.crust_data[vertexIndex]->relief_elevation >= 2000.0f)
            newElevation *= 0.4f;

        planet.crust_data[vertexIndex]->relief_elevation += newElevation;
    }
}

// void ContinentalCollision::triggerEvent(Planet& planet) {
void ContinentalCollision::triggerTerranesMigration(Planet& planet) {
    unsigned int collisionVertex = getVertexIndex();
    unsigned int plateAIdx = getPlateA();
    unsigned int plateBIdx = getPlateB();

    if (collisionVertex >= planet.vertices.size()) return;
    if (plateAIdx >= planet.plates.size() || plateBIdx >= planet.plates.size()) return;

    Plate& plateA = planet.plates[plateAIdx];
    Plate& plateB = planet.plates[plateBIdx];

    Vec3 collisionPoint = planet.vertices[collisionVertex];

    // -----------------------------------------------
    // 1. Buscar terrane más cercano de A y B
    // -----------------------------------------------

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

    if (!foundTerraneA || !foundTerraneB) return;

    std::vector<unsigned int>& terraneA = plateA.terranes[terraneA_idx];
    std::vector<unsigned int>& terraneB = plateB.terranes[terraneB_idx];

    Vec3 centroidA = plateA.terraneCentroids[terraneA_idx];
    Vec3 centroidB = plateB.terraneCentroids[terraneB_idx];

    // -----------------------------------------------
    // 2. Ganador = terrane más grande
    // -----------------------------------------------

    bool terraneA_wins = terraneA.size() >= terraneB.size();

    std::vector<unsigned int>& winningTerrane = terraneA_wins ? terraneA : terraneB;
    std::vector<unsigned int>& losingTerrane  = terraneA_wins ? terraneB : terraneA;

    unsigned int winningPlateIdx = terraneA_wins ? plateAIdx : plateBIdx;
    unsigned int losingPlateIdx  = terraneA_wins ? plateBIdx : plateAIdx;

    size_t winningTerraneIdx = terraneA_wins ? terraneA_idx : terraneB_idx;
    size_t losingTerraneIdx  = terraneA_wins ? terraneB_idx : terraneA_idx;

    // -----------------------------------------------
    // 3. Seleccionar vértices del terrane perdedor
    //    que estén dentro de COLLISION_RADIUS
    // -----------------------------------------------

    std::vector<unsigned int> verticesToTransfer;
    std::unordered_set<unsigned int> transferSet;

    for (unsigned int vIdx : losingTerrane) {
        if (vIdx >= planet.vertices.size()) continue;

        float dist = (planet.vertices[vIdx] - collisionPoint).length();

        if (dist <= COLLISION_RADIUS) {
            verticesToTransfer.push_back(vIdx);
            transferSet.insert(vIdx);
        }
    }

    if (verticesToTransfer.empty()) return;

    // -----------------------------------------------
    // 4. Transferir vértices (no elevación)
    // -----------------------------------------------


    Plate& losingPlate  = planet.plates[losingPlateIdx];
    Plate& winningPlate = planet.plates[winningPlateIdx];
    
    // Actualizar ownership
    for (unsigned int vIdx : verticesToTransfer)
        planet.verticesToPlates[vIdx] = winningPlateIdx;

    // Quitar de la placa perdedora
    losingPlate.vertices_indices.erase(
        std::remove_if(
            losingPlate.vertices_indices.begin(),
            losingPlate.vertices_indices.end(),
            [&transferSet](unsigned int v) { return transferSet.count(v) > 0; }
        ),
        losingPlate.vertices_indices.end()
    );

    // Agregar a la placa ganadora (sin duplicados)
    for (unsigned int vIdx : verticesToTransfer) {
        if (std::find(winningPlate.vertices_indices.begin(),
                    winningPlate.vertices_indices.end(), vIdx)
            == winningPlate.vertices_indices.end()) 
        {
            winningPlate.vertices_indices.push_back(vIdx);
        }
    }

    // Agregar al terrane ganador
    winningTerrane.insert(
        winningTerrane.end(),
        verticesToTransfer.begin(),
        verticesToTransfer.end()
    );

    // -----------------------------------------------
    // 5. Actualizar centroides
    // -----------------------------------------------

    Vec3 newWinCentroid(0,0,0);
    for (unsigned int vIdx : winningTerrane)
        newWinCentroid += planet.vertices[vIdx];

    newWinCentroid /= (float)winningTerrane.size();
    newWinCentroid.normalize();
    newWinCentroid *= planet.radius;

    if (terraneA_wins)
        plateA.terraneCentroids[winningTerraneIdx] = newWinCentroid;
    else
        plateB.terraneCentroids[winningTerraneIdx] = newWinCentroid;

    // Reducir terrane perdedor
    losingTerrane.erase(
        std::remove_if(
            losingTerrane.begin(),
            losingTerrane.end(),
            [&transferSet](unsigned int v) { return transferSet.count(v) > 0; }
        ),
        losingTerrane.end()
    );

    // Si queda vacío → eliminar
    if (losingTerrane.empty()) {
        losingPlate.terranes.erase(losingPlate.terranes.begin() + losingTerraneIdx);
        losingPlate.terraneCentroids.erase(losingPlate.terraneCentroids.begin() + losingTerraneIdx);
    } else {
        // Recalcular centroide del perdedor
        Vec3 newLoseCentroid(0,0,0);
        for (unsigned int vIdx : losingTerrane)
            newLoseCentroid += planet.vertices[vIdx];

        newLoseCentroid /= (float)losingTerrane.size();
        newLoseCentroid.normalize();
        newLoseCentroid *= planet.radius;

        if (terraneA_wins)
            plateB.terraneCentroids[losingTerraneIdx] = newLoseCentroid;
        else
            plateA.terraneCentroids[losingTerraneIdx] = newLoseCentroid;
    }
}