#version 330 core

layout(location = 0) in vec3 position;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;
uniform vec3 cameraPosition;

out vec3 worldPos;
out vec3 cameraPos;
out vec3 viewDir;
out vec3 normal;

void main() {
    
    vec4 worldPosition = model * vec4(position, 1.0);
    worldPos = worldPosition.xyz;
    
    
    cameraPos = cameraPosition;
    
    
    viewDir = normalize(cameraPosition - worldPos);
    
    
    normal = normalize(worldPos);
    
    gl_Position = projection * view * worldPosition;
}