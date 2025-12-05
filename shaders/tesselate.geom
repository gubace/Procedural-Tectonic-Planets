#version 330 core

layout(triangles) in;
layout(triangle_strip, max_vertices = 12) out;

in vec3 vWorldPos[];
in vec3 vNormal[];
in vec3 vColor[];

out vec3 gWorldPos;
out vec3 gNormal;
out vec3 gColor;

uniform mat4 projection;
uniform mat4 view;
uniform float planetRadius;

void emitVertex(vec3 pos, vec3 norm, vec3 col) {
    // Normaliser la position pour garder sur la sphère
    gWorldPos = normalize(pos) * planetRadius;
    gNormal = normalize(norm);
    gColor = vec3((1.0,0.0,0.0));
    gl_Position = projection * view * vec4(gWorldPos, 1.0);
    EmitVertex();
}

void main() {
    vec3 p0 = vWorldPos[0];
    vec3 p1 = vWorldPos[1];
    vec3 p2 = vWorldPos[2];
    
    vec3 n0 = vNormal[0];
    vec3 n1 = vNormal[1];
    vec3 n2 = vNormal[2];
    
    vec3 c0 = vColor[0];
    vec3 c1 = vColor[1];
    vec3 c2 = vColor[2];
    
    // Points milieux sur la sphère
    vec3 p01 = normalize(p0 + p1) * planetRadius;
    vec3 p12 = normalize(p1 + p2) * planetRadius;
    vec3 p20 = normalize(p2 + p0) * planetRadius;
    
    // Normales interpolées
    vec3 n01 = normalize(n0 + n1);
    vec3 n12 = normalize(n1 + n2);
    vec3 n20 = normalize(n2 + n0);
    
    // Couleurs interpolées
    vec3 c01 = (c0 + c1) * 0.5;
    vec3 c12 = (c1 + c2) * 0.5;
    vec3 c20 = (c2 + c0) * 0.5;
    
    // Triangle central
    emitVertex(p01, n01, c01);
    emitVertex(p12, n12, c12);
    emitVertex(p20, n20, c20);
    EndPrimitive();
    
    // Triangle coin 0
    emitVertex(p0, n0, c0);
    emitVertex(p01, n01, c01);
    emitVertex(p20, n20, c20);
    EndPrimitive();
    
    // Triangle coin 1
    emitVertex(p1, n1, c1);
    emitVertex(p12, n12, c12);
    emitVertex(p01, n01, c01);
    EndPrimitive();
    
    // Triangle coin 2
    emitVertex(p2, n2, c2);
    emitVertex(p20, n20, c20);
    emitVertex(p12, n12, c12);
    EndPrimitive();
}