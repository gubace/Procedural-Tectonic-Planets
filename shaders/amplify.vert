#version 330 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec3 color;

out vec3 vWorldPos;
out vec3 vNormal;
out vec3 vColor;

void main() {
    vWorldPos = position;
    vNormal = normal;
    vColor = color;
}