#include <algorithm>

#include "palette.h"

std::vector<Palette> Palette::palettes;
int Palette::currentPalette = -1;

void Palette::loadPalettes() {
    Palette earth;
    earth.color_deep     = Vec3(0.04f, 0.05f, 0.20f);
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
    if (t < color_deep_treshold) {
        float tt = smooth(t / color_deep_treshold);
        col = mix(color_deep, color_shallow, tt);
    } else if (t < color_shallow_treshold) {
        float tt = smooth((t - color_deep_treshold) / (color_shallow_treshold - color_deep_treshold));
        col = color_shallow;
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