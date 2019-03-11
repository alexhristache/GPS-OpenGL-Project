#define GLEW_STATIC
#define TINYOBJLOADER_IMPLEMENTATION

#include <iostream>
#include "glm/glm.hpp" //core glm functionality
#include "glm/gtc/matrix_transform.hpp" //glm extension for generating common transformation matrices
#include "glm/gtc/matrix_inverse.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "GLEW/glew.h"
#include "GLFW/glfw3.h"
#include <string>
#include "Shader.hpp"
#include "Camera.hpp"
#include "SkyBox.hpp"
#include "Model3D.hpp"
#include "Mesh.hpp"
#include <Windows.h>
#include <mmsystem.h>

int glWindowWidth = 1280;
int glWindowHeight = 720;
GLFWwindow* glWindow = NULL;
int retina_width, retina_height;

std::vector<const GLchar*> faces;
std::vector<const GLchar*> red_faces;
gps::SkyBox mySkyBox;

glm::mat4 model;
glm::mat4 view;
glm::mat4 projection;
glm::mat3 normalMatrix;
glm::mat3 lightDirMatrix;
int chooseLight;
int fog;
GLuint modelLoc;
GLuint viewLoc;
GLuint projectionLoc;
GLuint normalMatrixLoc;
GLuint lightDirMatrixLoc;
GLuint chooseLightLoc;
GLuint fogLoc;

glm::vec3 lightDir;
glm::vec3 lightColor;
glm::vec3 pointLightPosition;
GLuint lightDirLoc;
GLuint lightColorLoc;
GLuint pointLightPositionLoc;

const GLuint SHADOW_WIDTH = 8000, SHADOW_HEIGHT = 8000;
const float LIGHT_SPEED = 2.0f;

bool pressedKeys[1024];
bool firstMouse = true;
GLfloat angle;
GLfloat lightAngle;

gps::Camera myCamera(glm::vec3(0.0f, 1.0f, 2.5f), glm::vec3(0.0f, 0.0f, 0.0f)); //Camera(glm::vec3 cameraPosition, glm::vec3 cameraTarget)
glm::vec3 cameraPos = glm::vec3(0.0f, 2.0f, 5.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
GLfloat cameraSpeed = 0.2f;
GLfloat lastX = 320, lastY = 240;
GLfloat yaw = -90, pitch;

gps::Model3D geodudeModel;
gps::Model3D hoohModel;
gps::Model3D ivysaurModel;
gps::Model3D pokecenterModel;
gps::Model3D pokecenterInsideModel;
gps::Model3D pokeballModel;
gps::Model3D mountain1Model;
gps::Model3D mountain2Model;
gps::Model3D mazeModel;
gps::Model3D pikachuModel;
gps::Model3D articunoModel;
gps::Model3D mewModel;
gps::Model3D charizardModel;
gps::Model3D gastlyModel;
gps::Model3D haunterModel;
gps::Model3D ground;
gps::Model3D lightCube;

gps::Shader myCustomShader;
gps::Shader lightShader;
gps::Shader depthMapShader;
gps::Shader skyboxShader;

GLuint shadowMapFBO;
GLuint depthMapTexture;

bool animations = false;
bool presentation[30];
bool presentation_started = false;
float presentationSpeed = 0.15f;

float pokeballRotation = 0.0f;

GLenum glCheckError_(const char *file, int line) {
	GLenum errorCode;
	while ((errorCode = glGetError()) != GL_NO_ERROR) {
		std::string error;
		switch (errorCode) {
			case GL_INVALID_ENUM:                  error = "INVALID_ENUM"; break;
			case GL_INVALID_VALUE:                 error = "INVALID_VALUE"; break;
			case GL_INVALID_OPERATION:             error = "INVALID_OPERATION"; break;
			case GL_STACK_OVERFLOW:                error = "STACK_OVERFLOW"; break;
			case GL_STACK_UNDERFLOW:               error = "STACK_UNDERFLOW"; break;
			case GL_OUT_OF_MEMORY:                 error = "OUT_OF_MEMORY"; break;
			case GL_INVALID_FRAMEBUFFER_OPERATION: error = "INVALID_FRAMEBUFFER_OPERATION"; break;
		}
		std::cout << error << " | " << file << " (" << line << ")" << std::endl;
	}
	return errorCode;
}

#define glCheckError() glCheckError_(__FILE__, __LINE__)

void windowResizeCallback(GLFWwindow* window, int width, int height) {
	fprintf(stdout, "window resized to width: %d , and height: %d\n", width, height);
	//for RETINA display
	glfwGetFramebufferSize(glWindow, &retina_width, &retina_height);
	myCustomShader.useShaderProgram();
	//set projection matrix
	glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)retina_width / (float)retina_height, 0.1f, 1000.0f);
	//send matrix data to shader
	GLint projLoc = glGetUniformLocation(myCustomShader.shaderProgram, "projection");
	glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
	lightShader.useShaderProgram();
	glUniformMatrix4fv(glGetUniformLocation(lightShader.shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
	//set Viewport transform
	glViewport(0, 0, retina_width, retina_height);
}

void keyboardCallback(GLFWwindow* window, int key, int scancode, int action, int mode) {
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GL_TRUE);
	if (key >= 0 && key < 1024) {
		if (action == GLFW_PRESS)
			pressedKeys[key] = true;
		else if (action == GLFW_RELEASE)
			pressedKeys[key] = false;
	}
}

void mouseCallback(GLFWwindow* window, double xpos, double ypos) {
	
	if (firstMouse) {
		lastX = xpos;
		lastY = ypos;
		firstMouse = false;
	}
	float xoffset = xpos - lastX;
	float yoffset = lastY - ypos;
	lastX = xpos;
	lastY = ypos;
	float sensitivity = 0.05;
	xoffset *= sensitivity;
	yoffset *= sensitivity;
	yaw += xoffset;
	pitch += yoffset;
	if (pitch > 89.0f)
		pitch = 89.0f;
	if (pitch < -89.0f)
		pitch = -89.0f;
	cameraFront.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
	cameraFront.y = sin(glm::radians(pitch));
	cameraFront.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
	cameraFront = glm::normalize(cameraFront);
}

bool between(glm::vec3 v, glm::vec3 b1, glm::vec3 b2)
{
	glm::vec3 l, u;
	l.x = min(b1.x, b2.x);
	l.y = min(b1.y, b2.y);
	l.z = min(b1.z, b2.z);
	u.x = max(b1.x, b2.x);
	u.y = max(b1.y, b2.y);
	u.z = max(b1.z, b2.z);
	
	return (v.x >= l.x && v.x <= u.x) &&
		(v.y >= l.y && v.y <= u.y) &&
		(v.z >= l.z && v.z <= u.z);
}

float turnAround = 0.0f;

void processMovement() {
	if (pressedKeys[GLFW_KEY_Q]) {
		angle += 0.55f;
		if (angle > 360.0f)
			angle -= 360.0f;
	}

	if (pressedKeys[GLFW_KEY_E]) {
		angle -= 0.55f;
		if (angle < 0.0f)
			angle += 360.0f;
	}
	
	if (pressedKeys[GLFW_KEY_W])
	{
		if (between(cameraPos, glm::vec3(-3.0, 0.0, -22.8), glm::vec3(11.5, 10.0, -27.8)) ||
			between(cameraPos, glm::vec3(7.2, 0.0, -24.5), glm::vec3(12.6, 10.0, -47.5))  ||
			between(cameraPos, glm::vec3(12.5, 0.0, -43.5), glm::vec3(-11.4, 10.0, -49.0))||
			between(cameraPos, glm::vec3(-7.2, 0.0, -47.1), glm::vec3(-12.4, 10.0, -24.0))) 
		{
			std::cout << "Collision detected";
		}
		else
		{
			glm::vec3 v = cameraSpeed * cameraFront;
			cameraPos += glm::vec3(v.x, sin(glfwGetTime() * 13) * 0.012f, v.z);
		}
	}

	if (pressedKeys[GLFW_KEY_T])
	{
		if (cameraPos.y <= 2.0f)
			cameraPos += glm::vec3(0, glm::abs(sin(glfwGetTime() * 10) * 0.12f), 0);
		else
			cameraPos -= glm::vec3(0, glm::abs(sin(glfwGetTime() * 10) * 0.12f), 0);
	}

	if (pressedKeys[GLFW_KEY_S])
	{
		glm::vec3 v = cameraSpeed * cameraFront;
		cameraPos -= glm::vec3(v.x, 0.0f, v.z);
	}

	if (pressedKeys[GLFW_KEY_A])
		cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;

	if (pressedKeys[GLFW_KEY_D])
		cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;

	if (pressedKeys[GLFW_KEY_J]) {
		lightAngle += LIGHT_SPEED;
		if (lightAngle > 360.0f)
			lightAngle -= 360.0f;
		glm::vec3 lightDirTr = glm::vec3(glm::rotate(glm::mat4(1.0f), glm::radians(lightAngle), glm::vec3(0.0f, 1.0f, 0.0f)) * glm::vec4(lightDir, 1.0f));
		myCustomShader.useShaderProgram();
		glUniform3fv(lightDirLoc, 1, glm::value_ptr(lightDirTr));
	}

	if (pressedKeys[GLFW_KEY_L]) {
		lightAngle -= LIGHT_SPEED;
		if (lightAngle < 0.0f)
			lightAngle += 360.0f;
		glm::vec3 lightDirTr = glm::vec3(glm::rotate(glm::mat4(1.0f), glm::radians(lightAngle), glm::vec3(0.0f, 1.0f, 0.0f)) * glm::vec4(lightDir, 1.0f));
		myCustomShader.useShaderProgram();
		glUniform3fv(lightDirLoc, 1, glm::value_ptr(lightDirTr));
	}	

	if (pressedKeys[GLFW_KEY_ENTER]) {
		PlaySound(TEXT("pokemon_theme.wav"), NULL, SND_ASYNC | SND_FILENAME | SND_LOOP);
		animations = true;
	}

	if (pressedKeys[GLFW_KEY_N]) {
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	}

	if (pressedKeys[GLFW_KEY_B]) {
		glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
	}

	if (pressedKeys[GLFW_KEY_M]) {
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}

	if (pressedKeys[GLFW_KEY_P]) {
		std::cout << cameraPos.x << " " << cameraPos.y << " " << cameraPos.z << "\n";
		std::cout << "yaw = " << yaw << "\n";
		std::cout << "pitch = " << pitch << "\n";
	}
	
	if (pressedKeys[GLFW_KEY_Y]) {
		presentation[0] = true;
	}

	if (pressedKeys[GLFW_KEY_SPACE]) {
		cameraPos.y += cameraSpeed;
	}

	if (pressedKeys[GLFW_KEY_X]) {
		cameraPos.y -= cameraSpeed;
	}

	if (pressedKeys[GLFW_KEY_1])
	{
		myCustomShader.useShaderProgram();
		chooseLightLoc = glGetUniformLocation(myCustomShader.shaderProgram, "chooseLight");
		glUniform1i(chooseLightLoc, 1);
	}

	if (pressedKeys[GLFW_KEY_2])
	{
		myCustomShader.useShaderProgram();
		chooseLightLoc = glGetUniformLocation(myCustomShader.shaderProgram, "chooseLight");
		glUniform1i(chooseLightLoc, 2);
	}

	if (pressedKeys[GLFW_KEY_3])
	{
		myCustomShader.useShaderProgram();
		chooseLightLoc = glGetUniformLocation(myCustomShader.shaderProgram, "chooseLight");
		glUniform1i(chooseLightLoc, 3);
	}

	if (pressedKeys[GLFW_KEY_4])
	{
		myCustomShader.useShaderProgram();
		fogLoc = glGetUniformLocation(myCustomShader.shaderProgram, "fog");
		glUniform1i(fogLoc, 0);
	}

	if (pressedKeys[GLFW_KEY_5])
	{
		myCustomShader.useShaderProgram();
		fogLoc = glGetUniformLocation(myCustomShader.shaderProgram, "fog");
		glUniform1i(fogLoc, 1);
	}
}

bool initOpenGLWindow() {
	if (!glfwInit()) {
		fprintf(stderr, "ERROR: could not start GLFW3\n");
		return false;
	}
	
	//for Mac OS X
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	glWindow = glfwCreateWindow(glWindowWidth, glWindowHeight, "OpenGL Shader Example", NULL, NULL);
	if (!glWindow) {
		fprintf(stderr, "ERROR: could not open window with GLFW3\n");
		glfwTerminate();
		return false;
	}

	glfwSetWindowSizeCallback(glWindow, windowResizeCallback);
	glfwMakeContextCurrent(glWindow);

	glfwWindowHint(GLFW_SAMPLES, 4);

	// start GLEW extension handler
	glewExperimental = GL_TRUE;
	glewInit();

	// get version info
	const GLubyte* renderer = glGetString(GL_RENDERER); // get renderer string
	const GLubyte* version = glGetString(GL_VERSION); // version as a string
	printf("Renderer: %s\n", renderer);
	printf("OpenGL version supported %s\n", version);

	//for RETINA display
	glfwGetFramebufferSize(glWindow, &retina_width, &retina_height);

	glfwSetKeyCallback(glWindow, keyboardCallback);
	glfwSetCursorPosCallback(glWindow, mouseCallback);
    glfwSetInputMode(glWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	return true;
}

void initOpenGLState() {
	glClearColor(0.3f, 0.3f, 0.3f, 1.0f);
	glViewport(0, 0, retina_width, retina_height);
	glEnable(GL_DEPTH_TEST); // enable depth-testing
	glDepthFunc(GL_LESS); // depth-testing interprets a smaller value as "closer"
	glCullFace(GL_BACK); // cull back face
	glFrontFace(GL_CCW); // GL_CCW for counter clock-wise
}

void initFBOs() {
	//generate FBO ID
	glGenFramebuffers(1, &shadowMapFBO);

	//create depth texture for FBO
	glGenTextures(1, &depthMapTexture);
	glBindTexture(GL_TEXTURE_2D, depthMapTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	//attach texture to FBO
	glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMapTexture, 0);
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

glm::mat4 computeLightSpaceTrMatrix() {
	const GLfloat near_plane = -70.0f, far_plane = 70.0f;
	glm::mat4 lightProjection = glm::ortho(-70.0f, 70.0f, -70.0f, 70.0f, near_plane, far_plane);
	glm::vec3 lightDirTr = glm::vec3(glm::rotate(glm::mat4(1.0f), glm::radians(lightAngle), glm::vec3(0.0f, 1.0f, 0.0f)) * glm::vec4(lightDir, 1.0f));
	glm::mat4 lightView = glm::lookAt(lightDirTr * 1.0f, myCamera.getCameraTarget(), glm::vec3(0.0f, 1.0f, 0.0f));

	return lightProjection * lightView;
}

void initModels() {
	geodudeModel = gps::Model3D("objects/Geodude/Geodude.obj", "objects/Geodude/");
	hoohModel = gps::Model3D("objects/Hooh/Hooh.obj", "objects/Hooh/");
	ivysaurModel = gps::Model3D("objects/Ivysaur/Ivysaur.obj", "objects/Ivysaur/");
	pokecenterModel = gps::Model3D("objects/Pokecenter/Pokecenter.obj", "objects/Pokecenter/");
	pokecenterInsideModel = gps::Model3D("objects/PokecenterInside/PokecenterInside.obj", "objects/PokecenterInside/");
	pokeballModel = gps::Model3D("objects/Pokeball/Pokeball.obj", "objects/Pokeball/");
	mazeModel = gps::Model3D("objects/Maze/Maze.obj", "objects/Maze/");
	pikachuModel = gps::Model3D("objects/Pikachu/Pikachu.obj", "objects/Pikachu/");
	articunoModel = gps::Model3D("objects/Articuno/Articuno.obj", "objects/Articuno/");
	mountain1Model = gps::Model3D("objects/mountains/mountain1.obj", "objects/mountains/");
	mountain2Model = gps::Model3D("objects/mountains/mountain2.obj", "objects/mountains/");
	mewModel = gps::Model3D("objects/Mew/Mew.obj", "objects/Mew/");
	charizardModel = gps::Model3D("objects/Charizard/Charizard.obj", "objects/Charizard/");
	gastlyModel = gps::Model3D("objects/Gastly/Gastly.obj", "objects/Gastly/");
	haunterModel = gps::Model3D("objects/Haunter/Haunter.obj", "objects/Haunter/");
	ground = gps::Model3D("objects/Ground/Ground2.obj", "objects/Ground/");
	lightCube = gps::Model3D("objects/cube/cube.obj", "objects/cube/");

	red_faces.push_back("textures/skybox/right.tga");
	red_faces.push_back("textures/skybox/left.tga");
	red_faces.push_back("textures/skybox/up.tga");
	red_faces.push_back("textures/skybox/down.tga");
	red_faces.push_back("textures/skybox/back.tga");
	red_faces.push_back("textures/skybox/front.tga");
}

void initShaders() {
	myCustomShader.loadShader("shaders/shaderStart.vert", "shaders/shaderStart.frag");
	lightShader.loadShader("shaders/lightCube.vert", "shaders/lightCube.frag");
	depthMapShader.loadShader("shaders/simpleDepthMap.vert", "shaders/simpleDepthMap.frag");
}

void initUniforms() {
	myCustomShader.useShaderProgram();

	modelLoc = glGetUniformLocation(myCustomShader.shaderProgram, "model");
	viewLoc = glGetUniformLocation(myCustomShader.shaderProgram, "view");
	normalMatrixLoc = glGetUniformLocation(myCustomShader.shaderProgram, "normalMatrix");
	lightDirMatrixLoc = glGetUniformLocation(myCustomShader.shaderProgram, "lightDirMatrix");

	projection = glm::perspective(glm::radians(45.0f), (float)retina_width / (float)retina_height, 0.1f, 1000.0f);
	projectionLoc = glGetUniformLocation(myCustomShader.shaderProgram, "projection");
	glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));	

	//set the light direction (direction towards the light)
	lightDir = glm::vec3(0.0f, 1.0f, 2.0f);
	lightDirLoc = glGetUniformLocation(myCustomShader.shaderProgram, "lightDir");
	glUniform3fv(lightDirLoc, 1, glm::value_ptr(lightDir));

	pointLightPosition = glm::vec3(0.0f, 4.0f, -37.0f);
	pointLightPositionLoc = glGetUniformLocation(myCustomShader.shaderProgram, "positionP");
	glUniform3fv(pointLightPositionLoc, 1, glm::value_ptr(pointLightPosition));

	//set light color
	lightColor = glm::vec3(1.0f, 1.0f, 1.0f); //white light
	lightColorLoc = glGetUniformLocation(myCustomShader.shaderProgram, "lightColor");
	glUniform3fv(lightColorLoc, 1, glm::value_ptr(lightColor));

	lightShader.useShaderProgram();
	glUniformMatrix4fv(glGetUniformLocation(lightShader.shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

	mySkyBox.Load(red_faces);

	skyboxShader.loadShader("shaders/skyboxShader.vert", "shaders/skyboxShader.frag");
	skyboxShader.useShaderProgram();

	view = myCamera.getViewMatrix();
	glUniformMatrix4fv(glGetUniformLocation(skyboxShader.shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));

	projection = glm::perspective(glm::radians(45.0f), (float)retina_width / (float)retina_height, 0.1f, 1000.0f);
	glUniformMatrix4fv(glGetUniformLocation(skyboxShader.shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
}


float geodudeFloat = 0.0f;
float geodudeFloat2 = 0.0f;
float articunoFly = 0.0f;
float articunoAngle = 0.0f;
bool articunoDir = true;
float mazeFloat = -5.5f;

void renderScene() {

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	processMovement();	

	if (animations)
	{
		geodudeFloat = sin(glfwGetTime() * 2.0f) * 1.5f;
		geodudeFloat2 = sin(glm::radians(90.0f) + glfwGetTime() * 2.0f) * 1.5f;
	}

	if (presentation[0])
	{
		if (!presentation_started && cameraPos.z > -15.0f)
			cameraPos.z -= presentationSpeed;
		else if (!presentation_started) 
			presentation[1] = true, presentation_started = true;

		if (presentation[1])
		{
			if (cameraPos.x > -5.2f)
				cameraPos.x -= presentationSpeed;
			else presentation[2] = true, presentation[1] = false;
		}
		
		if (presentation[2]) {
			if (cameraPos.z > -26.2f)
				cameraPos.z -= presentationSpeed;
			else presentation[3] = true, presentation[2] = false;
		}
		
		if (presentation[3])
		{
			if (yaw <= -50)
				yaw += presentationSpeed * 2;
			else presentation[4] = true, presentation[3] = false;

			cameraFront.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
			cameraFront.y = sin(glm::radians(pitch));
			cameraFront.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
			cameraFront = glm::normalize(cameraFront);
		}

		if (presentation[4])
		{
			if (cameraPos.z <= -10.8f)
				cameraPos.z += presentationSpeed;
			else presentation[5] = true, presentation[4] = false;
		}

		if (presentation[5])
		{
			if (yaw <= 0)
				yaw += presentationSpeed * 2;
			else presentation[6] = true, presentation[5] = false;

			cameraFront.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
			cameraFront.y = sin(glm::radians(pitch));
			cameraFront.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
			cameraFront = glm::normalize(cameraFront);
		}

		if (presentation[6])
		{
			if (pitch <= 7.5f)
				pitch += presentationSpeed * 2;
			else presentation[7] = true, presentation[6] = false;

			cameraFront.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
			cameraFront.y = sin(glm::radians(pitch));
			cameraFront.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
			cameraFront = glm::normalize(cameraFront);
		}

		if (presentation[7])
		{
			if (cameraPos.x < 10.6f)
				cameraPos.x += presentationSpeed;
			else presentation[8] = true, presentation[7] = false;
		}

		if (presentation[8])
		{
			if (yaw >= -42.0f)
				yaw -= presentationSpeed * 2;
			else if (pitch <= 34.0f)
				pitch += presentationSpeed * 2;
			else presentation[9] = true, presentation[8] = false;

			cameraFront.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
			cameraFront.y = sin(glm::radians(pitch));
			cameraFront.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
			cameraFront = glm::normalize(cameraFront);
		}

		if (presentation[9])
		{
			if (yaw <= 12.0f)
				yaw += presentationSpeed * 2;
			else if (pitch >= 12.0f)
				pitch -= presentationSpeed * 2;
			else presentation[10] = true, presentation[9] = false;

			cameraFront.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
			cameraFront.y = sin(glm::radians(pitch));
			cameraFront.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
			cameraFront = glm::normalize(cameraFront);
		}

		if (presentation[10])
		{
			if (yaw <= 128.0f)
				yaw += presentationSpeed * 2;
			else if (cameraPos.y < 14.0f)
				cameraPos.y += presentationSpeed;
			else if (pitch >= -11.0f)
				pitch -= presentationSpeed * 2;
			else if (cameraPos.x < 29.0f)
				cameraPos.x += presentationSpeed;
			else if (cameraPos.z > -32.0f)
				cameraPos.z -= presentationSpeed;
			else if (lightAngle < 90)
			{
				lightAngle += LIGHT_SPEED;
				if (lightAngle > 360.0f)
					lightAngle -= 360.0f;
				glm::vec3 lightDirTr = glm::vec3(glm::rotate(glm::mat4(1.0f), glm::radians(lightAngle), glm::vec3(0.0f, 1.0f, 0.0f)) * glm::vec4(lightDir, 1.0f));
				myCustomShader.useShaderProgram();
				glUniform3fv(lightDirLoc, 1, glm::value_ptr(lightDirTr));
				animations = true;
			}
			else presentation[11] = true, presentation[10] = false, PlaySound(TEXT("pokemon_theme.wav"), NULL, SND_ASYNC | SND_FILENAME | SND_LOOP);

			cameraFront.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
			cameraFront.y = sin(glm::radians(pitch));
			cameraFront.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
			cameraFront = glm::normalize(cameraFront);
		}

		//std::cout << presentation[0] << " " << presentation[1] << " " << presentation[2] << " " << presentation[3] << "\n";

	}

	//render the scene to the depth buffer (first pass)
	depthMapShader.useShaderProgram();
	glUniformMatrix4fv(glGetUniformLocation(depthMapShader.shaderProgram, "lightSpaceTrMatrix"), 1,  GL_FALSE, glm::value_ptr(computeLightSpaceTrMatrix()));	
	glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
	glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);
	glClear(GL_DEPTH_BUFFER_BIT);
	
	model = glm::rotate(glm::mat4(1.0f), glm::radians(angle), glm::vec3(0, 1, 0)); //create shadow for Geodude
	model = glm::translate(model, glm::vec3(0, geodudeFloat, 0));
	glUniformMatrix4fv(glGetUniformLocation(depthMapShader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model)); 
	geodudeModel.Draw(depthMapShader);

	model = glm::mat4(1.0f);
	model = glm::rotate(model, glm::radians(angle), glm::vec3(0, 1, 0)); 
	model = glm::translate(model, glm::vec3(0, geodudeFloat, 0));
	glUniformMatrix4fv(glGetUniformLocation(depthMapShader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model)); 
	geodudeModel.Draw(depthMapShader);

	model = glm::mat4(1.0f);
	model = glm::rotate(model, glm::radians(angle), glm::vec3(0, 1, 0));
	model = glm::translate(model, glm::vec3(0.0f, geodudeFloat2, -8.0f));
	glUniformMatrix4fv(glGetUniformLocation(depthMapShader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model)); 
	geodudeModel.Draw(depthMapShader);

	model = glm::mat4(1.0f);
	model = glm::rotate(model, glm::radians(angle), glm::vec3(0, 1, 0));
	model = glm::translate(model, glm::vec3(0.0f, geodudeFloat, -16.0f));
	glUniformMatrix4fv(glGetUniformLocation(depthMapShader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model)); 
	geodudeModel.Draw(depthMapShader);

	model = glm::mat4(1.0f);
	model = glm::rotate(model, glm::radians(angle), glm::vec3(0, 1, 0)); 
	model = glm::translate(model, glm::vec3(0, geodudeFloat2, 0));
	model = glm::rotate(model, glm::radians(180.0f), glm::vec3(0, 1, 0)); 
	glUniformMatrix4fv(glGetUniformLocation(depthMapShader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model)); 
	geodudeModel.Draw(depthMapShader);

	model = glm::mat4(1.0f);
	model = glm::rotate(model, glm::radians(angle), glm::vec3(0, 1, 0));
	model = glm::translate(model, glm::vec3(0.0f, geodudeFloat, -8.0f));
	model = glm::rotate(model, glm::radians(180.0f), glm::vec3(0, 1, 0)); 
	glUniformMatrix4fv(glGetUniformLocation(depthMapShader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model)); 
	geodudeModel.Draw(depthMapShader);

	model = glm::mat4(1.0f);
	model = glm::rotate(model, glm::radians(angle), glm::vec3(0, 1, 0));
	model = glm::translate(model, glm::vec3(0.0f, geodudeFloat2, -16.0f));
	model = glm::rotate(model, glm::radians(180.0f), glm::vec3(0, 1, 0)); 
	glUniformMatrix4fv(glGetUniformLocation(depthMapShader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model)); 
	geodudeModel.Draw(depthMapShader);

	model = glm::rotate(glm::mat4(1.0f), glm::radians(angle), glm::vec3(0, 1, 0)); // 
	glUniformMatrix4fv(glGetUniformLocation(depthMapShader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model)); 
	ivysaurModel.Draw(depthMapShader);

	model = glm::rotate(glm::mat4(1.0f), glm::radians(angle), glm::vec3(0, 1, 0)); // 
	glUniformMatrix4fv(glGetUniformLocation(depthMapShader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model)); 
	pokecenterModel.Draw(depthMapShader);

	model = glm::rotate(glm::mat4(1.0f), glm::radians(angle), glm::vec3(0, 1, 0)); // 
	glUniformMatrix4fv(glGetUniformLocation(depthMapShader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model)); 
	//pokecenterInsideModel.Draw(depthMapShader);

	model = glm::rotate(glm::mat4(1.0f), glm::radians(angle), glm::vec3(0, 1, 0)); // 
	glUniformMatrix4fv(glGetUniformLocation(depthMapShader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model)); 
	pikachuModel.Draw(depthMapShader);
	
	model = glm::mat4(1.0f);
	if (articunoDir && animations)
	{
		articunoFly += 0.6f;
		model = glm::translate(model, glm::vec3(articunoFly, 0.0f, 0.0f));
		model = glm::translate(model, glm::vec3(-221.19205f, 13.29072f, -10.79475f));
		model = glm::rotate(model, glm::radians(0.0f), glm::vec3(0, 1, 0));
		model = glm::translate(model, glm::vec3(221.19205f, -13.29072f, 10.79475f));
		if (articunoFly > 350.0f)
			articunoDir = !articunoDir;
	}
	else if (animations)
	{
		articunoFly -= 0.6f;
		model = glm::translate(model, glm::vec3(articunoFly, 0.0f, 0.0f));
		model = glm::translate(model, glm::vec3(-221.19205f, 13.29072f, -10.79475f));
		model = glm::rotate(model, glm::radians(180.0f), glm::vec3(0, 1, 0));
		model = glm::translate(model, glm::vec3(221.19205f, -13.29072f, 10.79475f));

		if (articunoFly < 0.0f)
			articunoDir = !articunoDir;
	}
	glUniformMatrix4fv(glGetUniformLocation(depthMapShader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model)); 
	articunoModel.Draw(depthMapShader);

	model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(0, geodudeFloat - 3.0f, geodudeFloat / 2));
	model = glm::rotate(model, glm::radians(angle), glm::vec3(0, 1, 0));
	glUniformMatrix4fv(glGetUniformLocation(depthMapShader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model)); 
	mewModel.Draw(depthMapShader);

	model = glm::rotate(glm::mat4(1.0f), glm::radians(angle), glm::vec3(0, 1, 0));
	glUniformMatrix4fv(glGetUniformLocation(depthMapShader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model)); 
	//gastlyModel.Draw(depthMapShader);

	model = glm::rotate(glm::mat4(1.0f), glm::radians(angle), glm::vec3(0, 1, 0));
	glUniformMatrix4fv(glGetUniformLocation(depthMapShader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model)); 
	//haunterModel.Draw(depthMapShader);

	model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(0.0f, -1.5f, 0.0f));
	model = glm::rotate(model, glm::radians(angle), glm::vec3(0, 1, 0));
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	glUniformMatrix4fv(glGetUniformLocation(depthMapShader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model)); 
	charizardModel.Draw(depthMapShader);

	model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(0.0f, 15.0f, -30.0f));
	model = glm::scale(model, glm::vec3(0.4f, 0.4f, 0.4f));
	model = glm::rotate(model, glm::radians(180.0f), glm::vec3(0, 1, 0));
	model = glm::rotate(model, glm::radians(-20.0f), glm::vec3(1, 0, 0));
	glUniformMatrix4fv(glGetUniformLocation(depthMapShader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model)); 
	pokeballModel.Draw(depthMapShader);

	model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -1.0f, 0.0f));
	model = glm::rotate(model, glm::radians(angle), glm::vec3(0, 1, 0)); // 
	glUniformMatrix4fv(glGetUniformLocation(depthMapShader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model)); 
	hoohModel.Draw(depthMapShader);

	model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -1.0f, 0.0f)); //create model matrix for ground
	model = glm::rotate(model, glm::radians(angle), glm::vec3(0, 1, 0));
	glUniformMatrix4fv(glGetUniformLocation(depthMapShader.shaderProgram, "model"),  1, GL_FALSE, glm::value_ptr(model)); 
	ground.Draw(depthMapShader);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	//render the scene (second pass)
	myCustomShader.useShaderProgram(); 
	//send lightSpace matrix to shader
	glUniformMatrix4fv(glGetUniformLocation(myCustomShader.shaderProgram, "lightSpaceTrMatrix"), 1, GL_FALSE, glm::value_ptr(computeLightSpaceTrMatrix()));

	view = glm::lookAt(cameraPos, cameraPos + cameraFront, glm::vec3(0.0f, 1.0f, 0.0f));
	glUniformMatrix4fv(glGetUniformLocation(myCustomShader.shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view)); //send view matrix to shader

	lightDirMatrix = glm::mat3(glm::inverseTranspose(view)); //compute light direction transformation matrix
	glUniformMatrix3fv(lightDirMatrixLoc, 1, GL_FALSE, glm::value_ptr(lightDirMatrix)); //send lightDir matrix data to shader

	glViewport(0, 0, retina_width, retina_height);
	myCustomShader.useShaderProgram();
	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, depthMapTexture); //bind the depth map
	glUniform1i(glGetUniformLocation(myCustomShader.shaderProgram, "shadowMap"), 3);
	
	// GEODUDE MODELS //
	model = glm::mat4(1.0f);
	model = glm::rotate(model, glm::radians(angle), glm::vec3(0, 1, 0)); 
	model = glm::translate(model, glm::vec3(0, geodudeFloat, 0));
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model)); //send model matrix data to shader
	normalMatrix = glm::mat3(glm::inverseTranspose(view*model)); //compute normal matrix
	glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix)); //send normal matrix data to shader
	geodudeModel.Draw(myCustomShader);

	model = glm::mat4(1.0f);
	model = glm::rotate(model, glm::radians(angle), glm::vec3(0, 1, 0));
	model = glm::translate(model, glm::vec3(0.0f, geodudeFloat2, -8.0f));
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model)); //send model matrix data to shader
	normalMatrix = glm::mat3(glm::inverseTranspose(view*model)); //compute normal matrix
	glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix)); //send normal matrix data to shader
	geodudeModel.Draw(myCustomShader);

	model = glm::mat4(1.0f);
	model = glm::rotate(model, glm::radians(angle), glm::vec3(0, 1, 0));
	model = glm::translate(model, glm::vec3(0.0f, geodudeFloat, -16.0f));
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model)); //send model matrix data to shader
	normalMatrix = glm::mat3(glm::inverseTranspose(view*model)); //compute normal matrix
	glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix)); //send normal matrix data to shader
	geodudeModel.Draw(myCustomShader);

	model = glm::mat4(1.0f);
	model = glm::rotate(model, glm::radians(angle), glm::vec3(0, 1, 0)); 
	model = glm::translate(model, glm::vec3(0, geodudeFloat2, 0));
	model = glm::rotate(model, glm::radians(180.0f), glm::vec3(0, 1, 0)); 
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model)); //send model matrix data to shader
	normalMatrix = glm::mat3(glm::inverseTranspose(view*model)); //compute normal matrix
	glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix)); //send normal matrix data to shader
	geodudeModel.Draw(myCustomShader);

	model = glm::mat4(1.0f);
	model = glm::rotate(model, glm::radians(angle), glm::vec3(0, 1, 0));
	model = glm::translate(model, glm::vec3(0.0f, geodudeFloat, -8.0f));
	model = glm::rotate(model, glm::radians(180.0f), glm::vec3(0, 1, 0)); 
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model)); //send model matrix data to shader
	normalMatrix = glm::mat3(glm::inverseTranspose(view*model)); //compute normal matrix
	glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix)); //send normal matrix data to shader
	geodudeModel.Draw(myCustomShader);

	model = glm::mat4(1.0f);
	model = glm::rotate(model, glm::radians(angle), glm::vec3(0, 1, 0));
	model = glm::translate(model, glm::vec3(0.0f, geodudeFloat2, -16.0f));
	model = glm::rotate(model, glm::radians(180.0f), glm::vec3(0, 1, 0)); 
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model)); //send model matrix data to shader
	normalMatrix = glm::mat3(glm::inverseTranspose(view*model)); //compute normal matrix
	glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix)); //send normal matrix data to shader
	geodudeModel.Draw(myCustomShader);

	// HOOH MODEL //
	model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(0.0f, -1.0f, 0.0f));
	model = glm::rotate(model, glm::radians(angle), glm::vec3(0, 1, 0)); 
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model)); //send model matrix data to shader
	normalMatrix = glm::mat3(glm::inverseTranspose(view*model)); //compute normal matrix
	glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix)); //send normal matrix data to shader
	hoohModel.Draw(myCustomShader);
	
	// IVYSAUR MODEL //
	model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(0.0f, -1.0f, 0.0f));
	model = glm::rotate(model, glm::radians(angle), glm::vec3(0, 1, 0));
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	normalMatrix = glm::mat3(glm::inverseTranspose(view*model));
	glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
	ivysaurModel.Draw(myCustomShader);

	// POKECENTER MODEL //
	model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(0.0f, -1.0f, 0.0f));
	model = glm::rotate(model, glm::radians(angle), glm::vec3(0, 1, 0)); 
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model)); //send model matrix data to shader
	normalMatrix = glm::mat3(glm::inverseTranspose(view*model)); //compute normal matrix
	glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix)); //send normal matrix data to shader
	pokecenterModel.Draw(myCustomShader);

	// POKECENTERINSIDE MODEL //
	model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(0.0f, -1.0f, 0.0f));
	model = glm::rotate(model, glm::radians(angle), glm::vec3(0, 1, 0)); 
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model)); //send model matrix data to shader
	normalMatrix = glm::mat3(glm::inverseTranspose(view*model)); //compute normal matrix
	glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix)); //send normal matrix data to shader
	pokecenterInsideModel.Draw(myCustomShader);

	// POKEBALL MODEL //
	model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(0.0f, 15.0f, -30.0f));
	model = glm::scale(model, glm::vec3(0.4f, 0.4f, 0.4f));
	model = glm::rotate(model, glm::radians(180.0f), glm::vec3(0, 1, 0));
	model = glm::rotate(model, glm::radians(-20.0f), glm::vec3(1, 0, 0));

	if (animations)
		pokeballRotation += 0.5f;
	model = glm::rotate(model, glm::radians(pokeballRotation), glm::vec3(0, 0, 1));
	model = glm::rotate(model, glm::radians(angle), glm::vec3(0, 1, 0));
	if (pokeballRotation > 360) pokeballRotation = 0;
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model)); //send model matrix data to shader
	normalMatrix = glm::mat3(glm::inverseTranspose(view*model)); //compute normal matrix
	glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix)); //send normal matrix data to shader
	pokeballModel.Draw(myCustomShader);

	// MAZE MODEL //
	model = glm::mat4(1.0f);
	if (animations && mazeFloat < -0.9f)
		mazeFloat += 0.005;
	model = glm::translate(model, glm::vec3(0.0f, mazeFloat, 0.0f));
	model = glm::rotate(model, glm::radians(angle), glm::vec3(0, 1, 0));
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	normalMatrix = glm::mat3(glm::inverseTranspose(view*model));
	glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
	mazeModel.Draw(myCustomShader);

	// PIKACHU MODEL //
	model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(0.0f, -1.5f, 0.0f));
	model = glm::rotate(model, glm::radians(angle), glm::vec3(0, 1, 0));
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	normalMatrix = glm::mat3(glm::inverseTranspose(view*model));
	glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
	pikachuModel.Draw(myCustomShader);
	
	// ARTICUNO MODEL //
	model = glm::mat4(1.0f);

	if (articunoDir && animations)
	{
		articunoFly += 0.6f;
		model = glm::translate(model, glm::vec3(articunoFly, 0.0f, 0.0f));
		model = glm::translate(model, glm::vec3(-221.19205f, 13.29072f, -10.79475f));
		model = glm::rotate(model, glm::radians(0.0f), glm::vec3(0, 1, 0));
		model = glm::translate(model, glm::vec3(221.19205f, -13.29072f, 10.79475f));
		if (articunoFly > 350.0f)
			articunoDir = !articunoDir;
	}
	else if (animations)
	{
		articunoFly -= 0.6f;
		model = glm::translate(model, glm::vec3(articunoFly, 0.0f, 0.0f));
		model = glm::translate(model, glm::vec3(-221.19205f, 13.29072f, -10.79475f));
		model = glm::rotate(model, glm::radians(180.0f), glm::vec3(0, 1, 0));
		model = glm::translate(model, glm::vec3(221.19205f, -13.29072f, 10.79475f));

		if (articunoFly < 0.0f)
			articunoDir = !articunoDir;
	}
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	normalMatrix = glm::mat3(glm::inverseTranspose(view*model));
	glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
	articunoModel.Draw(myCustomShader);

	// MEW MODEL //
	model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(0, geodudeFloat - 3.0f, geodudeFloat/2));
	model = glm::rotate(model, glm::radians(angle), glm::vec3(0, 1, 0));
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	normalMatrix = glm::mat3(glm::inverseTranspose(view*model));
	glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
	mewModel.Draw(myCustomShader);

	// CHARIZARD MODEL //
	model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(0.0f, -1.5f, 0.0f));
	model = glm::rotate(model, glm::radians(angle), glm::vec3(0, 1, 0));
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	normalMatrix = glm::mat3(glm::inverseTranspose(view*model));
	glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
	charizardModel.Draw(myCustomShader);

	// GASTLY MODEL //
	model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(0.0f, -2.0f, 0.0f));
	model = glm::translate(model, glm::vec3(0, geodudeFloat, 0));
	model = glm::rotate(model, glm::radians(angle), glm::vec3(0, 1, 0));
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	normalMatrix = glm::mat3(glm::inverseTranspose(view*model));
	glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
	gastlyModel.Draw(myCustomShader);

	// HAUNTER MODEL //
	model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(0.0f, -2.0f, 0.0f));
	model = glm::translate(model, glm::vec3(0, geodudeFloat, 0));
	model = glm::rotate(model, glm::radians(angle), glm::vec3(0, 1, 0));
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	normalMatrix = glm::mat3(glm::inverseTranspose(view*model));
	glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
	haunterModel.Draw(myCustomShader);

	// MOUNTAINS //
	model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(0.0f, -1.1f, 0.0f));
	model = glm::rotate(model, glm::radians(angle), glm::vec3(0, 1, 0));
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model)); //send model matrix data to shader
	normalMatrix = glm::mat3(glm::inverseTranspose(view*model)); //create normal matrix
	glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix)); //send normal matrix data to shader
	mountain1Model.Draw(myCustomShader);
	mountain2Model.Draw(myCustomShader);

	// GROUND MODEL //
	model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -1.0f, 0.0f)); //create model matrix for ground
	model = glm::rotate(model, glm::radians(angle), glm::vec3(0, 1, 0));
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model)); //send model matrix data to shader
	normalMatrix = glm::mat3(glm::inverseTranspose(view*model)); //create normal matrix
	glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix)); //send normal matrix data to shader
	ground.Draw(myCustomShader);

	lightShader.useShaderProgram(); //draw a white cube around the light
	glUniformMatrix4fv(glGetUniformLocation(lightShader.shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
	model = glm::rotate(glm::mat4(1.0f), glm::radians(lightAngle), glm::vec3(0.0f, 1.0f, 0.0f));
	model = glm::translate(model, lightDir);
	model = glm::scale(model, glm::vec3(0.25f, 0.25f, 0.25f));
	glUniformMatrix4fv(glGetUniformLocation(lightShader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
	lightCube.Draw(lightShader);

	glUniformMatrix4fv(glGetUniformLocation(lightShader.shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
	//model = glm::rotate(glm::mat4(1.0f), glm::radians(lightAngle), glm::vec3(0.0f, 1.0f, 0.0f));
	model = glm::mat4(1.0f);
	model = glm::translate(model, pointLightPosition);
	model = glm::scale(model, glm::vec3(0.25f, 0.25f, 0.25f));
	glUniformMatrix4fv(glGetUniformLocation(lightShader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
	lightCube.Draw(lightShader);

	mySkyBox.Draw(skyboxShader, view, projection);
}

int main(int argc, const char * argv[]) {
	initOpenGLWindow();
	initOpenGLState();
	initFBOs();
	initModels();
	initShaders();
	initUniforms();	
	glCheckError();
	while (!glfwWindowShouldClose(glWindow)) {
		renderScene();
		glfwPollEvents();
		glfwSwapBuffers(glWindow);
	}
	glfwTerminate();
	return 0;
}