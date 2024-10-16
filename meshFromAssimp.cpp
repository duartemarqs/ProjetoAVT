/* --------------------------------------------------
Functions to handle with struct MyMesh based meshes from Assimp meshes:
	- import 3D files to Assimp meshes
	- creation of Mymesh array with VAO/VBO Geometry and Material 
 * it supports 3D files with till 2 diffuse textures, 1 specular map and 1 normal map
 * 
 * Some hints about Assimp:
 * The Importer class forms an C++ interface to the functionality of the Open Asset Import Library.
*Create an object of this class and call ReadFile() to import a file. If the import succeeds, the function returns 
*a pointer to the imported data. The data remains property of the importer object, it is intended to be accessed read-only. 
*The imported data will be destroyed along with the Importer object. If the import fails, ReadFile() returns a NULL pointer. 
*In this case you can retrieve a human-readable error description be calling GetErrorString(). 
 * You can call ReadFile multiple times on a single Assimp::Importer object, but keep in mind 
 *that each invocation will free the previous aiScene. If you really want to stick to aiScene, 
 create a fresh importer object for each scene object and keep it alive (i.e. store a list of (scene, importer) 
 tuples somewhere) as long as needed.

 *So declare an importer object as global in order to persist during whole application execution. Since we use the aiScene object
 *during the RenderScne, it is not possible to use multiple scene objects with the same importer because each time we call importer.Readfile 
 *the previous scene object is replaced with the new one. Thus we need to create an importer object for each scene model.
 *
 João Madeiras Pereira
----------------------------------------------------*/

#include <assert.h>
#include <stdlib.h>
#include <unordered_map>

// assimp include files. These three are usually needed.
#include "assimp/Importer.hpp"	
#include "assimp/postprocess.h"
#include "assimp/scene.h"

#include "AVTmathLib.h"
#include "VertexAttrDef.h"
#include "geometry.h"
#include "Texture_Loader.h"


using namespace std;

/* Directory name containing the current OBJ file. The OBJ filename should be the same*/
extern char model_dir[50];

#define aisgl_min(x,y) (x<y?x:y)
#define aisgl_max(x,y) (y>x?y:x)

void get_bounding_box_for_node(const aiNode* nd, const aiScene* scene, aiVector3D* min, aiVector3D* max)
{
	aiMatrix4x4 prev;
	unsigned int n = 0, t;

	for (; n < nd->mNumMeshes; ++n) {
		const aiMesh* mesh = scene->mMeshes[nd->mMeshes[n]];
		for (t = 0; t < mesh->mNumVertices; ++t) {

			aiVector3D tmp = mesh->mVertices[t];

			min->x = aisgl_min(min->x, tmp.x);
			min->y = aisgl_min(min->y, tmp.y);
			min->z = aisgl_min(min->z, tmp.z);

			max->x = aisgl_max(max->x, tmp.x);
			max->y = aisgl_max(max->y, tmp.y);
			max->z = aisgl_max(max->z, tmp.z);
		}
	}

	for (n = 0; n < nd->mNumChildren; ++n) {
		get_bounding_box_for_node(nd->mChildren[n], scene, min, max);
	}
}


void get_bounding_box(const aiScene* scene, aiVector3D* min, aiVector3D* max)
{

	min->x = min->y = min->z = 1e10f;
	max->x = max->y = max->z = -1e10f;
	get_bounding_box_for_node(scene->mRootNode, scene, min, max);
}

bool Import3DFromFile(const std::string& pFile, Assimp::Importer& importer, const aiScene*& scene, float& scaleFactor)
{
	//const aiScene* scene = NULL;
	
	scene = importer.ReadFile(pFile, aiProcessPreset_TargetRealtime_Quality); 

	// if the import fails, ReadFile() returns a NULL pointer.
	if (!scene)
	{
		printf("%s\n", importer.GetErrorString());
		return false;
	}

	// Now we can access the file's contents.
	printf("Import of scene %s succeeded.\n", pFile.c_str());

	aiVector3D scene_min, scene_max, scene_center;
	get_bounding_box(scene, &scene_min, &scene_max);
	float tmp;
	tmp = scene_max.x - scene_min.x;
	tmp = scene_max.y - scene_min.y > tmp ? scene_max.y - scene_min.y : tmp;
	tmp = scene_max.z - scene_min.z > tmp ? scene_max.z - scene_min.z : tmp;
	scaleFactor = 1.f / tmp;

	// We're done. Everything will be cleaned up by the importer destructor
	return true;
}


bool LoadGLTexturesTUs(const aiScene*& scene, GLuint*& textureIds, unordered_map<std::string, GLuint>& textureIdMap)  // Create OGL textures objects and maps them to texture units.  
{
	aiString path;	// filename
	string filename;

	/* scan scene's materials for textures */
	for (unsigned int m = 0; m < scene->mNumMaterials; ++m)
	{
		// o fragment shader suporta material com duas texturas difusas, 1 especular e 1 normal map
		for (unsigned int i = 0; i < scene->mMaterials[m]->GetTextureCount(aiTextureType_DIFFUSE); i++) {

			scene->mMaterials[m]->GetTexture(aiTextureType_DIFFUSE, i, &path);
			filename = model_dir;
			filename.append(path.data);
			//fill map with textures, OpenGL image ids set to 0
			textureIdMap[filename] = 0;
		}

		for (unsigned int i = 0; i < scene->mMaterials[m]->GetTextureCount(aiTextureType_SPECULAR); i++) {
			scene->mMaterials[m]->GetTexture(aiTextureType_SPECULAR, i, &path);
			filename = model_dir;
			filename.append(path.data);
			textureIdMap[filename] = 0;
		}

		for (unsigned int i = 0; i < scene->mMaterials[m]->GetTextureCount(aiTextureType_NORMALS); i++) {
			scene->mMaterials[m]->GetTexture(aiTextureType_NORMALS, i, &path);
			filename = model_dir;
			filename.append(path.data);
			textureIdMap[filename] = 0;
		}

	}

	int numTextures = textureIdMap.size();
	printf("numeros de mapas %d\n", numTextures);
	if(numTextures) {
		textureIds = new GLuint[numTextures];
		glGenTextures(numTextures, textureIds); /* Texture name generation */

		/* get iterator */
		unordered_map<std::string, GLuint>::iterator itr = textureIdMap.begin();
		filename = (*itr).first;  // get filename

		//create the texture objects array and asssociate them with TU and place the TU in the key value of the map
		for (int i = 0; itr != textureIdMap.end(); ++i, ++itr)
		{
			filename = (*itr).first;  // get filename
			Texture2D_Loader(textureIds, filename.c_str(), i);  //it already performs glBindTexture(GL_TEXTURE_2D, textureIds[i])
			(*itr).second = i;	  // save texture unit for filename in map
			//printf("textura = %s  TU = %d\n", filename.c_str(), i);
		}
	}
	return true;
}

/// Can't send color down as a pointer to aiColor4D because AI colors are ABGR.
//void Color4f(const aiColor4D *color)
//{
//	glColor4f(color->r, color->g, color->b, color->a);
//}

void set_float4(float f[4], float a, float b, float c, float d)
{
	f[0] = a;
	f[1] = b;
	f[2] = c;
	f[3] = d;
}

void color4_to_float4(const aiColor4D* c, float f[4])
{
	f[0] = c->r;
	f[1] = c->g;
	f[2] = c->b;
	f[3] = c->a;
}

vector<struct MyMesh> createMeshFromAssimp(const aiScene*& sc, GLuint*& textureIds) {

	vector<struct MyMesh> myMeshes;

	GLuint buffer;

	// unordered map which maps image filenames to texture units TU. This map is filled in the  LoadGLTexturesTUs()
	unordered_map<std::string, GLuint> textureIdMap;

	printf("Cena: numero total de malhas = %d\n", sc->mNumMeshes);

	LoadGLTexturesTUs(sc, textureIds, textureIdMap); //it creates the unordered map which maps image filenames to texture units TU

	// For each mesh
	for (unsigned int n = 0; n < sc->mNumMeshes; ++n)
	{
		const aiMesh* mesh = sc->mMeshes[n];

		// create array with faces
		// have to convert from Assimp format to array
		unsigned int* faceArray;
		faceArray = (unsigned int*)malloc(sizeof(unsigned int) * mesh->mNumFaces * 3);
		unsigned int faceIndex = 0;

		for (unsigned int t = 0; t < mesh->mNumFaces; ++t) {
			const aiFace* face = &mesh->mFaces[t];

			memcpy(&faceArray[faceIndex], face->mIndices, 3 * sizeof(unsigned int));
			faceIndex += 3;
		}
		struct MyMesh aMesh;
		aMesh.numIndexes = mesh->mNumFaces * 3;
		aMesh.type = GL_TRIANGLES;
		aMesh.mat.texCount = 0;

		// generate Vertex Array for mesh
		glGenVertexArrays(1, &(aMesh.vao));
		glBindVertexArray(aMesh.vao);

		// buffer for faces
		glGenBuffers(1, &buffer);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffer);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * aMesh.numIndexes, faceArray, GL_STATIC_DRAW);

		// buffer for vertex positions
		if (mesh->HasPositions()) {
			glGenBuffers(1, &buffer);
			glBindBuffer(GL_ARRAY_BUFFER, buffer);
			glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 3 * mesh->mNumVertices, mesh->mVertices, GL_STATIC_DRAW);
			glEnableVertexAttribArray(VERTEX_COORD_ATTRIB);
			glVertexAttribPointer(VERTEX_COORD_ATTRIB, 3, GL_FLOAT, 0, 0, 0);
		}

		// buffer for vertex normals
		if (mesh->HasNormals()) {
			glGenBuffers(1, &buffer);
			glBindBuffer(GL_ARRAY_BUFFER, buffer);
			glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 3 * mesh->mNumVertices, mesh->mNormals, GL_STATIC_DRAW);
			glEnableVertexAttribArray(NORMAL_ATTRIB);
			glVertexAttribPointer(NORMAL_ATTRIB, 3, GL_FLOAT, 0, 0, 0);
		}

		// buffers for vertex tangents and bitangents
		if (mesh->HasTangentsAndBitangents()) {
			glGenBuffers(1, &buffer);
			glBindBuffer(GL_ARRAY_BUFFER, buffer);
			glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 3 * mesh->mNumVertices, mesh->mTangents, GL_STATIC_DRAW);
			glEnableVertexAttribArray(TANGENT_ATTRIB);
			glVertexAttribPointer(TANGENT_ATTRIB, 3, GL_FLOAT, 0, 0, 0);

			glGenBuffers(1, &buffer);
			glBindBuffer(GL_ARRAY_BUFFER, buffer);
			glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 3 * mesh->mNumVertices, mesh->mBitangents, GL_STATIC_DRAW);
			glEnableVertexAttribArray(BITANGENT_ATTRIB);
			glVertexAttribPointer(BITANGENT_ATTRIB, 3, GL_FLOAT, 0, 0, 0);
		}

		// buffer for vertex texture coordinates
		if (mesh->HasTextureCoords(0)) {
			float* texCoords = (float*)malloc(sizeof(float) * 2 * mesh->mNumVertices);
			for (unsigned int k = 0; k < mesh->mNumVertices; ++k) {
				texCoords[k * 2] = mesh->mTextureCoords[0][k].x;
				texCoords[k * 2 + 1] = mesh->mTextureCoords[0][k].y;
			}
			glGenBuffers(1, &buffer);
			glBindBuffer(GL_ARRAY_BUFFER, buffer);
			glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 2 * mesh->mNumVertices, texCoords, GL_STATIC_DRAW);
			glEnableVertexAttribArray(TEXTURE_COORD_ATTRIB);
			glVertexAttribPointer(TEXTURE_COORD_ATTRIB, 2, GL_FLOAT, 0, 0, 0);
		}

		// unbind buffers
		glBindVertexArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

		// create material; each mesh has ONE material
		aiMaterial* mtl = sc->mMaterials[mesh->mMaterialIndex];

		aiString texPath;	//contains filename of texture

		string filename;
		GLuint TU;
		unsigned int TUcount = 0;

		for (unsigned int i = 0; i < mtl->GetTextureCount(aiTextureType_DIFFUSE); i++) {

			mtl->GetTexture(aiTextureType_DIFFUSE, i, &texPath);
			filename = model_dir;
			filename.append(texPath.data);
			TU = textureIdMap[filename];

			aMesh.texUnits[TUcount] = TU;
			aMesh.texTypes[TUcount] = DIFFUSE;
			aMesh.mat.texCount = TUcount + 1;
			TUcount++;
		}

		for (unsigned int i = 0; i < mtl->GetTextureCount(aiTextureType_SPECULAR); i++) {
			mtl->GetTexture(aiTextureType_SPECULAR, i, &texPath);
			filename = model_dir;
			filename.append(texPath.data);
			TU = textureIdMap[filename];

			aMesh.texUnits[TUcount] = TU;
			aMesh.texTypes[TUcount] = SPECULAR;
			aMesh.mat.texCount = TUcount + 1;
			TUcount++;
		}
		for (unsigned int i = 0; i < mtl->GetTextureCount(aiTextureType_NORMALS); i++) {
			mtl->GetTexture(aiTextureType_NORMALS, i, &texPath);
			filename = model_dir;
			filename.append(texPath.data);
			TU = textureIdMap[filename];

			aMesh.texUnits[TUcount] = TU;
			aMesh.texTypes[TUcount] = NORMALS;
			aMesh.mat.texCount = TUcount + 1;
			TUcount++;
		}

	
		float c[4];
		set_float4(c, 0.8f, 0.8f, 0.8f, 1.0f);
		aiColor4D diffuse;
		if (AI_SUCCESS == aiGetMaterialColor(mtl, AI_MATKEY_COLOR_DIFFUSE, &diffuse))
			color4_to_float4(&diffuse, c);
		memcpy(aMesh.mat.diffuse, c, sizeof(c));

		set_float4(c, 0.2f, 0.2f, 0.2f, 1.0f);
		aiColor4D ambient;
		if (AI_SUCCESS == aiGetMaterialColor(mtl, AI_MATKEY_COLOR_AMBIENT, &ambient))
			color4_to_float4(&ambient, c);
		memcpy(aMesh.mat.ambient, c, sizeof(c));

		set_float4(c, 0.0f, 0.0f, 0.0f, 1.0f);
		aiColor4D specular;
		if (AI_SUCCESS == aiGetMaterialColor(mtl, AI_MATKEY_COLOR_SPECULAR, &specular))
			color4_to_float4(&specular, c);
		memcpy(aMesh.mat.specular, c, sizeof(c));

		set_float4(c, 0.0f, 0.0f, 0.0f, 1.0f);
		aiColor4D emission;
		if (AI_SUCCESS == aiGetMaterialColor(mtl, AI_MATKEY_COLOR_EMISSIVE, &emission))
			color4_to_float4(&emission, c);
		memcpy(aMesh.mat.emissive, c, sizeof(c));

		float shininess = 0.0;
		unsigned int max;
		aiGetMaterialFloatArray(mtl, AI_MATKEY_SHININESS, &shininess, &max);
		aMesh.mat.shininess = shininess;

		myMeshes.push_back(aMesh);
	}
	// cleaning up
	textureIdMap.clear();
	
	return(myMeshes);
}