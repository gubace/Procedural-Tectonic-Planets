#include "planet.h"
#include "FastNoiseLite.h"
#include <random>
#include <iostream>
#include <limits>
#include <unordered_set>
#include <memory>

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

    // === Initialisation du bruit ===
    FastNoiseLite noise;
    noise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    noise.SetFrequency(1.5f);
    noise.SetFractalType(FastNoiseLite::FractalType_FBm);
    noise.SetFractalOctaves(3);
    noise.SetSeed((int)std::random_device{}());

    plates.clear();
    plates.resize(n_plates);
    colors.resize(vertices.size());

    // RNG
    std::mt19937 rng((unsigned)std::random_device{}());
    std::uniform_int_distribution<unsigned int> uid(0, (unsigned int)vertices.size() - 1);

    // === Choix des graines (centroïdes initiaux) ===
    std::vector<unsigned int> seeds;
    seeds.reserve(n_plates);
    std::unordered_set<unsigned int> chosen;
    while (seeds.size() < n_plates) {
        unsigned int s = uid(rng);
        if (chosen.insert(s).second) seeds.push_back(s);
    }

    // === Centroids initiaux ===
    std::vector<Vec3> centroids;
    centroids.reserve(n_plates);
    for (unsigned int s : seeds)
        centroids.push_back(vertices[s]);

    // === Perturbation des centroïdes ===
    for (unsigned int k = 0; k < n_plates; ++k) {
        Vec3 normalized_c = centroids[k];
        normalized_c.normalize();
        Vec3 c = normalized_c;
        float n = noise.GetNoise(c[0] * 2.0f, c[1] * 2.0f, c[2] * 2.0f);

        // Crée une légère rotation / translation bruitée sur la sphère
        Vec3 offset = Vec3(
            c[0] + 0.15f * n * c[1],
            c[1] + 0.15f * n * c[2],
            c[2] + 0.15f * n * c[0]
        );
        offset.normalize();
        centroids[k] = offset * radius;
    }

    std::vector<int> assign(vertices.size(), -1);
    
    // === Étape d’assignation (Voronoï sphérique) ===
    const float jitterStrength = 0.06f; // Force de la perturbation
    
    for (size_t v = 0; v < vertices.size(); ++v) {
        float bestDist = std::numeric_limits<float>::infinity();
        int bestK = -1;
        Vec3 vNorm = vertices[v];
        vNorm.normalize();

        for (unsigned int k = 0; k < n_plates; ++k) {
            Vec3 cNorm = centroids[k];
            cNorm.normalize();
            
            // Distance angulaire de base
            float angle = std::acos(std::max(-1.0f, std::min(1.0f, Vec3::dot(vNorm,cNorm))));
            
            // Ajouter du bruit à la distance
            float n = noise.GetNoise(
                vNorm[0] * 2.0f + k * 1000.0f,  // Offset par plaque
                vNorm[1] * 2.0f + k * 1000.0f, 
                vNorm[2] * 2.0f + k * 1000.0f
            );
            
            // Distance perturbée
            float dist = angle + jitterStrength * n;
            
            if (dist < bestDist) {
                bestDist = dist;
                bestK = (int)k;
            }
        }
        assign[v] = bestK;
    }

    // === Mise à jour des centroïdes ===
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

    // === Construction des plaques et couleurs ===
    for (unsigned int k = 0; k < n_plates; ++k)
        plates[k].vertices_indices.clear();

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


void Planet::printCrustAt(unsigned int vertex_index) {
    if (vertex_index >= crust_data.size() || !crust_data[vertex_index]) return;
    crust_data[vertex_index]->printInfo();
}


void Planet::assignCrustParameters() {
    crust_data.resize(vertices.size());

    // --- Configuration du bruit global ---
    FastNoiseLite noise;
    noise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    noise.SetFrequency(0.8f / radius);
    noise.SetFractalType(FastNoiseLite::FractalType_FBm);
    noise.SetFractalOctaves(5);
    noise.SetFractalLacunarity(2.0f);
    noise.SetFractalGain(0.5f);
    noise.SetSeed((int)std::random_device{}());

    const float continent_threshold = 0.15f;

    for (size_t i = 0; i < vertices.size(); ++i) {
        const Vec3& p = vertices[i];
        float n = noise.GetNoise(p[0], p[1], p[2]);

        if (n < continent_threshold) { //océanie
            float elevation = n * 4000.0f; 
            float thickness = 7.0f + 2.0f * (n + 1.0f) * 0.5f; 
            crust_data[i].reset(new OceanicCrust(thickness, elevation, 0.0f, Vec3(0,0,1)));
        } 
        else { // continental
            float elevation = (n - continent_threshold) * 3000.0f; 
            float thickness = 30.0f + 10.0f * n;
            crust_data[i].reset(new ContinentalCrust(thickness, elevation, 0.0f, "none", Vec3(0,1,0)));
        }
    }
}




std::vector<Vec3> Planet::vertexColorsForPlates() const {
    std::vector<Vec3> out(vertices.size(), Vec3(0.5f, 0.5f, 0.5f));
    if (plates.empty()) return out;
    unsigned int n = (unsigned int)plates.size();
    for (unsigned int k = 0; k < n; ++k) {
        Vec3 col = hsv2rgb((k / (float)n), 0.7f, 0.85f);
        for (unsigned int idx : plates[k].vertices_indices) {
            if (idx < out.size()) out[idx] = col;
        }
    }
    return out;
}

std::vector<Vec3> Planet::vertexColorsForCrustTypes() const {
    std::vector<Vec3> out(vertices.size(), Vec3(0.5f, 0.5f, 0.5f));
    for (size_t i = 0; i < vertices.size(); ++i) {
        if (i < crust_data.size() && crust_data[i]) {
            if (crust_data[i]->type == CrustType::Oceanic) {
                out[i] = Vec3(0.1f, 0.3f, 0.85f); // bleu océanique
            } else {
                out[i] = Vec3(0.15f, 0.8f, 0.2f); // vert continental
            }
        } else {
            out[i] = Vec3(0.6f, 0.6f, 0.6f);
        }
    }
    return out;
}


std::vector<Vec3> Planet::vertexColorsForCrustAndPlateBoundaries() const {
    std::vector<Vec3> out = vertexColorsForCrustTypes();
    if (plates.empty()) return out;

    // Associer chaque sommet à une plaque
    std::vector<int> plate_of(vertices.size(), -1);
    for (size_t k = 0; k < plates.size(); ++k) {
        for (unsigned int idx : plates[k].vertices_indices) {
            if (idx < plate_of.size())
                plate_of[idx] = static_cast<int>(k);
        }
    }

    // Construire la liste des voisins (adjacence)
    std::vector<std::vector<unsigned int>> neighbors(vertices.size());
    for (const Triangle &t : triangles) {
        unsigned int a = t[0], b = t[1], c = t[2];
        if (a < vertices.size() && b < vertices.size()) { neighbors[a].push_back(b); neighbors[b].push_back(a); }
        if (b < vertices.size() && c < vertices.size()) { neighbors[b].push_back(c); neighbors[c].push_back(b); }
        if (c < vertices.size() && a < vertices.size()) { neighbors[c].push_back(a); neighbors[a].push_back(c); }
    }

    // Nettoyer les doublons
    for (auto &nb : neighbors) {
        std::sort(nb.begin(), nb.end());
        nb.erase(std::unique(nb.begin(), nb.end()), nb.end());
    }

    // Colorer directement les sommets appartenant à une frontière
    for (size_t i = 0; i < vertices.size(); ++i) {
        int plate = (i < plate_of.size() ? plate_of[i] : -1);
        if (plate < 0) continue;

        for (unsigned int j : neighbors[i]) {
            if (j < plate_of.size() && plate_of[j] != plate) {
                out[i] = Vec3(1.0,0.0,0.0);  // couleur unique pour les bords
                break;
            }
        }
    }

    return out;
}