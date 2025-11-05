#include "planet.h"
#include <random>
#include <iostream>

void Planet::generatePlates(unsigned int n_plates) {


    if (n_plates == 0) return;
    if (n_plates > vertices.size()) n_plates = vertices.size();
    plates.clear();
    plates.resize(n_plates);

    // ensure per-vertex color array exists
    colors.resize(vertices.size());

    // palette pour les plaques (évite de mélanger palette et couleurs de sommets)
    std::vector<Vec3> plate_colors(n_plates);

    //choose random vertices as seeds for plates
    std::vector<unsigned int> seed_indices;
    for (unsigned int i = 0; i < n_plates; ++i) {
        unsigned int seed = rand() % vertices.size();
        seed_indices.push_back(seed);

        float color = (i + 1) / (float) n_plates;
        plate_colors[i] = Vec3(color, 1.0f - color, color * 0.5f);

        // assign seed vertex the plate color
        colors[seed] = plate_colors[i];
        plates[i].vertices_indices.push_back(seed);
    }

    for (unsigned int v = 0; v < vertices.size(); ++v) {
        if (std::find(seed_indices.begin(), seed_indices.end(), v) != seed_indices.end()) {
            continue; //skip seed vertices (they're already colored)
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
            // assign vertex to plate and set its color from the palette
            plates[closest_plate].vertices_indices.push_back(v);
            colors[v] = plate_colors[closest_plate];
        }
    }
}