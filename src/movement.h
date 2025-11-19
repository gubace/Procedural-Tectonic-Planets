#pragma once

#include "Vec3.h"
#include "planet.h"
#include "tectonicPhenomenon.h"

#include <iostream>
#include <memory>
#include <vector>

//---------------------------------------Movement Class--------------------------------------------
class Movement {
    public:
        Planet& planet;
        float movementAttenuation = 0.0001;

        Movement(Planet& p) : planet(p) {}

        void movePlates(float deltaTime);

        std::vector<std::unique_ptr<TectonicPhenomenon>> detectPhenomena(float convergenceThreshold = 0.001);
    
    private:
        void movePlate(Plate& plate, float deltaTime);
};