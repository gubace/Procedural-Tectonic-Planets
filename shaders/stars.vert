#version 330 core
layout (location = 0) in vec3 aPos;

out vec3 TexCoords;

uniform mat4 projection;
uniform mat4 view;

void main() {
    TexCoords = aPos;
    
    // Scaler le cube pour qu'il remplisse tout l'écran
    vec4 pos = projection * vec4(aPos * 1000.0, 1.0); // ajouter * view si on veut que les etoiles bougent avec la caméra
    
    // Forcer à être au fond (z = w pour que depth = 1.0)
    gl_Position = pos.xyww;
}