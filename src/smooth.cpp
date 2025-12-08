#include "planet.h"

void Planet::smooth()
{
    std::vector<Vec3> newVertices(vertices.size());

    const float lambda = 0.4f;    // smoothing strength (tangential)
    const int iterations = 1;

    for (int it = 0; it < iterations; it++)
    {
        for (int v = 0; v < vertices.size(); v++)
        {
            const auto& neigh = neighbors[v];
            if (neigh.empty()) {
                newVertices[v] = vertices[v];
                continue;
            }

            // 1. Compute average of neighbors
            Vec3 avg(0,0,0);
            for (unsigned int nv : neigh)
                avg += vertices[nv];
            avg /= float(neigh.size());

            // 2. Laplacian
            Vec3 lap = avg - vertices[v];

            // 3. Project Laplacian to tangent plane to preserve radial structure
            Vec3 normal = vertices[v];
            normal.normalize();
            Vec3 tangentLap = lap - normal * Vec3::dot(lap, normal);

            // 4. Move vertex only along tangent directions
            Vec3 smoothed = vertices[v] + lambda * tangentLap;

            // 5. Reproject to sphere with elevation preserved
            smoothed.normalize();
            smoothed *= radius + amplified_elevations[v];

            newVertices[v] = smoothed;
        }

        vertices = newVertices;
    }
}