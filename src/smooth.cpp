#include "planet.h"

void Planet::smoothColors() {
    std::vector<Vec3> newColors(colors.size());

    const float strength = 0.1f;
    const int iterations = 10;

    for (int it = 0; it < iterations; it++) {

        for (size_t v = 0; v < vertices.size(); v++) {

            const auto& neigh = neighbors[v];
            if (neigh.empty()) {
                newColors[v] = colors[v];
                continue;
            }

            Vec3 avg(0.0f, 0.0f, 0.0f);
            for (unsigned int nv : neigh) {
                avg += colors[nv];
            }
            avg /= float(neigh.size());

            newColors[v] = colors[v] * (1.0f - strength) + avg * strength;
        }

        colors = newColors;
    }
}


void Planet::smooth() {
    doSmooth(0.3f);
    doSmooth(-0.3f);
}

void Planet::doSmooth(float lambda) {
    std::vector<Vec3> newVertices(vertices.size());

    const int iterations = 10;

    for (int it = 0; it < iterations; it++)
    {
        for (int v = 0; v < vertices.size(); v++)
        {
            const auto& neigh = neighbors[v];
            if (neigh.empty()) {
                newVertices[v] = vertices[v];
                continue;
            }

            float currentRadius = vertices[v].length();

            float avgRadius = 0.0f;
            for (unsigned int nv : neigh) {
                avgRadius += vertices[nv].length();
            }
            avgRadius /= float(neigh.size());

            float smoothedRadius = currentRadius + lambda * (avgRadius - currentRadius);

            Vec3 avg(0,0,0);
            for (unsigned int nv : neigh) {
                Vec3 normalizedNeigh = vertices[nv];
                normalizedNeigh.normalize();
                avg += normalizedNeigh;
            }
            avg /= float(neigh.size());

            Vec3 normal = vertices[v];
            normal.normalize();

            Vec3 lap = avg - normal;
            Vec3 tangentLap = lap - normal * Vec3::dot(lap, normal);
            Vec3 smoothedDir = normal + lambda * tangentLap;
            smoothedDir.normalize();

            newVertices[v] = smoothedDir * smoothedRadius;
        }

        vertices = newVertices;
    }

}