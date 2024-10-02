#version 430

uniform mat4 m_pvm;
uniform mat4 m_viewModel;
uniform mat3 m_normal;

//uniform vec4 l_pos;

in vec4 position;
in vec4 normal;    //por causa do gerador de geometria
in vec4 texCoord;

uniform vec4 pointLight0location;
uniform vec4 pointLight1location;
uniform vec4 pointLight2location;
uniform vec4 pointLight3location;
uniform vec4 pointLight4location;
uniform vec4 pointLight5location;

out Data {
	vec3 normal;
	vec3 eye;
	vec3 lightDir[6];
    vec2 tex_coord;
    vec4 pos;  // Added this to pass the position to the fragment shader
} DataOut;

void main () {
    vec4 pos = m_viewModel * position;

    DataOut.normal = normalize(m_normal * normal.xyz);
//    DataOut.lightDir = vec3(l_pos - pos);
    DataOut.eye = vec3(-pos);
    DataOut.tex_coord = texCoord.st;
    DataOut.pos = pos;  // Pass the position to fragment shader

	DataOut.normal = normalize(m_normal * normal.xyz);
	DataOut.lightDir[0] = vec3(pointLight0location - pos);
	DataOut.lightDir[1] = vec3(pointLight1location - pos);
	DataOut.lightDir[2] = vec3(pointLight2location - pos);
	DataOut.lightDir[3] = vec3(pointLight3location - pos);
	DataOut.lightDir[4] = vec3(pointLight4location - pos);
	DataOut.lightDir[5] = vec3(pointLight5location - pos);
	DataOut.eye = vec3(-pos);
    gl_Position = m_pvm * position;  
}
