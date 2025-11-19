#include "movement.h"
#include "tectonicPhenomenon.h"
#include "planet.h"

#include <iostream>
#include <unordered_set>
#include <limits>
#include <algorithm>
#include <memory>
#include <vector>


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


std::vector<std::unique_ptr<TectonicPhenomenon>> Movement::detectPhenomena(float convergenceThreshold){
    std::vector<std::unique_ptr<TectonicPhenomenon>> out;
    if (planet.plates.empty() || planet.vertices.empty()) return out;

    // plate_of per-vertex
    std::vector<int> plate_of = assignPlatePerVertex(planet);

    std::vector<std::vector<unsigned int>> neighbors = buildVertexNeighbors(planet);

    size_t P = planet.plates.size();
    std::vector<Vec3> plate_centroid(P, Vec3(0.0f,0.0f,0.0f));
    std::vector<unsigned int> plate_count(P, 0u);
    populate_plate_centroids(plate_centroid, plate_count, P, planet);


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

            
            bool isOceanicA = false, isOceanicB = false;
            if (v < planet.crust_data.size() && planet.crust_data[v]) {
                isOceanicA = (dynamic_cast<OceanicCrust*>(planet.crust_data[v].get()) != nullptr);
            }
            if (nb < planet.crust_data.size() && planet.crust_data[nb]) {
                isOceanicB = (dynamic_cast<OceanicCrust*>(planet.crust_data[nb].get()) != nullptr);
            }

            // Détection de convergence (subduction ou collision)
            if (conv > convergenceThreshold) {
                // decide under/over plate according to rules
                unsigned int plate_under = pa, plate_over = pb;
                Subduction::SubductionType subType;
                bool isCollision = false;
                std::string reason;

                if (isOceanicA && isOceanicB) {
                    // older plate subducts
                    float ageA = plate_average_oceanic_age(pa);
                    float ageB = plate_average_oceanic_age(pb);
                    if (ageA > ageB) { 
                        plate_under = pa; plate_over = pb; 
                        reason = "oceanic-oceanic: older subducts"; 
                    } else { 
                        plate_under = pb; plate_over = pa; 
                        reason = "oceanic-oceanic: older subducts"; 
                    }
                    subType = Subduction::SubductionType::Oceanic_Oceanic;
                } else if (isOceanicA && !isOceanicB) {
                    plate_under = pa; plate_over = pb;
                    subType = Subduction::SubductionType::Oceanic_Continental;
                    reason = "oceanic under continental";
                } else if (!isOceanicA && isOceanicB) {
                    plate_under = pb; plate_over = pa;
                    subType = Subduction::SubductionType::Oceanic_Continental;
                    reason = "oceanic under continental";
                } else {
                    // continental-continental -> collision
                    isCollision = true;

                    float collisionMagnitude = conv;
                    reason = "continental-continental collision";
                    out.push_back(std::make_unique<ContinentalCollision>(
                        (unsigned int)pa, (unsigned int)pb, v,
                        collisionMagnitude, reason
                    ));
                }

                if (isCollision) continue;
                // Créer le phénomène de subduction
                out.push_back(std::make_unique<Subduction>(
                    (unsigned int)pa, (unsigned int)pb, v,
                    plate_under, plate_over, conv,
                    subType, reason
                ));
            }
            // Détection de divergence (rifting)
            else if (conv < -convergenceThreshold) {
                float divergence = std::abs(conv);
                std::string riftReason;
                
                if (isOceanicA && isOceanicB) {
                    riftReason = "oceanic-oceanic rifting: mid-ocean ridge";
                } else if (!isOceanicA && !isOceanicB) {
                    riftReason = "continental-continental rifting: continental rift zone";
                } else {
                    riftReason = "mixed rifting zone";
                }

                out.push_back(std::make_unique<Rifting>(
                    (unsigned int)pa, (unsigned int)pb, v,
                    divergence, riftReason
                ));
            }
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



