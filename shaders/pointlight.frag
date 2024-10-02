#version 430

out vec4 colorOut;

struct Materials {
	vec4 diffuse;
	vec4 ambient;
	vec4 specular;
	vec4 emissive;
	float shininess;
};

uniform Materials mat;

in Data {
	vec3 normal;
	vec3 eye;
	vec3 lightDir[6];
} DataIn;

void main() {

	vec4 spec = vec4(0.0);
	float intensity = 0.0f;
	float intSpec = 0.0f;

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

	// Iterate over the light sources
    for (int i = 0; i < 6; i++) {
        vec3 l = normalize(DataIn.lightDir[i]); // Get the light direction
        float intensity = max(dot(n, l), 0.0); // Calculate the diffuse intensity
        
        if (intensity > 0.0) {
            // Blinn-Phong specular calculation
            vec3 h = normalize(l + e); // Halfway vector
            float intSpec = max(dot(h, n), 0.0);
            spec += mat.specular * pow(intSpec, mat.shininess); // Specular component

            // Accumulate the diffuse and specular results from each light
//            finalDiffuse += intensity * mat.diffuse * lightColors[i]; // Add diffuse lighting
//            finalSpecular += spec * lightColors[i]; // Add specular lighting
        }
    }
	
	colorOut = max(intensity * mat.diffuse + spec, mat.ambient);
}