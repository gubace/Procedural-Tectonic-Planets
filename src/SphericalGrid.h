#pragma once
#include <vector>
#include <cmath>
#include <limits>
#include <utility>

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


    std::pair<uint32_t, uint32_t> nearestFromDifferentPlates(const Vec3& q, const Planet& planet) const {
        Vec3 qn = q / q.length();
        
        float theta = std::atan2(qn[1], qn[0]);
        float phi = std::acos(qn[2]);
        
        int u = (int)((theta + M_PI) / (2.0f * M_PI) * m_resolution) % m_resolution;
        int v = (int)(phi / M_PI * m_resolution);
        if (v >= (int)m_resolution) v = m_resolution - 1;
        
        // Collect all candidates from neighboring cells with their distances
        std::vector<std::pair<float, uint32_t>> candidates;
        
        // Search in progressively larger neighborhoods until we find points from 2 different plates
        for (int radius = 1; radius <= 3; ++radius) {
            for (int dv = -radius; dv <= radius; ++dv) {
                for (int du = -radius; du <= radius; ++du) {
                    // Only check the outer ring of the current radius
                    if (std::abs(dv) != radius && std::abs(du) != radius) continue;
                    
                    int nu = (u + du + m_resolution) % m_resolution;
                    int nv = v + dv;
                    if (nv < 0 || nv >= (int)m_resolution) continue;
                    
                    for (uint32_t idx : m_cells[nv * m_resolution + nu]) {
                        float d2 = (m_points[idx] - q).squareLength();
                        candidates.push_back({d2, idx});
                    }
                }
            }
            
            // Sort candidates by distance
            std::sort(candidates.begin(), candidates.end());
            
            // Find the two closest points from different plates
            if (candidates.size() >= 2) {
                uint32_t firstIdx = candidates[0].second;
                unsigned int firstPlate = planet.verticesToPlates[firstIdx];
                
                for (size_t i = 1; i < candidates.size(); ++i) {
                    uint32_t secondIdx = candidates[i].second;
                    unsigned int secondPlate = planet.verticesToPlates[secondIdx];
                    
                    if (secondPlate != firstPlate) {
                        return {firstIdx, secondIdx};
                    }
                }
            }
        }
        
        // Fallback: if not found in nearby cells, do exhaustive search (should rarely happen)
        std::vector<std::pair<float, uint32_t>> allPoints;
        for (uint32_t i = 0; i < m_points.size(); ++i) {
            float d2 = (m_points[i] - q).squareLength();
            allPoints.push_back({d2, i});
        }
        
        std::sort(allPoints.begin(), allPoints.end());
        
        if (allPoints.size() >= 2) {
            uint32_t firstIdx = allPoints[0].second;
            unsigned int firstPlate = planet.verticesToPlates[firstIdx];
            
            for (size_t i = 1; i < allPoints.size(); ++i) {
                uint32_t secondIdx = allPoints[i].second;
                unsigned int secondPlate = planet.verticesToPlates[secondIdx];
                
                if (secondPlate != firstPlate) {
                    return {firstIdx, secondIdx};
                }
            }
        }
        
        // Should never reach here if planet has at least 2 plates
        return {0, 1};
    }

private:
    const std::vector<Vec3>& m_points;
    unsigned int m_resolution;
    std::vector<std::vector<uint32_t>> m_cells;
};