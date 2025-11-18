#include "movement.h"
#include "subduction.h"
#include "planet.h"

#include <iostream>
#include <unordered_set>
#include <limits>
#include <algorithm>


//utils =========================================
std::vector<int> assignPlatePerVertex(const Planet& planet) {
    std::vector<int> plate_of(planet.vertices.size(), -1);
    for (size_t p = 0; p < planet.plates.size(); ++p) {
        for (unsigned int vid : planet.plates[p].vertices_indices) {
            if (vid < plate_of.size()) plate_of[vid] = static_cast<int>(p);
        }
    }
    return plate_of;
}


std::vector<std::vector<unsigned int>> buildVertexNeighbors(const Planet& planet) {

    std::vector<std::vector<unsigned int>> neighbors(planet.vertices.size());
        for (const Triangle &t : planet.triangles) {
            unsigned int a = t[0], b = t[1], c = t[2];
            if (a < neighbors.size() && b < neighbors.size()) { neighbors[a].push_back(b); neighbors[b].push_back(a); }
            if (b < neighbors.size() && c < neighbors.size()) { neighbors[b].push_back(c); neighbors[c].push_back(b); }
            if (c < neighbors.size() && a < neighbors.size()) { neighbors[c].push_back(a); neighbors[a].push_back(c); }
        }
        for (auto &nb : neighbors) {
            std::sort(nb.begin(), nb.end());
            nb.erase(std::unique(nb.begin(), nb.end()), nb.end());
        }
    return neighbors;
}

void populate_plate_centroids(std::vector<Vec3> &plate_centroid, std::vector<unsigned int> &plate_count, size_t P, Planet& planet) {
        for (size_t p = 0; p < P; ++p) {
            for (unsigned int vid : planet.plates[p].vertices_indices) {
                if (vid < planet.vertices.size()) {
                    plate_centroid[p] += planet.vertices[vid];
                    plate_count[p] += 1;
                }
            }
            if (plate_count[p] > 0) {
                plate_centroid[p] /= (float)plate_count[p];
                plate_centroid[p].normalize();
                plate_centroid[p] *= planet.radius;
            }
        }
    }



//public ========================================

void Movement::movePlates(float deltaTime) {
            for (Plate& plate : planet.plates) {
                movePlate(plate, deltaTime);
            }
        }


std::vector<SubductionCandidate> Movement::detectPotentialSubductions(float convergenceThreshold){
    std::vector<SubductionCandidate> out;
    if (planet.plates.empty() || planet.vertices.empty()) return out;

    // --- helper: plate_of per-vertex ---
    std::vector<int> plate_of = assignPlatePerVertex(planet);

    // --- build vertex neighbours from triangles ---
    std::vector<std::vector<unsigned int>> neighbors = buildVertexNeighbors(planet);


    // --- plate centroids (approx) ---
    size_t P = planet.plates.size();
    std::vector<Vec3> plate_centroid(P, Vec3(0.0f,0.0f,0.0f));
    std::vector<unsigned int> plate_count(P, 0u);
    populate_plate_centroids(plate_centroid, plate_count, P, planet);
  

    // --- helper: average oceanic age for a plate (sample up to N vertices) ---
    auto plate_average_oceanic_age = [&](unsigned int pidx)->float {
        const auto &plate = planet.plates[pidx];
        float sum = 0.0f; unsigned int c = 0;
        for (unsigned int vid : plate.vertices_indices) {
            if (vid < planet.crust_data.size() && planet.crust_data[vid]) {
                auto oc = dynamic_cast<OceanicCrust*>(planet.crust_data[vid].get());
                if (oc) { sum += oc->age; ++c; }
            }
            if (c >= 50) break; // limit cost
        }
        return (c>0) ? (sum / (float)c) : 0.0f;
    };

    // --- iterate boundary vertex pairs (avoid duplicates) ---
    struct Key { unsigned int a,b,v; };
    struct KeyHash { size_t operator()(Key const& k) const noexcept { return (k.a*73856093u) ^ (k.b*19349663u) ^ (k.v*83492791u); } };
    struct KeyEq { bool operator()(Key const& x, Key const& y) const noexcept { return x.a==y.a && x.b==y.b && x.v==y.v; } };
    std::unordered_set<Key,KeyHash,KeyEq> seen;

    for (unsigned int v = 0; v < planet.vertices.size(); ++v) {
        int pa = plate_of[v];

        if (pa < 0) continue;

        for (unsigned int nb : neighbors[v]) {
            int pb = plate_of[nb];
            if (pb < 0 || pb == pa) continue;
            unsigned int A = (unsigned int)std::min(pa,pb);
            unsigned int B = (unsigned int)std::max(pa,pb);
            Key k{A,B,v};
            if (seen.find(k) != seen.end()) continue;
            seen.insert(k);

            // compute linear velocities at vertex for both plates
            Vec3 r = planet.vertices[v];
            // plate A
            const Plate &plateA = planet.plates[pa];
            Vec3 omegaA = plateA.rotation_axis;
            omegaA.normalize();
            omegaA *= plateA.plate_velocity;
            Vec3 vA = Vec3::cross(omegaA, r);
            // plate B
            const Plate &plateB = planet.plates[pb];
            Vec3 omegaB = plateB.rotation_axis;
            omegaB.normalize();
            omegaB *= plateB.plate_velocity;
            Vec3 vB = Vec3::cross(omegaB, r);

            // convergence direction (centroidB - centroidA)
            Vec3 dir = plate_centroid[pb] - plate_centroid[pa];
            float dirlen = dir.length();
            if (dirlen < 1e-8f) continue;
            dir /= dirlen;

            Vec3 rel = vA - vB;
            float conv = Vec3::dot(rel, dir); // positive => A towards B
            if (conv <= convergenceThreshold) continue;

            // determine crust types at the boundary: sample v for pa, nb for pb
            bool isOceanicA = false, isOceanicB = false;
            if (v < planet.crust_data.size() && planet.crust_data[v]) {
                isOceanicA = (dynamic_cast<OceanicCrust*>(planet.crust_data[v].get()) != nullptr);
            }
            if (nb < planet.crust_data.size() && planet.crust_data[nb]) {
                isOceanicB = (dynamic_cast<OceanicCrust*>(planet.crust_data[nb].get()) != nullptr);
            }

            // decide under/over plate according to rules
            unsigned int plate_under = pa, plate_over = pb;
            SubductionCandidate::Type candType = SubductionCandidate::Type::Oceanic_Continental;
            std::string reason;

            if (isOceanicA && isOceanicB) {
                // older plate subducts
                float ageA = plate_average_oceanic_age(pa);
                float ageB = plate_average_oceanic_age(pb);
                if (ageA > ageB) { plate_under = pa; plate_over = pb; reason = "oceanic-oceanic: older subducts"; }
                else { plate_under = pb; plate_over = pa; reason = "oceanic-oceanic: older subducts"; }
                candType = SubductionCandidate::Type::Oceanic_Oceanic;
            } else if (isOceanicA && !isOceanicB) {
                plate_under = pa; plate_over = pb;
                candType = SubductionCandidate::Type::Oceanic_Continental;
                reason = "oceanic under continental";
            } else if (!isOceanicA && isOceanicB) {
                plate_under = pb; plate_over = pa;
                candType = SubductionCandidate::Type::Oceanic_Continental;
                reason = "oceanic under continental";
            } else {
                // continental-continental -> forced/partial subduction (choose smaller plate as under)
                size_t sizeA = plate_count[pa], sizeB = plate_count[pb];
                if (sizeA < sizeB) { plate_under = pa; plate_over = pb; }
                else { plate_under = pb; plate_over = pa; }
                candType = SubductionCandidate::Type::Continental_Continental;
                reason = "continental-continental: forced subduction/collision";
            }

            // ensure the chosen under-plate is actually the one moving toward the other:
            float proj_under = 0.0f, proj_over = 0.0f;
            if (plate_under == (unsigned int)pa) {
                proj_under = Vec3::dot(vA, dir);
                proj_over  = Vec3::dot(vB, dir);
            } else {
                proj_under = Vec3::dot(vB, dir);
                proj_over  = Vec3::dot(vA, dir);
            }
            // if the selected under plate is not the one moving more toward the other, skip (not a real plunge)
            if (proj_under <= proj_over + 1e-6f) continue;

            SubductionCandidate sc;
            sc.vertex_index = v;
            sc.plate_a = (unsigned int)pa;
            sc.plate_b = (unsigned int)pb;
            sc.plate_under = plate_under;
            sc.plate_over = plate_over;
            sc.convergence = conv;
            sc.type = candType;
            sc.reason = reason;
            out.push_back(sc);
        }
    }

    return out;
}




//private ========================================
void Movement::movePlate(Plate& plate, float deltaTime) {
            if (plate.plate_velocity == 0.0) {
                return;
            }
            
            for (int v = 0; v < plate.vertices_indices.size(); v++) {
                unsigned int vertexIndex = plate.vertices_indices[v];
                Vec3& vertexPos = planet.vertices[vertexIndex];

                // Calcul du déplacement circulaire autour de l'axe de rotation
                Vec3 toVertex = vertexPos; // assuming planet is centered at origin
                Vec3 axis = plate.rotation_axis;
                axis.normalize();

                // Angle de rotation basé sur la vitesse et le temps écoulé
                float angle = plate.plate_velocity * deltaTime * movementAttenuation;

                // Utilisation de la formule de Rodrigues pour la rotation
                Vec3 rotatedPos = toVertex * cos(angle) +
                                  Vec3::cross(axis, toVertex) * sin(angle) +
                                  axis * Vec3::dot(axis, toVertex) * (1 - cos(angle));

                // Mise à jour de la position du sommet
                vertexPos = rotatedPos;
            }
        }



