#version 330 core

in vec3 worldPos;
in vec3 cameraPos;
in vec3 viewDir;
in vec3 normal;

out vec4 fragColor;

// Uniforms de votre code
uniform vec3 lightDir;
uniform vec3 planetCenter;
uniform float planetRadius;
uniform float atmoRadius;

// Paramètres atmosphériques - AUGMENTÉS
const float scaleHeight = 0.05; // RÉDUIT pour plus de densité
const vec3 betaRayleigh = vec3(5.5e-3, 13.0e-3, 22.4e-3) * 5.0; // AUGMENTÉ
const vec3 betaMie = vec3(21e-3) * 5.0; // AUGMENTÉ
const float mieG = 0.76;
const int numSamples = 16;
const float exposure = 1.5;
const float intensity = 20.0;

const float PI = 3.141592653589793;

// Phase functions
float phaseRayleigh(float cosTheta) {
    return (3.0 / (16.0 * PI)) * (1.0 + cosTheta * cosTheta);
}

float phaseHG(float cosTheta, float g) {
    float denom = 1.0 + g * g - 2.0 * g * cosTheta;
    return (1.0 / (4.0 * PI)) * ((1.0 - g * g) / (denom * sqrt(denom)));
}

// Ray-sphere intersection
bool raySphereIntersect(vec3 ro, vec3 rd, vec3 center, float r, out float t0, out float t1) {
    vec3 oc = ro - center;
    float b = dot(oc, rd);
    float c = dot(oc, oc) - r * r;
    float disc = b * b - c;
    if (disc < 0.0) return false;
    float sqrtD = sqrt(disc);
    t0 = -b - sqrtD;
    t1 = -b + sqrtD;
    return true;
}

// Densité atmosphérique exponentielle
float densityAtHeight(float height) {
    return exp(-max(height, 0.0) / scaleHeight);
}

// Optical depth le long d'un rayon - VERSION SIMPLIFIÉE
float computeOpticalDepth(vec3 start, vec3 dir, float maxDist) {
    float stepSize = maxDist / 8.0; // RÉDUIT le nombre de samples
    float depth = 0.0;
    
    for (int i = 0; i < 8; ++i) {
        float t = (float(i) + 0.5) * stepSize;
        vec3 pos = start + dir * t;
        float height = length(pos - planetCenter) - planetRadius;
        depth += densityAtHeight(height) * stepSize;
    }
    
    return depth;
}

void main() {
    vec3 ro = cameraPos;
    vec3 rd = normalize(worldPos - cameraPos);
    
    // Intersection avec l'atmosphère
    float t0, t1;
    if (!raySphereIntersect(ro, rd, planetCenter, atmoRadius, t0, t1)) {
        discard;
    }
    
    // Intersection avec la planète (pour arrêter le ray marching)
    float tPlanet0, tPlanet1;
    bool hitsPlanet = raySphereIntersect(ro, rd, planetCenter, planetRadius, tPlanet0, tPlanet1);
    
    // Limiter le marching
    t0 = max(t0, 0.0);
    if (hitsPlanet && tPlanet0 > 0.0) {
        t1 = min(t1, tPlanet0);
    }
    
    float segmentLength = t1 - t0;
    float step = segmentLength / float(numSamples);
    
    // Direction du soleil normalisée
    vec3 sunDir = normalize(lightDir);
    
    // Accumulateurs
    vec3 accumRayleigh = vec3(0.0);
    vec3 accumMie = vec3(0.0);
    
    // Optical depth de la caméra au point
    float opticalDepthCamera = 0.0;
    
    // Ray marching
    for (int i = 0; i < numSamples; ++i) {
        float t = t0 + (float(i) + 0.5) * step;
        vec3 pos = ro + rd * t;
        float height = length(pos - planetCenter) - planetRadius;
        
        if (height < 0.0) continue;
        
        // Densité locale
        float localDensity = densityAtHeight(height);
        
        // Accumuler l'optical depth de la caméra
        opticalDepthCamera += localDensity * step;
        
        // Optical depth vers le soleil
        float tSun0, tSun1;
        float opticalDepthSun = 0.0;
        
        if (raySphereIntersect(pos, sunDir, planetCenter, atmoRadius, tSun0, tSun1)) {
            float sunPathLength = tSun1;
            opticalDepthSun = computeOpticalDepth(pos, sunDir, sunPathLength);
        }
        
        // Optical depth total
        float totalOpticalDepth = opticalDepthCamera + opticalDepthSun;
        
        // Atténuation - VERSION PLUS DOUCE
        vec3 tauRayleigh = betaRayleigh * totalOpticalDepth;
        vec3 tauMie = betaMie * totalOpticalDepth;
        vec3 transmittance = exp(-(tauRayleigh + tauMie));
        
        // Phase functions
        float mu = dot(rd, sunDir);
        float pr = phaseRayleigh(mu);
        float pm = phaseHG(mu, mieG);
        
        // Contribution de scattering
        accumRayleigh += transmittance * betaRayleigh * pr * localDensity * step;
        accumMie += transmittance * betaMie * pm * localDensity * step;
    }
    
    // Couleur finale
    vec3 sunColor = vec3(1.0, 0.98, 0.9);
    vec3 color = (accumRayleigh + accumMie) * sunColor * intensity;
    
    // Tone mapping plus doux
    color = 1.0 - exp(-color * exposure);
    
    // Ajouter une composante ambiante pour que l'atmosphère soit visible de tous les côtés
    float ambientStrength = 10.0;
    vec3 ambient = vec3(0.4, 0.6, 1.0) * ambientStrength;
    color += ambient * length(accumRayleigh + accumMie);
    
    // Effet Fresnel pour les bords
    float fresnel = pow(1.0 - abs(dot(viewDir, normal)), 2.0);
    color += vec3(0.1, 0.15, 0.3) * fresnel;
    
    // Alpha basé sur la densité - PLUS VISIBLE
    float alpha = clamp(length(color) * 1.2, 0.1, 0.95);
    
    fragColor = vec4(color, alpha);
}