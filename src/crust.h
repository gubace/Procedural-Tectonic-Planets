#pragma once

#include "Vec3.h"
#include <iostream>

enum class CrustType {
    Oceanic,
    Continental
};

enum class OrogenyType {
    Collisional = 0,
    Subduction  = 1,
    Rifting     = 2,
    NoneType    = 3
};

inline const char* OrogenyTypeToString(OrogenyType t) {
    switch (t) {
        case OrogenyType::Collisional: return "collisional";
        case OrogenyType::Subduction:  return "subduction";
        case OrogenyType::Rifting:     return "rifting";
        case OrogenyType::NoneType:    return "none";
    }
    return "unknown";
}

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
    OrogenyType orogeny_type; // use enum instead of string
    Vec3 fold_dir;       // f

    ContinentalCrust(float thick, float elev, float orogeny_age_,
                    OrogenyType orogeny_type_, const Vec3& fold)
        : Crust(CrustType::Continental, thick, elev),
            orogeny_age(orogeny_age_), orogeny_type(orogeny_type_), fold_dir(fold) {}

    void printInfo() const override {
        std::cout << "Continental Crust | thickness=" << thickness
                << " relief=" << relief_elevation
                << " orogeny_age=" << orogeny_age
                << " type=" << OrogenyTypeToString(orogeny_type)
                << " fold_dir=" << fold_dir[0] << "," << fold_dir[1] << "," << fold_dir[2]
                << "\n";
    }
};