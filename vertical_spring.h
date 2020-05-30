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
#include <omp.h>

class Spring
{
public:


	float * vertices, *normals;
	//unsigned int* ver_indices;
	int numX;
	int numY;
	float restLen;
	glm::vec3 start;
	std::vector<std::vector<glm::vec3>> pos_, vel_, acc_, v_force_, h_force_, vec_normals_, drag_;
	Spring(int nx,int ny, glm::vec3 start, float l);
	void init();
	void updateForce();
	void update(float dt, int type);
	void updateVertices();
	void updateNormals();
	void collDetection();
	void moveSpring(bool move, bool foward);
	void computeAcc(int i, int j);
	void updateMidpoint(float dt);
	void updateRK4(float dt);
	void updateEulerian(float dt);
	//glm::vec3 accelaration(int i, int j, glm::vec3 x1, glm::vec3 x2, glm::vec3 v1, glm::vec3 v2);
	float xslice;
	float yslice;
	glm::vec3 sphereCenter;
	float sphereRadius;
	float pcd;
	bool move;
	bool forward;
	//std::vector<bool> hit;
	int count;
	bool hasDrag;

private:
	float mass_;
	float k_;
	float kv_;
	glm::vec3 gravity_;
	glm::vec3 wind_;

};


