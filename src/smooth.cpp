#include "planet.h"

void Planet::smooth()
{
    std::vector<Vec3> newVertices(vertices.size());

    const float lambda = 0.4f;    // smoothing strength
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

            // 1. Calculer le rayon actuel du vertex
            float currentRadius = vertices[v].length();

            // 2. Calculer la moyenne des rayons des voisins
            float avgRadius = 0.0f;
            for (unsigned int nv : neigh) {
                avgRadius += vertices[nv].length();
            }
            avgRadius /= float(neigh.size());

            // 3. Lisser le rayon
            float smoothedRadius = currentRadius + lambda * (avgRadius - currentRadius);

            // 4. Calculer la position moyenne normalisée des voisins
            Vec3 avg(0,0,0);
            for (unsigned int nv : neigh) {
                Vec3 normalizedNeigh = vertices[nv];
                normalizedNeigh.normalize();
                avg += normalizedNeigh;
            }
            avg /= float(neigh.size());

            // 5. Direction du vertex actuel
            Vec3 normal = vertices[v];
            normal.normalize();

            // 6. Lissage tangentiel (Laplacien sur la sphère)
            Vec3 lap = avg - normal;
            Vec3 tangentLap = lap - normal * Vec3::dot(lap, normal);
            Vec3 smoothedDir = normal + lambda * tangentLap;
            smoothedDir.normalize();

            // 7. Appliquer le rayon lissé
            newVertices[v] = smoothedDir * smoothedRadius;
        }

        vertices = newVertices;
    }

    // 8. Recalculer les amplified_elevations à partir des nouvelles positions
    for (size_t i = 0; i < vertices.size(); i++) {
        amplified_elevations[i] = vertices[i].length() - radius;
    }
}