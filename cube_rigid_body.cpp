#include "cube_rigid_body.h"
#include <iostream>
/*
Cube::Cube(glm::vec3 s, float l)
{
	
	start = s;
	restLen = l;
	gravity_ = glm::vec3(0.f, -80.f, 0.f);
	//wind_ = glm::vec3(0.f, 0.f, -5.f);
	k_ = 40000;
	kv_ = 1000;
	//pcd = -400.f;

	xslice = 1.f / ((float)numX - 1);
	yslice = 1.f / ((float)numY - 1);

	sphereCenter = glm::vec3(0, 0, 0);
	sphereRadius = 0.5f;

	std::vector<glm::vec3> pos(8);
	std::vector<glm::vec3> vel(8);
	vertices = new float[5 * (numX - 1) * (numY - 1) * 2 * 3];
	normals = new float[(numX - 1) * (numY - 1) * 2 * 3 * 3];

	//ver_indices = new unsigned int[(numX - 1) * (numY - 1) * 2 * 3];

	pos_ = pos;
	vel_ = vel;
	acc_ = acc;
	v_force_ = v_force;
	h_force_ = h_force;
	vec_normals_ = vec_normals;
	drag_ = drag;

	pos_[0][0] = start;
	vel_[0][0] = glm::vec3(0, 0, 0);

	//#pragma omp parallel for
	for (int i = 0; i < numY; i++)
	{
		for (int j = 0; j < numX; j++)
		{

			v_force_[i][j] = glm::vec3(0, 0, 0);
			h_force_[i][j] = glm::vec3(0, 0, 0);

			vec_normals_[i][j] = glm::vec3(0, 0, 0);
			drag_[i][j] = glm::vec3(0, 0, 0);

			if (i == 0 && j == 0) { continue; }
			if (i == 0)
			{
				pos_[i][j] = pos_[i][j - 1] - glm::vec3(restLen, 0, 0);
				vel_[i][j] = glm::vec3(0, 0, 0);
			}
			else
			{
				pos_[i][j] = pos_[i - 1][j] - glm::vec3(0, restLen, 0);
				vel_[i][j] = glm::vec3(0, 0, 0);
			}
		}

	}

	for (int i = 0; i < (numX - 1) * (numY - 1) * 30; i++)
	{
		vertices[i] = 0.f;

	}

	for (int i = 0; i < (numX - 1) * (numY - 1) * 18; i++)
	{
		normals[i] = 0.f;
	}

}
*/