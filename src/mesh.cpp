#include "mesh.h"
#include "SphericalGrid.h"

#include <map>
#include <cmath>
#include <set>
#include <limits>
#include <algorithm>



void Mesh::setupSphere(float radius, unsigned int numPoints) {
    vertices.clear();
    normals.clear();
    triangles.clear();
    triangle_normals.clear();
    
    if (numPoints < 4) numPoints = 4;
    
    const float PI = 3.14159265358979323846f;
    const float PHI = (1.0f + std::sqrt(5.0f)) / 2.0f;
    
    //Generate Fibonacci sphere points
    std::cout << "Generating " << numPoints << " Fibonacci points..." << std::endl;
    for (unsigned int i = 0; i < numPoints; ++i) {
        float y = 1.0f - (2.0f * i) / (numPoints - 1.0f);
        float radiusAtY = std::sqrt(1.0f - y * y);
        const float goldenAngle = 2.0f * PI * (1.0f - 1.0f / PHI);
        float theta = goldenAngle * i;
        
        float x = radiusAtY * std::cos(theta);
        float z = radiusAtY * std::sin(theta);
        
        Vec3 pos(x * radius, y * radius, z * radius);
        vertices.push_back(pos);
        
        Vec3 normal(x, y, z);
        normal.normalize();
        normals.push_back(normal);
    }
    
    
    unsigned int northPole = 0;
    unsigned int southPole = numPoints - 1;
    
    std::cout << "Building neighbor graph..." << std::endl;
    
    const unsigned int K = 8; // Ne pas toucher ce paramètre
    std::vector<std::set<unsigned int>> adjacency(numPoints);
    
    const unsigned int gridRes = std::max(32u, (unsigned int)std::sqrt(numPoints) / 2);
    std::vector<std::vector<unsigned int>> spatialGrid(gridRes * gridRes);
    

    for (unsigned int i = 0; i < numPoints; ++i) {
        Vec3 p = vertices[i] / vertices[i].length();
        float theta = std::atan2(p[2], p[0]);
        float phi   = std::acos(p[1]);
        
        int u = (int)((theta + PI) / (2.0f * PI) * gridRes) % gridRes;
        int v = (int)(phi / PI * gridRes);
        if (v >= (int)gridRes) v = gridRes - 1;
        
        spatialGrid[v * gridRes + u].push_back(i);
    }
    

    for (unsigned int i = 0; i < numPoints; ++i) {
        Vec3 p = vertices[i] / vertices[i].length();
        float theta = std::atan2(p[2], p[0]);
        float phi   = std::acos(p[1]);
        
        int u = (int)((theta + PI) / (2.0f * PI) * gridRes) % gridRes;
        int v = (int)(phi / PI * gridRes);
        if (v >= (int)gridRes) v = gridRes - 1;
        
        std::vector<std::pair<float, unsigned int>> candidates;
        

        int searchRadius = 2;
        float absLat = std::abs(p[1]);
        if (absLat > 0.98f) searchRadius = 8;
        else if (absLat > 0.95f) searchRadius = 6;
        else if (absLat > 0.90f) searchRadius = 5;
        else if (absLat > 0.80f) searchRadius = 4;
        
        // Search in adaptive neighborhood
        for (int dv = -searchRadius; dv <= searchRadius; ++dv) {
            for (int du = -searchRadius; du <= searchRadius; ++du) {
                int nu = (u + du + gridRes) % gridRes;
                int nv = v + dv;
                if (nv < 0 || nv >= (int)gridRes) continue;
                
                for (unsigned int j : spatialGrid[nv * gridRes + nu]) {
                    if (i == j) continue;
                    float dist = (vertices[i] - vertices[j]).squareLength();
                    candidates.push_back({dist, j});
                }
            }
        }
        

        unsigned int kNeighbors = K;
        
        if (candidates.size() > 0) {
            std::partial_sort(candidates.begin(), 
                             candidates.begin() + std::min(kNeighbors, (unsigned int)candidates.size()),
                             candidates.end());
            
            for (unsigned int k = 0; k < std::min(kNeighbors, (unsigned int)candidates.size()); ++k) {
                unsigned int j = candidates[k].second;
                adjacency[i].insert(j);
                adjacency[j].insert(i);
            }
        }
    }
    

    auto connectPoleToRing = [&](unsigned int pole) {
        std::vector<std::pair<float, unsigned int>> nearPole;
        
        for (unsigned int i = 0; i < numPoints; ++i) {
            if (i == pole) continue;
            float dist = (vertices[i] - vertices[pole]).squareLength();
            nearPole.push_back({dist, i});
        }
        
        std::sort(nearPole.begin(), nearPole.end());
        
        unsigned int ringSize = std::min(8u, (unsigned int)nearPole.size());
        std::vector<unsigned int> ring;
        
        for (unsigned int k = 0; k < ringSize; ++k) {
            unsigned int neighbor = nearPole[k].second;
            adjacency[pole].insert(neighbor);
            adjacency[neighbor].insert(pole);
            ring.push_back(neighbor);
        }
        

        Vec3 polePos = vertices[pole];
        Vec3 poleNormal = polePos / polePos.length();
        

        Vec3 refVec = (std::abs(poleNormal[1]) < 0.9f) ? Vec3(0, 1, 0) : Vec3(1, 0, 0);
        Vec3 tangent = Vec3::cross(poleNormal, refVec);
        tangent.normalize();
        Vec3 bitangent = Vec3::cross(poleNormal, tangent);
        
        std::vector<std::pair<float, unsigned int>> angledRing;
        for (unsigned int idx : ring) {
            Vec3 toNeighbor = vertices[idx] - polePos;
            float x = Vec3::dot(toNeighbor, tangent);
            float y = Vec3::dot(toNeighbor, bitangent);
            float angle = std::atan2(y, x);
            angledRing.push_back({angle, idx});
        }
        
        std::sort(angledRing.begin(), angledRing.end());
        

        for (size_t i = 0; i < angledRing.size(); ++i) {
            unsigned int curr = angledRing[i].second;
            unsigned int next = angledRing[(i + 1) % angledRing.size()].second;
            adjacency[curr].insert(next);
            adjacency[next].insert(curr);
        }
    };
    
    std::cout << "Connecting poles..." << std::endl;
    connectPoleToRing(northPole);
    connectPoleToRing(southPole);
    

    std::cout << "Building triangles..." << std::endl;
    std::set<std::tuple<unsigned int, unsigned int, unsigned int>> triangle_set;
    
    for (unsigned int v0 = 0; v0 < numPoints; ++v0) {
        std::vector<unsigned int> neighbors(adjacency[v0].begin(), adjacency[v0].end());
        
        if (neighbors.size() < 2) continue;
        

        for (size_t i = 0; i < neighbors.size(); ++i) {
            unsigned int v1 = neighbors[i];
            
            for (size_t j = i + 1; j < neighbors.size(); ++j) {
                unsigned int v2 = neighbors[j];
                

                if (adjacency[v1].find(v2) != adjacency[v1].end()) {

                    std::vector<unsigned int> tri = {v0, v1, v2};
                    std::sort(tri.begin(), tri.end());
                    
                    auto tri_tuple = std::make_tuple(tri[0], tri[1], tri[2]);
                    
                    if (triangle_set.find(tri_tuple) == triangle_set.end()) {
                        triangle_set.insert(tri_tuple);
                        

                        Vec3 center = (vertices[v0] + vertices[v1] + vertices[v2]) / 3.0f;
                        Vec3 e01 = vertices[v1] - vertices[v0];
                        Vec3 e02 = vertices[v2] - vertices[v0];
                        Vec3 normal = Vec3::cross(e01, e02);
                        

                        if (Vec3::dot(normal, center) <= 0) {
                            triangles.push_back(Triangle(v0, v1, v2));
                        } else {
                            triangles.push_back(Triangle(v0, v2, v1));
                        }
                    }
                }
            }
        }
    }
    
    // Step 4: Compute triangle normals
    std::cout << "Computing normals..." << std::endl;
    triangle_normals.resize(triangles.size());
    for (size_t t = 0; t < triangles.size(); ++t) {
        const Triangle& tri = triangles[t];
        const Vec3& p0 = vertices[tri[0]];
        const Vec3& p1 = vertices[tri[1]];
        const Vec3& p2 = vertices[tri[2]];
        
        Vec3 e1 = p1 - p0;
        Vec3 e2 = p2 - p0;
        
        Vec3 n = Vec3::cross(e1, e2);
        n.normalize();
        
        triangle_normals[t] = n;
    }
    
    isSphere = true;
    colors.resize(vertices.size());
    
    std::cout << "Fibonacci sphere created with " << vertices.size() 
              << " vertices and " << triangles.size() << " triangles." << std::endl;
}