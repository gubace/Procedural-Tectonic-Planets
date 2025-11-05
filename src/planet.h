
#include "Vec3.h"
#include "mesh.h"
#include <vector>

class Planet : public Mesh {
    public:
        std::vector<Plate> plates;
        float radius = 1.0f;
        Planet(float r) : radius(r) {
            setupSphere(radius, 64, 32);
            generatePlates(10);
        }

        generatePlates(unsigned int n_plates);
};

class Plate {
    public:
        std::vector<unsigned int> vertices_indices;
        Vec3 plate_velocity; //TODO Vec2
}