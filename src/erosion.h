#pragma once


#include "crust.h"
#include "planet.h"


class Erosion {
   public:
    Planet& planet;

    static constexpr float erosion_coefficient = 30.0;
    static constexpr float dampening_coefficient = 0.05;
    static constexpr float sediment_coefficient = 0.05;

    float max_elevation = planet.max_elevation;
    float min_elevation = planet.min_elevation;

    Erosion(Planet & p) : planet(p) { }

    void erosion() {
        for (unsigned int vertexIdx = 0; vertexIdx < planet.vertices.size(); vertexIdx++) {
            CrustType crustType = planet.crust_data[vertexIdx]->type;
            if (crustType == CrustType::Continental) {
                planet.crust_data[vertexIdx]->relief_elevation = continentalErosion(planet.crust_data[vertexIdx]->relief_elevation);
            } else if (crustType == CrustType::Oceanic) {
                planet.crust_data[vertexIdx]->relief_elevation = oceaincDampening(planet.crust_data[vertexIdx]->relief_elevation);
            }
        }
    };

   private:
    float continentalErosion(float elevation) {
        return elevation - (elevation / max_elevation) * erosion_coefficient;
    };

    float oceaincDampening(float elevation) {
        return elevation -  (1 - (elevation / -min_elevation)) * dampening_coefficient;
    };
};