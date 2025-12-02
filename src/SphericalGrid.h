#pragma once

#include <vector>
#include <algorithm>
#include <cmath>
#include <utility>
#include <limits>

#include "kdtree.h"
#include "Vec3.h"
#include "planet.h"

using namespace Kdtree;

class SphericalKDTree {
public:
    SphericalKDTree(const std::vector<Vec3>& points, const Planet& /*planet*/) {
        m_pointsNormalized.resize(points.size());
        nodes.clear();
        nodes.reserve(points.size());
        for (size_t i = 0; i < points.size(); ++i) {
            const Vec3 &p = points[i];
            float len = p.length();
            Vec3 pn = (len > 1e-12f) ? (p / len) : p;
            m_pointsNormalized[i] = pn;
            CoordPoint cp(3);
            cp[0] = pn[0];
            cp[1] = pn[1];
            cp[2] = pn[2];
            nodes.emplace_back(cp, nullptr, static_cast<int>(i));
        }
        tree = new KdTree(&nodes, 2); // euclidean (squared)
    }

    ~SphericalKDTree() {
        delete tree;
    }

    uint32_t nearest(const Vec3& q) const {
        Vec3 qn;
        float qlen = q.length();
        if (qlen > 1e-12f) qn = q / qlen;
        else qn = q;
        CoordPoint cp = toCoord(qn);
        KdNodeVector res;
        tree->k_nearest_neighbors(cp, 1, &res);
        if (res.empty()) return 0;
        return static_cast<uint32_t>(res[0].index);
    }

    std::vector<uint32_t> kNearest(const Vec3& q, unsigned int k = 8) const {
        Vec3 qn;
        float qlen = q.length();
        if (qlen > 1e-12f) qn = q / qlen;
        else qn = q;
        CoordPoint cp = toCoord(qn);
        KdNodeVector res;
        tree->k_nearest_neighbors(cp, k, &res);
        std::vector<uint32_t> out;
        out.reserve(res.size());
        for (const auto &n : res) out.push_back(static_cast<uint32_t>(n.index));
        return out;
    }

    // find two nearest vertices that belong to different plates
    std::pair<uint32_t, uint32_t> nearestFromDifferentPlates(const Vec3& q, const Planet& planet) const {
        Vec3 qn;
        float qlen = q.length();
        if (qlen > 1e-12f) qn = q / qlen;
        else qn = q;
        CoordPoint cp = toCoord(qn);

        size_t total = nodes.size();
        size_t k = std::min<size_t>(8, total ? total : 1);
        while (k <= total) {
            KdNodeVector res;
            tree->k_nearest_neighbors(cp, k, &res);
            // collect only valid plate-annotated indices sorted by distance
            std::vector<uint32_t> candidates;
            candidates.reserve(res.size());
            for (const auto &n : res) {
                uint32_t idx = static_cast<uint32_t>(n.index);
                if (idx >= planet.verticesToPlates.size()) continue;
                unsigned int p = planet.verticesToPlates[idx];
                if (p >= planet.plates.size()) continue;
                candidates.push_back(idx);
            }
            if (candidates.size() >= 2) {
                // find two with different plates (candidates are in ascending distance order)
                for (size_t i = 0; i + 1 < candidates.size(); ++i) {
                    unsigned int pi = planet.verticesToPlates[candidates[i]];
                    for (size_t j = i + 1; j < candidates.size(); ++j) {
                        unsigned int pj = planet.verticesToPlates[candidates[j]];
                        if (pi != pj) return {candidates[i], candidates[j]};
                    }
                }
            }
            if (k == total) break;
            k = std::min<size_t>(k * 2, total);
        }

        // fallback exhaustive search among annotated vertices
        std::vector<std::pair<double,uint32_t>> all;
        all.reserve(total);
        for (size_t i = 0; i < m_pointsNormalized.size(); ++i) {
            if (i >= planet.verticesToPlates.size()) continue;
            unsigned int p = planet.verticesToPlates[i];
            if (p >= planet.plates.size()) continue;
            Vec3 d = m_pointsNormalized[i] - qn;
            double d2 = d.squareLength();
            all.emplace_back(d2, static_cast<uint32_t>(i));
        }
        if (all.size() >= 2) {
            std::sort(all.begin(), all.end());
            for (size_t i = 0; i + 1 < all.size(); ++i) {
                unsigned int pi = planet.verticesToPlates[all[i].second];
                for (size_t j = i + 1; j < all.size(); ++j) {
                    unsigned int pj = planet.verticesToPlates[all[j].second];
                    if (pi != pj) return {all[i].second, all[j].second};
                }
            }
        }
        return {0, 1};
    }

private:
    CoordPoint toCoord(const Vec3& v) const {
        CoordPoint cp(3);
        cp[0] = v[0];
        cp[1] = v[1];
        cp[2] = v[2];
        return cp;
    }

    KdTree* tree = nullptr;
    KdNodeVector nodes;
    std::vector<Vec3> m_pointsNormalized;
};