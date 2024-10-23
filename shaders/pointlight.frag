#version 430

out vec4 colorOut;

uniform sampler2D texmap;
uniform sampler2D texmap1;
uniform int texMode;

in Data {
	vec3 normal;
	vec3 eye;
	vec3 lightDir[9];
    flat int lightStates[9];
    vec2 tex_coord;
    vec4 pos;  // Position from vertex shader
} DataIn;

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

// Assimp object variables
uniform	sampler2D texUnitDiff;
// uniform	sampler2D texUnitDiff1;
uniform	sampler2D texUnitSpec;
uniform	sampler2D texUnitNormalMap;

uniform bool normalMap;  //for normal mapping
uniform bool specularMap;
uniform uint diffMapCount;

vec4 diff, auxSpec;

void main() {

	vec4 spec = vec4(0.0);
	float intensity = 0.0f;
	float intSpec = 0.0f;
    vec4 texel, texel1; 
    vec4 color = vec4(0.0, 0.0, 0.0, 1.0);
	vec3 n;

    if(normalMap)
		n = normalize(2.0 * texture(texUnitNormalMap, DataIn.tex_coord).rgb - 1.0);  //normal in tangent space
	else
		n = normalize(DataIn.normal);

    vec3 e = normalize(DataIn.eye);
    
	// Iterate over the light sources
    for (int i = 0; i < 8; i++) {

        // Check if the light is enabled
        if(DataIn.lightStates[i] == 0) {

            // Extract the light direction
            vec3 l = normalize(DataIn.lightDir[i].xyz); // Assume lightDir contains direction vectors
            intensity += max(dot(n, l), 0.0); // Calculate the diffuse intensity

            // Check if the intensity is greater than zero for specular calculation
            if (intensity > 0.0) {

                // Blinn-Phong specular calculation
                vec3 h = normalize(l + e); // Halfway vector
                float intSpec = max(dot(h, n), 0.0);
                spec += mat.specular * pow(intSpec, mat.shininess); // Specular component
            }
        }
    }

    if(mat.texCount == 0) {
		diff = mat.diffuse;
		auxSpec = mat.specular;
	}
	else {
		if(diffMapCount == 0)
			diff = mat.diffuse;
		else if(diffMapCount == 1)
			diff = mat.diffuse * texture(texUnitDiff, DataIn.tex_coord);
		// else
			// diff = mat.diffuse * texture(texUnitDiff, DataIn.tex_coord) * texture(texUnitDiff1, DataIn.tex_coord);

		if(specularMap) 
			auxSpec = mat.specular * texture(texUnitSpec, DataIn.tex_coord);
		else
			auxSpec = mat.specular;
	}
	
	// colorOut = max(intensity * mat.diffuse + spec, mat.ambient); // old
    colorOut = vec4((max(intensity * diff, diff*0.15) + spec).rgb, 1.0);

    if (texMode == 1) {  // multitexturing
        texel = texture(texmap, DataIn.tex_coord);  
        texel1 = texture(texmap1, DataIn.tex_coord);
        colorOut = max(intensity/3 *texel*texel1 + spec, 0.07*texel*texel1);
    } else if (texMode == 3) { // 2D lens flare
        texel = texture(texmap, DataIn.tex_coord);  //texel from element flare texture
		if((texel.a == 0.0)  || (mat.diffuse.a == 0.0) ) discard;
		else
			colorOut = mat.diffuse * texel;
    } else {
        colorOut = max(intensity/3 * mat.diffuse + spec, mat.ambient);
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