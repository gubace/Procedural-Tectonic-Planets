#include "planet.h"

#include <iostream>
#include <limits>
#include <memory>
#include <random>
#include <unordered_set>

#include "FastNoiseLite.h"
#include "crust.h"

// convert HSV->RGB
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
            case 0:
                r = v;
                g = t;
                b = p;
                break;
            case 1:
                r = q;
                g = v;
                b = p;
                break;
            case 2:
                r = p;
                g = v;
                b = t;
                break;
            case 3:
                r = p;
                g = q;
                b = v;
                break;
            case 4:
                r = t;
                g = p;
                b = v;
                break;
            case 5:
                r = v;
                g = p;
                b = q;
                break;
        }
    }
    return Vec3(r * 0.2, g * 0.6, b);
}

void Planet::detectVerticesNeighbors() {
    neighbors.resize(vertices.size());

    // Construire la liste des voisins (adjacence)
    for (const Triangle& t : triangles) {
        unsigned int a = t[0], b = t[1], c = t[2];
        if (a < vertices.size() && b < vertices.size()) {
            neighbors[a].push_back(b);
            neighbors[b].push_back(a);
        }
        if (b < vertices.size() && c < vertices.size()) {
            neighbors[b].push_back(c);
            neighbors[c].push_back(b);
        }
        if (c < vertices.size() && a < vertices.size()) {
            neighbors[c].push_back(a);
            neighbors[a].push_back(c);
        }
    }

    // Nettoyer les doublons
    for (auto& nb : neighbors) {
        std::sort(nb.begin(), nb.end());
        nb.erase(std::unique(nb.begin(), nb.end()), nb.end());
    }
}

void Planet::generatePlates(unsigned int n_plates) {
    if (n_plates == 0) return;
    if (vertices.empty()) return;
    if (n_plates > vertices.size()) n_plates = vertices.size();

    verticesToPlates.resize(vertices.size());

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
            c[2] + 0.15f * n * c[0]);
        offset.normalize();
        centroids[k] = offset * radius;
    }

    std::vector<int> assign(vertices.size(), -1);

    // === Étape d’assignation (Voronoï sphérique) ===
    const float jitterStrength = 0.06f;  // Force de la perturbation

    for (size_t v = 0; v < vertices.size(); ++v) {
        float bestDist = std::numeric_limits<float>::infinity();
        int bestK = -1;
        Vec3 vNorm = vertices[v];
        vNorm.normalize();

        for (unsigned int k = 0; k < n_plates; ++k) {
            Vec3 cNorm = centroids[k];
            cNorm.normalize();

            // Distance angulaire de base
            float angle = std::acos(std::max(-1.0f, std::min(1.0f, Vec3::dot(vNorm, cNorm))));

            // Ajouter du bruit à la distance
            float n = noise.GetNoise(
                vNorm[0] * 2.0f + k * 1000.0f,  // Offset par plaque
                vNorm[1] * 2.0f + k * 1000.0f,
                vNorm[2] * 2.0f + k * 1000.0f);

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
        verticesToPlates[(unsigned int)v] = k;
    }

    detectVerticesNeighbors();
    // splitPlates();

    std::uniform_real_distribution<float> dist01(0.1f, 0.9f);
    const float TWO_PI = 6.28318530717958647692f;
    for (int i = 0; i < n_plates; ++i) {
        Plate& plate = plates[i];
        plate.plate_velocity = dist01(rng);
        float z = 2.0f * dist01(rng) - 1.0f;
        float theta = TWO_PI * dist01(rng);
        float rxy = std::sqrt(std::max(0.0f, 1.0f - z * z));
        plate.rotation_axis = Vec3(rxy * std::cos(theta), rxy * std::sin(theta), z);
    }
}

void Planet::splitPlates() {
    for (int i = (int)triangles.size() - 1; i >= 0; i--) {
        Triangle t = triangles[i];
        unsigned int vIdx0 = t.v[0];
        unsigned int plateV0 = verticesToPlates[vIdx0];

        unsigned int vIdx1 = t.v[1];
        unsigned int plateV1 = verticesToPlates[vIdx1];

        unsigned int vIdx2 = t.v[2];
        unsigned int plateV2 = verticesToPlates[vIdx2];

        if (plateV0 != plateV1 || plateV0 != plateV2 || plateV1 != plateV2) {
            removeTriangle(i);
        }
    }
}

void Planet::printCrustAt(unsigned int vertex_index) {
    if (vertex_index >= crust_data.size() || !crust_data[vertex_index]) return;
    crust_data[vertex_index]->printInfo();
}

void Planet::assignCrustParameters() {
    crust_data.resize(vertices.size());

    // bruit
    FastNoiseLite noise;
    noise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    noise.SetFrequency(0.8f / radius);
    noise.SetFractalType(FastNoiseLite::FractalType_FBm);
    noise.SetFractalOctaves(5);
    noise.SetFractalLacunarity(2.0f);
    noise.SetFractalGain(0.5f);
    noise.SetSeed((int)std::random_device{}());

    const float continent_threshold = 0.15f;

    // Build mapping vertex -> plate index (if plates exist)
    std::vector<int> plate_of(vertices.size(), -1);
    for (size_t p = 0; p < plates.size(); ++p) {
        for (unsigned int idx : plates[p].vertices_indices) {
            if (idx < plate_of.size()) plate_of[idx] = static_cast<int>(p);
        }
    }

    // build vertex neighbors (adjacency) to detect plate boundaries
    std::vector<std::vector<unsigned int>> neighbors(vertices.size());
    for (const Triangle& t : triangles) {
        unsigned int a = t[0], b = t[1], c = t[2];
        if (a < vertices.size() && b < vertices.size()) {
            neighbors[a].push_back(b);
            neighbors[b].push_back(a);
        }
        if (b < vertices.size() && c < vertices.size()) {
            neighbors[b].push_back(c);
            neighbors[c].push_back(b);
        }
        if (c < vertices.size() && a < vertices.size()) {
            neighbors[c].push_back(a);
            neighbors[a].push_back(c);
        }
    }
    for (auto& nb : neighbors) {
        std::sort(nb.begin(), nb.end());
        nb.erase(std::unique(nb.begin(), nb.end()), nb.end());
    }

    // iterate vertices and generate parameters
    for (size_t i = 0; i < vertices.size(); ++i) {
        const Vec3& p = vertices[i];

        // base noise value
        float n = noise.GetNoise(p[0], p[1], p[2]);

        bool isBoundary = false;
        int myPlate = (i < plate_of.size() ? plate_of[i] : -1);
        if (myPlate >= 0) {
            for (unsigned int nb : neighbors[i]) {
                if (nb < plate_of.size() && plate_of[nb] != myPlate) {
                    isBoundary = true;
                    break;
                }
            }
        }

        if (n < continent_threshold) {  // oceanic

            float elevation = 0.0f;
            float thickness = 7.0f + 2.0f * (n + 1.0f) * 0.5f + (isBoundary ? 1.0f : 0.0f);

            // age
            float localNoise = noise.GetNoise(p[0] * 2.3f, p[1] * 1.7f, p[2] * 2.9f);
            float age = (0.5f * (localNoise + 1.0f)) * 200.0f;
            if (isBoundary) age *= 0.2f;

            Vec3 ridge_dir = Vec3(0.0f, 0.0f, 0.0f);  // TODO : compute ridge direction properly

            crust_data[i].reset(new OceanicCrust(thickness, elevation, age, ridge_dir));
        } else {  // continental

            float elevation = (n - continent_threshold) * 3000.0f;
            float thickness = 30.0f + 10.0f * n + (isBoundary ? 2.0f : 0.0f);

            float ageNoise = noise.GetNoise(p[0] * 1.2f + 10.0f, p[1] * 0.9f + 10.0f, p[2] * 1.7f + 10.0f);
            float orogeny_age = (0.5f * (ageNoise + 1.0f)) * 800.0f;
            if (!isBoundary) orogeny_age *= 1.2f;

            // orogeny type chosen from noise sample
            float typeSample = noise.GetNoise(p[0] * 2.0f + 5.0f, p[1] * 1.3f + 5.0f, p[2] * 2.7f + 5.0f);
            int typeIdx = static_cast<int>(std::floor((0.5f * (typeSample + 1.0f)) * 4.0f));
            typeIdx = std::max(0, std::min(3, typeIdx));
            OrogenyType orogeny_type = static_cast<OrogenyType>(typeIdx);

            Vec3 fold_dir = Vec3(0.0f, 0.0f, 0.0f);  // TODO: compute proper tangent

            if (noise.GetNoise(p[0] * 4.7f + 9.1f, p[1] * 3.3f + 8.2f, p[2] * 2.8f + 7.3f) < 0.0f) fold_dir *= -1.0f;

            crust_data[i].reset(new ContinentalCrust(thickness, elevation, orogeny_age, orogeny_type, fold_dir));
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


std::vector<Vec3> Planet::vertexColorsForElevation() const {
    std::vector<Vec3> out(vertices.size(), Vec3(0.0f, 0.0f, 0.0f)); // Initialisation sombre par défaut

    // Trouver les altitudes minimale et maximale pour normaliser les couleurs
    float minElevation = -8000.0f;
    float maxElevation = 8000.0f;

    for (size_t i = 0; i < vertices.size(); ++i) {
        if (i < crust_data.size() && crust_data[i]) {
            const Crust* crust = crust_data[i].get();
            minElevation = std::min(minElevation, crust->relief_elevation);
            maxElevation = std::max(maxElevation, crust->relief_elevation);
        }
    }

    // Éviter les divisions par zéro si toutes les altitudes sont identiques
    float elevationRange = maxElevation - minElevation;
    if (elevationRange < 1e-6f) elevationRange = 1.0f;

    // Générer les couleurs en fonction de l'altitude
    for (size_t i = 0; i < vertices.size(); ++i) {
        if (i < crust_data.size() && crust_data[i]) {
            const Crust* crust = crust_data[i].get();
            float normalizedElevation = (crust->relief_elevation - minElevation) / elevationRange;
            out[i] = Vec3(normalizedElevation, normalizedElevation, normalizedElevation); // Gris en fonction de l'altitude
        } else {
            out[i] = Vec3(0.0f, 0.0f, 0.0f); // Noir si pas de données de croûte
        }
    }

    return out;
}

std::vector<Vec3> Planet::vertexColorsForCrustTypes() const {
    std::vector<Vec3> out(vertices.size(), Vec3(0.5f, 0.5f, 0.5f));
    for (size_t i = 0; i < vertices.size(); ++i) {
        if (i >= crust_data.size() || !crust_data[i]) {
            out[i] = Vec3(0.6f, 0.6f, 0.6f);
            std::cout << "No crust data at vertex " << i << "\n"<<std::endl;
            continue;
            
        }

        // Dynamic cast to identify crust type
        const OceanicCrust* oc = dynamic_cast<const OceanicCrust*>(crust_data[i].get());
        const ContinentalCrust* cc = dynamic_cast<const ContinentalCrust*>(crust_data[i].get());

        auto clamp01 = [](float v) -> float { return std::max(0.0f, std::min(1.0f, v)); };
        auto mix = [](const Vec3& a, const Vec3& b, float t) -> Vec3 { return a * (1.0f - t) + b * t; };

        //std::cout << "Crust at vertex " << i << ": "<<std::endl;

        if (oc) {
            // Oceanic : base blue, vary with elevation (depth -> darker) and age (older -> darker/desaturated)
            float elev = oc->relief_elevation;
            float t = clamp01((elev + 4000.0f) / 6000.0f);
            Vec3 deepBlue(0.02f, 0.05f, 0.40f);
            Vec3 shallowBlue(0.12f, 0.45f, 0.8f);
            Vec3 col = mix(deepBlue, shallowBlue, t);

            float age = oc->age;
            float ageFactor = 1.0f - clamp01(age / 200.0f) * 0.45f;
            col *= ageFactor;

            out[i] = col;
        } else if (cc) {
            // Continental : vert -> marron -> blanc
            float elev = cc->relief_elevation * 3.0f;  // multicateur arbitraire
            // map elevation 0..4000
            float t = clamp01(elev / 4000.0f);
            Vec3 lowland(0.15f, 0.7f, 0.18f);
            Vec3 mountain(0.45f, 0.30f, 0.10f);
            Vec3 col = mix(lowland, mountain, t);

            if (elev > 3000.0f) {
                float snow = clamp01((elev - 2500.0f) / 1500.0f);
                col = mix(col, Vec3(1.0f, 1.0f, 1.0f), snow);
            }

            // small modulation by orogeny_age (older -> slightly darker)
            float oa = cc->orogeny_age;  // in Myr
            col *= (1.0f - clamp01(oa / 1200.0f) * 0.25f);

            out[i] = col;
        } else {
            out[i] = Vec3(0.6f, 0.6f, 0.6f);
        }
    }
    return out;
}
