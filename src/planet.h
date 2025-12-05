#pragma once

#include <memory>
#include <string>
#include <vector>
#include <map> 

#include "Vec3.h"
#include "crust.h"
#include "mesh.h"
#include "tectonicPhenomenon.h"

//---------------------------------------Planet Class--------------------------------------------

class Plate {
   public:
    std::vector<unsigned int> vertices_indices;
    float plate_velocity;
    Vec3 rotation_axis;
    std::map<unsigned int, std::vector<unsigned int>> closestFrontierVertices;
    std::vector<std::vector<unsigned int>> terranes;
    std::vector<Vec3> terraneCentroids;

    void fillTerranes(const Planet& planet);
};

class Planet : public Mesh {
   public:
    std::vector<Plate> plates;
    std::vector<float> amplified_elevations;
    std::vector<unsigned int> verticesToPlates;
    std::vector<std::unique_ptr<Crust>> crust_data;
    std::vector<std::vector<unsigned int>> neighbors;

    float max_elevation = 8000.0f;
    float min_elevation = -8000.0f;
    float ocean_level = 0.5f;

    float radius = 1.0f;

    Planet(float r, int points) : radius(r) {
        setupSphere(radius, points);
    }

    void generatePlates(unsigned int n_plates);
    void findFrontierVertices();
    void fillClosestFrontierVertices();
    void assignCrustParameters();
    void printCrustAt(unsigned int vertex_index);

    void detectVerticesNeighbors();
    std::vector<Vec3> vertexColorsForPlates() const;
    std::vector<Vec3> vertexColorsForCrustTypes() const;
    std::vector<Vec3> vertexColorsForElevation() const;
    std::vector<Vec3> vertexColorsForCrustTypesAmplified() const;
    Vec3 getColorFromHeightAndCrustType(float elevation, bool isOceanic, float age) const;
    
    float computeAverageDistanceFromOrigin() const;
    float computeMinDistanceFromOrigin() const;
    float computeMaxDistanceFromOrigin() const;
    

    void fillAllTerranes();

    unsigned int findclosestVertex(const Vec3& point, Planet& srcPlanet);
    void resample(Planet& srcPlanet);
};
