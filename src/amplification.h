#pragma once

#include "Vec3.h"
#include "crust.h"
#include "planet.h"

#include "ShaderProgram.h"
#include "FastNoiseLite.h"
#include <GL/glew.h>
#include <algorithm>
#include <vector>

class Amplification {
public:
    const float elevation_force = 0.1;
    FastNoiseLite noise;
    ShaderProgram* tessellateShader;
    GLuint vao, vbo, ebo;
    GLuint transformFeedbackBuffer;
    
    Amplification(Planet & p) {
        noise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
        noise.SetFrequency(12.0f);
        noise.SetFractalType(FastNoiseLite::FractalType_FBm);
        noise.SetFractalOctaves(5);
        noise.SetSeed(2);

        

    };
    



    // Amplification CPU simple (subdivision manuelle)
    void amplifyTerrain(Planet& planet) {
        std::cout << "Starting terrain amplification (CPU)..." << std::endl;
        std::cout << "Original vertices: " << planet.vertices.size() << std::endl;
        std::cout << "Original triangles: " << planet.triangles.size() << std::endl;
        
        std::vector<Vec3> newVertices;
        std::vector<Vec3> newNormals;
        std::vector<Vec3> newColors;
        std::vector<Triangle> newTriangles;
        
        // Pour chaque triangle, créer 4 sous-triangles
        for (const auto& tri : planet.triangles) {
            Vec3 p0 = planet.vertices[tri.v[0]];
            Vec3 p1 = planet.vertices[tri.v[1]];
            Vec3 p2 = planet.vertices[tri.v[2]];
            
            Vec3 n0 = planet.normals[tri.v[0]];
            Vec3 n1 = planet.normals[tri.v[1]];
            Vec3 n2 = planet.normals[tri.v[2]];
            
            Vec3 c0 = planet.colors[tri.v[0]];
            Vec3 c1 = planet.colors[tri.v[1]];
            Vec3 c2 = planet.colors[tri.v[2]];
            
            // Calculer les points milieux sur la sphère
            Vec3 p01 = ((p0 + p1) * planet.radius);
            Vec3 p12 = ((p1 + p2) * planet.radius);
            Vec3 p20 = ((p2 + p0) * planet.radius);
            p01.normalize();
            p12.normalize();
            p20.normalize();
            // Normales interpolées
            Vec3 n01 = (n0 + n1);
            Vec3 n12 = (n1 + n2);
            Vec3 n20 = (n2 + n0);
            n01.normalize();
            n12.normalize();
            n20.normalize();
            
            // Couleurs interpolées
            Vec3 c01 = (c0 + c1) * 0.5f;
            Vec3 c12 = (c1 + c2) * 0.5f;
            Vec3 c20 = (c2 + c0) * 0.5f;
        
            
            unsigned int baseIdx = newVertices.size();
            
            // Ajouter les 6 vertices
            newVertices.push_back(p0);
            newVertices.push_back(p1);
            newVertices.push_back(p2);
            newVertices.push_back(p01);
            newVertices.push_back(p12);
            newVertices.push_back(p20);
            
            newNormals.push_back(n0);
            newNormals.push_back(n1);
            newNormals.push_back(n2);
            newNormals.push_back(n01);
            newNormals.push_back(n12);
            newNormals.push_back(n20);
            
            newColors.push_back(c0);
            newColors.push_back(c1);
            newColors.push_back(c2);
            newColors.push_back(c01);
            newColors.push_back(c12);
            newColors.push_back(c20);
            
            // Créer les 4 triangles
            // Triangle coin 0
            newTriangles.push_back(Triangle(baseIdx + 0, baseIdx + 3, baseIdx + 5));
            // Triangle coin 1
            newTriangles.push_back(Triangle(baseIdx + 1, baseIdx + 4, baseIdx + 3));
            // Triangle coin 2
            newTriangles.push_back(Triangle(baseIdx + 2, baseIdx + 5, baseIdx + 4));
            // Triangle central
            newTriangles.push_back(Triangle(baseIdx + 3, baseIdx + 4, baseIdx + 5));
        }
        
        // Remplacer les anciennes données
        planet.vertices = std::move(newVertices);
        planet.normals = std::move(newNormals);
        planet.colors = std::move(newColors);
        planet.triangles = std::move(newTriangles);
        
        // Réinitialiser les données de croûte
        planet.crust_data.clear();
        for (size_t i = 0; i < planet.vertices.size(); i++) {
            OceanicCrust* crust = new OceanicCrust(
                7.0f,
                planet.vertices[i].length() - planet.radius,
                0.0f,
                Vec3(0, 0, 0),
                false
            );
            planet.crust_data.push_back(std::unique_ptr<Crust>(crust));
        }
        
        // Réinitialiser verticesToPlates
        planet.verticesToPlates.clear();
        planet.verticesToPlates.resize(planet.vertices.size(), 0);
        
        std::cout << "After amplification vertices: " << planet.vertices.size() << std::endl;
        std::cout << "After amplification triangles: " << planet.triangles.size() << std::endl;
        
        // Recalculer les normales et structures
        planet.recomputeNormals();
        planet.detectVerticesNeighbors();
        planet.assignCrustParameters();
        
        std::cout << "Amplification complete!" << std::endl;
    };


private:


    Vec3 addNoise(Vec3 position) {
        float noiseRaw = noise.GetNoise(position[0], position[1], position[2]);
        float n = 1.0f + noiseRaw * (elevation_force * 0.05f);
        return position * n;
    }
};