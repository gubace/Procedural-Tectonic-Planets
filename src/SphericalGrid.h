#pragma once
#include <vector>
#include <cmath>

#include "Vec3.h"
#include "planet.h"

class SphericalGrid {
public:
    SphericalGrid(const std::vector<Vec3>& points, Planet& planet, unsigned int resolution = 64)
        : m_points(points), m_resolution(resolution) {
        m_cells.resize(resolution * resolution);
        
        for (uint32_t i = 0; i < points.size(); ++i) {
            Vec3 p = points[i];

            if(planet.crust_data[i]->is_under_subduction) continue;

            p = p / p.length(); // normaliser
            
            // Coordonnées sphériques
            float theta = std::atan2(p[1], p[0]); // [-π, π]
            float phi = std::acos(p[2]); // [0, π]
            
            int u = (int)((theta + M_PI) / (2.0f * M_PI) * m_resolution) % m_resolution;
            int v = (int)(phi / M_PI * m_resolution);
            if (v >= (int)m_resolution) v = m_resolution - 1;
            
            m_cells[v * m_resolution + u].push_back(i);
        }
    }
    
    uint32_t nearest(const Vec3& q) const {
        Vec3 qn = q / q.length();
        
        float theta = std::atan2(qn[1], qn[0]);
        float phi = std::acos(qn[2]);
        
        int u = (int)((theta + M_PI) / (2.0f * M_PI) * m_resolution) % m_resolution;
        int v = (int)(phi / M_PI * m_resolution);
        if (v >= (int)m_resolution) v = m_resolution - 1;
        
        uint32_t bestIdx = 0;
        float bestD2 = std::numeric_limits<float>::infinity();
        
        // Chercher dans la cellule + 8 voisines
        for (int dv = -1; dv <= 1; ++dv) {
            for (int du = -1; du <= 1; ++du) {
                int nu = (u + du + m_resolution) % m_resolution;
                int nv = v + dv;
                if (nv < 0 || nv >= (int)m_resolution) continue;
                
                for (uint32_t idx : m_cells[nv * m_resolution + nu]) {
                    float d2 = (m_points[idx] - q).squareLength();
                    if (d2 < bestD2) {
                        bestD2 = d2;
                        bestIdx = idx;
                    }
                }
            }
        }
        return bestIdx;
    }

    std::vector<unsigned int> neighbors(const Vec3& q) const {
        Vec3 qn = q / q.length(); 

        
        float theta = std::atan2(qn[1], qn[0]);
        float phi = std::acos(qn[2]);

        
        int u = (int)((theta + M_PI) / (2.0f * M_PI) * m_resolution) % m_resolution;
        int v = (int)(phi / M_PI * m_resolution);
        if (v >= (int)m_resolution) v = m_resolution - 1;

        std::vector<uint32_t> result;

        
        for (int dv = -1; dv <= 1; ++dv) {
            for (int du = -1; du <= 1; ++du) {
                if (dv == 0 && du == 0) continue;

                int nu = (u + du + m_resolution) % m_resolution;
                int nv = v + dv;
                if (nv < 0 || nv >= (int)m_resolution) continue;
                
                const auto& cell = m_cells[nv * m_resolution + nu];
                result.insert(result.end(), cell.begin(), cell.end());
            }
        }

        return result;
    }

private:
    const std::vector<Vec3>& m_points;
    unsigned int m_resolution;
    std::vector<std::vector<uint32_t>> m_cells;
};