#pragma once

#include "Vec3.h"
#include "crust.h"
#include "planet.h"
#include "algorithm"

#include "FastNoiseLite.h"

class Amplification {
   public:
    const float elevation_force = 0.04;
    FastNoiseLite general_noise;
    FastNoiseLite mountain_noise;

    Amplification(Planet & p) {
        general_noise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
        general_noise.SetFrequency(10.0f);
        general_noise.SetFractalType(FastNoiseLite::FractalType_FBm);
        general_noise.SetFractalOctaves(1);
        general_noise.SetSeed(2);

        mountain_noise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
        mountain_noise.SetFrequency(40.0f);
        mountain_noise.SetFractalType(FastNoiseLite::FractalType_FBm);
        mountain_noise.SetFractalOctaves(1);
        mountain_noise.SetSeed(3);
    };

    void amplifyTerrain(Planet& planet) {
        Planet newPlanet(1.0f, planet.vertices.size() * 5);

        for (int vertexIdx = 0; vertexIdx < newPlanet.vertices.size(); vertexIdx++) {
            newPlanet.vertices[vertexIdx] = copyClosestVertex(planet, newPlanet, vertexIdx);
        }

        newPlanet.recomputeNormals();
        planet = std::move(newPlanet);
    };

   private:
    Vec3 copyClosestVertex(Planet& planet, Planet& newPlanet, unsigned int vertexIdx) {
        Vec3 vertexPosition = newPlanet.vertices[vertexIdx];
        unsigned int closestVertexIdx = 0;
        float closestDistance = planet.radius;

        for (int vertexIdx = 0; vertexIdx < planet.vertices.size(); vertexIdx++) {
            Vec3 difference = vertexPosition - planet.vertices[vertexIdx];
            float distance = difference.length();
            if (distance > closestDistance) {
                continue;
            }

            closestDistance = distance;
            closestVertexIdx = vertexIdx;
        }
        float crust_elevation = planet.crust_data[closestVertexIdx]->relief_elevation;
        float normalized_elevation = (crust_elevation - planet.min_elevation) / (planet.max_elevation - planet.min_elevation);

        newPlanet.amplified_elevations.push_back(crust_elevation);
        Vec3 elevated_position = vertexPosition * (1 + elevation_force * normalized_elevation);
        
        if (crust_elevation > 3000) {
            elevated_position = addNoise(elevated_position, mountain_noise);
        }
        // return elevated_position;
        return addNoise(elevated_position, general_noise);
    };

    Vec3 addNoise(Vec3 position, FastNoiseLite noise) {
        float noiseRaw = noise.GetNoise(position[0], position[1], position[2]);  // [-1, 1]

        float n = 1.0f + noiseRaw * (elevation_force * 0.05f);

        return position * n;
    }
};