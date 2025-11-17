#pragma once

#include "Vec3.h"
#include "mesh.h"
#include "crust.h"
#include <vector>
#include <memory>
#include <string>




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
    std::vector<std::unique_ptr<Crust>> crust_data;

    float radius = 1.0f;

    Planet(float r) : radius(r) {
        setupSphere(radius, 512, 256);
    }

    void generatePlates(unsigned int n_plates);
    void assignCrustParameters();
    void printCrustAt(unsigned int vertex_index);

    std::vector<Vec3> vertexColorsForPlates() const;
    std::vector<Vec3> vertexColorsForCrustTypes() const;
    std::vector<Vec3> vertexColorsForCrustAndPlateBoundaries() const;
};

