#pragma once

#include "Vec3.h"
#include "crust.h"
#include "planet.h"

#include "SphericalGrid.h"
#include "FastNoiseLite.h"
#include <GL/glew.h>
#include <algorithm>
#include <vector>

class Amplification {
public:
    const float elevation_force = 0.15f;
    FastNoiseLite general_noise;
    FastNoiseLite mountain_noise;
    SphericalKDTree accel;
    
    Amplification(Planet & p) : accel(p.vertices, p) {
        general_noise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
        general_noise.SetFrequency(50.0f);
        general_noise.SetFractalType(FastNoiseLite::FractalType_FBm);
        general_noise.SetFractalOctaves(1);
        general_noise.SetSeed(2);

        mountain_noise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
        mountain_noise.SetFrequency(10.0f);
        mountain_noise.SetFractalType(FastNoiseLite::FractalType_FBm);
        mountain_noise.SetFractalOctaves(3);
        mountain_noise.SetFractalLacunarity(2.0f);
        mountain_noise.SetFractalGain(0.5f);
        mountain_noise.SetSeed(3);
    };
    
void amplifyTerrain(Planet& planet) {

    Planet newPlanet(1.0f, planet.vertices.size() * 5);

    for (int vertexIdx = 0; vertexIdx < newPlanet.vertices.size(); vertexIdx++) {
        newPlanet.vertices[vertexIdx] = copyClosestVertex(planet, newPlanet, vertexIdx);
    }
    newPlanet.detectVerticesNeighbors();
    planet = std::move(newPlanet);

    planet.detectVerticesNeighbors();
    planet.smooth();
    planet.smooth();
    planet.smooth();
    addNoise(planet);
    planet.recomputeNormals();

    std::cout << "Amplification complete!" << std::endl;
}

private:
    Vec3 copyClosestVertex(Planet& planet, Planet& newPlanet, unsigned int vertexIdx) {
        Vec3 vertexPosition = newPlanet.vertices[vertexIdx];
        unsigned int closestVertexIdx = accel.nearest(vertexPosition);

        float crust_elevation = planet.crust_data[closestVertexIdx]->relief_elevation;
        float normalized_elevation = (crust_elevation - planet.min_elevation) / (planet.max_elevation - planet.min_elevation);

        newPlanet.amplified_elevations.push_back(crust_elevation);
        Vec3 elevated_position = vertexPosition * (1 + elevation_force * normalized_elevation);
        return elevated_position;
    };

    void addNoise(Planet& planet) {
        for (unsigned int vertexIdx = 0; vertexIdx < planet.vertices.size(); vertexIdx++) {
            float elevation = planet.amplified_elevations[vertexIdx];
            float normalized_elevation = (elevation - planet.min_elevation) / (planet.max_elevation - planet.min_elevation);

            Vec3 position = planet.vertices[vertexIdx];

            // Ruido general
            position = addNoiseToVertex(position, general_noise, 1.0f, elevation);

            // Ruido de montaña, según elevación
            if (normalized_elevation > 0.7f) {
                position = addNoiseToVertex(position, mountain_noise, 0.03f, elevation);
            } else {
                position = addNoiseToVertex(position, mountain_noise, 0.02f, elevation);
            }

            // Actualizar el vértice
            planet.vertices[vertexIdx] = position;
        }
    }

    Vec3 addNoiseToVertex(Vec3 position, FastNoiseLite noise, float strength, float elevation) {
        float min_h = 0.0f;
        float max_h = 8000.0f;

        // Clamp and smooth
        float t = (elevation - min_h) / (max_h - min_h);
        t = std::max(0.0f, std::min(1.0f, t));
        float noiseFactor = t * t * (3.0f - 2.0f * t);

        float noiseRaw = noise.GetNoise(position[0], position[1], position[2]);

        float noiseValue = noiseRaw * (elevation_force * strength * noiseFactor);
        noiseValue = std::max(-0.05f, std::min(0.05f, noiseValue));
        float n = 1.0f + noiseValue;

        return position * n;
    }
};