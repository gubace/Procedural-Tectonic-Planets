#version 330 core

layout(location = 0) in vec3 position;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;
uniform vec3 cameraPosition;

out vec3 worldPos;
out vec3 cameraPos;

void main() {
    worldPos = (model * vec4(position, 1.0)).xyz;

    cameraPos = cameraPosition;
    
    gl_Position = projection * view * model * vec4(position, 1.0);
}