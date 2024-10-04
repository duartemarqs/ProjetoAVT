#version 430

uniform sampler2D texmap;
uniform sampler2D texmap1;
uniform int texMode;

out vec4 colorOut;

struct Materials {
    vec4 diffuse;
    vec4 ambient;
    vec4 specular;
    vec4 emissive;
    float shininess;
    int texCount;
};

uniform Materials mat;

// Fog variables
uniform bool enableFog;  // Fog toggle
uniform vec3 fogColor = vec3(0.5, 0.6, 0.7);  // Color of the fog
uniform float fogDensity = 0.02;  // Density for the exponential fog

in Data {
	vec3 normal;
	vec3 eye;
	vec4 lightDir[9];
    int lightStates[9];
    vec2 tex_coord;
    vec4 pos;  // Position from vertex shader
} DataIn;

void main() {

	vec4 spec = vec4(0.0);
	float intensity = 0.0f;
	float intSpec = 0.0f;
    vec4 texel, texel1; 
    vec4 color = vec4(0.0, 0.0, 0.0, 1.0);


	vec3 n = normalize(DataIn.normal);
//	vec3 l = normalize(DataIn.lightDir); old formula lcoation
	vec3 e = normalize(DataIn.eye);
	
	// old way
//	vec3 l = normalize(DataIn.lightDir);
//	intensity = max(dot(n,l), 0.0);
//
//	
//	if (intensity > 0.0) {
//		vec3 h = normalize(l + e);
//		float intSpec = max(dot(h,n), 0.0);
//		spec = mat.specular * pow(intSpec, mat.shininess);
//	}
    
//    // Iterate over the light sources
    for (int i = 0; i < 9; i++) {
        // Check if the light is enabled
        if(DataIn.lightStates[i] == 0) {
            // Extract the light direction
            vec3 l = normalize(DataIn.lightDir[i].xyz); // Assume lightDir contains direction vectors
            intensity = max(dot(n, l), 0.0); // Calculate the diffuse intensity

            // Check if the intensity is greater than zero for specular calculation
            if (intensity > 0.0) {
                // Blinn-Phong specular calculation
                vec3 h = normalize(l + e); // Halfway vector
                float intSpec = max(dot(h, n), 0.0);
                spec += mat.specular * pow(intSpec, mat.shininess); // Specular component
            }
        }
    }
    
    
	
	colorOut = max(intensity/2 * mat.diffuse + spec, mat.ambient);

    if (texMode == 1) {  // multitexturing
        texel = texture(texmap, DataIn.tex_coord);  
        texel1 = texture(texmap1, DataIn.tex_coord);
        colorOut = max(intensity/2*texel*texel1 + spec, 0.07*texel*texel1);
    } else {
        colorOut = max(intensity/2 * mat.diffuse + spec, mat.ambient);
    }

    // Calculate fog if enabled
    if (enableFog) {
        float dist = length(DataIn.pos);  // Range-based distance
        float fogAmount = exp(-dist * fogDensity);  // Exponential fog formula
        fogAmount = clamp(fogAmount, 0.0, 1.0);  // Clamping fog amount
        vec3 finalColor = mix(fogColor, colorOut.rgb, fogAmount);  // Apply fog
        colorOut = vec4(finalColor, 1.0);  // Output final color with fog
    }
}
