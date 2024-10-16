#ifndef MESH_FROM_ASSIMP_H
#define MESH_FROM_ASSIMP_H

#include <vector>  // Incluir o cabeçalho da STL para o vector
#include <string>
#include <GL/glew.h>  // Exemplo de cabeçalho necessário para GLuint
#include <assimp/scene.h>  // Assimp para aiScene
#include <assimp/Importer.hpp>  // Assimp Importer

using namespace std;

// Declarações das funções
bool Import3DFromFile(const std::string& pFile, Assimp::Importer& importer, const aiScene*& sc, float& scaleFactor);
vector<struct MyMesh> createMeshFromAssimp(const aiScene*& sc, GLuint*& textureIds);

#endif
