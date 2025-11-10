#include "planet.h"
#include "FastNoiseLite.h"
#include <random>
#include <iostream>
#include <limits>
#include <unordered_set>

//convert HSV->RGB 
static Vec3 hsv2rgb(float h, float s, float v) {
    float r = 0, g = 0, b = 0;
    if (s <= 0.0f) {
        r = g = b = v;
    } else {
        float hh = h * 6.0f;
        int i = (int)std::floor(hh);
        float f = hh - i;
        float p = v * (1.0f - s);
        float q = v * (1.0f - s * f);
        float t = v * (1.0f - s * (1.0f - f));
        switch (i % 6) {
            case 0: r = v; g = t; b = p; break;
            case 1: r = q; g = v; b = p; break;
            case 2: r = p; g = v; b = t; break;
            case 3: r = p; g = q; b = v; break;
            case 4: r = t; g = p; b = v; break;
            case 5: r = v; g = p; b = q; break;
        }
    }
    return Vec3(r*0.2, g*0.6, b);
}

void Planet::generatePlates(unsigned int n_plates) {
    if (n_plates == 0) return;
    if (vertices.empty()) return;
    if (n_plates > vertices.size()) n_plates = vertices.size();

    plates.clear();
    plates.resize(n_plates);
    colors.resize(vertices.size());

    // RNG
    std::mt19937 rng((unsigned)std::random_device{}());
    std::uniform_int_distribution<unsigned int> uid(0, (unsigned int)vertices.size() - 1);

    std::vector<unsigned int> seeds;
    seeds.reserve(n_plates);
    std::unordered_set<unsigned int> chosen;
    while (seeds.size() < n_plates) {
        unsigned int s = uid(rng);
        if (chosen.insert(s).second) seeds.push_back(s);
    }

    //centroids
    std::vector<Vec3> centroids;
    centroids.reserve(n_plates);
    for (unsigned int s : seeds) centroids.push_back(vertices[s]);


    
    std::vector<int> assign(vertices.size(), -1);
    
    // assignment step: each vertex to nearest centroid
    for (size_t v = 0; v < vertices.size(); ++v) {
        float bestDist = std::numeric_limits<float>::infinity();
        int bestK = -1;
        for (unsigned int k = 0; k < n_plates; ++k) {
            Vec3 d = vertices[v] - centroids[k];
            float dist = d.squareLength();
            if (dist < bestDist) {
                bestDist = dist;
                bestK = (int)k;
            }
        }
        assign[v] = bestK;
    }

    // update centroids as mean of assigned vertices, then project back to sphere
    std::vector<Vec3> newCentroid(n_plates, Vec3(0.0f, 0.0f, 0.0f));
    std::vector<unsigned int> counts(n_plates, 0u);
    for (size_t v = 0; v < vertices.size(); ++v) {
        int k = assign[v];
        if (k >= 0) {
            newCentroid[k] += vertices[v];
            counts[k] += 1;
        }
    }
    for (unsigned int k = 0; k < n_plates; ++k) {
        if (counts[k] > 0) {

            newCentroid[k] /= (float)counts[k];
            newCentroid[k].normalize();
            newCentroid[k] *= radius;
            centroids[k] = newCentroid[k];

        } else {
            
            unsigned int s = uid(rng);
            centroids[k] = vertices[s];
            
        }
    }
    

    // build plates from final assignment and create color palette
    for (unsigned int k = 0; k < n_plates; ++k) plates[k].vertices_indices.clear();

    std::vector<Vec3> plate_colors(n_plates);
    for (unsigned int k = 0; k < n_plates; ++k) {
        float hue = (k / (float)n_plates);
        plate_colors[k] = hsv2rgb(hue, 0.7f, 0.85f);
    }

    for (size_t v = 0; v < vertices.size(); ++v) {
        int k = assign[v];
        if (k < 0) k = 0;
        plates[k].vertices_indices.push_back((unsigned int)v);
        colors[v] = plate_colors[k];
    }
}