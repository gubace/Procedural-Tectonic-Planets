#include <utility>
#include <limits>
#include <utility>
#include <memory>
#include <chrono>
#include <map>
#include <random>

#include "planet.h"
#include "crust.h"
#include "SphericalGrid.h"
#include "tectonicPhenomenon.h"
#include "rifting.h"
#include "UnionFind.h"



unsigned int Planet::findclosestVertex(const Vec3& point, Planet& srcPlanet){
    unsigned int closestIndex = 0;
    float minDistSq = std::numeric_limits<float>::max();

    for (unsigned int i = 0; i < srcPlanet.vertices.size(); ++i) {
        float distSq = (srcPlanet.vertices[i] - point).squareLength();
        if (distSq < minDistSq) {
            minDistSq = distSq;
            closestIndex = i;
        }
    }

    return closestIndex;
}

void computeCrustGenerationEvent(Planet& targetPlanet, Planet& srcPlanet,SphericalKDTree &accel, unsigned int vertexIndex, unsigned int closestIndex) { // Compute and trigger a crust generation event during resampling
        Vec3 currentVertex = targetPlanet.vertices[vertexIndex];
        std::pair<uint32_t, uint32_t> nearestDifferentPlates = accel.nearestFromDifferentPlates(currentVertex, srcPlanet);
        
        // Calculer le point milieu sur la ridge
        Vec3 q = (srcPlanet.vertices[nearestDifferentPlates.first] + srcPlanet.vertices[nearestDifferentPlates.second]) * 0.5f;
        
        Vec3 closestPlateBoundary = srcPlanet.vertices[nearestDifferentPlates.first];

        crustGeneration crustGenerationEvent(
            srcPlanet.verticesToPlates[nearestDifferentPlates.first],
            srcPlanet.verticesToPlates[nearestDifferentPlates.second],
            vertexIndex,
            0.02f,
            closestPlateBoundary,
            q,
            "Auto-generated rifting event during resampling"
        );

        crustGenerationEvent.triggerEvent(targetPlanet);
}

std::unique_ptr<Crust> copyCrust(Planet& srcPlanet, unsigned int closestIndex, 
                                  SphericalKDTree& accel, const Vec3& currentVertex) { 
    std::unique_ptr<Crust> crust_data;

    const Crust* srcCrust = srcPlanet.crust_data[closestIndex].get();
    
    // Vérifier si on est dans une zone de subduction océanique-continentale
    std::vector<unsigned int> neighbors = accel.kNearest(currentVertex, 2);
    
    bool hasOceanic = false;
    bool hasContinental = false;
    unsigned int continentalIndex = closestIndex;
    

    for (unsigned int neighborIdx : neighbors) {
        if (neighborIdx >= srcPlanet.crust_data.size() || !srcPlanet.crust_data[neighborIdx]) {
            continue;
        }
        
        const Crust* neighborCrust = srcPlanet.crust_data[neighborIdx].get();
        
        if (dynamic_cast<const OceanicCrust*>(neighborCrust)) {
            hasOceanic = true;
        } else if (dynamic_cast<const ContinentalCrust*>(neighborCrust)) {
            hasContinental = true;
            continentalIndex = neighborIdx;
        }
    }
    
    // Si on est dans une zone mixte océanique-continentale, privilégier la continentale
    if (hasOceanic && hasContinental) {

        bool isDifferentPlates = false;
        unsigned int closestPlate = srcPlanet.verticesToPlates[closestIndex];
        
        for (unsigned int neighborIdx : neighbors) {
            if (srcPlanet.verticesToPlates[neighborIdx] != closestPlate) {
                isDifferentPlates = true;
                break;
            }
        }
        

        if (isDifferentPlates) {
            srcCrust = srcPlanet.crust_data[continentalIndex].get();
            closestIndex = continentalIndex;
        }
    }
    
    //copy crust
    const OceanicCrust* oc = dynamic_cast<const OceanicCrust*>(srcCrust);
    if (oc) {
        if (oc->is_rifting) {
            crust_data = std::make_unique<OceanicCrust>(
                oc->thickness, 
                -5000.0f,
                oc->age, 
                oc->ridge_dir,
                false
            );
        } else {
            crust_data = std::make_unique<OceanicCrust>(
                oc->thickness, 
                oc->relief_elevation, 
                oc->age, 
                oc->ridge_dir,
                oc->is_rifting
            );
        }
    } else {
        const ContinentalCrust* cc = dynamic_cast<const ContinentalCrust*>(srcCrust);
        if (cc) {
            crust_data = std::make_unique<ContinentalCrust>(
                cc->thickness, 
                cc->relief_elevation, 
                cc->orogeny_age, 
                cc->orogeny_type, 
                cc->fold_dir
            );
        }
    }

    return crust_data;
}

 // threshold is kneighbors
unsigned int computePlateIndex(SphericalKDTree &accel, Planet &srcPlanet, unsigned int closestIndex, const Vec3 &currentVertex, int threshold = 3) {
    unsigned int closestPlate = srcPlanet.verticesToPlates[closestIndex];

    std::vector<unsigned int> neighbors = accel.kNearest(currentVertex, 8);

    if (neighbors.empty()) {
        return closestPlate; 
    }

    std::map<unsigned int, int> plateVotes;
    for (unsigned int neighborIdx : neighbors) {
        if (neighborIdx >= srcPlanet.verticesToPlates.size()) continue;
        if (neighborIdx == closestIndex) continue; // evitar doble conteo

        unsigned int neighborPlate = srcPlanet.verticesToPlates[neighborIdx];
        plateVotes[neighborPlate]++;
    }

    plateVotes[closestPlate]++;

    unsigned int majorityPlate = closestPlate;
    int maxVotes = 0;
    for (const auto &vote : plateVotes) {
        if (vote.second > maxVotes) {
            maxVotes = vote.second;
            majorityPlate = vote.first;
        }
    }

    int sameAsClosest = plateVotes[closestPlate];
    if (sameAsClosest < threshold) {
        return majorityPlate;
    } else {
        return closestPlate;
    }
}


// ================================ Clean plates ===================================

unsigned int findSurroundingMajorityPlate(
    Planet& planet,
    const std::vector<unsigned int>& islandVertices,
    unsigned int currentPlateId)
{
    std::map<unsigned int, int> plateVotes;
    
    for (unsigned int vertexIdx : islandVertices) {
        for (unsigned int neighborIdx : planet.neighbors[vertexIdx]) {
            unsigned int neighborPlate = planet.verticesToPlates[neighborIdx];
            
            // Ne compter que les voisins d'autres plaques
            if (neighborPlate != currentPlateId) {
                plateVotes[neighborPlate]++;
            }
        }
    }
    
    if (plateVotes.empty()) {
        return currentPlateId;
    }
    
    
    unsigned int majorityPlate = currentPlateId;
    int maxVotes = 0;
    
    for (const auto& [plateId, votes] : plateVotes) {
        if (votes > maxVotes) {
            maxVotes = votes;
            majorityPlate = plateId;
        }
    }
    
    return majorityPlate;
}

void cleanPlatesFast(Planet& planet, size_t minIslandSize) {
    auto t_start = std::chrono::steady_clock::now();
    
    //std::cout << "Fast plate cleaning (islands < " << minIslandSize << " vertices)..." << std::endl;
    
    size_t N = planet.vertices.size();
    UnionFind uf(N);
    
    // Étape 1: Unir les vertices connectés de la même plaque
    for (unsigned int i = 0; i < N; ++i) {
        unsigned int myPlate = planet.verticesToPlates[i];
        
        for (unsigned int neighborIdx : planet.neighbors[i]) {
            if (planet.verticesToPlates[neighborIdx] == myPlate) {
                uf.unite(i, neighborIdx);
            }
        }
    }
    
    // Étape 2: Compter la taille de chaque composante
    std::map<unsigned int, std::vector<unsigned int>> componentVertices;
    
    for (unsigned int i = 0; i < N; ++i) {
        unsigned int root = uf.find(i);
        componentVertices[root].push_back(i);
    }
    
    // Étape 3: Pour chaque plaque, identifier la composante principale
    std::map<unsigned int, unsigned int> plateMainComponent;
    std::map<unsigned int, size_t> plateMainComponentSize;
    
    for (const auto& [root, vertices] : componentVertices) {
        if (vertices.empty()) continue;
        
        unsigned int plateId = planet.verticesToPlates[vertices[0]];
        size_t componentSize = vertices.size();
        
        if (plateMainComponentSize.find(plateId) == plateMainComponentSize.end() ||
            componentSize > plateMainComponentSize[plateId]) {
            plateMainComponent[plateId] = root;
            plateMainComponentSize[plateId] = componentSize;
        }
    }
    
    
    unsigned int totalCleaned = 0;
    
    for (const auto& [root, vertices] : componentVertices) {
        if (vertices.empty()) continue;
        
        unsigned int plateId = planet.verticesToPlates[vertices[0]];
        size_t componentSize = vertices.size();
        
        
        if (root == plateMainComponent[plateId] || componentSize >= minIslandSize) {
            continue;
        }
        
        
        unsigned int newPlateId = findSurroundingMajorityPlate(planet, vertices, plateId);
        
        
        for (unsigned int vertexIdx : vertices) {
            planet.verticesToPlates[vertexIdx] = newPlateId;
        }
        
        totalCleaned += componentSize;
    }
    
    
    for (Plate& plate : planet.plates) {
        plate.vertices_indices.clear();
    }
    
    for (unsigned int i = 0; i < N; ++i) {
        unsigned int plateId = planet.verticesToPlates[i];
        if (plateId < planet.plates.size()) {
            planet.plates[plateId].vertices_indices.push_back(i);
        }
    }
    
    auto t_end = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(t_end - t_start).count();
    
    std::cout << "Fast plate cleaning: " << totalCleaned << " vertices reassigned in " 
              << duration << "ms" << std::endl;
}


//================================ Planet Resampling ===================================

void Planet::resample(Planet& srcPlanet) {
    
    auto t_total_start = std::chrono::steady_clock::now();

    size_t N = vertices.size();
    crust_data.resize(N);
    verticesToPlates.resize(N);

    std::vector<Vec3> srcVerticesCopy = srcPlanet.vertices;

    SphericalKDTree accel(srcVerticesCopy, srcPlanet);

    printf("SphericalKDTree built with %zu vertices\n", srcVerticesCopy.size());

    float expected_area = 4.0f * M_PI / float(srcPlanet.vertices.size());
    float expected_spacing = std::sqrt(expected_area);
    float expected_chord2 = expected_spacing * expected_spacing;

    std::cout << "Expected spacing: " << expected_spacing << ", expected chord^2: " << expected_chord2 << std::endl;


    for(int i = 0; i < N; ++i) {
        Vec3 currentVertex = vertices[i];
        unsigned int closestIndex = accel.nearest(currentVertex);

        if((srcPlanet.vertices[closestIndex] - currentVertex).squareLength() > expected_chord2) {
            float dist2 = (srcPlanet.vertices[closestIndex] - currentVertex).squareLength();
            computeCrustGenerationEvent(*this, srcPlanet, accel, i, closestIndex);
        } else if (closestIndex < srcPlanet.crust_data.size() && srcPlanet.crust_data[closestIndex]) {
            crust_data[i] = copyCrust(srcPlanet, closestIndex, accel, currentVertex);
        }

        unsigned int plateIndex = computePlateIndex(accel, srcPlanet, closestIndex, currentVertex);
        
        verticesToPlates[i] = plateIndex;

        if (i % 5000 == 0) {
            std::cout << "Resampling vertex " << i << "/" << vertices.size() << "\n";
        }
    }

    plates.resize(srcPlanet.plates.size());
    for(size_t i = 0; i < vertices.size(); ++i) {
        unsigned int plateIndex = verticesToPlates[i];
        if (plateIndex < plates.size()) {
            plates[plateIndex].vertices_indices.push_back(i);
        }
    }

    for(int i = 0; i < plates.size(); ++i) {
        plates[i].plate_velocity = srcPlanet.plates[i].plate_velocity;
        plates[i].rotation_axis = srcPlanet.plates[i].rotation_axis;
    }

    detectVerticesNeighbors();
    
    
    cleanPlatesFast(*this, N/100);
    
    findFrontierVertices();
    fillClosestFrontierVertices();
    fillAllTerranes();


    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> riftChance(1, 5);
    
    if (riftChance(gen) == 2) {
        std::cout << "\nrifting" << std::endl;

        PlateRifting rifter;
        bool riftSuccess = rifter.triggerRifting(*this);

        if (riftSuccess) {
            detectVerticesNeighbors();
            
            // ===== OPTIONNEL: Nettoyer après le rifting aussi =====
            cleanPlatesFast(*this, 20);
            
            findFrontierVertices();
            fillClosestFrontierVertices();
            fillAllTerranes();
        }
    }


    auto t_total_end = std::chrono::steady_clock::now();
    std::chrono::duration<double, std::milli> total_ms = t_total_end - t_total_start;
    std::cout << "reSampling total time: " << total_ms.count() << " ms" << std::endl;

}