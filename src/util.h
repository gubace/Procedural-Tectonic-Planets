#pragma once

#include <algorithm> // std::min, std::max
#include <cmath>     // std::acos, std::sqrt
#include <GL/glew.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glut.h>
#include <vector>
#include <string>
#include <iostream>

#include "tectonicPhenomenon.h"
#include "Vec3.h"
#include "planet.h"


static void drawArrow(const Vec3 & start, const Vec3 & dir, float shaftWidth = 1.5f, const Vec3 & color = Vec3(1.0f,0.0f,0.0f)) {
    const float PI = 3.14159265358979323846f;
    float len = dir.length();
    if (len <= 1e-6f) return;

    Vec3 d = dir / len;

    glPushAttrib(GL_CURRENT_BIT | GL_ENABLE_BIT | GL_LINE_BIT);
    glDisable(GL_LIGHTING);
    glColor3f(color[0], color[1], color[2]);


    glLineWidth(shaftWidth);
    glBegin(GL_LINES);
      glVertex3f(start[0], start[1], start[2]);
      glVertex3f(start[0] + dir[0], start[1] + dir[1], start[2] + dir[2]);
    glEnd();


    float coneLength = std::min(0.12f * len, 0.12f);
    float coneRadius = coneLength * 0.5f;

    Vec3 zAxis(0.0f, 0.0f, 1.0f);
    Vec3 axis = Vec3::cross(zAxis, d);
    float axisLen = axis.length();
    float angleDeg = 0.0f;
    if (axisLen > 1e-6f) {
        axis /= axisLen;
        float cosang = std::max(-1.0f, std::min(1.0f, Vec3::dot(zAxis, d)));
        angleDeg = std::acos(cosang) * 180.0f / PI;
    }

    Vec3 tip = start + dir;

    glPushMatrix();
      glTranslatef(tip[0], tip[1], tip[2]);
      if (axisLen > 1e-6f) glRotatef(angleDeg, axis[0], axis[1], axis[2]);

      glTranslatef(0.0f, 0.0f, -coneLength);

      glutSolidCone(coneRadius, coneLength, 8, 2);
    glPopMatrix();

    glPopAttrib();
}


static void drawPlateArrows(const Planet & planet, float visualScale = 0.4f) {

    for (const Plate &plate : planet.plates) {
        if (plate.vertices_indices.empty()) continue;


        Vec3 centroid(0.0f, 0.0f, 0.0f);
        for (unsigned int vid : plate.vertices_indices) {
            if (vid < planet.vertices.size()) centroid += planet.vertices[vid];
        }
        centroid /= (float)plate.vertices_indices.size();
        centroid.normalize();
        centroid *= planet.radius;


        Vec3 linVel = Vec3::cross(plate.rotation_axis, centroid);
        float lvlen = linVel.length();
        if (lvlen > 1e-6f) {
            linVel = (linVel / lvlen) * (plate.plate_velocity * visualScale);
        } else {
            linVel = Vec3(0.0f, 0.0f, 0.0f);
        }

        // choose color based on speed (fast -> red, slow -> green)
        float speed = plate.plate_velocity;
        Vec3 col = Vec3( std::min(1.0f, speed*1.5f), std::max(0.0f, 1.0f - speed), 0.1f );


        drawArrow(centroid, linVel, 2.0f, col);
    }
}



static void drawTectonicPhenomenaMarkers(const Planet & planet, const std::vector<std::unique_ptr<TectonicPhenomenon>> & phenomena, const  float markerSize = 0.02f) {
    glPushAttrib(GL_ENABLE_BIT | GL_CURRENT_BIT);
    glDisable(GL_LIGHTING);
    
    for (const auto& phenomenon : phenomena) {
        unsigned int vid = phenomenon->getVertexIndex();
        if (vid >= planet.vertices.size()) continue;
        
        Vec3 pos = planet.vertices[vid];
        Vec3 col(1.0f, 1.0f, 1.0f); // default white
        
        // Choose color based on phenomenon type
        switch (phenomenon->getType()) {
            case TectonicPhenomenon::Type::Subduction: {
                // Cast to Subduction to get subduction-specific info
                const Subduction* sub = dynamic_cast<const Subduction*>(phenomenon.get());
                if (sub) {
                    switch (sub->getSubductionType()) {
                        case Subduction::SubductionType::Oceanic_Oceanic:
                            col = Vec3(1.0f, 0.8f, 0.0f); // yellow
                            break;
                        case Subduction::SubductionType::Oceanic_Continental:
                            col = Vec3(1.0f, 0.2f, 0.2f); // red
                            break;
                        case Subduction::SubductionType::Continental_Continental:
                            col = Vec3(1.0f, 0.0f, 1.0f); // magenta
                            break;
                    }
                }
                break;
            }
            case TectonicPhenomenon::Type::ContinentalCollision:
                col = Vec3(0.8f, 0.0f, 0.8f); // purple
                break;
            case TectonicPhenomenon::Type::crustGeneration:
                col = Vec3(0.0f, 0.8f, 1.0f); // cyan
                break;
        }
        
        glColor3f(col[0], col[1], col[2]);
        glPushMatrix();
            glTranslatef(pos[0], pos[1], pos[2]);
            glutSolidSphere(markerSize, 8, 8);
        glPopMatrix();
    }
    
    glPopAttrib();
}

// convert HSV->RGB
static Vec3 hsv2rgb(float h, float s, float v) {
    float r = 0, g = 0, b = 0;
    if (s <= 0.0f) {
        r = g = b = v;
    } else {
        float hh = h * 6.0f;
        int i = (int)std::floor(hh);
        float f = hh - i;
        float p = v * (1.0f - s);
        float q = v * (1.0f - s * f);
        float t = v * (1.0f - s * (1.0f - f));
        switch (i % 6) {
            case 0:
                r = v;
                g = t;
                b = p;
                break;
            case 1:
                r = q;
                g = v;
                b = p;
                break;
            case 2:
                r = p;
                g = v;
                b = t;
                break;
            case 3:
                r = p;
                g = q;
                b = v;
                break;
            case 4:
                r = t;
                g = p;
                b = v;
                break;
            case 5:
                r = v;
                g = p;
                b = q;
                break;
        }
    }
    return Vec3(r * 0.2, g * 0.6, b);
}




inline void createAtmosphereSphere(float radius, int segments,GLuint &atmosphereVAO, GLuint &atmosphereVBO, GLuint &atmosphereEBO, int &atmosphereIndexCount) {
    std::vector<float> vertices;
    std::vector<unsigned int> indices;
    
    for (int lat = 0; lat <= segments; ++lat) {
        float theta = lat * M_PI / segments;
        float sinTheta = sin(theta);
        float cosTheta = cos(theta);
        
        for (int lon = 0; lon <= segments; ++lon) {
            float phi = lon * 2.0f * M_PI / segments;
            float sinPhi = sin(phi);
            float cosPhi = cos(phi);
            
            float x = cosPhi * sinTheta;
            float y = cosTheta;
            float z = sinPhi * sinTheta;
            
            vertices.push_back(x * radius);
            vertices.push_back(y * radius);
            vertices.push_back(z * radius);
        }
    }
    
    for (int lat = 0; lat < segments; ++lat) {
        for (int lon = 0; lon < segments; ++lon) {
            int first = (lat * (segments + 1)) + lon;
            int second = first + segments + 1;
            
            indices.push_back(first);
            indices.push_back(second);
            indices.push_back(first + 1);
            
            indices.push_back(second);
            indices.push_back(second + 1);
            indices.push_back(first + 1);
        }
    }
    
    atmosphereIndexCount = indices.size();
    
    glGenVertexArrays(1, &atmosphereVAO);
    glGenBuffers(1, &atmosphereVBO);
    glGenBuffers(1, &atmosphereEBO);
    
    glBindVertexArray(atmosphereVAO);
    
    glBindBuffer(GL_ARRAY_BUFFER, atmosphereVBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, atmosphereEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);
    
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    glBindVertexArray(0);
}