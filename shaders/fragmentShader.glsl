#version 330 core

in vec3 worldPos;
in vec3 cameraPos;

out vec4 fragColor;

uniform vec3 lightDir;
uniform vec3 planetCenter; // = vec3(0.0)
uniform float planetRadius; // = 1.0
uniform float atmoRadius; // = 1.05 par exemple

// Coefficients adaptés à votre échelle
#define RAY_BETA vec3(1.1e-3 * 5, 2.6e-3 * 5, 4.5e-3 * 5) 
#define MIE_BETA vec3(4.2e-3 * 5)
#define ABSORPTION_BETA vec3(2.04e-3, 4.97e-3, 1.95e-4)
#define G 0.76
#define HEIGHT_RAY 0.08
#define HEIGHT_MIE 0.012

#define HEIGHT_ABSORPTION 0.05
#define ABSORPTION_FALLOFF 0.002

// ... copiez les fonctions calculate_scattering,
vec3 calculate_scattering(
	vec3 start, 				// the start of the ray (the camera position)
	vec3 dir, 					// the direction of the ray (the camera vector)
	float max_dist, 			// the maximum distance the ray can travel (because something is in the way, like an object)
	vec3 scene_color,			// the color of the scene
	vec3 light_dir, 			// the direction of the light
	vec3 light_intensity,		// how bright the light is, affects the brightness of the atmosphere
	vec3 planet_position, 		// the position of the planet
	float planet_radius, 		// the radius of the planet
	float atmo_radius, 			// the radius of the atmosphere
	vec3 beta_ray, 				// the amount rayleigh scattering scatters the colors (for earth: causes the blue atmosphere)
	vec3 beta_mie, 				// the amount mie scattering scatters colors
	vec3 beta_absorption,   	// how much air is absorbed
	vec3 beta_ambient,			// the amount of scattering that always occurs, cna help make the back side of the atmosphere a bit brighter
	float g, 					// the direction mie scatters the light in (like a cone). closer to -1 means more towards a single direction
	float height_ray, 			// how high do you have to go before there is no rayleigh scattering?
	float height_mie, 			// the same, but for mie
	float height_absorption,	// the height at which the most absorption happens
	float absorption_falloff,	// how fast the absorption falls off from the absorption height
	int steps_i, 				// the amount of steps along the 'primary' ray, more looks better but slower
	int steps_l 				) {// the amount of steps along the light ray, more looks better but slower
	// add an offset to the camera position, so that the atmosphere is in the correct position
	start -= planet_position;
	// calculate the start and end position of the ray, as a distance along the ray
	// we do this with a ray sphere intersect
	float a = dot(dir, dir);
	float b = 2.0 * dot(dir, start);
	float c = dot(start, start) - (atmo_radius * atmo_radius);
	float d = (b * b) - 4.0 * a * c;
	
	// stop early if there is no intersect
	if (d < 0.0) return scene_color;
	
	// calculate the ray length
	vec2 ray_length = vec2(
		max((-b - sqrt(d)) / (2.0 * a), 0.0),
		min((-b + sqrt(d)) / (2.0 * a), max_dist)
	);
	
	// if the ray did not hit the atmosphere, return a black color
	if (ray_length.x > ray_length.y) return scene_color;
	// prevent the mie glow from appearing if there's an object in front of the camera
	bool allow_mie = max_dist > ray_length.y;
	// make sure the ray is no longer than allowed
	ray_length.y = min(ray_length.y, max_dist);
	ray_length.x = max(ray_length.x, 0.0);
	// get the step size of the ray
	float step_size_i = (ray_length.y - ray_length.x) / float(steps_i);
	
	// next, set how far we are along the ray, so we can calculate the position of the sample
	// if the camera is outside the atmosphere, the ray should start at the edge of the atmosphere
	// if it's inside, it should start at the position of the camera
	// the min statement makes sure of that
	float ray_pos_i = ray_length.x + step_size_i * 0.5;
	
	// these are the values we use to gather all the scattered light
	vec3 total_ray = vec3(0.0); // for rayleigh
	vec3 total_mie = vec3(0.0); // for mie
	
	// initialize the optical depth. This is used to calculate how much air was in the ray
	vec3 opt_i = vec3(0.0);
	
	// we define the density early, as this helps doing integration
	// usually we would do riemans summing, which is just the squares under the integral area
	// this is a bit innefficient, and we can make it better by also taking the extra triangle at the top of the square into account
	// the starting value is a bit inaccurate, but it should make it better overall
	vec3 prev_density = vec3(0.0);
	
	// also init the scale height, avoids some vec2's later on
	vec2 scale_height = vec2(height_ray, height_mie);
	
	// Calculate the Rayleigh and Mie phases.
	// This is the color that will be scattered for this ray
	// mu, mumu and gg are used quite a lot in the calculation, so to speed it up, precalculate them
	float mu = dot(dir, light_dir);
	float mumu = mu * mu;
	float gg = g * g;
	float phase_ray = 3.0 / (50.2654824574 /* (16 * pi) */) * (1.0 + mumu);
	float phase_mie = allow_mie ? 3.0 / (25.1327412287 /* (8 * pi) */) * ((1.0 - gg) * (mumu + 1.0)) / (pow(1.0 + gg - 2.0 * mu * g, 1.5) * (2.0 + gg)) : 0.0;
	
	// now we need to sample the 'primary' ray. this ray gathers the light that gets scattered onto it
	for (int i = 0; i < steps_i; ++i) {
		
		// calculate where we are along this ray
		vec3 pos_i = start + dir * ray_pos_i;
		
		// and how high we are above the surface
		float height_i = length(pos_i) - planet_radius;
		
		// now calculate the density of the particles (both for rayleigh and mie)
		vec3 density = vec3(exp(-height_i / scale_height), 0.0);
		
		// and the absorption density. this is for ozone, which scales together with the rayleigh, 
		// but absorbs the most at a specific height, so use the sech function for a nice curve falloff for this height
		// clamp it to avoid it going out of bounds. This prevents weird black spheres on the night side
		float denom = (height_absorption - height_i) / absorption_falloff;
		density.z = (1.0 / (denom * denom + 1.0)) * density.x;
		
		// multiply it by the step size here
		// we are going to use the density later on as well
		density *= step_size_i;
		
		// Add these densities to the optical depth, so that we know how many particles are on this ray.
		// max here is needed to prevent opt_i from potentially becoming negative
		opt_i += density;
		
		// and update the previous density
		prev_density = density;

		// Calculate the step size of the light ray.
		// again with a ray sphere intersect
		// a, b, c and d are already defined
		a = dot(light_dir, light_dir);
		b = 2.0 * dot(light_dir, pos_i);
		c = dot(pos_i, pos_i) - (atmo_radius * atmo_radius);
		d = (b * b) - 4.0 * a * c;

		// no early stopping, this one should always be inside the atmosphere
		// calculate the ray length
		float step_size_l = (-b + sqrt(d)) / (2.0 * a * float(steps_l));

		// and the position along this ray
		// this time we are sure the ray is in the atmosphere, so set it to 0
		float ray_pos_l = step_size_l * 0.5;

		// and the optical depth of this ray
		vec3 opt_l = vec3(0.0);
		
		// again, use the prev density for better integration
		vec3 prev_density_l = vec3(0.0);
		
		// now sample the light ray
		// this is similar to what we did before
		for (int l = 0; l < steps_l; ++l) {

			// calculate where we are along this ray
			vec3 pos_l = pos_i + light_dir * ray_pos_l;

			// the heigth of the position
			float height_l = length(pos_l) - planet_radius;

			// calculate the particle density, and add it
			// this is a bit verbose
			// first, set the density for ray and mie
			vec3 density_l = vec3(exp(-height_l / scale_height), 0.0);
			
			// then, the absorption
			float denom = (height_absorption - height_l) / absorption_falloff;
			density_l.z = (1.0 / (denom * denom + 1.0)) * density_l.x;
			
			// multiply the density by the step size
			density_l *= step_size_l;
			
			// and add it to the total optical depth
			opt_l += density_l;
			
			// and update the previous density
			prev_density_l = density_l;

			// and increment where we are along the light ray.
			ray_pos_l += step_size_l;
			
		}
		
		// Now we need to calculate the attenuation
		// this is essentially how much light reaches the current sample point due to scattering
		vec3 attn = exp(-beta_ray * (opt_i.x + opt_l.x) - beta_mie * (opt_i.y + opt_l.y) - beta_absorption * (opt_i.z + opt_l.z));

		// accumulate the scattered light (how much will be scattered towards the camera)
		total_ray += density.x * attn;
		total_mie += density.y * attn;

		// and increment the position on this ray
		ray_pos_i += step_size_i;
		
	}
	
	// calculate how much light can pass through the atmosphere
	vec3 opacity = exp(-(beta_mie * opt_i.y + beta_ray * opt_i.x + beta_absorption * opt_i.z));
	
	// calculate and return the final color
	return (
		phase_ray * beta_ray * total_ray // rayleigh color
	   	+ phase_mie * beta_mie * total_mie // mie
		+ opt_i.x * beta_ambient // and ambient
	) * light_intensity + scene_color * opacity; // now make sure the background is rendered correctly
}

vec2 ray_sphere_intersect(
	vec3 start, // starting position of the ray
	vec3 dir, // the direction of the ray
	float radius) {
	// ray-sphere intersection that assumes
	// the sphere is centered at the origin.
	// No intersection when result.x > result.y
	float a = dot(dir, dir);
	float b = 2.0 * dot(dir, start);
	float c = dot(start, start) - (radius * radius);
	float d = (b*b) - 4.0*a*c;
	if (d < 0.0) return vec2(1e5,-1e5);
	return vec2(
		(-b - sqrt(d))/(2.0*a),
		(-b + sqrt(d))/(2.0*a)
	);
}




void main() {
    vec3 rayDir = normalize(worldPos - cameraPos);
    vec3 rayOrigin = cameraPos;
    
    // Calculer l'intersection avec la planète pour déterminer max_dist
    vec2 planet_intersect = ray_sphere_intersect(rayOrigin - planetCenter, rayDir, planetRadius);
    
    // Si on voit la planète, limiter la distance, sinon utiliser une grande valeur
    float max_dist = (planet_intersect.x > 0.0 && planet_intersect.x < 1000.0) ? planet_intersect.x : 10.0;
    
    // Render avec l'atmosphère
    vec3 col = calculate_scattering(
        rayOrigin,
        rayDir,
        max_dist,
        vec3(0.1,0.1,0.1),
        normalize(lightDir),
        vec3(30.0),
        planetCenter,
        planetRadius,
        atmoRadius,
        RAY_BETA * 10.0,
        MIE_BETA * 15.0,
        ABSORPTION_BETA,
        vec3(0.0),
        G,
        HEIGHT_RAY,
        HEIGHT_MIE,
        HEIGHT_ABSORPTION,
        ABSORPTION_FALLOFF,
        16,
        8
    );
    
    // Tone mapping
    col = 1.0 - exp(-col * 0.5); // Ajustement du tone mapping
    
    // Alpha basé sur la densité atmosphérique
    float alpha = clamp(length(col) * 1.5, 0.0, 0.95);
    
    fragColor = vec4(col, alpha);
}

/*void main() {
    // Test simple : afficher en bleu semi-transparent
    vec3 toCamera = normalize(cameraPos - worldPos);
    float fresnel = 1.0 - abs(dot(toCamera, normalize(worldPos - planetCenter)));
    
    vec3 blueColor = vec3(0.6, 0.3, 1.0);
    float alpha = fresnel * 0.5;
    
    fragColor = vec4(blueColor, alpha);
}*/