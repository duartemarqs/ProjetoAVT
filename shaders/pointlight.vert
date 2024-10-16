#version 430

uniform mat4 m_pvm;
uniform mat4 m_viewModel;
uniform mat3 m_normal;

uniform vec4 l_pos;
uniform bool normalMap;

in vec3 position;
in vec3 normal, tangent, bitangent;    //por causa do gerador de geometria
in vec2 texCoord;

uniform vec4 pointLight0location; 
uniform vec4 pointLight1location; 
uniform vec4 pointLight2location; 
uniform vec4 pointLight3location; 
uniform vec4 pointLight4location; 
uniform vec4 pointLight5location; 
uniform vec4 directionalLightLocation;
uniform vec4 spotLight0location;
uniform vec4 spotLight1location;

uniform int pointLight0state;
uniform int pointLight1state;
uniform int pointLight2state;
uniform int pointLight3state;
uniform int pointLight4state;
uniform int pointLight5state;
uniform int directionalLightstate;
uniform int spotLight0state;
uniform int spotLight1state;

out Data {
	vec3 normal;
	vec3 eye;
	vec3 lightDir[9];
	flat int lightStates[9];
    vec2 tex_coord;
    vec4 pos;  // Added this to pass the position to the fragment shader
} DataOut;

void main () {
	vec3 n, t, b;
	vec3 lightDir, eyeDir;
	vec3 aux;

    vec4 pos = m_viewModel * vec4(position, 1.0);

	n = normalize(m_normal * normal.xyz);
	eyeDir =  vec3(-pos);

	// Transform eye and light direction vectors by tangent basis
	if(normalMap)  {
		t = normalize(m_normal * tangent.xyz);
		b = normalize(m_normal * bitangent.xyz);

		for (int i = 0; i < 9; i++) {
			aux.x = dot(lightDir, t);
			aux.y = dot(lightDir, b);
			aux.z = dot(lightDir, n);
			DataOut.lightDir[i] = normalize(aux);
		}
	} else {
		for (int i = 0; i < 9; i++) {
			DataOut.lightDir[i] = vec3(l_pos - pos);
		}
	}
	
	aux.x = dot(eyeDir, t);
	aux.y = dot(eyeDir, b);
	aux.z = dot(eyeDir, n);
	eyeDir = normalize(aux);

    DataOut.normal = n;
    DataOut.eye = vec3(-pos);
    DataOut.tex_coord = texCoord.st;
    DataOut.pos = pos;  // Pass the position to fragment shader

	DataOut.lightDir[0] = vec3(pointLight0location - pos);
	DataOut.lightDir[1] = vec3(pointLight1location - pos);
	DataOut.lightDir[2] = vec3(pointLight2location - pos);
	DataOut.lightDir[3] = vec3(pointLight3location - pos);
	DataOut.lightDir[4] = vec3(pointLight4location - pos);
	DataOut.lightDir[5] = vec3(pointLight5location - pos);
	DataOut.lightDir[6] = vec3(directionalLightLocation - pos);
	DataOut.lightDir[7] = vec3(spotLight0location - pos);
	DataOut.lightDir[8] = vec3(spotLight1location - pos);

	DataOut.lightStates[0] = int(pointLight0state);
	DataOut.lightStates[1] = int(pointLight1state);
	DataOut.lightStates[2] = int(pointLight2state);
	DataOut.lightStates[3] = int(pointLight3state);
	DataOut.lightStates[4] = int(pointLight4state);
	DataOut.lightStates[5] = int(pointLight5state);
	DataOut.lightStates[6] = int(directionalLightstate);
	DataOut.lightStates[7] = int(spotLight0state);
	DataOut.lightStates[8] = int(spotLight1state);	

	DataOut.eye = vec3(-pos);
    gl_Position = m_pvm * vec4(position, 1.0);  
}