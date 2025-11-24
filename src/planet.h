#pragma once

#include <memory>
#include <string>
#include <vector>

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
};

class Planet : public Mesh {
   public:
    std::vector<Plate> plates;
    std::vector<unsigned int> verticesToPlates;
    std::vector<std::unique_ptr<Crust>> crust_data;
    std::vector<std::vector<unsigned int>> neighbors;

    float radius = 1.0f;

    Planet(float r) : radius(r) {
        setupSphere(radius, 256, 128);
    }

    void generatePlates(unsigned int n_plates);
    void splitPlates();
    void assignCrustParameters();
    void printCrustAt(unsigned int vertex_index);

    void detectVerticesNeighbors();
    std::vector<Vec3> vertexColorsForPlates() const;
    std::vector<Vec3> vertexColorsForCrustTypes() const;
    std::vector<Vec3> vertexColorsForElevation() const;

    unsigned int findclosestVertex(const Vec3& point, Planet& srcPlanet);
    void resample(Planet& srcPlanet);
};
