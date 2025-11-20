#pragma once

#include <iostream>
#include <memory>
#include <vector>

#include "Vec3.h"
#include "planet.h"
#include "tectonicPhenomenon.h"

//---------------------------------------Movement Class--------------------------------------------
class Movement {
   public:
    Planet& planet;
    float movementAttenuation = 0.00001;

    Movement(Planet& p) : planet(p) {}

    void movePlates(float deltaTime);

    std::vector<std::unique_ptr<TectonicPhenomenon>> detectPhenomena(float convergenceThreshold = 0.001);

   private:
    void movePlate(Plate& plate, float deltaTime);
};