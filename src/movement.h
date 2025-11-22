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

    Movement(Planet& p) : planet(p) {}

    void movePlates(float deltaTime);

    //TODO: Reemplazar todo este método:
    // Mejor detectar los bordes de las placas desde que se crea cada placa y en el remeshing
    // Y para detectar colisiones pues se revisan todos los vértices (optimizable) y se mira en borde de la placa para computar sus parámetros
    std::vector<std::unique_ptr<TectonicPhenomenon>> detectPhenomena();

   private:
    void movePlate(Plate& plate, float deltaTime);
};