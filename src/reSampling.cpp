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

//================================ Planet Resampling ===================================

void Planet::resample(Planet& srcPlanet) { // Resample crust and plate data from srcPlanet to this planet
    
    auto t_total_start = std::chrono::steady_clock::now();

    size_t N = vertices.size();
    crust_data.resize(N);
    verticesToPlates.resize(N);

    std::vector<Vec3> srcVerticesCopy = srcPlanet.vertices;

    SphericalKDTree accel(srcVerticesCopy, srcPlanet);

    printf("SphericalKDTree built with %zu vertices\n", srcVerticesCopy.size());

    float expected_area = 4.0f * M_PI / float(srcPlanet.vertices.size());
    float expected_spacing = std::sqrt(expected_area); // approx chord length
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
    findFrontierVertices();
    fillClosestFrontierVertices();
    fillAllTerranes();


    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> riftChance(1, 5);
    
    if (riftChance(gen) == 2) { // N chances sur 10
        std::cout << "\nrifting" << std::endl;

        PlateRifting rifter;
        bool riftSuccess = rifter.triggerRifting(*this);

        if (riftSuccess) {
            detectVerticesNeighbors();
            findFrontierVertices();
            fillClosestFrontierVertices();
            fillAllTerranes();
        }
    }


    auto t_total_end = std::chrono::steady_clock::now();
    std::chrono::duration<double, std::milli> total_ms = t_total_end - t_total_start;
    std::cout << "reSampling total time: " << total_ms.count() << " ms" << std::endl;

}