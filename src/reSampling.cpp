#include <utility>
#include <limits>
#include <utility>

#include "planet.h"
#include "crust.h"
#include "SphericalGrid.h"
#include "tectonicPhenomenon.h"
#include "rifting.h"

#include <chrono>


unsigned int Planet::findclosestVertex(const Vec3& point, Planet& srcPlanet){
    unsigned int closestIndex = 0;
    float minDistSq = std::numeric_limits<float>::max();

    for (unsigned int i = 0; i < vertices.size(); ++i) {
        float distSq = (srcPlanet.vertices[i] - point).squareLength();
        if (distSq < minDistSq) {
            minDistSq = distSq;
            closestIndex = i;
        }
    }

    return closestIndex;
}


void Planet::resample(Planet& srcPlanet) {
    
    auto t_total_start = std::chrono::steady_clock::now();

    size_t N = vertices.size();
    crust_data.resize(N);
    verticesToPlates.resize(N);

    std::vector<Vec3> srcVerticesCopy = srcPlanet.vertices;

    SphericalGrid accel(srcVerticesCopy, srcPlanet, 64); // résolution 64x64
    printf("SphericalGrid built with %zu vertices\n", srcVerticesCopy.size());

    for(int i = 0; i < N; ++i) {
        Vec3 currentVertex = vertices[i];
        unsigned int closestIndex = accel.nearest(currentVertex);

        if((srcPlanet.vertices[closestIndex] - currentVertex).squareLength() > 0.0004f) {
            std::pair<uint32_t, uint32_t> nearestDifferentPlates = accel.nearestFromDifferentPlates(currentVertex, srcPlanet);
            
            // Calculer le point milieu sur la ridge
            Vec3 q = (srcPlanet.vertices[nearestDifferentPlates.first] + srcPlanet.vertices[nearestDifferentPlates.second]) * 0.5f;
            
            Vec3 closestPlateBoundary = srcPlanet.vertices[nearestDifferentPlates.first];



            crustGeneration crustGenerationEvent(
                srcPlanet.verticesToPlates[nearestDifferentPlates.first],
                srcPlanet.verticesToPlates[nearestDifferentPlates.second],
                i,
                0.02f,
                closestPlateBoundary,
                q,
                "Auto-generated rifting event during resampling"
            );

            crustGenerationEvent.triggerEvent(*this);
        }
        
        else if (closestIndex < srcPlanet.crust_data.size() && srcPlanet.crust_data[closestIndex]) {
            const Crust* srcCrust = srcPlanet.crust_data[closestIndex].get();
            
            const OceanicCrust* oc = dynamic_cast<const OceanicCrust*>(srcCrust);
            if (oc) {
                if (oc->is_rifting) {
                    crust_data[i] = std::make_unique<OceanicCrust>(
                        oc->thickness, 
                        -5000.0f,
                        oc->age, 
                        oc->ridge_dir,
                        false
                    );
                }else{
                    crust_data[i] = std::make_unique<OceanicCrust>(
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
                    crust_data[i] = std::make_unique<ContinentalCrust>(
                        cc->thickness, 
                        cc->relief_elevation, 
                        cc->orogeny_age, 
                        cc->orogeny_type, 
                        cc->fold_dir
                    );
                }
            }
        }

        unsigned int plateIndex;
        std::vector<unsigned int> neighbors = accel.kNearest(currentVertex, 8);
        
        if (neighbors.empty()) {
            plateIndex = srcPlanet.verticesToPlates[closestIndex];
        } else {
            std::map<unsigned int, int> plateVotes;
            
            for (unsigned int neighborIdx : neighbors) {
                if (neighborIdx < srcPlanet.verticesToPlates.size()) {
                    unsigned int neighborPlate = srcPlanet.verticesToPlates[neighborIdx];
                    plateVotes[neighborPlate]++;
                }
            }
            
            unsigned int majorityPlate = srcPlanet.verticesToPlates[closestIndex];
            int maxVotes = 0;
            
            for (const auto& vote : plateVotes) {
                if (vote.second > maxVotes) {
                    maxVotes = vote.second;
                    majorityPlate = vote.first;
                }
            }
            
            unsigned int closestPlate = srcPlanet.verticesToPlates[closestIndex];
            int sameAsClosest = plateVotes[closestPlate];
            
            if (sameAsClosest < 3) {
                plateIndex = majorityPlate;
            } else {
                plateIndex = closestPlate;
            }
        }
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


    auto t_total_end = std::chrono::steady_clock::now();
    std::chrono::duration<double, std::milli> total_ms = t_total_end - t_total_start;
    std::cout << "reSampling total time: " << total_ms.count() << " ms" << std::endl;

}