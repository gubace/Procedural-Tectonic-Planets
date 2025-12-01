#include "mesh.h"
#include "SphericalGrid.h"

#include <map>
#include <cmath>
#include <set>
#include <limits>
#include <algorithm>


#include <Mathematics/Delaunay3.h>   // chemin possible : GTE/Mathematics/Delaunay3.h
#include <Mathematics/Vector3.h>
#include <unordered_map>
#include <array>
#include <cstdint>

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




void Mesh::setupSphere(float radius, unsigned int numPoints) {
    vertices.clear();
    normals.clear();
    triangles.clear();
    triangle_normals.clear();
    
    if (numPoints < 4) numPoints = 4;
    
    const float PI = 3.14159265358979323846f;
    const float PHI = (1.0f + std::sqrt(5.0f)) / 2.0f;
    
    // Generate Fibonacci sphere points (ton code)
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

    // --- Build Delaunay3 with GTE ---
    // Convert to gte::Vector3<float>
    std::vector<gte::Vector3<float>> gtePts;
    gtePts.reserve(vertices.size());
    for (const auto &v : vertices) {
        gtePts.emplace_back(v[0], v[1], v[2]); // adapte si ton Vec3 a d'autres noms
    }

    // Création Delaunay3 (API similaire à l'exemple GTE)
    gte::Delaunay3<float> delaunay;
    delaunay(gtePts); // op() construit la triangulation

    // Récupère indices des tétraèdres (4 * Ntetra entries)
    const std::vector<int32_t> &tindices = delaunay.GetIndices();
    size_t numTetra = delaunay.GetNumTetrahedra();

    // Map faces -> compteur + store one orientation (pour reconstruire triangle)
    struct FaceInfo { int count; std::array<int32_t,3> oriented; };
    std::unordered_map<uint64_t, FaceInfo> faceMap;
    faceMap.reserve(numTetra * 4);

    // Pour chaque tétra (quatre sommets)
    for (size_t t = 0; t < numTetra; ++t) {
        int32_t v0 = tindices[4*t + 0];
        int32_t v1 = tindices[4*t + 1];
        int32_t v2 = tindices[4*t + 2];
        int32_t v3 = tindices[4*t + 3];

        // 4 faces : (v0,v1,v2), (v0,v1,v3), (v0,v2,v3), (v1,v2,v3)
        std::array<std::array<int32_t,3>,4> faces = {{
            {v0,v1,v2},
            {v0,v1,v3},
            {v0,v2,v3},
            {v1,v2,v3}
        }};

        for (auto &f : faces) {
            uint64_t key = pack_face_key(f[0], f[1], f[2]);
            auto it = faceMap.find(key);
            if (it == faceMap.end()) {
                FaceInfo info;
                info.count = 1;
                info.oriented = f; // conserve orientation tel quel (peut être utile)
                faceMap.emplace(key, info);
            } else {
                it->second.count += 1;
            }
        }
    }

    // Les faces frontière sont celles avec count == 1
    triangles.reserve(faceMap.size());
    for (auto const &kv : faceMap) {
        if (kv.second.count == 1) {
            auto f = kv.second.oriented;
            // ajouter le triangle en utilisant l'orientation conservée
            Triangle tri;
            tri[0] = static_cast<unsigned int>(f[0]);
            tri[1] = static_cast<unsigned int>(f[1]);
            tri[2] = static_cast<unsigned int>(f[2]);
            triangles.push_back(tri);
        }
    }

    isSphere = true;
}