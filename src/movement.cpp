#include "movement.h"

#include <algorithm>
#include <iostream>
#include <limits>
#include <memory>
#include <unordered_set>
#include <vector>

#include "planet.h"


// public ========================================

void Movement::movePlates(float deltaTime) {
    for (Plate& plate : planet->plates) {
        movePlate(plate, deltaTime);
    }
    tectonicPhenomena = detectPhenomena();
    triggerEvents();
}


void Movement::triggerTerranesMigration() {
    for (const auto& phenomenon : tectonicPhenomena) {
        if (phenomenon->getType() == TectonicPhenomenon::Type::ContinentalCollision) {
            ContinentalCollision* collisionPhenomenon = dynamic_cast<ContinentalCollision*>(phenomenon.get());
            collisionPhenomenon->triggerTerranesMigration(*planet);
        }
    }
}


std::vector<std::unique_ptr<TectonicPhenomenon>> Movement::detectPhenomena() {
    std::vector<std::unique_ptr<TectonicPhenomenon>> phenomena;

    if (planet->plates.empty() || planet->vertices.empty()) {
        return phenomena;
    }


    std::vector<Vec3> plateCentroids = computePlateCentroids();
    // le cache sert a eviter de detecter plusieurs fois la meme interaction
    PhenomenaDetectionCache cache;
    
    for (unsigned int vertexIdx = 0; vertexIdx < planet->vertices.size(); ++vertexIdx) {
        int plateA = planet->verticesToPlates[vertexIdx];
        if (plateA < 0) continue;
        
        for (unsigned int neighborIdx : planet->neighbors[vertexIdx]) {
            int plateB = planet->verticesToPlates[neighborIdx];
            if (plateB < 0 || plateB == plateA) continue;
            

            if (cache.alreadyProcessed(plateA, plateB, vertexIdx)) {
                continue;
            }
            
            // Analyser l'interaction entre les deux plaques
            PlateInteraction interaction = analyzePlateInteraction(
                plateA, plateB, vertexIdx, neighborIdx, plateCentroids
            );
            
            // Créer le phénomène approprié selon le type d'interaction
            auto phenomenon = createPhenomenon(interaction, cache);
            if (phenomenon) {
                phenomena.push_back(std::move(phenomenon));
            }
        }
    }
    
    return phenomena;
}

// private ========================================

std::vector<Vec3> Movement::computePlateCentroids() const {
    size_t numPlates = planet->plates.size();
    std::vector<Vec3> centroids(numPlates, Vec3(0.0f, 0.0f, 0.0f));
    std::vector<unsigned int> counts(numPlates, 0);
    
    for (size_t p = 0; p < numPlates; ++p) {
        for (unsigned int vidx : planet->plates[p].vertices_indices) {
            if (vidx < planet->vertices.size()) {
                centroids[p] += planet->vertices[vidx];
                counts[p]++;
            }
        }
        
        if (counts[p] > 0) {
            centroids[p] /= static_cast<float>(counts[p]);
            centroids[p].normalize();
            centroids[p] *= planet->radius;
        }
    }
    
    return centroids;
}

//dans le cas ou collision entre 2 oceans le plus vieux plonge
float Movement::computePlateAverageOceanicAge(unsigned int plateIdx) const {
    if (plateIdx >= planet->plates.size()) return 0.0f;
    
    const Plate& plate = planet->plates[plateIdx];
    float sumAge = 0.0f;
    unsigned int count = 0;
    
    const unsigned int maxSamples = 50;
    
    for (unsigned int vidx : plate.vertices_indices) {
        if (vidx >= planet->crust_data.size() || !planet->crust_data[vidx]) {
            continue;
        }
        
        auto* oceanicCrust = dynamic_cast<OceanicCrust*>(planet->crust_data[vidx].get());
        if (oceanicCrust) {
            sumAge += oceanicCrust->age;
            count++;
            
            if (count >= maxSamples) break;
        }
    }
    
    return (count > 0) ? (sumAge / static_cast<float>(count)) : 0.0f;
}


bool Movement::isOceanicCrust(unsigned int vertexIdx) const {
    if (vertexIdx >= planet->crust_data.size() || !planet->crust_data[vertexIdx]) {
        return false;
    }
    return dynamic_cast<OceanicCrust*>(planet->crust_data[vertexIdx].get()) != nullptr;
}


Vec3 Movement::computePlateVelocity(const Plate& plate, const Vec3& position) const {
    Vec3 omega = plate.rotation_axis;
    omega.normalize();
    omega *= plate.plate_velocity;
    return Vec3::cross(omega, position);
}


PlateInteraction Movement::analyzePlateInteraction(int plateA, int plateB, unsigned int vertexIdx, unsigned int neighborIdx, const std::vector<Vec3>& plateCentroids) const 
{
    PlateInteraction interaction;
    interaction.plateA = static_cast<unsigned int>(plateA);
    interaction.plateB = static_cast<unsigned int>(plateB);
    interaction.vertexIdx = vertexIdx;
    interaction.neighborIdx = neighborIdx;
    

    interaction.isOceanicA = isOceanicCrust(vertexIdx);
    interaction.isOceanicB = isOceanicCrust(neighborIdx);
    

    interaction.ageA = interaction.isOceanicA ? computePlateAverageOceanicAge(plateA) : 0.0f;
    interaction.ageB = interaction.isOceanicB ? computePlateAverageOceanicAge(plateB) : 0.0f;
    
    // Direction de A vers B
    interaction.directionAtoB = plateCentroids[plateB] - plateCentroids[plateA];
    float length = interaction.directionAtoB.length();
    if (length > 1e-8f) {
        interaction.directionAtoB /= length;
    } else {
        interaction.directionAtoB = Vec3(0.0f, 0.0f, 0.0f);
    }
    
    // Calculer les vitesses relatives
    Vec3 position = planet->vertices[vertexIdx];
    Vec3 velocityA = computePlateVelocity(planet->plates[plateA], position);
    Vec3 velocityB = computePlateVelocity(planet->plates[plateB], position);
    
    Vec3 relativeVelocity = velocityA - velocityB;
    interaction.convergence = Vec3::dot(relativeVelocity, interaction.directionAtoB);
    
    return interaction;
}





std::unique_ptr<TectonicPhenomenon> Movement::createConvergencePhenomenon(
    const PlateInteraction& interaction,
    PhenomenaDetectionCache& cache) 
{
    // ===== Cas 1: Continental-Continental = Collision =====
    if (!interaction.isOceanicA && !interaction.isOceanicB) {
        return std::make_unique<ContinentalCollision>(
            interaction.plateA,
            interaction.plateB,
            interaction.vertexIdx,
            interaction.convergence,
            "continental-continental collision"
        );
    }
    
    // ===== Cas 2: Oceanic-Oceanic = Subduction (le plus vieux plonge) =====
    if (interaction.isOceanicA && interaction.isOceanicB) {
        unsigned int plateUnder = (interaction.ageA > interaction.ageB) 
            ? interaction.plateA 
            : interaction.plateB;
        unsigned int plateOver = (interaction.ageA > interaction.ageB) 
            ? interaction.plateB 
            : interaction.plateA;
        
        return std::make_unique<Subduction>(
            interaction.plateA,
            interaction.plateB,
            interaction.vertexIdx,
            plateUnder,
            plateOver,
            interaction.convergence,
            Subduction::SubductionType::Oceanic_Oceanic,
            "oceanic-oceanic: older plate subducts"
        );
    }
    
    // ===== Cas 3: Mixed = Subduction (océanique plonge sous continental) ===== -> peut etre encore des bugs dans ce cas la
    unsigned int plateUnder = interaction.isOceanicA ? interaction.plateA : interaction.plateB;
    unsigned int plateOver = interaction.isOceanicA ? interaction.plateB : interaction.plateA;
    
    return std::make_unique<Subduction>(
        interaction.plateA,
        interaction.plateB,
        interaction.vertexIdx,
        plateUnder,
        plateOver,
        interaction.convergence,
        Subduction::SubductionType::Oceanic_Continental,
        "oceanic under continental"
    );
}


std::unique_ptr<TectonicPhenomenon> Movement::createDivergencePhenomenon(
    const PlateInteraction& interaction) 
{
    float divergence = std::abs(interaction.convergence);
    std::string reason;
    
    if (interaction.isOceanicA && interaction.isOceanicB) {
        reason = "oceanic-oceanic rifting: mid-ocean ridge";
    } else if (!interaction.isOceanicA && !interaction.isOceanicB) {
        reason = "continental-continental rifting: continental rift zone";
    } else {
        reason = "mixed rifting zone";
    }
    
    return std::make_unique<crustGeneration>(
        interaction.plateA,
        interaction.plateB,
        interaction.vertexIdx,
        divergence,
        reason
    );
}



std::unique_ptr<TectonicPhenomenon> Movement::createPhenomenon(
    const PlateInteraction& interaction,
    PhenomenaDetectionCache& cache) 
{
    // ===== CONVERGENCE (plaques se rapprochent) =====
    if (interaction.convergence > convergenceThreshold) {
        return createConvergencePhenomenon(interaction, cache);
    }
    
    // ===== DIVERGENCE (plaques s'éloignent) =====
    else if (interaction.convergence < -convergenceThreshold) {
        return createDivergencePhenomenon(interaction);
    }
    
    // Pas de phénomène significatif
    return nullptr;
}


void Movement::movePlate(Plate& plate, float deltaTime) {
    if (plate.plate_velocity == 0.0) {
        return;
    }
    for (int v = 0; v < plate.vertices_indices.size(); v++) {
        unsigned int vertexIndex = plate.vertices_indices[v];
        Vec3& vertexPos = planet->vertices[vertexIndex];

        Vec3 toVertex = vertexPos;
        Vec3 axis = plate.rotation_axis;
        axis.normalize();

        float angle = plate.plate_velocity * deltaTime * movementAttenuation;

        Vec3 rotatedPos = toVertex * cos(angle) +
                          Vec3::cross(axis, toVertex) * sin(angle) +
                          axis * Vec3::dot(axis, toVertex) * (1 - cos(angle));

        vertexPos = rotatedPos;
    }
}

void Movement::triggerEvents() {
    for (const auto& phenomenon : tectonicPhenomena) {
        phenomenon->triggerEvent(*planet);
    }
}