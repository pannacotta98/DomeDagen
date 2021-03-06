#pragma once
#include <string>
#include <vector>

#include "glm/glm.hpp"
#include "glad/glad.h"

struct Vertex
{
	glm::vec3 mPosition;
	glm::vec3 mNormal;
	glm::vec2 mTexCoords;
};

struct Texture
{
	unsigned mId = 0;
	std::string mType;
	std::string mPath;
};

//This class was written with help of tutorial 
//https://learnopengl.com/Model-Loading/Assimp
class Mesh
{
public:
	//Ctor
	Mesh(std::vector<Vertex> vertices, std::vector<unsigned> indices, std::vector<Texture> textures);

	//Render mesh
	void render() const;
private:
	//Mesh data
	std::vector<Vertex> mVertices;
	std::vector<unsigned> mIndices;
	std::vector<Texture> mTextures;

	//Render handles
	unsigned VAO = 0, VBO = 0, EBO = 0;

	//Upload data to GPU
	void uploadMeshToGPU();
};