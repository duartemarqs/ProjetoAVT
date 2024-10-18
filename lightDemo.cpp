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
// Author: Jo�o Madeiras Pereira
//

#include <math.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <cstdlib>
#include <iomanip>  // Para o setprecision

// Library effective with Windows
#include <windows.h>

// include GLEW to access OpenGL 3.3 functions
#include <GL/glew.h>

// GLUT is the toolkit to interface with the OS
#include <GL/freeglut.h>

#include <IL/il.h>

// assimp include files. These three are usually needed.
#include "assimp/Importer.hpp"	//OO version Header!
#include "assimp/scene.h"
#include "meshFromAssimp.h"

// Use Very Simple Libs
#include "VSShaderlib.h"
#include "AVTmathLib.h"
#include "VertexAttrDef.h"
#include "geometry.h"
#include "Texture_Loader.h"

#include "avtFreeType.h"

// Macro definitions
#ifndef M_PI
#define M_PI 3.14159265358979323846
#define NUM_POINT_LIGHTS 6
#endif

#define NUM_TRANSPARENT_OBJS 3
#define NUM_BUOYS 5
#define NUM_CREATURES 8
#define MAX_RADIUS 25.0

#define GAME_WIN 3
#define GAME_ACTIVE 2
#define GAME_PAUSED 1
#define GAME_OVER 0
#define CAPTION "AVT Demo: Phong Shading and Text rendered with FreeType"


using namespace std;

int WindowHandle = 0;
int WinX = 1024, WinY = 768;

unsigned int FrameCount = 0;

//shaders
VSShaderLib shader;  //geometry
VSShaderLib shaderText;  //render bitmap text

bool directionalLightState = true;
bool pointLightState = true;
bool spotLightState = true;

//File with the font
const string font_name = "fonts/arial.ttf";

//Vectors with meshes
vector<struct MyMesh> myMeshes;
vector<struct MyMesh> movingMeshes;
vector<struct MyMesh> houseMeshes;
vector<struct MyMesh> treeMeshes;
vector<struct MyMesh> boatMeshes;
vector<struct MyMesh> rowMeshes;
vector<struct MyMesh> ballMeshes;
vector<struct MyMesh> finishLineMeshes;
vector<struct MyMesh> assimpMeshes;

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
GLint lightStatesLoc;
//GLint tex_loc, tex_loc1, tex_loc2;

// Textures
GLint tex_loc, tex_loc1;
GLint texMode_uniformId;
GLuint TextureArray[3];

// Camera Position
float camX, camY, camZ;

// Mouse Tracking Variables
int startX, startY, tracking = 0;

// Camera Spherical Coordinates
float alpha = 39.0f, beta = 51.0f;
float r = 10.0f;

// Frame counting and FPS computation
long myTime, timebase = 0, frame = 0;
char s[32];
float lightPos[4] = { 4.0f, 6.0f, 2.0f, 1.0f };

//Fog
bool fogEnabled = false;

// Camera -------------
class Camera {
public:
	float camPos[3] = { 0,0,0 };
	float camTarget[3] = { 0.0f, 0.0f, 0.0f };
	int type = 0; //0 - perspective , 1 - ortho
};

// Boat ---------------
class Boat {
public:
	float speed;
	float direction[3];
	float pos[3];
	float angle;
};


// Water creatures
class WaterCreature {
public:
	float speed;
	float direction[3];
	float pos[3];
	float angle;
	float oscillationTime;	// Tempo acumulado para controlar a oscila��o
	float timeElapsed;		// Tempo decorrido desde o �ltimo aumento de velocidade

	WaterCreature() : timeElapsed(0.0f) {};
};

// Global variables
Camera cams[4];
int activeCam = 0;

Boat boat;
vector<struct WaterCreature> waterCreatures;

float deltaT = 0.05;	
float decaySpeed = 0.99;
float deltaTimeElapsed = 1.0f / 60.0f;	// velocity increase time for water creatures (for 60 FPS)

struct Vec3 {
	float x, y, z;

	Vec3 operator+(const Vec3& other) const {
		return { x + other.x, y + other.y, z + other.z };
	}

	Vec3 operator-(const Vec3& other) const {
		return { x - other.x, y - other.y, z - other.z };
	}

	Vec3 operator*(float scalar) const {
		return { x * scalar, y * scalar, z * scalar };
	}

	float length() const {
		return sqrt(x * x + y * y + z * z);
	}
};

struct Sphere {
	Vec3 center;
	float radius;
};

// Define buoy spheres
Sphere buoySpheres[5] = {
	{ { 15.0f, 0.2f, 1.5f }, 1.0f }, // buoy 1
	{ { 12.0f, 0.2f, 6.5f }, 1.0f }, // buoy 2
	{ { 18.0f, 0.2f, 3.0f }, 1.0f }, // buoy 3
	{ { 6.0f, 0.2f, 5.0f }, 1.0f },  // buoy 4
	{ { 22.0f, 0.2f, -1.0f }, 1.0f } // buoy 5
};

Sphere finishLineSphere = { { -17.0f, 0.2f, 0.0f }, 2.0f }; // Centro entre os portões, raio ajustado


const float defaultColor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };

class Light {
public:
	// directional light position
	//float directionalLightPos[4]{ 1.0f, 1000.0f, 1.0f, 0.0f };
	// random positions for lights
	float position[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	float direction[3];
	float color[4];
	GLint location = 0;
	int enabled = 1;
	GLint enabledLocation = 0;
	//float color[4]; // Color of the light (r, g, b, a)
	//GLint pointlightLocations[NUM_POINT_LIGHTS];
	//GLint pointlightLocations;	
	// 

	// Constructor
	Light(const float pos[4], const float col[4], int enabled) {
		// Copy the values of pos and col into the member arrays
		for (int i = 0; i < 4; ++i) {
			position[i] = pos[i];
		}
		for (int i = 0; i < 4; ++i) {
			color[i] = col[i];
		}
		enabled = enabled;
	}

	// Getter method for pointLightPosition
	const float* getLightPosition() const {
		return position;
	}
	// Setter method for pointLightPosition
	void setLightPosition(const float newPosition[4]) {
		// Use a loop to copy each element
		for (int i = 0; i < 4; i++) {
			position[i] = newPosition[i];
		}
	}

	// Setter method for direction
	void setDirection(const float newDirection[3]) {
		// Use a loop to copy each element
		for (int i = 0; i < 2; i++) {
			direction[i] = newDirection[i];
		}
		// direction[3] = 0.0f;
	}

};

class Spotlight : public Light {
public:
	float direction[3];
	//float position[4];
	float cutOffAngle = 0.0;
	float intensity = 0.0;


	Spotlight(const float position[4], float direction[3], float cutOffAngle, float intensity, int enabled)
		: Light(position, defaultColor, enabled)  // Call the Light constructor with default white color
	{
		for (int i = 0; i < 3; ++i) {
			this->direction[i] = direction[i];
		}

		this->cutOffAngle = cutOffAngle;
		this->intensity = intensity;
	}

};

int states[9] = { 0,1,0,1,0,0,0,1,1 };

float positions[9][4] = {
		{ -10.98f, 5.0f, -2.35f, 1.0f },
		{ -12.0f, 5.0f, 8.02f, 1.0f },
		{ -1.67f, 5.0f, 13.08f, 1.0f },
		{ -3.35f, 5.0f, 0.58f, 1.0f },
		{ 5.87f, 5.0f, -5.36f, 1.0f },
		{ 5.80f, 5.0f, 18.31f, 1.0f },

		{ 10.0f, 20.0f, 10.0f, 0.0f }, /// Directional Light

		{ 1.0f, 2.0f, 1.0f, 1.0f }, /// Spotlight 1
		{ 1.0f, 3.0f, 1.0f, 1.0f }, /// Spotlight 2
};

float colors[9][4] = {
	{0.3f, 0.7f, 0.2f, 1.0f},  // Greenish
	{0.8f, 0.1f, 0.3f, 1.0f},  // Reddish
	{0.2f, 0.4f, 0.8f, 1.0f},  // Blueish
	{0.9f, 0.8f, 0.1f, 1.0f},  // Yellowish
	{0.5f, 0.2f, 0.6f, 1.0f},  // Purplish
	{0.1f, 0.9f, 0.7f, 1.0f},   // Tealish
	{0.9f, 0.8f, 0.1f, 1.0f},  // Yellowish
	{0.5f, 0.2f, 0.6f, 1.0f},  // Purplish
	{0.1f, 0.9f, 0.7f, 1.0f},   // Tealish
};

float direction[2][3] = { { 1.0f, -1.0f, 0.0f } , { 2.0f, -1.0f, 0.0f } };
float cutOffAngle[2] = { 0.5f, 0.5 };
float intensity[2] = { 50.0f, 50.f };

std::vector<Light> lights;

//endof setup of pointlights

// Game setup
class Game
{
public:
	unsigned int lives;
	float playTime;
	int state;

	Game() : lives(5), playTime(0.0), state(GAME_ACTIVE) {};
};

Game boatGame;

/* IMPORTANT: Use the next data to make this Assimp demo to work*/

// Create an instance of the Importer class
Assimp::Importer importer;

// the global Assimp scene object
const aiScene* scene;

// scale factor for the Assimp model to fit in the window
float scaleFactor;

char model_dir[50];  // initialized by the user input at the console

GLint normalMap_loc;
GLint specularMap_loc;
GLint diffMapCount_loc;

// for the input model
GLuint* textureIds; 

// By default if there is a normal map then bump effect is implemented. press key "b" to enable/disable normal mapping 
bool normalMapKey = TRUE;

/* Assimp (end) */


// Fun��o para gerar um valor aleat�rio entre min e max
float randomFloat(float min, float max) {
	return min + static_cast<float>(rand()) / (static_cast<float>(RAND_MAX / (max - min)));
}

// Fun��o para inicializar uma criatura com posi��o e dire��o aleat�rias
void initCreature(WaterCreature& creature, float maxRadius) {
	// Define posi��o aleat�ria no plano (x, z) e y = 0
	creature.pos[0] = randomFloat(-maxRadius, maxRadius);  // posi��o x
	creature.pos[1] = 0.0f;                                // posi��o y (n�vel da �gua)
	creature.pos[2] = randomFloat(-maxRadius, maxRadius);  // posi��o z

	// Define uma dire��o aleat�ria
	creature.direction[0] = randomFloat(-1.0f, 1.0f); // dire��o x
	creature.direction[1] = 0.0f;                     // dire��o y (sempre 0)
	creature.direction[2] = randomFloat(-1.0f, 1.0f); // dire��o z

	// Normaliza a dire��o para garantir que tenha magnitude 1
	float magnitude = sqrt(creature.direction[0] * creature.direction[0] +
		creature.direction[2] * creature.direction[2]);
	creature.direction[0] /= magnitude;
	creature.direction[2] /= magnitude;

	// Define uma velocidade e �ngulo aleat�rios
	creature.speed = randomFloat(0.5f, 1.0f);
	creature.angle = randomFloat(0.0f, 360.0f);			// �ngulo de rota��o
	creature.oscillationTime = 0.0f;					// tempo de oscila��o
}

Sphere creatureSpheres[NUM_CREATURES];

// Inicializa N criaturas com posi��es aleat�rias
void initializeCreatures(int numCreatures, float maxRadius) {
	waterCreatures.clear();
	for (int i = 0; i < numCreatures; i++) {
		WaterCreature creature;
		initCreature(creature, maxRadius);
		waterCreatures.push_back(creature);

		// Bounding spheres para as criaturas
		creatureSpheres[i].center = { creature.pos[0], creature.pos[1], creature.pos[2] };
		creatureSpheres[i].radius = 1.0f;
	}
}

void updateCreatures(float deltaT, float maxRadius) {
	for (int i = 0; i < waterCreatures.size(); i++) {
		WaterCreature& creature = waterCreatures[i];

		// Atualiza a posi��o com base na dire��o e velocidade
		creature.pos[0] += creature.direction[0] * creature.speed * deltaT;
		creature.pos[2] += creature.direction[2] * creature.speed * deltaT;

		// Atualizar esferas de colisão
		for (int i = 0; i < waterCreatures.size(); ++i) {
			creatureSpheres[i].center = { waterCreatures[i].pos[0], waterCreatures[i].pos[1], waterCreatures[i].pos[2] };
		}


		// Atualiza o tempo de oscila��o
		creature.oscillationTime += deltaT;
		creature.timeElapsed += deltaTimeElapsed;

		// Aumentar a velocidade a cada 30 segundos
		if (creature.timeElapsed >= 30.0f) {
			creature.speed += 1.0;
			creature.timeElapsed = 0.0f; // Reseta o contador de tempo
		}

		// Define a oscila��o angular (30 graus para ambos os lados)
		float oscillationAmplitude = 30.0f;  // Amplitude m�xima de oscila��o (30�)
		float oscillationFrequency = 2.0f;   // Velocidade da oscila��o (ajust�vel)

		// �ngulo de oscila��o baseado no seno do tempo
		float oscillationAngle = oscillationAmplitude * sin(oscillationFrequency * creature.oscillationTime);

		// Aplica o �ngulo oscilante � rota��o da criatura (somente para a visualiza��o)
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

void updateSpotlights() {

	float boatPosX = boat.pos[0];
	float boatPosY = boat.pos[1];
	float boatPosZ = boat.pos[2];

	float distanceFromBoat = 0.001f; 
	float angleRad = boat.angle * M_PI / 180.0f; // Converter ângulo para radianos

	
	lights[7].position[0] = boatPosX + distanceFromBoat * cos(angleRad);
	lights[7].position[1] = boatPosY + 2.0f; 
	lights[7].position[2] = boatPosZ - distanceFromBoat * sin(angleRad);

	lights[8].position[0] = boatPosX + distanceFromBoat * cos(angleRad);
	lights[8].position[1] = boatPosY + 2.0f;
	lights[8].position[2] = boatPosZ - distanceFromBoat * sin(angleRad);

	printf("Posição do barco: x = %f, y = %f, z = %f\n", boat.pos[0], boat.pos[1], boat.pos[2]);

	lights[7].location = glGetUniformLocation(shader.getProgramIndex(), "spotLight0location");
	lights[8].location = glGetUniformLocation(shader.getProgramIndex(), "spotLight1location");
}




bool checkCollision(const Sphere& a, const Sphere& b) {
	Vec3 delta = a.center - b.center;
	float distanceSquared = delta.x * delta.x + delta.y * delta.y + delta.z * delta.z;
	float radiusSum = a.radius + b.radius;
	return distanceSquared <= (radiusSum * radiusSum);
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

float lastCollisionTime = 0.0f; // Tempo da última colisão com uma criatura
float invulnerabilityTime = 1.0f; // Tempo de invulnerabilidade em segundos



void animation(int value) {
	if (boatGame.state != GAME_ACTIVE) {
		return;
	}

	boat.direction[0] = sin(boat.angle * M_PI / 180);
	boat.direction[1] = 0;
	boat.direction[2] = cos(boat.angle * M_PI / 180);

	// Atualiza a posição do barco
	boat.pos[0] += boat.direction[0] * boat.speed * deltaT;
	boat.pos[1] += boat.direction[1] * boat.speed * deltaT;
	boat.pos[2] += boat.direction[2] * boat.speed * deltaT;

	//printf("Posição do barco: x = %f, y = %f, z = %f\n", boat.pos[0], boat.pos[1], boat.pos[2]);

	updateSpotlights();

	Sphere boatSphere;
	boatSphere.center = { boat.pos[0], boat.pos[1], boat.pos[2] };
	boatSphere.radius = 1.0f;

	// Verificar colisões com boias
	for (int i = 0; i < 5; ++i) {
		if (checkCollision(boatSphere, buoySpheres[i])) {
			std::cout << "Colisão com boia " << (i + 1) << std::endl;
			boat.speed = 0.0f;
			Vec3 impactDirection = buoySpheres[i].center - boatSphere.center;
			impactDirection = impactDirection * (1.0f / impactDirection.length()); // Normaliza

			// Mover a boia na direção do impacto
			buoySpheres[i].center = buoySpheres[i].center + (impactDirection * 0.5f);
		}
	}

	// Verificar colisões com criaturas aquáticas
	for (int i = 0; i < waterCreatures.size(); ++i) {
		if (checkCollision(boatSphere, creatureSpheres[i])) {

			// Obter o tempo atual (tempo total de jogo)
			float currentTime = glutGet(GLUT_ELAPSED_TIME) / 1000.0f; // tempo em segundos

			// Verifica se passou o tempo de invulnerabilidade desde a última colisão
			if (currentTime - lastCollisionTime >= invulnerabilityTime) {
				// Atualiza o tempo da última colisão
				lastCollisionTime = currentTime;

				// Reduz uma vida
				boatGame.lives--;
				printf("PLAYER'S LIFE: %d\n", boatGame.lives);

				// Verifica se o jogador perdeu todas as vidas (Game over)
				if (boatGame.lives == 0) {
					boatGame.state = GAME_OVER;
				}

				printf("Colidiu com uma criatura!\n");
			}

			break; // sai do loop após a colisão
		}
	}


	// Verificar colisão com a meta
	if (checkCollision(boatSphere, finishLineSphere)) {
		std::cout << "Barco alcançou a linha de chegada!" << std::endl;
		boatGame.state = GAME_WIN;  
		boat.speed = 0.0f; 
	}


	if (boat.speed > 0) boat.speed *= decaySpeed;

	// Atualizar a câmera
	float dist = -10.0f;
	float height = 7.5f;

	camX = dist * sin((alpha + boat.angle) * M_PI / 180.0f) * cos(beta * M_PI / 180.0f);
	camZ = dist * cos((alpha + boat.angle) * M_PI / 180.0f) * cos(beta * M_PI / 180.0f);
	camY = dist * sin(beta * M_PI / 180.0f);

	cams[2].camPos[0] = boat.pos[0] + camX;
	cams[2].camPos[1] = boat.pos[1] + camY + height;
	cams[2].camPos[2] = boat.pos[2] + camZ;

	cams[2].camTarget[0] = boat.pos[0];
	cams[2].camTarget[1] = boat.pos[1];
	cams[2].camTarget[2] = boat.pos[2];

	updateCreatures(deltaT, MAX_RADIUS);

	boatGame.playTime += deltaTimeElapsed;

	glutTimerFunc(1 / deltaT, animation, 0);
}


void refresh(int value)
{
	// Atualiza a cena
	glutPostRedisplay();

	// Nova chamada ao temporizador ap�s 16 milissegundos
	glutTimerFunc(1000 / 60, refresh, 0);
}

// ------------------------------------------------------------
//
// Reshape Callback Function
//

void changeSize(int w, int h) {

	float ratio;
	// Prevent a divide by zero, when window is too short
	if (h == 0)
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
void renderBalls(GLint loc) {
	int meshId = 0;
	pushMatrix(MODEL);

	// Ativa o blending para transparência
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	do {
		// send the material
		loc = glGetUniformLocation(shader.getProgramIndex(), "mat.ambient");
		glUniform4fv(loc, 1, ballMeshes[meshId].mat.ambient);
		loc = glGetUniformLocation(shader.getProgramIndex(), "mat.diffuse");
		// printf("BALL ALPHA: %f\n", ballMeshes[meshId].mat.diffuse[3]);
		// Definir transparência (50%)
		// ballMeshes[meshId].mat.diffuse[3] = 0.5f; 

		glUniform4fv(loc, 1, ballMeshes[meshId].mat.diffuse);
		loc = glGetUniformLocation(shader.getProgramIndex(), "mat.specular");
		glUniform4fv(loc, 1, ballMeshes[meshId].mat.specular);
		loc = glGetUniformLocation(shader.getProgramIndex(), "mat.shininess");
		glUniform1f(loc, ballMeshes[meshId].mat.shininess);
		pushMatrix(MODEL);

		/*
		*/
		if (meshId == 0) {
			translate(MODEL, -2.0, 0.5, 4.0);
		}
		else if (meshId == 2) {
			translate(MODEL, 8.0, 0.5, 2.0);
		}
		else {
			translate(MODEL, 2.0, 0.5, 6.0);
		}

		// send matrices to OGL
		computeDerivedMatrix(PROJ_VIEW_MODEL);
		glUniformMatrix4fv(vm_uniformId, 1, GL_FALSE, mCompMatrix[VIEW_MODEL]);
		glUniformMatrix4fv(pvm_uniformId, 1, GL_FALSE, mCompMatrix[PROJ_VIEW_MODEL]);
		computeNormalMatrix3x3();
		glUniformMatrix3fv(normal_uniformId, 1, GL_FALSE, mNormal3x3);

		// Render mesh
		glBindVertexArray(ballMeshes[meshId].vao);
		glDrawElements(ballMeshes[meshId].type, ballMeshes[meshId].numIndexes, GL_UNSIGNED_INT, 0);
		glBindVertexArray(0);

		popMatrix(MODEL);
		meshId++;
	} while (meshId < ballMeshes.size());

	popMatrix(MODEL);
	glDisable(GL_BLEND);
}

void renderBuoys(GLint loc) {
	int meshId = 1; // myMeshes[0] has the terrain mesh
	pushMatrix(MODEL);

	do {
		// send the material
		loc = glGetUniformLocation(shader.getProgramIndex(), "mat.ambient");
		glUniform4fv(loc, 1, myMeshes[meshId].mat.ambient);
		loc = glGetUniformLocation(shader.getProgramIndex(), "mat.diffuse");
		glUniform4fv(loc, 1, myMeshes[meshId].mat.diffuse);
		loc = glGetUniformLocation(shader.getProgramIndex(), "mat.specular");
		glUniform4fv(loc, 1, myMeshes[meshId].mat.specular);
		loc = glGetUniformLocation(shader.getProgramIndex(), "mat.shininess");
		glUniform1f(loc, myMeshes[meshId].mat.shininess);
		pushMatrix(MODEL);

		// Use the buoySpheres' positions for translation
		if (meshId >= 1 && meshId <= 5) {
			translate(MODEL, buoySpheres[meshId - 1].center.x, buoySpheres[meshId - 1].center.y, buoySpheres[meshId - 1].center.z);
		}

		// send matrices to OGL
		computeDerivedMatrix(PROJ_VIEW_MODEL);
		glUniformMatrix4fv(vm_uniformId, 1, GL_FALSE, mCompMatrix[VIEW_MODEL]);
		glUniformMatrix4fv(pvm_uniformId, 1, GL_FALSE, mCompMatrix[PROJ_VIEW_MODEL]);
		computeNormalMatrix3x3();
		glUniformMatrix3fv(normal_uniformId, 1, GL_FALSE, mNormal3x3);

		// Render mesh
		glBindVertexArray(myMeshes[meshId].vao);
		glDrawElements(myMeshes[meshId].type, myMeshes[meshId].numIndexes, GL_UNSIGNED_INT, 0);
		glBindVertexArray(0);

		popMatrix(MODEL);
		meshId++;
	} while (meshId < myMeshes.size());

	popMatrix(MODEL);
}


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

		// Transladar criatura para a sua posi��o atual
		translate(MODEL, waterCreatures[meshId].pos[0], waterCreatures[meshId].pos[1], waterCreatures[meshId].pos[2]);
		// Rodar criatura em torno do eixo Y (oscila��es)
		rotate(MODEL, waterCreatures[meshId].angle, 0.0f, 1.0f, 0.0f);

		// Dar uma diferente inclina��o a cada creature (cone)
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


void renderFinishLine(GLint loc) {
	int meshId = 0;
	pushMatrix(MODEL);

	do {
		loc = glGetUniformLocation(shader.getProgramIndex(), "mat.ambient");
		glUniform4fv(loc, 1, finishLineMeshes[meshId].mat.ambient);
		loc = glGetUniformLocation(shader.getProgramIndex(), "mat.diffuse");
		glUniform4fv(loc, 1, finishLineMeshes[meshId].mat.diffuse);
		loc = glGetUniformLocation(shader.getProgramIndex(), "mat.specular");
		glUniform4fv(loc, 1, finishLineMeshes[meshId].mat.specular);
		loc = glGetUniformLocation(shader.getProgramIndex(), "mat.shininess");
		glUniform1f(loc, finishLineMeshes[meshId].mat.shininess);
		pushMatrix(MODEL);

		// Transformações
		if (meshId == 0) { // Primeiro portão
			translate(MODEL, -13.0, 0.0, 5.2f); 
			rotate(MODEL, -60.0f, 0.0f, 1.0f, 0.0f);
		}
		else if (meshId == 1) { // Segundo portão
			translate(MODEL, -12.5f, 0.0, -2.5f); 
			rotate(MODEL, 60.0f, 0.0f, 1.0f, 0.0f);
		}

		computeDerivedMatrix(PROJ_VIEW_MODEL);
		glUniformMatrix4fv(vm_uniformId, 1, GL_FALSE, mCompMatrix[VIEW_MODEL]);
		glUniformMatrix4fv(pvm_uniformId, 1, GL_FALSE, mCompMatrix[PROJ_VIEW_MODEL]);
		computeNormalMatrix3x3();
		glUniformMatrix3fv(normal_uniformId, 1, GL_FALSE, mNormal3x3);

		glBindVertexArray(finishLineMeshes[meshId].vao);
		glDrawElements(finishLineMeshes[meshId].type, finishLineMeshes[meshId].numIndexes, GL_UNSIGNED_INT, 0);
		glBindVertexArray(0);

		popMatrix(MODEL);
		meshId++;
	} while (meshId < finishLineMeshes.size());

	popMatrix(MODEL);
}


void renderBoat(GLint loc) {
	int meshId = 0;

	pushMatrix(MODEL);

	translate(MODEL, boat.pos[0], boat.pos[1], boat.pos[2]);
	rotate(MODEL, boat.angle, 0.0f, 1.0f, 0.0f);  

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
			translate(MODEL, 0.0f, 0.1f, 0.0f);
			rotate(MODEL, 90, 1, 0, 0);
		}
		else if (meshId == 1) {	 // left wall
			translate(MODEL, -0.5f, 0.5f, 0.0f);	// baseWidth = 1.0 => 0.5baseWidth = 0.5
			rotate(MODEL, 90, 0, 1, 0);
		}
		else if (meshId == 2) {	// right wall
			translate(MODEL, 0.5f, 0.5f, 0.0f);
			rotate(MODEL, 90, 0, 1, 0);
		}
		else if (meshId == 3) { // back wall
			translate(MODEL, 0.0f, 0.5f, 1.25f);		// baseLength = 2.2 => 0.5baseLength = 1.25
		}
		else if (meshId == 4) { // front wall
			translate(MODEL, 0.0f, 0.5f, -1.25f);
		}
		else if (meshId == 5) { // prow
			translate(MODEL, 0.0f, 0.5f, 1.25f);
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
	float oarMovementAngle = 0.0f;

	// movimento dos remos
	if (boat.speed > 0) {
		oarMovementAngle = sin(glutGet(GLUT_ELAPSED_TIME) * 0.001f * boat.speed * 0.5f) * 15.0f;
	}

	pushMatrix(MODEL);

	translate(MODEL, boat.pos[0], boat.pos[1], boat.pos[2]);
	rotate(MODEL, boat.angle, 0.0f, 1.0f, 0.0f);

	do {
		// Send the material
		loc = glGetUniformLocation(shader.getProgramIndex(), "mat.ambient");
		glUniform4fv(loc, 1, rowMeshes[meshId].mat.ambient);
		loc = glGetUniformLocation(shader.getProgramIndex(), "mat.diffuse");
		glUniform4fv(loc, 1, rowMeshes[meshId].mat.diffuse);
		loc = glGetUniformLocation(shader.getProgramIndex(), "mat.specular");
		glUniform4fv(loc, 1, rowMeshes[meshId].mat.specular);
		loc = glGetUniformLocation(shader.getProgramIndex(), "mat.shininess");
		glUniform1f(loc, rowMeshes[meshId].mat.shininess);

		pushMatrix(MODEL);

		// Transformações
		if (meshId == 0) { // remo esquerdo
			translate(MODEL, -1.0f, 1.0f, 0.0f);
			rotate(MODEL, 90 + oarMovementAngle, 0, 0, 1);
		}
		else if (meshId == 1) {
			translate(MODEL, -2.1f, 0.95f, 0.0f);
			rotate(MODEL, 90 + oarMovementAngle, 0, 0, 1);
		}
		else if (meshId == 2) { // remo direito
			translate(MODEL, 1.0f, 1.0f, 0.0f);
			rotate(MODEL, -90 - oarMovementAngle, 0, 0, 1);
		}
		else if (meshId == 3) {
			translate(MODEL, 2.1f, 0.95f, 0.0f);
			rotate(MODEL, -90 - oarMovementAngle, 0, 0, 1);
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

// Recursive render of the Assimp Scene Graph
void aiRecursiveRender(const aiNode* node, vector<struct MyMesh>& myMeshes, GLuint*& textureIds)
{
	GLint loc;

	// Get node transformation matrix
	aiMatrix4x4 m = node->mTransformation;
	// OpenGL matrices are column major
	m.Transpose();

	// save model matrix and apply node transformation
	pushMatrix(MODEL);

	float aux[16];
	memcpy(aux, &m, sizeof(float) * 16);
	multMatrix(MODEL, aux);


	// draw all meshes assigned to this node
	for (unsigned int n = 0; n < node->mNumMeshes; ++n) {

		// send the material
		loc = glGetUniformLocation(shader.getProgramIndex(), "mat.ambient");
		glUniform4fv(loc, 1, myMeshes[node->mMeshes[n]].mat.ambient);
		loc = glGetUniformLocation(shader.getProgramIndex(), "mat.diffuse");
		glUniform4fv(loc, 1, myMeshes[node->mMeshes[n]].mat.diffuse);
		loc = glGetUniformLocation(shader.getProgramIndex(), "mat.specular");
		glUniform4fv(loc, 1, myMeshes[node->mMeshes[n]].mat.specular);
		loc = glGetUniformLocation(shader.getProgramIndex(), "mat.emissive");
		glUniform4fv(loc, 1, myMeshes[node->mMeshes[n]].mat.emissive);
		loc = glGetUniformLocation(shader.getProgramIndex(), "mat.shininess");
		glUniform1f(loc, myMeshes[node->mMeshes[n]].mat.shininess);
		loc = glGetUniformLocation(shader.getProgramIndex(), "mat.texCount");
		glUniform1i(loc, myMeshes[node->mMeshes[n]].mat.texCount);

		unsigned int  diffMapCount = 0;  // Read 2 diffuse textures

		// Devido ao fragment shader suporta 2 texturas difusas simultaneas, 1 especular e 1 normal map
		glUniform1i(normalMap_loc, false);   // GLSL normalMap variable initialized to 0
		glUniform1i(specularMap_loc, false);
		glUniform1ui(diffMapCount_loc, 0);

		if (myMeshes[node->mMeshes[n]].mat.texCount != 0)
			for (unsigned int i = 0; i < myMeshes[node->mMeshes[n]].mat.texCount; ++i) {

				// Activate a TU with a Texture Object
				GLuint TU = myMeshes[node->mMeshes[n]].texUnits[i];
				glActiveTexture(GL_TEXTURE0 + TU);
				glBindTexture(GL_TEXTURE_2D, textureIds[TU]);

				if (myMeshes[node->mMeshes[n]].texTypes[i] == DIFFUSE) {
					if (diffMapCount == 0) {
						diffMapCount++;
						loc = glGetUniformLocation(shader.getProgramIndex(), "texUnitDiff");
						glUniform1i(loc, TU);
						glUniform1ui(diffMapCount_loc, diffMapCount);
					}
					else if (diffMapCount == 1) {
						diffMapCount++;
						loc = glGetUniformLocation(shader.getProgramIndex(), "texUnitDiff1");
						glUniform1i(loc, TU);
						glUniform1ui(diffMapCount_loc, diffMapCount);
					}
					else printf("Only supports a Material with a maximum of 2 diffuse textures\n");
				}
				else if (myMeshes[node->mMeshes[n]].texTypes[i] == SPECULAR) {
					loc = glGetUniformLocation(shader.getProgramIndex(), "texUnitSpec");
					glUniform1i(loc, TU);
					glUniform1i(specularMap_loc, true);
				}
				else if (myMeshes[node->mMeshes[n]].texTypes[i] == NORMALS) { //Normal map
					loc = glGetUniformLocation(shader.getProgramIndex(), "texUnitNormalMap");
					if (normalMapKey)
						glUniform1i(normalMap_loc, normalMapKey);
					glUniform1i(loc, TU);

				}
				else printf("Texture Map not supported\n");
			}

		// send matrices to OGL
		computeDerivedMatrix(PROJ_VIEW_MODEL);
		glUniformMatrix4fv(vm_uniformId, 1, GL_FALSE, mCompMatrix[VIEW_MODEL]);
		glUniformMatrix4fv(pvm_uniformId, 1, GL_FALSE, mCompMatrix[PROJ_VIEW_MODEL]);
		computeNormalMatrix3x3();
		glUniformMatrix3fv(normal_uniformId, 1, GL_FALSE, mNormal3x3);

		// bind VAO
		glBindVertexArray(myMeshes[node->mMeshes[n]].vao);

		if (!shader.isProgramValid()) {
			printf("Program Not Valid!\n");
			exit(1);
		}
		// draw
		glDrawElements(myMeshes[node->mMeshes[n]].type, myMeshes[node->mMeshes[n]].numIndexes, GL_UNSIGNED_INT, 0);
		glBindVertexArray(0);
	}

	// draw all children
	for (unsigned int n = 0; n < node->mNumChildren; ++n) {
		aiRecursiveRender(node->mChildren[n], myMeshes, textureIds);
	}
	popMatrix(MODEL);
}

void renderPauseScreen() {
	//Render text (bitmap fonts) in screen coordinates. So use ortoghonal projection with viewport coordinates.
	glDisable(GL_DEPTH_TEST);

	//the glyph contains transparent background colors and non-transparent for the actual character pixels. So we use the blending
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	int m_viewport[4];
	glGetIntegerv(GL_VIEWPORT, m_viewport);

	pushMatrix(MODEL);
	loadIdentity(MODEL);
	pushMatrix(PROJECTION);
	loadIdentity(PROJECTION);
	pushMatrix(VIEW);
	loadIdentity(VIEW);

	ortho(m_viewport[0], m_viewport[0] + m_viewport[2] - 1, m_viewport[1], m_viewport[1] + m_viewport[3] - 1, -1, 1);
	RenderText(shaderText, "Press P to continue", 25.0f, 25.0f, 0.5f, 0.5f, 0.8f, 0.2f);
	RenderText(shaderText, "PAUSE", 600.0f, 440.0f, 1.0f, 0.3, 0.7f, 0.9f);

	popMatrix(PROJECTION);
	popMatrix(VIEW);
	popMatrix(MODEL);
	glEnable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);
}

void renderGameOverScreen() {
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	int m_viewport[4];
	glGetIntegerv(GL_VIEWPORT, m_viewport);

	pushMatrix(MODEL);
	loadIdentity(MODEL);
	pushMatrix(PROJECTION);
	loadIdentity(PROJECTION);
	pushMatrix(VIEW);
	loadIdentity(VIEW);

	ortho(m_viewport[0], m_viewport[0] + m_viewport[2] - 1, m_viewport[1], m_viewport[1] + m_viewport[3] - 1, -1, 1);
	RenderText(shaderText, "GAME OVER", 550.0f, 440.0f, 1.0f, 0.8f, 0.3f, 0.1f);
	RenderText(shaderText, "Press 'R' to restart", 550.0f, 380.0f, 0.6f, 0.6f, 0.6f, 0.6f);

	popMatrix(PROJECTION);
	popMatrix(VIEW);
	popMatrix(MODEL);

	glEnable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);
}


void renderGameWinScreen() {
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	int m_viewport[4];
	glGetIntegerv(GL_VIEWPORT, m_viewport);

	pushMatrix(MODEL);
	loadIdentity(MODEL);
	pushMatrix(PROJECTION);
	loadIdentity(PROJECTION);
	pushMatrix(VIEW);
	loadIdentity(VIEW);

	ortho(m_viewport[0], m_viewport[0] + m_viewport[2] - 1, m_viewport[1], m_viewport[1] + m_viewport[3] - 1, -1, 1);
	RenderText(shaderText, "You Won!", 550.0f, 440.0f, 1.0f, 0.8f, 0.3f, 0.1f);
	RenderText(shaderText, "Press 'R' to play again", 550.0f, 380.0f, 0.6f, 0.6f, 0.6f, 0.6f);

	popMatrix(PROJECTION);
	popMatrix(VIEW);
	popMatrix(MODEL);

	glEnable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);
}


void renderScene(void) {

	GLint loc;
	GLint m_view[4];

	FrameCount++;
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	// load identity matrices
	loadIdentity(VIEW);
	loadIdentity(MODEL);

	if (activeCam == 3) lookAt(camX, camY, camZ, 0, 0, 0, 0, 1, 0);

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

	glDisable(GL_CULL_FACE);

	// use our shader
	glUseProgram(shader.getProgramIndex());

	// Enable/disable fog
	glUniform1i(glGetUniformLocation(shader.getProgramIndex(), "enableFog"), fogEnabled);

	// Initialize the directional light
	glUniform1i(glGetUniformLocation(shader.getProgramIndex(), "directionalLightstate"), directionalLightState);

	// Initialize the point lights
	for (int i = 0; i < 6; ++i) {
		glUniform1i(glGetUniformLocation(shader.getProgramIndex(), ("pointLight" + std::to_string(i) + "state").c_str()), pointLightState);
	}

	// Initialize the spotlights
	for (int i = 0; i < 2; ++i) {
		glUniform1i(glGetUniformLocation(shader.getProgramIndex(), ("spotLight" + std::to_string(i) + "state").c_str()), spotLightState);
	}

	// Para pointlights
	float res[4];
	for (int i = 0; i < 6; ++i) {
		multMatrixPoint(VIEW, lights[i].position, res);
		glUniform4fv(lights[i].location, 1, res);
	}

	// Para a luz direcional
	multMatrixPoint(VIEW, lights[6].position, res);
	glUniform4fv(lights[6].location, 1, res);

	// Para spotlights
	multMatrixPoint(VIEW, lights[7].position, res);
	glUniform4fv(lights[7].location, 1, res);
	multMatrixPoint(VIEW, lights[8].position, res);
	glUniform4fv(lights[8].location, 1, res);

	// Enviar os estados das luzes
	glUniform1iv(lightStatesLoc, 9, states);

	//Associar os Texture Units aos Objects Texture
	//stone.tga loaded in TU0; checker.tga loaded in TU1;  lightwood.tga loaded in TU2
	/*
	*/
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, TextureArray[0]);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, TextureArray[1]);

	//Indicar aos dois samplers do GLSL quais os Texture Units a serem usados
	glUniform1i(tex_loc, 0);
	glUniform1i(tex_loc1, 1);

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

	rotate(MODEL, -90, 1, 0, 0);

	// send matrices to OGL
	computeDerivedMatrix(PROJ_VIEW_MODEL);
	glUniformMatrix4fv(vm_uniformId, 1, GL_FALSE, mCompMatrix[VIEW_MODEL]);
	glUniformMatrix4fv(pvm_uniformId, 1, GL_FALSE, mCompMatrix[PROJ_VIEW_MODEL]);
	computeNormalMatrix3x3();
	glUniformMatrix3fv(normal_uniformId, 1, GL_FALSE, mNormal3x3);

	// Modulate Phong color with texel color for the terrain
	glUniform1i(texMode_uniformId, 1); // multitexturing

	// Render mesh
	glBindVertexArray(myMeshes[0].vao);

	glDrawElements(myMeshes[0].type, myMeshes[0].numIndexes, GL_UNSIGNED_INT, 0);
	glBindVertexArray(0);

	popMatrix(MODEL);

	// Reset texture
	glUniform1i(texMode_uniformId, 0);

	// Render and transform objects
	renderBuoys(loc);

	// scale(MODEL, 0.5, 1.0, 0.5);
	translate(MODEL, -1.0, 0.0, 12.0);
	renderHouse(loc);
	translate(MODEL, 1.0, 0.0, -12.0);
	// scale(MODEL, -0.5, -1.0, -0.5);

	renderTree(loc);
	renderBoat(loc);
	renderRows(loc);

	renderCreatures(loc);
	renderBalls(loc);
	renderFinishLine(loc);

	// sets the model matrix to a scale matrix so that the model fits in the window
	pushMatrix(MODEL);
	scale(MODEL, scaleFactor*6.0, scaleFactor*6.0, scaleFactor*6.0);
	rotate(MODEL, -90, 1.0, 0.0, 0.0);
	// translate(MODEL, -5.0, 0.0, -5.0);
	aiRecursiveRender(scene->mRootNode, assimpMeshes, textureIds);
	popMatrix(MODEL);

	//Render text (bitmap fonts) in screen coordinates. So use ortoghonal projection with viewport coordinates.
	glDisable(GL_DEPTH_TEST);
	//the glyph contains transparent background colors and non-transparent for the actual character pixels. So we use the blending
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	int m_viewport[4];
	glGetIntegerv(GL_VIEWPORT, m_viewport);

	if (boatGame.state == GAME_ACTIVE) {
		stringstream ss, oss;
		
		ss << boatGame.lives;

		// Formatar o tempo com duas casas decimais
		oss << std::fixed << setprecision(2) << boatGame.playTime;

		pushMatrix(MODEL);
		loadIdentity(MODEL);
		pushMatrix(PROJECTION);
		loadIdentity(PROJECTION);
		pushMatrix(VIEW);
		loadIdentity(VIEW);

		ortho(m_viewport[0], m_viewport[0] + m_viewport[2] - 1, m_viewport[1], m_viewport[1] + m_viewport[3] - 1, -1, 1);
		RenderText(shaderText, "Lives: " + ss.str(), 25.0f, 680.0f, 0.5f, 0.8f, 0.4f, 0.1f);
		RenderText(shaderText, "Time: " + oss.str(), 25.0f, 640.0f, 0.5f, 0.8f, 0.4f, 0.1f);

		popMatrix(PROJECTION);
		popMatrix(VIEW);
		popMatrix(MODEL);
		glEnable(GL_DEPTH_TEST);
		glDisable(GL_BLEND);
	}
	else if (boatGame.state == GAME_PAUSED) {
		renderPauseScreen();
	}
	else if (boatGame.state == GAME_OVER) {
		renderGameOverScreen();
	}

	else if (boatGame.state == GAME_WIN) {
		renderGameWinScreen();
	}

	glutSwapBuffers();
}


// ------------------------------------------------------------
//
// Events from the Keyboard
//

void processKeys(unsigned char key, int xx, int yy)
{
	switch (key) {

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

	case 'F': case 'f':
		printf("fog ativado: %d\n", fogEnabled);
		fogEnabled = !fogEnabled;  // Toggle fog on and off
		glUniform1i(glGetUniformLocation(shader.getProgramIndex(), "enableFog"), fogEnabled);
		break;
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
		if (boat.speed < 0) boat.speed = 0;  // N�o permitir velocidade negativa
		break;

	case 27:
		glutLeaveMainLoop();
		break;

	case 'N': case 'n':	
		// Toggle directional light
		glUseProgram(shader.getProgramIndex());
		directionalLightState = !directionalLightState;
		glUniform1i(glGetUniformLocation(shader.getProgramIndex(), "directionalLightstate"), directionalLightState);
		printf("Directional Light: %s\n", directionalLightState ? "Off" : "On");
		break;

	case 'm': glEnable(GL_MULTISAMPLE); break;


	case 'H': case 'h':
		// Toggle all spot lights
		glUseProgram(shader.getProgramIndex());
		spotLightState = !spotLightState;
		for (int i = 0; i < 2; ++i) {
			glUniform1i(glGetUniformLocation(shader.getProgramIndex(), ("spotLight" + std::to_string(i) + "state").c_str()), spotLightState);
		}
		printf("Spot Lights: %s\n", spotLightState ? "Off" : "On");
		break;

	case 'c': // Toggle all point lights
		glUseProgram(shader.getProgramIndex());
		pointLightState = !pointLightState;
		for (int i = 0; i < 6; ++i) {
			glUniform1i(glGetUniformLocation(shader.getProgramIndex(), ("pointLight" + std::to_string(i) + "state").c_str()), pointLightState);
		}
		break;

	case 'p':
		// Alterna entre pausado e ativo
		if (boatGame.state == GAME_ACTIVE) {
			boatGame.state = GAME_PAUSED;
		}
		else if (boatGame.state == GAME_PAUSED) {
			boatGame.state = GAME_ACTIVE;

			// Reagendar a animação quando o jogo for retomado
			glutTimerFunc(1 / deltaT, animation, 0);
		}
		break;

	case 'r':
		boatGame.lives = 5;
		boat.pos[0] = 20.50f;
		boat.pos[1] = 0.0f;
		boat.pos[2] = -17.86f;
		boat.speed = 0.0f;
		boat.angle = 0.0f;
		
		// Reiniciar jogo
		boatGame.playTime = 0.0;
		boatGame.state = GAME_ACTIVE;
		glutTimerFunc(1 / deltaT, animation, 0);
		break;
	}
}


// ------------------------------------------------------------
//
// Mouse Events
//

void processMouseButtons(int button, int state, int xx, int yy)
{
	// start tracking the mouse
	if (state == GLUT_DOWN) {
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

	deltaX = -xx + startX;
	deltaY = yy - startY;

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

	//camX = rAux * sin((alpha + boat.angle) * 3.14f / 180.0f) * cos(betaAux * 3.14f / 180.0f);
	//camZ = rAux * cos((alphaAux + boat.angle) * 3.14f / 180.0f) * cos(betaAux * 3.14f / 180.0f);
	//camY = rAux * sin(betaAux * 3.14f / 180.0f);

	//  uncomment this if not using an idle or refresh func
	//	glutPostRedisplay();
}


void mouseWheel(int wheel, int direction, int x, int y) {

	r += direction * 0.1f;
	if (r < 0.1f)
		r = 0.1f;

	camX = r * sin(alpha * 3.14f / 180.0f) * cos(beta * 3.14f / 180.0f);
	camZ = r * cos(alpha * 3.14f / 180.0f) * cos(beta * 3.14f / 180.0f);
	camY = r * sin(beta * 3.14f / 180.0f);

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
	glBindFragDataLocation(shader.getProgramIndex(), 0, "colorOut");
	glBindAttribLocation(shader.getProgramIndex(), VERTEX_COORD_ATTRIB, "position");
	glBindAttribLocation(shader.getProgramIndex(), NORMAL_ATTRIB, "normal");
	glBindAttribLocation(shader.getProgramIndex(), TEXTURE_COORD_ATTRIB, "texCoord");
	glBindAttribLocation(shader.getProgramIndex(), TANGENT_ATTRIB, "tangent");	// for normal mapping
	glBindAttribLocation(shader.getProgramIndex(), BITANGENT_ATTRIB, "bitangent");

	glLinkProgram(shader.getProgramIndex());

	printf("InfoLog for Per Fragment Phong Lightning Shader\n%s\n\n", shader.getAllInfoLogs().c_str());

	if (!shader.isProgramValid()) {
		printf("GLSL Model Program Not Valid!\n");
		printf("InfoLog\n%s\n\n", shader.getAllInfoLogs().c_str());
		exit(1);
	}

	texMode_uniformId = glGetUniformLocation(shader.getProgramIndex(), "texMode"); // different modes of texturing
	pvm_uniformId = glGetUniformLocation(shader.getProgramIndex(), "m_pvm");
	vm_uniformId = glGetUniformLocation(shader.getProgramIndex(), "m_viewModel");
	normal_uniformId = glGetUniformLocation(shader.getProgramIndex(), "m_normal");
	
	// Normal mapping
	lPos_uniformId = glGetUniformLocation(shader.getProgramIndex(), "l_pos");
	normalMap_loc = glGetUniformLocation(shader.getProgramIndex(), "normalMap");
	specularMap_loc = glGetUniformLocation(shader.getProgramIndex(), "specularMap");
	diffMapCount_loc = glGetUniformLocation(shader.getProgramIndex(), "diffMapCount");

	// Lights
	lights[0].location = glGetUniformLocation(shader.getProgramIndex(), "pointLight0location");
	lights[1].location = glGetUniformLocation(shader.getProgramIndex(), "pointLight1location");
	lights[2].location = glGetUniformLocation(shader.getProgramIndex(), "pointLight2location");
	lights[3].location = glGetUniformLocation(shader.getProgramIndex(), "pointLight3location");
	lights[4].location = glGetUniformLocation(shader.getProgramIndex(), "pointLight4location");
	lights[5].location = glGetUniformLocation(shader.getProgramIndex(), "pointLight5location");
	lights[6].location = glGetUniformLocation(shader.getProgramIndex(), "directionalLightLocation");
	lights[7].location = glGetUniformLocation(shader.getProgramIndex(), "spotlight0LightLocation");
	lights[8].location = glGetUniformLocation(shader.getProgramIndex(), "spotlight1LightLocation");
	GLint lightStatesLoc = glGetUniformLocation(shader.getProgramIndex(), "lightStates");
	
	tex_loc = glGetUniformLocation(shader.getProgramIndex(), "texmap");
	tex_loc1 = glGetUniformLocation(shader.getProgramIndex(), "texmap1");
	/*
	tex_loc2 = glGetUniformLocation(shader.getProgramIndex(), "texmap2");
	*/

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

MyMesh createBuoys() {
	MyMesh buoyMesh;

	float amb[] = { 0.7f, 0.15f, 0.1f, 1.0f };
	float diff[] = { 1.5f, 0.6f, 1.0f, 1.0f };
	float spec[] = { 1.0f, 1.0f, 0.8f, 1.0f };
	float emissive[] = { 0.0f, 0.0f, 0.0f, 1.0f };
	float shininess = 90.0f;
	int texcount = 0;

	for (int i = 0; i < NUM_BUOYS; i++) {
		buoyMesh = createTorus(0.2, 0.5, 20, 20);
		memcpy(buoyMesh.mat.ambient, amb, 4 * sizeof(float));
		memcpy(buoyMesh.mat.diffuse, diff, 4 * sizeof(float));
		memcpy(buoyMesh.mat.specular, spec, 4 * sizeof(float));
		memcpy(buoyMesh.mat.emissive, emissive, 4 * sizeof(float));
		buoyMesh.mat.shininess = shininess;
		buoyMesh.mat.texCount = texcount;
		myMeshes.push_back(buoyMesh);
	}

	return buoyMesh;
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

MyMesh createFinishLine(float height, float width, float amb, float diff, float spec, float emissive, float shininess, int texcount, MyMesh amesh) {
	
	float amb1[4] = { amb, amb, amb, 1.0f };
	float diff1[4] = { diff, diff, diff, 1.0f };
	float spec1[4] = { spec, spec, spec, 1.0f };
	float emissive1[4] = { emissive, emissive, emissive, 1.0f };

	// Criar o primeiro portão 
	amesh = createQuad(width, height);
	setMaterial(amesh, amb, diff, spec, emissive, shininess, texcount);
	finishLineMeshes.push_back(amesh);

	// Criar o segundo portão 
	amesh = createQuad(width, height);
	setMaterial(amesh, amb, diff, spec, emissive, shininess, texcount);
	finishLineMeshes.push_back(amesh);

	return amesh;
}



void createRows(MyMesh amesh, float baseLength, float amb, float diff, float spec, float emissive, float shininess, int texcount) {
	// Remos direito e esquerdo
	amesh = createCylinder(baseLength * (5.0 / 7.0), baseLength / 25.0, 20);	// cabo
	setMaterial(amesh, amb, diff, spec, emissive, shininess, texcount);
	rowMeshes.push_back(amesh);
	amesh = createQuad(0.5, 0.8);	// p�
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

	float amb[] = { 0.1f, 0.15f, 0.2f, 1.0f };
	float diff[] = { 0.6f, 1.0f, 1.5f, 1.0f };
	float spec[] = { 0.0f, 0.0f, 0.8f, 1.0f };
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

/*
*/
MyMesh createGlassBalls(float radius, int divisions) {
	MyMesh ballMesh;

	// Alpha < 1.0 for transparency
	float amb[] = { 0.5f, 0.0f, 0.0f, 0.5f };
	float diff[] = { 0.0f, 0.0f, 0.5f, 0.2f };
	float spec[] = { 0.0f, 1.0f, 0.0f, 0.3f };
	float emissive[] = { 0.0f, 0.0f, 0.0f, 1.0f };
	float shininess = 100.0f;
	int texcount = 0;

	for (int i = 0; i < NUM_TRANSPARENT_OBJS; i++) {
		ballMesh = createSphere(radius, divisions);
		memcpy(ballMesh.mat.ambient, amb, 4 * sizeof(float));
		memcpy(ballMesh.mat.diffuse, diff, 4 * sizeof(float));
		memcpy(ballMesh.mat.specular, spec, 4 * sizeof(float));
		memcpy(ballMesh.mat.emissive, emissive, 4 * sizeof(float));
		ballMesh.mat.shininess = shininess;
		ballMesh.mat.texCount = texcount;
		ballMeshes.push_back(ballMesh);
	}

	return ballMesh;
}


int init()
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

	/* Cam[2] � implementada no Animation() */

	// original perspective
	cams[3].camPos[0] = 1;
	cams[3].camPos[1] = 1;
	cams[3].camPos[2] = 1;
	//cams[3].type = 0;

	// set the camera position based on its spherical coordinates
	camX = r * sin(alpha * 3.14f / 180.0f) * cos(beta * 3.14f / 180.0f);
	camZ = r * cos(alpha * 3.14f / 180.0f) * cos(beta * 3.14f / 180.0f);
	camY = r * sin(beta * 3.14f / 180.0f);

	//Boat on the start position
	boat.pos[0] = 20.50f;
	boat.pos[1] = 0.0f;
	boat.pos[2] = -17.86f;

	// Texture Object definition
	glGenTextures(2, TextureArray);
	Texture2D_Loader(TextureArray, "just_water.jfif", 0);
	Texture2D_Loader(TextureArray, "wrinkled-paper.jpg", 1);

	float amb[] = { 0.2f, 0.15f, 0.1f, 1.0f };
	float diff[] = { 0.8f, 0.6f, 0.4f, 1.0f };
	float spec[] = { 0.8f, 0.8f, 0.8f, 1.0f };
	float emissive[] = { 0.0f, 0.0f, 0.0f, 1.0f };
	float shininess = 100.0f;
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

	amesh = createGlassBalls(0.5, 20);
	amesh = createBuoys();

	// Calling createTrees with the extracted values
	amesh = createTrees(3.5f, 0.1f, 20, 0.7, 20, amb_value, diff_value, spec_value, emissive_value, shininess, texcount, amesh);

	// Calling createHouses with the extracted values
	amesh = createHouses(3.5f, 50.0f, 50, 0.9, 20, amb_value, diff_value, spec_value, emissive_value, shininess, texcount, amesh);
	// printf("H� %d house meshes\n", houseMeshes.size());

	// Calling createBoat with the extracted values
	amesh = createBoat(1.0f, 2.5f, 0.8f, amb_value, diff_value, spec_value, emissive_value, shininess, texcount, amesh);

	// Create creatures that swim
	amesh = createWaterCreatures();

	//Create gates for the finish line
	amesh = createFinishLine(4.5f, 2.5f, 0.2f, 0.5f, 0.3f, 0.0f, 32.0f, 1, amesh);

	// std::string assimpFilePath = "statue/statue.obj";
	std::string filePath;

	while (true) {
		cout << "Input the directory name containing the OBJ file: ";
		cin >> model_dir;

		std::ostringstream oss;
		oss << model_dir << "/" << model_dir << ".obj";
		filePath = oss.str();   //path of OBJ file in the VS project

		strcat(model_dir, "/");  //directory path in the VS project

		//check if file exists
		ifstream fin(filePath.c_str());
		if (!fin.fail()) {
			fin.close();
			break;
		}
		else
			printf("Couldn't open file: %s\n", filePath.c_str());
	}

	//import 3D file into Assimp scene graph
	if (!Import3DFromFile(filePath, importer, scene, scaleFactor)) {
		printf("Couldn't open file: %s\n", filePath.c_str());
		return(0);
	}

	//creation of Mymesh array with VAO Geometry and Material and array of Texture Objs for the model input by the user
	assimpMeshes = createMeshFromAssimp(scene, textureIds);

	// some GL settings
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glEnable(GL_MULTISAMPLE);
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

	return(1);
}


// ------------------------------------------------------------
//
// Main function
//


int main(int argc, char** argv) {

	//  GLUT initialization
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGBA | GLUT_MULTISAMPLE);

	glutInitContextVersion(4, 3);
	glutInitContextProfile(GLUT_CORE_PROFILE);
	glutInitContextFlags(GLUT_FORWARD_COMPATIBLE | GLUT_DEBUG);

	glutInitWindowPosition(100, 100);
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
	glutMouseWheelFunc(mouseWheel);

	//	return from main loop
	glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_GLUTMAINLOOP_RETURNS);

	//	Init GLEW
	glewExperimental = GL_TRUE;
	glewInit();

	printf("Vendor: %s\n", glGetString(GL_VENDOR));
	printf("Renderer: %s\n", glGetString(GL_RENDERER));
	printf("Version: %s\n", glGetString(GL_VERSION));
	printf("GLSL: %s\n", glGetString(GL_SHADING_LANGUAGE_VERSION));

	// Initialize lights vector
	lights.push_back(Light(positions[0], colors[0], states[0]));
	lights.push_back(Light(positions[1], colors[1], states[1]));
	lights.push_back(Light(positions[2], colors[2], states[2]));
	lights.push_back(Light(positions[3], colors[3], states[3]));
	lights.push_back(Light(positions[4], colors[4], states[4]));
	lights.push_back(Light(positions[5], colors[5], states[5]));
	lights.push_back(Light(positions[6], colors[6], states[6])); // Directional light
	lights.push_back(Spotlight(positions[7], direction[0], cutOffAngle[0], intensity[0], states[7])); // Spotlight 1
	lights.push_back(Spotlight(positions[8], direction[1], cutOffAngle[1], intensity[1], states[8])); // Spotlight 2

	if (!setupShaders())
		return(1);

	// init();
	if (!init())
		printf("Could not Load the Model\n");

	// Iniciar a anima��o
	glutTimerFunc(1 / deltaT, animation, 0);

	//  GLUT main loop
	glutMainLoop();

	return(0);
}


