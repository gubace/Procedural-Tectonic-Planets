#pragma once

#include <algorithm> // std::min, std::max
#include <cmath>     // std::acos, std::sqrt
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glut.h>
#include <vector>
#include <string>
#include <iostream>

#include "subduction.h"
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



static void drawSubductionMarkers(const Planet & planet, const std::vector<SubductionCandidate> & cands, float markerSize = 0.02f) {
    glPushAttrib(GL_ENABLE_BIT | GL_CURRENT_BIT);
    glDisable(GL_LIGHTING); // optional: or enable lighting for shaded spheres
    for (const SubductionCandidate & sc : cands) {
        if (sc.vertex_index >= planet.vertices.size()) continue;
        Vec3 pos = planet.vertices[sc.vertex_index];

        // choose color by type
        Vec3 col(1.0f, 0.0f, 0.0f); // default red
        switch (sc.type) {
            case SubductionCandidate::Type::Oceanic_Oceanic:      col = Vec3(1.0f, 0.8f, 0.0f); break; // yellow
            case SubductionCandidate::Type::Oceanic_Continental: col = Vec3(1.0f, 0.2f, 0.2f); break; // red
            case SubductionCandidate::Type::Continental_Continental: col = Vec3(1.0f, 0.0f, 1.0f); break; // magenta
        }

        glColor3f(col[0], col[1], col[2]);
        glPushMatrix();
            glTranslatef(pos[0], pos[1], pos[2]);
            // using glutSolidSphere for simplicity
            glutSolidSphere(markerSize, 8, 8);
        glPopMatrix();
    }
    glPopAttrib();
}