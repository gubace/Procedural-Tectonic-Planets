
#include "Vec3.h"
#include "mesh.h"
#include <vector>
#include <memory>
#include <string>

enum class CrustType {
    Oceanic,
    Continental
};

//---------------------------------------Crust Class--------------------------------------------
class Crust {
    public:
        CrustType type;
        float thickness;         // e
        float relief_elevation;  // z

        Crust(CrustType t, float thick, float elev)
            : type(t), thickness(thick), relief_elevation(elev) {}

        virtual ~Crust() = default;
        virtual void printInfo() const = 0;

};

class OceanicCrust : public Crust {
public:
    float age;        // ao
    Vec3 ridge_dir;   // r

    OceanicCrust(float thick, float elev, float age_, const Vec3& ridge)
        : Crust(CrustType::Oceanic, thick, elev),
            age(age_), ridge_dir(ridge) {}

    void printInfo() const override {
        std::cout << "Oceanic Crust | thickness=" << thickness
                << " relief=" << relief_elevation
                << " age=" << age
                << " ridge_dir=" << ridge_dir[0] << "," << ridge_dir[1] << "," << ridge_dir[2]
                << "\n";
    }
};

class ContinentalCrust : public Crust {
public:
    float orogeny_age;   // ac
    std::string orogeny_type; // o (type d’orogénèse)
    Vec3 fold_dir;       // f

    ContinentalCrust(float thick, float elev, float orogeny_age_,
                    const std::string& orogeny_type_, const Vec3& fold)
        : Crust(CrustType::Continental, thick, elev),
            orogeny_age(orogeny_age_), orogeny_type(orogeny_type_), fold_dir(fold) {}

    void printInfo() const override {
        std::cout << "Continental Crust | thickness=" << thickness
                << " relief=" << relief_elevation
                << " orogeny_age=" << orogeny_age
                << " type=" << orogeny_type
                << " fold_dir=" << fold_dir[0] << "," << fold_dir[1] << "," << fold_dir[2]
                << "\n";
    }
};

//---------------------------------------Planet Class--------------------------------------------


class Plate {
    public:
        std::vector<unsigned int> vertices_indices;
        Vec3 plate_velocity; //TODO Vec2
};


class Planet : public Mesh {
public:
    std::vector<Plate> plates;
    std::vector<std::unique_ptr<Crust>> crust_data;

    float radius = 1.0f;

    Planet(float r) : radius(r) {
        setupSphere(radius, 256, 128);
    }

    void generatePlates(unsigned int n_plates);
    void assignCrustParameters();
    void printCrustAt(unsigned int vertex_index);

    std::vector<Vec3> vertexColorsForPlates() const;
    std::vector<Vec3> vertexColorsForCrustTypes() const;
    std::vector<Vec3> vertexColorsForCrustAndPlateBoundaries(float borderBlend = 1.0f, const Vec3& borderColor = Vec3(0.0f, 0.0f, 0.0f)) const;
};

