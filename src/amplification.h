#pragma once

#include "Vec3.h"
#include "crust.h"
#include "planet.h"
#include "algorithm"

#include "FastNoiseLite.h"

class Amplification {
   public:
    const float elevation_force = 0.1;
    FastNoiseLite noise;

    Amplification(Planet & p) {
        noise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
        noise.SetFrequency(4.0f);
        noise.SetFractalType(FastNoiseLite::FractalType_FBm);
        noise.SetFractalOctaves(3);
        noise.SetSeed(2);
    };

    void amplifyTerrain(Planet& planet) {
        Planet newPlanet(1.0f, planet.vertices.size() * 4);

        for (int vertexIdx = 0; vertexIdx < newPlanet.vertices.size(); vertexIdx++) {
            newPlanet.vertices[vertexIdx] = copyClosestVertex(planet, newPlanet, vertexIdx);
        }

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
        return addNoise(elevated_position);
    };

    Vec3 addNoise(Vec3 position) {
        float noiseRaw = noise.GetNoise(position[0], position[1], position[2]);
        float n0 = (noiseRaw + 1.0f) * 0.5f;
        float n = 1.0f + n0 * (elevation_force / 2);

        return position * n;
    }
};