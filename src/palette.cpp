#include "palette.h"

std::vector<Palette> Palette::palettes;
int Palette::currentPalette = -1;

void Palette::loadPalettes() {
    Palette earth;
    earth.color_lowland  = Vec3(0.10f, 0.20f, 0.20f);
    earth.color_midland  = Vec3(0.25f, 0.35f, 0.35f);
    earth.color_highland = Vec3(0.25f, 0.25f, 0.20f);
    earth.color_snow     = Vec3(0.8f, 0.9f, 0.9f);
    earth.color_deep     = Vec3(0.02f, 0.05f, 0.40f);
    earth.color_shallow  = Vec3(0.12f, 0.45f, 0.8f);
    palettes.push_back(earth);

    Palette mars;
    mars.color_lowland  = Vec3(0.45f, 0.18f, 0.10f);   // rojizo oscuro (valles)
    mars.color_midland  = Vec3(0.60f, 0.28f, 0.15f);   // óxido naranja
    mars.color_highland = Vec3(0.75f, 0.40f, 0.22f);   // polvo marciano brillante
    mars.color_snow     = Vec3(0.90f, 0.85f, 0.80f);   // hielo de CO₂ / casquete polar
    mars.color_deep     = Vec3(0.10f, 0.05f, 0.03f);   // sombras muy oscuras
    mars.color_shallow  = Vec3(0.40f, 0.20f, 0.12f);   // transiciones suaves
    palettes.push_back(mars);

    Palette alien;
    alien.color_lowland  = Vec3(0.0f, 1.0f, 0.2f);    // verde neón brillante
    alien.color_midland  = Vec3(0.8f, 0.0f, 0.9f);    // púrpura eléctrico
    alien.color_highland = Vec3(0.2f, 0.6f, 1.0f);    // azul cobalto brillante
    alien.color_snow     = Vec3(1.0f, 0.4f, 0.8f);    // rosa fluorescente
    alien.color_deep     = Vec3(0.05f, 0.0f, 0.2f);   // sombras azuladas oscuras
    alien.color_shallow  = Vec3(0.5f, 1.0f, 0.5f);    // verde limón suave
    palettes.push_back(alien);


    Palette alien1;
    alien1.color_lowland  = Vec3(1.0f, 0.3f, 0.0f);   // rojo fuego
    alien1.color_midland  = Vec3(1.0f, 0.6f, 0.2f);   // naranja brillante
    alien1.color_highland = Vec3(0.8f, 0.0f, 0.8f);   // púrpura intenso
    alien1.color_snow     = Vec3(0.9f, 1.0f, 0.9f);   // blanco verdoso cristalino
    alien1.color_deep     = Vec3(0.1f, 0.0f, 0.1f);   // sombras profundas moradas
    alien1.color_shallow  = Vec3(1.0f, 0.8f, 0.5f);   // reflejos dorados
    palettes.push_back(alien1);


    Palette alien2;
    alien2.color_lowland  = Vec3(0.0f, 1.0f, 1.0f);   // cian neón
    alien2.color_midland  = Vec3(1.0f, 0.0f, 1.0f);   // magenta eléctrico
    alien2.color_highland = Vec3(1.0f, 1.0f, 0.0f);   // amarillo fluorescente
    alien2.color_snow     = Vec3(0.6f, 0.2f, 1.0f);   // violeta brillante
    alien2.color_deep     = Vec3(0.0f, 0.0f, 0.3f);   // sombras azul oscuro
    alien2.color_shallow  = Vec3(0.0f, 1.0f, 0.5f);   // verde lima eléctrico
    palettes.push_back(alien2);
}

Palette Palette::getNextPallete() {
    currentPalette = (currentPalette + 1) % palettes.size();
    return palettes[currentPalette];
}

Palette Palette::getCurrentPalette() {
    return palettes[currentPalette];
}