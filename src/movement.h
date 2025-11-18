#pragma once

#include "Vec3.h"
#include "planet.h"
#include <iostream>

//---------------------------------------Movement Class--------------------------------------------
class Movement {
    public:
        Planet& planet;
        float movementAttenuation = 0.0001;

        Movement(Planet& p) : planet(p) {}

        void movePlates(float deltaTime) {
            for (Plate& plate : planet.plates) {
                printf("plate_velocity : %f \n",plate.plate_velocity);
                movePlate(plate, deltaTime);
            }
        }
    
    private:
        void movePlate(Plate& plate, float deltaTime) {
            if (plate.plate_velocity == 0.0) {
                return;
            }
            
            for (int v = 0; v < plate.vertices_indices.size(); v++) {
                unsigned int vertexIndex = plate.vertices_indices[v];
                Vec3& vertexPos = planet.vertices[vertexIndex];

                // Calcul du déplacement circulaire autour de l'axe de rotation
                Vec3 toVertex = vertexPos; // assuming planet is centered at origin
                Vec3 axis = plate.rotation_axis;
                axis.normalize();

                // Angle de rotation basé sur la vitesse et le temps écoulé
                float angle = plate.plate_velocity * deltaTime * movementAttenuation;

                // Utilisation de la formule de Rodrigues pour la rotation
                Vec3 rotatedPos = toVertex * cos(angle) +
                                  Vec3::cross(axis, toVertex) * sin(angle) +
                                  axis * Vec3::dot(axis, toVertex) * (1 - cos(angle));

                // Mise à jour de la position du sommet
                vertexPos = rotatedPos;
            }
        }
};