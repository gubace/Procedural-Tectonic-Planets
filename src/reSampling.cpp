#include <utility>
#include <limits>

#include "planet.h"
#include "crust.h"


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
    
    crust_data.resize(vertices.size());
    verticesToPlates.resize(vertices.size());

    for(int i = 0; i < vertices.size(); ++i) {
        Vec3 currentVertex = vertices[i];
        unsigned int closestIndex = srcPlanet.findclosestVertex(currentVertex, srcPlanet);

        if (closestIndex < srcPlanet.crust_data.size() && srcPlanet.crust_data[closestIndex]) {
            crust_data[i] = std::move(srcPlanet.crust_data[closestIndex]);
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