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
    vec3 lightDir;
    vec2 tex_coord;
    vec4 pos;  // Position from vertex shader
} DataIn;

void main() {

    vec4 spec = vec4(0.0);
    vec4 texel, texel1; 

    vec3 n = normalize(DataIn.normal);
    vec3 l = normalize(DataIn.lightDir);
    vec3 e = normalize(DataIn.eye);

    float intensity = max(dot(n,l), 0.0);

    if (intensity > 0.0) {
        vec3 h = normalize(l + e);
        float intSpec = max(dot(h,n), 0.0);
        spec = mat.specular * pow(intSpec, mat.shininess);
    }

    if (texMode == 1) {  // multitexturing
        texel = texture(texmap, DataIn.tex_coord);  
        texel1 = texture(texmap1, DataIn.tex_coord);
        colorOut = max(intensity*texel*texel1 + spec, 0.07*texel*texel1);
    } else {
        colorOut = max(intensity * mat.diffuse + spec, mat.ambient);
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
