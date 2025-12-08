#version 330 core
out vec4 FragColor;
in vec3 TexCoords;

// Fonction de hash pour générer des nombres pseudo-aléatoires
float hash(vec3 p) {
    p = fract(p * 0.3183099 + 0.1);
    p *= 17.0;
    return fract(p.x * p.y * p.z * (p.x + p.y + p.z));
}

// Génère un bruit pour créer des variations d'étoiles
float noise(vec3 p) {
    vec3 i = floor(p);
    vec3 f = fract(p);
    f = f * f * (3.0 - 2.0 * f);
    
    return mix(
        mix(mix(hash(i + vec3(0,0,0)), hash(i + vec3(1,0,0)), f.x),
            mix(hash(i + vec3(0,1,0)), hash(i + vec3(1,1,0)), f.x), f.y),
        mix(mix(hash(i + vec3(0,0,1)), hash(i + vec3(1,0,1)), f.x),
            mix(hash(i + vec3(0,1,1)), hash(i + vec3(1,1,1)), f.x), f.y),
        f.z
    );
}

void main() {
    vec3 dir = normalize(TexCoords);
    
    // Fond de l'espace avec gradient subtil
    vec3 spaceColor = vec3(0.0, 0.01, 0.05);
    
    // Ajouter un gradient subtil du haut vers le bas
    float gradient = (dir.y + 1.0) * 0.5;
    spaceColor = mix(vec3(0.0, 0.0, 0.02), vec3(0.0, 0.02, 0.08), gradient);
    
    // Générer des étoiles de différentes tailles
    vec3 color = spaceColor;
    
    // Grandes étoiles brillantes (peu nombreuses)
    vec3 gridPos1 = floor(dir * 400.0);
    float star1 = hash(gridPos1);
    if (star1 > 0.995) {
        float brightness = (star1 - 0.995) * 100.0;

        brightness *= 0.8 + 0.2 * noise(dir * 100.0 + vec3(0.0, 0.0, 1.0));

        vec3 starColor = mix(vec3(0.8, 0.9, 1.0), vec3(1.0, 0.95, 0.8), hash(gridPos1 + vec3(1.0)));
        color += starColor * brightness;
    }
    
    // Étoiles moyennes
    vec3 gridPos2 = floor(dir * 800.0);
    float star2 = hash(gridPos2);
    if (star2 > 0.992) {
        float brightness = (star2 - 0.992) * 60.0;
        brightness *= 0.9 + 0.1 * noise(dir * 200.0);
        vec3 starColor = mix(vec3(0.9, 0.95, 1.0), vec3(1.0, 0.98, 0.9), hash(gridPos2 + vec3(2.0)));
        color += starColor * brightness;
    }
    
    // Petites étoiles (nombreuses)
    vec3 gridPos3 = floor(dir * 1600.0);
    float star3 = hash(gridPos3);
    if (star3 > 0.988) {
        float brightness = (star3 - 0.988) * 40.0;
        vec3 starColor = vec3(0.95, 0.97, 1.0);
        color += starColor * brightness;
    }
    
    // Très petites étoiles
    vec3 gridPos4 = floor(dir * 3000.0);
    float star4 = hash(gridPos4);
    if (star4 > 0.985) {
        float brightness = (star4 - 0.985) * 20.0;
        color += vec3(0.9, 0.9, 1.0) * brightness;
    }
    

    float nebula = noise(dir * 5.0) * 0.03;
    color += vec3(0.05, 0.03, 0.08) * nebula;
    
    FragColor = vec4(color, 1.0);
}