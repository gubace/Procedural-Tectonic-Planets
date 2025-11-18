#pragma once
#include <cstdint>
#include <string>


struct SubductionCandidate {
    enum class Type : uint8_t {
        Oceanic_Oceanic,
        Oceanic_Continental,
        Continental_Continental
    };

    // location (vertex index on the mesh
    unsigned int vertex_index = 0;

    // plates involved (under = plate that would subduct, over = overriding plate)
    unsigned int plate_under = 0;
    unsigned int plate_over = 0;

    // plate indices (both sides)
    unsigned int plate_a = 0;
    unsigned int plate_b = 0;

    // positive when A -> B converging (magnitude of convergence in world units)
    float convergence = 0.0f;

    Type type = Type::Oceanic_Continental;

    std::string reason;

};