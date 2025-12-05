#pragma once

#include <vector>
#include "Vec3.h"

class Palette {
public:
    static std::vector<Palette> palettes;

    Vec3 color_lowland;
    Vec3 color_midland;
    Vec3 color_highland;
    Vec3 color_snow;
    Vec3 color_deep;
    Vec3 color_shallow;

    Palette() {}

    static void loadPalettes();

    static Palette getNextPallete();
    static Palette getCurrentPalette();

private:
    static int currentPalette;
};