#include "movement.h"

#include <algorithm>
#include <iostream>
#include <limits>
#include <memory>
#include <unordered_set>
#include <vector>

#include "planet.h"

void populate_plate_centroids(std::vector<Vec3>& plate_centroid, std::vector<unsigned int>& plate_count, size_t P, Planet& planet) {
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

// public ========================================

void Movement::movePlates(float deltaTime) {
    for (Plate& plate : planet->plates) {
        movePlate(plate, deltaTime);
    }
    tectonicPhenomena = detectPhenomena();
    triggerEvents();
}

// private ========================================

std::vector<std::unique_ptr<TectonicPhenomenon>> Movement::detectPhenomena() {
    std::vector<std::unique_ptr<TectonicPhenomenon>> out;
    if (planet->plates.empty() || planet->vertices.empty()) return out;

    std::vector<std::vector<unsigned int>> neighbors = planet->neighbors;

    size_t P = planet->plates.size();
    std::vector<Vec3> plate_centroid(P, Vec3(0.0f, 0.0f, 0.0f));
    std::vector<unsigned int> plate_count(P, 0u);
    populate_plate_centroids(plate_centroid, plate_count, P, *planet);

    auto plate_average_oceanic_age = [&](unsigned int pidx) -> float {
        const auto& plate = planet->plates[pidx];
        float sum = 0.0f;
        unsigned int c = 0;
        for (unsigned int vid : plate.vertices_indices) {
            if (vid < planet->crust_data.size() && planet->crust_data[vid]) {
                auto oc = dynamic_cast<OceanicCrust*>(planet->crust_data[vid].get());
                if (oc) {
                    sum += oc->age;
                    ++c;
                }
            }
            if (c >= 50) break;
        }
        return (c > 0) ? (sum / (float)c) : 0.0f;
    };

    struct Key {
        unsigned int a, b, v;
    };
    struct KeyHash {
        size_t operator()(Key const& k) const noexcept { 
            return (k.a * 73856093u) ^ (k.b * 19349663u) ^ (k.v * 83492791u); 
        }
    };
    struct KeyEq {
        bool operator()(Key const& x, Key const& y) const noexcept { 
            return x.a == y.a && x.b == y.b && x.v == y.v; 
        }
    };
    std::unordered_set<Key, KeyHash, KeyEq> seen;

    // ✅ Nouveau: tracker les collisions déjà détectées par paire de plaques
    struct CollisionKey {
        unsigned int plateA, plateB;
        bool operator==(const CollisionKey& other) const {
            return (plateA == other.plateA && plateB == other.plateB) ||
                   (plateA == other.plateB && plateB == other.plateA);
        }
    };
    struct CollisionKeyHash {
        size_t operator()(const CollisionKey& k) const noexcept {
            unsigned int a = std::min(k.plateA, k.plateB);
            unsigned int b = std::max(k.plateA, k.plateB);
            return (a * 73856093u) ^ (b * 19349663u);
        }
    };
    std::unordered_set<CollisionKey, CollisionKeyHash> collisionsSeen;

    for (unsigned int v = 0; v < planet->vertices.size(); ++v) {
        int pa = planet->verticesToPlates[v];
        if (pa < 0) continue;

        for (unsigned int nb : neighbors[v]) {
            int pb = planet->verticesToPlates[nb];
            if (pb < 0 || pb == pa) continue;
            unsigned int A = (unsigned int)std::min(pa, pb);
            unsigned int B = (unsigned int)std::max(pa, pb);
            Key k{A, B, v};
            if (seen.find(k) != seen.end()) continue;
            seen.insert(k);

            Vec3 r = planet->vertices[v];

            const Plate& plateA = planet->plates[pa];
            Vec3 omegaA = plateA.rotation_axis;
            omegaA.normalize();
            omegaA *= plateA.plate_velocity;
            Vec3 vA = Vec3::cross(omegaA, r);

            const Plate& plateB = planet->plates[pb];
            Vec3 omegaB = plateB.rotation_axis;
            omegaB.normalize();
            omegaB *= plateB.plate_velocity;
            Vec3 vB = Vec3::cross(omegaB, r);

            Vec3 dir = plate_centroid[pb] - plate_centroid[pa];
            float dirlen = dir.length();
            if (dirlen < 1e-8f) continue;
            dir /= dirlen;

            Vec3 rel = vA - vB;
            float conv = Vec3::dot(rel, dir);

            bool isOceanicA = false, isOceanicB = false;
            if (v < planet->crust_data.size() && planet->crust_data[v]) {
                isOceanicA = (dynamic_cast<OceanicCrust*>(planet->crust_data[v].get()) != nullptr);
            }
            if (nb < planet->crust_data.size() && planet->crust_data[nb]) {
                isOceanicB = (dynamic_cast<OceanicCrust*>(planet->crust_data[nb].get()) != nullptr);
            }

            // Détection de convergence
            if (conv > convergenceThreshold) {
                unsigned int plate_under = pa, plate_over = pb;
                Subduction::SubductionType subType;
                bool isCollision = false;
                std::string reason;

                if (isOceanicA && isOceanicB) {
                    float ageA = plate_average_oceanic_age(pa);
                    float ageB = plate_average_oceanic_age(pb);
                    if (ageA > ageB) {
                        plate_under = pa;
                        plate_over = pb;
                    } else {
                        plate_under = pb;
                        plate_over = pa;
                    }
                    subType = Subduction::SubductionType::Oceanic_Oceanic;
                    reason = "oceanic-oceanic: older subducts";
                } else if (isOceanicA && !isOceanicB) {
                    plate_under = pa;
                    plate_over = pb;
                    subType = Subduction::SubductionType::Oceanic_Continental;
                    reason = "oceanic under continental";
                } else if (!isOceanicA && isOceanicB) {
                    plate_under = pb;
                    plate_over = pa;
                    subType = Subduction::SubductionType::Oceanic_Continental;
                    reason = "oceanic under continental";
                } else {
                    // ✅ Continental-continental collision
                    isCollision = true;
                    
                    // Vérifier si collision déjà détectée pour cette paire de plaques
                    CollisionKey collKey{(unsigned int)pa, (unsigned int)pb};
                    // if (collisionsSeen.find(collKey) == collisionsSeen.end()) {
                        collisionsSeen.insert(collKey);
                        
                        float collisionMagnitude = conv;
                        reason = "continental-continental collision";
                        
                        out.push_back(std::make_unique<ContinentalCollision>(
                            (unsigned int)pa, (unsigned int)pb, v,
                            collisionMagnitude, reason));
                        
                        // std::cout << "Collision detected between plates " << pa 
                        //           << " and " << pb << " at vertex " << v << std::endl;
                    // }
                }

                if (isCollision) continue;
                
                out.push_back(std::make_unique<Subduction>(
                    (unsigned int)pa, (unsigned int)pb, v,
                    plate_under, plate_over, conv,
                    subType, reason));
            }
            // Détection de divergence
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

                out.push_back(std::make_unique<crustGeneration>(
                    (unsigned int)pa, (unsigned int)pb, v,
                    divergence, riftReason));
            }
        }
    }

    return out;
}

void Movement::movePlate(Plate& plate, float deltaTime) {
    if (plate.plate_velocity == 0.0) {
        return;
    }

    for (int v = 0; v < plate.vertices_indices.size(); v++) {
        unsigned int vertexIndex = plate.vertices_indices[v];
        Vec3& vertexPos = planet->vertices[vertexIndex];

        // Calcul du déplacement circulaire autour de l'axe de rotation
        Vec3 toVertex = vertexPos;  // assuming planet is centered at origin
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

void Movement::triggerEvents() {
    for (const auto& phenomenon : tectonicPhenomena) {
        phenomenon->triggerEvent(*planet);
    }
}

void Movement::triggerTerranesMigration() {
    for (const auto& phenomenon : tectonicPhenomena) {
        if (phenomenon->getType() == TectonicPhenomenon::Type::ContinentalCollision) {
            ContinentalCollision* collisionPhenomenon = dynamic_cast<ContinentalCollision*>(phenomenon.get());
            collisionPhenomenon->triggerTerranesMigration(*planet);
        }
    }
}