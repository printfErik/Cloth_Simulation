#pragma once

#include "glad/glad.h"  //Include order can matter here
#ifdef __APPLE__
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#else
#include <SDL.h>
#include <SDL_opengl.h>
#endif

#define GLM_FORCE_RADIANS
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

#include <cstdio>
#include <vector>
class Cube
{
public:
	float* vertices, * normals;
	//unsigned int* ver_indices;
	int numX;
	int numY;
	float restLen;
	glm::vec3 start;
	std::vector<glm::vec3> pos_, vel_, acc_, v_force_, h_force_, vec_normals_, drag_;
	Cube(glm::vec3 start, float l);
	void updateForce();
	void update(float dt);
	void updateVertices();
	void updateNormals();
	void collDetection();
	//glm::vec3 accelaration(int i, int j, glm::vec3 x1, glm::vec3 x2, glm::vec3 v1, glm::vec3 v2);
	float xslice;
	float yslice;
	glm::vec3 sphereCenter;
	float sphereRadius;
	float pcd;


private:
	float mass_;
	float k_;
	float kv_;
	glm::vec3 gravity_;
	glm::vec3 wind_;


};

