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
    const float elevation_force = 0.05f;
    const int amplification_quality = 4;
    FastNoiseLite general_noise;
    FastNoiseLite ground_noise;
    FastNoiseLite mountain_noise;
    SphericalKDTree accel;
    
    Amplification(Planet & p) : accel(p.vertices, p) {
        general_noise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
        general_noise.SetFrequency(2.0 * (float) amplification_quality);
        general_noise.SetFractalType(FastNoiseLite::FractalType_FBm);
        general_noise.SetFractalOctaves(2);
        general_noise.SetFractalLacunarity(2.0f);
        general_noise.SetFractalGain(0.5f);
        general_noise.SetSeed(2);

        ground_noise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
        ground_noise.SetFrequency(30.0f * (float) amplification_quality);
        ground_noise.SetFractalType(FastNoiseLite::FractalType_FBm);
        ground_noise.SetFractalOctaves(2);
        ground_noise.SetFractalLacunarity(2.0f);
        ground_noise.SetFractalGain(0.5f);
        ground_noise.SetSeed(4);

        mountain_noise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
        mountain_noise.SetFrequency(50.0f * (float) amplification_quality);
        mountain_noise.SetFractalType(FastNoiseLite::FractalType_FBm);
        mountain_noise.SetFractalOctaves(3);
        mountain_noise.SetFractalLacunarity(5.0f);
        mountain_noise.SetFractalGain(0.2f);
        mountain_noise.SetSeed(3);
    };
    

void amplifyTerrain(Planet& planet) {
    Planet newPlanet(1.0f, planet.vertices.size() * amplification_quality);

    for (int vertexIdx = 0; vertexIdx < newPlanet.vertices.size(); vertexIdx++) {
        newPlanet.vertices[vertexIdx] = copyClosestVertex(planet, newPlanet, vertexIdx);
    }
    newPlanet.detectVerticesNeighbors();
    planet = std::move(newPlanet);

    planet.detectVerticesNeighbors();
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
            

            if (normalized_elevation > 0.9f) {
                position = addNoiseToVertex(position, mountain_noise, 0.1f);
            } else if (normalized_elevation > 0.7f) {
                position = addNoiseToVertex(position, ground_noise, 0.05f);
            }

            position = addNoiseToVertex(position, general_noise, 0.04f);

            if (position.length() > planet.max_real_elevation) planet.max_real_elevation = position.length();
            if (position.length() < planet.min_real_elevation) planet.min_real_elevation = position.length();

            planet.vertices[vertexIdx] = position;
        }
    }

    Vec3 addNoiseToVertex(Vec3 position, FastNoiseLite noise, float strength, bool isOceanic = false) {
        float noiseRaw = noise.GetNoise(position[0], position[1], position[2]);
        float n = 1.0f + noiseRaw * elevation_force * strength;

        return position * n;
    }
};