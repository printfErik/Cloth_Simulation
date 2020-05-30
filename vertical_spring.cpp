#include "vertical_spring.h"
#include <iostream>

Spring::Spring(int nx,int ny, glm::vec3 s, float l)
{
	numX = nx;
	numY = ny;
	start = s;
	restLen = l;

}

void Spring::init()
{
	gravity_ = glm::vec3(0.f, -50.f, 0.f);
	wind_ = glm::vec3(0.f, 0.f, -5.f);
	k_ = 50000;
	kv_ = 1500;
	pcd = -400.f;

	count = 0;
	hasDrag = true;
	move = false;
	forward = true;
	xslice = 1.f / ((float)numX - 1);
	yslice = 1.f / ((float)numY - 1);

	sphereCenter = glm::vec3(0, 0, 0);
	sphereRadius = 0.5f;

	std::vector<std::vector<glm::vec3>> pos(numY, std::vector<glm::vec3>(numX));
	std::vector<std::vector<glm::vec3>> vel(numY, std::vector<glm::vec3>(numX));
	std::vector<std::vector<glm::vec3>> acc(numY, std::vector<glm::vec3>(numX));
	std::vector<std::vector<glm::vec3>> v_force(numY, std::vector<glm::vec3>(numX));
	std::vector<std::vector<glm::vec3>> h_force(numY, std::vector<glm::vec3>(numX));
	std::vector<std::vector<glm::vec3>> vec_normals(numY, std::vector<glm::vec3>(numX));
	std::vector<std::vector<glm::vec3>> drag(numY, std::vector<glm::vec3>(numX));
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


void Spring::updateForce()
{
	glm::vec3 dir_F, dampedF, stringF, unit, rv1, rv2, edge0, edge1, edge2, n1, n2, force1, force2;;
	float length, v1, v2;
	#pragma omp parallel for
	for (int i = 0; i < numY; i++)
	{
		for (int j = 0; j < numX; j++)
		{
			if (i != 0)
			{
				dir_F = pos_[i][j] - pos_[i - 1][j];
				length = glm::length(dir_F);
				unit = glm::normalize(dir_F);
				v1 = glm::dot(dir_F, vel_[i - 1][j]);
				v2 = glm::dot(dir_F, vel_[i][j]);
				dampedF = -(kv_ * (v2 - v1)) * unit;
				stringF = -(k_ * (length - restLen)) * unit;
				v_force_[i][j] = (dampedF + stringF);
			}
			if (j != 0)
			{
				dir_F = pos_[i][j] - pos_[i][j-1];
				length = glm::length(dir_F);
				unit = glm::normalize(dir_F);
				v1 = glm::dot(dir_F, vel_[i][j-1]);
				v2 = glm::dot(dir_F, vel_[i][j]);
				dampedF = -(kv_ * (v2 - v1)) * unit;
				stringF = -(k_ * (length - restLen)) * unit;
				h_force_[i][j] = (dampedF + stringF);
			}
			
		}
		
	}
	if (hasDrag)
	{
		#pragma omp parallel for
		for (int i = 0; i < numY - 1; i++)
		{
			for (int j = 0; j < numX - 1; j++)
			{

				rv1 = (vel_[i][j] + vel_[i + 1][j] + vel_[i + 1][j + 1]) / 3.f - wind_;
				rv2 = (vel_[i][j] + vel_[i][j + 1] + vel_[i + 1][j + 1]) / 3.f - wind_;
				edge0 = pos_[i + 1][j] - pos_[i][j];
				edge1 = pos_[i + 1][j + 1] - pos_[i][j];
				edge2 = pos_[i][j + 1] - pos_[i][j];
				n1 = glm::cross(edge0, edge1);
				n2 = glm::cross(edge1, edge2);

				force1 = pcd * (glm::length(rv1) * glm::dot(rv1, n1)) * n1 / (2.f * glm::length(n1));
				force2 = pcd * (glm::length(rv2) * glm::dot(rv2, n2)) * n2 / (2.f * glm::length(n2));

				drag_[i][j] += (force1 / 3.f + force2 / 3.f);
				drag_[i + 1][j + 1] += (force1 / 3.f + force2 / 3.f);
				drag_[i + 1][j] += force1 / 3.f;
				drag_[i][j + 1] += force2 / 3.f;
			}
		}
	}
	
}

void Spring::update(float dt, int type)
{
	if (type == 0)
	{
		updateMidpoint(dt);
	}
	else if (type == 1)
	{
		updateRK4(dt);
	}
	else
	{
		updateEulerian(dt);
	}
	
}

void Spring::updateMidpoint(float dt)
{
	moveSpring(move, forward);
	std::vector<std::vector<glm::vec3>> init_vel, init_pos; //, k2_vel, k2_pos, k3_vel, k3_pos, k4_vel, k4_pos, init_acc, k2_acc, k3_acc;
	for (int iter = 0; iter < 1; iter++)
	{
		init_vel = vel_;
		init_pos = pos_;

		updateForce();
		#pragma omp parallel for
		for (int i = 1; i < numY; i++)
		{
			for (int j = 0; j < numX; j++)
			{
				computeAcc(i, j);
				drag_[i][j] = glm::vec3();

				pos_[i][j] += (vel_[i][j] * dt * 0.5f);
				vel_[i][j] += (acc_[i][j] * dt * 0.5f);
			}

		}
		collDetection();
		updateForce();
		#pragma omp parallel for
		for (int i = 1; i < numY; i++)
		{
			for (int j = 0; j < numX; j++)
			{
				computeAcc(i, j);
				drag_[i][j] = glm::vec3();
				pos_[i][j] = init_pos[i][j] + (vel_[i][j] * dt);
				vel_[i][j] = init_vel[i][j] + (acc_[i][j] * dt);
			}

		}
		collDetection();
		updateVertices();
		updateNormals();
	}
}


void Spring::updateRK4(float dt)
{
	moveSpring(move, forward);
	std::vector<std::vector<glm::vec3>> init_vel, init_pos, k2_vel, k2_pos, k3_vel, k3_pos, k4_vel, k4_pos, init_acc, k2_acc, k3_acc;
	for (int iter = 0; iter < 1; iter++)
	{
		init_vel = vel_;
		init_pos = pos_;
		updateForce();
		for (int i = 1; i < numY; i++)
		{
			for (int j = 0; j < numX; j++)
			{
				computeAcc(i, j);
				drag_[i][j] = glm::vec3();

				pos_[i][j] += (vel_[i][j] * dt * 0.5f);
				vel_[i][j] += (acc_[i][j] * dt * 0.5f);
			}

		}
		init_acc = acc_;

		collDetection();
		k2_vel = vel_;
		k2_pos = pos_;
		updateForce();

		for (int i = 1; i < numY; i++)
		{
			for (int j = 0; j < numX; j++)
			{
				computeAcc(i, j);
				drag_[i][j] = glm::vec3();
				pos_[i][j] = init_pos[i][j] + (vel_[i][j] * dt * 0.5f);
				vel_[i][j] = init_vel[i][j] + (acc_[i][j] * dt * 0.5f);
			}

		}
		k2_acc = acc_;

		collDetection();
		k3_vel = vel_;
		k3_pos = pos_;
		updateVertices();
		updateNormals();

		updateForce();

		for (int i = 1; i < numY; i++)
		{
			for (int j = 0; j < numX; j++)
			{
				computeAcc(i,j);
				drag_[i][j] = glm::vec3();
				pos_[i][j] = init_pos[i][j] + (vel_[i][j] * dt);
				vel_[i][j] = init_vel[i][j] + (acc_[i][j] * dt);

			}

		}
		k3_acc = acc_;

		collDetection();
		k4_pos = pos_;
		k4_vel = vel_;
		updateForce();
		for (int i = 1; i < numY; i++)
		{
			for (int j = 0; j < numX; j++)
			{
				computeAcc(i,j);
				drag_[i][j] = glm::vec3();
				pos_[i][j] = init_pos[i][j] + dt * ((1.f / 6.f) * init_vel[i][j] + (1.f / 3.f) * k2_vel[i][j] + (1.f / 3.f) * k3_vel[i][j] + (1.f / 6.f) * k4_vel[i][j]);
				vel_[i][j] = init_vel[i][j] + dt * ((1.f / 6.f) * init_acc[i][j] + (1.f / 3.f) * k2_acc[i][j] + (1.f / 3.f) * k3_acc[i][j] + (1.f / 6.f) * acc_[i][j]);
			}

		}
		collDetection();
		updateVertices();
		updateNormals();
	}
}

void Spring::updateEulerian(float dt)
{
	moveSpring(move, forward);
	for (int iter = 0; iter < 1; iter++)
	{
		updateForce();

		
		for (int i = 1; i < numY; i++)
		{
			for (int j = 0; j < numX; j++)
			{
				computeAcc(i, j);
				drag_[i][j] = glm::vec3();

				pos_[i][j] += (vel_[i][j] * dt);
				vel_[i][j] += (acc_[i][j] * dt);
			}

		}
		collDetection();
		updateVertices();
		updateNormals();
	}

}

void Spring::computeAcc(int i, int j)
{
	if (i == (numY - 1))
	{
		if (j == 0)
		{
			acc_[i][j] = (v_force_[i][j] - h_force_[i][j + 1] + drag_[i][j] + gravity_ + wind_);
		}
		else if (j == numX - 1)
		{
			acc_[i][j] = (v_force_[i][j] + h_force_[i][j] + drag_[i][j] + gravity_ + wind_);
		}
		else
		{
			acc_[i][j] = (v_force_[i][j] + h_force_[i][j] - h_force_[i][j + 1] + drag_[i][j] + gravity_ + wind_);
		}

	}
	else
	{
		if (j == 0)
		{
			acc_[i][j] = (v_force_[i][j] - v_force_[i + 1][j] - h_force_[i][j + 1] + drag_[i][j] + gravity_ + wind_);
		}
		else if (j == numX - 1)
		{
			acc_[i][j] = (v_force_[i][j] - v_force_[i + 1][j] + h_force_[i][j] + drag_[i][j] + gravity_ + wind_);
		}
		else
		{
			acc_[i][j] = (v_force_[i][j] - v_force_[i + 1][j] + h_force_[i][j] - h_force_[i][j + 1] + drag_[i][j] + gravity_ + wind_);
		}
	}
}


void Spring::updateVertices()
{
	#pragma omp parallel for
	for (int i = 0; i < numY-1; i++)
	{
		for (int j = 0; j < numX -1; j++)
		{
			int index = (i * (numX-1) + j) * 30;

			// 1st
			// x,y,z
			vertices[index] = pos_[i][j].x;
			vertices[++index] = pos_[i][j].y;
			vertices[++index] = pos_[i][j].z;

			//u,v
			vertices[++index] = xslice * j;
			vertices[++index] = yslice * i;

			// 2nd
			// x,y,z
			vertices[++index] = pos_[i+1][j].x;
			vertices[++index] = pos_[i+1][j].y;
			vertices[++index] = pos_[i+1][j].z;


			//u,v
			vertices[++index] = xslice * j;
			vertices[++index] = yslice * (i + 1);

			// 3rd
			// x,y,z
			vertices[++index] = pos_[i+1][j+1].x;
			vertices[++index] = pos_[i+1][j+1].y;
			vertices[++index] = pos_[i+1][j+1].z;

			//u,v
			vertices[++index] = xslice * (j + 1);
			vertices[++index] = yslice * (i + 1);

			// 4th
			// x,y,z
			vertices[++index] = pos_[i][j].x;
			vertices[++index] = pos_[i][j].y;
			vertices[++index] = pos_[i][j].z;


			//u,v
			vertices[++index] = xslice * j;
			vertices[++index] = yslice * i;

			// 5th
			// x,y,z
			vertices[++index] = pos_[i+1][j+1].x;
			vertices[++index] = pos_[i+1][j+1].y;
			vertices[++index] = pos_[i+1][j+1].z;

			//u,v
			vertices[++index] = xslice * (j + 1);
			vertices[++index] = yslice * (i + 1);

			// 6th
			// x,y,z
			vertices[++index] = pos_[i][j+1].x;
			vertices[++index] = pos_[i][j+1].y;
			vertices[++index] = pos_[i][j+1].z;


			//u,v
			vertices[++index] = xslice * (j + 1);
			vertices[++index] = yslice * i;
		
		}
		
	}


}

void Spring::updateNormals()
{
	glm::vec3 edge0, edge1, edge2, normal1, normal2;
	//#pragma omp parallel for
	for (int i = 0; i < numY - 1; i++)
	{
		for (int j = 0; j < numX - 1; j++)
		{
			edge0 = pos_[i+1][j] - pos_[i][j];
			edge1 = pos_[i + 1][j+1] - pos_[i][j];
			edge2 = pos_[i][j + 1] - pos_[i][j];
			normal1 = glm::cross(edge0, edge1);
			normal2 = glm::cross(edge1, edge2);

			vec_normals_[i][j] += (normal1 + normal2);
			vec_normals_[i+1][j] += normal1;
			vec_normals_[i+1][j+1] += (normal1 + normal2);
			vec_normals_[i][j+1] += normal2;
	
		}
	}

	for (int i = 0; i < numY; i++)
	{
		for (int j = 0; j < numX ; j++)
		{
			vec_normals_[i][j] = glm::normalize(vec_normals_[i][j]);
		}
	}

	//#pragma omp parallel for
	for (int i = 0; i < numY - 1; i++)
	{
		for (int j = 0; j < numX - 1; j++)
		{

			int index = (i * (numX - 1) + j) * 18;

			normals[index] = vec_normals_[i][j].x;
			normals[++index] = vec_normals_[i][j].y;
			normals[++index] = vec_normals_[i][j].z;

			normals[++index] = vec_normals_[i + 1][j].x;
			normals[++index] = vec_normals_[i + 1][j].y;
			normals[++index] = vec_normals_[i + 1][j].z;

			normals[++index] = vec_normals_[i + 1][j + 1].x;
			normals[++index] = vec_normals_[i + 1][j + 1].y;
			normals[++index] = vec_normals_[i + 1][j + 1].z;

			normals[++index] = vec_normals_[i][j].x;
			normals[++index] = vec_normals_[i][j].y;
			normals[++index] = vec_normals_[i][j].z;

			normals[++index] = vec_normals_[i + 1][j + 1].x;
			normals[++index] = vec_normals_[i + 1][j + 1].y;
			normals[++index] = vec_normals_[i + 1][j + 1].z;

			normals[++index] = vec_normals_[i][j + 1].x;
			normals[++index] = vec_normals_[i][j + 1].y;
			normals[++index] = vec_normals_[i][j + 1].z;
		}
	}
	
}

void Spring::collDetection()
{
	float d;
	glm::vec3 n, bounce;
	//#pragma omp parallel for
	for (int i = 0; i < numY; i++) 
	{
		for (int j = 0; j < numX; j++)
		{
			d = glm::length(sphereCenter - pos_[i][j]);
			if (d < sphereRadius + 0.05)
			{
				n = glm::normalize(-1.f * (sphereCenter - pos_[i][j]));
				pos_[i][j] += (0.07f + sphereRadius - d) * n;

				bounce = glm::dot(vel_[i][j], n) * n;
				vel_[i][j] -= 1.5f * bounce;
				count++;
			}
		}
		
	}


}

void Spring::moveSpring(bool move, bool forward)
{
	if (!move) {
		count = 0;
		return;
	}
	for (int j = 0; j < numX; j++)
	{
		if (forward)
		{
			pos_[0][j].z += restLen / 5.f;
			sphereCenter.z += (float)count * 1.f / (float)(numX * numY) * 0.08 / (float)numX;
			
		}
		else
		{
			pos_[0][j].z -= restLen / 5.f;
			sphereCenter.z -= (float)count * 1.f / (float)(numX * numY) * 0.08 / (float)numX;
		}
		
	}
	
}



