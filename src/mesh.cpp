#include "mesh.h"

void Mesh::setupSphere(float radius, unsigned int sectors, unsigned int stacks) {
    if (sectors < 3) sectors = 3;
    if (stacks < 2) stacks = 2;
    vertices.clear();
    normals.clear();
    triangles.clear();
    triangle_normals.clear();

    const float PI = 3.14159265358979323846f;

    // Generate vertices (stacks+1) x (sectors+1) to duplicate seam vertex for simpler indexing
    for (unsigned int i = 0; i <= stacks; ++i) {
        float stackRatio = static_cast<float>(i) / static_cast<float>(stacks); // 0..1
        float phi = PI * stackRatio; // 0..pi

        float sinPhi = std::sin(phi);
        float cosPhi = std::cos(phi);

        for (unsigned int j = 0; j <= sectors; ++j) {
            float sectorRatio = static_cast<float>(j) / static_cast<float>(sectors); // 0..1
            float theta = 2.0f * PI * sectorRatio; // 0..2pi

            float sinTheta = std::sin(theta);
            float cosTheta = std::cos(theta);

            float x = radius * sinPhi * cosTheta;
            float y = radius * cosPhi;
            float z = radius * sinPhi * sinTheta;

            Vec3 pos(x, y, z);
            vertices.push_back(pos);

            // vertex normal (normalized position)
            float nx = sinPhi * cosTheta;
            float ny = cosPhi;
            float nz = sinPhi * sinTheta;
            float len = std::sqrt(nx*nx + ny*ny + nz*nz);
            if (len == 0.f) len = 1.f;
            normals.push_back( Vec3(nx/len, ny/len, nz/len) );
        }
    }

    // Generate triangles using indexing
    unsigned int ringVerts = sectors + 1;
    for (unsigned int i = 0; i < stacks; ++i) {
        for (unsigned int j = 0; j < sectors; ++j) {
            unsigned int first = i * ringVerts + j;
            unsigned int second = first + ringVerts;

            // two triangles per rectangular patch
            triangles.push_back( Triangle(first, second, first + 1) );
            triangles.push_back( Triangle(first + 1, second, second + 1) );
        }
    }

    // Compute triangle normals
    triangle_normals.resize(triangles.size());
    for (size_t t = 0; t < triangles.size(); ++t) {
        const Triangle &tri = triangles[t];
        const Vec3 &p0 = vertices[tri[0]];
        const Vec3 &p1 = vertices[tri[1]];
        const Vec3 &p2 = vertices[tri[2]];

        // edge vectors
        Vec3 e1 = p1;
        e1 -= p0;
        Vec3 e2 = p2;
        e2 -= p0;

        // cross product e1 x e2
        Vec3 n( e1[1]*e2[2] - e1[2]*e2[1],
                e1[2]*e2[0] - e1[0]*e2[2],
                e1[0]*e2[1] - e1[1]*e2[0] );

        // normalize
        float nl = std::sqrt(n[0]*n[0] + n[1]*n[1] + n[2]*n[2]);
        if (nl == 0.f) nl = 1.f;
        n /= nl;

        triangle_normals[t] = n;
    }

    isSphere = true;
}

void Mesh::computeTrianglesNormals(){

        triangle_normals.clear();
        for( unsigned int i = 0 ; i < triangles.size() ;i++ ){
            const Vec3 & e0 = vertices[triangles[i][1]] - vertices[triangles[i][0]];
            const Vec3 & e1 = vertices[triangles[i][2]] - vertices[triangles[i][0]];
            Vec3 n = Vec3::cross( e0, e1 );
            n.normalize();
            triangle_normals.push_back( n );
            }


        
        }

void Mesh::addNoise(){
        for( unsigned int i = 0 ; i < vertices.size() ; i ++ ){
            float factor = 0.03;
            const Vec3 & p = vertices[i];
            const Vec3 & n = normals[i];
            vertices[i] = Vec3( p[0] + factor*((double)(rand()) / (double)
            (RAND_MAX))*n[0], p[1] + factor*((double)(rand()) / (double)
            (RAND_MAX))*n[1], p[2] + factor*((double)(rand()) / (double)
            (RAND_MAX))*n[2]);
        }
    }



void Mesh::computeNormals(){
    computeTrianglesNormals();
}