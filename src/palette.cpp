#include <algorithm>

#include "palette.h"

std::vector<Palette> Palette::palettes;
int Palette::currentPalette = -1;

void Palette::loadPalettes() {
    Palette earth;
    earth.color_deep     = Vec3(0.04f, 0.15f, 0.40f);
    earth.color_shallow  = Vec3(0.12f, 0.45f, 0.80f);
    earth.color_shore  = Vec3(0.70f, 0.70f, 0.50f);
    earth.color_lowland  = Vec3(0.20f, 0.40f, 0.30f);
    earth.color_midland  = Vec3(0.10f, 0.30f, 0.20f);
    earth.color_highland = Vec3(0.15f, 0.25f, 0.20f);
    earth.color_snow     = Vec3(0.8f, 0.9f, 0.9f);
    palettes.push_back(earth);

    Palette mars;
    mars.color_deep     = Vec3(0.10f, 0.05f, 0.03f);   // sombras muy oscuras
    mars.color_shallow  = Vec3(0.40f, 0.20f, 0.12f);   // transiciones suaves
    mars.color_shore  = Vec3(0.45f, 0.18f, 0.10f);
    mars.color_lowland  = Vec3(0.45f, 0.18f, 0.10f);   // rojizo oscuro (valles)
    mars.color_midland  = Vec3(0.60f, 0.28f, 0.15f);   // óxido naranja
    mars.color_highland = Vec3(0.75f, 0.40f, 0.22f);   // polvo marciano brillante
    mars.color_snow     = Vec3(0.90f, 0.85f, 0.80f);   // hielo de CO₂ / casquete polar
    palettes.push_back(mars);

    Palette venus;
    venus.color_deep     = Vec3(0.20f, 0.16f, 0.10f);   // tonos oscuros amarillentos
    venus.color_shallow  = Vec3(0.35f, 0.28f, 0.15f);
    venus.color_shore    = Vec3(0.55f, 0.45f, 0.25f);
    venus.color_lowland  = Vec3(0.70f, 0.55f, 0.30f);
    venus.color_midland  = Vec3(0.80f, 0.65f, 0.35f);
    venus.color_highland = Vec3(0.90f, 0.75f, 0.45f);
    venus.color_snow     = Vec3(1.00f, 0.90f, 0.60f);   // bruma amarillenta brillante
    palettes.push_back(venus);

    Palette bioluminescent_world;
    bioluminescent_world.color_deep     = Vec3(0.00f, 0.02f, 0.05f); // casi negro azulado
    bioluminescent_world.color_shallow  = Vec3(0.00f, 0.10f, 0.20f);
    bioluminescent_world.color_shore    = Vec3(0.00f, 0.25f, 0.35f);
    bioluminescent_world.color_lowland  = Vec3(0.00f, 0.40f, 0.25f); 
    bioluminescent_world.color_midland  = Vec3(0.10f, 0.60f, 0.30f); 
    bioluminescent_world.color_highland = Vec3(0.30f, 0.80f, 0.50f);
    bioluminescent_world.color_snow     = Vec3(0.60f, 1.00f, 0.80f); // brillo verde-menta
    palettes.push_back(bioluminescent_world);

    Palette zibzoob;
    zibzoob.color_deep     = Vec3(0.05f, 0.0f, 0.2f);   // sombras azuladas oscuras
    zibzoob.color_shallow  = Vec3(0.5f, 1.0f, 0.5f);    // verde limón suave
    zibzoob.color_shore  = Vec3(0.0f, 1.0f, 0.2f);
    zibzoob.color_lowland  = Vec3(0.0f, 1.0f, 0.2f);    // verde neón brillante
    zibzoob.color_midland  = Vec3(0.8f, 0.0f, 0.9f);    // púrpura eléctrico
    zibzoob.color_highland = Vec3(0.2f, 0.6f, 1.0f);    // azul cobalto brillante
    zibzoob.color_snow     = Vec3(1.0f, 0.4f, 0.8f);    // rosa fluorescente
    palettes.push_back(zibzoob);


    Palette blingblang;
    blingblang.color_deep     = Vec3(0.1f, 0.0f, 0.1f);   // sombras profundas moradas
    blingblang.color_shallow  = Vec3(1.0f, 0.8f, 0.5f);   // reflejos dorados
    blingblang.color_shore  = Vec3(1.0f, 0.3f, 0.0f);
    blingblang.color_lowland  = Vec3(1.0f, 0.3f, 0.0f);   // rojo fuego
    blingblang.color_midland  = Vec3(1.0f, 0.6f, 0.2f);   // naranja brillante
    blingblang.color_highland = Vec3(0.8f, 0.0f, 0.8f);   // púrpura intenso
    blingblang.color_snow     = Vec3(0.9f, 1.0f, 0.9f);   // blanco verdoso cristalino
    palettes.push_back(blingblang);


    Palette zoobzib;
    zoobzib.color_deep     = Vec3(0.0f, 0.0f, 0.3f);   // sombras azul oscuro
    zoobzib.color_shallow  = Vec3(0.0f, 1.0f, 1.0f);   // verde lima eléctrico
    zoobzib.color_lowland  = Vec3(0.0f, 1.0f, 1.0f);   // cian neón
    zoobzib.color_shore  = Vec3(1.0f, 0.3f, 0.0f);
    zoobzib.color_midland  = Vec3(1.0f, 0.0f, 1.0f);   // magenta eléctrico
    zoobzib.color_highland = Vec3(1.0f, 1.0f, 0.0f);   // amarillo fluorescente
    zoobzib.color_snow     = Vec3(0.6f, 0.2f, 1.0f);   // violeta brillante
    palettes.push_back(zoobzib);

    Palette ice_world;
    ice_world.color_deep     = Vec3(0.02f, 0.10f, 0.20f); // azul profundo
    ice_world.color_shallow  = Vec3(0.10f, 0.30f, 0.50f);
    ice_world.color_shore    = Vec3(0.50f, 0.70f, 0.85f);
    ice_world.color_lowland  = Vec3(0.70f, 0.85f, 0.95f);
    ice_world.color_midland  = Vec3(0.80f, 0.90f, 1.00f);
    ice_world.color_highland = Vec3(0.90f, 0.95f, 1.00f);
    ice_world.color_snow     = Vec3(1.00f, 1.00f, 1.00f); // blanco puro
    palettes.push_back(ice_world);

    Palette aurora_world;
    aurora_world.color_deep     = Vec3(0.00f, 0.00f, 0.05f); // noche profunda
    aurora_world.color_shallow  = Vec3(0.05f, 0.05f, 0.15f);
    aurora_world.color_shore    = Vec3(0.10f, 0.20f, 0.35f);
    aurora_world.color_lowland  = Vec3(0.25f, 0.45f, 0.55f);
    aurora_world.color_midland  = Vec3(0.45f, 0.75f, 0.70f);
    aurora_world.color_highland = Vec3(0.65f, 0.95f, 0.85f);
    aurora_world.color_snow     = Vec3(0.80f, 1.00f, 0.95f); // luz de aurora
    palettes.push_back(aurora_world);

    Palette colorful_world;
    colorful_world.color_deep     = Vec3(0.20f, 0.00f, 0.20f); // púrpura oscuro saturado
    colorful_world.color_shallow  = Vec3(0.40f, 0.05f, 0.30f);
    colorful_world.color_shore    = Vec3(0.70f, 0.15f, 0.30f);
    colorful_world.color_lowland  = Vec3(0.90f, 0.40f, 0.20f);
    colorful_world.color_midland  = Vec3(0.95f, 0.65f, 0.10f);
    colorful_world.color_highland = Vec3(0.90f, 0.85f, 0.20f);
    colorful_world.color_snow     = Vec3(1.00f, 0.95f, 0.60f); // vibrante y cálido
    palettes.push_back(colorful_world);
}

Palette Palette::getNextPallete() {
    currentPalette = (currentPalette + 1) % palettes.size();
    return palettes[currentPalette];
}

Palette Palette::getCurrentPalette() {
    return palettes[currentPalette];
}

Vec3 Palette::getColorFromValue(float t) const {
    Vec3 col;
    if (t < color_shallow_treshold) {
        float tt = (t - color_deep_treshold) / (color_shallow_treshold - color_deep_treshold);
        col = mix(color_deep, color_shallow, tt);
    } else if (t < color_shore_treshold) {
        float tt = smooth((t - color_shallow_treshold) / (color_shore_treshold - color_shallow_treshold));
        col = mix(color_shore, color_lowland, tt);
    } else if (t < color_lowland_treshold) {
        float tt = smooth((t - color_shore_treshold) / (color_lowland_treshold - color_shore_treshold));
        col = mix(color_lowland, color_midland, tt);
    } else if (t < color_midland_treshold) {
        float tt = smooth((t - color_lowland_treshold) / (color_midland_treshold - color_lowland_treshold));
        col = mix(color_midland, color_highland, tt);
    } else if (t < color_highland_treshold) {
        float tt = smooth((t - color_midland_treshold) / (color_highland_treshold - color_midland_treshold));
        col = mix(color_highland, color_snow, tt);
    } else {
        col = color_snow;
    }

    return col;
}