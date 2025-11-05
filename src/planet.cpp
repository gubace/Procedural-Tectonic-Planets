#include "planet.h"


void Planet::generatePlates(unsigned int n_plates) {
    // Placeholder implementation: evenly distribute vertices among plates
    plates.clear();
    plates.resize(n_plates);

    //choose random vertices as seeds for plates
    std::vector<unsigned int> seed_indices;
    for (unsigned int i = 0; i < n_plates; ++i) {
        unsigned int seed = rand() % vertices.size();
        seed_indices.push_back(seed);
        plates[i].vertices_indices.push_back(seed);
    }


    

    for (unsigned int v = 0; v < vertices.size(); ++v) {
        if (std::find(seed_indices.begin(), seed_indices.end(), v) != seed_indices.end()) {
            continue; //skip seed vertices
        } else {
            //assign vertex to closest plate seed
            float min_dist = FLT_MAX;
            unsigned int closest_plate = 0;
            for (unsigned int p = 0; p < n_plates; ++p) {
                float dist = (vertices[v] - vertices[seed_indices[p]]).length();
                if (dist < min_dist) {
                    min_dist = dist;
                    closest_plate = p;
                }
            }
            plates[closest_plate].vertices_indices.push_back(v);
        }
    }
        
}