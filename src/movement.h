#pragma once

#include "Vec3.h"
#include "planet.h"
#include "subduction.h"

#include <iostream>


//---------------------------------------Movement Class--------------------------------------------
class Movement {
    public:
        Planet& planet;
        float movementAttenuation = 0.0001;

        Movement(Planet& p) : planet(p) {}

        void movePlates(float deltaTime);

        std::vector<SubductionCandidate> detectPotentialSubductions(float convergenceThreshold = 0.01f);
    
    private:
        void movePlate(Plate& plate, float deltaTime);
};