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
    float convergenceThreshold = 0.00001;

    std::vector<std::unique_ptr<TectonicPhenomenon>> tectonicPhenomena;
    
    Movement(Planet& p) : planet(p) {}

    void movePlates(float deltaTime);

   private:
    void movePlate(Plate& plate, float deltaTime);
    void triggerEvents();
    std::vector<std::unique_ptr<TectonicPhenomenon>> detectPhenomena();
};