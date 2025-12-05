#include "mesh.h"
#include "SphericalGrid.h"

#include <map>
#include <cmath>
#include <set>
#include <limits>
#include <algorithm>


#include <Mathematics/ConvexHull3.h> 
#include <Mathematics/Vector3.h>
#include <unordered_map>
#include <array>
#include <cstdint>

#include <chrono>

static inline uint64_t pack_face_key(unsigned int i, unsigned int j, unsigned int k)
{
    // trie i<=j<=k
    if (i > j) std::swap(i, j);
    if (j > k) std::swap(j, k);
    if (i > j) std::swap(i, j);
    // on assume moins de 2^21 (~2,097,152) sommets ; si tu as plus, augmente shift
    const uint64_t SHIFT = 21;
    return ( (uint64_t)i << (2*SHIFT) ) | ( (uint64_t)j << SHIFT ) | (uint64_t)k;
}

void Mesh::recomputeNormals() {

    for (unsigned int i = 0; i < vertices.size(); i++)
        normals[i] = Vec3(0.0, 0.0, 0.0);

    
    for (unsigned int i = 0; i < triangles.size(); i++) {

        unsigned int i0 = triangles[i].v[0];
        unsigned int i1 = triangles[i].v[2];
        unsigned int i2 = triangles[i].v[1];

        Vec3 e01 = vertices[i1] - vertices[i0];
        Vec3 e02 = vertices[i2] - vertices[i0];

        Vec3 n = Vec3::cross(e01, e02);
        n.normalize();

        normals[i0] += n;
        normals[i1] += n;
        normals[i2] += n;
    }

    for (unsigned int i = 0; i < vertices.size(); i++)
        normals[i].normalize();
}


void Mesh::setupSphere(float radius, unsigned int numPoints) {
    vertices.clear();
    normals.clear();
    triangles.clear();
    triangle_normals.clear();
    
    if (numPoints < 4) numPoints = 4;
    
    const float PI = 3.14159265358979323846f;
    const float PHI = (1.0f + std::sqrt(5.0f)) / 2.0f;
    
    // Generate Fibonacci sphere points
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

    std::cout << "finished generating points" << std::endl;

    


    std::vector<gte::Vector3<float>> gtePts;
    gtePts.reserve(vertices.size());

    for (const auto &v : vertices) {
        gtePts.emplace_back(v[0], v[1], v[2]);
    }


    gte::ConvexHull3<float> ch;

    auto t_total_start = std::chrono::steady_clock::now();

    ch(gtePts, 0); // J'ai essayé d'utiliser des threads, mais c'est plus lent

    size_t dim = ch.GetDimension();
    auto hull = ch.GetHull();


    auto t_total_end = std::chrono::steady_clock::now();
    std::chrono::duration<double, std::milli> total_ms = t_total_end - t_total_start;
    std::cout << "GetHull total time: " << total_ms.count() << " ms" << std::endl;

    if (dim == 3) {
        // hull contains triples of indices (triangle faces)
        for (size_t i = 0; i + 2 < hull.size(); i += 3) {
            Triangle tri;
            tri[0] = static_cast<unsigned int>(hull[i + 0]);
            tri[2] = static_cast<unsigned int>(hull[i + 1]);
            tri[1] = static_cast<unsigned int>(hull[i + 2]);
            triangles.push_back(tri);
        }
    } else if (dim == 2) {
        // hull is an ordered polygon (convex). Triangulate as fan.
        if (hull.size() >= 3) {
            unsigned int v0 = static_cast<unsigned int>(hull[0]);
            for (size_t i = 1; i + 1 < hull.size(); ++i) {
                Triangle tri;
                tri[0] = v0;
                tri[1] = static_cast<unsigned int>(hull[i]);
                tri[2] = static_cast<unsigned int>(hull[i + 1]);
                triangles.push_back(tri);
            }
        }
    }



    // isSphere flag
    isSphere = true;
}