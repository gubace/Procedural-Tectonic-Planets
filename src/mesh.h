#pragma once

#include <iostream>
#include <vector>
#include <algorithm>
#include <string>
#include <cstdio>
#include <cstdlib>
#include <float.h>

#include "Vec3.h"

struct Triangle {
    inline Triangle () {
        v[0] = v[1] = v[2] = 0;
    }
    inline Triangle (const Triangle & t) {
        v[0] = t.v[0];   v[1] = t.v[1];   v[2] = t.v[2];
    }
    inline Triangle (unsigned int v0, unsigned int v1, unsigned int v2) {
        v[0] = v0;   v[1] = v1;   v[2] = v2;
    }
    unsigned int & operator [] (unsigned int iv) { return v[iv]; }
    unsigned int operator [] (unsigned int iv) const { return v[iv]; }
    inline virtual ~Triangle () {}
    inline Triangle & operator = (const Triangle & t) {
        v[0] = t.v[0];   v[1] = t.v[1];   v[2] = t.v[2];
        return (*this);
    }
    // membres indices des sommets du triangle:
    unsigned int v[3];
};



class Mesh {
    public:
        std::vector< Vec3 > colors;
        std::vector< Vec3 > vertices; //array of mesh vertices positions
        std::vector< Vec3 > normals; //array of vertices normals useful for the display
        std::vector< Triangle > triangles; //array of mesh triangles
        std::vector< Vec3 > triangle_normals; //triangle normals to display face normals
        bool isSphere = false;


        //Compute face normals for the display
        void computeTrianglesNormals();

        void addNoise();
        
        void computeNormals();

        void setupSphere(float radius = 1.0f, unsigned int sectors = 32, unsigned int stacks = 16);

        void removeTriangle(unsigned int idx);
};



