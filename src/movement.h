#pragma once

#include <iostream>
#include <memory>
#include <vector>
#include <unordered_set>

#include "Vec3.h"
#include "planet.h"
#include "tectonicPhenomenon.h"

// ===== Structures auxiliaires =====

struct PlateInteraction {
    unsigned int plateA;
    unsigned int plateB;
    unsigned int vertexIdx;
    unsigned int neighborIdx;
    
    float convergence;      // > 0 = convergence, < 0 = divergence
    
    bool isOceanicA;
    bool isOceanicB;
    
    float ageA;             // Âge moyen de la plaque A (si océanique)
    float ageB;             // Âge moyen de la plaque B (si océanique)
    
    Vec3 directionAtoB;     // Direction normalisée de A vers B
};

class PhenomenaDetectionCache {
private:
    struct EdgeKey {
        unsigned int a, b, v;
        bool operator==(const EdgeKey& other) const {
            return a == other.a && b == other.b && v == other.v;
        }
    };
    
    struct EdgeKeyHash {
        size_t operator()(const EdgeKey& k) const noexcept {
            return (k.a * 73856093u) ^ (k.b * 19349663u) ^ (k.v * 83492791u);
        }
    };
    
    std::unordered_set<EdgeKey, EdgeKeyHash> processedEdges;
    
public:
    bool alreadyProcessed(int plateA, int plateB, unsigned int vertex) {
        unsigned int minPlate = std::min((unsigned int)plateA, (unsigned int)plateB);
        unsigned int maxPlate = std::max((unsigned int)plateA, (unsigned int)plateB);
        EdgeKey key{minPlate, maxPlate, vertex};
        return !processedEdges.insert(key).second;
    }
    
};



//---------------------------------------Movement Class--------------------------------------------
class Movement {
   public:
    Planet* planet; //pointeur pour pouvoir copier
    float movementAttenuation = 0.005f;
    float convergenceThreshold = 0.00001f;

    std::vector<std::unique_ptr<TectonicPhenomenon>> tectonicPhenomena;
    
    Movement(Planet& p) : planet(&p) {}

    void movePlates(float deltaTime);
    void triggerTerranesMigration();
    std::vector<std::unique_ptr<TectonicPhenomenon>> detectPhenomena();

   private:

    void movePlate(Plate& plate, float deltaTime);
    void triggerEvents();
    
    std::vector<Vec3> computePlateCentroids() const;
    float computePlateAverageOceanicAge(unsigned int plateIdx) const;
    bool isOceanicCrust(unsigned int vertexIdx) const;
    Vec3 computePlateVelocity(const Plate& plate, const Vec3& position) const;
    
    PlateInteraction analyzePlateInteraction(
        int plateA, int plateB, 
        unsigned int vertexIdx, unsigned int neighborIdx,
        const std::vector<Vec3>& plateCentroids) const;
    
    std::unique_ptr<TectonicPhenomenon> createPhenomenon(
        const PlateInteraction& interaction,
        PhenomenaDetectionCache& cache);
    
    std::unique_ptr<TectonicPhenomenon> createConvergencePhenomenon(
        const PlateInteraction& interaction,
        PhenomenaDetectionCache& cache);
    
    std::unique_ptr<TectonicPhenomenon> createDivergencePhenomenon(
        const PlateInteraction& interaction);
};