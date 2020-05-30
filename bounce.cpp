//CSCI 5611 OpenGL Animation Tutorial 
//A 1D bouncing ball

//Running on Mac OSX
//  Download the SDL2 Framework from here: https://www.libsdl.org/download-2.0.php
//  Open the .dmg and move the file SDL2.Framework into the directory /Library/Frameworks/
//  Make sure you place this cpp file in the same directory with the "glad" folder and the "glm" folder
//  g++ Bounce.cpp glad/glad.c -framework OpenGL -framework SDL2; ./a.out

//Running on Windows
//  Download the SDL2 *Development Libararies* from here: https://www.libsdl.org/download-2.0.php
//  Place SDL2.dll, the 3 .lib files, and the include directory in locations known to MSVC
//  Add both Bounce.cpp and glad/glad.c to the project file
//  Compile and run

//Running on Ubuntu
//  sudo apt-get install libsdl2-2.0-0 libsdl2-dev
//  Make sure you place this cpp file in the same directory with the "glad" folder and the "glm" folder
//  g++ Bounce.cpp glad/glad.c -lGL -lSDL; ./a.out

#include "glad/glad.h"  //Include order can matter here

#ifdef __APPLE__
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#else
#include <SDL.h>
#include <SDL_opengl.h>
#endif
#include <cstdio>

#define GLM_FORCE_RADIANS
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

#include <cstdio>
#include "vertical_spring.h"
#include "camera.h"
// Library to load obj files
// https://github.com/syoyo/tinyobjloader

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

#include <iostream>




using namespace std;

bool saveOutput = false; //Make to true to save out your animation
int screen_width = 800;
int screen_height = 600;

GLuint vao[2];
GLuint vbo[3];		

// Shader sources
const GLchar* vertexSource =
"#version 150 core\n"
"in vec3 position;"
"in vec3 inNormal;"
"in vec2 inTexcoord;"
"const vec3 inLightDir = normalize(vec3(0,2,2));"
"out vec3 normal;"
"out vec3 lightDir;"
"out vec2 texcoord;"
"uniform mat4 model;"
"uniform mat4 view;"
"uniform mat4 proj;"
"void main() {"
"   gl_PointSize = 3.0;"
"   gl_Position = proj * view * model * vec4(position,1.0);"
"   vec4 norm4 = transpose(inverse(model)) * vec4(inNormal,1.0);"
"   normal = normalize(norm4.xyz);"
"   lightDir = (view * vec4(inLightDir,0)).xyz;"
"	texcoord = inTexcoord;"
"}";

const GLchar* fragmentSource =
"#version 150 core\n"
"in vec3 normal;"
"in vec3 lightDir;"
"in vec2 texcoord;"
"out vec4 outColor;"
"uniform sampler2D tex0;"
"const float ambient = .2;"
"void main() {"
"	vec3 color = texture(tex0,texcoord).rgb;"
"   vec3 diffuseC = color*max(dot(lightDir,normal),0);"
"   vec3 ambC = color*ambient;"
"	vec3 combined = diffuseC + ambC;"
"   outColor = vec4(combined, 1.0);"
"}";

bool fullscreen = false;
void Win2PPM(int width, int height);
void draw(float dt);

//Index of where to model, view, and projection matricies are stored on the GPU
GLint uniModel, uniView, uniProj;

float aspect; //aspect ratio (needs to be updated if the window is resized)
Spring* spring;
Camera* camera;

int totalXNodes;
int totalYNodes;
int numVertsModel;
int method;

std::vector<tinyobj::real_t> loadModel(const char* filename) {
	std::string inputfile = filename;
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;

	std::string err;
	bool ret =
		tinyobj::LoadObj(&attrib, &shapes, &materials, &err, inputfile.c_str());
	if (!err.empty()) {  // `err` may contain warning message.
		std::cerr << err << std::endl;
	}

	if (!ret) {
		exit(1);
	}

	std::vector<tinyobj::real_t> model;

	// Loop over shapes
	for (size_t s = 0; s < shapes.size(); s++) {
		// Loop over faces(polygon)
		size_t index_offset = 0;
		for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++) {
			unsigned fv = shapes[s].mesh.num_face_vertices[f];
			// Loop over vertices in the face.
			for (size_t v = 0; v < fv; v++) {
				tinyobj::real_t vx, vy, vz, nx, ny, nz, tx, ty;
				// access to vertex
				tinyobj::index_t idx = shapes[s].mesh.indices[index_offset + v];
				vx = attrib.vertices[3 * idx.vertex_index + 0];
				vy = attrib.vertices[3 * idx.vertex_index + 1];
				vz = attrib.vertices[3 * idx.vertex_index + 2];
				if (attrib.normals.size()) {
					nx = attrib.normals[3 * idx.normal_index + 0];
					ny = attrib.normals[3 * idx.normal_index + 1];
					nz = attrib.normals[3 * idx.normal_index + 2];
				}
				else {
					nx = 0.;
					ny = 0.;
					nz = 0.;
				}
				if (attrib.texcoords.size()) {
					tx = attrib.texcoords[2 * idx.texcoord_index + 0];
					ty = attrib.texcoords[2 * idx.texcoord_index + 1];
				}
				else {
					tx = 0.;
					ty = 0.;
				}
				tinyobj::real_t data[8] = { vx, vy, vz, nx, ny, nz, tx, ty };
				model.insert(model.end(), data, data + 8);
			}
			index_offset += fv;
		}
	}
	return model;
}

int main(int argc, char** argv) {


	SDL_Init(SDL_INIT_VIDEO);  //Initialize Graphics (for OpenGL)

	

	//Ask SDL to get a recent version of OpenGL (3.2 or greater)
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);

	//Create a window (offsetx, offsety, width, height, flags)
	SDL_Window* window = SDL_CreateWindow("My OpenGL Program", 0, 0, screen_width, screen_height, SDL_WINDOW_OPENGL);
	aspect = screen_width / (float)screen_height; //aspect ratio (needs to be updated if the window is resized

	//The above window cannot be resized which makes some code slightly easier.
	//Below show how to make a full screen window or allow resizing
	//SDL_Window* window = SDL_CreateWindow("My OpenGL Program", 0, 0, screen_width, screen_height, SDL_WINDOW_FULLSCREEN|SDL_WINDOW_OPENGL);
	//SDL_Window* window = SDL_CreateWindow("My OpenGL Program", 100, 100, screen_width, screen_height, SDL_WINDOW_RESIZABLE|SDL_WINDOW_OPENGL);
	//SDL_Window* window = SDL_CreateWindow("My OpenGL Program",SDL_WINDOWPOS_UNDEFINED,SDL_WINDOWPOS_UNDEFINED,0,0,SDL_WINDOW_FULLSCREEN_DESKTOP|SDL_WINDOW_OPENGL); //Boarderless window "fake" full screen



	//Create a context to draw in
	SDL_GLContext context = SDL_GL_CreateContext(window);

	if (gladLoadGLLoader(SDL_GL_GetProcAddress)) {
		printf("\nOpenGL loaded\n");
		printf("Vendor:   %s\n", glGetString(GL_VENDOR));
		printf("Renderer: %s\n", glGetString(GL_RENDERER));
		printf("Version:  %s\n\n", glGetString(GL_VERSION));
	}
	else {
		printf("ERROR: Failed to initialize OpenGL context.\n");
		return -1;
	}


	SDL_Surface* surface = SDL_LoadBMP("earth.bmp");

	GLuint tex0;
	glGenTextures(1, &tex0);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, tex0);

	// What to do outside 0-1 range
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	// glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	// glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	// Load the texture into memory
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, surface->w, surface->h, 0, GL_BGRA,
		GL_UNSIGNED_BYTE, surface->pixels);
	glGenerateMipmap(GL_TEXTURE_2D);

	SDL_FreeSurface(surface);


	//Build a Vertex Array Object. This stores the VBO and attribute mappings in one object
	glGenVertexArrays(2, vao); //Create a VAO
	glBindVertexArray(vao[0]); //Bind the above created VAO to the current context

	totalXNodes = 30;
	totalYNodes = 30;
	spring = new Spring(totalXNodes, totalYNodes, glm::vec3( 0.5, 1, 1), 0.1f);
	camera = new Camera(glm::vec3(0.f, 0.f, -5.f), 800.f, 600.f);
	spring->init();
	method = 0;

	//Allocate memory on the graphics card to store geometry (vertex buffer object)
	glGenBuffers(3, vbo);  //Create 1 buffer called vbo
	glBindBuffer(GL_ARRAY_BUFFER, vbo[0]); //Set the vbo as the active array buffer (Only one buffer can be active at a time)
	glBufferData(GL_ARRAY_BUFFER, (spring->numX - 1 )* (spring->numY - 1) * 30 * sizeof(float), spring->vertices, GL_STATIC_DRAW); //upload vertices to vbo
	//GL_STATIC_DRAW means we won't change the geometry, GL_DYNAMIC_DRAW = geometry changes infrequently
	//GL_STREAM_DRAW = geom. changes frequently.  This effects which types of GPU memory is used


	//Load the vertex Shader
	GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShader, 1, &vertexSource, NULL);
	glCompileShader(vertexShader);

	//Let's double check the shader compiled 
	GLint status;
	glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &status);
	if (!status) {
		char buffer[512];
		glGetShaderInfoLog(vertexShader, 512, NULL, buffer);
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,
			"Compilation Error",
			"Failed to Compile: Check Consol Output.",
			NULL);
		printf("Vertex Shader Compile Failed. Info:\n\n%s\n", buffer);
	}

	GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShader, 1, &fragmentSource, NULL);
	glCompileShader(fragmentShader);

	//Double check the shader compiled 
	glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &status);
	if (!status) {
		char buffer[512];
		glGetShaderInfoLog(fragmentShader, 512, NULL, buffer);
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,
			"Compilation Error",
			"Failed to Compile: Check Consol Output.",
			NULL);
		printf("Fragment Shader Compile Failed. Info:\n\n%s\n", buffer);
	}

	//Join the vertex and fragment shaders together into one program
	GLuint shaderProgram = glCreateProgram();
	glAttachShader(shaderProgram, vertexShader);
	glAttachShader(shaderProgram, fragmentShader);
	glBindFragDataLocation(shaderProgram, 0, "outColor"); // set output
	glLinkProgram(shaderProgram); //run the linker

	glUseProgram(shaderProgram); //Set the active shader (only one can be used at a time)


	//Tell OpenGL how to set fragment shader input 
	GLint posAttrib = glGetAttribLocation(shaderProgram, "position");
	glVertexAttribPointer(posAttrib, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), 0);
	//Attribute, vals/attrib., type, normalized?, stride, offset
	//Binds to VBO current GL_ARRAY_BUFFER 
	glEnableVertexAttribArray(posAttrib);

	/*
	GLint colAttrib = glGetAttribLocation(shaderProgram, "inColor");
	glVertexAttribPointer(colAttrib, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(colAttrib);
	*/

	GLint texAttrib = glGetAttribLocation(shaderProgram, "inTexcoord");
	glVertexAttribPointer(texAttrib, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(texAttrib);

	glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);
	glBufferData(GL_ARRAY_BUFFER, (spring->numX - 1) * (spring->numY - 1) * 18 *sizeof(float), spring->normals, GL_STATIC_DRAW); //upload normals to vbo
	GLint normAttrib = glGetAttribLocation(shaderProgram, "inNormal");
	glVertexAttribPointer(normAttrib, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(normAttrib);


	//Where to model, view, and projection matricies are stored on the GPU
	uniModel = glGetUniformLocation(shaderProgram, "model");
	uniView = glGetUniformLocation(shaderProgram, "view");
	uniProj = glGetUniformLocation(shaderProgram, "proj");

	std::vector<tinyobj::real_t> model = loadModel("sphere.obj");
	numVertsModel = model.size() / 8;
	int totalNumVerts = numVertsModel;
	std::vector<tinyobj::real_t> modelData;
	modelData.reserve(model.size());
	modelData.insert(modelData.end(), model.begin(), model.end());
	
	glBindVertexArray(vao[1]);
	glBindBuffer(GL_ARRAY_BUFFER, vbo[2]);
	glBufferData(GL_ARRAY_BUFFER, totalNumVerts * 8 * sizeof(float),
		&modelData[0],
		GL_STATIC_DRAW);  // upload vertices to vbo


	glVertexAttribPointer(posAttrib, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), 0);
	glEnableVertexAttribArray(posAttrib);

	glVertexAttribPointer(normAttrib, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float),
		(void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(normAttrib);

	glVertexAttribPointer(texAttrib, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float),
		(void*)(6 * sizeof(float)));
	glEnableVertexAttribArray(texAttrib);

	glBindVertexArray(0);

	glEnable(GL_DEPTH_TEST);
	//Event Loop (Loop forever processing each event as fast as possible)
	SDL_Event windowEvent;

	
	bool quit = false;
	//float lastTime = SDL_GetTicks() / 1000.f;
	float dt = 0;
	int frame = 0;
	unsigned t0 = SDL_GetTicks();
	unsigned t1 = t0;

	while (!quit) {
		while (SDL_PollEvent(&windowEvent)) {
			if (windowEvent.type == SDL_QUIT) quit = true; //Exit event loop
		  //List of keycodes: https://wiki.libsdl.org/SDL_Keycode - You can catch many special keys
		  //Scancode referes to a keyboard position, keycode referes to the letter (e.g., EU keyboards)
			if (windowEvent.type == SDL_KEYUP && windowEvent.key.keysym.sym == SDLK_ESCAPE)
				quit = true; ; //Exit event loop
			if (windowEvent.type == SDL_KEYUP && windowEvent.key.keysym.sym == SDLK_f) //If "f" is pressed
				fullscreen = !fullscreen;
			SDL_SetWindowFullscreen(window, fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0); //Set to full screen 

			// w s a d q e control
			if (windowEvent.key.keysym.sym == SDLK_w && windowEvent.type == SDL_KEYDOWN) camera->positiveMovement.z = 1;
			else if (windowEvent.key.keysym.sym == SDLK_w && windowEvent.type == SDL_KEYUP) camera->positiveMovement.z = 0;

			if (windowEvent.key.keysym.sym == SDLK_s && windowEvent.type == SDL_KEYDOWN) camera->negativeMovement.z = -1;
			else if (windowEvent.key.keysym.sym == SDLK_s && windowEvent.type == SDL_KEYUP) camera->negativeMovement.z = 0;

			if (windowEvent.key.keysym.sym == SDLK_a && windowEvent.type == SDL_KEYDOWN) camera->positiveMovement.x = -1;
			else if (windowEvent.key.keysym.sym == SDLK_a && windowEvent.type == SDL_KEYUP) camera->positiveMovement.x = 0;

			if (windowEvent.key.keysym.sym == SDLK_d && windowEvent.type == SDL_KEYDOWN) camera->negativeMovement.x = 1;
			else if (windowEvent.key.keysym.sym == SDLK_d && windowEvent.type == SDL_KEYUP) camera->negativeMovement.x = 0;

			if (windowEvent.key.keysym.sym == SDLK_q && windowEvent.type == SDL_KEYDOWN) camera->positiveMovement.y = 1;
			else if (windowEvent.key.keysym.sym == SDLK_q && windowEvent.type == SDL_KEYUP) camera->positiveMovement.y = 0;

			if (windowEvent.key.keysym.sym == SDLK_e && windowEvent.type == SDL_KEYDOWN) camera->negativeMovement.y = -1;
			else if (windowEvent.key.keysym.sym == SDLK_e && windowEvent.type == SDL_KEYUP) camera->negativeMovement.y = 0;
			
			// left right up down control
			if (windowEvent.type == SDL_KEYDOWN && windowEvent.key.keysym.sym == SDLK_LEFT) camera->negativeTurn.x = 1;
			else if (windowEvent.type == SDL_KEYUP && windowEvent.key.keysym.sym == SDLK_LEFT) camera->negativeTurn.x = 0;

			if (windowEvent.type == SDL_KEYDOWN && windowEvent.key.keysym.sym == SDLK_RIGHT) camera->positiveTurn.x = -1;
			else if (windowEvent.type == SDL_KEYUP && windowEvent.key.keysym.sym == SDLK_RIGHT) camera->positiveTurn.x = 0;

			if (windowEvent.type == SDL_KEYDOWN && windowEvent.key.keysym.sym == SDLK_UP) camera->positiveTurn.y = -1;
			else if (windowEvent.type == SDL_KEYUP && windowEvent.key.keysym.sym == SDLK_UP) camera->positiveTurn.y = 0;

			if (windowEvent.type == SDL_KEYDOWN && windowEvent.key.keysym.sym == SDLK_DOWN) camera->negativeTurn.y = 1;
			else if (windowEvent.type == SDL_KEYUP && windowEvent.key.keysym.sym == SDLK_DOWN) camera->negativeTurn.y = 0;

			if (windowEvent.type == SDL_KEYDOWN && windowEvent.key.keysym.sym == SDLK_z) spring->sphereCenter.y += 0.03;
			
			if (windowEvent.type == SDL_KEYDOWN && windowEvent.key.keysym.sym == SDLK_x) spring->sphereCenter.y -= 0.03;
			
			if (windowEvent.type == SDL_KEYDOWN && windowEvent.key.keysym.sym == SDLK_c) spring->sphereCenter.x += 0.03;

			if (windowEvent.type == SDL_KEYDOWN && windowEvent.key.keysym.sym == SDLK_v) spring->sphereCenter.x -= 0.03;

			if (windowEvent.type == SDL_KEYDOWN && windowEvent.key.keysym.sym == SDLK_t) spring->sphereCenter.z += 0.03;

			if (windowEvent.type == SDL_KEYDOWN && windowEvent.key.keysym.sym == SDLK_g) spring->sphereCenter.z -= 0.03;

			if (windowEvent.type == SDL_KEYDOWN && windowEvent.key.keysym.sym == SDLK_n)
			{
				spring->move = true;
				spring->forward = true;
			}
			else if (windowEvent.type == SDL_KEYUP && windowEvent.key.keysym.sym == SDLK_n) spring->move = false;

			if (windowEvent.type == SDL_KEYDOWN && windowEvent.key.keysym.sym == SDLK_m) {
				spring->move = true;
				spring->forward = false;
			}
			else if (windowEvent.type == SDL_KEYUP && windowEvent.key.keysym.sym == SDLK_m) spring->move = false;

			if (windowEvent.key.keysym.sym == SDLK_r && windowEvent.type == SDL_KEYDOWN) spring->init();
			if (windowEvent.key.keysym.sym == SDLK_p && windowEvent.type == SDL_KEYDOWN) spring->hasDrag = !spring->hasDrag;
			if (windowEvent.key.keysym.sym == SDLK_1 && windowEvent.type == SDL_KEYDOWN) method = 1;
			if (windowEvent.key.keysym.sym == SDLK_0 && windowEvent.type == SDL_KEYDOWN) method = 0;
			if (windowEvent.key.keysym.sym == SDLK_2 && windowEvent.type == SDL_KEYDOWN) method = 2;
		}

		// Clear the screen to default color
		glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		/*
		if (!saveOutput) dt = (SDL_GetTicks() / 1000.f) - lastTime;
		if (dt > .1) dt = .1; //Have some max dt
		lastTime = SDL_GetTicks() / 1000.f;
		if (saveOutput) dt += (1.0f / 60); //Fix framerate at 14 FPS
		*/
		dt = 0.002;
		draw(dt);
		
		//if (saveOutput) Win2PPM(screen_width, screen_height);


		SDL_GL_SwapWindow(window); //Double buffering


		frame++;
		t1 = SDL_GetTicks();
		if (t1 - t0 > 1000)
		{
			printf("Average Frames Per Second: %.4f\r", frame / ((t1 - t0) / 1000.f));
			fflush(stdout);
			t0 = t1;
			frame = 0;
		}
	}

	//Clean Up
	glDeleteProgram(shaderProgram);
	glDeleteShader(fragmentShader);
	glDeleteShader(vertexShader);

	glDeleteBuffers(3, vbo);
	glDeleteVertexArrays(2, vao);

	SDL_GL_DeleteContext(context);
	SDL_Quit();
	return 0;
}


void draw(float dt) {
	
	spring->update(dt,method);
	
	camera->updateCamera(dt, 800.f, 600.f);
	

	glUniformMatrix4fv(uniProj, 1, GL_FALSE, glm::value_ptr(camera->proj));
	glUniformMatrix4fv(uniView, 1, GL_FALSE, glm::value_ptr(camera->view));
	
	glm::mat4 model = glm::mat4();
		
	glUniformMatrix4fv(uniModel, 1, GL_FALSE, glm::value_ptr(model));

	glBindVertexArray(vao[0]);
	glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);
	glBufferData(GL_ARRAY_BUFFER, (spring->numX - 1) * (spring->numY - 1) * 18 * sizeof(float), spring->normals, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
	glBufferData(GL_ARRAY_BUFFER, (spring->numX - 1) * (spring->numY - 1) * 30 * sizeof(float), spring->vertices, GL_STATIC_DRAW);
	
	glEnable(GL_PROGRAM_POINT_SIZE);

	//glDrawArrays(GL_POINTS, 0, (spring->numX - 1) * (spring->numY - 1) * 6);
	glDrawArrays(GL_TRIANGLES, 0, (spring->numX - 1) * (spring->numY - 1) * 6);//(Primitives, Which VBO, Number of vertices)
	//glDrawArrays(GL_LINE_STRIP, 0, totalXNodes * totalYNodes);


	glBindVertexArray(vao[1]);
	glBindBuffer(GL_ARRAY_BUFFER, vbo[2]);
	model = glm::translate(model, spring->sphereCenter);
	model = glm::scale(model,spring->sphereRadius * glm::vec3(1, 1, 1));
	glUniformMatrix4fv(uniModel, 1, GL_FALSE, glm::value_ptr(model));

	glDrawArrays(GL_TRIANGLES, 0, numVertsModel);
	
}