#include "planet.h"
#include "crust.h"


unsigned int planet::findclosestVertex(const Vec3& point, planet& srcPlanet){
    unsigned int closestIndex = 0;
    float minDistSq = std::numeric_limits<float>::max();

    for (unsigned int i = 0; i < vertices.size(); ++i) {
        float distSq = (srcPlanet.vertices[i] - point).lengthSquared();
        if (distSq < minDistSq) {
            minDistSq = distSq;
            closestIndex = i;
        }
    }

    return closestIndex;
}


void planet::resample(planet& srcPlanet) {
    
    for(int i = 0; i < vertices.size(); ++i) {
        Vec3 currentVertex = vertices[i];
        unsigned int closestIndex = srcPlanet.findclosestVertex(currentVertex, srcPlanet);

        if (closestIndex < srcPlanet.crust_data.size()) {
            crust_data[i] = srcPlanet.crust_data[closestIndex];
        }

        
    }


}