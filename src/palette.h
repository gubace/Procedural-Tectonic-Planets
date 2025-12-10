#pragma once

#include <vector>
#include "Vec3.h"

class Palette {
public:
    static std::vector<Palette> palettes;

    Vec3 color_deep;
    Vec3 color_shallow;
    Vec3 color_lowland;
    Vec3 color_midland;
    Vec3 color_highland;
    Vec3 color_snow;

    static constexpr float color_deep_treshold = 0.3;
    static constexpr float color_shallow_treshold = 0.5;
    static constexpr float color_lowland_treshold = 0.55;
    static constexpr float color_midland_treshold = 0.7;
    static constexpr float color_highland_treshold = 0.9;

    static Vec3 mix(const Vec3& a, const Vec3& b, float t) {
        return a * (1.0f - t) + b * t;
    }

    static float smooth(float x) {
        x = clamp01(x);
        return x * x * (3.0f - 2.0f * x);
    }
    
    static float clamp01(float x) {
        return (x < 0.0f) ? 0.0f : (x > 1.0f) ? 1.0f : x;
    }

    Palette() {}

    Vec3 getColorFromValue(float t) const;

    static void loadPalettes();

    static Palette getNextPallete();
    static Palette getCurrentPalette();

private:
    static int currentPalette;
};