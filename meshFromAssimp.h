#ifndef MESH_FROM_ASSIMP_H
#define MESH_FROM_ASSIMP_H

#include <vector>  // Incluir o cabe�alho da STL para o vector
#include <string>
#include <GL/glew.h>  // Exemplo de cabe�alho necess�rio para GLuint
#include <assimp/scene.h>  // Assimp para aiScene
#include <assimp/Importer.hpp>  // Assimp Importer

using namespace std;

// Declara��es das fun��es
bool Import3DFromFile(const std::string& pFile, Assimp::Importer& importer, const aiScene*& sc, float& scaleFactor);
vector<struct MyMesh> createMeshFromAssimp(const aiScene*& sc, GLuint*& textureIds);

#endif
