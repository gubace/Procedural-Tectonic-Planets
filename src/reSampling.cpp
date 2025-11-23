#include <utility>
#include <limits>

#include "planet.h"
#include "crust.h"
#include "SphericalGrid.h"


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
    
    size_t N = vertices.size();
    crust_data.resize(N);
    verticesToPlates.resize(N);

    std::vector<Vec3> srcVerticesCopy = srcPlanet.vertices;

    SphericalGrid accel(srcVerticesCopy, 64); // résolution 64x64
    printf("SphericalGrid built with %zu vertices\n", srcVerticesCopy.size());

    for(int i = 0; i < N; ++i) { //TODO c'est lent Octree ?
        Vec3 currentVertex = vertices[i];
        unsigned int closestIndex = accel.nearest(currentVertex);
        
        if (closestIndex < srcPlanet.crust_data.size() && srcPlanet.crust_data[closestIndex]) {
            const Crust* srcCrust = srcPlanet.crust_data[closestIndex].get();
            
            const OceanicCrust* oc = dynamic_cast<const OceanicCrust*>(srcCrust);
            if (oc) {
                crust_data[i] = std::make_unique<OceanicCrust>(
                    oc->thickness, 
                    oc->relief_elevation, 
                    oc->age, 
                    oc->ridge_dir
                );
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

        unsigned int plateIndex = srcPlanet.verticesToPlates[closestIndex];
        verticesToPlates[i] = plateIndex;
        if (i % 1000 == 0) {
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

}