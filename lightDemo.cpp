//
// AVT: Phong Shading and Text rendered with FreeType library
// The text rendering was based on https://learnopengl.com/In-Practice/Text-Rendering
// This demo was built for learning purposes only.
// Some code could be severely optimised, but I tried to
// keep as simple and clear as possible.
//
// The code comes with no warranties, use it at your own risk.
// You may use it, or parts of it, wherever you want.
// 
// Author: João Madeiras Pereira
//

#include <math.h>
#include <iostream>
#include <sstream>
#include <string>
#include <cstdlib>

// include GLEW to access OpenGL 3.3 functions
#include <GL/glew.h>


// GLUT is the toolkit to interface with the OS
#include <GL/freeglut.h>

#include <IL/il.h>


// Use Very Simple Libs
#include "VSShaderlib.h"
#include "AVTmathLib.h"
#include "VertexAttrDef.h"
#include "geometry.h"

#include "avtFreeType.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#define NUM_POINT_LIGHTS 6
#endif

#define NUM_CREATURES 8
#define MAX_RADIUS 25.0

using namespace std;

#define CAPTION "AVT Demo: Phong Shading and Text rendered with FreeType"
int WindowHandle = 0;
int WinX = 1024, WinY = 768;

unsigned int FrameCount = 0;

//shaders
VSShaderLib shader;  //geometry
VSShaderLib shaderText;  //render bitmap text

//File with the font
const string font_name = "fonts/arial.ttf";

//Vectors with meshes
vector<struct MyMesh> myMeshes;
vector<struct MyMesh> movingMeshes;
vector<struct MyMesh> houseMeshes;
vector<struct MyMesh> treeMeshes;
vector<struct MyMesh> boatMeshes;
vector<struct MyMesh> rowMeshes;

//External array storage defined in AVTmathLib.cpp

/// The storage for matrices
extern float mMatrix[COUNT_MATRICES][16];
extern float mCompMatrix[COUNT_COMPUTED_MATRICES][16];

/// The normal matrix
extern float mNormal3x3[9];

GLint pvm_uniformId;
GLint vm_uniformId;
GLint normal_uniformId;
GLint lPos_uniformId;
GLint pointlights_loc;
GLint pointlights_loc1;
GLint pointlights_loc2;
GLint pointlights_loc3;
GLint pointlights_loc4;
GLint pointlights_loc5;
GLint pointlights_loc6;
GLint tex_loc, tex_loc1, tex_loc2;
	
// Camera Position
float camX, camY, camZ;

// Mouse Tracking Variables
int startX, startY, tracking = 0;

// Camera Spherical Coordinates
float alpha = 39.0f, beta = 51.0f;
float r = 10.0f;

// Frame counting and FPS computation
long myTime,timebase = 0,frame = 0;
char s[32];
float lightPos[4] = {4.0f, 6.0f, 2.0f, 1.0f};

// Camera -------------
class Camera {
public:
	float camPos[3] = { 0,0,0 };
	float camTarget[3] = { 0.0f, 0.0f, 0.0f };
	int type = 0; //0 - perspective , 1 - ortho
};

// global declaration 
Camera cams[4];
int activeCam = 0;


// Boat ---------------
class Boat {
public:
	float speed;
	float direction[3];
	float pos[3];
	float angle;  
};

Boat boat;
float deltaT = 0.05;
float decaySpeed = 0.99;  

// Water creatures
class WaterCreature {
public:
	float speed;
	float direction[3];
	float pos[3];
	float angle;
	float oscillationTime;	// Tempo acumulado para controlar a oscilação
	float timeElapsed;		// Tempo decorrido desde o último aumento de velocidade

	WaterCreature() : timeElapsed(0.0f) {};
};

float deltaTimeElapsed = 1.0f / 60.0f;	// for 60 FPS

vector<struct WaterCreature> waterCreatures;

class PointLight {
public:
	// directional light position
	//float directionalLightPos[4]{ 1.0f, 1000.0f, 1.0f, 0.0f };
	// random pointlights positions for lights
	float position[4];
	float color[4];
	GLint location = 0;
	bool enabled = true;
	//float color[4]; // Color of the light (r, g, b, a)
	//GLint pointlightLocations[NUM_POINT_LIGHTS];
	//GLint pointlightLocations;	
	// 

	// Constructor
	PointLight(const float pos[4], const float col[4]) {
		// Copy the values of pos and col into the member arrays
		for (int i = 0; i < 4; ++i) {
			position[i] = pos[i];
		}
		for (int i = 0; i < 4; ++i) {
			color[i] = col[i];
		}
		enabled = true;
	}

	// Getter method for pointLightPosition
	const float* getPointLightPosition() const {
		return position;
	}
	// Setter method for pointLightPosition
	void setPointLightPosition(const float newPosition[4]) {
		// Use a loop to copy each element
		for (int i = 0; i < 4; i++) {
			position[i] = newPosition[i];
		}
	}

};



float positions[NUM_POINT_LIGHTS][4] = {
		{ 3.0f, 3.0f, 3.0f, 1.0f },
		{ 7.0f, 7.0f, 7.0f, 1.0f },
		{ 2.0f, 10.0f, 2.0f, 1.0f },
		{ 5.0f, 5.0f, 5.0f, 1.0f },
		{ 17.0f, 8.0f, 7.0f, 1.0f },
		{ 1.0f, 2.0f, 1.0f, 1.0f }
};

float colors[6][4] = { 
	{0.3f, 0.7f, 0.2f, 1.0f},  // Greenish
	{0.8f, 0.1f, 0.3f, 1.0f},  // Reddish
	{0.2f, 0.4f, 0.8f, 1.0f},  // Blueish
	{0.9f, 0.8f, 0.1f, 1.0f},  // Yellowish
	{0.5f, 0.2f, 0.6f, 1.0f},  // Purplish
	{0.1f, 0.9f, 0.7f, 1.0f}   // Tealish
};

std::vector<PointLight> pointLights;
//PointLight pointLights[6] = {	PointLight(positions[0], colors[0]), 
//								PointLight(positions[1], colors[1]), };



//endof setup of pointlights


// Função para gerar um valor aleatório entre min e max
float randomFloat(float min, float max) {
	return min + static_cast<float>(rand()) / (static_cast<float>(RAND_MAX / (max - min)));
}

// Função para inicializar uma criatura com posição e direção aleatórias
void initCreature(WaterCreature& creature, float maxRadius) {
	// Define posição aleatória no plano (x, z) e y = 0
	creature.pos[0] = randomFloat(-maxRadius, maxRadius);  // posição x
	creature.pos[1] = 0.0f;                                // posição y (nível da água)
	creature.pos[2] = randomFloat(-maxRadius, maxRadius);  // posição z

	// Define uma direção aleatória
	creature.direction[0] = randomFloat(-1.0f, 1.0f); // direção x
	creature.direction[1] = 0.0f;                     // direção y (sempre 0)
	creature.direction[2] = randomFloat(-1.0f, 1.0f); // direção z

	// Normaliza a direção para garantir que tenha magnitude 1
	float magnitude = sqrt(creature.direction[0] * creature.direction[0] +
		creature.direction[2] * creature.direction[2]);
	creature.direction[0] /= magnitude;
	creature.direction[2] /= magnitude;

	// Define uma velocidade e ângulo aleatórios
	creature.speed = randomFloat(0.5f, 1.0f);       
	creature.angle = randomFloat(0.0f, 360.0f);			// ângulo de rotação
	creature.oscillationTime = 0.0f;					// tempo de oscilação
}

// Inicializa N criaturas com posições aleatórias
void initializeCreatures(int numCreatures, float maxRadius) {
	waterCreatures.clear();
	for (int i = 0; i < numCreatures; i++) {
		WaterCreature creature;
		initCreature(creature, maxRadius);
		waterCreatures.push_back(creature);
	}
}

void updateCreatures(float deltaT, float maxRadius) {
	for (int i = 0; i < waterCreatures.size(); i++) {
		WaterCreature& creature = waterCreatures[i];

		// Atualiza a posição com base na direção e velocidade
		creature.pos[0] += creature.direction[0] * creature.speed * deltaT;
		creature.pos[2] += creature.direction[2] * creature.speed * deltaT;

		// Atualiza o tempo de oscilação
		creature.oscillationTime += deltaT;
		creature.timeElapsed += deltaTimeElapsed;

		// Aumentar a velocidade a cada 30 segundos
		if (creature.timeElapsed >= 30.0f) {
			creature.speed += 1.0;
			creature.timeElapsed = 0.0f; // Reseta o contador de tempo
		}

		// Define a oscilação angular (30 graus para ambos os lados)
		float oscillationAmplitude = 30.0f;  // Amplitude máxima de oscilação (30º)
		float oscillationFrequency = 2.0f;   // Velocidade da oscilação (ajustável)

		// Ângulo de oscilação baseado no seno do tempo
		float oscillationAngle = oscillationAmplitude * sin(oscillationFrequency * creature.oscillationTime);

		// Aplica o ângulo oscilante à rotação da criatura (somente para a visualização)
		creature.angle = oscillationAngle;

		// Verifica se ultrapassou o raio limite
		float distanceFromCenter = sqrt(creature.pos[0] * creature.pos[0] +
			creature.pos[2] * creature.pos[2]);
		
		// Se ultrapassou o raio limite, reinicializa a criatura
		if (distanceFromCenter > maxRadius) {
			initCreature(creature, maxRadius);
		}
	}
}


void timer(int value)
{
	std::ostringstream oss;
	oss << CAPTION << ": " << FrameCount << " FPS @ (" << WinX << "x" << WinY << ")";
	std::string s = oss.str();
	glutSetWindow(WindowHandle);
	glutSetWindowTitle(s.c_str());
    FrameCount = 0;
    glutTimerFunc(1000, timer, 0);
}

void animation(int value) {
	// Atualiza a direção do barco
	boat.direction[0] = sin(boat.angle * M_PI / 180);
	boat.direction[1] = 0;
	boat.direction[2] = cos(boat.angle * M_PI / 180);

	// Atualiza a posição do barco
	boat.pos[0] += boat.direction[0] * boat.speed * deltaT;
	boat.pos[1] += boat.direction[1] * boat.speed * deltaT;
	boat.pos[2] += boat.direction[2] * boat.speed * deltaT;

	if (boat.speed > 0) boat.speed *= decaySpeed;

	float dist = -10.0f;
	float height = 7.5f;

	// Atualiza a posição da câmara atrás do barco
	cams[2].camPos[0] = boat.pos[0] + boat.direction[0] * dist;
	cams[2].camPos[1] = boat.pos[1] + height;
	cams[2].camPos[2] = boat.pos[2] + boat.direction[2] * dist;

	cams[2].camTarget[0] = boat.pos[0];
	cams[2].camTarget[1] = boat.pos[1];
	cams[2].camTarget[2] = boat.pos[2];

	// Atualiza as criaturas
	updateCreatures(deltaT, MAX_RADIUS);

	// Volta a chamar animation
	glutTimerFunc(1 / deltaT, animation, 0);
}

void refresh(int value)
{
	// Atualiza a cena
	glutPostRedisplay();

	// Nova chamada ao temporizador após 16 milissegundos
	glutTimerFunc(1000 / 60, refresh, 0);
}

// ------------------------------------------------------------
//
// Reshape Callback Function
//

void changeSize(int w, int h) {

	float ratio;
	// Prevent a divide by zero, when window is too short
	if(h == 0)
		h = 1;
	// set the viewport to be the entire window
	glViewport(0, 0, w, h);
	// set the projection matrix
	ratio = (1.0f * w) / h;
	loadIdentity(PROJECTION);
	perspective(53.13f, ratio, 0.1f, 1000.0f);
}


// ------------------------------------------------------------
//
// Render stufff
//

/*
*/
void renderCreatures(GLint loc) {
	int meshId = 0;
	pushMatrix(MODEL);

	do {
		// printf("Dentro do while, meshId = %d\n", meshId);
		// send the material
		loc = glGetUniformLocation(shader.getProgramIndex(), "mat.ambient");
		glUniform4fv(loc, 1, movingMeshes[meshId].mat.ambient);
		loc = glGetUniformLocation(shader.getProgramIndex(), "mat.diffuse");
		glUniform4fv(loc, 1, movingMeshes[meshId].mat.diffuse);
		loc = glGetUniformLocation(shader.getProgramIndex(), "mat.specular");
		glUniform4fv(loc, 1, movingMeshes[meshId].mat.specular);
		loc = glGetUniformLocation(shader.getProgramIndex(), "mat.shininess");
		glUniform1f(loc, movingMeshes[meshId].mat.shininess);
		pushMatrix(MODEL);

		// Transladar criatura para a sua posição atual
		translate(MODEL, waterCreatures[meshId].pos[0], waterCreatures[meshId].pos[1], waterCreatures[meshId].pos[2]);
		// Rodar criatura em torno do eixo Y (oscilações)
		rotate(MODEL, waterCreatures[meshId].angle, 0.0f, 1.0f, 0.0f);

		// Dar uma diferente inclinação a cada creature (cone)
		/*
		*/
		if (meshId % 2 == 0) {
			// translate(MODEL, 2.0, 0.0, 4.0);
			rotate(MODEL, 60, 0.0, 0.0, 1.0);
		}
		else {
			// translate(MODEL, 4.0, 0.0, 8.0);
			rotate(MODEL, 90, 0.0, 0.0, 1.0);
		}

		// send matrices to OGL
		computeDerivedMatrix(PROJ_VIEW_MODEL);
		glUniformMatrix4fv(vm_uniformId, 1, GL_FALSE, mCompMatrix[VIEW_MODEL]);
		glUniformMatrix4fv(pvm_uniformId, 1, GL_FALSE, mCompMatrix[PROJ_VIEW_MODEL]);
		computeNormalMatrix3x3();
		glUniformMatrix3fv(normal_uniformId, 1, GL_FALSE, mNormal3x3);

		// Render mesh
		glBindVertexArray(movingMeshes[meshId].vao);

		glDrawElements(movingMeshes[meshId].type, movingMeshes[meshId].numIndexes, GL_UNSIGNED_INT, 0);
		glBindVertexArray(0);

		popMatrix(MODEL);
		meshId++;
	} while (meshId < movingMeshes.size());

	popMatrix(MODEL);
}

void renderTree(GLint loc) {
	int meshId = 0;
	pushMatrix(MODEL);

	do {
		// printf("Dentro do while, meshId = %d\n", meshId);
		// send the material
		loc = glGetUniformLocation(shader.getProgramIndex(), "mat.ambient");
		glUniform4fv(loc, 1, treeMeshes[meshId].mat.ambient);
		loc = glGetUniformLocation(shader.getProgramIndex(), "mat.diffuse");
		glUniform4fv(loc, 1, treeMeshes[meshId].mat.diffuse);
		loc = glGetUniformLocation(shader.getProgramIndex(), "mat.specular");
		glUniform4fv(loc, 1, treeMeshes[meshId].mat.specular);
		loc = glGetUniformLocation(shader.getProgramIndex(), "mat.shininess");
		glUniform1f(loc, treeMeshes[meshId].mat.shininess);
		pushMatrix(MODEL);

		// Transformations
		if (meshId == 0) { // cylinder
			translate(MODEL, -4.0, 1.75, -2.0);
		}
		else if (meshId == 1) {	// pyramid
			translate(MODEL, -4.0, 3.5, -2.0);
		}
		/*
		*/

		// send matrices to OGL
		computeDerivedMatrix(PROJ_VIEW_MODEL);
		glUniformMatrix4fv(vm_uniformId, 1, GL_FALSE, mCompMatrix[VIEW_MODEL]);
		glUniformMatrix4fv(pvm_uniformId, 1, GL_FALSE, mCompMatrix[PROJ_VIEW_MODEL]);
		computeNormalMatrix3x3();
		glUniformMatrix3fv(normal_uniformId, 1, GL_FALSE, mNormal3x3);

		// Render mesh
		glBindVertexArray(treeMeshes[meshId].vao);

		glDrawElements(treeMeshes[meshId].type, treeMeshes[meshId].numIndexes, GL_UNSIGNED_INT, 0);
		glBindVertexArray(0);

		popMatrix(MODEL);
		meshId++;
	} while (meshId < treeMeshes.size());

	popMatrix(MODEL);
}

void renderHouse(GLint loc) {
	int meshId = 0;
	pushMatrix(MODEL);

	do {
		// printf("Dentro do while, meshId = %d\n", meshId);
		// send the material
		loc = glGetUniformLocation(shader.getProgramIndex(), "mat.ambient");
		glUniform4fv(loc, 1, houseMeshes[meshId].mat.ambient);
		loc = glGetUniformLocation(shader.getProgramIndex(), "mat.diffuse");
		glUniform4fv(loc, 1, houseMeshes[meshId].mat.diffuse);
		loc = glGetUniformLocation(shader.getProgramIndex(), "mat.specular");
		glUniform4fv(loc, 1, houseMeshes[meshId].mat.specular);
		loc = glGetUniformLocation(shader.getProgramIndex(), "mat.shininess");
		glUniform1f(loc, houseMeshes[meshId].mat.shininess);
		pushMatrix(MODEL);

		// Transformations
		if (meshId == 0) { // cube
			translate(MODEL, 2.0, 0.0, 1.4);
		}
		/*
		*/
		else if (meshId == 1) {	// pyramid
			translate(MODEL, 2.5, 1.0, 1.9);
			rotate(MODEL, -45, 0, 1, 0);
		}

		// send matrices to OGL
		computeDerivedMatrix(PROJ_VIEW_MODEL);
		glUniformMatrix4fv(vm_uniformId, 1, GL_FALSE, mCompMatrix[VIEW_MODEL]);
		glUniformMatrix4fv(pvm_uniformId, 1, GL_FALSE, mCompMatrix[PROJ_VIEW_MODEL]);
		computeNormalMatrix3x3();
		glUniformMatrix3fv(normal_uniformId, 1, GL_FALSE, mNormal3x3);

		// Render mesh
		glBindVertexArray(houseMeshes[meshId].vao);

		glDrawElements(houseMeshes[meshId].type, houseMeshes[meshId].numIndexes, GL_UNSIGNED_INT, 0);
		glBindVertexArray(0);

		popMatrix(MODEL);
		meshId++;
	} while (meshId < houseMeshes.size());

	popMatrix(MODEL);
}

void renderBoat(GLint loc) {
	int meshId = 0;

	pushMatrix(MODEL);
	// 1. Transladar o barco para a sua posição atual
	translate(MODEL, boat.pos[0], boat.pos[1], boat.pos[2]);

	// 2. Rodar o barco em torno do eixo Y (depois da translação)
	rotate(MODEL, boat.angle, 0.0f, 1.0f, 0.0f);  // Rotacionar sobre o próprio eixo

	do {
		// send the material
		loc = glGetUniformLocation(shader.getProgramIndex(), "mat.ambient");
		glUniform4fv(loc, 1, boatMeshes[meshId].mat.ambient);
		loc = glGetUniformLocation(shader.getProgramIndex(), "mat.diffuse");
		glUniform4fv(loc, 1, boatMeshes[meshId].mat.diffuse);
		loc = glGetUniformLocation(shader.getProgramIndex(), "mat.specular");
		glUniform4fv(loc, 1, boatMeshes[meshId].mat.specular);
		loc = glGetUniformLocation(shader.getProgramIndex(), "mat.shininess");
		glUniform1f(loc, boatMeshes[meshId].mat.shininess);
		pushMatrix(MODEL);

		// Transformations
		if (meshId == 0) {	// base
			translate(MODEL, 0.0f, 0.0f, 0.0f);
			rotate(MODEL, 90, 1, 0, 0);
		}
		else if (meshId == 1) {	 // left wall
			translate(MODEL, -0.5f, 0.4f, 0.0f);	// baseWidth = 1.0 => 0.5baseWidth = 0.5
			rotate(MODEL, 90, 0, 1, 0);
		}
		else if (meshId == 2) {	// right wall
			translate(MODEL, 0.5f, 0.4f, 0.0f);
			rotate(MODEL, 90, 0, 1, 0);
		}
		else if (meshId == 3) { // back wall
			translate(MODEL, 0.0f, 0.4f, 1.25f);		// baseLength = 2.2 => 0.5baseLength = 1.25
		}
		else if (meshId == 4) { // front wall
			translate(MODEL, 0.0f, 0.4f, -1.25f);
		}
		else if (meshId == 5) { // prow
			translate(MODEL, 0.0f, 0.4f, 1.25f);
			rotate(MODEL, 45, 0, 0, -1);
			rotate(MODEL, 90, 1, 0, 0);
		}

		// send matrices to OGL
		computeDerivedMatrix(PROJ_VIEW_MODEL);
		glUniformMatrix4fv(vm_uniformId, 1, GL_FALSE, mCompMatrix[VIEW_MODEL]);
		glUniformMatrix4fv(pvm_uniformId, 1, GL_FALSE, mCompMatrix[PROJ_VIEW_MODEL]);
		computeNormalMatrix3x3();
		glUniformMatrix3fv(normal_uniformId, 1, GL_FALSE, mNormal3x3);

		// Render mesh
		glBindVertexArray(boatMeshes[meshId].vao);

		glDrawElements(boatMeshes[meshId].type, boatMeshes[meshId].numIndexes, GL_UNSIGNED_INT, 0);
		glBindVertexArray(0);

		popMatrix(MODEL);
		meshId++;
	} while (meshId < boatMeshes.size());

	popMatrix(MODEL);
}

void renderRows(GLint loc) {
	int meshId = 0;

	pushMatrix(MODEL);
	// 1. Transladar o barco para a sua posição atual
	translate(MODEL, boat.pos[0], boat.pos[1], boat.pos[2]);

	// 2. Rodar sobre o próprio eixo
	rotate(MODEL, boat.angle, 0.0f, 1.0f, 0.0f);  

	do {
		// send the material
		loc = glGetUniformLocation(shader.getProgramIndex(), "mat.ambient");
		glUniform4fv(loc, 1, rowMeshes[meshId].mat.ambient);
		loc = glGetUniformLocation(shader.getProgramIndex(), "mat.diffuse");
		glUniform4fv(loc, 1, rowMeshes[meshId].mat.diffuse);
		loc = glGetUniformLocation(shader.getProgramIndex(), "mat.specular");
		glUniform4fv(loc, 1, rowMeshes[meshId].mat.specular);
		loc = glGetUniformLocation(shader.getProgramIndex(), "mat.shininess");
		glUniform1f(loc, rowMeshes[meshId].mat.shininess);
		pushMatrix(MODEL);

		// Transformations
		if (meshId == 0) { // left row
			translate(MODEL, -1.0f, 0.9f, 0.0f);
			rotate(MODEL, 90, 0, 0, 1);
		}
		else if (meshId == 1) {
			translate(MODEL, -2.1f, 0.85f, 0.0f);
			rotate(MODEL, 90, 0, 0, 1);
		}
		else if (meshId == 2) { // right row
			translate(MODEL, 1.0f, 0.9f, 0.0f);
			rotate(MODEL, -90, 0, 0, 1);
		}
		else if (meshId == 3) {
			translate(MODEL, 2.1f, 0.85f, 0.0f);
			rotate(MODEL, -90, 0, 0, 1);
		}

		// send matrices to OGL
		computeDerivedMatrix(PROJ_VIEW_MODEL);
		glUniformMatrix4fv(vm_uniformId, 1, GL_FALSE, mCompMatrix[VIEW_MODEL]);
		glUniformMatrix4fv(pvm_uniformId, 1, GL_FALSE, mCompMatrix[PROJ_VIEW_MODEL]);
		computeNormalMatrix3x3();
		glUniformMatrix3fv(normal_uniformId, 1, GL_FALSE, mNormal3x3);

		// Render mesh
		glBindVertexArray(rowMeshes[meshId].vao);

		glDrawElements(rowMeshes[meshId].type, rowMeshes[meshId].numIndexes, GL_UNSIGNED_INT, 0);
		glBindVertexArray(0);

		popMatrix(MODEL);
		meshId++;
	} while (meshId < rowMeshes.size());

	popMatrix(MODEL);
}

void renderScene(void) {

	GLint loc;
	GLint m_view[4];

	FrameCount++;
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	// load identity matrices
	loadIdentity(VIEW);
	loadIdentity(MODEL);
	// set the camera using a function similar to gluLookAt
	//lookAt(camX, camY, camZ, 0,0,0, 0,1,0);
	if (activeCam == 3) lookAt(camX, camY, camZ, 0, 0, 0, 0, 1, 0);

	/*printf("Camera Position: x: %.2f, y: %.2f, z: %.2f\n",
		cams[activeCam].camPos[0], cams[activeCam].camPos[1], cams[activeCam].camPos[2]);

	printf("Camera Target: x: %.2f, y: %.2f, z: %.2f\n",
		cams[activeCam].camTarget[0], cams[activeCam].camTarget[1], cams[activeCam].camTarget[2]); */

	lookAt(cams[activeCam].camPos[0], cams[activeCam].camPos[1], cams[activeCam].camPos[2],
		cams[activeCam].camTarget[0], cams[activeCam].camTarget[1], cams[activeCam].camTarget[2],
		0, 1, 0);

	glGetIntegerv(GL_VIEWPORT, m_view);

	float ratio = ((m_view[2] - m_view[0]) / (m_view[3] - m_view[1]));
	loadIdentity(PROJECTION);

	if (cams[activeCam].type == 0) {
		perspective(53.13f, ratio, 1, 100);
	}
	else {
		ortho(ratio * -25, ratio * 25, -25, 25, 0.1, 100);
	}

	//glDisable(GL_CULL_FACE);

	// use our shader
	glUseProgram(shader.getProgramIndex());

	//send the light position in eye coordinates
	//glUniform4fv(lPos_uniformId, 1, lightPos); //efeito capacete do mineiro, ou seja lighPos foi definido em eye coord 

	float res[4];
	//multMatrixPoint(VIEW, lightPos,res);   //lightPos definido em World Coord so is converted to eye space
	//glUniform4fv(lPos_uniformId, 1, res);multMatrixPoint(VIEW, lightPos,res);   //lightPos definido em World Coord so is converted to eye space
	//glUniform4fv(lPos_uniformId, 1, res);

	// Para pointlights
	multMatrixPoint(VIEW, pointLights[0].position, res);
	glUniform4fv(pointLights[0].location, 1, res);
	multMatrixPoint(VIEW, pointLights[1].position, res);
	glUniform4fv(pointLights[1].location, 1, res);
	multMatrixPoint(VIEW, pointLights[2].position, res);
	glUniform4fv(pointLights[2].location, 1, res);
	multMatrixPoint(VIEW, pointLights[3].position, res);
	glUniform4fv(pointLights[3].location, 1, res);
	multMatrixPoint(VIEW, pointLights[4].position, res);
	glUniform4fv(pointLights[4].location, 1, res);
	multMatrixPoint(VIEW, pointLights[5].position, res);
	glUniform4fv(pointLights[5].location, 1, res);

	// int objId = 0; //id of the object mesh - to be used as index of mesh: Mymeshes[objID] means the current mesh

	// for (int i = 0; i < 2; ++i) {
		// for (int j = 0; j < 3; ++j) {

	// send the material for the terrain
	loc = glGetUniformLocation(shader.getProgramIndex(), "mat.ambient");
	glUniform4fv(loc, 1, myMeshes[0].mat.ambient);
	loc = glGetUniformLocation(shader.getProgramIndex(), "mat.diffuse");
	glUniform4fv(loc, 1, myMeshes[0].mat.diffuse);
	loc = glGetUniformLocation(shader.getProgramIndex(), "mat.specular");
	glUniform4fv(loc, 1, myMeshes[0].mat.specular);
	loc = glGetUniformLocation(shader.getProgramIndex(), "mat.shininess");
	glUniform1f(loc, myMeshes[0].mat.shininess);
	pushMatrix(MODEL);
	translate(MODEL, 0.0f, 0.0f, 0.0f);

	// if ((i == 0) && (j == 0))
	rotate(MODEL, -90, 1, 0, 0);
			
	/*
	if ((i == 1) && (j == 1)) {
		rotate(MODEL, -45, 0, 1, 0);
		translate(MODEL, 2.0, 1, 1.4);
	}
	*/
				

	// send matrices to OGL
	computeDerivedMatrix(PROJ_VIEW_MODEL);
	glUniformMatrix4fv(vm_uniformId, 1, GL_FALSE, mCompMatrix[VIEW_MODEL]);
	glUniformMatrix4fv(pvm_uniformId, 1, GL_FALSE, mCompMatrix[PROJ_VIEW_MODEL]);
	computeNormalMatrix3x3();
	glUniformMatrix3fv(normal_uniformId, 1, GL_FALSE, mNormal3x3);

	// Render mesh
	glBindVertexArray(myMeshes[0].vao);

	glDrawElements(myMeshes[0].type, myMeshes[0].numIndexes, GL_UNSIGNED_INT, 0);
	glBindVertexArray(0);

	popMatrix(MODEL);
			// objId++;
		// }
	// }


	// Render and transform boat parts into one combined mesh
	/*
	translate(MODEL, 5.0, 0.0, 0.0);
	rotate(MODEL, 30, 0.0, 1.0, 0.0);
	*/
	renderHouse(loc);
	renderTree(loc);
	renderBoat(loc);
	renderRows(loc);
	renderCreatures(loc);

	//Render text (bitmap fonts) in screen coordinates. So use ortoghonal projection with viewport coordinates.
	glDisable(GL_DEPTH_TEST);
	//the glyph contains transparent background colors and non-transparent for the actual character pixels. So we use the blending
	glEnable(GL_BLEND);  
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	int m_viewport[4];
	glGetIntegerv(GL_VIEWPORT, m_viewport);

	//viewer at origin looking down at  negative z direction
	pushMatrix(MODEL);
	loadIdentity(MODEL);
	pushMatrix(PROJECTION);
	loadIdentity(PROJECTION);
	pushMatrix(VIEW);
	loadIdentity(VIEW);
	ortho(m_viewport[0], m_viewport[0] + m_viewport[2] - 1, m_viewport[1], m_viewport[1] + m_viewport[3] - 1, -1, 1);
	RenderText(shaderText, "This is a sample text", 25.0f, 25.0f, 1.0f, 0.5f, 0.8f, 0.2f);
	RenderText(shaderText, "AVT Light and Text Rendering Demo", 440.0f, 570.0f, 0.5f, 0.3, 0.7f, 0.9f);
	popMatrix(PROJECTION);
	popMatrix(VIEW);
	popMatrix(MODEL);
	glEnable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);

	glutSwapBuffers();
}

// ------------------------------------------------------------
//
// Events from the Keyboard
//

void processKeys(unsigned char key, int xx, int yy)
{
	switch(key) {

	case '1':
		activeCam = 0;
		printf("Active Camera: %d\n", activeCam);
		printf("Camera 1 Position: (%f, %f, %f)\n", cams[activeCam].camPos[0], cams[activeCam].camPos[1], cams[activeCam].camPos[2]);
		break;

	case '2':
		activeCam = 1;
		printf("Active Camera: %d\n", activeCam);
		printf("Camera 2 Position: (%f, %f, %f)\n", cams[activeCam].camPos[0], cams[activeCam].camPos[1], cams[activeCam].camPos[2]);
		break;

	case '3':
		activeCam = 2;
		printf("Active Camera: %d\n", activeCam);
		printf("Camera 3 Position: (%f, %f, %f)\n", cams[activeCam].camPos[0], cams[activeCam].camPos[1], cams[activeCam].camPos[2]);
		break;
	case '4':
		activeCam = 3;
		printf("Active Camera: %d\n", activeCam);
		printf("Camera 3 Position: (%f, %f, %f)\n", cams[activeCam].camPos[0], cams[activeCam].camPos[1], cams[activeCam].camPos[2]);
		break;

		
	case 'a':  
		boat.angle += 5.0f;  
		break;

	case 'd':  
		boat.angle -= 5.0f;  
		break;

	case 'w': 
		boat.speed += 0.1f;  
		break;

	case 's': 
		boat.speed -= 0.1f; 
		if (boat.speed < 0) boat.speed = 0;  // Não permitir velocidade negativa
		break;
 
		case 27:
			glutLeaveMainLoop();
			break;

		case 'c': 
			printf("Camera Spherical Coordinates (%f, %f, %f)\n", alpha, beta, r);
			break;
		case 'm': glEnable(GL_MULTISAMPLE); break;
		case 'n': glDisable(GL_MULTISAMPLE); break;
	}
}


// ------------------------------------------------------------
//
// Mouse Events
//

void processMouseButtons(int button, int state, int xx, int yy)
{
	// start tracking the mouse
	if (state == GLUT_DOWN)  {
		startX = xx;
		startY = yy;
		if (button == GLUT_LEFT_BUTTON)
			tracking = 1;
		else if (button == GLUT_RIGHT_BUTTON)
			tracking = 2;
	}

	//stop tracking the mouse
	else if (state == GLUT_UP) {
		if (tracking == 1) {
			alpha -= (xx - startX);
			beta += (yy - startY);
		}
		else if (tracking == 2) {
			r += (yy - startY) * 0.01f;
			if (r < 0.1f)
				r = 0.1f;
		}
		tracking = 0;
	}
}

// Track mouse motion while buttons are pressed

void processMouseMotion(int xx, int yy)
{

	int deltaX, deltaY;
	float alphaAux, betaAux;
	float rAux;

	deltaX =  - xx + startX;
	deltaY =    yy - startY;

	// left mouse button: move camera
	if (tracking == 1) {


		alphaAux = alpha + deltaX;
		betaAux = beta + deltaY;

		if (betaAux > 85.0f)
			betaAux = 85.0f;
		else if (betaAux < -85.0f)
			betaAux = -85.0f;
		rAux = r;
	}
	// right mouse button: zoom
	else if (tracking == 2) {

		alphaAux = alpha;
		betaAux = beta;
		rAux = r + (deltaY * 0.01f);
		if (rAux < 0.1f)
			rAux = 0.1f;
	}

	camX = rAux * sin(alphaAux * 3.14f / 180.0f) * cos(betaAux * 3.14f / 180.0f);
	camZ = rAux * cos(alphaAux * 3.14f / 180.0f) * cos(betaAux * 3.14f / 180.0f);
	camY = rAux *   						       sin(betaAux * 3.14f / 180.0f);

//  uncomment this if not using an idle or refresh func
//	glutPostRedisplay();
}


void mouseWheel(int wheel, int direction, int x, int y) {

	r += direction * 0.1f;
	if (r < 0.1f)
		r = 0.1f;

	camX = r * sin(alpha * 3.14f / 180.0f) * cos(beta * 3.14f / 180.0f);
	camZ = r * cos(alpha * 3.14f / 180.0f) * cos(beta * 3.14f / 180.0f);
	camY = r *   						     sin(beta * 3.14f / 180.0f);

//  uncomment this if not using an idle or refresh func
//	glutPostRedisplay();
}

// --------------------------------------------------------
//
// Shader Stuff
//


GLuint setupShaders() {


	// Shader for models
	shader.init();
	shader.loadShader(VSShaderLib::VERTEX_SHADER, "shaders/pointlight.vert");
	shader.loadShader(VSShaderLib::FRAGMENT_SHADER, "shaders/pointlight.frag");

	// set semantics for the shader variables
	glBindFragDataLocation(shader.getProgramIndex(), 0,"colorOut");
	glBindAttribLocation(shader.getProgramIndex(), VERTEX_COORD_ATTRIB, "position");
	glBindAttribLocation(shader.getProgramIndex(), NORMAL_ATTRIB, "normal");
	//glBindAttribLocation(shader.getProgramIndex(), TEXTURE_COORD_ATTRIB, "texCoord");

	glLinkProgram(shader.getProgramIndex());

	printf("InfoLog for Per Fragment Phong Lightning Shader\n%s\n\n", shader.getAllInfoLogs().c_str());

	if (!shader.isProgramValid()) {
		printf("GLSL Model Program Not Valid!\n");
		printf("InfoLog\n%s\n\n", shader.getAllInfoLogs().c_str());
		exit(1);
	}

	pvm_uniformId = glGetUniformLocation(shader.getProgramIndex(), "m_pvm");
	vm_uniformId = glGetUniformLocation(shader.getProgramIndex(), "m_viewModel");
	normal_uniformId = glGetUniformLocation(shader.getProgramIndex(), "m_normal");
	//lPos_uniformId = glGetUniformLocation(shader.getProgramIndex(), "l_pos");
	pointLights[0].location = glGetUniformLocation(shader.getProgramIndex(), "pointLight0location");
	pointLights[1].location = glGetUniformLocation(shader.getProgramIndex(), "pointLight1location");
	pointLights[2].location = glGetUniformLocation(shader.getProgramIndex(), "pointLight2location");
	pointLights[3].location = glGetUniformLocation(shader.getProgramIndex(), "pointLight3location");
	pointLights[4].location = glGetUniformLocation(shader.getProgramIndex(), "pointLight4location");
	pointLights[5].location = glGetUniformLocation(shader.getProgramIndex(), "pointLight5location");
	tex_loc = glGetUniformLocation(shader.getProgramIndex(), "texmap");
	tex_loc1 = glGetUniformLocation(shader.getProgramIndex(), "texmap1");
	tex_loc2 = glGetUniformLocation(shader.getProgramIndex(), "texmap2");
	
	printf("InfoLog for Per Fragment Phong Lightning Shader\n%s\n\n", shader.getAllInfoLogs().c_str());

	// Shader for bitmap Text
	shaderText.init();
	shaderText.loadShader(VSShaderLib::VERTEX_SHADER, "shaders/text.vert");
	shaderText.loadShader(VSShaderLib::FRAGMENT_SHADER, "shaders/text.frag");

	glLinkProgram(shaderText.getProgramIndex());
	printf("InfoLog for Text Rendering Shader\n%s\n\n", shaderText.getAllInfoLogs().c_str());

	if (!shaderText.isProgramValid()) {
		printf("GLSL Text Program Not Valid!\n");
		exit(1);
	}
	
	return(shader.isProgramLinked() && shaderText.isProgramLinked());
}

void setMaterial(MyMesh& mesh, float amb, float diff, float spec, float emissive, float shininess, int texcount) {
	// Criar arrays para as propriedades do material
	float amb1[4] = { amb, amb, amb, 1.0f };
	float diff1[4] = { diff, diff, diff, 1.0f };
	float spec1[4] = { spec, spec, spec, 1.0f };
	float emissive1[4] = { emissive, emissive, emissive, 1.0f };

	// Atribuir as propriedades ao mesh
	memcpy(mesh.mat.ambient, amb1, 4 * sizeof(float));
	memcpy(mesh.mat.diffuse, diff1, 4 * sizeof(float));
	memcpy(mesh.mat.specular, spec1, 4 * sizeof(float));
	memcpy(mesh.mat.emissive, emissive1, 4 * sizeof(float));
	mesh.mat.shininess = shininess;
	mesh.mat.texCount = texcount;
}

MyMesh createTrees(float height, float radius, int sides, float Sradius, int Sdivisions, float amb, float diff, float spec, float emissive, float shininess, int texcount, MyMesh amesh) {

	// Create arrays for material properties
	float amb1[4] = { amb, amb, amb, 1.0f };
	float diff1[4] = { diff, diff, diff, 1.0f };
	float spec1[4] = { spec, spec, spec, 1.0f };
	float emissive1[4] = { emissive, emissive, emissive, 1.0f };

	// create geometry and VAO of the cylinder
	amesh = createCylinder(height, radius, sides);
	setMaterial(amesh, amb, diff, spec, emissive, shininess, texcount);
	treeMeshes.push_back(amesh);

	// create geometry and VAO of the sphere
	amesh = createSphere(Sradius, Sdivisions);
	setMaterial(amesh, amb, diff, spec, emissive, shininess, texcount);
	treeMeshes.push_back(amesh);

	return amesh; // Return the last created mesh if needed
}

MyMesh createHouses(float height, float radius, int sides, float Sradius, int Sdivisions, float amb, float diff, float spec, float emissive, float shininess, int texcount, MyMesh amesh) {

	// Create arrays for material properties
	float amb1[4] = { amb, amb, amb, 1.0f };
	float diff1[4] = { diff, diff, diff, 1.0f };
	float spec1[4] = { spec, spec, spec, 1.0f };
	float emissive1[4] = { emissive, emissive, emissive, 1.0f };

	// Create geometry and VAO of the cube
	amesh = createCube();
	setMaterial(amesh, amb, diff, spec, emissive, shininess, texcount);
	houseMeshes.push_back(amesh);

	// create geometry and VAO of the sphere
	amesh = createPyramid(1.0f, 1.0f, 4);
	setMaterial(amesh, amb, diff, spec, emissive, shininess, texcount);
	houseMeshes.push_back(amesh);

	return amesh; // Return the last created mesh if needed
}

/*
*/
void createRows(MyMesh amesh, float baseLength, float amb, float diff, float spec, float emissive, float shininess, int texcount) {
	// Remos direito e esquerdo
	amesh = createCylinder(baseLength * (5.0 / 7.0), baseLength / 25.0, 20);	// cabo
	setMaterial(amesh, amb, diff, spec, emissive, shininess, texcount);
	rowMeshes.push_back(amesh);
	amesh = createQuad(0.5, 0.8);	// pá
	setMaterial(amesh, amb, diff, spec, emissive, shininess, texcount);
	rowMeshes.push_back(amesh);

	amesh = createCylinder(baseLength * (5.0 / 7.0), baseLength / 25.0, 20);
	setMaterial(amesh, amb, diff, spec, emissive, shininess, texcount);
	rowMeshes.push_back(amesh);
	amesh = createQuad(0.5, 0.8);
	setMaterial(amesh, amb, diff, spec, emissive, shininess, texcount);
	rowMeshes.push_back(amesh);
}

MyMesh createBoat(float baseWidth, float baseLength, float height, float amb, float diff, float spec, float emissive, float shininess, int texcount, MyMesh amesh) {
	// MyMesh boat;

	/*
	*/
	// Create arrays for material properties
	float amb1[4] = { amb, amb, amb, 1.0f };
	float diff1[4] = { diff, diff, diff, 1.0f };
	float spec1[4] = { spec, spec, spec, 1.0f };
	float emissive1[4] = { emissive, emissive, emissive, 1.0f };

	// Criar e configurar as partes do barco (base, paredes, proa)
	// MyMesh base = createQuad(baseWidth, baseLength);
	amesh = createQuad(baseWidth, baseLength); // base

	// Configurar materiais
	setMaterial(amesh, amb, diff, spec, emissive, shininess, texcount);
	boatMeshes.push_back(amesh);	// (index 0)

	amesh = createQuad(baseLength, height);	// parede direita
	setMaterial(amesh, amb, diff, spec, emissive, shininess, texcount);
	boatMeshes.push_back(amesh);

	amesh = createQuad(baseLength, height);	// parede esquerda
	setMaterial(amesh, amb, diff, spec, emissive, shininess, texcount);
	boatMeshes.push_back(amesh);

	amesh = createQuad(baseWidth, height); // parede frontal
	setMaterial(amesh, amb, diff, spec, emissive, shininess, texcount);
	boatMeshes.push_back(amesh);

	amesh = createQuad(baseWidth, height);	// parede traseira
	setMaterial(amesh, amb, diff, spec, emissive, shininess, texcount);
	boatMeshes.push_back(amesh);

	amesh = createPyramid(height, baseWidth / 2, 4);	// proa
	setMaterial(amesh, amb, diff, spec, emissive, shininess, texcount);
	boatMeshes.push_back(amesh);

	createRows(amesh, baseLength, amb, diff, spec, emissive, shininess, texcount);
	
	return amesh;
}

MyMesh createWaterCreatures() {
	MyMesh creatureMesh;
	// int numberOfCreatures = 2;

	float amb[] = { 0.2f, 0.15f, 0.1f, 1.0f };
	float diff[] = { 1.0f, 0.6f, 1.5f, 1.0f };
	float spec[] = { 0.8f, 1.0f, 0.8f, 1.0f };
	float emissive[] = { 0.0f, 0.0f, 0.0f, 1.0f };
	float shininess = 100.0f;
	int texcount = 0;

	for (int i = 0; i < NUM_CREATURES; i++) {
		creatureMesh = createCone(2.0, 0.5, 20);
		memcpy(creatureMesh.mat.ambient, amb, 4 * sizeof(float));
		memcpy(creatureMesh.mat.diffuse, diff, 4 * sizeof(float));
		memcpy(creatureMesh.mat.specular, spec, 4 * sizeof(float));
		memcpy(creatureMesh.mat.emissive, emissive, 4 * sizeof(float));
		creatureMesh.mat.shininess = shininess;
		creatureMesh.mat.texCount = texcount;
		movingMeshes.push_back(creatureMesh);
	}

	initializeCreatures(NUM_CREATURES, MAX_RADIUS);

	return creatureMesh;
}

// ------------------------------------------------------------
//
// Model loading and OpenGL setup
//

//void setupPointLights() {
//	float positions[NUM_POINT_LIGHTS][4] = {
//		{ -35.0f, 4.0f, -35.0f, 1.0f },
//		{ 40.0f, 7.0f, 40.0f, 1.0f },
//		{ 2.0f, 10.0f, 2.0f, 1.0f },
//		{ 0.0f, 4.0f, 15.0f, 1.0f },
//		{ 17.0f, -8.0f, 17.0f, 1.0f },
//		{ 1.0f, -2.0f, 1.0f, 1.0f }
//	};
//
//	float colors[6][4] = {
//		{0.3f, 0.7f, 0.2f, 1.0f},  // Greenish
//		{0.8f, 0.1f, 0.3f, 1.0f},  // Reddish
//		{0.2f, 0.4f, 0.8f, 1.0f},  // Blueish
//		{0.9f, 0.8f, 0.1f, 1.0f},  // Yellowish
//		{0.5f, 0.2f, 0.6f, 1.0f},  // Purplish
//		{0.1f, 0.9f, 0.7f, 1.0f}   // Tealish
//	};
//
//	pointLights.push_back(PointLight(positions[0], colors[0]));
//	pointLights.push_back(PointLight(positions[1], colors[1]));
//	pointLights.push_back(PointLight(positions[2], colors[2]));
//	pointLights.push_back(PointLight(positions[3], colors[3]));
//	pointLights.push_back(PointLight(positions[4], colors[4]));
//	pointLights.push_back(PointLight(positions[5], colors[5]));
//}
//
//void sendPointLightData(GLuint shaderProgram) {
//	for (int i = 0; i < pointLights.size(); i++) {
//		std::string index = std::to_string(i);
//		GLint posLoc = glGetUniformLocation(shaderProgram, ("pointLights[" + index + "].position").c_str());
//		GLint colLoc = glGetUniformLocation(shaderProgram, ("pointLights[" + index + "].color").c_str());
//
//		// Directly passing arrays of floats
//		glUniform4fv(posLoc, 1, pointLights[i].position);
//		glUniform4fv(colLoc, 1, pointLights[i].color);
//	}
//}

void init()
{
	MyMesh amesh;

	/* Initialization of DevIL */
	if (ilGetInteger(IL_VERSION_NUM) < IL_VERSION)
	{
		printf("wrong DevIL version \n");
		exit(0);
	}
	ilInit();

	/// Initialization of freetype library with font_name file
	freeType_init(font_name);


	// top 
	cams[0].camPos[0] = 0;
	cams[0].camPos[1] = 40;
	cams[0].camPos[2] = 0.1;
	// top ortho 
	cams[1].camPos[0] = 0;
	cams[1].camPos[1] = 40;
	cams[1].camPos[2] = 0.1;
	cams[1].type = 1;

	/* Cam[2] é implementada no Animation() */

	// original perspective
	cams[3].camPos[0] = 1;
	cams[3].camPos[1] = 1;
	cams[3].camPos[2] = 1;
	//cams[3].type = 0;

	// set the camera position based on its spherical coordinates
	camX = r * sin(alpha * 3.14f / 180.0f) * cos(beta * 3.14f / 180.0f);
	camZ = r * cos(alpha * 3.14f / 180.0f) * cos(beta * 3.14f / 180.0f);
	camY = r *   						     sin(beta * 3.14f / 180.0f);

	
	float amb[]= {0.2f, 0.15f, 0.1f, 1.0f};
	float diff[] = {0.8f, 0.6f, 0.4f, 1.0f};
	float spec[] = {0.8f, 0.8f, 0.8f, 1.0f};
	float emissive[] = {0.0f, 0.0f, 0.0f, 1.0f};
	float shininess= 100.0f;
	int texcount = 0;

	//Flat terrain
	amesh = createQuad(50, 50);
	memcpy(amesh.mat.ambient, amb, 4 * sizeof(float));
	memcpy(amesh.mat.diffuse, diff, 4 * sizeof(float));
	memcpy(amesh.mat.specular, spec, 4 * sizeof(float));
	memcpy(amesh.mat.emissive, emissive, 4 * sizeof(float));
	amesh.mat.shininess = shininess;
	amesh.mat.texCount = texcount;
	myMeshes.push_back(amesh);

	// Extracting individual float values from the arrays
	float amb_value = amb[0]; // Use the first value of amb array as the single float value
	float diff_value = diff[0]; // Similarly, use the first value of diff array
	float spec_value = spec[0]; // Use the first value of spec array
	float emissive_value = emissive[0]; // Use the first value of emissive array

	// Calling createTrees with the extracted values
	amesh = createTrees(3.5f, 0.1f, 20, 0.7, 20, amb_value, diff_value, spec_value, emissive_value, shininess, texcount, amesh);

	// Calling createHouses with the extracted values
	amesh = createHouses(3.5f, 50.0f, 50, 0.9, 20, amb_value, diff_value, spec_value, emissive_value, shininess, texcount, amesh);
	// printf("Há %d house meshes\n", houseMeshes.size());

	// Calling createBoat with the extracted values
	amesh = createBoat(1.0f, 2.5f, 0.8f, amb_value, diff_value, spec_value, emissive_value, shininess, texcount, amesh);

	// Create creatures that swim
	amesh = createWaterCreatures();

	// Create pointlights
	/*setupPointLights();
	sendPointLightData(shader.getProgramIndex());*/
	

	// some GL settings
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glEnable(GL_MULTISAMPLE);
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

}


// ------------------------------------------------------------
//
// Main function
//


int main(int argc, char **argv) {

	//  GLUT initialization
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DEPTH|GLUT_DOUBLE|GLUT_RGBA|GLUT_MULTISAMPLE);

	glutInitContextVersion (4, 3);
	glutInitContextProfile (GLUT_CORE_PROFILE );
	glutInitContextFlags(GLUT_FORWARD_COMPATIBLE | GLUT_DEBUG);

	glutInitWindowPosition(100,100);
	glutInitWindowSize(WinX, WinY);
	WindowHandle = glutCreateWindow(CAPTION);


	//  Callback Registration
	glutDisplayFunc(renderScene);
	glutReshapeFunc(changeSize);

	glutTimerFunc(0, timer, 0);
	//glutIdleFunc(renderScene);  // Use it for maximum performance
	glutTimerFunc(0, refresh, 0);    //use it to to get 60 FPS whatever

	//	Mouse and Keyboard Callbacks
	glutKeyboardFunc(processKeys);
	glutMouseFunc(processMouseButtons);
	glutMotionFunc(processMouseMotion);
	glutMouseWheelFunc ( mouseWheel ) ;
	

	//	return from main loop
	glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_GLUTMAINLOOP_RETURNS);

	//	Init GLEW
	glewExperimental = GL_TRUE;
	glewInit();

	printf ("Vendor: %s\n", glGetString (GL_VENDOR));
	printf ("Renderer: %s\n", glGetString (GL_RENDERER));
	printf ("Version: %s\n", glGetString (GL_VERSION));
	printf ("GLSL: %s\n", glGetString (GL_SHADING_LANGUAGE_VERSION));

	// Initialize lights vector
	pointLights.push_back(PointLight(positions[0], colors[0]));
	pointLights.push_back(PointLight(positions[1], colors[1]));
	pointLights.push_back(PointLight(positions[2], colors[2]));
	pointLights.push_back(PointLight(positions[3], colors[3]));
	pointLights.push_back(PointLight(positions[4], colors[4]));
	pointLights.push_back(PointLight(positions[5], colors[5]));
	pointLights[0] = PointLight(positions[0], colors[0]);

	for (int i = 0; i < 6; i++) {
		for (int j = 0; j < 4; j++) {
			printf("Active PointLight: %f\n", pointLights[i].position[j]);
		}
	}

	if (!setupShaders())
		return(1);

	init();

	// Iniciar a animação
	glutTimerFunc(1 / deltaT, animation, 0);

	//  GLUT main loop
	glutMainLoop();

	return(0);
}



