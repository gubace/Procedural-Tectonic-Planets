

#include <iostream>
#include <limits>
#include <memory>
#include <random>
#include <algorithm>
#include <unordered_set>


#include "planet.h"
#include "FastNoiseLite.h"
#include "crust.h"
#include "SphericalGrid.h"
#include "util.h"



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


    std::vector<int> assign(vertices.size(), -1);

    // === Étape d’assignation (Voronoï sphérique) ===
    const float jitterStrength = 0.02f;  // Force de la perturbation

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
    findFrontierVertices();
    fillClosestFrontierVertices();

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

void Planet::findFrontierVertices() {
    for (unsigned int i = 0; i < vertices.size(); i++) {
        const std::vector<unsigned int>& vertexNeighbors = neighbors[i];
        unsigned int currentPlateIdx = verticesToPlates[i];

        for (unsigned int n = 0; n < vertexNeighbors.size(); n++) {
            unsigned int neighborIdx = vertexNeighbors[n];
            unsigned int neighborPlateIdx = verticesToPlates[neighborIdx];
            if (neighborPlateIdx != currentPlateIdx) {
                plates[currentPlateIdx].closestFrontierVertices[i] = std::vector<unsigned int>();
                break;
            }
        }
    }
}

void Planet::fillClosestFrontierVertices() { //TODO Optimiser cette fonction

    for (Plate& plate : plates) {
        std::vector<unsigned int> frontierVertices;
        for (const auto& pair : plate.closestFrontierVertices) {
            frontierVertices.push_back(pair.first);
        }

        if (frontierVertices.empty()) continue;
        std::map<unsigned int, std::vector<unsigned int>> newMapping;

        for (unsigned int vertexIdx : plate.vertices_indices) {
            float minDist = std::numeric_limits<float>::max();
            unsigned int closestFrontier = frontierVertices[0];

            for (unsigned int frontierIdx : frontierVertices) {
                float dist = (vertices[vertexIdx] - vertices[frontierIdx]).length();

                if (dist < minDist) {
                    minDist = dist;
                    closestFrontier = frontierIdx;
                }
            }
            newMapping[closestFrontier].push_back(vertexIdx);
        }
        plate.closestFrontierVertices = newMapping;
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

    const float continent_threshold = 0.05f;

    // ✅ Utiliser les constantes de la classe
    const float ocean_depth_range = std::abs(min_elevation);  // 8000.0f
    const float continent_height_range = max_elevation;        // 8000.0f

    // Build mapping vertex -> plate index (if plates exist)
    std::vector<int> plate_of(vertices.size(), -1);
    for (size_t p = 0; p < plates.size(); ++p) {
        for (unsigned int idx : plates[p].vertices_indices) {
            if (idx < plate_of.size()) plate_of[idx] = static_cast<int>(p);
        }
    }

    // build vertex neighbors (adjacency) to detect plate boundaries
    std::vector<std::vector<unsigned int>> neighbors_local(vertices.size());
    for (const Triangle& t : triangles) {
        unsigned int a = t[0], b = t[1], c = t[2];
        if (a < vertices.size() && b < vertices.size()) {
            neighbors_local[a].push_back(b);
            neighbors_local[b].push_back(a);
        }
        if (b < vertices.size() && c < vertices.size()) {
            neighbors_local[b].push_back(c);
            neighbors_local[c].push_back(b);
        }
        if (c < vertices.size() && a < vertices.size()) {
            neighbors_local[c].push_back(a);
            neighbors_local[a].push_back(c);
        }
    }
    for (auto& nb : neighbors_local) {
        std::sort(nb.begin(), nb.end());
        nb.erase(std::unique(nb.begin(), nb.end()), nb.end());
    }

    // iterate vertices and generate parameters
    for (size_t i = 0; i < vertices.size(); ++i) {
        const Vec3& p = vertices[i];

        // base noise value [-1, 1]
        float n = noise.GetNoise(p[0], p[1], p[2]);

        bool isBoundary = false;
        int myPlate = (i < plate_of.size() ? plate_of[i] : -1);
        if (myPlate >= 0) {
            for (unsigned int nb : neighbors_local[i]) {
                if (nb < plate_of.size() && plate_of[nb] != myPlate) {
                    isBoundary = true;
                    break;
                }
            }
        }

        if (n < continent_threshold) {

            
            float normalizedNoise = (n + 1.0f) / (1.0f + continent_threshold); // [0, 1]
            float elevation = min_elevation + normalizedNoise * (ocean_depth_range * 0.875f); // 87.5% de la profondeur max
            

            elevation = std::max(min_elevation, std::min(elevation, -500.0f));


            float thickness = 5.0f + 3.0f * normalizedNoise + (isBoundary ? 1.5f : 0.0f);

            float localNoise = noise.GetNoise(p[0] * 2.3f, p[1] * 1.7f, p[2] * 2.9f);
            float age = (0.5f * (localNoise + 1.0f)) * 200.0f;

            Vec3 ridge_dir = Vec3(0.0f, 0.0f, 0.0f);

            crust_data[i].reset(new OceanicCrust(thickness, elevation, age, ridge_dir));
            
        } else {
            
            float normalizedNoise = (n - continent_threshold) / (1.0f - continent_threshold); // [0, 1]
            float elevation = normalizedNoise * continent_height_range;
            


            
            elevation = std::max(0.0f, std::min(elevation, max_elevation));


            float thickness = 30.0f + 30.0f * normalizedNoise + (isBoundary ? 10.0f : 0.0f);

            float ageNoise = noise.GetNoise(p[0] * 1.2f + 10.0f, p[1] * 0.9f + 10.0f, p[2] * 1.7f + 10.0f);
            float orogeny_age = (0.5f * (ageNoise + 1.0f)) * 800.0f;
            
            float typeSample = noise.GetNoise(p[0] * 2.0f + 5.0f, p[1] * 1.3f + 5.0f, p[2] * 2.7f + 5.0f);
            int typeIdx = static_cast<int>(std::floor((0.5f * (typeSample + 1.0f)) * 4.0f));
            typeIdx = std::max(0, std::min(3, typeIdx));
            OrogenyType orogeny_type = static_cast<OrogenyType>(typeIdx);

            Vec3 fold_dir = Vec3(0.0f, 0.0f, 0.0f);

            if (noise.GetNoise(p[0] * 4.7f + 9.1f, p[1] * 3.3f + 8.2f, p[2] * 2.8f + 7.3f) < 0.0f) 
                fold_dir *= -1.0f;

            crust_data[i].reset(new ContinentalCrust(thickness, elevation, orogeny_age, orogeny_type, fold_dir));
        }
    }

    for (Plate& plate : plates) {
        plate.fillTerranes(*this);
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
    std::vector<Vec3> out(vertices.size(), Vec3(0.0f, 0.0f, 0.0f));

    float elevationRange = max_elevation - min_elevation;
    if (elevationRange < 1e-6f) elevationRange = 1.0f;

    auto clamp01 = [](float v) -> float { return std::max(0.0f, std::min(1.0f, v)); };

    for (size_t i = 0; i < vertices.size(); ++i) {
        if (i < crust_data.size() && crust_data[i]) {
            float t = (crust_data[i]->relief_elevation - min_elevation) / elevationRange;
            t = clamp01(t);

            // grayscale height map: 0 = black (min_elevation), 1 = white (max_elevation)
            out[i] = Vec3(t, t, t);
        } else {
            // no data -> black
            out[i] = Vec3(0.0f, 0.0f, 0.0f);
        }
    }

    return out;
};
    

Vec3 Planet::getColorFromHeightAndCrustType(float elevation, bool isOceanic, float age) const {
    auto clamp01 = [](float v) -> float { return std::max(0.0f, std::min(1.0f, v)); };
    auto mix = [](const Vec3& a, const Vec3& b, float t) -> Vec3 { return a * (1.0f - t) + b * t; };
    auto smooth = [&](float x) -> float { x = clamp01(x); return x * x * (3.0f - 2.0f * x); };

    if (isOceanic) {
        float t = clamp01((elevation - min_elevation) / std::abs(min_elevation));

        t = smooth(t);

        Vec3 col = mix(palette.color_deep, palette.color_shallow, t);

        float ageFactor = 1.0f - clamp01(age / 200.0f) * 0.45f;
        col *= ageFactor;

        return col;
    } else {
        float t = clamp01(elevation / max_elevation);

        Vec3 col;

        if (t < 0.3f) {
            float tt = smooth(t / 0.3f);
            col = mix(palette.color_lowland, palette.color_midland, tt);

        } else if (t < 0.6f) {
            float tt = smooth((t - 0.3f) / 0.3f);
            col = mix(palette.color_midland, palette.color_highland, tt);

        } else {
            float tt = smooth((t - 0.6f) / 0.4f);
            col = mix(palette.color_highland, palette.color_snow, tt);
        }

        col *= (1.0f - clamp01(age / 1200.0f) * 0.25f);

        return col;
    }
}

std::vector<Vec3> Planet::vertexColorsForCrustTypesAmplified() const {
    std::vector<Vec3> out(vertices.size(), Vec3(0.5f, 0.5f, 0.5f));
    if (amplified_elevations.empty()) {
        return out;
    }

    for (size_t i = 0; i < vertices.size(); ++i) {
        float height = amplified_elevations[i];

        float shoreWidth = 0.02f * max_elevation;
        float shoreT = (height - ocean_level) / shoreWidth;
        shoreT = std::max(0.0f, std::min(1.0f, shoreT));
        shoreT = shoreT * shoreT * (3 - 2*shoreT);

        Vec3 waterColor = getColorFromHeightAndCrustType(height, true, 0);
        Vec3 landColor  = getColorFromHeightAndCrustType(height, false, 0);

        out[i] = waterColor * (1.0f - shoreT) + landColor * shoreT;
    }
    return out;
}

std::vector<Vec3> Planet::vertexColorsForCrustTypesNormalized() const {
    std::vector<Vec3> out(vertices.size(), Vec3(0.5f, 0.5f, 0.5f));

    float minZ = min_real_elevation;
    float maxZ = max_real_elevation;
    float real_ocean_level = minZ + (maxZ - minZ) * ocean_level;

    float maxDelta = std::max(real_ocean_level - minZ, maxZ - real_ocean_level);
    if (maxDelta <= 0.0f) maxDelta = 1.0f;

    float low  = real_ocean_level - maxDelta;
    float high = real_ocean_level + maxDelta;

    for (size_t i = 0; i < vertices.size(); ++i) {
        float height = vertices[i].length();

        // Normalización centrada en real_ocean_level
        float normalized = (height - low) / (high - low);

        out[i] = palette.getColorFromValue(normalized);
    }
    return out;
}

void Planet::increaseWaterLevel() {
    ocean_level += 0.001;
    if (ocean_level >= 1.0) ocean_level = 0.0;

    std::cout << "Ocean level: " << ocean_level * 100 << "%" << std::endl;
}

void Planet::decreaseWaterLevel() {
    ocean_level -= 0.001;
    if (ocean_level <= 0.0) ocean_level = 1.0;

    std::cout << "Ocean level: " << ocean_level * 100 << "%" << std::endl;
}

std::vector<Vec3> Planet::vertexColorsForCrustTypes() const {
    std::vector<Vec3> out(vertices.size(), Vec3(0.5f, 0.5f, 0.5f));
    
    for (size_t i = 0; i < vertices.size(); ++i) {
        if (i >= crust_data.size() || !crust_data[i]) {
            out[i] = Vec3(0.6f, 0.6f, 0.6f);
            continue;
        }

        const OceanicCrust* oc = dynamic_cast<const OceanicCrust*>(crust_data[i].get());
        const ContinentalCrust* cc = dynamic_cast<const ContinentalCrust*>(crust_data[i].get());

        if (oc) {
            out[i] = getColorFromHeightAndCrustType(oc->relief_elevation, true, oc->age);
        } else if (cc) {
            out[i] = getColorFromHeightAndCrustType(cc->relief_elevation, false, cc->orogeny_age);
        } else {
            out[i] = Vec3(0.6f, 0.6f, 0.6f);
        }
    }
    return out;
}


void Planet::fillAllTerranes() {
    for (Plate& plate : plates) {
        plate.fillTerranes(*this);
    }
}

float Planet::computeAverageDistanceFromOrigin() const {
    if (vertices.empty()) {
        return 0.0f;
    }
    
    float totalDistance = 0.0f;
    
    for (const Vec3& vertex : vertices) {
        float distance = vertex.length();
        totalDistance += distance;
    }
    
    return totalDistance / static_cast<float>(vertices.size());
}

float Planet::computeMinDistanceFromOrigin() const {
    if (vertices.empty()) return 0.0f;
    
    float minDist = std::numeric_limits<float>::max();
    for (const Vec3& vertex : vertices) {
        float dist = vertex.length();
        if (dist < minDist) minDist = dist;
    }
    return minDist;
}

float Planet::computeMaxDistanceFromOrigin() const {
    if (vertices.empty()) return 0.0f;
    
    float maxDist = 0.0f;
    for (const Vec3& vertex : vertices) {
        float dist = vertex.length();
        if (dist > maxDist) maxDist = dist;
    }
    return maxDist;
} 

float Planet::relativeVelocity(Plate & plateA, Plate & plateB) {
    float v = std::abs(plateA.plate_velocity - plateB.plate_velocity);
    if (v < 0.0f) v = 0.0f;
    // clamp to max_velocity to keep g() in [0,1]
    if (v > max_velocity) v = max_velocity;
    return v;
}