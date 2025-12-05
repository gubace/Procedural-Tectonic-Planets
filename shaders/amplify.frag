#version 330 core

in vec3 gWorldPos;
in vec3 gNormal;
in vec3 gColor;

out vec4 FragColor;

uniform vec3 lightDir;

void main() {
    vec3 norm = normalize(gNormal);
    vec3 light = normalize(lightDir);
    float diff = max(dot(norm, light), 0.0);
    
    vec3 ambient = 0.3 * gColor;
    vec3 diffuse = diff * gColor;
    
    FragColor = vec4(ambient + diffuse, 1.0);
}